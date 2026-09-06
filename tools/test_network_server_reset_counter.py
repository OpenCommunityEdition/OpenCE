"""Execute the actual reset counter/store segment at signed-wrap boundaries.

This is deliberately not a full reset-state-machine or external-API test.
The full function is pinned before executing its call-free 0x4A..0x77 segment.
Source review establishes unsigned addition and VC7's implementation-defined
conversion back to signed storage; these executions check the emitted behavior.
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
UNIT = "source/networking/network_server_manager"
NAME = "_network_game_server_reset_to_pregame"
TARGET_SHA = "0c0c9c29b194f113e494f367fd6db171972ec1225ffd1dde7e517b5b3dfd048f"


class CounterResultMismatch(AssertionError):
    """An executed counter/store segment produced the wrong counter."""


@contextmanager
def quiet_unicorn_windows_exceptions():
    # Unicorn handles Windows mapping exceptions internally. Keep pytest's
    # real exceptions/results intact, without faulthandler's misleading dumps.
    enabled = faulthandler.is_enabled()
    if enabled:
        faulthandler.disable()
    try:
        yield
    finally:
        if enabled:
            faulthandler.enable()


@pytest.fixture(scope="module", params=["january", "rebuilt"])
def reset_code(request, tmp_path_factory):
    target = ROOT / f"build/split/{UNIT}.obj"
    if not target.is_file():
        pytest.skip("locally supplied January target is unavailable")
    obj = cc.load(target)
    info = cc.section_info(obj, NAME)
    assert (info["size"], info["relocation_count"], info["normalized_sha256"]) == (
        592, 26, TARGET_SHA)
    if request.param == "rebuilt":
        compiler = ROOT / "xbox/bin/vc7/CL.Exe"
        if not compiler.is_file():
            pytest.skip("locally supplied VC7 compiler is unavailable")
        output = tmp_path_factory.mktemp("reset-counter") / "rebuilt.obj"
        result = subprocess.run(
            [sys.executable, "-B", "tools/campaign/gate.py", UNIT,
             "--fn", NAME, "--out", str(output),
             "--forbid-emitted-symbol", "_point_from_line3d"],
            cwd=ROOT, env=dict(os.environ, HALO_CL=str(compiler)),
            capture_output=True, text=True,
        )
        assert result.returncode == 0, result.stdout + result.stderr
        obj = cc.load(output)
        assert cc.section_infos_equal(info, cc.section_info(obj, NAME))
    symbol = cc.symbol(obj, NAME)
    section = obj["sections"][symbol["section"] - 1]
    raw = bytes(cc._section_bytes(obj, section))
    assert raw[0x4A:0x50] == bytes.fromhex("8b8e34040000")
    assert raw[0x53] == 0x41  # INC ECX, between the real load and store.
    assert raw[0x71:0x77] == bytes.fromhex("898e34040000")
    assert not any(0x4A <= r["address"] < 0x77 for r in info["relocations"])
    return raw


def execute_stores(raw, value, fill):
    u = pytest.importorskip("unicorn")
    from unicorn import x86_const as x
    machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    code, server = 0x100000, 0x200000
    machine.mem_map(code, 0x1000)
    machine.mem_map(server, 0x1000)
    machine.mem_write(code, raw)
    before = bytearray([fill] * 0x1000)
    struct.pack_into("<I", before, 0x434, value)
    machine.mem_write(server, bytes(before))
    machine.reg_write(x.UC_X86_REG_ESI, server)
    machine.reg_write(x.UC_X86_REG_EBX, 0)
    machine.reg_write(x.UC_X86_REG_ESP, 0x300000)
    machine.emu_start(code + 0x4A, code + 0x77, count=20)
    assert machine.reg_read(x.UC_X86_REG_EIP) == code + 0x77
    after = bytes(machine.mem_read(server, len(before)))
    actual = struct.unpack_from("<I", after, 0x434)[0]
    if actual != (value + 1) & 0xFFFFFFFF:
        raise CounterResultMismatch("counter did not advance modulo 2^32")
    expected = bytearray(before)
    struct.pack_into("<I", expected, 0x434, (value + 1) & 0xFFFFFFFF)
    expected[0x47C:0x480] = bytes(4)
    expected[0x484:0x488] = bytes(4)
    expected[0x4B8:0x4BA] = bytes(2)
    assert after == expected
    assert bytes(machine.mem_read(code, len(raw))) == raw
    assert machine.reg_read(x.UC_X86_REG_ESI) == server
    assert machine.reg_read(x.UC_X86_REG_ESP) == 0x30000C


@pytest.mark.parametrize("value", [0, 1, 0x7FFFFFFE, 0x7FFFFFFF,
                                  0x80000000, 0xFFFFFFFE, 0xFFFFFFFF])
@pytest.mark.parametrize("fill", [0x55, 0xAA])
def test_actual_counter_wrap_and_adjacent_stores(reset_code, value, fill):
    with quiet_unicorn_windows_exceptions():
        execute_stores(reset_code, value, fill)


def test_executed_decrement_mutation_is_rejected(reset_code):
    # Validate mutation setup outside the expected runtime failure scope.
    assert reset_code[0x53] == 0x41
    mutant = bytearray(reset_code)
    mutant[0x53] = 0x49  # DEC ECX; only in-memory test bytes are changed.
    with quiet_unicorn_windows_exceptions(), pytest.raises(
        CounterResultMismatch, match="counter did not advance"
    ):
        execute_stores(bytes(mutant), 0x7FFFFFFF, 0x55)
