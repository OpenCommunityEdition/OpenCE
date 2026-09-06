"""Bounded target/first/actual runtime differential for ``actor_aim_jump``.

The harness executes the real i386 function bytes from the frozen January
actor_moving object, the first natural reconciled candidate, and the frozen
ordinary-build candidate.  Each role also executes its real ``scale_vector3d``
machine-code section; scale is observed at entry but is never replaced by a
semantic stub.  Only the genuine datum and swarm APIs are stubbed.

The script writes only its requested scratch JSON.  It does not compile, invoke
a build system, edit source/configuration, or update matching/park state.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

import unicorn as u
from unicorn.x86_const import (
    UC_X86_REG_EAX,
    UC_X86_REG_EBP,
    UC_X86_REG_EBX,
    UC_X86_REG_ECX,
    UC_X86_REG_EDI,
    UC_X86_REG_EDX,
    UC_X86_REG_EFLAGS,
    UC_X86_REG_EIP,
    UC_X86_REG_ESI,
    UC_X86_REG_ESP,
    UC_X86_REG_FPCW,
    UC_X86_REG_FPSW,
)


SCRIPT = Path(__file__).resolve()
ROOT = next(parent for parent in SCRIPT.parents if (parent / "tools/coff_compare.py").is_file())
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import function_sections  # noqa: E402
from tools.regression_gate import _section_aux_selection  # noqa: E402


TARGET = ROOT / "build/split/source/ai/actor_moving.obj"
FIRST = ROOT / "scratch/actor-aim-jump-fable-natural-first-20260906.obj"
ACTUAL = ROOT / "scratch/actor-aim-jump-fable-natural-final-20260906/source/ai/actor_moving.obj"
TARGET_SCALE_OBJECT = ROOT / "build/split/source/ai/action_charge.obj"
OUTPUT_DEFAULT = ROOT / "scratch/actor-aim-jump-runtime-differential-report.json"

PINNED_HASHES = {
    "target": "d693781be5186ab1f618cc7765826856e679f27a40fc94802686c338c60520fa",
    "first": "7f5804d1041cdf8ec25b61eac25cb4ac65f6339459653ace3776c9a844f4b38c",
    "actual": "0e478075c11e850ecc001bda9663fdf9221681b0dd57039d0794323c5a9d3f69",
    "target_scale_object": "76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea",
}

AIM = "_actor_aim_jump"
SCALE = "_scale_vector3d"
AIM_FACTS = {
    "target": {
        "size": 256,
        "relocations": 4,
        "normalized_sha256": "141b04fcbe71e0761c688c7a766b0791721fecc38d3d58eb14e6b319fa4f0e84",
        "relocation_addresses": [5, 17, 62, 222],
    },
    "first": {
        "size": 256,
        "relocations": 4,
        "normalized_sha256": "4087deef3f73fc2eae5eff84aa196f9f11f4c64d74b797c8e981124ab6eb5817",
        "relocation_addresses": [5, 17, 62, 223],
    },
    "actual": {
        "size": 256,
        "relocations": 4,
        "normalized_sha256": "4087deef3f73fc2eae5eff84aa196f9f11f4c64d74b797c8e981124ab6eb5817",
        "relocation_addresses": [5, 17, 62, 223],
    },
}
MATH_HELPER_FACTS = {
    "_scale_vector3d": {
        "size": 48,
        "normalized_sha256": "6436919fbdc80b5d5fe76d9c92fa146e5ac7d3b4637c9b75aaa62401f0c54a3b",
    },
    "_magnitude3d": {
        "size": 48,
        "normalized_sha256": "114f10e7ee4543446daaaafc859a4ec729ba6c8c74c38825d05bd0f00a42352c",
    },
    "_square_root": {
        "size": 16,
        "normalized_sha256": "0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7",
    },
}
EXPECTED_RELOCATION_NAMES = [
    "_actor_data",
    "_datum_get",
    "_actor_type_swarm_aim_jump",
    "_scale_vector3d",
]

CODE = 0x100000
STOP = 0x10F000
SCALE_CODE = 0x110000
STUBS = 0x200000
GLOBALS = 0x300000
ACTOR_DATA_SLOT = GLOBALS + 0x100
ACTOR_DATA_VALUE = 0x2468ACE0
ACTOR = 0x400000
ACTOR_SIZE = 0x800
VECTOR = 0x410000
VECTOR_SIZE = 0x100
STACK = 0x500000
STACK_SIZE = 0x10000
ENTRY_SP = STACK + 0x8000
CALLER_FRAME_SIZE = 0x60

STUB_ADDRESSES = {
    "_datum_get": STUBS + 0x100,
    "_actor_type_swarm_aim_jump": STUBS + 0x200,
}

VEHICLE_OFFSET = 0x158
SWARM_OFFSET = 0x006
ACTION_OFFSET = 0x06C
CHARGE_GOAL_OFFSET = 0x0A0
TARGETED_OFFSET = 0x530
ALIGNMENT_I_OFFSET = 0x534
ALIGNMENT_J_OFFSET = 0x538
HORIZONTAL_VELOCITY_OFFSET = 0x53C
VERTICAL_VELOCITY_OFFSET = 0x540

ACTION_CHARGE = 10
CHARGE_GOAL_MELEE_LEAPING = 3
NONE = 0xFFFFFFFF
FPCW_VALUES = (0x027F, 0x037F)

NONVOLATILE = {
    "ebp": UC_X86_REG_EBP,
    "ebx": UC_X86_REG_EBX,
    "esi": UC_X86_REG_ESI,
    "edi": UC_X86_REG_EDI,
}


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def verify_hash(path: Path, expected: str, role: str) -> dict:
    actual = sha256_path(path)
    if actual.lower() != expected.lower():
        raise AssertionError((role, "SHA-256", str(path), actual, expected))
    return {"path": str(path), "sha256": actual, "bytes": path.stat().st_size}


def p32(value: int) -> bytes:
    return struct.pack("<I", value & 0xFFFFFFFF)


def f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


def f32_bits(value: float) -> int:
    return struct.unpack("<I", struct.pack("<f", f32(value)))[0]


def bits_f32(value: int) -> float:
    return struct.unpack("<f", struct.pack("<I", value & 0xFFFFFFFF))[0]


def read32(machine: u.Uc, address: int) -> int:
    return struct.unpack("<I", machine.mem_read(address, 4))[0]


def write32(data: bytearray, offset: int, value: int) -> None:
    data[offset : offset + 4] = p32(value)


def write16(data: bytearray, offset: int, value: int) -> None:
    data[offset : offset + 2] = struct.pack("<H", value & 0xFFFF)


def write_float(data: bytearray, offset: int, value: float) -> None:
    data[offset : offset + 4] = struct.pack("<f", f32(value))


def seeded_bytes(size: int, seed: int) -> bytes:
    return bytes(((index * 37 + seed * 53 + 11) & 0xFF) for index in range(size))


def ordered_float_bits(bits: int) -> int:
    return (~bits & 0xFFFFFFFF) if (bits & 0x80000000) else (bits | 0x80000000)


def ulp_distance(left: int, right: int) -> int:
    return abs(ordered_float_bits(left) - ordered_float_bits(right))


def function_section(path: Path, name: str, expected_hash: str) -> dict:
    actual_hash = sha256_path(path)
    if actual_hash != expected_hash:
        raise AssertionError(("object hash mismatch", str(path), expected_hash, actual_hash))
    obj = cc.load(path)
    functions = function_sections(obj)
    if name not in functions:
        raise AssertionError((str(path), "missing function", name))
    function = functions[name]
    symbol = cc.symbol(obj, name)
    section = obj["sections"][function.section_number - 1]
    return {
        "path": path,
        "object_hash": actual_hash,
        "object": obj,
        "function": function,
        "symbol": symbol,
        "raw": bytes(cc._section_bytes(obj, section)),
        "selection": _section_aux_selection(obj, symbol["section"]),
    }


def relocation_destination(name: str) -> int:
    if name == "_actor_data":
        return ACTOR_DATA_SLOT
    if name == "_scale_vector3d":
        return SCALE_CODE
    if name in STUB_ADDRESSES:
        return STUB_ADDRESSES[name]
    raise AssertionError(("unexpected aim relocation", name))


def prepare_aim(path: Path, role: str, expected_hash: str) -> dict:
    prepared = function_section(path, AIM, expected_hash)
    info = prepared["function"].info
    raw = bytearray(prepared["raw"])
    relocations = []
    for relocation in info["relocations"]:
        target = relocation.get("symbolic_target", relocation["target"])
        if not isinstance(target, (list, tuple)) or target[0] != "symbol":
            raise AssertionError((role, "unresolved relocation", target))
        name, addend = target[1:]
        destination = relocation_destination(name) + addend
        if relocation["type"] == cc.IMAGE_REL_I386_REL32:
            patched = destination - (CODE + relocation["address"] + 4)
        elif relocation["type"] == cc.IMAGE_REL_I386_DIR32:
            patched = destination
        else:
            raise AssertionError((role, "unsupported relocation type", relocation))
        raw[relocation["address"] : relocation["address"] + 4] = p32(patched)
        relocations.append([relocation["address"], relocation["type"], name, addend])

    facts = AIM_FACTS[role]
    checks = {
        "size_256": info["size"] == facts["size"],
        "relocations_4": info["relocation_count"] == facts["relocations"],
        "normalized_sha256": info["normalized_sha256"] == facts["normalized_sha256"],
        "relocation_names": [item[2] for item in relocations] == EXPECTED_RELOCATION_NAMES,
        "relocation_addresses": [item[0] for item in relocations] == facts["relocation_addresses"],
        "external_function_owner": prepared["symbol"]["storage"] == 2
        and prepared["symbol"]["type"] == 0x20,
        "comdat_noduplicates": prepared["selection"] == 1,
    }
    if not all(checks.values()):
        raise AssertionError((role, checks, relocations))
    prepared.update({"role": role, "runtime_raw": bytes(raw), "relocations": relocations, "checks": checks})
    return prepared


def prepare_math_helper(path: Path, role: str, expected_hash: str, name: str) -> dict:
    prepared = function_section(path, name, expected_hash)
    info = prepared["function"].info
    facts = MATH_HELPER_FACTS[name]
    checks = {
        "size": info["size"] == facts["size"],
        "relocations_0": info["relocation_count"] == 0,
        "normalized_sha256": info["normalized_sha256"] == facts["normalized_sha256"],
        "external_function_owner": prepared["symbol"]["storage"] == 2
        and prepared["symbol"]["type"] == 0x20,
        # The inherited target helper is NODUPLICATES.  Both newly emitted
        # candidate copies are SELECTANY; that conflict is reported rather
        # than treated as a complete/linkable-object claim.
        "comdat_selection_expected": prepared["selection"] == (1 if role == "target" else 2),
    }
    if not all(checks.values()):
        raise AssertionError((role, name, checks))
    prepared.update({"role": role, "name": name, "checks": checks})
    return prepared


def case(
    name: str,
    path: str,
    *,
    vehicle: int = NONE,
    swarm: int = 0,
    targeted: int = 1,
    action: int = 0,
    goal: int = 0,
    leap: int = 0,
    i: float = 0.6,
    j: float = 0.8,
    horizontal: float = 10.0,
    vertical: float = 0.0,
    maximum: float = 5.0,
) -> dict:
    return {
        "name": name,
        "expected_path": path,
        "vehicle": vehicle,
        "swarm": swarm,
        "targeted": targeted,
        "action": action,
        "goal": goal,
        "leap": leap,
        "alignment_i": i,
        "alignment_j": j,
        "horizontal": horizontal,
        "vertical": vertical,
        "maximum": maximum,
    }


def cases() -> list[dict]:
    return [
        case("vehicle_blocks_idle", "vehicle", vehicle=0x01020304, targeted=0),
        case("vehicle_blocks_targeted", "vehicle", vehicle=0x89ABCDEF, targeted=0x7F),
        case("swarm_one", "swarm", swarm=1, targeted=0),
        case("swarm_high_bit", "swarm", swarm=0x80, targeted=0xFF),
        case("ordinary_not_targeted_zero", "idle", targeted=0),
        case("ordinary_not_targeted_high_vehicle_none", "idle", targeted=0, leap=0x80),
        case("leap_true_unscaled", "unscaled", leap=1, i=0.3, j=-0.7, horizontal=8.25, vertical=4.5, maximum=1.0),
        case("leap_high_bit_unscaled", "unscaled", leap=0x80, i=-1.25, j=0.375, horizontal=3.5, vertical=-7.0, maximum=0.5),
        case("charge_melee_forces_leap", "unscaled", action=ACTION_CHARGE, goal=CHARGE_GOAL_MELEE_LEAPING, i=1.1, j=2.2, horizontal=3.3, vertical=4.4, maximum=0.25),
        case("charge_wrong_goal_clamps", "scaled", action=ACTION_CHARGE, goal=2, i=0.6, j=0.8, horizontal=10.0, vertical=0.0, maximum=5.0),
        case("wrong_action_melee_goal_clamps", "scaled", action=9, goal=CHARGE_GOAL_MELEE_LEAPING, i=0.6, j=0.8, horizontal=10.0, vertical=0.0, maximum=5.0),
        case("below_limit_unscaled", "unscaled", i=0.3, j=0.4, horizontal=2.0, vertical=0.5, maximum=5.0),
        case("equal_limit_unscaled", "unscaled", i=0.6, j=0.8, horizontal=5.0, vertical=0.0, maximum=5.0),
        case("classic_3_4_12_clamp", "scaled", i=0.6, j=0.8, horizontal=5.0, vertical=12.0, maximum=6.0),
        case("vertical_only_clamp", "scaled", i=0.0, j=0.0, horizontal=99.0, vertical=-10.0, maximum=3.0),
        case("zero_vector_unscaled", "unscaled", i=0.0, j=-0.0, horizontal=123.0, vertical=0.0, maximum=1.0),
        case("fractional_rounding_a", "scaled", i=1.234567, j=-2.345678, horizontal=3.456789, vertical=4.567891, maximum=2.75),
        case("fractional_rounding_b", "scaled", i=0.33333334, j=0.14285715, horizontal=7.1234565, vertical=-1.2345, maximum=1.125),
        case("fractional_rounding_c", "scaled", i=-12.34567, j=0.7654321, horizontal=0.9876543, vertical=8.125, maximum=4.25),
        case("fractional_rounding_d", "scaled", i=1234.125, j=-0.0012345, horizontal=0.031337, vertical=0.765625, maximum=3.0),
        case("small_finite_clamp", "scaled", i=1.17549435e-20, j=-3.25e-20, horizontal=1.125e10, vertical=2.5e-10, maximum=1.0e-10),
        case("large_finite_clamp", "scaled", i=12345.5, j=-5432.25, horizontal=8192.0, vertical=65536.0, maximum=4096.0),
    ]


def machine_argument(machine: u.Uc, index: int) -> int:
    return read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4 + 4 * index)


def execute(prepared: dict, scale: dict, scenario: dict, fpcw: int, seed: int) -> dict:
    machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    for address, size in (
        (CODE, 0x1000),
        (SCALE_CODE, 0x1000),
        (STUBS, 0x1000),
        (GLOBALS, 0x1000),
        (ACTOR, 0x1000),
        (VECTOR, 0x1000),
        (STACK, STACK_SIZE),
    ):
        machine.mem_map(address, size)
    machine.mem_write(CODE, prepared["runtime_raw"])
    machine.mem_write(SCALE_CODE, scale["raw"])
    machine.mem_write(STUBS, b"\xC3" * 0x1000)
    machine.mem_write(ACTOR_DATA_SLOT, p32(ACTOR_DATA_VALUE))

    actor_before = bytearray(seeded_bytes(ACTOR_SIZE, seed))
    write32(actor_before, VEHICLE_OFFSET, scenario["vehicle"])
    actor_before[SWARM_OFFSET] = scenario["swarm"] & 0xFF
    write16(actor_before, ACTION_OFFSET, scenario["action"])
    write16(actor_before, CHARGE_GOAL_OFFSET, scenario["goal"])
    actor_before[TARGETED_OFFSET] = scenario["targeted"] & 0xFF
    write_float(actor_before, ALIGNMENT_I_OFFSET, scenario["alignment_i"])
    write_float(actor_before, ALIGNMENT_J_OFFSET, scenario["alignment_j"])
    write_float(actor_before, HORIZONTAL_VELOCITY_OFFSET, scenario["horizontal"])
    write_float(actor_before, VERTICAL_VELOCITY_OFFSET, scenario["vertical"])
    machine.mem_write(ACTOR, bytes(actor_before))

    vector_before = seeded_bytes(VECTOR_SIZE, seed + 71)
    machine.mem_write(VECTOR, vector_before)
    stack_before = bytearray(seeded_bytes(STACK_SIZE, seed + 103))
    entry_offset = ENTRY_SP - STACK
    actor_handle = (0xA5000000 | seed) & 0xFFFFFFFF
    unit_handle = (0x5A000000 | (seed * 17)) & 0xFFFFFFFF
    frame = [
        STOP,
        actor_handle,
        unit_handle,
        scenario["leap"],
        f32_bits(scenario["maximum"]),
        VECTOR,
    ]
    for index, value in enumerate(frame):
        write32(stack_before, entry_offset + 4 * index, value)
    caller_before = bytes(stack_before[entry_offset : entry_offset + CALLER_FRAME_SIZE])
    machine.mem_write(STACK, bytes(stack_before))

    saved = {
        "ebp": 0x1122AABB,
        "ebx": 0x2233BBCC,
        "esi": 0x3344CCDD,
        "edi": 0x4455DDEE,
    }
    machine.reg_write(UC_X86_REG_EAX, 0x89ABCDEF)
    machine.reg_write(UC_X86_REG_ECX, 0x76543210)
    machine.reg_write(UC_X86_REG_EDX, 0x0BADF00D)
    for name, register in NONVOLATILE.items():
        machine.reg_write(register, saved[name])
    machine.reg_write(UC_X86_REG_ESP, ENTRY_SP)
    initial_eflags = 0x202 | (0x400 if seed & 1 else 0)
    machine.reg_write(UC_X86_REG_EFLAGS, initial_eflags)
    machine.reg_write(UC_X86_REG_FPCW, fpcw)

    calls = []
    writes = []

    def return_from_stub(return_value: int) -> None:
        esp = machine.reg_read(UC_X86_REG_ESP)
        return_address = read32(machine, esp)
        machine.reg_write(UC_X86_REG_EAX, return_value & 0xFFFFFFFF)
        machine.reg_write(UC_X86_REG_ESP, esp + 4)
        machine.reg_write(UC_X86_REG_EIP, return_address)

    def on_code(_machine, address, _size, _user):
        if address == STUB_ADDRESSES["_datum_get"]:
            args = [machine_argument(machine, 0), machine_argument(machine, 1)]
            calls.append({"name": "_datum_get", "args": args})
            if args != [ACTOR_DATA_VALUE, actor_handle]:
                raise AssertionError((prepared["role"], scenario["name"], "datum args", args))
            return_from_stub(ACTOR)
        elif address == STUB_ADDRESSES["_actor_type_swarm_aim_jump"]:
            args = [machine_argument(machine, index) for index in range(4)]
            calls.append({"name": "_actor_type_swarm_aim_jump", "args": args})
            expected = [actor_handle, unit_handle, f32_bits(scenario["maximum"]), VECTOR]
            if args != expected:
                raise AssertionError((prepared["role"], scenario["name"], "swarm args", args, expected))
            return_from_stub(0xA1B2C3D4)
        elif address == SCALE_CODE:
            args = [machine_argument(machine, index) for index in range(3)]
            calls.append({"name": "_scale_vector3d", "args": args})
            if args[0] != VECTOR or args[2] != VECTOR:
                raise AssertionError((prepared["role"], scenario["name"], "scale args", args))

    def on_write(_machine, _access, address, size, value, _user):
        if ACTOR <= address < ACTOR + ACTOR_SIZE:
            writes.append(["actor", address - ACTOR, size, value & ((1 << (size * 8)) - 1)])
        elif VECTOR <= address < VECTOR + VECTOR_SIZE:
            writes.append(["vector", address - VECTOR, size, value & ((1 << (size * 8)) - 1)])

    machine.hook_add(u.UC_HOOK_CODE, on_code)
    machine.hook_add(u.UC_HOOK_MEM_WRITE, on_write)
    machine.emu_start(CODE, STOP, count=5000)

    actor_after = bytes(machine.mem_read(ACTOR, ACTOR_SIZE))
    vector_after = bytes(machine.mem_read(VECTOR, VECTOR_SIZE))
    caller_after = bytes(machine.mem_read(ENTRY_SP, CALLER_FRAME_SIZE))
    final_nonvolatile = {
        name: machine.reg_read(register) & 0xFFFFFFFF for name, register in NONVOLATILE.items()
    }
    result = {
        "role": prepared["role"],
        "calls": calls,
        "writes": writes,
        "actor_sha256": sha256_bytes(actor_after),
        "vector_sha256": sha256_bytes(vector_after),
        "vector_bits": [read32(machine, VECTOR + 4 * index) for index in range(3)],
        "local_magnitude_bits": read32(machine, ENTRY_SP - 8),
        "al": machine.reg_read(UC_X86_REG_EAX) & 0xFF,
        "esp": machine.reg_read(UC_X86_REG_ESP) & 0xFFFFFFFF,
        "eip": machine.reg_read(UC_X86_REG_EIP) & 0xFFFFFFFF,
        "eflags": machine.reg_read(UC_X86_REG_EFLAGS) & 0xFFFFFFFF,
        "fpcw": machine.reg_read(UC_X86_REG_FPCW) & 0xFFFF,
        "fpsw": machine.reg_read(UC_X86_REG_FPSW) & 0xFFFF,
        "nonvolatile": final_nonvolatile,
        "caller_frame_exact": caller_after == caller_before,
    }

    expected_actor = bytearray(actor_before)
    expected_actor[TARGETED_OFFSET] = 0
    path = scenario["expected_path"]
    expected_call_names = ["_datum_get"]
    if path == "swarm":
        expected_call_names.append("_actor_type_swarm_aim_jump")
    elif path == "scaled":
        expected_call_names.append("_scale_vector3d")
    vector_write_count = {"vehicle": 0, "swarm": 0, "idle": 0, "unscaled": 3, "scaled": 6}[path]
    vector_writes = [item for item in writes if item[0] == "vector"]
    actor_writes = [item for item in writes if item[0] == "actor"]
    constructed_bits = [
        f32_bits(f32(scenario["alignment_i"]) * f32(scenario["horizontal"])),
        f32_bits(f32(scenario["alignment_j"]) * f32(scenario["horizontal"])),
        f32_bits(scenario["vertical"]),
    ]
    checks = {
        "call_order_and_branches": [item["name"] for item in calls] == expected_call_names,
        "return_al_true": result["al"] == 1,
        "esp_cdecl_return": result["esp"] == ENTRY_SP + 4,
        "eip_stop": result["eip"] == STOP,
        "nonvolatile_preserved": final_nonvolatile == saved,
        "direction_flag_preserved": (result["eflags"] & 0x400) == (initial_eflags & 0x400),
        "fpcw_preserved": result["fpcw"] == fpcw,
        "caller_frame_exact": result["caller_frame_exact"],
        "actor_full_envelope": actor_after == bytes(expected_actor),
        "only_targeted_actor_write": actor_writes == [["actor", TARGETED_OFFSET, 1, 0]],
        "vector_write_count": len(vector_writes) == vector_write_count,
        "vector_write_offsets": [item[1] for item in vector_writes]
        == ([0, 4, 8] * (2 if path == "scaled" else 1) if vector_write_count else []),
        "vector_guard_preserved": vector_after[12:] == vector_before[12:],
    }
    if path in ("vehicle", "swarm", "idle"):
        checks["vector_unchanged"] = vector_after == vector_before
    else:
        checks["constructed_store_bits"] = [item[3] for item in vector_writes[:3]] == constructed_bits
        if path == "unscaled":
            checks["unscaled_final_bits"] = result["vector_bits"] == constructed_bits
        else:
            scale_bits = calls[-1]["args"][1]
            expected_scaled = [
                f32_bits(bits_f32(component) * bits_f32(scale_bits))
                for component in constructed_bits
            ]
            checks["real_scale_machine_results"] = result["vector_bits"] == expected_scaled
    failed = [name for name, passed in checks.items() if not passed]
    if failed:
        raise AssertionError(
            (prepared["role"], scenario["name"], hex(fpcw), "failed checks", failed)
        )
    result["checks"] = checks
    return result


def compare_runs(target: dict, first: dict, scenario: dict) -> dict:
    target_names = [item["name"] for item in target["calls"]]
    first_names = [item["name"] for item in first["calls"]]
    if target_names != first_names:
        raise AssertionError((scenario["name"], "call path differs", target_names, first_names))
    if target["actor_sha256"] != first["actor_sha256"]:
        raise AssertionError((scenario["name"], "actor state differs"))
    if [(item[0], item[1], item[2]) for item in target["writes"]] != [
        (item[0], item[1], item[2]) for item in first["writes"]
    ]:
        raise AssertionError((scenario["name"], "write shape/order differs"))
    if target["fpsw"] != first["fpsw"]:
        raise AssertionError((scenario["name"], "x87 status differs", target["fpsw"], first["fpsw"]))
    if target["nonvolatile"] != first["nonvolatile"]:
        raise AssertionError((scenario["name"], "nonvolatile state differs"))

    vector_ulp = [
        ulp_distance(left, right) for left, right in zip(target["vector_bits"], first["vector_bits"])
    ]
    target_scale = next((item["args"][1] for item in target["calls"] if item["name"] == SCALE), None)
    first_scale = next((item["args"][1] for item in first["calls"] if item["name"] == SCALE), None)
    scale_ulp = None if target_scale is None else ulp_distance(target_scale, first_scale)
    magnitude_ulp = ulp_distance(
        target["local_magnitude_bits"], first["local_magnitude_bits"]
    )
    if (
        max(vector_ulp) > 4
        or (scale_ulp is not None and scale_ulp > 4)
        or (scenario["expected_path"] in ("unscaled", "scaled") and magnitude_ulp > 4)
    ):
        raise AssertionError(
            (
                scenario["name"],
                "floating residual exceeds bound",
                magnitude_ulp,
                scale_ulp,
                vector_ulp,
            )
        )
    for left_bits, right_bits in zip(target["vector_bits"], first["vector_bits"]):
        # No-work paths intentionally retain arbitrary seeded vector bytes,
        # which may encode NaNs.  Identical bits are the relevant proof there.
        if left_bits == right_bits:
            continue
        left = bits_f32(left_bits)
        right = bits_f32(right_bits)
        if not (math.isfinite(left) and math.isfinite(right)) or not math.isclose(
            left, right, rel_tol=5e-7, abs_tol=1e-12
        ):
            raise AssertionError((scenario["name"], "floating outputs not semantically close", left, right))

    return {
        "call_names_equal": True,
        "actor_full_state_equal": True,
        "write_shape_and_order_equal": True,
        "fpsw_equal": True,
        "magnitude_bits": {
            "target": target["local_magnitude_bits"],
            "first": first["local_magnitude_bits"],
            "ulp": magnitude_ulp,
        },
        "scale_bits": {
            "target": target_scale,
            "first": first_scale,
            "ulp": scale_ulp,
        },
        "vector_bits": {
            "target": target["vector_bits"],
            "first": first["vector_bits"],
            "ulp": vector_ulp,
        },
        "bit_exact_vector": target["vector_bits"] == first["vector_bits"],
    }


MUTANTS = {
    "datum_actor_argument_uses_unit_slot": {
        "offset": 0x0D,
        "from": 0x08,
        "to": 0x0C,
        "case": "vehicle_blocks_targeted",
        "expected_marker": "datum args",
    },
    "general_targeted_clear_writes_one": {
        "offset": 0xEC,
        "from": 0x00,
        "to": 0x01,
        "case": "vehicle_blocks_targeted",
        "expected_marker": "actor_full_envelope",
    },
    "general_return_al_false": {
        "offset": 0xEE,
        "from": 0x01,
        "to": 0x00,
        "case": "vehicle_blocks_targeted",
        "expected_marker": "return_al_true",
    },
}


def mutate_runtime(raw: bytes, mutation: dict) -> bytes:
    changed = bytearray(raw)
    offset = mutation["offset"]
    if changed[offset] != mutation["from"]:
        raise AssertionError(
            ("negative-control preimage", offset, changed[offset], mutation["from"])
        )
    changed[offset] = mutation["to"]
    return bytes(changed)


def run_negative_controls(target: dict, target_scale: dict) -> dict:
    by_name = {scenario["name"]: scenario for scenario in cases()}
    results = {}
    for index, (name, mutation) in enumerate(MUTANTS.items(), start=1):
        mutant = dict(target)
        mutant["role"] = "negative_control:" + name
        mutant["runtime_raw"] = mutate_runtime(target["runtime_raw"], mutation)
        detected = False
        observation = None
        try:
            execute(
                mutant,
                target_scale,
                by_name[mutation["case"]],
                FPCW_VALUES[0],
                0x70 + index,
            )
        except AssertionError as error:
            observation = repr(error)
            detected = mutation["expected_marker"] in observation
        results[name] = {
            "mutation": mutation,
            "detected_by_expected_check": detected,
            "observation": observation,
        }
    return {
        "pass": all(item["detected_by_expected_check"] for item in results.values()),
        "mutants": results,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=OUTPUT_DEFAULT)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        raise ValueError(f"refusing to overwrite output: {output}")
    if not output.is_relative_to((ROOT / "scratch").resolve()):
        raise ValueError(f"output must remain under scratch: {output}")

    target = prepare_aim(TARGET, "target", PINNED_HASHES["target"])
    first = prepare_aim(FIRST, "first", PINNED_HASHES["first"])
    actual = prepare_aim(ACTUAL, "actual", PINNED_HASHES["actual"])
    target_helpers = {
        name: prepare_math_helper(
            TARGET_SCALE_OBJECT,
            "target",
            PINNED_HASHES["target_scale_object"],
            name,
        )
        for name in MATH_HELPER_FACTS
    }
    first_helpers = {
        name: prepare_math_helper(FIRST, "first", PINNED_HASHES["first"], name)
        for name in MATH_HELPER_FACTS
    }
    actual_helpers = {
        name: prepare_math_helper(ACTUAL, "actual", PINNED_HASHES["actual"], name)
        for name in MATH_HELPER_FACTS
    }
    target_scale = target_helpers[SCALE]
    first_scale = first_helpers[SCALE]
    actual_scale = actual_helpers[SCALE]
    for name in MATH_HELPER_FACTS:
        if not (
            target_helpers[name]["raw"]
            == first_helpers[name]["raw"]
            == actual_helpers[name]["raw"]
        ):
            raise AssertionError(f"target/first/actual {name} machine bodies differ")
    if first["raw"] != actual["raw"] or first["runtime_raw"] != actual["runtime_raw"]:
        raise AssertionError("first and actual actor_aim_jump machine bodies differ")

    results = []
    bit_exact_count = 0
    measured_residuals = []
    for case_index, scenario in enumerate(cases()):
        for fpcw in FPCW_VALUES:
            seed = case_index * len(FPCW_VALUES) + FPCW_VALUES.index(fpcw) + 1
            target_run = execute(target, target_scale, scenario, fpcw, seed)
            first_run = execute(first, first_scale, scenario, fpcw, seed)
            actual_run = execute(actual, actual_scale, scenario, fpcw, seed)
            comparison = compare_runs(target_run, first_run, scenario)
            actual_comparison = compare_runs(target_run, actual_run, scenario)
            comparable_keys = (
                "calls",
                "writes",
                "actor_sha256",
                "vector_sha256",
                "vector_bits",
                "local_magnitude_bits",
                "al",
                "esp",
                "eip",
                "eflags",
                "fpcw",
                "fpsw",
                "nonvolatile",
                "caller_frame_exact",
                "checks",
            )
            if any(first_run[key] != actual_run[key] for key in comparable_keys):
                raise AssertionError((scenario["name"], hex(fpcw), "first/actual runtime differs"))
            if comparison != actual_comparison:
                raise AssertionError((scenario["name"], hex(fpcw), "target comparison differs for actual"))
            if comparison["bit_exact_vector"]:
                bit_exact_count += 1
            if (
                comparison["magnitude_bits"]["ulp"]
                or comparison["scale_bits"]["ulp"] not in (None, 0)
                or any(comparison["vector_bits"]["ulp"])
            ):
                measured_residuals.append(
                    {
                        "case": scenario["name"],
                        "fpcw": fpcw,
                        "magnitude": comparison["magnitude_bits"],
                        "scale": comparison["scale_bits"],
                        "vector": comparison["vector_bits"],
                    }
                )
            results.append(
                {
                    "case": scenario,
                    "fpcw": fpcw,
                    "target": target_run,
                    "first": first_run,
                    "actual": actual_run,
                    "comparison": comparison,
                }
            )

    negative_controls = run_negative_controls(target, target_scale)
    if not negative_controls["pass"]:
        raise AssertionError(("negative controls", negative_controls))

    report = {
        "pass": True,
        "scope": "one target/first/actual natural actor_aim_jump function with real scale machine bodies",
        "build_or_production_mutation": False,
        "script": {"path": str(SCRIPT), "sha256": sha256_path(SCRIPT)},
        "inputs": {
            "target": verify_hash(TARGET, PINNED_HASHES["target"], "target"),
            "first": verify_hash(FIRST, PINNED_HASHES["first"], "first"),
            "actual": verify_hash(ACTUAL, PINNED_HASHES["actual"], "actual"),
            "target_scale_object": verify_hash(
                TARGET_SCALE_OBJECT,
                PINNED_HASHES["target_scale_object"],
                "target_scale_object",
            ),
        },
        "coff_guards": {
            "target_aim": target["checks"],
            "first_aim": first["checks"],
            "actual_aim": actual["checks"],
            "target_math_helpers": {
                name: helper["checks"] for name, helper in target_helpers.items()
            },
            "first_math_helpers": {
                name: helper["checks"] for name, helper in first_helpers.items()
            },
            "actual_math_helpers": {
                name: helper["checks"] for name, helper in actual_helpers.items()
            },
            "scale_raw_sha256": sha256_bytes(target_scale["raw"]),
            "target_first_actual_helper_raw_exact": True,
            "first_and_actual_aim_raw_exact": True,
            "candidate_extra_math_comdats_observed_not_called_by_aim": [
                "_magnitude3d",
                "_square_root",
            ],
            "helper_linkage_boundary": (
                "all three inherited ActionCharge owners are NODUPLICATES selection 1; "
                "all three first/actual candidate emissions are SELECTANY selection 2"
            ),
        },
        "coverage": {
            "case_count": len(cases()),
            "fpcw_values": list(FPCW_VALUES),
            "execution_count_per_role": len(results),
            "paths": sorted({item["expected_path"] for item in cases()}),
            "vehicle_swarm_targeted_leap_charge_and_clamp_branches": True,
            "real_scale_machine_body_executed": True,
            "stubbed_genuine_apis_only": list(STUB_ADDRESSES),
            "full_actor_envelope_bytes": ACTOR_SIZE,
            "full_vector_buffer_bytes": VECTOR_SIZE,
            "actor_memory_description": "seeded 0x800-byte test envelope; not an actor sizeof claim",
            "handle_description": (
                "arbitrary abstract forwarding inputs; the datum stub returns the seeded actor owner, "
                "not a claim that the values are valid engine handles"
            ),
            "output_alias_scope": (
                "tested output points to a separate guarded vector buffer and does not alias actor fields; "
                "the explicit-three-store schedule is not proven equivalent for aliased outputs"
            ),
        },
        "comparison_summary": {
            "all_call_paths_arguments_and_order_validated": True,
            "all_nonfloating_state_write_shapes_and_abi_validated": True,
            "all_x87_status_words_equal": True,
            "bit_exact_vector_execution_count": bit_exact_count,
            "bit_exact_output_execution_count": bit_exact_count,
            "measured_internal_or_output_residual_execution_count": len(measured_residuals),
            "measured_internal_or_output_residuals": measured_residuals,
            "bit_different_output_execution_count": sum(
                1 for item in measured_residuals if any(item["vector"]["ulp"])
            ),
            "maximum_allowed_observed_ulp": 4,
            "first_actual_all_tested_runtime_observations_exact": True,
        },
        "negative_controls": negative_controls,
        "results": results,
        "conclusion": (
            "For this bounded finite corpus and separate-output/no-alias domain, the natural explicit-store "
            "first and actual candidates follow the target control/API/write/ABI paths. Any listed "
            "differences are measured x87 rounding-schedule residuals, not hidden or treated as exact or "
            "full semantic equivalence. Candidate-emitted SELECTANY math helpers conflict with the inherited "
            "NODUPLICATES owner, so this is only partial fuzzy runtime evidence under the existing Geometry "
            "boundary and not a complete/linkable-object claim."
        ),
    }
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"PASS: {output}")
    print(f"sha256={sha256_path(output)}")
    print(
        f"executions_per_role={len(results)} "
        f"measured_residuals={len(measured_residuals)} "
        f"negative_controls={negative_controls['pass']}"
    )


if __name__ == "__main__":
    main()
