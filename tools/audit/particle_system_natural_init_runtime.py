"""Bounded target-first runtime oracle for a particle-system constructor pair.

This diagnostic executes the real i386 COFF bytes for
``particle_system_new_unattached`` and its private
``particle_system_initialize`` together.  It compares the frozen January
target, the first natural readable candidate, and the ordinary final object.

Only genuine external dependencies are modeled: datum allocation/access/
deletion, tag access, scenario location, particle lighting, local seeded
random range, and the still-unwritten single-system update routine.  The
typed stubs validate cdecl arguments and order and model only the narrow
effects this pair observes.  They are not executions of the engine's data
array, tag cache, lighting, RNG, or update implementations.

The oracle writes only its requested JSON report.  It does not compile,
invoke a build system, or modify source, configuration, or frozen inputs.
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
from tools.regression_gate import _section_aux_selection  # noqa: E402


BEFORE_MANIFEST_DEFAULT = ROOT / "scratch/particle-systems-natural-init-before-20260906.json"
FIRST_DEFAULT = ROOT / "scratch/particle-systems-natural-init-first-20260906.obj"
ACTUAL_DEFAULT = ROOT / "scratch/particle-systems-natural-init-final-20260906/source/effects/particle_systems.obj"
ACTUAL_MANIFEST_DEFAULT = ROOT / "scratch/particle-systems-natural-init-final-20260906.json"
OUTPUT_DEFAULT = ROOT / "scratch/particle-system-natural-init-runtime-differential-report.json"

PINNED_HASHES = {
    "before_manifest": "78a20ee5df5cdcf11d1966f5e165a749936d4d2c135e21bf47c194d2fbacc7c1",
    "target": "25cc3edef1c9b54ba12f677a88a0b5b93aff8f66fcdff7b29d8faa532b939e56",
    "first": "2b77e6f2d6172b68eb39e112ff15eb00b37dd7882ebf368ea73540e305b6a7bb",
    "actual": "dbd5c047795668b690db29b64c13e5a22096cafd0942d7f9c4e02d4a1e1248b9",
    "actual_manifest": "288d0bb6a9b7c59baedb5d74e4f37e7b7d5c2178cad29183f785d08d24124def",
    "source": "4e5da4295689ee5901cf086d725f8360ed58291790a49ea91403e72d861aa520",
}

FUNCTIONS = {
    "initialize": {
        "target_name": "_code_0008f640",
        "candidate_name": "_particle_system_initialize",
        "size": 288,
        "relocations": 9,
        "target_normalized": "ca7322edb56c3275a3480eed1dfbfd87edfb8152a11ff78a201a35fceee09409",
        "candidate_normalized": "35f03b7cc10bfb87ba227093a550ab55df522596813eeae9746560f8b04db055",
        "target_addresses": [0x0B, 0x14, 0x24, 0x37, 0x71, 0xA9, 0xC3, 0xC9, 0x107],
        "candidate_addresses": [0x0B, 0x15, 0x25, 0x38, 0x68, 0xA7, 0xC1, 0xC7, 0x102],
        "targets": [
            "global:particle_systems",
            "api:datum_get",
            "api:tag_get",
            "api:scenario_location_from_point",
            "api:tag_block_get_element_with_size",
            "api:tag_block_get_element_with_size",
            "api:get_global_local_random_seed_address",
            "api:real_seed_random_range",
            "dependency:code_0008e7f0",
        ],
    },
    "unattached": {
        "target_name": "_particle_system_new_unattached",
        "candidate_name": "_particle_system_new_unattached",
        "size": 208,
        "relocations": 8,
        "target_normalized": "39b66f0d516aefd4eee70e8ddb5c9ce80138d8a1f11f467f688738ef348b8e38",
        "candidate_normalized": "39b66f0d516aefd4eee70e8ddb5c9ce80138d8a1f11f467f688738ef348b8e38",
        "target_addresses": [0x07, 0x0E, 0x22, 0x2B, 0x9C, 0xA2, 0xB0, 0xB7],
        "candidate_addresses": [0x07, 0x0E, 0x22, 0x2B, 0x9C, 0xA2, 0xB0, 0xB7],
        "targets": [
            "global:particle_systems",
            "api:datum_new",
            "global:particle_systems",
            "api:datum_get",
            "api:light_particle",
            "internal:particle_system_initialize",
            "global:particle_systems",
            "api:datum_delete",
        ],
    },
}

CODE_UNATTACHED = 0x100000
CODE_INITIALIZE = 0x110000
STOP = 0x120000
STUBS = 0x200000
GLOBALS = 0x300000
PARTICLE_SYSTEMS_SLOT = GLOBALS + 0x100
RNG_RESULT = GLOBALS + 0x200
PARTICLE_SYSTEMS_TOKEN = 0x31415926

SYSTEM_PAGE = 0x400000
SYSTEM = SYSTEM_PAGE + 0x800
SYSTEM_SIZE = 0x158
DEFINITION_PAGE = 0x500000
DEFINITION = DEFINITION_PAGE + 0x800
TYPE_PAGE = 0x510000
TYPE_BASE = TYPE_PAGE + 0x200
TYPE_STRIDE = 0x80
STATE_PAGE = 0x520000
STATE_BASE = STATE_PAGE + 0x100
STATE_STRIDE = 0x200
STATE_SIZE = 0xC0
INPUT_PAGE = 0x600000
POSITION = INPUT_PAGE + 0x100
VELOCITY = INPUT_PAGE + 0x200
COLOR = INPUT_PAGE + 0x300
SEED_PAGE = 0x610000
SEED_ADDRESS = SEED_PAGE + 0x100
STACK_PAGE = 0x700000
STACK_SIZE = 0x20000
ENTRY_SP = STACK_PAGE + 0x10000
CALLER_WINDOW_SIZE = 0x80
PAGE_SIZE = 0x1000

SYSTEM_HANDLE = 0x4A3B0000
SYSTEM_IDENTIFIER = SYSTEM_HANDLE >> 16
DEFINITION_INDEX = 0x12345678
PCTL_TAG = 0x7063746C
INITIAL_UPDATE_BITS = 0x3A83126F
FPCW_VALUES = (0x027F, 0x037F)

SYSTEM_FLAGS = 0x04
SYSTEM_DEFINITION = 0x08
SYSTEM_OBJECT = 0x0C
SYSTEM_SCALE = 0x14
SYSTEM_LOCATION = 0x18
SYSTEM_POSITION = 0x20
SYSTEM_VELOCITY = 0x2C
SYSTEM_COLOR = 0x38
SYSTEM_LIGHTING = 0x48
SYSTEM_TYPES = 0x58
RUNTIME_TYPE_STRIDE = 0x40

TYPE_STATE_INDEX = 0x00
TYPE_TRANSITION_INDEX = 0x02
TYPE_TIME_LEFT = 0x04
TYPE_STATE_LENGTH = 0x08
TYPE_MOVING_FORWARD = 0x38
TYPE_PARTICLE_COUNT = 0x3A
TYPE_FIRST_PARTICLE = 0x3C

POSITION_BITS = (0x3F800000, 0xC0200000, 0x40700000)
VELOCITY_BITS = (0xBE800000, 0x3F400000, 0xC1000000)
COLOR_BITS = (0x3F19999A, 0x3E4CCCCD, 0x3F666666, 0x3F000000)
SCALE_BITS = 0x3F400000
LIGHT_BITS = (0x3DCCCCCD, 0x3E4CCCCD, 0x3E99999A)
DIFFUSE_BITS = (0x3F000000, 0x3F200000, 0x3F400000)
LOCATION_WORDS = (0x10203040, 0x50607080)
LOWER_BITS = (0x3F000000, 0xBF800000, 0x00000000, 0x3DCCCCCD)
UPPER_BITS = (0x3FC00000, 0x40000000, 0x41200000, 0x3E4CCCCD)
DURATION_BITS = (0x3F800000, 0x3E800000, 0x40A00000, 0x3E19999A)

NONVOLATILE = {
    UC_X86_REG_EBP: ("ebp", 0x1122AABB),
    UC_X86_REG_EBX: ("ebx", 0x2233BBCC),
    UC_X86_REG_ESI: ("esi", 0x3344CCDD),
    UC_X86_REG_EDI: ("edi", 0x4455DDEE),
}

STUB_NAMES = (
    "_datum_new",
    "_datum_get",
    "_datum_delete",
    "_tag_get",
    "_scenario_location_from_point",
    "_tag_block_get_element_with_size",
    "_light_particle",
    "_get_global_local_random_seed_address",
    "_real_seed_random_range",
    "_code_0008e7f0",
)
STUB_ADDRESSES = {name: STUBS + 0x80 * (index + 1) for index, name in enumerate(STUB_NAMES)}


def p16(value: int) -> bytes:
    return struct.pack("<H", value & 0xFFFF)


def p32(value: int) -> bytes:
    return struct.pack("<I", value & 0xFFFFFFFF)


def read_u16(data: bytes | bytearray, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def read_u32(data: bytes | bytearray, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def machine_u32(machine: u.Uc, address: int) -> int:
    return struct.unpack("<I", machine.mem_read(address, 4))[0]


def put16(data: bytearray, offset: int, value: int) -> None:
    data[offset : offset + 2] = p16(value)


def put32(data: bytearray, offset: int, value: int) -> None:
    data[offset : offset + 4] = p32(value)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def seeded_bytes(size: int, seed: int) -> bytes:
    return bytes(((index * 37 + seed * 53 + 11) & 0xFF) for index in range(size))


def changed_ranges(before: bytes, after: bytes) -> list[list[int]]:
    changed = [index for index, pair in enumerate(zip(before, after)) if pair[0] != pair[1]]
    if not changed:
        return []
    result = []
    start = previous = changed[0]
    for index in changed[1:]:
        if index != previous + 1:
            result.append([start, previous + 1])
            start = index
        previous = index
    result.append([start, previous + 1])
    return result


def verify_hash(path: Path, expected: str, label: str) -> dict:
    actual = sha256_path(path)
    if actual.lower() != expected.lower():
        raise AssertionError((label, "sha256", str(path), actual, expected))
    return {"path": str(path), "sha256": actual}


def manifest_target(path: Path, expected_manifest_hash: str) -> tuple[Path, str, dict]:
    manifest_guard = verify_hash(path, expected_manifest_hash, "before manifest")
    data = json.loads(path.read_text(encoding="utf-8"))
    record = data["frozen_files"]["build/split/source/effects/particle_systems.obj"]
    target = Path(record["path"])
    expected = record["sha256"].lower()
    verify_hash(target, expected, "frozen target")
    if expected != PINNED_HASHES["target"]:
        raise AssertionError(("manifest target pin changed", expected, PINNED_HASHES["target"]))
    return target, expected, manifest_guard


def verify_actual_manifest(path: Path, expected_hash: str, actual: Path, actual_hash: str) -> dict:
    guard = verify_hash(path, expected_hash, "actual manifest")
    data = json.loads(path.read_text(encoding="utf-8"))
    record = data["objects"]["source/effects/particle_systems"]
    if Path(record["path"]).resolve() != actual.resolve() or record["sha256"].lower() != actual_hash.lower():
        raise AssertionError(("actual manifest/object mismatch", record, str(actual), actual_hash))
    source = data["frozen_files"]["source/effects/particle_systems.c"]
    if source["sha256"].lower() != PINNED_HASHES["source"]:
        raise AssertionError(("actual source pin changed", source["sha256"], PINNED_HASHES["source"]))
    verify_hash(Path(source["path"]), source["sha256"], "actual frozen source")
    guard["source"] = source
    return guard


def canonical_relocation(kind: str, name: str) -> str:
    common = {
        "_particle_systems": "global:particle_systems",
        "_datum_new": "api:datum_new",
        "_datum_get": "api:datum_get",
        "_datum_delete": "api:datum_delete",
        "_tag_get": "api:tag_get",
        "_scenario_location_from_point": "api:scenario_location_from_point",
        "_tag_block_get_element_with_size": "api:tag_block_get_element_with_size",
        "_light_particle": "api:light_particle",
        "_get_global_local_random_seed_address": "api:get_global_local_random_seed_address",
        "_real_seed_random_range": "api:real_seed_random_range",
        "_code_0008e7f0": "dependency:code_0008e7f0",
    }
    if name in common:
        return common[name]
    if kind == "unattached" and name in {"_code_0008f640", "_particle_system_initialize"}:
        return "internal:particle_system_initialize"
    raise AssertionError((kind, "unexpected relocation destination", name))


def relocation_destination(identity: str) -> int:
    if identity == "global:particle_systems":
        return PARTICLE_SYSTEMS_SLOT
    if identity == "internal:particle_system_initialize":
        return CODE_INITIALIZE
    mapping = {
        "api:datum_new": "_datum_new",
        "api:datum_get": "_datum_get",
        "api:datum_delete": "_datum_delete",
        "api:tag_get": "_tag_get",
        "api:scenario_location_from_point": "_scenario_location_from_point",
        "api:tag_block_get_element_with_size": "_tag_block_get_element_with_size",
        "api:light_particle": "_light_particle",
        "api:get_global_local_random_seed_address": "_get_global_local_random_seed_address",
        "api:real_seed_random_range": "_real_seed_random_range",
        "dependency:code_0008e7f0": "_code_0008e7f0",
    }
    return STUB_ADDRESSES[mapping[identity]]


def prepare_function(path: Path, role: str, kind: str) -> dict:
    obj = cc.load(path)
    spec = FUNCTIONS[kind]
    name = spec["target_name"] if role == "target" else spec["candidate_name"]
    symbol = cc.symbol(obj, name)
    section = obj["sections"][symbol["section"] - 1]
    info = cc.section_info(obj, name)
    raw_original = bytes(cc._section_bytes(obj, section))
    runtime = bytearray(raw_original)
    base = CODE_INITIALIZE if kind == "initialize" else CODE_UNATTACHED
    relocations = []
    for relocation in info["relocations"]:
        target = relocation.get("symbolic_target", relocation["target"])
        if not isinstance(target, (list, tuple)) or len(target) != 3 or target[0] != "symbol":
            raise AssertionError((role, kind, "unresolved relocation", target))
        target_name, addend = target[1], int(target[2])
        identity = canonical_relocation(kind, target_name)
        destination = relocation_destination(identity) + addend
        if relocation["type"] == cc.IMAGE_REL_I386_REL32:
            value = destination - (base + relocation["address"] + 4)
        elif relocation["type"] == cc.IMAGE_REL_I386_DIR32:
            value = destination
        else:
            raise AssertionError((role, kind, "unsupported relocation type", relocation))
        runtime[relocation["address"] : relocation["address"] + 4] = p32(value)
        relocations.append([relocation["address"], relocation["type"], identity, addend])

    expected_addresses = spec["target_addresses"] if role == "target" else spec["candidate_addresses"]
    expected_normalized = spec["target_normalized"] if role == "target" else spec["candidate_normalized"]
    expected_storage = 2 if role == "target" or kind == "unattached" else 3
    expected_types = [
        cc.IMAGE_REL_I386_DIR32 if identity.startswith("global:") else cc.IMAGE_REL_I386_REL32
        for identity in spec["targets"]
    ]
    checks = {
        "padded_size": info["size"] == spec["size"],
        "relocation_count": info["relocation_count"] == spec["relocations"],
        "normalized_sha256": info["normalized_sha256"] == expected_normalized,
        "relocation_addresses": [item[0] for item in relocations] == expected_addresses,
        "relocation_types": [item[1] for item in relocations] == expected_types,
        "relocation_targets": [item[2] for item in relocations] == spec["targets"],
        "relocation_addends_zero": all(item[3] == 0 for item in relocations),
        "code_section_flags": section["flags"] == 0x60501020,
        "function_type": symbol["type"] == 0x20,
        "storage": symbol["storage"] == expected_storage,
        "comdat_selection_noduplicates": _section_aux_selection(obj, symbol["section"]) == 1,
    }
    failed = [key for key, passed in checks.items() if not passed]
    if failed:
        raise AssertionError((role, kind, "COFF guard", failed, relocations, info))
    return {
        "role": role,
        "kind": kind,
        "name": name,
        "raw": raw_original,
        "runtime_raw": bytes(runtime),
        "raw_sha256": sha256_bytes(raw_original),
        "normalized_sha256": info["normalized_sha256"],
        "relocations": relocations,
        "checks": checks,
        "storage": symbol["storage"],
    }


def helper_fact(path: Path) -> dict | None:
    obj = cc.load(path)
    matches = [symbol for symbol in obj["symbols"] if symbol["name"] == "_real_local_random_range" and symbol["section"] > 0]
    if not matches:
        return None
    if len(matches) != 1:
        raise AssertionError((str(path), "ambiguous real_local_random_range", matches))
    symbol = matches[0]
    info = cc.section_info(obj, symbol["name"])
    return {
        "name": symbol["name"],
        "size": info["size"],
        "relocation_count": info["relocation_count"],
        "normalized_sha256": info["normalized_sha256"],
        "storage": symbol["storage"],
        "selection": _section_aux_selection(obj, symbol["section"]),
    }


def prepare_role(path: Path, role: str, expected_hash: str) -> dict:
    guard = verify_hash(path, expected_hash, role)
    return {
        "role": role,
        "path": str(path),
        "sha256": guard["sha256"],
        "initialize": prepare_function(path, role, "initialize"),
        "unattached": prepare_function(path, role, "unattached"),
        "real_local_random_range_helper": helper_fact(path),
    }


def make_case(name: str, entry: str, counts: list[int], *, allocation_none: bool = False) -> dict:
    return {
        "name": name,
        "entry": entry,
        "state_counts": counts,
        "allocation_none": allocation_none,
    }


def cases() -> list[dict]:
    return [
        make_case("initialize_types_0_success_update", "initialize", []),
        make_case("initialize_types_1_positive", "initialize", [1]),
        make_case("initialize_types_4_positive", "initialize", [1, 1, 2, 1]),
        make_case("initialize_mixed_zero_continues", "initialize", [1, 0, 1, 0]),
        make_case("unattached_allocation_none", "unattached", [1], allocation_none=True),
        make_case("unattached_types_0_success", "unattached", []),
        make_case("unattached_one_positive_success", "unattached", [1]),
        make_case("unattached_four_positive_success", "unattached", [1, 2, 1, 3]),
        make_case("unattached_mixed_zero_failure_delete", "unattached", [1, 0, 1, 1]),
        make_case("unattached_one_zero_failure_delete", "unattached", [0]),
    ]


def page_offset(address: int, page: int) -> int:
    if not page <= address < page + PAGE_SIZE:
        raise AssertionError(("address outside page", hex(address), hex(page)))
    return address - page


def initial_pages(case: dict, seed: int) -> dict[str, bytes]:
    pages = {
        "system": bytearray(seeded_bytes(PAGE_SIZE, seed + 1)),
        "definition": bytearray(seeded_bytes(PAGE_SIZE, seed + 2)),
        "types": bytearray(seeded_bytes(PAGE_SIZE, seed + 3)),
        "states": bytearray(seeded_bytes(PAGE_SIZE, seed + 4)),
        "inputs": bytearray(seeded_bytes(PAGE_SIZE, seed + 5)),
        "globals": bytearray(seeded_bytes(PAGE_SIZE, seed + 6)),
        "seed": bytearray(seeded_bytes(PAGE_SIZE, seed + 7)),
    }
    system = page_offset(SYSTEM, SYSTEM_PAGE)
    put16(pages["system"], system, SYSTEM_IDENTIFIER)
    put32(pages["system"], system + SYSTEM_DEFINITION, DEFINITION_INDEX)
    put32(pages["system"], system + SYSTEM_FLAGS, 0xA4B5C6D0)
    for index, bits in enumerate(POSITION_BITS):
        put32(pages["system"], system + SYSTEM_POSITION + 4 * index, bits)

    definition = page_offset(DEFINITION, DEFINITION_PAGE)
    put32(pages["definition"], definition + 0x5C, len(case["state_counts"]))
    put32(pages["definition"], definition + 0x60, TYPE_BASE)
    for index, count in enumerate(case["state_counts"]):
        type_offset = page_offset(TYPE_BASE + index * TYPE_STRIDE, TYPE_PAGE)
        put32(pages["types"], type_offset + 0x68, count)
        put32(pages["types"], type_offset + 0x6C, STATE_BASE + index * STATE_STRIDE)
        state_offset = page_offset(STATE_BASE + index * STATE_STRIDE, STATE_PAGE)
        put32(pages["states"], state_offset + 0x20, LOWER_BITS[index])
        put32(pages["states"], state_offset + 0x24, UPPER_BITS[index])

    for address, words in ((POSITION, POSITION_BITS), (VELOCITY, VELOCITY_BITS), (COLOR, COLOR_BITS)):
        offset = page_offset(address, INPUT_PAGE)
        for index, bits in enumerate(words):
            put32(pages["inputs"], offset + 4 * index, bits)
    put32(pages["globals"], page_offset(PARTICLE_SYSTEMS_SLOT, GLOBALS), PARTICLE_SYSTEMS_TOKEN)
    put32(pages["globals"], page_offset(RNG_RESULT, GLOBALS), 0x7FC01234)
    put32(pages["seed"], page_offset(SEED_ADDRESS, SEED_PAGE), 0x13579BDF ^ seed)
    return {name: bytes(data) for name, data in pages.items()}


def normalized_event(name: str, args: list, **extra) -> dict:
    result = {"name": name, "args": args}
    result.update(extra)
    return result


def advance_seed(seed_page: bytearray, ordinal: int) -> None:
    offset = page_offset(SEED_ADDRESS, SEED_PAGE)
    previous = read_u32(seed_page, offset)
    put32(seed_page, offset, previous * 1664525 + 1013904223 + ordinal)


def simulate_initialize(case: dict, system_page: bytearray, seed_page: bytearray, events: list[dict]) -> bool:
    record = page_offset(SYSTEM, SYSTEM_PAGE)
    events.append(normalized_event("body:particle_system_initialize", [SYSTEM_HANDLE]))
    events.append(normalized_event("datum_get", ["particle_systems", SYSTEM_HANDLE]))
    events.append(normalized_event("tag_get", [PCTL_TAG, DEFINITION_INDEX]))
    events.append(normalized_event("scenario_location_from_point", ["system.location", "system.position"]))
    for index, word in enumerate(LOCATION_WORDS):
        put32(system_page, record + SYSTEM_LOCATION + 4 * index, word)
    put32(system_page, record + SYSTEM_FLAGS, read_u32(system_page, record + SYSTEM_FLAGS) | 2)

    success = True
    rng_ordinal = 0
    for index, count in enumerate(case["state_counts"]):
        events.append(normalized_event("tag_block_get_element_with_size", ["definition.types", index, TYPE_STRIDE]))
        runtime = record + SYSTEM_TYPES + index * RUNTIME_TYPE_STRIDE
        if count != 0:
            put16(system_page, runtime + TYPE_STATE_INDEX, 0)
            put16(system_page, runtime + TYPE_TRANSITION_INDEX, 0xFFFF)
            system_page[runtime + TYPE_MOVING_FORWARD] = 1
            put16(system_page, runtime + TYPE_PARTICLE_COUNT, 0)
            put32(system_page, runtime + TYPE_FIRST_PARTICLE, 0xFFFFFFFF)
            if count > 0:
                events.append(normalized_event("tag_block_get_element_with_size", [f"type[{index}].type_states", 0, STATE_SIZE]))
                events.append(normalized_event("get_global_local_random_seed_address", []))
                events.append(normalized_event(
                    "real_seed_random_range",
                    ["local_seed", LOWER_BITS[index], UPPER_BITS[index]],
                    return_bits=DURATION_BITS[index],
                ))
                advance_seed(seed_page, rng_ordinal)
                rng_ordinal += 1
                put32(system_page, runtime + TYPE_TIME_LEFT, DURATION_BITS[index])
                put32(system_page, runtime + TYPE_STATE_LENGTH, DURATION_BITS[index])
        else:
            success = False
    if success:
        events.append(normalized_event("code_0008e7f0", [INITIAL_UPDATE_BITS, SYSTEM_HANDLE]))
    return success


def reference(case: dict, seed: int) -> dict:
    initial = initial_pages(case, seed)
    system_page = bytearray(initial["system"])
    seed_page = bytearray(initial["seed"])
    events: list[dict] = []
    deleted = False
    if case["entry"] == "initialize":
        success = simulate_initialize(case, system_page, seed_page, events)
        return_value = int(success)
        return_kind = "al"
    else:
        events.append(normalized_event("body:particle_system_new_unattached", [
            DEFINITION_INDEX, "position", "velocity", "color", SCALE_BITS,
        ]))
        events.append(normalized_event("datum_new", ["particle_systems"]))
        if case["allocation_none"]:
            return_value = 0xFFFFFFFF
        else:
            record = page_offset(SYSTEM, SYSTEM_PAGE)
            system_page[record : record + SYSTEM_SIZE] = bytes(SYSTEM_SIZE)
            put16(system_page, record, SYSTEM_IDENTIFIER)
            events.append(normalized_event("datum_get", ["particle_systems", SYSTEM_HANDLE]))
            put32(system_page, record + SYSTEM_DEFINITION, DEFINITION_INDEX)
            put32(system_page, record + SYSTEM_OBJECT, 0xFFFFFFFF)
            for offset, words in (
                (SYSTEM_POSITION, POSITION_BITS),
                (SYSTEM_VELOCITY, VELOCITY_BITS),
                (SYSTEM_COLOR, COLOR_BITS),
            ):
                for index, bits in enumerate(words):
                    put32(system_page, record + offset + 4 * index, bits)
            put32(system_page, record + SYSTEM_SCALE, SCALE_BITS)
            put32(system_page, record + SYSTEM_FLAGS, read_u32(system_page, record + SYSTEM_FLAGS) | 1)
            events.append(normalized_event("light_particle", ["system.position", "system.lighting", "stack_local", 0]))
            for index, bits in enumerate(LIGHT_BITS):
                put32(system_page, record + SYSTEM_LIGHTING + 4 * index, bits)
            success = simulate_initialize(case, system_page, seed_page, events)
            if success:
                return_value = SYSTEM_HANDLE
            else:
                events.append(normalized_event("datum_delete", ["particle_systems", SYSTEM_HANDLE]))
                put16(system_page, record, 0)
                deleted = True
                return_value = 0xFFFFFFFF
        return_kind = "eax"
    return {
        "initial": initial,
        "system": bytes(system_page),
        "seed": bytes(seed_page),
        "events": events,
        "return_value": return_value,
        "return_kind": return_kind,
        "deleted": deleted,
    }


def machine_argument(machine: u.Uc, index: int) -> int:
    return machine_u32(machine, machine.reg_read(UC_X86_REG_ESP) + 4 + 4 * index)


class Runtime:
    def __init__(self, prepared: dict, case: dict, seed: int, fpcw: int):
        self.prepared = prepared
        self.case = case
        self.seed = seed
        self.fpcw = fpcw
        self.expected = reference(case, seed)
        self.initial = self.expected["initial"]
        self.machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        self.events: list[dict] = []
        self.rng_ordinal = 0
        self.deleted = False
        self.stub_mutations: list[dict] = []

    def map_memory(self) -> None:
        for address, size in (
            (CODE_UNATTACHED, PAGE_SIZE),
            (CODE_INITIALIZE, PAGE_SIZE),
            (STOP, PAGE_SIZE),
            (STUBS, PAGE_SIZE),
            (GLOBALS, PAGE_SIZE),
            (SYSTEM_PAGE, PAGE_SIZE),
            (DEFINITION_PAGE, PAGE_SIZE),
            (TYPE_PAGE, PAGE_SIZE),
            (STATE_PAGE, PAGE_SIZE),
            (INPUT_PAGE, PAGE_SIZE),
            (SEED_PAGE, PAGE_SIZE),
            (STACK_PAGE, STACK_SIZE),
        ):
            self.machine.mem_map(address, size)
        self.machine.mem_write(CODE_UNATTACHED, self.prepared["unattached"]["runtime_raw"])
        self.machine.mem_write(CODE_INITIALIZE, self.prepared["initialize"]["runtime_raw"])
        self.machine.mem_write(STUBS, b"\xC3" * PAGE_SIZE)
        rng_stub = b"\xD9\x05" + p32(RNG_RESULT) + b"\xC3"
        self.machine.mem_write(STUB_ADDRESSES["_real_seed_random_range"], rng_stub)
        for name, address in (
            ("globals", GLOBALS),
            ("system", SYSTEM_PAGE),
            ("definition", DEFINITION_PAGE),
            ("types", TYPE_PAGE),
            ("states", STATE_PAGE),
            ("inputs", INPUT_PAGE),
            ("seed", SEED_PAGE),
        ):
            self.machine.mem_write(address, self.initial[name])

    def set_frame(self) -> bytes:
        stack = bytearray(seeded_bytes(STACK_SIZE, self.seed + 20))
        frame = ENTRY_SP - STACK_PAGE
        put32(stack, frame, STOP)
        if self.case["entry"] == "initialize":
            args = [SYSTEM_HANDLE]
        else:
            args = [DEFINITION_INDEX, POSITION, VELOCITY, COLOR, SCALE_BITS]
        for index, value in enumerate(args):
            put32(stack, frame + 4 + 4 * index, value)
        self.machine.mem_write(STACK_PAGE, bytes(stack))
        caller = bytes(stack[frame : frame + CALLER_WINDOW_SIZE])
        self.machine.reg_write(UC_X86_REG_ESP, ENTRY_SP)
        for register, (_, value) in NONVOLATILE.items():
            self.machine.reg_write(register, value)
        self.machine.reg_write(UC_X86_REG_EAX, 0xA0A1A2A3)
        self.machine.reg_write(UC_X86_REG_ECX, 0xB0B1B2B3)
        self.machine.reg_write(UC_X86_REG_EDX, 0xC0C1C2C3)
        self.machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
        self.machine.reg_write(UC_X86_REG_FPCW, self.fpcw)
        return caller

    def poison(self, eax: int) -> None:
        self.machine.reg_write(UC_X86_REG_EAX, eax & 0xFFFFFFFF)
        self.machine.reg_write(UC_X86_REG_ECX, 0xC1C2C3C4 ^ self.rng_ordinal)
        self.machine.reg_write(UC_X86_REG_EDX, 0xD1D2D3D4 ^ self.rng_ordinal)

    def return_from_stub(self, eax: int) -> None:
        esp = self.machine.reg_read(UC_X86_REG_ESP)
        return_address = machine_u32(self.machine, esp)
        self.poison(eax)
        self.machine.reg_write(UC_X86_REG_ESP, esp + 4)
        self.machine.reg_write(UC_X86_REG_EIP, return_address)

    def require(self, condition: bool, detail) -> None:
        if not condition:
            raise AssertionError((self.prepared["role"], self.case["name"], detail))

    def body_entry(self, address: int) -> None:
        if address == CODE_UNATTACHED:
            args = [machine_argument(self.machine, index) for index in range(5)]
            self.require(args == [DEFINITION_INDEX, POSITION, VELOCITY, COLOR, SCALE_BITS], ("unattached args", args))
            self.events.append(normalized_event(
                "body:particle_system_new_unattached",
                [DEFINITION_INDEX, "position", "velocity", "color", SCALE_BITS],
            ))
        elif address == CODE_INITIALIZE:
            handle = machine_argument(self.machine, 0)
            self.require(handle == SYSTEM_HANDLE, ("initializer handle", handle))
            self.events.append(normalized_event("body:particle_system_initialize", [handle]))

    def on_stub(self, address: int) -> None:
        reverse = {value: key for key, value in STUB_ADDRESSES.items()}
        name = reverse[address]
        if name == "_datum_new":
            pool = machine_argument(self.machine, 0)
            self.require(pool == PARTICLE_SYSTEMS_TOKEN, (name, "pool", pool))
            self.events.append(normalized_event("datum_new", ["particle_systems"]))
            if self.case["allocation_none"]:
                self.return_from_stub(0xFFFFFFFF)
                return
            self.machine.mem_write(SYSTEM, bytes(SYSTEM_SIZE))
            self.machine.mem_write(SYSTEM, p16(SYSTEM_IDENTIFIER))
            self.stub_mutations.append({"name": "datum_new", "record_clear_size": SYSTEM_SIZE, "identifier": SYSTEM_IDENTIFIER})
            self.return_from_stub(SYSTEM_HANDLE)
        elif name == "_datum_get":
            pool, handle = machine_argument(self.machine, 0), machine_argument(self.machine, 1)
            self.require(pool == PARTICLE_SYSTEMS_TOKEN and handle == SYSTEM_HANDLE, (name, pool, handle))
            self.require(machine_u32(self.machine, SYSTEM) & 0xFFFF == SYSTEM_IDENTIFIER, (name, "identifier"))
            self.events.append(normalized_event("datum_get", ["particle_systems", handle]))
            self.return_from_stub(SYSTEM)
        elif name == "_datum_delete":
            pool, handle = machine_argument(self.machine, 0), machine_argument(self.machine, 1)
            self.require(pool == PARTICLE_SYSTEMS_TOKEN and handle == SYSTEM_HANDLE, (name, pool, handle))
            self.events.append(normalized_event("datum_delete", ["particle_systems", handle]))
            self.machine.mem_write(SYSTEM, p16(0))
            self.deleted = True
            self.stub_mutations.append({"name": "datum_delete", "cleared": "system.identifier"})
            self.return_from_stub(0xD00D0001)
        elif name == "_tag_get":
            group, index = machine_argument(self.machine, 0), machine_argument(self.machine, 1)
            self.require((group, index) == (PCTL_TAG, DEFINITION_INDEX), (name, group, index))
            self.events.append(normalized_event("tag_get", [group, index]))
            self.return_from_stub(DEFINITION)
        elif name == "_scenario_location_from_point":
            location, point = machine_argument(self.machine, 0), machine_argument(self.machine, 1)
            self.require((location, point) == (SYSTEM + SYSTEM_LOCATION, SYSTEM + SYSTEM_POSITION), (name, location, point))
            self.require(tuple(machine_u32(self.machine, point + 4 * i) for i in range(3)) == POSITION_BITS, (name, "point contents"))
            self.events.append(normalized_event("scenario_location_from_point", ["system.location", "system.position"]))
            self.machine.mem_write(location, b"".join(p32(word) for word in LOCATION_WORDS))
            self.stub_mutations.append({"name": "scenario_location_from_point", "wrote": "system.location"})
            self.return_from_stub(0xD00D0002)
        elif name == "_tag_block_get_element_with_size":
            block, index, size = (machine_argument(self.machine, i) for i in range(3))
            if block == DEFINITION + 0x5C:
                self.require(0 <= index < len(self.case["state_counts"]), (name, "type index", index))
                self.require(size == TYPE_STRIDE, (name, "type size", size))
                self.require(machine_u32(self.machine, block + 4) == TYPE_BASE, (name, "type block address"))
                label = "definition.types"
                result = TYPE_BASE + index * TYPE_STRIDE
            else:
                matches = [i for i in range(len(self.case["state_counts"])) if block == TYPE_BASE + i * TYPE_STRIDE + 0x68]
                self.require(len(matches) == 1, (name, "block pointer", hex(block)))
                type_index = matches[0]
                self.require(index == 0 and size == STATE_SIZE and self.case["state_counts"][type_index] > 0,
                             (name, "state request", type_index, index, size))
                result = STATE_BASE + type_index * STATE_STRIDE
                self.require(machine_u32(self.machine, block + 4) == result, (name, "state block address"))
                label = f"type[{type_index}].type_states"
            self.events.append(normalized_event("tag_block_get_element_with_size", [label, index, size]))
            self.return_from_stub(result)
        elif name == "_light_particle":
            point, lighting, diffuse, block = (machine_argument(self.machine, i) for i in range(4))
            self.require(point == SYSTEM + SYSTEM_POSITION and lighting == SYSTEM + SYSTEM_LIGHTING and block == 0,
                         (name, point, lighting, block))
            self.require(STACK_PAGE <= diffuse <= ENTRY_SP - 12, (name, "diffuse local", hex(diffuse)))
            self.require(tuple(machine_u32(self.machine, point + 4 * i) for i in range(3)) == POSITION_BITS,
                         (name, "position contents"))
            self.events.append(normalized_event("light_particle", ["system.position", "system.lighting", "stack_local", 0]))
            self.machine.mem_write(lighting, b"".join(p32(word) for word in LIGHT_BITS))
            self.machine.mem_write(diffuse, b"".join(p32(word) for word in DIFFUSE_BITS))
            self.stub_mutations.append({"name": "light_particle", "wrote": ["system.lighting", "stack_local"]})
            self.return_from_stub(0xD00D0003)
        elif name == "_get_global_local_random_seed_address":
            self.events.append(normalized_event("get_global_local_random_seed_address", []))
            self.return_from_stub(SEED_ADDRESS)
        elif name == "_real_seed_random_range":
            seed_pointer, lower, upper = (machine_argument(self.machine, i) for i in range(3))
            positive = [i for i, count in enumerate(self.case["state_counts"]) if count > 0]
            self.require(self.rng_ordinal < len(positive), (name, "extra call", self.rng_ordinal))
            type_index = positive[self.rng_ordinal]
            self.require((seed_pointer, lower, upper) == (SEED_ADDRESS, LOWER_BITS[type_index], UPPER_BITS[type_index]),
                         (name, seed_pointer, lower, upper, type_index))
            result_bits = DURATION_BITS[type_index]
            self.events.append(normalized_event(
                "real_seed_random_range",
                ["local_seed", lower, upper],
                return_bits=result_bits,
            ))
            previous = machine_u32(self.machine, SEED_ADDRESS)
            updated = (previous * 1664525 + 1013904223 + self.rng_ordinal) & 0xFFFFFFFF
            self.machine.mem_write(SEED_ADDRESS, p32(updated))
            self.machine.mem_write(RNG_RESULT, p32(result_bits))
            self.stub_mutations.append({"name": "real_seed_random_range", "seed_before": previous, "seed_after": updated})
            self.rng_ordinal += 1
            self.poison(0xD00D0004)
            # The mapped stub executes FLD [RNG_RESULT]; RET so the genuine
            # x87 return channel, rather than a Python float surrogate, is used.
        elif name == "_code_0008e7f0":
            delta_time, handle = machine_argument(self.machine, 0), machine_argument(self.machine, 1)
            self.require((delta_time, handle) == (INITIAL_UPDATE_BITS, SYSTEM_HANDLE), (name, delta_time, handle))
            self.events.append(normalized_event("code_0008e7f0", [delta_time, handle]))
            self.return_from_stub(0xD00D0005)
        else:
            raise AssertionError(("unhandled stub", name))

    def run(self) -> dict:
        self.map_memory()
        caller_before = self.set_frame()
        reverse = {value: key for key, value in STUB_ADDRESSES.items()}

        def on_code(_machine, address, _size, _user):
            if address in (CODE_UNATTACHED, CODE_INITIALIZE):
                self.body_entry(address)
            elif address in reverse:
                self.on_stub(address)

        self.machine.hook_add(u.UC_HOOK_CODE, on_code)
        entry = CODE_INITIALIZE if self.case["entry"] == "initialize" else CODE_UNATTACHED
        self.machine.emu_start(entry, STOP, count=100000)

        final_pages = {
            "system": bytes(self.machine.mem_read(SYSTEM_PAGE, PAGE_SIZE)),
            "definition": bytes(self.machine.mem_read(DEFINITION_PAGE, PAGE_SIZE)),
            "types": bytes(self.machine.mem_read(TYPE_PAGE, PAGE_SIZE)),
            "states": bytes(self.machine.mem_read(STATE_PAGE, PAGE_SIZE)),
            "inputs": bytes(self.machine.mem_read(INPUT_PAGE, PAGE_SIZE)),
            "globals": bytes(self.machine.mem_read(GLOBALS, PAGE_SIZE)),
            "seed": bytes(self.machine.mem_read(SEED_PAGE, PAGE_SIZE)),
        }
        caller_after = bytes(self.machine.mem_read(ENTRY_SP, CALLER_WINDOW_SIZE))
        final_eax = self.machine.reg_read(UC_X86_REG_EAX) & 0xFFFFFFFF
        final_nonvolatile = {
            name: self.machine.reg_read(register) & 0xFFFFFFFF
            for register, (name, _) in NONVOLATILE.items()
        }
        expected_nonvolatile = {name: value for _, (name, value) in NONVOLATILE.items()}
        errors = []
        if self.events != self.expected["events"]:
            errors.append("events")
        if final_pages["system"] != self.expected["system"]:
            errors.append("system_memory")
        if final_pages["seed"] != self.expected["seed"]:
            errors.append("seed_memory")
        for name in ("definition", "types", "states", "inputs"):
            if final_pages[name] != self.initial[name]:
                errors.append(name + "_memory")
        # Ignore RNG_RESULT, a declared stub workspace, while requiring the
        # actual particle_systems global pointer and the rest of its guard page.
        global_expected = bytearray(self.initial["globals"])
        global_actual = bytearray(final_pages["globals"])
        rng_offset = page_offset(RNG_RESULT, GLOBALS)
        global_actual[rng_offset : rng_offset + 4] = global_expected[rng_offset : rng_offset + 4]
        if global_actual != global_expected:
            errors.append("global_memory")
        if self.expected["return_kind"] == "al":
            if (final_eax & 0xFF) != self.expected["return_value"]:
                errors.append("return_al")
        elif final_eax != self.expected["return_value"]:
            errors.append("return_eax")
        if self.machine.reg_read(UC_X86_REG_ESP) != ENTRY_SP + 4:
            errors.append("esp")
        if self.machine.reg_read(UC_X86_REG_EIP) != STOP:
            errors.append("eip")
        if final_nonvolatile != expected_nonvolatile:
            errors.append("nonvolatile")
        if self.machine.reg_read(UC_X86_REG_EFLAGS) & 0x400:
            errors.append("direction_flag")
        if (self.machine.reg_read(UC_X86_REG_FPCW) & 0xFFFF) != self.fpcw:
            errors.append("fpcw")
        if self.machine.reg_read(UC_X86_REG_FPSW) & 0x3800:
            errors.append("x87_top")
        if caller_after != caller_before:
            errors.append("caller_frame")
        if self.deleted != self.expected["deleted"]:
            errors.append("delete_state")

        record_offset = page_offset(SYSTEM, SYSTEM_PAGE)
        record = final_pages["system"][record_offset : record_offset + SYSTEM_SIZE]
        record_before = self.initial["system"][record_offset : record_offset + SYSTEM_SIZE]
        guard = (
            final_pages["system"][record_offset - 0x80 : record_offset]
            + final_pages["system"][record_offset + SYSTEM_SIZE : record_offset + SYSTEM_SIZE + 0x80]
        )
        return {
            "case": self.case["name"],
            "entry": self.case["entry"],
            "fpcw": self.fpcw,
            "events": self.events,
            "return_kind": self.expected["return_kind"],
            "return_value": final_eax & (0xFF if self.expected["return_kind"] == "al" else 0xFFFFFFFF),
            "expected_return_value": self.expected["return_value"],
            "deleted": self.deleted,
            "record_sha256": sha256_bytes(record),
            "system_page_sha256": sha256_bytes(final_pages["system"]),
            "system_guard_sha256": sha256_bytes(guard),
            "seed_page_sha256": sha256_bytes(final_pages["seed"]),
            "record_changed_ranges": changed_ranges(record_before, record),
            "stub_mutations": self.stub_mutations,
            "abi": {
                "esp": self.machine.reg_read(UC_X86_REG_ESP) & 0xFFFFFFFF,
                "eip": self.machine.reg_read(UC_X86_REG_EIP) & 0xFFFFFFFF,
                "nonvolatile": final_nonvolatile,
                "direction_flag": bool(self.machine.reg_read(UC_X86_REG_EFLAGS) & 0x400),
                "fpcw": self.machine.reg_read(UC_X86_REG_FPCW) & 0xFFFF,
                "x87_top": self.machine.reg_read(UC_X86_REG_FPSW) & 0x3800,
                "caller_frame_sha256": sha256_bytes(caller_after),
            },
            "errors": errors,
        }


def semantic_signature(outcome: dict) -> dict:
    return {
        "events": outcome["events"],
        "return_kind": outcome["return_kind"],
        "return_value": outcome["return_value"],
        "deleted": outcome["deleted"],
        "record_sha256": outcome["record_sha256"],
        "system_page_sha256": outcome["system_page_sha256"],
        "system_guard_sha256": outcome["system_guard_sha256"],
        "seed_page_sha256": outcome["seed_page_sha256"],
        "record_changed_ranges": outcome["record_changed_ranges"],
    }


def run_role(prepared: dict, target_signatures: dict | None = None) -> tuple[dict, dict]:
    outcomes = []
    signatures = {}
    for case_index, case in enumerate(cases()):
        for mode_index, fpcw in enumerate(FPCW_VALUES):
            seed = 0x40 + case_index * len(FPCW_VALUES) + mode_index
            outcome = Runtime(prepared, case, seed, fpcw).run()
            if outcome["errors"]:
                raise AssertionError((prepared["role"], case["name"], hex(fpcw), outcome["errors"], outcome["events"]))
            key = f"{case['name']}@{fpcw:04x}"
            signature = semantic_signature(outcome)
            if target_signatures is not None and signature != target_signatures[key]:
                raise AssertionError((prepared["role"], key, "target semantic difference", target_signatures[key], signature))
            signatures[key] = signature
            outcomes.append(outcome)
    return {
        "role": prepared["role"],
        "path": prepared["path"],
        "sha256": prepared["sha256"],
        "executions": len(outcomes),
        "all_pass": True,
        "outcomes": outcomes,
    }, signatures


def mutated_role(target: dict, function: str, offset: int, expected: int, replacement: int) -> dict:
    result = copy.deepcopy(target)
    body = bytearray(result[function]["runtime_raw"])
    if body[offset] != expected:
        raise AssertionError(("negative control byte guard", function, hex(offset), hex(body[offset]), hex(expected)))
    body[offset] = replacement
    result[function]["runtime_raw"] = bytes(body)
    return result


def run_negative_controls(target: dict) -> list[dict]:
    controls = [
        {
            "name": "initializer_wrong_flag_bit",
            "function": "initialize",
            "offset": 0x40,
            "expected": 0x02,
            "replacement": 0x04,
            "case": "initialize_types_1_positive",
            "required_errors": {"system_memory"},
        },
        {
            "name": "initializer_inverted_zero_state_branch",
            "function": "initialize",
            "offset": 0x80,
            "expected": 0x74,
            "replacement": 0x75,
            "case": "initialize_mixed_zero_continues",
            "required_errors": {"events", "system_memory"},
        },
        {
            "name": "unattached_skip_failure_delete",
            "function": "unattached",
            "offset": 0xAD,
            "expected": 0x75,
            "replacement": 0xEB,
            "case": "unattached_mixed_zero_failure_delete",
            "required_errors": {"events", "system_memory", "return_eax", "delete_state"},
        },
    ]
    indexed = {case["name"]: case for case in cases()}
    results = []
    for index, control in enumerate(controls):
        mutant = mutated_role(
            target,
            control["function"],
            control["offset"],
            control["expected"],
            control["replacement"],
        )
        outcome = Runtime(mutant, indexed[control["case"]], 0x90 + index, FPCW_VALUES[1]).run()
        observed = set(outcome["errors"])
        detected = control["required_errors"].issubset(observed)
        if not detected:
            raise AssertionError((control["name"], "negative control not precisely detected", outcome["errors"]))
        results.append({
            "name": control["name"],
            "mutation": {
                "function": control["function"],
                "offset": control["offset"],
                "before": control["expected"],
                "after": control["replacement"],
            },
            "case": control["case"],
            "required_errors": sorted(control["required_errors"]),
            "observed_errors": outcome["errors"],
            "detected": detected,
        })
    return results


def concise_role(prepared: dict) -> dict:
    return {
        "path": prepared["path"],
        "sha256": prepared["sha256"],
        "initialize": {
            "name": prepared["initialize"]["name"],
            "raw_sha256": prepared["initialize"]["raw_sha256"],
            "normalized_sha256": prepared["initialize"]["normalized_sha256"],
            "relocations": prepared["initialize"]["relocations"],
            "checks": prepared["initialize"]["checks"],
        },
        "unattached": {
            "name": prepared["unattached"]["name"],
            "raw_sha256": prepared["unattached"]["raw_sha256"],
            "normalized_sha256": prepared["unattached"]["normalized_sha256"],
            "relocations": prepared["unattached"]["relocations"],
            "checks": prepared["unattached"]["checks"],
        },
        "real_local_random_range_helper": prepared["real_local_random_range_helper"],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--before-manifest", type=Path, default=BEFORE_MANIFEST_DEFAULT)
    parser.add_argument("--before-manifest-sha", default=PINNED_HASHES["before_manifest"])
    parser.add_argument("--first", type=Path, default=FIRST_DEFAULT)
    parser.add_argument("--first-sha", default=PINNED_HASHES["first"])
    parser.add_argument("--actual", type=Path, default=ACTUAL_DEFAULT)
    parser.add_argument("--actual-sha", default=PINNED_HASHES["actual"])
    parser.add_argument("--actual-manifest", type=Path, default=ACTUAL_MANIFEST_DEFAULT)
    parser.add_argument("--actual-manifest-sha", default=PINNED_HASHES["actual_manifest"])
    parser.add_argument("--output", type=Path, default=OUTPUT_DEFAULT)
    args = parser.parse_args()

    target_path, target_hash, before_guard = manifest_target(args.before_manifest, args.before_manifest_sha)
    actual_manifest_guard = verify_actual_manifest(
        args.actual_manifest, args.actual_manifest_sha, args.actual, args.actual_sha
    )
    roles = {
        "target": prepare_role(target_path, "target", target_hash),
        "first": prepare_role(args.first, "first", args.first_sha),
        "actual": prepare_role(args.actual, "actual", args.actual_sha),
    }
    target_run, signatures = run_role(roles["target"])
    first_run, _ = run_role(roles["first"], signatures)
    actual_run, _ = run_role(roles["actual"], signatures)
    negatives = run_negative_controls(roles["target"])

    shared_trace = sha256_bytes(json.dumps(signatures, sort_keys=True, separators=(",", ":")).encode("utf-8"))
    report = {
        "verdict": "PASS",
        "scope": "real initializer and real public unattached constructor; typed external stubs",
        "input_guards": {
            "before_manifest": before_guard,
            "actual_manifest": actual_manifest_guard,
            "roles": {name: concise_role(role) for name, role in roles.items()},
        },
        "case_count": len(cases()),
        "fpcw_values": list(FPCW_VALUES),
        "executions_per_role": len(cases()) * len(FPCW_VALUES),
        "shared_semantic_trace_sha256": shared_trace,
        "roles": {
            "target": target_run,
            "first": first_run,
            "actual": actual_run,
        },
        "negative_controls": negatives,
        "findings": [
            "All roles match an independent byte-level reference and one another for complete system-page state, 0x158-byte record state, adjacent 0x80-byte guards, API order/arguments, local-seed mutations, semantic return channel, and i386 caller ABI.",
            "Initializer boolean results are checked in AL only; constructor handles are checked in all of EAX. Volatile integer registers are deliberately poisoned by stubs and are not compared at the public boundary.",
            "The readable initializer residual remains fuzzy: runtime agreement does not convert its different normalized bytes into strict credit.",
            "The candidate-only 32-byte SELECTANY real_local_random_range COMDAT is reported as an uncredited ownership delta and is not executed; its inlined genuine get-seed/range calls are the ones exercised inside the real initializer body.",
        ],
        "limits": [
            "datum_new/get/delete are deterministic typed stubs. datum_new models the genuine observed record clear plus identifier assignment; datum_delete models persistent storage with identifier clearing. Data-array search and bookkeeping do not execute.",
            "tag_get and tag-block access validate genuine pointers, indices, and 0x80/0xC0 element sizes against guard-separated modeled storage; the engine tag cache does not execute.",
            "scenario location and light_particle write deterministic outputs, while real_seed_random_range returns configured finite float bits through a genuine x87 FLD return stub and deterministically mutates the modeled local seed. Their engine algorithms do not execute.",
            "code_0008e7f0 is an audited call-recording stub. The oracle proves the 0.001f/system-handle call contract, not the still-unwritten update routine or its effects.",
            "Counts are restricted to the valid 0..4 runtime capacity in this corpus. This is a bounded branch corpus, not exhaustive fuzzing or whole-engine execution.",
        ],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        "verdict": report["verdict"],
        "output": str(args.output),
        "executions_per_role": report["executions_per_role"],
        "shared_semantic_trace_sha256": shared_trace,
        "negative_controls": [item["name"] for item in negatives],
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
