"""Execute January and freshly rebuilt countdown-timer increment bodies."""

from contextlib import contextmanager
import faulthandler
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

import pytest

from tools import coff_compare as cc
from tools.regression_gate import _section_aux_selection


ROOT = Path(__file__).resolve().parents[1]
UNIT = "source/networking/network_server_manager"
SOURCE = ROOT / (UNIT + ".c")
TARGET = ROOT / ("build/split/" + UNIT + ".obj")
COMPILER = ROOT / "xbox/bin/vc7/CL.Exe"
FUNCTION = "_countdown_timer_increment"

SYSTEM_MILLISECONDS = "_system_milliseconds"
DISPLAY_ASSERT = "_display_assert"
SYSTEM_EXIT = "_system_exit"
FILE_LITERAL = "??_C@_0DD@CFCGIJJL@c?3?2halo?2SOURCE?2networking?2networ@"
ADJUSTMENT_LITERAL = "??_C@_0BA@LLGNDOEI@adjustment?5?$DO?$DN?50?$AA@"
TIMER_LITERAL = "??_C@_0BL@DEKPBLAE@timer?9?$DOtime_remaining?5?$DO?$DN?50?$AA@"
LITERALS = {
    FILE_LITERAL: b"c:\\halo\\SOURCE\\networking\\network_server_manager.c\0",
    ADJUSTMENT_LITERAL: b"adjustment >= 0\0",
    TIMER_LITERAL: b"timer->time_remaining >= 0\0",
}

TARGET_SPEC = (
    144,
    "0291de1fcc1f65baf92a784ad590c78925729c9c4ba0b9a1bbf33a6e8cdd7aa5",
    [
        (0x05, 20, SYSTEM_MILLISECONDS),
        (0x39, 6, FILE_LITERAL),
        (0x3E, 6, ADJUSTMENT_LITERAL),
        (0x43, 20, DISPLAY_ASSERT),
        (0x4A, 20, SYSTEM_EXIT),
        (0x76, 6, FILE_LITERAL),
        (0x7B, 6, TIMER_LITERAL),
        (0x80, 20, DISPLAY_ASSERT),
        (0x87, 20, SYSTEM_EXIT),
    ],
)
REBUILT_SPEC = (
    144,
    "96937b1e7ff0aa3415fa0b0a397feddc2b30699e3d34b7be1db7f80b3eb96485",
    [
        (0x05, 20, SYSTEM_MILLISECONDS),
        (0x37, 6, FILE_LITERAL),
        (0x3C, 6, ADJUSTMENT_LITERAL),
        (0x41, 20, DISPLAY_ASSERT),
        (0x48, 20, SYSTEM_EXIT),
        (0x74, 6, FILE_LITERAL),
        (0x79, 6, TIMER_LITERAL),
        (0x7E, 20, DISPLAY_ASSERT),
        (0x85, 20, SYSTEM_EXIT),
    ],
)

PAGE = 0x1000
CODE = 0x00100000
STOP = CODE + 0x0F00
API_PAGE = 0x00200000
APIS = {
    SYSTEM_MILLISECONDS: API_PAGE + 0x100,
    DISPLAY_ASSERT: API_PAGE + 0x200,
    SYSTEM_EXIT: API_PAGE + 0x300,
}
DATA_PAGE = 0x00300000
LITERAL_ADDRESSES = {
    FILE_LITERAL: DATA_PAGE + 0x100,
    ADJUSTMENT_LITERAL: DATA_PAGE + 0x200,
    TIMER_LITERAL: DATA_PAGE + 0x300,
}
TIMER_PAGE = 0x00400000
TIMER = TIMER_PAGE + 0x800
STACK_PAGE = 0x00500000
STACK_SIZE = 0x20000
ENTRY_ESP = STACK_PAGE + 0x10000
SAVED_REGISTERS = {
    "EBP": 0x1B1B1B1B,
    "EBX": 0x2B2B2B2B,
    "ESI": 0x3B3B3B3B,
    "EDI": 0x4B4B4B4B,
}


class RuntimeSemanticMismatch(AssertionError):
    """Raised only after two successfully executed bodies produce different results."""


@contextmanager
def quiet_unicorn_windows_exceptions():
    """Hide only Unicorn's internally handled Windows mapping probes."""
    enabled = faulthandler.is_enabled()
    if enabled:
        faulthandler.disable()
    try:
        yield
    finally:
        if enabled:
            faulthandler.enable()


def u32(value):
    return value & 0xFFFFFFFF


def s32(value):
    value = u32(value)
    return value - 0x100000000 if value & 0x80000000 else value


def relocation_name(relocation):
    target = relocation.get("symbolic_target", relocation["target"])
    assert len(target) == 3 and target[0] == "symbol" and target[2] == 0, relocation
    return target[1]


def verify_literal_contracts(obj):
    for name, payload in LITERALS.items():
        owners = [symbol for symbol in obj["symbols"]
                  if symbol["name"] == name and symbol["section"] > 0]
        assert len(owners) == 1, (name, owners)
        owner = owners[0]
        section = obj["sections"][owner["section"] - 1]
        assert (owner["value"], owner["type"], owner["storage"]) == (0, 0, 2)
        assert section["name"] == ".rdata"
        assert section["flags"] == 0x40301040
        assert section["reloc_count"] == 0
        assert _section_aux_selection(obj, section["index"]) == 2
        assert bytes(cc._section_bytes(obj, section)) == payload


def prepare_body(obj, spec):
    expected_size, expected_sha, expected_relocations = spec
    owner = cc.symbol(obj, FUNCTION)
    section = obj["sections"][owner["section"] - 1]
    info = cc.section_info(obj, FUNCTION)
    assert (owner["value"], owner["type"], owner["storage"]) == (0, 0x20, 2)
    assert section["name"] == ".text" and section["flags"] == 0x60501020
    assert _section_aux_selection(obj, section["index"]) == 1
    assert (info["size"], info["relocation_count"], info["normalized_sha256"]) == (
        expected_size, len(expected_relocations), expected_sha)
    actual_relocations = [
        (relocation["address"], relocation["type"], relocation_name(relocation))
        for relocation in info["relocations"]
    ]
    assert actual_relocations == expected_relocations
    verify_literal_contracts(obj)

    raw = bytearray(cc._section_bytes(obj, section))
    for offset, kind, name in actual_relocations:
        assert struct.unpack_from("<I", raw, offset)[0] == 0
        if kind == cc.IMAGE_REL_I386_REL32:
            assert name in APIS
            value = APIS[name] - (CODE + offset + 4)
        else:
            assert kind == cc.IMAGE_REL_I386_DIR32 and name in LITERAL_ADDRESSES
            value = LITERAL_ADDRESSES[name]
        struct.pack_into("<I", raw, offset, u32(value))
    return bytes(raw)


@pytest.fixture(scope="module")
def bodies(tmp_path_factory):
    pytest.importorskip("unicorn")
    if not TARGET.is_file() or not COMPILER.is_file():
        pytest.skip("locally supplied January target/VC7 compiler is unavailable")

    source_before = SOURCE.read_bytes()
    normalized_source = source_before.replace(b"\r\n", b"\n")
    update_start = normalized_source.index(b"unsigned long countdown_timer_update(")
    update_end = normalized_source.index(b"long countdown_timer_get_time_remaining(", update_start)
    update_body = normalized_source[update_start:update_end]
    update_tokens = b" ".join(update_body.split())
    assert update_tokens.count(
        b"timer->time_remaining = (long)( (unsigned long)timer->time_remaining "
        b"- (unsigned long)elapsed_time);") == 1
    assert b"timer->time_remaining -= elapsed_time;" not in update_body
    increment_start = normalized_source.rindex(b"void countdown_timer_increment(")
    increment_end = normalized_source.index(b"void countdown_timer_decrement(", increment_start)
    increment_body = normalized_source[increment_start:increment_end]
    assert b" ".join(increment_body.split()).count(
        b"(unsigned long)timer->time_remaining + (unsigned long)adjustment") == 1

    output = tmp_path_factory.mktemp("countdown-timer-increment") / "rebuilt.obj"
    completed = subprocess.run(
        [sys.executable, "-B", "tools/campaign/gate.py", UNIT,
         "--fn", FUNCTION, "--out", str(output)],
        cwd=ROOT,
        env=dict(os.environ, HALO_CL=str(COMPILER), PYTHONDONTWRITEBYTECODE="1"),
        capture_output=True,
        text=True,
    )
    assert completed.returncode == 0, completed.stdout + completed.stderr
    assert output.is_file() and output.stat().st_size > 0
    assert SOURCE.read_bytes() == source_before, "canonical source changed during fixture"

    return {
        "target": prepare_body(cc.load(TARGET), TARGET_SPEC),
        "rebuilt": prepare_body(cc.load(output), REBUILT_SPEC),
    }


def make_case(label, remaining, last, now, adjustment, maximum):
    return {
        "label": label,
        "remaining": u32(remaining),
        "last": u32(last),
        "now": u32(now),
        "adjustment": u32(adjustment),
        "maximum": u32(maximum),
    }


def cases():
    result = [
        make_case("ordinary_no_clock_progress", 30_000, 100, 100, 5_000, 30_999),
        make_case("ordinary_forward_elapsed", 100, 100, 110, 5, 1_000),
        make_case("elapsed_exhausts_timer", 5, 100, 110, 5, 1_000),
        make_case("elapsed_equals_timer", 10, 100, 110, 0, 1_000),
        make_case("positive_to_negative_clock_sign_transition", 100,
                  0x7FFFFFFF, 0x80000000, 0, 1_000),
        make_case("negative_half_clock_forward", 100,
                  0x80000000, 0x80000001, 0, 1_000),
        make_case("tickcount_full_wrap", 100, 0xFFFFFFFF, 0, 0, 1_000),
        make_case("negative_elapsed_path", 100,
                  0x80000000, 0x7FFFFFFF, 0, 1_000),
        make_case("negative_elapsed_subtraction_wrap", 0x7FFFFFFF,
                  0x80000000, 0x7FFFFFFF, 0, 0x7FFFFFFF),
        make_case("increment_sign_overflow_clamps", 0x7FFFFFFF, 1, 1, 1, 30_999),
        make_case("increment_clamps_to_maximum", 29_000, 1, 1, 5_000, 30_999),
        make_case("largest_nonoverflow_increment", 0, 1, 1,
                  0x7FFFFFFF, 0x7FFFFFFF),
        make_case("negative_adjustment_asserts_after_update", 100, 100, 101, -1, 1_000),
        make_case("negative_maximum_final_assert", 0, 1, 1, 0, -1),
        make_case("negative_maximum_overflow_final_assert", 0x7FFFFFFF, 1, 1, 1, -1),
        make_case("initial_negative_remaining_wraps", -1, 1, 1, 1, 123),
        make_case("positive_clock_to_minus_one_skips", 100, 0, 0xFFFFFFFF, 0, 1_000),
    ]
    generator = random.Random(0x11B660)
    for index in range(256):
        result.append(make_case(
            f"random_valid_{index:03d}",
            generator.randrange(0x80000000),
            generator.randrange(0x100000000),
            generator.randrange(0x100000000),
            generator.randrange(0x80000000),
            generator.randrange(0x80000000),
        ))
    for index in range(32):
        result.append(make_case(
            f"random_negative_adjustment_{index:03d}",
            generator.randrange(0x80000000),
            generator.randrange(0x100000000),
            generator.randrange(0x100000000),
            generator.randrange(0x80000000, 0x100000000),
            generator.randrange(0x80000000),
        ))
    for index in range(32):
        result.append(make_case(
            f"random_negative_maximum_{index:03d}",
            generator.randrange(0x80000000),
            generator.randrange(0x100000000),
            generator.randrange(0x100000000),
            generator.randrange(0x80000000),
            generator.randrange(0x80000000, 0x100000000),
        ))
    assert len(result) == 337
    return result


def range_contains(base, length, address, size):
    return size > 0 and base <= address and address + size <= base + length


def ranges_overlap(first_base, first_length, second_base, second_length):
    return first_base < second_base + second_length and second_base < first_base + first_length


def execute(code, item):
    # Unicorn can raise and internally handle Windows mapping exceptions while
    # constructing the machine as well as while running it.
    with quiet_unicorn_windows_exceptions():
        return _execute(code, item)


def _execute(code, item):
    import unicorn as u
    from unicorn import x86_const as x

    cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    for base, length in (
        (CODE, PAGE), (API_PAGE, PAGE), (DATA_PAGE, PAGE),
        (TIMER_PAGE, PAGE), (STACK_PAGE, STACK_SIZE),
    ):
        cpu.mem_map(base, length)
    cpu.mem_write(CODE, code)
    for address in APIS.values():
        cpu.mem_write(address, b"\xC3")
    for name, address in LITERAL_ADDRESSES.items():
        cpu.mem_write(address, LITERALS[name])

    timer_before = bytes([0xA5]) * PAGE
    cpu.mem_write(TIMER_PAGE, timer_before)
    cpu.mem_write(TIMER, struct.pack("<II", item["remaining"], item["last"]))
    frame = struct.pack(
        "<IIII", STOP, TIMER, item["adjustment"], item["maximum"])
    cpu.mem_write(STACK_PAGE, bytes([0x5A]) * STACK_SIZE)
    cpu.mem_write(ENTRY_ESP, frame)

    register_ids = {
        "EBP": x.UC_X86_REG_EBP,
        "EBX": x.UC_X86_REG_EBX,
        "ESI": x.UC_X86_REG_ESI,
        "EDI": x.UC_X86_REG_EDI,
    }
    for name, value in SAVED_REGISTERS.items():
        cpu.reg_write(register_ids[name], value)
    cpu.reg_write(x.UC_X86_REG_EAX, 0xA0A0A0A0)
    cpu.reg_write(x.UC_X86_REG_ECX, 0xC0C0C0C0)
    cpu.reg_write(x.UC_X86_REG_EDX, 0xD0D0D0D0)
    cpu.reg_write(x.UC_X86_REG_EFLAGS, 0x202)
    cpu.reg_write(x.UC_X86_REG_ESP, ENTRY_ESP)
    cpu.reg_write(x.UC_X86_REG_EIP, CODE)

    calls, diagnostics, exits = [], [], []
    timer_reads, timer_writes, callback_errors = [], [], []
    instruction_count = [0]

    def fail(error):
        callback_errors.append(error)
        cpu.emu_stop()

    def check_memory(address, size, write, value=0):
        if ranges_overlap(address, size, TIMER_PAGE, PAGE):
            assert range_contains(TIMER_PAGE, PAGE, address, size)
            offset = address - TIMER
            assert size == 4 and offset in (0, 4), ("timer access", address, size)
            record = (offset, size, u32(value)) if write else (offset, size)
            (timer_writes if write else timer_reads).append(record)
            return
        if ranges_overlap(address, size, STACK_PAGE, STACK_SIZE):
            assert range_contains(STACK_PAGE, STACK_SIZE, address, size)
            assert range_contains(ENTRY_ESP - 0x100,
                                  0x100 + len(frame), address, size), (
                                      "stack access", address, size)
            return
        raise AssertionError(("unexpected memory access", address, size, write))

    def on_read(_cpu, _access, address, size, _value, _unused):
        try:
            check_memory(address, size, False)
        except Exception as error:  # Unicorn callbacks do not reliably propagate.
            fail(error)

    def on_write(_cpu, _access, address, size, value, _unused):
        try:
            check_memory(address, size, True, value)
        except Exception as error:  # Unicorn callbacks do not reliably propagate.
            fail(error)

    def read_word(address):
        return struct.unpack("<I", bytes(cpu.mem_read(address, 4)))[0]

    def on_code(_cpu, address, size, _unused):
        try:
            instruction_count[0] += 1
            if range_contains(CODE, len(code), address, size):
                return
            api = next((name for name, location in APIS.items()
                        if address == location), None)
            assert api is not None, ("unexpected execution", address, size)
            assert size == 1
            calls.append(api)
            esp = cpu.reg_read(x.UC_X86_REG_ESP)
            if api == SYSTEM_MILLISECONDS:
                cpu.reg_write(x.UC_X86_REG_EAX, item["now"])
                cpu.reg_write(x.UC_X86_REG_ECX, 0xC1C1C1C1)
                cpu.reg_write(x.UC_X86_REG_EDX, 0xD1D1D1D1)
            elif api == DISPLAY_ASSERT:
                diagnostics.append((
                    read_word(esp + 4), read_word(esp + 8),
                    read_word(esp + 12), read_word(esp + 16),
                ))
                cpu.reg_write(x.UC_X86_REG_EAX, 0xA2A2A2A2)
                cpu.reg_write(x.UC_X86_REG_ECX, 0xC2C2C2C2)
                cpu.reg_write(x.UC_X86_REG_EDX, 0xD2D2D2D2)
            else:
                exits.append(s32(read_word(esp + 4)))
                cpu.emu_stop()
        except Exception as error:  # Preserve the real callback failure.
            fail(error)

    cpu.hook_add(u.UC_HOOK_MEM_READ, on_read)
    cpu.hook_add(u.UC_HOOK_MEM_WRITE, on_write)
    cpu.hook_add(u.UC_HOOK_CODE, on_code)
    try:
        with quiet_unicorn_windows_exceptions():
            cpu.emu_start(CODE, STOP, count=500)
    except u.UcError as error:
        raise AssertionError(("Unicorn execution failed", item["label"], error)) from error
    if callback_errors:
        raise callback_errors[0]
    assert instruction_count[0] < 500

    fatal = bool(exits)
    expected_calls = ([SYSTEM_MILLISECONDS, DISPLAY_ASSERT, SYSTEM_EXIT]
                      if fatal else [SYSTEM_MILLISECONDS])
    assert calls == expected_calls
    assert exits == ([-1] if fatal else [])
    if fatal:
        assert len(diagnostics) == 1
        information, file_name, line, fatal_flag = diagnostics[0]
        assert file_name == LITERAL_ADDRESSES[FILE_LITERAL] and fatal_flag == 1
        assert (information, line) in {
            (LITERAL_ADDRESSES[ADJUSTMENT_LITERAL], 0x68),
            (LITERAL_ADDRESSES[TIMER_LITERAL], 0x75),
        }
    else:
        assert not diagnostics
        assert cpu.reg_read(x.UC_X86_REG_EIP) == STOP
        assert cpu.reg_read(x.UC_X86_REG_ESP) == ENTRY_ESP + 4
        for name, value in SAVED_REGISTERS.items():
            assert cpu.reg_read(register_ids[name]) == value

    timer_after = bytes(cpu.mem_read(TIMER_PAGE, PAGE))
    timer_offset = TIMER - TIMER_PAGE
    assert timer_after[:timer_offset] == timer_before[:timer_offset]
    assert timer_after[timer_offset + 8:] == timer_before[timer_offset + 8:]
    assert bytes(cpu.mem_read(ENTRY_ESP, len(frame))) == frame
    assert bytes(cpu.mem_read(CODE, len(code))) == code
    assert read_word(TIMER + 4) == item["now"]
    assert {(offset, size) for offset, size in timer_reads} <= {(0, 4), (4, 4)}
    assert {(offset, size) for offset, size, _value in timer_writes} <= {(0, 4), (4, 4)}

    pointer_names = {value: name for name, value in LITERAL_ADDRESSES.items()}
    return {
        "outcome": "fatal" if fatal else "normal",
        "timer": bytes(cpu.mem_read(TIMER, 8)),
        "remaining": read_word(TIMER),
        "calls": tuple(calls),
        "diagnostics": tuple((pointer_names[info], pointer_names[file_name], line, flag)
                             for info, file_name, line, flag in diagnostics),
        "exits": tuple(exits),
        "reads": tuple(timer_reads),
        "writes": tuple(timer_writes),
        "eax": cpu.reg_read(x.UC_X86_REG_EAX),
    }


def semantic_view(observation):
    return (
        observation["outcome"], observation["timer"], observation["calls"],
        observation["diagnostics"], observation["exits"],
    )


def require_same(left, right):
    if semantic_view(left) != semantic_view(right):
        raise RuntimeSemanticMismatch((semantic_view(left), semantic_view(right)))


def test_337_real_body_cases_match_target_and_preserve_contracts(bodies):
    footprints = {"target_read": set(), "target_write": set(),
                  "rebuilt_read": set(), "rebuilt_write": set()}
    target_observations = {}
    volatile_differences = 0
    assertion_literals = set()
    for item in cases():
        target = execute(bodies["target"], item)
        rebuilt = execute(bodies["rebuilt"], item)
        require_same(target, rebuilt)
        target_observations[item["label"]] = target
        volatile_differences += target["eax"] != rebuilt["eax"]
        assertion_literals.update(entry[0] for entry in target["diagnostics"])
        footprints["target_read"].update(target["reads"])
        footprints["target_write"].update((offset, size) for offset, size, _ in target["writes"])
        footprints["rebuilt_read"].update(rebuilt["reads"])
        footprints["rebuilt_write"].update((offset, size) for offset, size, _ in rebuilt["writes"])

    assert all(value == {(0, 4), (4, 4)} for value in footprints.values())
    assert volatile_differences > 0  # EAX is intentionally not a void-return contract.
    assert assertion_literals == {ADJUSTMENT_LITERAL, TIMER_LITERAL}
    expected_boundaries = {
        "positive_to_negative_clock_sign_transition": (100, "normal"),
        "negative_half_clock_forward": (99, "normal"),
        "tickcount_full_wrap": (99, "normal"),
        "negative_elapsed_path": (101, "normal"),
        "negative_elapsed_subtraction_wrap": (0x7FFFFFFF, "normal"),
        "increment_sign_overflow_clamps": (30_999, "normal"),
        "negative_adjustment_asserts_after_update": (99, "fatal"),
        "negative_maximum_final_assert": (0xFFFFFFFF, "fatal"),
    }
    for label, expected in expected_boundaries.items():
        observed = target_observations[label]
        assert (observed["remaining"], observed["outcome"]) == expected


def test_real_overflow_branch_mutation_is_rejected(bodies):
    item = next(item for item in cases()
                if item["label"] == "increment_sign_overflow_clamps")
    reference = execute(bodies["rebuilt"], item)
    require_same(execute(bodies["target"], item), reference)
    mutant = bytearray(bodies["rebuilt"])
    assert mutant[0x56:0x58] == b"\x7D\x05"
    mutant[0x56] = 0xEB  # JGE -> JMP: bypass the overflow clamp.
    changed = execute(bytes(mutant), item)
    with pytest.raises(RuntimeSemanticMismatch):
        require_same(reference, changed)


def test_real_timer_update_branch_mutation_is_rejected(bodies):
    item = next(item for item in cases()
                if item["label"] == "ordinary_forward_elapsed")
    reference = execute(bodies["rebuilt"], item)
    require_same(execute(bodies["target"], item), reference)
    mutant = bytearray(bodies["rebuilt"])
    assert mutant[0x14:0x16] == b"\x7E\x14"
    mutant[0x14] = 0xEB  # JLE -> JMP: always bypass elapsed-time processing.
    changed = execute(bytes(mutant), item)
    with pytest.raises(RuntimeSemanticMismatch):
        require_same(reference, changed)
