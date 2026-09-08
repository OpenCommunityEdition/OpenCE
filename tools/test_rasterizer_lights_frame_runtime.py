"""Execute January and freshly compiled lens-flare visibility updates.

Renderer services and the private result-address getter are contract stubs,
not a GPU simulation or execution of that getter's addressing body. Cases use
the January 640x480 query bound; arbitrary signed-long hardware results are not
claimed safe. The frame body, guards, smoothing, stores and calling ABI execute
as actual x86 instructions in Unicorn.
"""

from contextlib import contextmanager
import faulthandler
import os
from pathlib import Path
import struct
import subprocess
import sys

import pytest

from tools import coff_compare as cc


ROOT = Path(__file__).resolve().parents[1]
UNIT = "source/rasterizer/rasterizer_lights"
FUNCTION = "_rasterizer_lights_begin_for_new_frame"
TARGET = ROOT / ("build/split/" + UNIT + ".obj")
COMPILER = ROOT / "xbox/bin/vc7/CL.Exe"
EXACT_SHA = "ba6000638ca4648f9c8be2129eb56614787733165ab716dc580fa0647dc7bfe0"

CODE, STOP, API = 0x100000, 0x100F00, 0x200000
DATA, PARAMETERS, RESULTS, STACK = 0x300000, 0x400000, 0x500000, 0x600000
ENTRY_ESP = STACK + 0x10000
FRAME_LIMIT = 1024
PARAMETER_SIZE = 0x28
PIXEL_LIMIT = 640 * 480
PROFILE = 24
FILE_LITERAL = "??_C@_0CO@MGBADAON@c?3?2halo?2SOURCE?2rasterizer?2raster@"
INDEX_LITERAL = "??_C@_0DP@MMPMMAFC@lens_flare_index?$DO?$DN0?5?$CG?$CG?5lens_flar@"
API_NAMES = (
    "_rasterizer_profile_begin", "_rasterizer_profile_end",
    "_lens_flare_occlusion_test_results_get",
    "_rasterizer_widget_get_occlusion_test_result", "_display_assert", "_system_exit",
)
ADDRESSES = {name: API + 0x100 * (index + 1)
             for index, name in enumerate(API_NAMES)}
ADDRESSES.update({
    "_rasterizer_debug_options": DATA,
    "_global_screenshot_count": DATA + 0x100,
    "_global_screenshot_size": DATA + 0x102,
    "_local_lens_flare_count": DATA + 0x104,
    "_local_lens_flare_parameters": PARAMETERS,
    FILE_LITERAL: DATA + 0x200,
    INDEX_LITERAL: DATA + 0x300,
})


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


def prepare_body(obj):
    owner = cc.symbol(obj, FUNCTION)
    section = obj["sections"][owner["section"] - 1]
    info = cc.section_info(obj, FUNCTION)
    assert (owner["value"], owner["type"], owner["storage"]) == (0, 0x20, 2)
    assert (info["size"], info["relocation_count"], info["normalized_sha256"]) == (
        304, 15, EXACT_SHA)
    assert section["flags"] == 0x60501020
    raw = bytearray(cc._section_bytes(obj, section))
    for relocation in info["relocations"]:
        target = relocation.get("symbolic_target", relocation["target"])
        assert target[0] == "symbol" and target[1] in ADDRESSES
        name, addend = target[1:]
        offset, kind = relocation["address"], relocation["type"]
        assert addend == (0x1F if name == "_rasterizer_debug_options" else 0)
        value = ADDRESSES[name] + addend
        if kind == cc.IMAGE_REL_I386_REL32:
            assert name in API_NAMES and addend == 0
            value -= CODE + offset + 4
        else:
            assert kind == cc.IMAGE_REL_I386_DIR32 and name not in API_NAMES
        struct.pack_into("<I", raw, offset, value & 0xFFFFFFFF)
    return bytes(raw)


@pytest.fixture(scope="module")
def bodies(tmp_path_factory):
    pytest.importorskip("unicorn")
    if not TARGET.is_file() or not COMPILER.is_file():
        pytest.skip("locally supplied January object/VC7 compiler is unavailable")
    source = ROOT / (UNIT + ".c")
    before = source.read_bytes()
    output = tmp_path_factory.mktemp("lights-frame") / "rebuilt.obj"
    result = subprocess.run(
        [sys.executable, "-B", "tools/campaign/gate.py", UNIT,
         "--fn", FUNCTION, "--forbid-emitted-symbol", "_point_from_line3d",
         "--out", str(output)], cwd=ROOT, capture_output=True, text=True,
        env=dict(os.environ, HALO_CL=str(COMPILER), PYTHONDONTWRITEBYTECODE="1"))
    assert result.returncode == 0, result.stdout + result.stderr
    assert source.read_bytes() == before
    target, rebuilt = cc.load(TARGET), cc.load(output)
    assert cc.section_infos_equal(
        cc.section_info(target, FUNCTION), cc.section_info(rebuilt, FUNCTION)), (
            "January frame body or ordered relocation identities differ")
    return prepare_body(target), prepare_body(rebuilt)


def expected_visibility(previous, visible, denominator):
    assert 0 <= previous <= 255 and 0 <= visible <= PIXEL_LIMIT
    assert -0x80000000 <= denominator <= 0x7FFFFFFF
    if denominator <= 0:
        latest = 0
    else:
        numerator = 255 * visible + (denominator >> 1)
        assert numerator <= 0x7FFFFFFF
        latest = min(255, numerator // denominator)
    if latest == 0:
        return 0
    if latest > previous:
        return (3 * previous + latest) // 4
    if latest < previous:
        return (previous + latest) // 2
    return previous


def execute(raw, records, *, enabled=1, screenshot_count=0, screenshot_size=1):
    import unicorn as u
    from unicorn import x86_const as x

    assert len(records) <= FRAME_LIMIT
    expected = [expected_visibility(*record) for record in records]
    active = bool(enabled) and not (
        screenshot_count > 1 or screenshot_count == 1 and screenshot_size > 1)
    saved = {x.UC_X86_REG_EBP: 0x11223344, x.UC_X86_REG_EBX: 0x22334455,
             x.UC_X86_REG_ESI: 0x33445566, x.UC_X86_REG_EDI: 0x44556677}
    data = bytearray(b"\xA7" * 0x1000)
    data[0x1F] = enabled
    struct.pack_into("<hhi", data, 0x100, screenshot_count, screenshot_size, len(records))
    parameters = bytearray(b"\xB6" * (PARAMETER_SIZE * FRAME_LIMIT))
    results = bytearray(b"\xC5" * 0x1000)
    for index, (previous, _, denominator) in enumerate(records):
        struct.pack_into("<i", parameters, index * PARAMETER_SIZE + 0x24, denominator)
        results[0x100 + index] = previous
    expected_data = bytearray(data)
    expected_results = bytearray(results)
    if active:
        struct.pack_into("<i", expected_data, 0x104, 0)
        expected_results[0x100:0x100 + len(records)] = bytes(expected)

    trace = []
    reverse = {ADDRESSES[name]: name for name in API_NAMES}
    with quiet_unicorn_windows_exceptions():
        cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        for address, size in ((CODE, 0x1000), (API, 0x1000), (DATA, 0x1000),
                              (PARAMETERS, 0xA000), (RESULTS, 0x1000),
                              (STACK, 0x20000)):
            cpu.mem_map(address, size)
        cpu.mem_write(CODE, raw)
        cpu.mem_write(API, b"\xC3" * 0x1000)
        cpu.mem_write(DATA, bytes(data))
        cpu.mem_write(PARAMETERS, bytes(parameters))
        cpu.mem_write(RESULTS, bytes(results))
        cpu.mem_write(ENTRY_ESP, struct.pack("<I", STOP))
        cpu.reg_write(x.UC_X86_REG_ESP, ENTRY_ESP)
        for register, value in saved.items():
            cpu.reg_write(register, value)

        def api_hook(machine, address, size, user_data):
            if address not in reverse:
                return
            name = reverse[address]
            esp = machine.reg_read(x.UC_X86_REG_ESP)
            argument = struct.unpack("<I", machine.mem_read(esp + 4, 4))[0]
            returned = 0xD0123456
            if name in ("_rasterizer_profile_begin", "_rasterizer_profile_end"):
                assert argument == PROFILE
                trace.append((name, argument))
            elif name == "_lens_flare_occlusion_test_results_get":
                pointer = machine.reg_read(x.UC_X86_REG_EAX)
                index, remainder = divmod(pointer - PARAMETERS, PARAMETER_SIZE)
                assert remainder == 0 and 0 <= index < len(records)
                returned = RESULTS + 0x100 + index
                trace.append((name, index))
            elif name == "_rasterizer_widget_get_occlusion_test_result":
                assert 0 <= argument < len(records) and records[argument][2] > 0
                returned = records[argument][1]
                trace.append((name, argument))
            else:
                raise AssertionError("Unexpected assertion/exit on valid frame: " + name)
            machine.reg_write(x.UC_X86_REG_EAX, returned)
            machine.reg_write(x.UC_X86_REG_ECX, 0xE0234567)
            machine.reg_write(x.UC_X86_REG_EDX, 0xF0345678)

        cpu.hook_add(u.UC_HOOK_CODE, api_hook)
        cpu.emu_start(CODE, STOP, count=500000)
        assert cpu.reg_read(x.UC_X86_REG_EIP) == STOP
        assert cpu.reg_read(x.UC_X86_REG_ESP) == ENTRY_ESP + 4
        assert all(cpu.reg_read(register) == value for register, value in saved.items())
        assert bytes(cpu.mem_read(PARAMETERS, len(parameters))) == bytes(parameters)
        assert bytes(cpu.mem_read(DATA, 0x1000)) == bytes(expected_data)
        actual_results = bytes(cpu.mem_read(RESULTS, 0x1000))
        assert actual_results == bytes(expected_results), "visibility/store mismatch"

    expected_trace = [("_rasterizer_profile_begin", PROFILE)]
    if active:
        for index, (_, _, denominator) in enumerate(records):
            expected_trace.append(("_lens_flare_occlusion_test_results_get", index))
            if denominator > 0:
                expected_trace.append(("_rasterizer_widget_get_occlusion_test_result", index))
    expected_trace.append(("_rasterizer_profile_end", PROFILE))
    assert trace == expected_trace
    return actual_results, trace


def test_real_frame_bodies_cover_rounding_saturation_and_smoothing(bodies):
    records = [(previous, visible, denominator)
               for previous in (0, 1, 63, 127, 128, 254, 255)
               for visible, denominator in (
                   (0, 1), (1, 1), (1, 2), (1, 3), (2, 3), (127, 255),
                   (255, 255), (256, 255), (PIXEL_LIMIT, PIXEL_LIMIT),
                   (PIXEL_LIMIT, 0x7FFFFFFF), (0, 0), (0, -1))]
    assert execute(bodies[0], records) == execute(bodies[1], records)


@pytest.mark.parametrize(("enabled", "count", "size"), (
    (0, 0, 1), (1, 2, 1), (1, 1, 2), (1, 1, 1), (1, 0, 3), (1, -1, 3),
))
def test_real_frame_gates_keep_or_reset_the_queue(bodies, enabled, count, size):
    records = [(127, 255, 255), (200, 0, 0)]
    options = dict(enabled=enabled, screenshot_count=count, screenshot_size=size)
    assert execute(bodies[0], records, **options) == execute(bodies[1], records, **options)


@pytest.mark.parametrize("count", (0, FRAME_LIMIT))
def test_real_frame_queue_boundaries(bodies, count):
    records = [(index & 255, index % 257, 256) for index in range(count)]
    assert execute(bodies[0], records) == execute(bodies[1], records)


def test_smoothing_branch_negative_control(bodies):
    mutant = bytearray(bodies[1])
    assert mutant[0xC6] == 0x76  # jbe selects the non-increasing visibility path.
    mutant[0xC6] = 0x77
    with pytest.raises(AssertionError, match="visibility/store mismatch"):
        execute(bytes(mutant), [(128, 255, 255)])
