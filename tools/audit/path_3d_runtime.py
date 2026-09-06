"""Bounded target-first execution of the two public Path 3D functions.

Both genuine machine bodies execute, including build_path's call to available.
Only three real external APIs are modeled: typed tag-block lookup, collision
query, and csmemset. This verifies their call contracts, not collision physics.
The complete 0x5c path result and optional outputs are checked. The authentic
compiler reuse of a parameter-slot byte is permitted explicitly. Avoidance
bits are forwarded but unused, as in January; no private ABI is invented.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

import unicorn as u
from unicorn.x86_const import *

ROOT = next(p for p in Path(__file__).resolve().parents if (p / 'tools/coff_compare.py').is_file())
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc
from tools.campaign._coff_tools import function_sections

AVAILABLE, BUILD = '_path_3d_available', '_path_3d_build_path'
TAG, COLLISION, MEMSET = '_tag_block_get_element_with_size', '_collision_bsp_test_vector', '_csmemset'
CODE, STUB, INPUT, OUTPUT, STACK = 0x100000, 0x110000, 0x200000, 0x210000, 0x300000
STOP, SP = CODE + 0xF000, STACK + 0x8000
STRUCTURE, START, END, BSP = INPUT + 0x100, INPUT + 0x800, INPUT + 0x900, INPUT + 0xC00
FINISH, ENDPOINT, PATH = OUTPUT + 0x100, OUTPUT + 0x200, OUTPUT + 0x400
FUNCTIONS = {AVAILABLE: CODE, BUILD: CODE + 0x1000}
STUBS = {TAG: STUB, COLLISION: STUB + 0x100, MEMSET: STUB + 0x200}
LITERALS = {'__real@3f800000': (INPUT + 0x1800, struct.pack('<f', 1.0)),
    '__real@3dcccccd': (INPUT + 0x1804, struct.pack('<f', 0.1))}
PATHS = {
    'target': ROOT / 'scratch/path-3d-before-20260906/files/build/split/source/ai/path.obj',
    'first': ROOT / 'scratch/path-3d-first-20260906.obj',
    'actual': ROOT / 'scratch/path-3d-first-20260906/source/ai/path.obj',
    'source': ROOT / 'scratch/path-3d-first-20260906/files/source/ai/path.c',
    'header': ROOT / 'scratch/path-3d-first-20260906/files/source/ai/path.h',
}
PINS = {
    'target': '1b989e4dc3bf3d9623cec07fa19164794044eb0bf359cf08480cf0ea8b3a709c',
    'first': '49c38a22e31a3bd9c63dda20ec5aaa0ca086c732c1f5b426fd61519f3e317f7c',
    'actual': 'b4eaa84ee1c42250a24879f3a846fef236904440bba63330b2c730365f245d40',
    'source': '19ac10b95d6d91d2fcb44a9a8f1f7074d0c3192d5f6cc7ac55f6c678dbb35ff2',
    'header': '4454550317b6b9e9a31c3911400274b1c0937a8cccabcd474b3c2b63b6dd49c0',
}
SAVED = {UC_X86_REG_EBP: 0x12345678, UC_X86_REG_EBX: 0x23456789,
    UC_X86_REG_ESI: 0x3456789A, UC_X86_REG_EDI: 0x456789AB}
START_BYTES, END_BYTES = struct.pack('<3f', -1.0, 2.0, 0.5), struct.pack('<3f', 0.0, 2.0, 0.5)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def contains(base, length, address, size):
    return base <= address and address + size <= base + length


def cases():
    branches = (
        ('no_hit_poison_t', 0, 0x7FC00000, True),
        ('hit_endpoint', 1, 0x3F800000, True),
        ('hit_near_endpoint', 1, 0x3F400000, True),
        ('hit_blocked', 1, 0x3F000000, False),
        ('strict_threshold_outside', 1, 0x3F2F0BB2, False),
        ('strict_threshold_inside', 1, 0x3F2F0BB3, True),
        ('hit_nan_t', 1, 0x7FC00000, False),
    )
    result = []
    for name, hit, t_bits, available in branches:
        for avoidance in (0, 0x80000000, 0x3F800000, 0x7FC00000, 0xDEADBEEF):
            for outputs in range(4):
                result.append({'name': f'{name}:available:{avoidance:08x}:{outputs}',
                    'entry': AVAILABLE, 'hit': hit, 't_bits': t_bits, 'available': available,
                    'avoidance': avoidance, 'outputs': outputs})
            result.append({'name': f'{name}:build:{avoidance:08x}', 'entry': BUILD,
                'hit': hit, 't_bits': t_bits, 'available': available,
                'avoidance': avoidance, 'outputs': 3})
    return result


def expected_output(case, before):
    after = bytearray(before)
    if case['entry'] == AVAILABLE:
        if case['outputs'] & 1:
            after[FINISH - OUTPUT] = int(case['available'])
        if case['outputs'] & 2:
            after[ENDPOINT - OUTPUT:ENDPOINT - OUTPUT + 12] = END_BYTES
    else:
        result = bytearray(0x5C)
        if case['available']:
            result[0] = 1
            result[4:16] = END_BYTES
            result[0x10:0x14] = b'\xFF' * 4
            result[0x18:0x1A] = b'\x01\x01'
            result[0x1C:0x20] = b'\xFF' * 4
            result[0x20:0x2C] = END_BYTES
        after[PATH - OUTPUT:PATH - OUTPUT + 0x5C] = result
    return bytes(after)


class Loader:
    def __init__(self, role, path, patches=None):
        self.role, self.obj = role, cc.load(path)
        self.sections = function_sections(self.obj)
        self.uc = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        for base, size in ((CODE, 0x10000), (STUB, 0x1000), (INPUT, 0x2000),
                           (OUTPUT, 0x1000), (STACK, 0x10000)):
            self.uc.mem_map(base, size)
        for address in STUBS.values():
            self.uc.mem_write(address, b'\xC3')
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
                    symbol, addend = target[1:]
                    if symbol in FUNCTIONS:
                        destination = FUNCTIONS[symbol]
                    elif symbol in STUBS:
                        destination = STUBS[symbol]
                    else:
                        destination, expected = LITERALS[symbol]
                        defined = next((s for s in self.obj['symbols'] if s['name'] == symbol and s['section'] > 0), None)
                        if defined:
                            data = bytes(cc._section_bytes(self.obj, self.obj['sections'][defined['section'] - 1]))[defined['value']:]
                            assert data == expected
                    destination += addend
                if relocation['type'] == cc.IMAGE_REL_I386_REL32:
                    destination -= address + relocation['address'] + 4
                else:
                    assert relocation['type'] == cc.IMAGE_REL_I386_DIR32
                struct.pack_into('<I', raw, relocation['address'], destination & 0xFFFFFFFF)
            self.uc.mem_write(address, bytes(raw))
        self.uc.mem_protect(CODE, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        self.uc.mem_protect(STUB, 0x1000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        self.uc.mem_protect(INPUT, 0x2000, u.UC_PROT_READ)

    def run(self, case, control):
        cpu = self.uc
        inputs = bytearray((i * 17 + 3) & 255 for i in range(0x2000))
        inputs[START - INPUT:START - INPUT + 12] = START_BYTES
        inputs[END - INPUT:END - INPUT + 12] = END_BYTES
        struct.pack_into('<3I', inputs, STRUCTURE + 0xB0 - INPUT, 1, BSP, 0)
        for address, payload in LITERALS.values():
            inputs[address - INPUT:address - INPUT + len(payload)] = payload
        cpu.mem_write(INPUT, bytes(inputs))
        outputs = bytes((i * 31 + 7) & 255 for i in range(0x1000))
        cpu.mem_write(OUTPUT, outputs)
        args = [STRUCTURE, START, case['avoidance'], END]
        if case['entry'] == BUILD:
            args.append(PATH)
        else:
            args += [FINISH if case['outputs'] & 1 else 0, ENDPOINT if case['outputs'] & 2 else 0]
        stack = bytearray(b'\x5A' * 0x10000)
        struct.pack_into('<' + 'I' * (len(args) + 1), stack, SP - STACK, STOP, *args)
        cpu.mem_write(STACK, bytes(stack))
        state = {'trace': [], 'events': [], 'result': None, 't_reads': 0}

        def arguments(count):
            return struct.unpack('<' + 'I' * count, cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 4, count * 4))

        def on_code(emulator, address, size, unused):
            if address in FUNCTIONS.values():
                name = next(n for n, a in FUNCTIONS.items() if a == address)
                state['trace'].append(name)
                if name == AVAILABLE:
                    actual = arguments(6)
                    assert actual[:4] == (STRUCTURE, START, case['avoidance'], END), ('forwarding', actual)
                return
            name = next((n for n, a in STUBS.items() if a == address), None)
            if name is None:
                assert any(contains(a, self.sections[n].info['size'], address, size) for n, a in FUNCTIONS.items())
                return
            if name == TAG:
                actual = arguments(3)
                assert actual == (STRUCTURE + 0xB0, 0, 0x60), ('tag_arguments', actual)
                result = BSP
                state['events'].append([name, *actual, result])
            elif name == COLLISION:
                actual = arguments(8)
                assert actual[:5] == (1, BSP, 0, 0, START), ('collision_arguments', actual)
                assert actual[6] == 0x7F7FFFFF
                assert contains(STACK, SP - STACK, actual[5], 12)
                assert bytes(cpu.mem_read(actual[5], 12)) == struct.pack('<3f', 1.0, 0.0, 0.0)
                assert contains(STACK, SP - STACK, actual[7], 0x418)
                cpu.mem_write(actual[7], struct.pack('<I', case['t_bits']) + b'\xA5' * (0x418 - 4))
                state['result'] = actual[7]
                result = case['hit']
                state['events'].append([name, *actual, result, case['t_bits']])
            else:
                actual = arguments(3)
                assert actual == (PATH, 0, 0x5C), ('memset_arguments', actual)
                cpu.mem_write(PATH, b'\0' * 0x5C)
                result = PATH
                state['events'].append([name, *actual])
            cpu.reg_write(UC_X86_REG_EAX, 0xABCD1200 | result if name == COLLISION else result)
            cpu.reg_write(UC_X86_REG_ECX, 0xA1B2C3D4)
            cpu.reg_write(UC_X86_REG_EDX, 0xB2C3D4E5)

        def on_memory(emulator, access, address, size, value, unused):
            if access == u.UC_MEM_WRITE:
                permitted = contains(STACK + 64, SP - STACK - 64, address, size)
                if case['entry'] == BUILD:
                    permitted |= address == SP + 0x17 and size == 1
                    permitted |= contains(PATH, 0x5C, address, size)
                else:
                    permitted |= bool(case['outputs'] & 1) and contains(FINISH, 1, address, size)
                    permitted |= bool(case['outputs'] & 2) and contains(ENDPOINT, 12, address, size)
                assert permitted, ('forbidden_write', address, size)
            elif state['result'] is not None and address == state['result']:
                assert size == 4
                state['t_reads'] += 1

        hooks = [cpu.hook_add(u.UC_HOOK_CODE, on_code),
                 cpu.hook_add(u.UC_HOOK_MEM_READ | u.UC_HOOK_MEM_WRITE, on_memory)]
        cpu.reg_write(UC_X86_REG_ESP, SP)
        cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
        cpu.reg_write(UC_X86_REG_FPCW, control)
        cpu.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        cpu.reg_write(UC_X86_REG_FPSW, 0)
        tag = cpu.reg_read(UC_X86_REG_FPTAG)
        for register, value in SAVED.items():
            cpu.reg_write(register, value)
        try:
            cpu.emu_start(FUNCTIONS[case['entry']], STOP, count=50000)
        finally:
            for hook in hooks:
                cpu.hook_del(hook)
        eax = cpu.reg_read(UC_X86_REG_EAX)
        assert eax & 255 == int(case['available']), ('return_boolean', eax & 255, case['available'])
        assert cpu.reg_read(UC_X86_REG_EIP) == STOP and cpu.reg_read(UC_X86_REG_ESP) == SP + 4
        assert all(cpu.reg_read(r) == value for r, value in SAVED.items())
        assert cpu.reg_read(UC_X86_REG_FPCW) == control
        assert cpu.reg_read(UC_X86_REG_FPTAG) == tag and not cpu.reg_read(UC_X86_REG_FPSW) & 0x3800
        assert not cpu.reg_read(UC_X86_REG_EFLAGS) & 0x400
        assert bytes(cpu.mem_read(INPUT, 0x2000)) == bytes(inputs)
        final_output = bytes(cpu.mem_read(OUTPUT, 0x1000))
        assert final_output == expected_output(case, outputs), ('result_memory', case['name'])
        if case['entry'] == BUILD:
            stack[SP - STACK + 0x17] = int(case['available'])
        assert bytes(cpu.mem_read(SP, 0x10000 - (SP - STACK))) == bytes(stack[SP - STACK:])
        assert bytes(cpu.mem_read(STACK, 64)) == bytes(stack[:64])
        assert state['trace'] == ([AVAILABLE] if case['entry'] == AVAILABLE else [BUILD, AVAILABLE])
        assert [e[0] for e in state['events']] == ([TAG, COLLISION] if case['entry'] == AVAILABLE else [MEMSET, TAG, COLLISION])
        assert (state['t_reads'] > 0) == bool(case['hit']), ('t_short_circuit', state['t_reads'], case['hit'])
        return {'al': eax & 255, 'esp_final': cpu.reg_read(UC_X86_REG_ESP),
            'fpcw': control, 'fpsw': cpu.reg_read(UC_X86_REG_FPSW), 'events': state['events'],
            'trace': state['trace'], 't_reads': state['t_reads'], 'output_sha256': digest(final_output),
            'input_immutable': True, 'stack_and_output_guards': True, 'nonvolatiles_preserved': True,
            'path_result_hex': final_output[PATH - OUTPUT:PATH - OUTPUT + 0x5C].hex() if case['entry'] == BUILD else None}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'scratch/path-3d-runtime-report.json')
    args = parser.parse_args()
    assert not args.output.exists(), 'preserve existing reports'
    for role, path in PATHS.items():
        assert digest(path.read_bytes()) == PINS[role], role
    loaders = {role: Loader(role, PATHS[role]) for role in ('target', 'first', 'actual')}
    results = []
    corpus = cases()
    for case in corpus:
        for control in (0x027F, 0x037F):
            outcomes = {role: loader.run(case, control) for role, loader in loaders.items()}
            assert outcomes['target'] == outcomes['first'] == outcomes['actual'], case['name']
            results.append({'case': case['name'], 'control': control, 'target': outcomes['target'],
                'first_equal': True, 'actual_equal': True})
    negatives = []
    for entry, offset in ((AVAILABLE, 0xC1), (BUILD, 0x80)):
        loader = Loader('negative', PATHS['target'], {entry: [(offset, 1, 0)]})
        case = next(c for c in corpus if c['entry'] == entry and c['available'])
        try:
            loader.run(case, 0x037F)
        except AssertionError as error:
            assert error.args[0][0] == 'return_boolean', repr(error)
            negatives.append({'entry': entry, 'offset': offset, 'expected': 1, 'replacement': 0,
                'detected_by': 'return_boolean', 'kind': 'host copy of target instruction modified only for emulator mapping'})
        else:
            raise AssertionError('wrong boolean mutation not detected')
    report = {'scope': __doc__, 'tool_sha256': digest(Path(__file__).read_bytes()), 'pins': PINS,
        'scenario_count': len(corpus), 'execution_count': len(results) * len(loaders),
        'negative_execution_count': len(negatives), 'all_pass': True, 'negative_controls': negatives,
        'limitations': ['Collision geometry is a genuine external API model, not a physics implementation test.',
            'Finite unit-vector branch samples plus qNaN t and ignored avoidance-bit probes; no exhaustive floating-point claim.',
            'Returns are boolean AL; upper EAX and ordinary arithmetic flags are not return contracts.',
            'Extra ordinary helper COMDATs are not called by these exact bodies and earn no target credit.'],
        'results': results}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('scenario_count', 'execution_count', 'all_pass', 'negative_controls')}))


if __name__ == '__main__':
    main()
