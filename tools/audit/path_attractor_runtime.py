"""Bounded execution of the natural, deliberately non-exact Path attractor pair.

Both machine bodies execute; only assert/exit are modeled. Simple exactly
representable vectors cover January's unusual signed projection, endpoint
fallback, degenerate segments and radius branches. This is not an exhaustive
IEEE equivalence proof: the helper has a real x87 evaluation-order residual.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

import unicorn as u
from unicorn.x86_const import *

ROOT = next(p for p in Path(__file__).resolve().parents if (p / 'tools/coff_compare.py').is_file())
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc
from tools.campaign._coff_tools import function_sections

CLOSEST, WEIGHT = '_closest_point_to_attractor', '_path_attractor_weight'
CODE, DATA, OUT, STACK = 0x100000, 0x200000, 0x210000, 0x300000
SP, STOP, RETURN = STACK + 0x8000, CODE + 0xF000, CODE + 0xE000
P0, P1, STATE, RESULT, DISTANCE, VALUE = DATA + 0x100, DATA + 0x200, DATA + 0x400, OUT + 0x100, OUT + 0x200, OUT + 0x300
Q = STATE + 0x28
FUNCTIONS = {CLOSEST: CODE, WEIGHT: CODE + 0x1000}
ASSERT, EXIT = CODE + 0xC000, CODE + 0xD000
FILE_SYMBOL = '??_C@_0BJ@IILBBPFP@c?3?2halo?2SOURCE?2ai?2path?4c?$AA@'
ASSERT_SYMBOL = '??_C@_0BD@GFDHLODO@distance_reference?$AA@'
LITERALS = {
    '__real@00000000': (DATA + 0x800, b'\0' * 4),
    '__real@3f800000': (DATA + 0x804, struct.pack('<f', 1.0)),
    FILE_SYMBOL: (DATA + 0x900, b'c:\\halo\\SOURCE\\ai\\path.c\0'),
    ASSERT_SYMBOL: (DATA + 0xA00, b'distance_reference\0'),
}
PATHS = {
    'target': ROOT / 'scratch/path-attractor-before-20260906/files/build/split/source/ai/path.obj',
    'first': ROOT / 'scratch/path-attractor-first-20260906.obj',
    'actual': ROOT / 'scratch/path-attractor-first-20260906/source/ai/path.obj',
    'source': ROOT / 'scratch/path-attractor-first-20260906/files/source/ai/path.c',
    'header': ROOT / 'scratch/path-attractor-first-20260906/files/source/ai/path.h',
}
PINS = {
    'target': '1b989e4dc3bf3d9623cec07fa19164794044eb0bf359cf08480cf0ea8b3a709c',
    'first': '5a038c57b6f10b647dc1b28f5f5481347f8060064cb3e15b5ff1fdef1f3187a7',
    'actual': 'f1ad2465e98fc17c0d3eecc15ac2b208f891cfbebf1f71699c01f7d6fd6a6ae4',
    'source': 'e1ce54c7a73bdbbcaf30eaf67a86d722e264ecc261356d72d4aca0c6348e75b7',
    'header': '6a728e532f5df0ed5879af0b0c94b65f43d23f465b07e31503f04d6ef30aea16',
}
SAVED = {UC_X86_REG_EBP: 0x12345678, UC_X86_REG_EBX: 0x23456789,
    UC_X86_REG_ESI: 0x3456789A, UC_X86_REG_EDI: 0x456789AB}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def projected(p0, p1, q):
    segment = tuple(b - a for a, b in zip(p0, p1))
    denominator = sum(v * v for v in segment)
    if denominator == 0:
        return (math.nan,) * 3
    t = sum((a - b) * v for a, b, v in zip(p0, q, segment)) / denominator
    return p1 if t < 0 or t > 1 else tuple(a + v * t for a, v in zip(p0, segment))


def cases():
    result = []
    for axis in range(3):
        p0 = (0.0, 0.0, 0.0)
        p1 = tuple(2.0 if i == axis else 0.0 for i in range(3))
        for along in (-3.0, -2.0, -1.0, 0.0, 1.0):
            q = tuple(along if i == axis else (1.0 if i == (axis + 1) % 3 else 0.0) for i in range(3))
            result.append({'name': f'axis{axis}:{along}', 'p0': p0, 'p1': p1, 'q': q})
    result += [
        {'name': 'translated_oblique', 'p0': (1., 2., 3.), 'p1': (3., 4., 5.), 'q': (0., 1., 2.)},
        {'name': 'degenerate', 'p0': (1., 2., 3.), 'p1': (1., 2., 3.), 'q': (0., 0., 0.)},
    ]
    corpus = []
    for geometry in result:
        corpus.append(dict(geometry, entry=CLOSEST, radius=4., weight=2., null=False))
        for radius in (0., 1., 2., 4., -4.):
            corpus.append(dict(geometry, name=geometry['name'] + f':radius{radius}', entry=WEIGHT, radius=radius, weight=2., null=False))
    corpus.append(dict(result[0], name='null_distance_assert', entry=WEIGHT, radius=4., weight=2., null=True))
    return corpus


class Loader:
    def __init__(self, path, patches=None):
        self.obj = cc.load(path)
        self.sections = function_sections(self.obj)
        self.cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        for base, size in ((CODE, 0x10000), (DATA, 0x1000), (OUT, 0x1000), (STACK, 0x10000)):
            self.cpu.mem_map(base, size)
        symbols = dict(FUNCTIONS, _display_assert=ASSERT, _system_exit=EXIT)
        symbols.update({name: value[0] for name, value in LITERALS.items()})
        for name, address in FUNCTIONS.items():
            section = self.sections[name]
            raw = cc._section_bytes(self.obj, self.obj['sections'][section.section_number - 1])
            for offset, expected, replacement in (patches or {}).get(name, []):
                assert raw[offset] == expected
                raw[offset] = replacement
            for relocation in section.info['relocations']:
                target = relocation.get('symbolic_target', relocation['target'])
                if target[0] == 'internal':
                    destination = address + target[1]
                else:
                    assert target[0] == 'symbol', target
                    destination = symbols[target[1]] + target[2]
                    if target[1] in LITERALS:
                        defined = next((s for s in self.obj['symbols'] if s['name'] == target[1] and s['section'] > 0), None)
                        if defined:
                            data = bytes(cc._section_bytes(self.obj, self.obj['sections'][defined['section'] - 1]))[defined['value']:]
                            assert data == LITERALS[target[1]][1]
                if relocation['type'] == cc.IMAGE_REL_I386_REL32:
                    destination -= address + relocation['address'] + 4
                else:
                    assert relocation['type'] == cc.IMAGE_REL_I386_DIR32
                struct.pack_into('<I', raw, relocation['address'], destination & 0xFFFFFFFF)
            self.cpu.mem_write(address, bytes(raw))
        self.cpu.mem_write(ASSERT, b'\xC3')
        self.cpu.mem_write(RETURN, b'\xD9\x1D' + struct.pack('<I', VALUE) + b'\xE9' + struct.pack('<i', STOP - RETURN - 11))
        self.cpu.mem_protect(CODE, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        self.cpu.mem_protect(DATA, 0x1000, u.UC_PROT_READ)

    def run(self, case, control):
        cpu = self.cpu
        inputs = bytearray(b'\xA5' * 0x1000)
        for address, values in ((P0, case['p0']), (P1, case['p1']), (Q, case['q'])):
            inputs[address - DATA:address - DATA + 12] = struct.pack('<3f', *values)
        struct.pack_into('<2f', inputs, STATE + 0x38 - DATA, case['radius'], case['weight'])
        for address, data in LITERALS.values():
            inputs[address - DATA:address - DATA + len(data)] = data
        cpu.mem_write(DATA, bytes(inputs))
        before = b'\x5A' * 0x1000
        cpu.mem_write(OUT, before)
        args = (P0, P1, Q, RESULT) if case['entry'] == CLOSEST else (STATE, P0, P1, 0 if case['null'] else DISTANCE)
        stack = bytearray(b'\xA6' * 0x10000)
        struct.pack_into('<5I', stack, SP - STACK, STOP if case['entry'] == CLOSEST else RETURN, *args)
        cpu.mem_write(STACK, bytes(stack))
        trace, events = [], []

        def on_code(emulator, address, size, unused):
            if address in FUNCTIONS.values():
                name = next(n for n, a in FUNCTIONS.items() if a == address)
                trace.append(name)
                if name == CLOSEST:
                    actual = struct.unpack('<4I', cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 4, 16))
                    assert actual[:3] == (P0, P1, Q)
                return
            if address == ASSERT:
                actual = struct.unpack('<4I', cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 4, 16))
                assert case['null'] and actual == (LITERALS[ASSERT_SYMBOL][0], LITERALS[FILE_SYMBOL][0], 1631, 1)
                events.append('assert')
            elif address == EXIT:
                assert events == ['assert']
                assert struct.unpack('<I', cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 4, 4))[0] == 0xFFFFFFFF
                events.append('exit')
                cpu.emu_stop()
            else:
                assert RETURN <= address < RETURN + 11 or any(a <= address and address + size <= a + self.sections[n].info['size'] for n, a in FUNCTIONS.items())

        def on_write(emulator, access, address, size, value, unused):
            valid = STACK + 64 <= address and address + size <= SP
            valid |= case['entry'] == CLOSEST and address == SP + 8 and size == 4
            valid |= case['entry'] == CLOSEST and RESULT <= address and address + size <= RESULT + 12
            valid |= case['entry'] == WEIGHT and not case['null'] and (address, size) in ((DISTANCE, 4), (VALUE, 4))
            assert valid, ('write_boundary', address, size)

        hooks = [cpu.hook_add(u.UC_HOOK_CODE, on_code), cpu.hook_add(u.UC_HOOK_MEM_WRITE, on_write)]
        cpu.reg_write(UC_X86_REG_ESP, SP)
        cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
        cpu.reg_write(UC_X86_REG_FPCW, control)
        cpu.reg_write(UC_X86_REG_FPSW, 0)
        cpu.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        initial_tag = cpu.reg_read(UC_X86_REG_FPTAG)
        for reg, value in SAVED.items():
            cpu.reg_write(reg, value)
        try:
            cpu.emu_start(FUNCTIONS[case['entry']], STOP, count=10000)
        finally:
            for hook in hooks:
                cpu.hook_del(hook)
        output = bytes(cpu.mem_read(OUT, 0x1000))
        assert bytes(cpu.mem_read(DATA, 0x1000)) == inputs
        assert bytes(cpu.mem_read(STACK, 64)) == stack[:64]
        assert cpu.reg_read(UC_X86_REG_FPCW) == control
        assert not cpu.reg_read(UC_X86_REG_EFLAGS) & 0x400
        assert trace == ([CLOSEST] if case['entry'] == CLOSEST else [WEIGHT, CLOSEST])
        if case['null']:
            assert events == ['assert', 'exit'] and output == before
        else:
            assert not events and cpu.reg_read(UC_X86_REG_EIP) == STOP
            assert cpu.reg_read(UC_X86_REG_ESP) == SP + 4
            assert all(cpu.reg_read(r) == value for r, value in SAVED.items())
            assert cpu.reg_read(UC_X86_REG_FPTAG) == initial_tag and not cpu.reg_read(UC_X86_REG_FPSW) & 0x3800
            expected = bytearray(before)
            point = projected(case['p0'], case['p1'], case['q'])
            if case['entry'] == CLOSEST:
                actual = struct.unpack('<3f', output[RESULT - OUT:RESULT - OUT + 12])
                assert all((math.isnan(a) and math.isnan(b)) or a == b for a, b in zip(actual, point)), ('projection_result', actual, point)
                expected[RESULT - OUT:RESULT - OUT + 12] = output[RESULT - OUT:RESULT - OUT + 12]
            else:
                squared = sum((a - b) ** 2 for a, b in zip(point, case['q']))
                distance, weight = 3.4028234663852886e38, 0.0
                if squared < case['radius'] ** 2:
                    distance = math.sqrt(squared)
                    weight = (1 - distance / case['radius']) * case['weight']
                expected[DISTANCE - OUT:DISTANCE - OUT + 4] = struct.pack('<f', distance)
                expected[VALUE - OUT:VALUE - OUT + 4] = struct.pack('<f', weight)
            assert output == expected, ('result_memory', case['name'])
            # January deliberately reuses helper's p1 parameter slot for t.
            if case['entry'] == CLOSEST:
                stack[SP - STACK + 8:SP - STACK + 12] = cpu.mem_read(SP + 8, 4)
            assert bytes(cpu.mem_read(SP, 0x8000)) == stack[0x8000:]
        return {'output': output.hex(), 'trace': trace, 'events': events,
            'fpsw': cpu.reg_read(UC_X86_REG_FPSW), 'input_immutable': True, 'guards': True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    for role, path in PATHS.items():
        assert digest(path.read_bytes()) == PINS[role], role
    loaders = {role: Loader(PATHS[role]) for role in ('target', 'first', 'actual')}
    results = []
    for case in cases():
        for control in (0x027F, 0x037F):
            outputs = {role: loader.run(case, control) for role, loader in loaders.items()}
            assert outputs['target'] == outputs['first'] == outputs['actual'], case['name']
            results.append({'case': case['name'], 'control': control, 'outcome_sha256': digest(json.dumps(outputs['target'], sort_keys=True).encode())})
    negative = Loader(PATHS['target'], {CLOSEST: [(0x2B, 0x20, 0x00)]})
    try:
        negative.run(next(c for c in cases() if c['name'] == 'axis0:-1.0'), 0x037F)
    except AssertionError as error:
        assert error.args[0][0] == 'projection_result', repr(error)
    else:
        raise AssertionError('wrong signed projection instruction was not detected')
    report = {'scope': __doc__, 'pins': PINS, 'tool_sha256': digest(Path(__file__).read_bytes()),
        'scenario_count': len(cases()), 'execution_count': len(results) * 3, 'all_pass': True,
        'negative_control': 'host-only target fsub [eax] changed to fadd [eax]; projection_result rejects',
        'results': results}
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('scenario_count', 'execution_count', 'all_pass', 'negative_control')}))


if __name__ == '__main__':
    main()
