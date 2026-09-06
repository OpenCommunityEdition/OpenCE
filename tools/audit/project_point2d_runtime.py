"""Independent, bounded raw helper probe; not a wrapper or whole-engine test."""
from pathlib import Path
import hashlib
import json
import struct
import sys
import unicorn as u
from unicorn.x86_const import *

ROOT = next(p for p in Path(__file__).resolve().parents if (p / 'tools/coff_compare.py').is_file())
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

PATHS = {
    'target': 'scratch/render-debug-2d-decals-target-20260906.obj',
    'candidate': 'scratch/render-debug-2d-first-20260906.obj',
    'current_collision': 'scratch/render-debug-2d-collision-bsp-base-20260906.obj',
    'current_breakable': 'build/base/source/physics/breakable_surfaces.obj',
}
PIN = {
    'target': 'f4de9ec343021d668ea367880bb8c65e982b9b60b03c2a23014616872f305083',
    'candidate': 'b1c86df56a33151eccc6bcdef73adbb3ded3db3da32a3778c501e59c1781f5b3',
    'current_collision': 'daa47065ad65714b79c2f4ff0db3fab06b004d5ffe22b309d76589d8013e5909',
    'current_breakable': '93bc5bd69da3adab5865b60ad0ca02df28ae533702f81eb45feda1c66dbad333',
}
TABLE = struct.pack('<12h', 2, 1, 1, 2, 0, 2, 2, 0, 1, 0, 0, 1)
LITERALS = {
    '_global_projection3d_mappings': TABLE,
    '__real@3f1a36e2e0000000': bytes.fromhex('000000e0e2361a3f'),
    '__real@00000000': b'\0' * 4,
    '??_C@_0BE@DKJODJEH@?4?4?2math?2real_math?4h?$AA@': b'..\\math\\real_math.h\0',
    '??_C@_0CB@FEGCJDLB@projection?$DO?$DN_x?5?$CG?$CG?5projection?$DM?$DN_z@': b'projection>=_x && projection<=_z\0',
    '??_C@_0L@LKENDBEE@?$HO?$CIsign?$CG?$HO1?$CJ?$AA@': b'~(sign&~1)\0',
}
CODE, STOP, DATA, STACK, SP = 0x100000, 0x100F00, 0x200000, 0x300000, 0x308000
P2, PLANE, P3 = DATA + 0x800, DATA + 0x900, DATA + 0xA00
SAVED = {UC_X86_REG_EBP: 0x12345678, UC_X86_REG_EBX: 0x23456789,
    UC_X86_REG_ESI: 0x3456789A, UC_X86_REG_EDI: 0x456789AB}

def execute(path, projection, sign, normal_bits, control):
    obj = cc.load(path)
    sym = cc.symbol(obj, '_project_point2d')
    section = obj['sections'][sym['section'] - 1]
    raw = cc._section_bytes(obj, section)
    cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    for base, size in ((CODE, 0x1000), (DATA, 0x1000), (STACK, 0x10000)):
        cpu.mem_map(base, size)
    addresses = {name: DATA + index * 0x100 for index, name in enumerate(LITERALS)}
    for name, value in LITERALS.items():
        cpu.mem_write(addresses[name], value)
    addresses.update({'_display_assert': CODE + 0xE00, '_system_exit': CODE + 0xE10})
    # Assertions are trap-only: reaching either is a failed valid-input test.
    cpu.mem_write(CODE + 0xE00, b'\xCC' * 32)
    for relocation in cc.section_info(obj, '_project_point2d')['relocations']:
        target = relocation.get('symbolic_target', relocation['target'])
        assert target[0] == 'symbol', target
        address = addresses[target[1]] + target[2]
        if relocation['type'] == cc.IMAGE_REL_I386_REL32:
            address -= CODE + relocation['address'] + 4
        else:
            assert relocation['type'] == cc.IMAGE_REL_I386_DIR32
        struct.pack_into('<I', raw, relocation['address'], address & 0xFFFFFFFF)
    cpu.mem_write(CODE, bytes(raw))
    cpu.mem_write(P2, struct.pack('<2f', 2.0, -1.0))
    plane = bytearray(struct.pack('<4f', 0.5, -0.25, 2.0, 3.0))
    struct.pack_into('<I', plane, projection * 4, normal_bits)
    cpu.mem_write(PLANE, bytes(plane))
    cpu.mem_write(P3 - 16, b'\xA5' * 44)
    cpu.mem_write(SP, struct.pack('<6I', STOP, P2, PLANE, projection, sign, P3))
    cpu.reg_write(UC_X86_REG_ESP, SP)
    cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
    cpu.reg_write(UC_X86_REG_FPCW, control)
    cpu.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
    cpu.reg_write(UC_X86_REG_FPSW, 0)
    tag = cpu.reg_read(UC_X86_REG_FPTAG)
    for register, value in SAVED.items():
        cpu.reg_write(register, value)
    cpu.mem_protect(CODE, 0x1000, u.UC_PROT_READ | u.UC_PROT_EXEC)
    cpu.emu_start(CODE, STOP, count=1000)
    assert cpu.reg_read(UC_X86_REG_EIP) == STOP
    assert cpu.reg_read(UC_X86_REG_ESP) == SP + 4
    assert cpu.reg_read(UC_X86_REG_EAX) == P3
    assert all(cpu.reg_read(r) == v for r, v in SAVED.items())
    assert cpu.reg_read(UC_X86_REG_FPCW) == control
    assert cpu.reg_read(UC_X86_REG_FPTAG) == tag
    assert cpu.reg_read(UC_X86_REG_FPSW) & 0x3800 == 0
    assert not cpu.reg_read(UC_X86_REG_EFLAGS) & 0x400
    assert bytes(cpu.mem_read(P3 - 16, 16)) == bytes(cpu.mem_read(P3 + 12, 16)) == b'\xA5' * 16
    assert bytes(cpu.mem_read(P2, 8)) == struct.pack('<2f', 2.0, -1.0)
    assert bytes(cpu.mem_read(PLANE, 16)) == bytes(plane)
    return bytes(cpu.mem_read(P3, 12)).hex()


def main():
    for role, relative in PATHS.items():
        assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == PIN[role]
    finite_cases = 0
    nan_cases = []
    for control in (0x027F, 0x037F):
        for projection in range(3):
            for sign in (0, 1):
                for bits in (0, 0x80000000, 0x3F800000, 0xBF800000, 0x38D1B716, 0x38D1B717, 0x38D1B718, 0x7FC00000):
                    outcomes = {role: execute(ROOT / path, projection, sign, bits, control)
                        for role, path in PATHS.items()}
                    if bits == 0x7FC00000:
                        assert len(set(outcomes.values())) == 2
                        assert outcomes['candidate'] == outcomes['current_collision'] == outcomes['current_breakable']
                        nan_cases.append({'control': control, 'projection': projection, 'sign': sign, 'outcomes': outcomes})
                    else:
                        assert len(set(outcomes.values())) == 1, (control, projection, sign, bits, outcomes)
                        finite_cases += 1
    print(json.dumps({'finite_cases': finite_cases, 'finite_executions': finite_cases * len(PATHS),
        'known_nan_cases': len(nan_cases), 'known_nan_executions': len(nan_cases) * len(PATHS),
        'example_nan_difference': nan_cases[0]}, indent=2))


if __name__ == '__main__':
    main()
