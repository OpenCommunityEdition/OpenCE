"""Pinned actual-x86 audit of the January private base-layer wrapper.

The draw_gravy_layer call is an observed private-ABI boundary, not execution
of the residual renderer or a GPU-equivalence claim. Progress patterns outside
[0, 1], including quiet NaNs, test bit transport only. No C wrapper is generated.
The ordinary role must be pinned independently by the invoking orchestrator.
The target's public placeholder metadata is preserved; the two candidate roles
use the explicitly reconstructed private semantic name, not a historical name claim.
"""

from contextlib import contextmanager
import argparse
import faulthandler
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

if not __debug__:
    raise RuntimeError("audit requires Python assertion checks; do not use -O")

TARGET_SHA = "3e92c6262d38b8ad207566eeb9ef5610ddf12260b1dd0c2ad161445ec7eb6836"
FIRST_SHA = "0c328c780ffc7b65cf2a4a2cce7fa9eb7b91c006158b23e0572892d04cf22282"
BODY_SHA = "b97e9b8de80651484aec3b85c442890712ab8a10727916fdee74edff460d8d68"
TARGET_NAME, NAME = "_code_000d1930", "_draw_gravy_base_layer"
CODE, STOP, CALLEE = 0x100000, 0x100F00, 0x200100
CONSTANTS, COLOR_PAGE, COLOR, STACK = 0x300000, 0x400000, 0x400100, 0x500000
ESP = STACK + 0x10000
EXPECTED_LAYER = struct.pack("<7f", 640, 480, 0, 0, 320, 240, 0)
LITERALS = (("__real@4074000000000000", 320.0),
            ("__real@406e000000000000", 240.0))
CONTROL_WORDS = (0x027F, 0x037F)
PROGRESS_BITS = (0, 0x80000000, 0x3F000000, 0xBF000000,
                 0x3F800000, 0xBF800000, 1, 0x80000001,
                 0x7F7FFFFF, 0xFF7FFFFF, 0x7FC12345, 0xFFC54321)


def validate_case(progress_bits, control_word):
    assert type(progress_bits) is int and 0 <= progress_bits <= 0xFFFFFFFF
    assert control_word in CONTROL_WORDS


def verify_pin(data, expected_sha):
    assert re.fullmatch(r"[0-9a-f]{64}", expected_sha), "invalid SHA-256 pin"
    actual = hashlib.sha256(data).hexdigest()
    assert actual == expected_sha, "object SHA-256 mismatch"
    return actual


def prepare_body(obj, name, storage):
    owner = cc.symbol(obj, name)
    section = obj["sections"][owner["section"] - 1]
    info = cc.section_info(obj, name)
    assert (owner["value"], owner["type"], owner["storage"]) == (0, 32, storage)
    assert section["flags"] == 0x60501020
    assert (info["size"], info["relocation_count"], info["normalized_sha256"]) == (
        96, 3, BODY_SHA)
    raw = bytearray(cc._section_bytes(obj, section))
    payloads = []
    for symbol, value in LITERALS:
        literal = cc.symbol(obj, symbol)
        literal_section = obj["sections"][literal["section"] - 1]
        assert literal["value"] == 0 and literal_section["size"] == 8
        assert literal_section["name"] == ".rdata"
        payload = bytes(cc._section_bytes(obj, literal_section))
        assert payload == struct.pack("<d", value), "literal payload mismatch"
        payloads.append(payload)
    expected = ((8, cc.IMAGE_REL_I386_DIR32, LITERALS[0][0], CONSTANTS),
                (28, cc.IMAGE_REL_I386_DIR32, LITERALS[1][0], CONSTANTS + 8),
                (76, cc.IMAGE_REL_I386_REL32, "_draw_gravy_layer", CALLEE))
    assert len(info["relocations"]) == len(expected)
    for relocation, (offset, kind, symbol, destination) in zip(info["relocations"], expected):
        assert (relocation["address"], relocation["type"]) == (offset, kind)
        assert relocation.get("symbolic_target", relocation["target"]) == ["symbol", symbol, 0]
        if kind == cc.IMAGE_REL_I386_REL32:
            destination -= CODE + offset + 4
        struct.pack_into("<I", raw, offset, destination)
    return bytes(raw), b"".join(payloads), info


def check_boundary(layer, color, arguments, progress_bits):
    assert layer == EXPECTED_LAYER, "layer construction mismatch"
    assert color == COLOR, "color pointer mismatch"
    assert arguments == (0x3F800000, progress_bits), "alpha/progress transport mismatch"


@contextmanager
def quiet_unicorn_windows_exceptions():
    enabled = faulthandler.is_enabled()
    if enabled:
        faulthandler.disable()
    try:
        yield
    finally:
        if enabled:
            faulthandler.enable()


def execute(raw, literals, progress_bits, control_word):
    import unicorn as u
    from unicorn import x86_const as x

    validate_case(progress_bits, control_word)
    assert len(raw) == 96 and literals == struct.pack("<2d", 320, 240)
    saved = {x.UC_X86_REG_EBP: 0x11223344, x.UC_X86_REG_EBX: 0x22334455,
             x.UC_X86_REG_ESI: 0x33445566, x.UC_X86_REG_EDI: 0x44556677}
    color = bytearray(b"\xB6" * 0x1000)
    color[0x100:0x10C] = struct.pack("<3f", 0.125, 0.5, 0.875)
    constants = literals + b"\xC5" * (0x1000 - len(literals))
    stack = bytearray(b"\xA7" * 0x20000)
    struct.pack_into("<II", stack, ESP - STACK, STOP, progress_bits)
    observed = []
    with quiet_unicorn_windows_exceptions():
        cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        for address, size in ((CODE, 0x1000), (CALLEE & ~0xFFF, 0x1000),
                              (CONSTANTS, 0x1000), (COLOR_PAGE, 0x1000), (STACK, len(stack))):
            cpu.mem_map(address, size)
        cpu.mem_write(CODE, raw)
        cpu.mem_write(CALLEE, b"\xC3")
        cpu.mem_write(CONSTANTS, constants)
        cpu.mem_write(COLOR_PAGE, bytes(color))
        cpu.mem_write(STACK, bytes(stack))
        cpu.mem_protect(CODE, 0x1000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        cpu.mem_protect(CALLEE & ~0xFFF, 0x1000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        cpu.mem_protect(CONSTANTS, 0x1000, u.UC_PROT_READ)
        cpu.mem_protect(COLOR_PAGE, 0x1000, u.UC_PROT_READ)
        cpu.reg_write(x.UC_X86_REG_ESP, ESP)
        cpu.reg_write(x.UC_X86_REG_ECX, COLOR)
        cpu.reg_write(x.UC_X86_REG_EFLAGS, 0x2)
        cpu.reg_write(x.UC_X86_REG_FPCW, control_word)
        cpu.reg_write(x.UC_X86_REG_FPSW, 0)
        cpu.reg_write(x.UC_X86_REG_FPTAG, 0xFFFF)
        initial_tag = cpu.reg_read(x.UC_X86_REG_FPTAG)
        assert initial_tag == 0xFFFF, "x87 initialization is not empty"
        for register, value in saved.items():
            cpu.reg_write(register, value)

        def boundary(machine, address, size, user_data):
            if address != CALLEE:
                return
            assert not observed, "duplicate callee call"
            call_esp = machine.reg_read(x.UC_X86_REG_ESP)
            layer_address = machine.reg_read(x.UC_X86_REG_EAX)
            assert call_esp == ESP - 44 and layer_address == ESP - 32, "private stack ABI mismatch"
            layer = bytes(machine.mem_read(layer_address, 28))
            arguments = struct.unpack("<II", machine.mem_read(call_esp + 4, 8))
            color_pointer = machine.reg_read(x.UC_X86_REG_ECX)
            check_boundary(layer, color_pointer, arguments, progress_bits)
            observed.append((layer.hex(), color_pointer, arguments))
            assert machine.reg_read(x.UC_X86_REG_FPSW) & 0x3800 == 0, "x87 depth leak at call"
            assert machine.reg_read(x.UC_X86_REG_FPTAG) == initial_tag, "x87 nonempty at call"
            assert machine.reg_read(x.UC_X86_REG_EFLAGS) & 0x400 == 0, "direction flag set at call"
            machine.reg_write(x.UC_X86_REG_EAX, 0xD0123456)
            machine.reg_write(x.UC_X86_REG_ECX, 0xE0234567)
            machine.reg_write(x.UC_X86_REG_EDX, 0xF0345678)

        cpu.hook_add(u.UC_HOOK_CODE, boundary)
        cpu.emu_start(CODE, STOP, count=1000)
        assert len(observed) == 1, "missing callee call"
        assert cpu.reg_read(x.UC_X86_REG_EIP) == STOP
        assert cpu.reg_read(x.UC_X86_REG_ESP) == ESP + 4, "wrapper stack cleanup mismatch"
        assert all(cpu.reg_read(register) == value for register, value in saved.items())
        assert cpu.reg_read(x.UC_X86_REG_FPCW) == control_word
        assert cpu.reg_read(x.UC_X86_REG_FPSW) & 0x3800 == 0, "x87 depth leak at return"
        assert cpu.reg_read(x.UC_X86_REG_FPTAG) == initial_tag, "x87 nonempty at return"
        assert cpu.reg_read(x.UC_X86_REG_EFLAGS) & 0x400 == 0, "direction flag set at return"
        assert bytes(cpu.mem_read(CONSTANTS, 0x1000)) == constants
        assert bytes(cpu.mem_read(COLOR_PAGE, 0x1000)) == bytes(color)
        result_stack = bytes(cpu.mem_read(STACK, len(stack)))
        assert result_stack[:ESP - STACK - 44] == stack[:ESP - STACK - 44]
        assert result_stack[ESP - STACK:] == stack[ESP - STACK:]
    return observed[0]


def mutation(raw, kind):
    result = bytearray(raw)
    if kind == "width":
        assert result[0x25:0x2C] == bytes.fromhex("c7 45 e4 00 00 20 44")
        struct.pack_into("<f", result, 0x28, 641.0)
    elif kind == "progress":
        assert result[0x0C:0x0F] == bytes.fromhex("8b 45 08")
        result[0x0E] = 0x0C
    else:
        raise ValueError("unknown mutation")
    return bytes(result)


def audit(target_path, first_path, ordinary_path, ordinary_sha):
    roles = (("target", target_path, TARGET_SHA, TARGET_NAME, 2),
             ("first", first_path, FIRST_SHA, NAME, 3),
             ("ordinary", ordinary_path, ordinary_sha, NAME, 3))
    prepared, records = [], []
    for role, path, pin, name, storage in roles:
        path = Path(path).resolve(strict=True)
        data = path.read_bytes()
        verify_pin(data, pin)
        prepared.append(prepare_body(cc.load(data), name, storage))
        records.append({"role": role, "path": str(path), "sha256": pin, "symbol": name})
    assert all(cc.section_infos_equal(prepared[0][2], item[2]) for item in prepared[1:])
    baseline = None
    for raw, literals, _ in prepared:
        results = [execute(raw, literals, bits, cw) for cw in CONTROL_WORDS for bits in PROGRESS_BITS]
        if baseline is None:
            baseline = results
        assert results == baseline, "role execution mismatch"
    rejected = []
    raw, literals, _ = prepared[2]
    for kind, expected in (("width", "layer construction mismatch"),
                           ("progress", "alpha/progress transport mismatch")):
        try:
            execute(mutation(raw, kind), literals, 0x3F000000, CONTROL_WORDS[0])
        except AssertionError as error:
            assert str(error) == expected, "mutation rejected for the wrong reason"
            rejected.append(kind)
        else:
            raise AssertionError("mutation was not rejected: " + kind)
    return {"verdict": "PASS", "scope": "wrapper construction/private ABI only; not renderer equivalence",
            "harness_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "roles": records, "cases_per_role": len(baseline), "executions": 3 * len(baseline),
            "progress_bits": [f"{value:08x}" for value in PROGRESS_BITS],
            "control_words": [f"{value:04x}" for value in CONTROL_WORDS],
            "mutations_rejected": rejected}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for role in ("target", "first", "ordinary"):
        parser.add_argument("--" + role, type=Path, required=True)
    parser.add_argument("--ordinary-sha256", required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    assert not args.report.exists(), "refusing to overwrite an audit report"
    report = audit(args.target, args.first, args.ordinary, args.ordinary_sha256)
    with args.report.open("x", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
