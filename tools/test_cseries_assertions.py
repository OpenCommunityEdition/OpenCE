"""Execute actual VC7 macro expansions, with bounded diagnostic API doubles."""
from contextlib import contextmanager
import faulthandler
from pathlib import Path
import struct
import subprocess

import pytest

from tools import coff_compare as cc

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tools/fixtures/cseries_assertions.c"
COMPILER = ROOT / "xbox/bin/vc7/CL.Exe"


@contextmanager
def quiet_unicorn_windows_exceptions():
    """Do not print Unicorn's internally handled Windows memory probes."""
    enabled = faulthandler.is_enabled()
    if enabled:
        faulthandler.disable()
    try:
        yield
    finally:
        if enabled:
            faulthandler.enable()


@pytest.fixture(scope="module", params=[(False, True), (True, True), (False, False), (True, False)],
                ids=["matching-debug", "nonmatching-debug", "matching-ndebug", "nonmatching-ndebug"])
def compiled_assertions(request, tmp_path_factory):
    if not COMPILER.is_file():
        pytest.skip("the locally supplied VC7/XDK compiler is unavailable")
    folder = tmp_path_factory.mktemp("cseries-assertions")
    output = folder / "assertions.obj"
    nonmatching, debug = request.param
    cmd = [str(COMPILER), "/nologo", "/c", "/O2", "/Oy-", "/DDEBUG" if debug else "/DNDEBUG",
           "/Dxbox", "/I" + str(ROOT / "xbox/include"),
           "/I" + str(ROOT / "source"), "/Fo" + str(output), str(FIXTURE)]
    if nonmatching:
        cmd.insert(1, "/DNON_MATCHING")
    result = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return cc.load(output), nonmatching


def run_probe(obj, name, condition):
    u = pytest.importorskip("unicorn")
    from unicorn import x86_const as x
    cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    cpu.mem_map(0x100000, (len(obj['sections']) + 1) * 0x1000)
    cpu.mem_map(0x300000, 0x10000)
    cpu.mem_map(0x400000, 0x1000)
    cpu.mem_map(0x500000, 0x1000)
    addresses = {s['index']: 0x100000 + s['index'] * 0x1000 for s in obj['sections']}
    hooks = {n: 0x400000 + i * 0x10 for i, n in enumerate((
        '_assertion_condition_probe', '_assertion_diagnostic_probe',
        '_csprintf', '_display_assert', '_system_exit', '_halt_and_catch_fire'))}
    externals = {**hooks, '_temporary': 0x500000}
    ranges = []
    for section in obj['sections']:
        if section['name'].startswith('.debug') or section['name'] == '.drectve':
            continue
        assert section['name'] in ('.text', '.rdata', '.data', '.bss')
        assert section['size'] <= 0x1000
        data = bytearray(cc._section_bytes(obj, section))
        base = addresses[section['index']]
        for i in range(section['reloc_count']):
            offset, index, kind = struct.unpack_from('<IIH', obj['data'], section['reloc'] + i * 10)
            symbol = obj['by_index'][index]
            if symbol['section'] > 0:
                target = addresses[symbol['section']] + symbol['value']
            else:
                assert symbol['section'] == 0 and symbol['name'] in externals, symbol
                target = externals[symbol['name']]
            addend = struct.unpack_from('<I', data, offset)[0]
            assert kind in (cc.IMAGE_REL_I386_DIR32, cc.IMAGE_REL_I386_REL32)
            value = target + addend - (base + offset + 4 if kind == cc.IMAGE_REL_I386_REL32 else 0)
            struct.pack_into('<I', data, offset, value & 0xffffffff)
        if data:
            cpu.mem_write(base, bytes(data))
        if section['flags'] & cc.IMAGE_SCN_CNT_CODE:
            ranges.append((base, base + len(data)))
    for address in hooks.values():
        cpu.mem_write(address, b'\xc3')
    stop, sp = 0x400f00, 0x308000
    cpu.mem_write(stop, b'\xf4')
    cpu.mem_write(sp, struct.pack('<I', stop))
    cpu.mem_write(0x500200, b'payload\0')
    cpu.reg_write(x.UC_X86_REG_ESP, sp)
    saved = {x.UC_X86_REG_EBX: 0x12345678, x.UC_X86_REG_ESI: 0x23456789,
             x.UC_X86_REG_EDI: 0x3456789a, x.UC_X86_REG_EBP: 0x456789ab}
    for register, value in saved.items():
        cpu.reg_write(register, value)
    calls = []
    diagnostics = []

    def string(address):
        assert address != 0
        data = bytes(cpu.mem_read(address, 256))
        assert b'\0' in data
        return data.split(b'\0', 1)[0].decode('ascii')

    def on_code(uc, address, size, unused):
        if address == stop:
            uc.emu_stop()
            return
        called = next((n for n, a in hooks.items() if a == address), None)
        if called is None:
            assert any(start <= address and address + size <= end for start, end in ranges)
            return
        calls.append(called)
        stack = uc.reg_read(x.UC_X86_REG_ESP)
        args = struct.unpack('<4I', bytes(uc.mem_read(stack + 4, 16)))
        if called == '_assertion_condition_probe':
            uc.reg_write(x.UC_X86_REG_EAX, condition & 0xffffffff)
        elif called == '_assertion_diagnostic_probe':
            uc.reg_write(x.UC_X86_REG_EAX, 0x500200)
        elif called == '_csprintf':
            assert args[0] == externals['_temporary']
            assert string(args[1]) == 'diagnostic: %s'
            assert string(args[2]) == 'payload'
            uc.mem_write(args[0], b'diagnostic: payload\0')
            uc.reg_write(x.UC_X86_REG_EAX, args[0])
        elif called == '_display_assert':
            diagnostics.append((string(args[0]) if args[0] else None, string(args[1]), args[2], args[3]))
        elif called == '_system_exit':
            assert args[0] == 0xffffffff
            uc.emu_stop()
        elif called == '_halt_and_catch_fire':
            uc.emu_stop()

    handle = cpu.hook_add(u.UC_HOOK_CODE, on_code)
    entry = cc.symbol(obj, '_' + name)
    try:
        cpu.emu_start(addresses[entry['section']] + entry['value'], stop + 1, count=10000)
    finally:
        cpu.hook_del(handle)
    if '_system_exit' not in calls and '_halt_and_catch_fire' not in calls:
        assert cpu.reg_read(x.UC_X86_REG_EIP) == stop
        assert cpu.reg_read(x.UC_X86_REG_ESP) == sp + 4
        assert all(cpu.reg_read(r) == v for r, v in saved.items())
    else:
        assert cpu.reg_read(x.UC_X86_REG_EIP) == hooks[calls[-1]]
    return calls, diagnostics


def fixture_macro_line(name):
    source = FIXTURE.read_text(encoding='utf-8')
    start = source.index('void ' + name + '(')
    end = source.index(';', source.index('{', start))
    # VC7 expands __LINE__ at the end of a multiline macro invocation.
    return source.count('\n', 0, end) + 1


CASES = [(f'{family}{matching}_{kind}', condition)
         for family in ('assertion', 'warning', 'halting')
         for matching in ('', '_matching')
         for kind in ('plain', 'diagnostic', 'formatted')
         for condition in ((None,) if family == 'halting' else (0, 1, -1))]


@pytest.mark.parametrize('name,condition', CASES)
def test_diagnostic_family_contract(compiled_assertions, name, condition):
    obj, nonmatching = compiled_assertions
    with quiet_unicorn_windows_exceptions():
        calls, diagnostics = run_probe(obj, name, condition or 0)
    halting = name.startswith('halting_')
    warning = name.startswith('warning_')
    fails = halting or not condition
    expected = [] if halting else ['_assertion_condition_probe']
    if fails:
        if not name.endswith('_plain'):
            expected.append('_assertion_diagnostic_probe')
        if name.endswith('_formatted'):
            expected.append('_csprintf')
        expected.append('_display_assert')
        if halting:
            expected.append('_halt_and_catch_fire')
        elif not warning:
            expected.append('_system_exit')
    assert calls == expected
    assert len(diagnostics) == int(fails)
    if diagnostics:
        message, filename, line, fatal = diagnostics[0]
        assert fatal == int(not warning)
        assert message == (None if halting and name.endswith('_plain') else
                           'assertion_condition_probe()' if name.endswith('_plain') else
                           'diagnostic: payload' if name.endswith('_formatted') else 'payload')
        if '_matching_' in name and not nonmatching:
            assert (filename, line) == ('original.c', 8505)
        else:
            assert filename == str(FIXTURE)
            assert line == fixture_macro_line(name)


@pytest.mark.parametrize('flags', [0, 0x800, 0x1000, 0x1800])
def test_disconnected_weapon_assertion_polarity(compiled_assertions, flags):
    obj, _ = compiled_assertions
    with quiet_unicorn_windows_exceptions():
        calls, diagnostics = run_probe(obj, 'assertion_disconnected_weapon', flags)
    assert calls.count('_assertion_condition_probe') == 1
    assert bool(diagnostics) == bool(flags & 0x800)
    assert ('_csprintf' in calls) == bool(flags & 0x800)
