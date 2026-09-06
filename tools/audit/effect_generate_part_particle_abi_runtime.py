"""Bounded ABI differential for the particle branch of effect_generate_part.

This scratch oracle executes the real final particle-system branch bytes from
the frozen pre-schema object, the frozen schema object, and the January target.
Only the two random helpers and the final public particle-system API are
stubbed.  The final API stub captures the caller-created stack frame; it does
not replace any computation performed in the branch.

The pre-schema object is an intentional negative witness: its unprototyped
call promotes ``real scale`` to ``double``.  The schema and January objects
must instead pass the fifth argument as one 32-bit ``real`` word.

The script writes only its requested scratch JSON.  It never compiles, invokes
a build, changes source/configuration, or updates matching/park state.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
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


BEFORE = ROOT / "scratch/particle-systems-fable-pair-before-20260906/source/effects/effects.obj"
SCHEMA = ROOT / "scratch/particle-systems-fable-pair-schema-20260906/source/effects/effects.obj"
TARGET = ROOT / "build/split/source/effects/effects.obj"
# Preserve the audited target after later semantic symbol-manifest changes.
TARGET_CALLEE = ROOT / "scratch/particle-systems-fable-pair-final-20260906/files/build/split/source/effects/particle_systems.obj"
BEFORE_MANIFEST = ROOT / "scratch/particle-systems-fable-pair-before-20260906.json"
SCHEMA_MANIFEST = ROOT / "scratch/particle-systems-fable-pair-schema-20260906.json"
RUNTIME_SUMMARY = ROOT / "scratch/particle-systems-fable-pair-schema-runtime-20260906/summary.json"
EFFECTS_RUNTIME = ROOT / "scratch/particle-systems-fable-pair-schema-runtime-20260906/source/effects/effects.json"
BEFORE_SOURCE = ROOT / "scratch/particle-systems-fable-pair-before-20260906/files/source/effects/effects.c"
SCHEMA_SOURCE = ROOT / "scratch/particle-systems-fable-pair-schema-20260906/files/source/effects/effects.c"
BEFORE_HEADER = ROOT / "scratch/particle-systems-fable-pair-before-20260906/files/source/effects/particle_systems.h"
SCHEMA_HEADER = ROOT / "scratch/particle-systems-fable-pair-schema-20260906/files/source/effects/particle_systems.h"
OUTPUT_DEFAULT = ROOT / "scratch/effect-generate-part-particle-abi-runtime-20260906.json"

PINS = {
    "before_object": "91abeef842a77f23426aa0e1a95c52bfd505141fe251ba726130d70a881089ca",
    "schema_object": "ee030a6c036b90ecfa45a449d2041c7f49009d26fda643734f5ae2cce3771dc9",
    "target_object": "3f65028de11f5774bd1b9f07dec02e7cf614f740ff0da8f588f9e2ef811d7894",
    "target_callee_object": "25cc3edef1c9b54ba12f677a88a0b5b93aff8f66fcdff7b29d8faa532b939e56",
    "before_manifest": "8bf3891c9b86f0348f62e1d786cdb9b589294c8bda0caae1d2bc3338f3a602d4",
    "schema_manifest": "e09348fe3c43396b90b443370346e7b9e47c860ba4be4e7febdcbc594751278b",
    "runtime_summary": "02b2f0be70f65f24d002d68deaba7418a7cd1d1fb5fc032741eb9c2ddee35e16",
    "effects_runtime": "3f5311f6ce7c559bda753015fab6ee2d6b4e961ec7f6dfa104c2255f58206893",
    "before_source": "08166c1fb788f323969cb6c6816c0dc10ff47f8c4dbf0958caef05df16ae2443",
    "schema_source": "771f0df15de5122079c76d94202084dc81468a41ad781fc450dc69c8ed846b15",
    "before_header": "60aa82aaee4300ef225ebdc6388934e89494fd5667f4bda27472399bac0d42e7",
    "schema_header": "1b1ee6bf88186bcc3ac1f6634fd979d8862e19eb21952065a7b9065519b697e5",
}

FUNCTION = "_effect_generate_part"
CALLEE = "_particle_system_new_unattached"
FUNCTION_FACTS = {
    "before": {
        "size": 976,
        "relocations": 28,
        "normalized_sha256": "a1ef8676e8952af52dbecfeaf78c8273d0b0a959bd48fa00a580c132b4c92128",
        "section": 83,
        "branch_entry": 0x334,
        "entry_sp_delta": -0xBC,
        "saved_ebx_delta": -0xB8,
        "saved_edi_delta": -0xBC,
        "direction_delta": -0xB4,
        "velocity_delta": -0x10,
        "color_delta": -0xA8,
        "tail_relocations": [
            [0x37F, "_get_global_local_random_seed_address"],
            [0x387, "_effect_random_translational_velocity"],
            [0x3C0, CALLEE],
        ],
        "abi": "promoted_double_negative_witness",
    },
    "schema": {
        "size": 976,
        "relocations": 28,
        "normalized_sha256": "84354fb7e9646e34b10f0c114f47b31599ff63d73c6d8a168fd93c23b811b7e3",
        "section": 83,
        "branch_entry": 0x334,
        "entry_sp_delta": -0xBC,
        "saved_ebx_delta": -0xB8,
        "saved_edi_delta": -0xBC,
        "direction_delta": -0xB4,
        "velocity_delta": -0x10,
        "color_delta": -0xA8,
        "tail_relocations": [
            [0x37F, "_get_global_local_random_seed_address"],
            [0x387, "_effect_random_translational_velocity"],
            [0x3BB, CALLEE],
        ],
        "abi": "real32",
    },
    "target": {
        "size": 992,
        "relocations": 27,
        "normalized_sha256": "eb3a1fe032ad59202ce0a3fc407f282644a5ffb8e7af42b974f68761622ca9d3",
        "section": 31,
        "branch_entry": 0x348,
        "entry_sp_delta": -0xC0,
        "saved_ebx_delta": -0xBC,
        "saved_edi_delta": -0xC0,
        "direction_delta": -0xB8,
        "velocity_delta": -0x14,
        "color_delta": -0xAC,
        "tail_relocations": [
            [0x393, "_get_global_local_random_seed_address"],
            [0x39B, "_effect_random_translational_velocity"],
            [0x3CF, CALLEE],
        ],
        "abi": "real32",
    },
}

CALLEE_FACTS = {
    "size": 208,
    "relocations": 8,
    "normalized_sha256": "39b66f0d516aefd4eee70e8ddb5c9ce80138d8a1f11f467f688738ef348b8e38",
    "scale_copy_offset": 0x86,
    # mov edx,[ebp+18h]; mov [eax+14h],edx
    "scale_copy_bytes": "8b5518895014",
}

CODE = 0x100000
STOP = CODE + 0xF00
STUBS = 0x200000
EFFECT = 0x300000
PART = 0x301000
WORLD = 0x302000
FORWARD = 0x303000
UP = 0x304000
SEED_ADDRESS = 0x305000
STACK = 0x400000
STACK_SIZE = 0x20000
FRAME = STACK + 0x10000

STUB_ADDRESSES = {
    "_get_global_local_random_seed_address": STUBS + 0x100,
    "_effect_random_translational_velocity": STUBS + 0x200,
    CALLEE: STUBS + 0x300,
}

FPCW_VALUES = (0x027F, 0x037F)
SCALE_CASES = (
    ("one", 0x3F800000),
    ("negative_two_point_five", 0xC0200000),
    ("point_one", 0x3DCCCCCD),
    ("minimum_normal", 0x00800000),
    ("maximum_finite", 0x7F7FFFFF),
    ("negative_zero", 0x80000000),
)

DEFINITION_INDEX = 0x13579BDF
LOWER_BITS = 0x3E800000
UPPER_BITS = 0x40100000
CONE_BITS = 0x3F000000
SCALE_A_FLAGS = 0x01234567
SCALE_B_FLAGS = 0x89ABCDEF
HELPER_DIRECTION_BITS = [0x3F800000, 0x40000000, 0x40400000]
HELPER_VELOCITY_BITS = [0x3E800000, 0xBFC00000, 0x40700000]
EFFECT_VELOCITY_BITS = [0x40000000, 0x3F000000, 0xBFA00000]
EXPECTED_VELOCITY_BITS = [0x40100000, 0xBF800000, 0x40200000]
EFFECT_COLOR_BITS = [0x3E000000, 0x3F400000, 0x3F000000]
EXPECTED_COLOR_BITS = [0x3F800000, *EFFECT_COLOR_BITS]
STUB_RETURN = 0x76543210

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


def p32(value: int) -> bytes:
    return struct.pack("<I", value & 0xFFFFFFFF)


def read32(machine: u.Uc, address: int) -> int:
    return struct.unpack("<I", machine.mem_read(address, 4))[0]


def write32(data: bytearray, offset: int, value: int) -> None:
    data[offset : offset + 4] = p32(value)


def seeded_bytes(size: int, seed: int) -> bytes:
    return bytes(((index * 37 + seed * 53 + 11) & 0xFF) for index in range(size))


def verify_hash(path: Path, expected: str, label: str) -> dict:
    actual = sha256_path(path)
    if actual != expected:
        raise AssertionError((label, "SHA-256", str(path), actual, expected))
    return {"path": str(path), "sha256": actual, "bytes": path.stat().st_size}


def function_section(path: Path, expected_hash: str, name: str) -> dict:
    verify_hash(path, expected_hash, name)
    obj = cc.load(path)
    functions = function_sections(obj)
    if name not in functions:
        raise AssertionError((str(path), "missing function", name))
    function = functions[name]
    symbol = cc.symbol(obj, name)
    section = obj["sections"][function.section_number - 1]
    return {
        "path": path,
        "object": obj,
        "function": function,
        "symbol": symbol,
        "raw": bytes(cc._section_bytes(obj, section)),
        "selection": _section_aux_selection(obj, symbol["section"]),
    }


def relocation_name(relocation: dict) -> str:
    target = relocation.get("symbolic_target", relocation["target"])
    if not isinstance(target, (list, tuple)) or target[0] != "symbol":
        raise AssertionError(("unresolved relocation", relocation))
    if target[2] != 0:
        raise AssertionError(("unexpected addend", relocation))
    return target[1]


def prepare(role: str, path: Path, expected_hash: str) -> dict:
    prepared = function_section(path, expected_hash, FUNCTION)
    facts = FUNCTION_FACTS[role]
    info = prepared["function"].info
    checks = {
        "size": info["size"] == facts["size"],
        "relocation_count": info["relocation_count"] == facts["relocations"],
        "normalized_sha256": info["normalized_sha256"] == facts["normalized_sha256"],
        "section_number": prepared["symbol"]["section"] == facts["section"],
        "private_function_symbol": prepared["symbol"]["storage"] == 3
        and prepared["symbol"]["type"] == 0x20,
        "comdat_noduplicates": prepared["selection"] == 1,
    }
    if not all(checks.values()):
        raise AssertionError((role, checks))

    tail = []
    raw = bytearray(prepared["raw"])
    branch_entry = facts["branch_entry"]
    for relocation in info["relocations"]:
        if relocation["address"] < branch_entry:
            continue
        name = relocation_name(relocation)
        tail.append([relocation["address"], name])
        if relocation["type"] != cc.IMAGE_REL_I386_REL32:
            raise AssertionError((role, "tail relocation is not REL32", relocation))
        destination = STUB_ADDRESSES[name]
        displacement = destination - (CODE + relocation["address"] + 4)
        raw[relocation["address"] : relocation["address"] + 4] = p32(displacement)
    if tail != facts["tail_relocations"]:
        raise AssertionError((role, "tail relocations", tail, facts["tail_relocations"]))

    abi_bytes = {
        "before": (0x3B5, bytes.fromhex("d94510dd1c24")),
        "schema": (0x391, bytes.fromhex("8b4d1051")),
        "target": (0x3A5, bytes.fromhex("8b451050")),
    }
    offset, expected = abi_bytes[role]
    if prepared["raw"][offset : offset + len(expected)] != expected:
        raise AssertionError((role, "ABI instruction guard", offset))

    prepared.update(
        {
            "role": role,
            "facts": facts,
            "checks": checks,
            "tail_relocations": tail,
            "runtime_raw": bytes(raw),
            "abi_instruction_offset": offset,
            "abi_instruction_bytes": expected.hex(),
        }
    )
    return prepared


def verify_callee() -> dict:
    prepared = function_section(TARGET_CALLEE, PINS["target_callee_object"], CALLEE)
    info = prepared["function"].info
    offset = CALLEE_FACTS["scale_copy_offset"]
    expected = bytes.fromhex(CALLEE_FACTS["scale_copy_bytes"])
    checks = {
        "size": info["size"] == CALLEE_FACTS["size"],
        "relocation_count": info["relocation_count"] == CALLEE_FACTS["relocations"],
        "normalized_sha256": info["normalized_sha256"] == CALLEE_FACTS["normalized_sha256"],
        "public_function_symbol": prepared["symbol"]["storage"] == 2
        and prepared["symbol"]["type"] == 0x20,
        "scale_copy_bytes": prepared["raw"][offset : offset + len(expected)] == expected,
    }
    if not all(checks.values()):
        raise AssertionError((CALLEE, checks))
    return {
        "checks": checks,
        "scale_copy_offset": offset,
        "scale_copy_bytes": expected.hex(),
        "decoded_meaning": "mov edx,[ebp+0x18]; mov [eax+0x14],edx",
        "abi_conclusion": "callee reads and stores exactly one 32-bit fifth argument",
    }


def verify_frozen_context() -> dict:
    paths = {
        "before_manifest": BEFORE_MANIFEST,
        "schema_manifest": SCHEMA_MANIFEST,
        "runtime_summary": RUNTIME_SUMMARY,
        "effects_runtime": EFFECTS_RUNTIME,
        "before_source": BEFORE_SOURCE,
        "schema_source": SCHEMA_SOURCE,
        "before_header": BEFORE_HEADER,
        "schema_header": SCHEMA_HEADER,
    }
    pinned = {name: verify_hash(path, PINS[name], name) for name, path in paths.items()}

    before_source = BEFORE_SOURCE.read_text(encoding="utf-8")
    schema_source = SCHEMA_SOURCE.read_text(encoding="utf-8")
    include = '#include "effects/particle_systems.h"\n'
    anchor = '#include "effects/particle_system_definitions.h"\n'
    source_checks = {
        "before_has_no_owner_include": include not in before_source,
        "schema_has_one_owner_include": schema_source.count(include) == 1,
        "only_effects_source_change_is_owner_include": schema_source
        == before_source.replace(anchor, anchor + include, 1),
    }

    prototype = (
        "long particle_system_new_unattached(\n"
        "\tlong definition_index,\n"
        "\treal_point3d const *position,\n"
        "\treal_vector3d const *velocity,\n"
        "\treal_argb_color const *color,\n"
        "\treal scale);"
    )
    before_header = BEFORE_HEADER.read_text(encoding="utf-8")
    schema_header = SCHEMA_HEADER.read_text(encoding="utf-8")
    source_checks.update(
        {
            "before_owner_already_has_one_public_prototype": before_header.count(prototype) == 1,
            "schema_has_one_public_prototype": schema_header.count(prototype) == 1,
            "prototype_text_unchanged_by_schema": prototype in before_header
            and prototype in schema_header,
        }
    )
    if not all(source_checks.values()):
        raise AssertionError(("source/header context", source_checks))

    summary = json.loads(RUNTIME_SUMMARY.read_text(encoding="utf-8"))
    effects = json.loads(EFFECTS_RUNTIME.read_text(encoding="utf-8"))
    summary_checks = {
        "five_consumers": summary["unit_count"] == 5,
        "all_consumer_classifications": summary["section_classification_totals"]
        == {
            "compiler_local_label_spelling_only": 5,
            "runtime_or_owner_changed_REVIEW": 1,
            "unchanged": 618,
        },
        "only_effects_requires_review": summary["review_required_units"]
        == ["source/effects/effects"],
        "no_new_point_unit": summary["forbidden_point_new_units"] == [],
        "effects_106_sections": effects["section_classification_counts"]
        == {
            "compiler_local_label_spelling_only": 2,
            "runtime_or_owner_changed_REVIEW": 1,
            "unchanged": 103,
        },
        "only_effect_generate_part_runtime_changed": effects["issues"]["runtime_changed"]
        == [".text|owners=_effect_generate_part"],
        "no_runtime_added": effects["issues"]["runtime_added"] == [],
        "no_runtime_lost": effects["issues"]["runtime_lost"] == [],
        "no_sections_added": effects["all_sections_added"] == [],
        "no_sections_lost": effects["all_sections_lost"] == [],
        "no_program_symbol_delta": effects["issues"][
            "program_symbol_delta_after_local_names_only"
        ]
        == {"added": [], "removed": []},
        "no_common_delta": effects["issues"]["common_delta"]
        == {"added": [], "removed": []},
        "no_definition_transitions": effects["issues"]["definition_transition_leads"] == [],
    }
    runtime_changes = effects["runtime_changes"]
    summary_checks.update(
        {
            "two_effects_local_label_only": {
                key: value["classification"] for key, value in runtime_changes.items()
                if value["classification"] == "compiler_local_label_spelling_only"
            }
            == {
                ".text|owners=_effect_allowed_by_environment": "compiler_local_label_spelling_only",
                ".text|owners=_effect_evaluate_function_integral": "compiler_local_label_spelling_only",
            },
            "raw_local_names_11_each_way": all(
                len(effects["raw_symbol_inventory_delta"][side]) == 11
                and all(item["name"].startswith("$L") for item in effects["raw_symbol_inventory_delta"][side])
                for side in ("added", "removed")
            ),
        }
    )
    if not all(summary_checks.values()):
        raise AssertionError(("frozen runtime classification", summary_checks))
    return {"pins": pinned, "source_checks": source_checks, "summary_checks": summary_checks}


def as_float(bits: int) -> float:
    return struct.unpack("<f", p32(bits))[0]


def promoted_double_words(bits: int) -> list[int]:
    packed = struct.pack("<d", float(as_float(bits)))
    return list(struct.unpack("<II", packed))


def machine_argument(machine: u.Uc, index: int) -> int:
    return read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4 + 4 * index)


def execute(prepared: dict, scale_name: str, scale_bits: int, fpcw: int, seed: int) -> dict:
    role = prepared["role"]
    facts = prepared["facts"]
    machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    for address, size in (
        (CODE, 0x1000),
        (STUBS, 0x1000),
        (EFFECT, 0x1000),
        (PART, 0x1000),
        (WORLD, 0x1000),
        (FORWARD, 0x1000),
        (UP, 0x1000),
        (SEED_ADDRESS, 0x1000),
        (STACK, STACK_SIZE),
    ):
        machine.mem_map(address, size)
    machine.mem_write(CODE, prepared["runtime_raw"])
    machine.mem_write(STUBS, b"\xC3" * 0x1000)

    effect_before = bytearray(seeded_bytes(0x100, seed + 1))
    for index, bits in enumerate(EFFECT_COLOR_BITS):
        write32(effect_before, 0x18 + 4 * index, bits)
    for index, bits in enumerate(EFFECT_VELOCITY_BITS):
        write32(effect_before, 0x24 + 4 * index, bits)
    machine.mem_write(EFFECT, bytes(effect_before))

    part_before = bytearray(seeded_bytes(0x100, seed + 2))
    for offset, bits in (
        (0x24, DEFINITION_INDEX),
        (0x40, LOWER_BITS),
        (0x44, UPPER_BITS),
        (0x48, CONE_BITS),
        (0x60, SCALE_A_FLAGS),
        (0x64, SCALE_B_FLAGS),
    ):
        write32(part_before, offset, bits)
    machine.mem_write(PART, bytes(part_before))

    world_before = seeded_bytes(0x100, seed + 3)
    forward_before = seeded_bytes(0x100, seed + 4)
    up_before = seeded_bytes(0x100, seed + 5)
    seed_before = seeded_bytes(0x100, seed + 6)
    machine.mem_write(WORLD, world_before)
    machine.mem_write(FORWARD, forward_before)
    machine.mem_write(UP, up_before)
    machine.mem_write(SEED_ADDRESS, seed_before)

    stack_before = bytearray(seeded_bytes(STACK_SIZE, seed + 7))
    frame_offset = FRAME - STACK
    saved = {
        "ebp": 0x1122AABB,
        "ebx": 0x2233BBCC,
        "esi": PART,
        "edi": 0x4455DDEE,
    }
    write32(stack_before, frame_offset, saved["ebp"])
    write32(stack_before, frame_offset + 4, STOP)
    write32(stack_before, frame_offset + 8, FORWARD)
    write32(stack_before, frame_offset + 0xC, UP)
    write32(stack_before, frame_offset + 0x10, scale_bits)
    write32(stack_before, frame_offset + facts["saved_ebx_delta"], saved["ebx"])
    write32(stack_before, frame_offset + facts["saved_edi_delta"], saved["edi"])
    caller_before = bytes(stack_before[frame_offset : frame_offset + 0x80])
    machine.mem_write(STACK, bytes(stack_before))

    entry_sp = FRAME + facts["entry_sp_delta"]
    direction_pointer = FRAME + facts["direction_delta"]
    velocity_pointer = FRAME + facts["velocity_delta"]
    color_pointer = FRAME + facts["color_delta"]
    initial_eflags = 0x202 | (0x400 if seed & 1 else 0)
    machine.reg_write(UC_X86_REG_EAX, 0xA0A1A2A3)
    machine.reg_write(UC_X86_REG_ECX, 0xB0B1B2B3)
    machine.reg_write(UC_X86_REG_EDX, 0xC0C1C2C3)
    machine.reg_write(UC_X86_REG_EBX, EFFECT)
    machine.reg_write(UC_X86_REG_ESI, PART)
    machine.reg_write(UC_X86_REG_EDI, WORLD)
    machine.reg_write(UC_X86_REG_EBP, FRAME)
    machine.reg_write(UC_X86_REG_ESP, entry_sp)
    machine.reg_write(UC_X86_REG_EFLAGS, initial_eflags)
    machine.reg_write(UC_X86_REG_FPCW, fpcw)

    calls: list[dict] = []

    def return_from_stub(return_value: int) -> None:
        esp = machine.reg_read(UC_X86_REG_ESP)
        return_address = read32(machine, esp)
        machine.reg_write(UC_X86_REG_EAX, return_value & 0xFFFFFFFF)
        machine.reg_write(UC_X86_REG_ESP, esp + 4)
        machine.reg_write(UC_X86_REG_EIP, return_address)

    def on_code(_machine, address, _size, _user):
        if address == STUB_ADDRESSES["_get_global_local_random_seed_address"]:
            staged = [machine_argument(machine, index) for index in range(8)]
            calls.append({"name": "_get_global_local_random_seed_address", "staged_args": staged})
            return_from_stub(SEED_ADDRESS)
        elif address == STUB_ADDRESSES["_effect_random_translational_velocity"]:
            args = [machine_argument(machine, index) for index in range(9)]
            effect_register = machine.reg_read(UC_X86_REG_EAX) & 0xFFFFFFFF
            calls.append(
                {
                    "name": "_effect_random_translational_velocity",
                    "effect_register": effect_register,
                    "args": args,
                }
            )
            machine.mem_write(args[2], b"".join(p32(bits) for bits in HELPER_DIRECTION_BITS))
            machine.mem_write(args[3], b"".join(p32(bits) for bits in HELPER_VELOCITY_BITS))
            return_from_stub(0x0BADF00D)
        elif address == STUB_ADDRESSES[CALLEE]:
            args = [machine_argument(machine, index) for index in range(6)]
            velocity = [read32(machine, args[2] + 4 * index) for index in range(3)]
            color = [read32(machine, args[3] + 4 * index) for index in range(4)]
            calls.append(
                {
                    "name": CALLEE,
                    "args_first_six_words": args,
                    "velocity_bits": velocity,
                    "color_bits": color,
                    "esp_at_entry": machine.reg_read(UC_X86_REG_ESP) & 0xFFFFFFFF,
                }
            )
            return_from_stub(STUB_RETURN)

    machine.hook_add(u.UC_HOOK_CODE, on_code)
    machine.emu_start(CODE + facts["branch_entry"], STOP, count=1000)

    caller_after = bytes(machine.mem_read(FRAME, 0x80))
    final_nonvolatile = {
        name: machine.reg_read(register) & 0xFFFFFFFF for name, register in NONVOLATILE.items()
    }
    final_call = calls[-1]
    random_call = calls[-2]
    expected_random_args = [
        SEED_ADDRESS,
        FORWARD,
        direction_pointer,
        velocity_pointer,
        LOWER_BITS,
        UPPER_BITS,
        CONE_BITS,
        SCALE_A_FLAGS,
        SCALE_B_FLAGS,
    ]
    correct_scale = final_call["args_first_six_words"][4] == scale_bits
    double_words = promoted_double_words(scale_bits)
    baseline_double = final_call["args_first_six_words"][4:6] == double_words
    checks = {
        "exact_call_order": [item["name"] for item in calls]
        == [
            "_get_global_local_random_seed_address",
            "_effect_random_translational_velocity",
            CALLEE,
        ],
        "random_effect_register": random_call["effect_register"] == EFFECT,
        "random_nine_args": random_call["args"] == expected_random_args,
        "final_definition": final_call["args_first_six_words"][0] == DEFINITION_INDEX,
        "final_world_point": final_call["args_first_six_words"][1] == WORLD,
        "final_velocity_pointer": final_call["args_first_six_words"][2] == velocity_pointer,
        "final_color_pointer": final_call["args_first_six_words"][3] == color_pointer,
        "final_velocity_bits": final_call["velocity_bits"] == EXPECTED_VELOCITY_BITS,
        "final_argb_bits": final_call["color_bits"] == EXPECTED_COLOR_BITS,
        "esp_cdecl_return": machine.reg_read(UC_X86_REG_ESP) == FRAME + 8,
        "eip_stop": machine.reg_read(UC_X86_REG_EIP) == STOP,
        "nonvolatile_preserved": final_nonvolatile == saved,
        "stub_return_in_eax": machine.reg_read(UC_X86_REG_EAX) == STUB_RETURN,
        "caller_frame_memory_exact": caller_after == caller_before,
        "fpcw_preserved": (machine.reg_read(UC_X86_REG_FPCW) & 0xFFFF) == fpcw,
        "x87_top_balanced": (machine.reg_read(UC_X86_REG_FPSW) & 0x3800) == 0,
        "direction_flag_preserved": (machine.reg_read(UC_X86_REG_EFLAGS) & 0x400)
        == (initial_eflags & 0x400),
        "effect_unchanged": bytes(machine.mem_read(EFFECT, 0x100)) == bytes(effect_before),
        "part_unchanged": bytes(machine.mem_read(PART, 0x100)) == bytes(part_before),
        "world_unchanged": bytes(machine.mem_read(WORLD, 0x100)) == world_before,
        "forward_unchanged": bytes(machine.mem_read(FORWARD, 0x100)) == forward_before,
        "up_unchanged": bytes(machine.mem_read(UP, 0x100)) == up_before,
        "seed_storage_unchanged": bytes(machine.mem_read(SEED_ADDRESS, 0x100)) == seed_before,
    }
    if facts["abi"] == "real32":
        checks["fifth_argument_is_exact_real32"] = correct_scale
    else:
        checks["promoted_double_words_exact"] = baseline_double
        checks["correct_real32_predicate_intentionally_fails"] = not correct_scale
    failed = [name for name, passed in checks.items() if not passed]
    if failed:
        raise AssertionError((role, scale_name, hex(fpcw), "failed checks", failed, calls))

    return {
        "role": role,
        "scale_case": scale_name,
        "scale_bits": scale_bits,
        "fpcw": fpcw,
        "calls": calls,
        "correct_real32_predicate": correct_scale,
        "expected_promoted_double_words": double_words,
        "final_esp": machine.reg_read(UC_X86_REG_ESP) & 0xFFFFFFFF,
        "final_eip": machine.reg_read(UC_X86_REG_EIP) & 0xFFFFFFFF,
        "final_eax": machine.reg_read(UC_X86_REG_EAX) & 0xFFFFFFFF,
        "final_eflags": machine.reg_read(UC_X86_REG_EFLAGS) & 0xFFFFFFFF,
        "final_fpcw": machine.reg_read(UC_X86_REG_FPCW) & 0xFFFF,
        "final_fpsw": machine.reg_read(UC_X86_REG_FPSW) & 0xFFFF,
        "final_nonvolatile": final_nonvolatile,
        "checks": checks,
    }


def semantic_signature(run: dict) -> dict:
    random_call = run["calls"][1]
    final_call = run["calls"][2]
    return {
        "call_order": [item["name"] for item in run["calls"]],
        "random_effect_register": random_call["effect_register"],
        "random_scalar_args": [random_call["args"][0], random_call["args"][1], *random_call["args"][4:]],
        "definition": final_call["args_first_six_words"][0],
        "world_point": final_call["args_first_six_words"][1],
        "velocity_bits": final_call["velocity_bits"],
        "color_bits": final_call["color_bits"],
        "scale_bits": final_call["args_first_six_words"][4],
        "eax": run["final_eax"],
        "esp": run["final_esp"],
        "eip": run["final_eip"],
        "nonvolatile": run["final_nonvolatile"],
        "fpcw": run["final_fpcw"],
        "x87_top": run["final_fpsw"] & 0x3800,
    }


def compare_correct(target: dict, schema: dict) -> dict:
    target_signature = semantic_signature(target)
    schema_signature = semantic_signature(schema)
    if target_signature != schema_signature:
        raise AssertionError(("target/schema semantic signature", target_signature, schema_signature))
    return {"equal": True, "signature": target_signature}


def compare_baseline_negative(before: dict, schema: dict) -> dict:
    before_call = before["calls"][2]
    schema_call = schema["calls"][2]
    equal_non_scale_semantics = {
        "call_order": [item["name"] for item in before["calls"]]
        == [item["name"] for item in schema["calls"]],
        "first_four_final_arguments": before_call["args_first_six_words"][:4]
        == schema_call["args_first_six_words"][:4],
        "velocity": before_call["velocity_bits"] == schema_call["velocity_bits"],
        "color": before_call["color_bits"] == schema_call["color_bits"],
        "stack_balanced": before["final_esp"] == schema["final_esp"] == FRAME + 8,
        "nonvolatile": before["final_nonvolatile"] == schema["final_nonvolatile"],
    }
    scale_words = before_call["args_first_six_words"][4:6]
    checks = {
        **equal_non_scale_semantics,
        "baseline_exact_promoted_double": scale_words == before["expected_promoted_double_words"],
        "baseline_fails_real32_contract": not before["correct_real32_predicate"],
        "schema_passes_real32_contract": schema["correct_real32_predicate"],
    }
    if not all(checks.values()):
        raise AssertionError(("baseline negative witness", checks))
    return {
        "pass": True,
        "classification": "intentional frozen-binary negative witness",
        "checks": checks,
        "baseline_argument_words_5_and_6": scale_words,
        "schema_argument_word_5": schema_call["args_first_six_words"][4],
    }


def require_correct_pair(target: dict, schema: dict) -> None:
    if semantic_signature(target) != semantic_signature(schema):
        raise AssertionError("semantic outcome mismatch")


def synthetic_negative_controls(target: dict, schema: dict) -> dict:
    mutations = {
        "scale_word": ("calls", 2, "args_first_six_words", 4),
        "velocity_component": ("calls", 2, "velocity_bits", 1),
        "color_component": ("calls", 2, "color_bits", 2),
        "stack_pointer": ("final_esp",),
        "saved_ebx": ("final_nonvolatile", "ebx"),
    }
    results = {}
    for name, path in mutations.items():
        mutant = copy.deepcopy(schema)
        cursor = mutant
        for key in path[:-1]:
            cursor = cursor[key]
        old = cursor[path[-1]]
        cursor[path[-1]] = (old ^ 1) & 0xFFFFFFFF
        detected = False
        try:
            require_correct_pair(target, mutant)
        except AssertionError:
            detected = True
        results[name] = {
            "classification": "synthetic outcome mutation, not a binary mutation",
            "from": old,
            "to": cursor[path[-1]],
            "detected": detected,
        }
    if not all(item["detected"] for item in results.values()):
        raise AssertionError(("synthetic negative controls", results))
    return {"pass": True, "count": len(results), "results": results}


def build_report() -> dict:
    frozen_context = verify_frozen_context()
    callee = verify_callee()
    prepared = {
        "before": prepare("before", BEFORE, PINS["before_object"]),
        "schema": prepare("schema", SCHEMA, PINS["schema_object"]),
        "target": prepare("target", TARGET, PINS["target_object"]),
    }
    runs = []
    comparisons = []
    baseline_witnesses = []
    first_target = None
    first_schema = None
    for case_index, (scale_name, scale_bits) in enumerate(SCALE_CASES):
        for mode_index, fpcw in enumerate(FPCW_VALUES):
            seed = 0x30 + case_index * len(FPCW_VALUES) + mode_index
            target_run = execute(prepared["target"], scale_name, scale_bits, fpcw, seed)
            schema_run = execute(prepared["schema"], scale_name, scale_bits, fpcw, seed)
            before_run = execute(prepared["before"], scale_name, scale_bits, fpcw, seed)
            comparisons.append(
                {
                    "scale_case": scale_name,
                    "scale_bits": scale_bits,
                    "fpcw": fpcw,
                    **compare_correct(target_run, schema_run),
                }
            )
            baseline_witnesses.append(
                {
                    "scale_case": scale_name,
                    "scale_bits": scale_bits,
                    "fpcw": fpcw,
                    **compare_baseline_negative(before_run, schema_run),
                }
            )
            runs.extend([target_run, schema_run, before_run])
            if first_target is None:
                first_target = target_run
                first_schema = schema_run

    negative_controls = synthetic_negative_controls(first_target, first_schema)
    object_facts = {}
    for role, item in prepared.items():
        object_facts[role] = {
            "path": str(item["path"]),
            "sha256": PINS[f"{role}_object"],
            "size": item["function"].info["size"],
            "relocations": item["function"].info["relocation_count"],
            "normalized_sha256": item["function"].info["normalized_sha256"],
            "selection": item["selection"],
            "tail_relocations": item["tail_relocations"],
            "abi_instruction_offset": item["abi_instruction_offset"],
            "abi_instruction_bytes": item["abi_instruction_bytes"],
        }

    return {
        "pass": True,
        "scope": {
            "function": FUNCTION,
            "path": "PARTICLE_SYSTEM_DEFINITION_TAG branch only",
            "real_bytes": ["before", "schema", "January target"],
            "stubbed_external_calls": list(STUB_ADDRESSES),
            "fpcw_modes": list(FPCW_VALUES),
            "fpcw_note": "two disclosed conventional modes; no claim that either establishes the game process default",
            "x87_note": "FPSW mask checks balanced TOP only; it does not claim an empty physical register stack",
        },
        "object_facts": object_facts,
        "callee_evidence": callee,
        "frozen_context": frozen_context,
        "coverage": {
            "scale_cases": [{"name": name, "bits": bits} for name, bits in SCALE_CASES],
            "correct_pair_comparisons": len(comparisons),
            "baseline_negative_witnesses": len(baseline_witnesses),
            "executions": len(runs),
        },
        "comparisons": comparisons,
        "baseline_negative_witnesses": baseline_witnesses,
        "synthetic_negative_controls": negative_controls,
        "runs": runs,
        "verdict": {
            "schema_matches_january_real32_call_contract": True,
            "before_uses_wrong_promoted_double_contract": True,
            "stack_and_nonvolatile_preserved_all_runs": True,
            "collateral_runtime_owner_count": 1,
            "other_consumer_runtime_sections_unchanged": 618,
            "compiler_local_label_only_sections": 5,
            "new_or_lost_runtime_sections": 0,
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=OUTPUT_DEFAULT)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        raise SystemExit(f"refusing to overwrite existing report: {output}")
    if ROOT not in output.parents:
        raise SystemExit(f"output must stay beneath repository root: {output}")
    report = build_report()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(
        "PASS "
        f"executions={report['coverage']['executions']} "
        f"correct_pairs={report['coverage']['correct_pair_comparisons']} "
        f"baseline_negative={report['coverage']['baseline_negative_witnesses']} "
        f"synthetic_negative={report['synthetic_negative_controls']['count']} "
        f"report={output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
