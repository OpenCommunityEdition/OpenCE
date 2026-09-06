"""Target-first bounded replay of the 24-byte January obstacle-disc append API.

The actual function body executes for every role. Only display_assert and
nonreturning system_exit are modeled. The real projection table is extracted
from independently pinned January/current RealMath objects. Entire container,
height, padding, counters, capacity behavior and return ABI are verified.
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

FUNCTION = '_obstacles_add_disc'
CODE, DATA, OUTPUT, STACK = 0x100000, 0x200000, 0x210000, 0x300000
ASSERT, EXIT, STOP, SP = CODE + 0x1000, CODE + 0x2000, CODE + 0xF000, STACK + 0x8000
CENTER, MAPPING, OBSTACLES = DATA + 0x100, DATA + 0x200, OUTPUT + 0x100
DISC_SIZE, CONTAINER_SIZE, CAPACITY = 0x18, 0xC08, 128
FILE = '??_C@_0CD@MNFANEEP@c?3?2halo?2SOURCE?2ai?2path_obstacles@'
DISC_ASSERT = '??_C@_0EG@PILJPJIJ@obstacles?9?$DOdisc_count?$DO?$DN0?5?$CG?$CG?5obst@'
OBSTACLE_ASSERT = '??_C@_0FB@IMJPHNIA@obstacles?9?$DOobstacle_count?$DO?$DN0?5?$CG?$CG?5@'
LITERALS = {
    FILE: (DATA + 0x300, b'c:\\halo\\SOURCE\\ai\\path_obstacles.c\0'),
    DISC_ASSERT: (DATA + 0x400, b'obstacles->disc_count>=0 && obstacles->disc_count<=MAXIMUM_DISC_COUNT\0'),
    OBSTACLE_ASSERT: (DATA + 0x500, b'obstacles->obstacle_count>=0 && obstacles->obstacle_count<=obstacles->disc_count\0'),
}
PATHS = {
    'target': ROOT / 'scratch/path-obstacles-disc-before-20260906/files/build/split/source/ai/path_obstacles.obj',
    'first': ROOT / 'scratch/path-obstacles-disc-first-20260906.obj',
    'actual': ROOT / 'scratch/path-obstacles-disc-final-20260906/source/ai/path_obstacles.obj',
    'source': ROOT / 'scratch/path-obstacles-disc-final-20260906/files/source/ai/path_obstacles.c',
    'avoidance': ROOT / 'scratch/path-obstacles-disc-final-20260906/files/source/ai/path_obstacle_avoidance.c',
    'header': ROOT / 'scratch/path-obstacles-disc-final-20260906/files/source/ai/path.h',
    'first_source': ROOT / 'scratch/path-obstacles-disc-first-20260906/files/source/ai/path_obstacles.c',
    'first_header': ROOT / 'scratch/path-obstacles-disc-first-20260906/files/source/ai/path.h',
    'math_target': ROOT / 'scratch/path-obstacles-disc-math-target-20260906.obj',
    'math_base': ROOT / 'scratch/path-obstacles-disc-math-base-20260906.obj',
}
PINS = {
    'target': '8d8a2106cce26a6c271939867ccb6088d9a132ac4f7ddf8cd7c749489e329bc3',
    'first': 'c58e42656fcc61b7fe18a44b4491c0ab86b1c54d6b068eba17cd250527d50696',
    'actual': 'f047e98c104cce284b4bb6aa1a62876bb2fb29fa0c1a8bf3f92beaa16339f065',
    'source': '673b2024a44b2e838bbf4e1cb1ff3131f6a97661e297cd304cfe42bd578cae1f',
    'avoidance': '88b882f0f618ca97dd83dbeb18af9daa692e9fd799883945a7b9b354976f70dd',
    'header': 'de5430e9a180de843feaacdd13276f65047b68a783641603bbcceed46a003eec',
    'first_source': 'ed9b86ccce3bb45767aa4f8aff42e11362715ad71d76e06fd7d93b8df6a6f798',
    'first_header': '0d7574f5b7757a628d10d984a446d35cc67e614cf9c26baa2325d91461c12e85',
    'math_target': '93e4b179d3e52fe3d62a998ba3951a42eb57dddd2790713dee08e1de91780460',
    'math_base': '0ce96c40a06a39ec5742710e962f69121833314adae49bb32fea9dad0497bf43',
}
SAVED = {UC_X86_REG_EBP: 0x12345678, UC_X86_REG_EBX: 0x23456789,
    UC_X86_REG_ESI: 0x3456789A, UC_X86_REG_EDI: 0x456789AB}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def cases():
    result = []
    payloads = ((0x3FA00000, 0xC0200000, 0x40700000, 0x3F000000),
                (0x80000000, 0x7FC12345, 0x7F800000, 0x80000000))
    for count in (0, 1, 17, 127, 128):
        for flags in (0, 1, 2, 3, 0x8000, 0x8001, 0x7FFF, 0xFFFF):
            for payload in payloads:
                result.append({'name': f'{count}:{flags:04x}:{payload[0]:08x}',
                    'count': count, 'obstacle_count': count // 2, 'optional_count': count // 3,
                    'flags': flags, 'center': payload[:3], 'radius': payload[3],
                    'object': 0xFFFFFFFF if payload[0] == 0x80000000 else 0x12345678, 'assert_line': None})
    for name, count, obstacle_count, line in (
        ('negative_disc_count', -1, 0, 104), ('overflow_disc_count', 129, 0, 104),
        ('negative_obstacle_count', 1, -1, 105), ('excess_obstacle_count', 1, 2, 105)):
        result.append({'name': name, 'count': count, 'obstacle_count': obstacle_count,
            'optional_count': 0, 'flags': 1, 'center': payloads[0][:3],
            'radius': payloads[0][3], 'object': 0x12345678, 'assert_line': line})
    return result


def expected_output(case, before):
    result = bytearray(before)
    if case['assert_line'] is not None or case['count'] == CAPACITY:
        return bytes(result)
    base = OBSTACLES - OUTPUT
    struct.pack_into('<h', result, base + 2, case['count'] + 1)
    if case['flags'] & 1:
        struct.pack_into('<h', result, base + 4, case['optional_count'] + 1)
    disc = base + 8 + DISC_SIZE * case['count']
    struct.pack_into('<HHI4I', result, disc, case['flags'], 0xFFFF, case['object'],
        case['center'][0], case['center'][1], case['radius'], case['center'][2])
    return bytes(result)


def projection_table(path):
    obj = cc.load(path)
    symbol = cc.symbol(obj, '_global_projection3d_mappings')
    data = bytes(cc._section_bytes(obj, obj['sections'][symbol['section'] - 1]))
    return data[symbol['value']:symbol['value'] + 24]


class Loader:
    def __init__(self, path, mapping, patch=None):
        self.obj = cc.load(path)
        self.info = cc.section_info(self.obj, FUNCTION)
        symbol = cc.symbol(self.obj, FUNCTION)
        raw = cc._section_bytes(self.obj, self.obj['sections'][symbol['section'] - 1])
        if patch:
            offset, expected, replacement = patch
            assert raw[offset] == expected
            raw[offset] = replacement
        destinations = {'_display_assert': ASSERT, '_system_exit': EXIT,
            '_global_projection3d_mappings': MAPPING}
        destinations.update({name: pair[0] for name, pair in LITERALS.items()})
        for relocation in self.info['relocations']:
            target = relocation.get('symbolic_target', relocation['target'])
            assert target[0] == 'symbol', target
            name, addend = target[1:]
            destination = destinations[name] + addend
            if name in LITERALS:
                owner = cc.symbol(self.obj, name)
                assert owner['section'] > 0
                data = bytes(cc._section_bytes(self.obj, self.obj['sections'][owner['section'] - 1]))[owner['value']:]
                assert data == LITERALS[name][1]
            if relocation['type'] == cc.IMAGE_REL_I386_REL32:
                destination -= CODE + relocation['address'] + 4
            else:
                assert relocation['type'] == cc.IMAGE_REL_I386_DIR32
            struct.pack_into('<I', raw, relocation['address'], destination & 0xFFFFFFFF)
        self.cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        for base, size in ((CODE, 0x10000), (DATA, 0x1000), (OUTPUT, 0x1000), (STACK, 0x10000)):
            self.cpu.mem_map(base, size)
        self.cpu.mem_write(CODE, bytes(raw))
        self.cpu.mem_write(ASSERT, b'\xC3')
        self.cpu.mem_protect(CODE, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        self.cpu.mem_protect(DATA, 0x1000, u.UC_PROT_READ)
        self.mapping = mapping

    def run(self, case, control):
        cpu = self.cpu
        inputs = bytearray(b'\xC5' * 0x1000)
        inputs[CENTER - DATA:CENTER - DATA + 12] = struct.pack('<3I', *case['center'])
        inputs[MAPPING - DATA:MAPPING - DATA + 24] = self.mapping
        for address, payload in LITERALS.values():
            inputs[address - DATA:address - DATA + len(payload)] = payload
        cpu.mem_write(DATA, bytes(inputs))
        before = bytearray((i * 19 + 7) & 255 for i in range(0x1000))
        struct.pack_into('<3h', before, OBSTACLES - OUTPUT, case['obstacle_count'], case['count'], case['optional_count'])
        cpu.mem_write(OUTPUT, bytes(before))
        stack = bytearray(b'\xA6' * 0x10000)
        # Full capacity and assertion branches must not dereference center.
        center = CENTER if case['assert_line'] is None and case['count'] < CAPACITY else 0
        struct.pack_into('<6I', stack, SP - STACK, STOP, OBSTACLES, case['object'],
            0xBEEF0000 | case['flags'], center, case['radius'])
        cpu.mem_write(STACK, bytes(stack))
        events = []

        def on_code(emulator, address, size, unused):
            if address == ASSERT:
                actual = struct.unpack('<4I', cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 4, 16))
                expression = DISC_ASSERT if case['assert_line'] == 104 else OBSTACLE_ASSERT
                assert case['assert_line'] is not None and actual == (LITERALS[expression][0], LITERALS[FILE][0], case['assert_line'], 1)
                events.append(['assert', case['assert_line']])
            elif address == EXIT:
                assert events == [['assert', case['assert_line']]]
                assert struct.unpack('<I', cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 4, 4))[0] == 0xFFFFFFFF
                events.append(['exit', -1])
                cpu.emu_stop()
            else:
                assert CODE <= address and address + size <= CODE + self.info['size']

        def on_write(emulator, access, address, size, value, unused):
            valid = STACK + 64 <= address and address + size <= SP
            valid |= OBSTACLES <= address and address + size <= OBSTACLES + CONTAINER_SIZE
            assert valid, ('write_boundary', address, size)

        hooks = [cpu.hook_add(u.UC_HOOK_CODE, on_code), cpu.hook_add(u.UC_HOOK_MEM_WRITE, on_write)]
        cpu.reg_write(UC_X86_REG_ESP, SP)
        cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
        cpu.reg_write(UC_X86_REG_FPCW, control)
        cpu.reg_write(UC_X86_REG_FPSW, 0)
        cpu.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        tag = cpu.reg_read(UC_X86_REG_FPTAG)
        for register, value in SAVED.items():
            cpu.reg_write(register, value)
        try:
            cpu.emu_start(CODE, STOP, count=10000)
        finally:
            for hook in hooks:
                cpu.hook_del(hook)
        output = bytes(cpu.mem_read(OUTPUT, 0x1000))
        assert output == expected_output(case, before), ('container_result', case['name'])
        assert bytes(cpu.mem_read(DATA, 0x1000)) == inputs
        assert bytes(cpu.mem_read(SP, 0x8000)) == stack[0x8000:]
        assert bytes(cpu.mem_read(STACK, 64)) == stack[:64]
        assert cpu.reg_read(UC_X86_REG_FPCW) == control and cpu.reg_read(UC_X86_REG_FPTAG) == tag
        assert not cpu.reg_read(UC_X86_REG_FPSW) & 0x3800
        assert not cpu.reg_read(UC_X86_REG_EFLAGS) & 0x400
        al = None
        if case['assert_line'] is None:
            al = cpu.reg_read(UC_X86_REG_EAX) & 255
            assert al == int(case['count'] < CAPACITY) and not events
            assert cpu.reg_read(UC_X86_REG_EIP) == STOP and cpu.reg_read(UC_X86_REG_ESP) == SP + 4
            assert all(cpu.reg_read(r) == value for r, value in SAVED.items())
        else:
            assert events == [['assert', case['assert_line']], ['exit', -1]]
        return {'al': al, 'events': events, 'output_sha256': digest(output),
            'fpsw': cpu.reg_read(UC_X86_REG_FPSW), 'input_immutable': True, 'stack_guards': True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    for role, path in PATHS.items():
        assert digest(path.read_bytes()) == PINS[role], role
    mapping = projection_table(PATHS['math_target'])
    assert mapping == projection_table(PATHS['math_base'])
    assert struct.unpack('<12h', mapping) == (2, 1, 1, 2, 0, 2, 2, 0, 1, 0, 0, 1)
    loaders = {role: Loader(PATHS[role], mapping) for role in ('target', 'first', 'actual')}
    results = []
    for case in cases():
        for control in (0x027F, 0x037F):
            outputs = {role: loader.run(case, control) for role, loader in loaders.items()}
            assert outputs['target'] == outputs['first'] == outputs['actual'], case['name']
            results.append({'case': case['name'], 'control': control, 'outcome': outputs['target']})
    negative_case = next(c for c in cases() if c['count'] == 1 and c['flags'] == 1)
    negatives = []
    for label, patch in (('wrong_disc_stride', (0x7F, 0x40, 0x00)), ('wrong_height_source', (0xBF, 8, 4))):
        try:
            Loader(PATHS['target'], mapping, patch).run(negative_case, 0x037F)
        except AssertionError as error:
            assert error.args[0][0] == 'container_result', repr(error)
            negatives.append({'label': label, 'offset': patch[0], 'expected': patch[1], 'replacement': patch[2], 'detected_by': 'container_result'})
        else:
            raise AssertionError('target-byte mutation not detected: ' + label)
    report = {'scope': __doc__, 'pins': PINS, 'tool_sha256': digest(Path(__file__).read_bytes()),
        'scenario_count': len(cases()), 'execution_count': len(results) * 3, 'all_pass': True,
        'negative_controls': negatives, 'projection_table_hex': mapping.hex(),
        'limitations': ['No arbitrary pointer aliasing, signaling NaNs or exhaustive floating-point claim.',
            'The other incomplete obstacle functions are not implemented or approved by this replay.'],
        'results': results}
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('scenario_count', 'execution_count', 'all_pass', 'negative_controls')}))


if __name__ == '__main__':
    main()
