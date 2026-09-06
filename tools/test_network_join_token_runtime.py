"""Execute January and rebuilt join-token owners with bounded external APIs.

This tests the actual COFF function, not a handwritten substitute. Release
behavior is checked against the supplied-source algorithm, not January debug
bytes. Test mutations remain in memory and never alter a production object.
"""
from contextlib import contextmanager
import faulthandler
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

import pytest

from tools import coff_compare as cc

ROOT = Path(__file__).resolve().parents[1]
UNIT = 'source/networking/network_server_manager'
NAME = '_network_game_generate_join_game_token'
# The first sixteen bytes of the source-attested nineteen-byte initializer.
TOKEN = b'message in a bottle'[:16]
TARGET_SHA = '13f2acfff35e86b80795cc14e79579c1f1335feed3725b35f127a889ca3a8d59'
TARGET_RELOCATIONS = [
    (0x60, 6, ('defined-noncode', '.rdata', '??_C@_0DD@CFCGIJJL@c?3?2halo?2SOURCE?2networking?2networ@', 0)),
    (0x65, 6, ('defined-noncode', '.rdata', '??_C@_0L@DEEHFDNB@join_token?$AA@', 0)),
    (0x6a, 20, ('symbol', '_display_assert', 0)),
    (0x71, 20, ('symbol', '_system_exit', 0)),
    (0x7e, 20, ('symbol', '_csmemset', 0)),
    (0x8a, 20, ('symbol', '_csmemcpy', 0)),
]


class TokenWriteContractError(AssertionError):
    """Raised only after executing a bad token-write API call."""


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


@pytest.fixture(scope='module', params=['january', 'debug', 'release'])
def owner(request, tmp_path_factory):
    target = ROOT / f'build/split/{UNIT}.obj'
    if not target.is_file():
        pytest.skip('locally supplied January target is unavailable')
    obj = cc.load(target)
    info = cc.section_info(obj, NAME)
    assert (info['size'], info['relocation_count'], info['normalized_sha256']) == (160, 6, TARGET_SHA)
    assert [(r['address'], r['type'], tuple(r['target'])) for r in info['relocations']] == TARGET_RELOCATIONS
    if request.param != 'january':
        if not (ROOT / 'xbox/bin/vc7/CL.Exe').is_file():
            pytest.skip('locally supplied VC7 compiler is unavailable')
        output = tmp_path_factory.mktemp('join-token') / (request.param + '.obj')
        cmd = [sys.executable, '-B', 'tools/campaign/gate.py', UNIT,
               '--fn', NAME, '--out', str(output)]
        if request.param == 'release':
            cmd += ['--cflag', '/UDEBUG', '--cflag', '/DNDEBUG']
        env = dict(os.environ, HALO_CL=str(ROOT / 'xbox/bin/vc7/CL.Exe'))
        result = subprocess.run(cmd, cwd=ROOT, env=env, capture_output=True, text=True)
        assert result.returncode == 0, result.stdout + result.stderr
        obj = cc.load(output)
        if request.param == 'debug':
            assert cc.section_infos_equal(info, cc.section_info(obj, NAME))
    return obj, request.param == 'release'


def execute(obj, release, pointer, fill, checksum, mutation=None):
    u = pytest.importorskip('unicorn')
    from unicorn import x86_const as x
    machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    code_address, stack, data, api = 0x100000, 0x200000, 0x300000, 0x400000
    for base, size in ((code_address, 0x1000), (stack, 0x10000), (data, 0x2000), (api, 0x1000)):
        machine.mem_map(base, size)
    functions = ('_display_assert', '_system_exit', '_csmemset', '_csmemcpy',
                 '_csstrlen', '_tag_groups_checksum')
    hooks = {name: api + 16 * i for i, name in enumerate(functions)}
    for address in hooks.values():
        machine.mem_write(address, b'\xc3')
    entry = cc.symbol(obj, NAME)
    assert entry['value'] == 0 and entry['section'] > 0
    section = obj['sections'][entry['section'] - 1]
    raw = bytearray(cc._section_bytes(obj, section))
    assert section['name'] == '.text' and len(raw) < 0x1000
    if mutation:
        offset, expected, replacement = mutation
        assert raw[offset:offset + len(expected)] == expected
        raw[offset:offset + len(expected)] = replacement
    mapped = {}
    for i in range(section['reloc_count']):
        offset, index, kind = struct.unpack_from('<IIH', obj['data'], section['reloc'] + i * 10)
        symbol = obj['by_index'][index]
        if symbol['section'] > 0:
            number = symbol['section']
            constant = obj['sections'][number - 1]
            assert constant['name'] == '.rdata' and constant['reloc_count'] == 0
            if number not in mapped:
                payload = bytes(cc._section_bytes(obj, constant))
                assert len(payload) <= 0x1000
                address = 0x500000 + len(mapped) * 0x1000
                machine.mem_map(address, 0x1000)
                machine.mem_write(address, payload)
                mapped[number] = address
            target = mapped[number] + symbol['value']
        else:
            assert symbol['section'] == 0 and symbol['name'] in hooks, symbol
            target = hooks[symbol['name']]
        assert kind in (cc.IMAGE_REL_I386_DIR32, cc.IMAGE_REL_I386_REL32)
        addend = struct.unpack_from('<I', raw, offset)[0]
        value = target + addend
        if kind == cc.IMAGE_REL_I386_REL32:
            value -= code_address + offset + 4
        struct.pack_into('<I', raw, offset, value & 0xffffffff)
    machine.mem_write(code_address, bytes(raw))
    before = bytes([fill]) * 0x2000
    machine.mem_write(data, before)
    stop, sp = api + 0xf00, stack + 0x8000
    stack_before = bytes((index * 29 + 7) & 0xff for index in range(0x10000))
    machine.mem_write(stack, stack_before)
    machine.mem_write(stop, b'\xf4')
    machine.mem_write(sp, struct.pack('<II', stop, pointer))
    machine.reg_write(x.UC_X86_REG_ESP, sp)
    machine.reg_write(x.UC_X86_REG_EAX, 0x90909090 ^ checksum)
    machine.reg_write(x.UC_X86_REG_ECX, 0xa5a5a5a5 ^ fill)
    machine.reg_write(x.UC_X86_REG_EDX, 0x5a5a5a5a ^ pointer)
    machine.reg_write(x.UC_X86_REG_EFLAGS, 0x246 if fill & 1 else 0x202)
    saved = {x.UC_X86_REG_EBX: 0x12345678, x.UC_X86_REG_ESI: 0x23456789,
             x.UC_X86_REG_EDI: 0x3456789a, x.UC_X86_REG_EBP: 0x456789ab}
    for register, value in saved.items():
        machine.reg_write(register, value)
    calls, diagnostics, timestamps = [], [], []

    def cstring(address):
        value = bytes(machine.mem_read(address, 128))
        assert b'\0' in value
        return value.split(b'\0', 1)[0]

    def on_code(cpu, address, size, unused):
        if address == stop:
            cpu.emu_stop()
            return
        name = next((name for name, value in hooks.items() if address == value), None)
        if name is None:
            assert code_address <= address < address + size <= code_address + len(raw)
            return
        args = struct.unpack('<4I', bytes(cpu.mem_read(cpu.reg_read(x.UC_X86_REG_ESP) + 4, 16)))
        calls.append(name)
        if name == '_display_assert':
            diagnostics.append((cstring(args[0]), cstring(args[1]), args[2], args[3]))
        elif name == '_system_exit':
            assert args[0] == 0xffffffff
            cpu.emu_stop()
        elif name == '_csmemset':
            assert pointer
            if args[:3] != (pointer, 0, 4):
                raise TokenWriteContractError('clear arguments differ')
            cpu.mem_write(pointer, b'\0' * 4)
            cpu.reg_write(x.UC_X86_REG_EAX, pointer)
        elif name == '_csmemcpy':
            assert pointer
            if args[0] != pointer or args[2] != 16:
                raise TokenWriteContractError('copy arguments differ')
            assert stack <= args[1] < stack + 0x10000 - 19
            assert bytes(cpu.mem_read(args[1], 19)) == b'message in a bottle'
            cpu.mem_write(pointer, bytes(cpu.mem_read(args[1], 16)))
            cpu.reg_write(x.UC_X86_REG_EAX, pointer)
        elif name == '_csstrlen':
            timestamp = cstring(args[0])
            assert re.fullmatch(rb'[A-Z][a-z]{2} [ 0-9][0-9] [0-9]{4}[0-9]{2}:[0-9]{2}:[0-9]{2}', timestamp)
            timestamps.append(timestamp)
            cpu.reg_write(x.UC_X86_REG_EAX, len(timestamp))
        elif name == '_tag_groups_checksum':
            cpu.reg_write(x.UC_X86_REG_EAX, checksum)

    machine.hook_add(u.UC_HOOK_CODE, on_code)
    machine.emu_start(code_address, stop + 1, count=10000)
    after = bytes(machine.mem_read(data, 0x2000))
    if not pointer:
        assert calls == ['_display_assert', '_system_exit']
        assert diagnostics == [(b'join_token', b'c:\\halo\\SOURCE\\networking\\network_server_manager.c', 1754, 1)]
        assert after == before and machine.reg_read(x.UC_X86_REG_EIP) == hooks['_system_exit']
    else:
        assert not diagnostics
        expected_calls = ['_csmemset', '_csmemcpy'] + (['_csstrlen', '_tag_groups_checksum'] if release else [])
        assert calls == expected_calls
        expected = bytearray(TOKEN)
        if release:
            assert len(timestamps) == 1
            timestamp_xor = 0
            for byte in timestamps[0]:
                timestamp_xor ^= byte
            for i, byte in enumerate(struct.pack('<I', checksum)):
                expected[i] ^= timestamp_xor ^ byte
        position = pointer - data
        assert 0 <= position <= 0x2000 - 16
        assert after == before[:position] + expected + before[position + 16:]
        assert machine.reg_read(x.UC_X86_REG_EIP) == stop
        assert machine.reg_read(x.UC_X86_REG_ESP) == sp + 4
        assert all(machine.reg_read(r) == value for r, value in saved.items())
    assert bytes(machine.mem_read(code_address, len(raw))) == bytes(raw)
    # The return address is preserved. VC7 reuses the dead incoming pointer
    # slot in the release build; that is legal callee parameter storage.
    assert bytes(machine.mem_read(sp, 4)) == struct.pack('<I', stop)
    # Allow the callee's locals/outgoing arguments, but protect the rest of the
    # stack and the caller-owned area above the incoming return/argument pair.
    assert bytes(machine.mem_read(stack, 0x8000 - 0x200)) == stack_before[:0x8000 - 0x200]
    assert bytes(machine.mem_read(sp + 8, 0x8000 - 8)) == stack_before[0x8000 + 8:]


@pytest.mark.parametrize('offset', [0x100, 0x101, 0xff9])
@pytest.mark.parametrize('fill', [0, 0x55, 0xaa, 0xff])
@pytest.mark.parametrize('checksum', [0, 0x12345678, 0xffffffff])
def test_generated_token_and_output_bounds(owner, offset, fill, checksum):
    with quiet_unicorn_windows_exceptions():
        execute(*owner, 0x300000 + offset, fill, checksum)


def test_null_token_reports_then_exits_without_memory_write(owner):
    with quiet_unicorn_windows_exceptions():
        execute(*owner, 0, 0xaa, 0x12345678)


@pytest.mark.parametrize('mutation', [(0x78, b'\x6a\x04', b'\x6a\x10'),
                                      (0x82, b'\x6a\x10', b'\x6a\x0f')])
def test_wrong_clear_or_copy_size_is_rejected(owner, mutation):
    if owner[1]:
        pytest.skip('mutations identify January debug instruction offsets')
    entry = cc.symbol(owner[0], NAME)
    raw = cc._section_bytes(owner[0], owner[0]['sections'][entry['section'] - 1])
    offset, expected, replacement = mutation
    assert raw[offset:offset + len(expected)] == expected
    reason = 'clear arguments differ' if offset == 0x78 else 'copy arguments differ'
    with quiet_unicorn_windows_exceptions(), pytest.raises(TokenWriteContractError, match=reason):
        execute(*owner, 0x300100, 0xaa, 0, mutation=mutation)
