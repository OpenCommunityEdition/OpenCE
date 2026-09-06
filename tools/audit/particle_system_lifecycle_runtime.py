"""Scratch-only i386 runtime oracle for the particle-system lifecycle pair.

The oracle executes the real ``particle_system_delete`` and
``particle_systems_reconnect_to_structure_bsp`` COFF sections together.  The
January target is pinned through its frozen before manifest.  Optional first
and actual candidates are accepted only through caller-supplied, SHA-pinned
frozen manifests.

The data/tag/location APIs are deterministic typed stubs.  ``datum_delete``
models the pinned genuine implementation: it clears only the datum identifier,
updates data-array bookkeeping, and deliberately leaves payload bytes mapped
and unchanged.  That makes reconnect's authenticated post-delete read of the
particle next handle an actual machine-code observation, not a rewritten body.

Only the requested scratch JSON is written.  This script does not compile,
invoke Ninja, or modify production source, configuration, or build output.
"""

from __future__ import annotations

import argparse
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
)


SCRIPT = Path(__file__).resolve()
ROOT = next(parent for parent in SCRIPT.parents if (parent / "tools/coff_compare.py").is_file())
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import function_sections  # noqa: E402
from tools.regression_gate import _section_aux_selection  # noqa: E402


UNIT = "source/effects/particle_systems"
BEFORE_MANIFEST = ROOT / "scratch/particle-systems-fable-pair-before-20260906.json"
OUTPUT_DEFAULT = ROOT / "scratch/particle-system-lifecycle-pair-runtime-differential-report.json"
DATA_SOURCE = ROOT / "source/memory/data.c"
DATA_HEADER = ROOT / "source/memory/data.h"
DATA_OBJECT = ROOT / "build/split/source/memory/data.obj"

PINNED = {
    "before_manifest": "8bf3891c9b86f0348f62e1d786cdb9b589294c8bda0caae1d2bc3338f3a602d4",
    "target_object": "a6c6b08ada7fe558c34455c623d8cf5249798f7e023b3cb3ddd4167dc3ac20ba",
    "data_source": "c71bce78bf45f52a779841e75f69891b5319abd5a7a73af9cfa7736991b7ad2b",
    "data_header": "14cf20daa6d3bb033ed8b69f6ba8714486f2d9bf20e044f2e8885202aa93c192",
    "data_object": "ae62cbda45efcd892efaa29b1ef7e2a1d4c8b33b6dc3cf568f7d42416efce730",
}

DELETE_NAMES = ("_particle_system_delete", "_code_0008dd80")
RECONNECT = "_particle_systems_reconnect_to_structure_bsp"
FUNCTION_FACTS = {
    "delete": {
        "size": 176,
        "relocations": 9,
        "normalized_sha256": "f2c3884188b26d73b9d5a9d81de37b82c608b7db3db3038c2ef295266580b8b4",
        "addresses": [11, 19, 35, 84, 91, 97, 107, 147, 154],
        "targets": [
            "global:particle_systems",
            "api:datum_get",
            "api:tag_get",
            "global:system_particles",
            "api:datum_get",
            "global:system_particles",
            "api:datum_delete",
            "global:particle_systems",
            "api:datum_delete",
        ],
    },
    "reconnect": {
        "size": 320,
        "relocations": 15,
        "normalized_sha256": "c630b0dfe3bfedc35c02a46b52dee34411bf25ac8c7c5f6f7a5aedceef400175",
        "addresses": [7, 16, 50, 57, 73, 99, 148, 155, 170, 187, 194, 217, 233, 279, 286],
        "targets": [
            "global:particle_systems",
            "api:data_next_index",
            "global:particle_systems",
            "api:datum_get",
            "api:tag_get",
            "api:object_get_location",
            "global:system_particles",
            "api:datum_get",
            "api:scenario_location_from_point",
            "global:system_particles",
            "api:datum_delete",
            "api:scenario_location_from_point",
            "private:particle_system_delete",
            "global:particle_systems",
            "api:data_next_index",
        ],
    },
}

CODE_DELETE = 0x100000
CODE_RECONNECT = 0x110000
STOP = 0x120000
STUBS = 0x200000
GLOBALS = 0x300000
PARTICLE_SYSTEMS_SLOT = GLOBALS + 0x100
SYSTEM_PARTICLES_SLOT = GLOBALS + 0x104
PARTICLE_SYSTEMS_POOL = GLOBALS + 0x200
SYSTEM_PARTICLES_POOL = GLOBALS + 0x300
SYSTEM_BASE = 0x400000
PARTICLE_BASE = 0x500000
DEFINITION_BASE = 0x600000
ARENA_SIZE = 0x10000
STACK = 0x700000
STACK_SIZE = 0x10000
ENTRY_SP = STACK + 0x8000
CALLER_WINDOW = 0x80

SYSTEM_SIZE = 0x158
SYSTEM_STRIDE = 0x200
PARTICLE_SIZE = 0x80
PARTICLE_STRIDE = 0x100
DEFINITION_SIZE = 0x68
DEFINITION_STRIDE = 0x100
MAXIMUM_SLOTS = 32

SYSTEM_DEFINITION_OFFSET = 0x08
SYSTEM_OBJECT_OFFSET = 0x0C
SYSTEM_LOCATION_OFFSET = 0x18
SYSTEM_POSITION_OFFSET = 0x20
SYSTEM_TYPES_OFFSET = 0x58
PARTICLE_TYPE_SIZE = 0x40
TYPE_FIRST_PARTICLE_OFFSET = 0x3C
SYSTEM_TYPE_HEAD_OFFSET = SYSTEM_TYPES_OFFSET + TYPE_FIRST_PARTICLE_OFFSET
DEFINITION_TYPES_OFFSET = 0x5C
PARTICLE_NEXT_OFFSET = 0x04
PARTICLE_LOCATION_OFFSET = 0x14
PARTICLE_POSITION_OFFSET = 0x1C
LOCATION_CLUSTER_OFFSET = 0x04
LOCATION_BONUS_OFFSET = 0x06

NONE = 0xFFFFFFFF
PARTICLE_SYSTEM_TAG = 0x7063746C

API_NAMES = (
    "_data_next_index",
    "_datum_get",
    "_tag_get",
    "_object_get_location",
    "_scenario_location_from_point",
    "_datum_delete",
)
STUB_ADDRESSES = {
    name: STUBS + 0x100 * (index + 1) for index, name in enumerate(API_NAMES)
}
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


def p16(value: int) -> bytes:
    return struct.pack("<H", value & 0xFFFF)


def p32(value: int) -> bytes:
    return struct.pack("<I", value & 0xFFFFFFFF)


def u16(data: bytes) -> int:
    return struct.unpack("<H", data)[0]


def u32(data: bytes) -> int:
    return struct.unpack("<I", data)[0]


def s16(value: int) -> int:
    return struct.unpack("<h", p16(value))[0]


def seeded_bytes(size: int, seed: int) -> bytes:
    return bytes(((index * 37 + seed * 61 + 17) & 0xFF) for index in range(size))


def system_handle(slot: int) -> int:
    return ((0x2100 + slot * 7) << 16) | slot


def particle_handle(slot: int) -> int:
    return ((0x3100 + slot * 11) << 16) | slot


def write16(data: bytearray, offset: int, value: int) -> None:
    data[offset : offset + 2] = p16(value)


def write32(data: bytearray, offset: int, value: int) -> None:
    data[offset : offset + 4] = p32(value)


def changed_ranges(before: bytes, after: bytes) -> list[list[int]]:
    ranges = []
    start = None
    for index, (left, right) in enumerate(zip(before, after)):
        if left != right and start is None:
            start = index
        elif left == right and start is not None:
            ranges.append([start, index])
            start = None
    if start is not None:
        ranges.append([start, len(before)])
    return ranges


def resolve_manifest_path(manifest_path: Path, value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else (manifest_path.parent / path).resolve()


def load_manifest_object(path: Path, expected_hash: str, role: str) -> tuple[Path, dict]:
    evidence = verify_hash(path, expected_hash, role + "_manifest")
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if role == "target":
        frozen_name = "build/split/source/effects/particle_systems.obj"
        if frozen_name not in manifest.get("frozen_files", {}):
            raise AssertionError((role, "manifest missing frozen target", frozen_name))
        record = manifest["frozen_files"][frozen_name]
    else:
        if UNIT not in manifest.get("objects", {}):
            raise AssertionError((role, "manifest missing object", UNIT))
        record = manifest["objects"][UNIT]
    object_path = resolve_manifest_path(path, record["path"])
    object_evidence = verify_hash(object_path, record["sha256"], role + "_object")
    if role == "target" and object_evidence["sha256"] != PINNED["target_object"]:
        raise AssertionError((role, "unexpected target object", object_evidence))
    return object_path, {
        "manifest": evidence,
        "phase": manifest.get("phase"),
        "object": object_evidence,
    }


def canonical_relocation_name(name: str) -> str:
    if name == "_particle_systems":
        return "global:particle_systems"
    if name == "_system_particles":
        return "global:system_particles"
    if name in DELETE_NAMES:
        return "private:particle_system_delete"
    if name in API_NAMES:
        return "api:" + name[1:]
    raise AssertionError(("unexpected relocation owner", name))


def relocation_destination(name: str) -> int:
    if name == "_particle_systems":
        return PARTICLE_SYSTEMS_SLOT
    if name == "_system_particles":
        return SYSTEM_PARTICLES_SLOT
    if name in DELETE_NAMES:
        return CODE_DELETE
    if name in STUB_ADDRESSES:
        return STUB_ADDRESSES[name]
    raise AssertionError(("unexpected relocation destination", name))


def find_function_name(obj: dict, kind: str) -> str:
    functions = function_sections(obj)
    choices = DELETE_NAMES if kind == "delete" else (RECONNECT,)
    found = [name for name in choices if name in functions]
    if len(found) != 1:
        raise AssertionError((kind, "function identity", found))
    return found[0]


def prepare_function(path: Path, role: str, kind: str) -> dict:
    obj = cc.load(path)
    name = find_function_name(obj, kind)
    function = function_sections(obj)[name]
    symbol = cc.symbol(obj, name)
    section = obj["sections"][function.section_number - 1]
    raw = bytearray(cc._section_bytes(obj, section))
    relocations = []
    base = CODE_DELETE if kind == "delete" else CODE_RECONNECT
    for relocation in function.info["relocations"]:
        target = relocation.get("symbolic_target", relocation["target"])
        if not isinstance(target, (list, tuple)) or target[0] != "symbol":
            raise AssertionError((role, kind, "unresolved relocation", target))
        target_name, addend = target[1:]
        destination = relocation_destination(target_name) + addend
        if relocation["type"] == cc.IMAGE_REL_I386_REL32:
            value = destination - (base + relocation["address"] + 4)
        elif relocation["type"] == cc.IMAGE_REL_I386_DIR32:
            value = destination
        else:
            raise AssertionError((role, kind, "relocation type", relocation))
        raw[relocation["address"] : relocation["address"] + 4] = p32(value)
        relocations.append(
            [
                relocation["address"],
                relocation["type"],
                canonical_relocation_name(target_name),
                addend,
            ]
        )

    facts = FUNCTION_FACTS[kind]
    expected_storage = 2 if role == "target" or kind == "reconnect" else 3
    expected_types = [
        cc.IMAGE_REL_I386_DIR32 if target.startswith("global:")
        else cc.IMAGE_REL_I386_REL32
        for target in facts["targets"]
    ]
    checks = {
        "size": function.info["size"] == facts["size"],
        "relocation_count": function.info["relocation_count"] == facts["relocations"],
        "normalized_sha256": function.info["normalized_sha256"] == facts["normalized_sha256"],
        "relocation_addresses": [item[0] for item in relocations] == facts["addresses"],
        "relocation_targets": [item[2] for item in relocations] == facts["targets"],
        "relocation_types_exact": [item[1] for item in relocations] == expected_types,
        "relocation_addends_zero": all(item[3] == 0 for item in relocations),
        "code_section_flags": section["flags"] == 1615859744,
        "external_function_type": symbol["type"] == 0x20,
        "storage": symbol["storage"] == expected_storage,
        "comdat_noduplicates": _section_aux_selection(obj, symbol["section"]) == 1,
    }
    failed = [key for key, value in checks.items() if not value]
    if failed:
        raise AssertionError((role, kind, "COFF guard", failed, relocations))
    return {
        "role": role,
        "kind": kind,
        "name": name,
        "path": path,
        "raw": bytes(cc._section_bytes(obj, section)),
        "runtime_raw": bytes(raw),
        "raw_sha256": sha256_bytes(bytes(cc._section_bytes(obj, section))),
        "runtime_sha256": sha256_bytes(bytes(raw)),
        "checks": checks,
        "relocations": relocations,
        "storage": symbol["storage"],
        "selection": _section_aux_selection(obj, symbol["section"]),
    }


def prepare_role(path: Path, role: str) -> dict:
    return {
        "role": role,
        "path": path,
        "sha256": sha256_path(path),
        "delete": prepare_function(path, role, "delete"),
        "reconnect": prepare_function(path, role, "reconnect"),
    }


def particle(slot: int, valid: bool = True) -> dict:
    return {"slot": slot, "location_valid": valid}


def system(
    slot: int,
    types: list[list[dict]],
    *,
    attached: bool = True,
    system_location_valid: bool = True,
) -> dict:
    return {
        "slot": slot,
        "types": types,
        "attached": attached,
        "system_location_valid": system_location_valid,
    }


def scenario(name: str, entry: str, systems: list[dict], entry_slot: int | None = None) -> dict:
    return {"name": name, "entry": entry, "systems": systems, "entry_slot": entry_slot}


def scenarios() -> list[dict]:
    return [
        scenario("delete_types_0", "delete", [system(0, [])], 0),
        scenario("delete_types_1_chain", "delete", [system(0, [[particle(0), particle(1), particle(2)]])], 0),
        scenario("reconnect_empty", "reconnect", []),
        scenario(
            "reconnect_delete_then_continue_types_3_and_2",
            "reconnect",
            [
                system(
                    0,
                    [[particle(0), particle(1)], [], [particle(2)]],
                    attached=False,
                    system_location_valid=False,
                ),
                system(2, [[particle(3)], [particle(4)]], attached=True),
            ],
        ),
        scenario(
            "reconnect_types_4_all_splice_shapes",
            "reconnect",
            [
                system(
                    0,
                    [
                        [particle(0, False), particle(1)],
                        [particle(2), particle(3, False), particle(4)],
                        [particle(5), particle(6, False)],
                        [particle(7, False), particle(8, False)],
                    ],
                    attached=False,
                    system_location_valid=True,
                )
            ],
        ),
    ]


class World:
    def __init__(self, test: dict, seed: int):
        self.test = test
        self.seed = seed
        self.machine: u.Uc | None = None
        self.system_arena = bytearray(seeded_bytes(ARENA_SIZE, seed + 1))
        self.particle_arena = bytearray(seeded_bytes(ARENA_SIZE, seed + 2))
        self.definition_arena = bytearray(seeded_bytes(ARENA_SIZE, seed + 3))
        self.events: list[dict] = []
        self.body_writes: list[list] = []
        self.private_delete_entries = 0
        self.definition_addresses: dict[int, int] = {}
        self.point_results: dict[int, tuple[int, int]] = {}
        self.object_locations: dict[int, bytes] = {}
        self.pool = {
            "systems": {
                "token": PARTICLE_SYSTEMS_POOL,
                "base": SYSTEM_BASE,
                "stride": SYSTEM_STRIDE,
                "size": SYSTEM_SIZE,
                "maximum_count": MAXIMUM_SLOTS,
            },
            "particles": {
                "token": SYSTEM_PARTICLES_POOL,
                "base": PARTICLE_BASE,
                "stride": PARTICLE_STRIDE,
                "size": PARTICLE_SIZE,
                "maximum_count": MAXIMUM_SLOTS,
            },
        }
        for kind in self.pool:
            for slot in range(MAXIMUM_SLOTS):
                self.write_u16(self.slot_address(kind, slot), 0)

        used_particle_slots: set[int] = set()
        used_system_slots: set[int] = set()
        for ordinal, spec in enumerate(test["systems"]):
            slot = spec["slot"]
            if slot in used_system_slots:
                raise AssertionError((test["name"], "duplicate system slot", slot))
            if not 0 <= len(spec["types"]) <= 4:
                raise AssertionError((test["name"], "invalid type count", len(spec["types"])))
            used_system_slots.add(slot)
            system_address = self.slot_address("systems", slot)
            identifier = 0x2100 + slot * 7
            self.write_u16(system_address, identifier)
            definition_handle = (0x7100 + slot) << 16 | slot
            definition_address = DEFINITION_BASE + slot * DEFINITION_STRIDE
            self.definition_addresses[definition_handle] = definition_address
            self.write_u32(system_address + SYSTEM_DEFINITION_OFFSET, definition_handle)
            object_index = (0x6100 + slot) << 16 | slot if spec["attached"] else NONE
            self.write_u32(system_address + SYSTEM_OBJECT_OFFSET, object_index)
            self.write_u16(system_address + SYSTEM_LOCATION_OFFSET + LOCATION_BONUS_OFFSET, 0x4400 + slot)
            self.point_results[system_address + SYSTEM_POSITION_OFFSET] = (
                0x1000 + ordinal,
                10 + ordinal if spec["system_location_valid"] else -1,
            )
            if spec["attached"]:
                self.object_locations[object_index] = (
                    p32(0x2000 + ordinal) + p16(20 + ordinal) + p16(0x6600 + ordinal)
                )
            self.write_u32(definition_address + DEFINITION_TYPES_OFFSET, len(spec["types"]))
            for type_index, chain in enumerate(spec["types"]):
                handles = []
                for particle_spec in chain:
                    particle_slot = particle_spec["slot"]
                    if particle_slot in used_particle_slots:
                        raise AssertionError((test["name"], "duplicate particle slot", particle_slot))
                    used_particle_slots.add(particle_slot)
                    particle_address = self.slot_address("particles", particle_slot)
                    particle_identifier = 0x3100 + particle_slot * 11
                    self.write_u16(particle_address, particle_identifier)
                    # Authenticated bodies never consume +8.  Pinning it to
                    # NONE makes the wrong-next-field byte mutant deterministic.
                    self.write_u32(particle_address + 0x08, NONE)
                    self.write_u16(
                        particle_address + PARTICLE_LOCATION_OFFSET + LOCATION_BONUS_OFFSET,
                        0x5500 + particle_slot,
                    )
                    self.point_results[particle_address + PARTICLE_POSITION_OFFSET] = (
                        0x3000 + particle_slot,
                        30 + particle_slot if particle_spec["location_valid"] else -1,
                    )
                    handles.append(particle_handle(particle_slot))
                for index, handle in enumerate(handles):
                    particle_address = self.datum_address("particles", handle)
                    next_handle = handles[index + 1] if index + 1 < len(handles) else NONE
                    self.write_u32(particle_address + PARTICLE_NEXT_OFFSET, next_handle)
                head = handles[0] if handles else NONE
                self.write_u32(
                    system_address + SYSTEM_TYPE_HEAD_OFFSET + type_index * PARTICLE_TYPE_SIZE,
                    head,
                )

        for kind in self.pool:
            active = [
                slot
                for slot in range(MAXIMUM_SLOTS)
                if self.read_u16(self.slot_address(kind, slot)) != 0
            ]
            self.pool[kind]["count"] = max(active) + 1 if active else 0
            self.pool[kind]["actual_count"] = len(active)
            self.pool[kind]["first_free_absolute_index"] = next(
                (
                    slot
                    for slot in range(MAXIMUM_SLOTS)
                    if self.read_u16(self.slot_address(kind, slot)) == 0
                ),
                MAXIMUM_SLOTS,
            )
        self.initial = self.snapshot()

    def arena(self, address: int, size: int = 1) -> tuple[int, bytearray]:
        for base, data in (
            (SYSTEM_BASE, self.system_arena),
            (PARTICLE_BASE, self.particle_arena),
            (DEFINITION_BASE, self.definition_arena),
        ):
            if base <= address and address + size <= base + len(data):
                return address - base, data
        raise AssertionError(("address outside modeled arenas", hex(address), size))

    def read(self, address: int, size: int) -> bytes:
        if self.machine is not None:
            return bytes(self.machine.mem_read(address, size))
        offset, arena = self.arena(address, size)
        return bytes(arena[offset : offset + size])

    def write(self, address: int, value: bytes) -> None:
        if self.machine is not None:
            self.machine.mem_write(address, value)
            return
        offset, arena = self.arena(address, len(value))
        arena[offset : offset + len(value)] = value

    def read_u16(self, address: int) -> int:
        return u16(self.read(address, 2))

    def read_u32(self, address: int) -> int:
        return u32(self.read(address, 4))

    def write_u16(self, address: int, value: int) -> None:
        self.write(address, p16(value))

    def write_u32(self, address: int, value: int) -> None:
        self.write(address, p32(value))

    def attach(self, machine: u.Uc) -> None:
        self.machine = machine
        machine.mem_write(SYSTEM_BASE, bytes(self.system_arena))
        machine.mem_write(PARTICLE_BASE, bytes(self.particle_arena))
        machine.mem_write(DEFINITION_BASE, bytes(self.definition_arena))

    def slot_address(self, kind: str, slot: int) -> int:
        record = self.pool[kind]
        return record["base"] + slot * record["stride"]

    def pool_kind(self, token: int) -> str:
        matches = [kind for kind, value in self.pool.items() if value["token"] == token]
        if len(matches) != 1:
            raise AssertionError(f"unknown pool token 0x{token:08x}")
        return matches[0]

    def datum_address(self, kind: str, handle: int) -> int:
        slot = s16(handle)
        if not 0 <= slot < self.pool[kind]["maximum_count"]:
            raise AssertionError(f"{kind} invalid absolute index in handle 0x{handle:08x}")
        address = self.slot_address(kind, slot)
        identifier = self.read_u16(address)
        expected = (handle >> 16) & 0xFFFF
        if identifier == 0 or identifier != expected:
            raise AssertionError(
                f"{kind} invalid handle 0x{handle:08x}: identifier 0x{identifier:04x}"
            )
        return address

    def datum_get(self, token: int, handle: int) -> int:
        kind = self.pool_kind(token)
        address = self.datum_address(kind, handle)
        self.events.append({"api": "datum_get", "args": [token, handle], "result": address})
        return address

    def tag_get(self, tag: int, handle: int) -> int:
        if tag != PARTICLE_SYSTEM_TAG or handle not in self.definition_addresses:
            raise AssertionError(f"tag_get invalid args 0x{tag:08x}, 0x{handle:08x}")
        address = self.definition_addresses[handle]
        self.events.append({"api": "tag_get", "args": [tag, handle], "result": address})
        return address

    def data_next_index(self, token: int, previous: int) -> int:
        kind = self.pool_kind(token)
        if kind != "systems":
            raise AssertionError("data_next_index used non-system pool")
        absolute_index = s16(previous) + 1
        result = NONE
        record = self.pool[kind]
        if 0 <= absolute_index < record["count"]:
            for slot in range(absolute_index, record["count"]):
                identifier = self.read_u16(self.slot_address(kind, slot))
                if identifier:
                    result = (identifier << 16) | slot
                    break
        self.events.append(
            {"api": "data_next_index", "args": [token, previous], "result": result}
        )
        return result

    def datum_delete(self, token: int, handle: int) -> None:
        kind = self.pool_kind(token)
        address = self.datum_address(kind, handle)
        before = self.read(address, self.pool[kind]["size"])
        next_before = self.read_u32(address + PARTICLE_NEXT_OFFSET) if kind == "particles" else None
        self.write_u16(address, 0)
        record = self.pool[kind]
        slot = s16(handle)
        if slot < record["first_free_absolute_index"]:
            record["first_free_absolute_index"] = slot
        if slot + 1 == record["count"]:
            record["count"] -= 1
            while (
                record["count"] > 0
                and self.read_u16(self.slot_address(kind, record["count"] - 1)) == 0
            ):
                record["count"] -= 1
        record["actual_count"] -= 1
        after = self.read(address, self.pool[kind]["size"])
        payload_preserved = before[2:] == after[2:]
        self.events.append(
            {
                "api": "datum_delete",
                "args": [token, handle],
                "kind": kind,
                "slot": slot,
                "next_before": next_before,
                "payload_after_identifier_preserved": payload_preserved,
            }
        )

    def object_get_location(self, object_handle: int, location: int) -> int:
        if object_handle not in self.object_locations:
            raise AssertionError(f"unknown object handle 0x{object_handle:08x}")
        value = self.object_locations[object_handle]
        self.write(location, value)
        self.events.append(
            {
                "api": "object_get_location",
                "args": [object_handle, location],
                "location": value.hex(),
                "result": location,
            }
        )
        return location

    def scenario_location_from_point(self, location: int, point: int) -> None:
        if point not in self.point_results:
            raise AssertionError(f"unknown scenario point pointer 0x{point:08x}")
        leaf, cluster = self.point_results[point]
        bonus_before = self.read_u16(location + LOCATION_BONUS_OFFSET)
        self.write_u32(location, leaf)
        self.write_u16(location + LOCATION_CLUSTER_OFFSET, cluster)
        self.events.append(
            {
                "api": "scenario_location_from_point",
                "args": [location, point],
                "leaf": leaf & 0xFFFFFFFF,
                "cluster": cluster,
                "bonus_preserved": self.read_u16(location + LOCATION_BONUS_OFFSET) == bonus_before,
            }
        )

    def body_write_u32(self, address: int, value: int) -> None:
        self.write_u32(address, value)
        self.body_writes.append([address, 4, value & 0xFFFFFFFF])

    def pool_metadata(self) -> dict:
        return {
            kind: {
                key: value[key]
                for key in ("count", "actual_count", "first_free_absolute_index")
            }
            for kind, value in self.pool.items()
        }

    def snapshot(self) -> dict[str, bytes]:
        if self.machine is None:
            return {
                "systems": bytes(self.system_arena),
                "particles": bytes(self.particle_arena),
                "definitions": bytes(self.definition_arena),
            }
        return {
            "systems": bytes(self.machine.mem_read(SYSTEM_BASE, ARENA_SIZE)),
            "particles": bytes(self.machine.mem_read(PARTICLE_BASE, ARENA_SIZE)),
            "definitions": bytes(self.machine.mem_read(DEFINITION_BASE, ARENA_SIZE)),
        }

    def topology(self) -> dict[str, list[list[int]]]:
        result = {}
        system_record = self.pool["systems"]
        for slot in range(system_record["count"]):
            identifier = self.read_u16(self.slot_address("systems", slot))
            if not identifier:
                continue
            handle = (identifier << 16) | slot
            address = self.datum_address("systems", handle)
            definition_handle = self.read_u32(address + SYSTEM_DEFINITION_OFFSET)
            definition_address = self.definition_addresses[definition_handle]
            type_count = self.read_u32(definition_address + DEFINITION_TYPES_OFFSET)
            chains = []
            for type_index in range(type_count):
                particle_index = self.read_u32(
                    address + SYSTEM_TYPE_HEAD_OFFSET + type_index * PARTICLE_TYPE_SIZE
                )
                chain = []
                seen = set()
                while particle_index != NONE:
                    if particle_index in seen or len(chain) >= MAXIMUM_SLOTS:
                        raise AssertionError(f"cycle in final particle topology for system 0x{handle:08x}")
                    seen.add(particle_index)
                    particle_address = self.datum_address("particles", particle_index)
                    chain.append(particle_index)
                    particle_index = self.read_u32(particle_address + PARTICLE_NEXT_OFFSET)
                chains.append(chain)
            result[f"0x{handle:08x}"] = chains
        return result


def reference_delete(world: World, system_handle: int) -> None:
    world.private_delete_entries += 1
    system_address = world.datum_get(PARTICLE_SYSTEMS_POOL, system_handle)
    definition_handle = world.read_u32(system_address + SYSTEM_DEFINITION_OFFSET)
    definition_address = world.tag_get(PARTICLE_SYSTEM_TAG, definition_handle)
    type_count = world.read_u32(definition_address + DEFINITION_TYPES_OFFSET)
    for type_index in range(type_count):
        particle_handle = world.read_u32(
            system_address + SYSTEM_TYPE_HEAD_OFFSET + type_index * PARTICLE_TYPE_SIZE
        )
        while particle_handle != NONE:
            particle_address = world.datum_get(SYSTEM_PARTICLES_POOL, particle_handle)
            next_handle = world.read_u32(particle_address + PARTICLE_NEXT_OFFSET)
            world.datum_delete(SYSTEM_PARTICLES_POOL, particle_handle)
            particle_handle = next_handle
    world.datum_delete(PARTICLE_SYSTEMS_POOL, system_handle)


def reference_reconnect(world: World) -> None:
    system_handle = world.data_next_index(PARTICLE_SYSTEMS_POOL, NONE)
    while system_handle != NONE:
        system_address = world.datum_get(PARTICLE_SYSTEMS_POOL, system_handle)
        definition_handle = world.read_u32(system_address + SYSTEM_DEFINITION_OFFSET)
        definition_address = world.tag_get(PARTICLE_SYSTEM_TAG, definition_handle)
        object_handle = world.read_u32(system_address + SYSTEM_OBJECT_OFFSET)
        if object_handle != NONE:
            world.object_get_location(object_handle, system_address + SYSTEM_LOCATION_OFFSET)
        else:
            world.scenario_location_from_point(
                system_address + SYSTEM_LOCATION_OFFSET,
                system_address + SYSTEM_POSITION_OFFSET,
            )
            if s16(world.read_u16(system_address + SYSTEM_LOCATION_OFFSET + LOCATION_CLUSTER_OFFSET)) == -1:
                reference_delete(world, system_handle)
                system_handle = world.data_next_index(PARTICLE_SYSTEMS_POOL, system_handle)
                continue

        type_count = world.read_u32(definition_address + DEFINITION_TYPES_OFFSET)
        for type_index in range(type_count):
            reference_address = (
                system_address + SYSTEM_TYPE_HEAD_OFFSET + type_index * PARTICLE_TYPE_SIZE
            )
            particle_handle = world.read_u32(reference_address)
            while particle_handle != NONE:
                particle_address = world.datum_get(SYSTEM_PARTICLES_POOL, particle_handle)
                world.scenario_location_from_point(
                    particle_address + PARTICLE_LOCATION_OFFSET,
                    particle_address + PARTICLE_POSITION_OFFSET,
                )
                cluster = s16(
                    world.read_u16(
                        particle_address + PARTICLE_LOCATION_OFFSET + LOCATION_CLUSTER_OFFSET
                    )
                )
                if cluster == -1:
                    world.datum_delete(SYSTEM_PARTICLES_POOL, particle_handle)
                    # Genuine datum_delete leaves +4 intact.  Read it only after
                    # the logical deletion, matching the January reconnect body.
                    next_handle = world.read_u32(particle_address + PARTICLE_NEXT_OFFSET)
                    world.body_write_u32(reference_address, next_handle)
                    particle_handle = next_handle
                else:
                    reference_address = particle_address + PARTICLE_NEXT_OFFSET
                    particle_handle = world.read_u32(reference_address)
        system_handle = world.data_next_index(PARTICLE_SYSTEMS_POOL, system_handle)


def make_reference(test: dict, seed: int) -> World:
    world = World(test, seed)
    if test["entry"] == "delete":
        spec = next(item for item in test["systems"] if item["slot"] == test["entry_slot"])
        identifier = world.read_u16(world.slot_address("systems", spec["slot"]))
        reference_delete(world, (identifier << 16) | spec["slot"])
    else:
        reference_reconnect(world)
    return world


def machine_argument(machine: u.Uc, index: int) -> int:
    esp = machine.reg_read(UC_X86_REG_ESP)
    return u32(bytes(machine.mem_read(esp + 4 + index * 4, 4)))


def first_event_difference(expected: list[dict], actual: list[dict]) -> dict:
    for index in range(max(len(expected), len(actual))):
        left = expected[index] if index < len(expected) else None
        right = actual[index] if index < len(actual) else None
        if left != right:
            return {"index": index, "expected": left, "actual": right}
    return {}


def execute_case(prepared: dict, test: dict, seed: int) -> dict:
    reference = make_reference(test, seed)
    expected_snapshot = reference.snapshot()
    runtime = World(test, seed)
    machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    for address, size in (
        (CODE_DELETE, 0x1000),
        (CODE_RECONNECT, 0x1000),
        (STOP, 0x1000),
        (STUBS, 0x1000),
        (GLOBALS, 0x1000),
        (SYSTEM_BASE, ARENA_SIZE),
        (PARTICLE_BASE, ARENA_SIZE),
        (DEFINITION_BASE, ARENA_SIZE),
        (STACK, STACK_SIZE),
    ):
        machine.mem_map(address, size)
    machine.mem_write(CODE_DELETE, prepared["delete"]["runtime_raw"])
    machine.mem_write(CODE_RECONNECT, prepared["reconnect"]["runtime_raw"])
    machine.mem_write(STUBS, b"\xC3" * 0x1000)
    machine.mem_write(PARTICLE_SYSTEMS_SLOT, p32(PARTICLE_SYSTEMS_POOL))
    machine.mem_write(SYSTEM_PARTICLES_SLOT, p32(SYSTEM_PARTICLES_POOL))
    runtime.attach(machine)

    stack = bytearray(seeded_bytes(STACK_SIZE, seed + 19))
    entry_offset = ENTRY_SP - STACK
    frame = [STOP]
    if test["entry"] == "delete":
        spec = next(item for item in test["systems"] if item["slot"] == test["entry_slot"])
        identifier = runtime.read_u16(runtime.slot_address("systems", spec["slot"]))
        frame.append((identifier << 16) | spec["slot"])
    for index, value in enumerate(frame):
        write32(stack, entry_offset + index * 4, value)
    expected_caller_window = bytes(stack[entry_offset : entry_offset + CALLER_WINDOW])
    machine.mem_write(STACK, bytes(stack))

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

    hook_errors: list[str] = []
    body_writes: list[list] = []

    def return_from_stub(value: int) -> None:
        esp = machine.reg_read(UC_X86_REG_ESP)
        return_address = u32(bytes(machine.mem_read(esp, 4)))
        machine.reg_write(UC_X86_REG_EAX, value & 0xFFFFFFFF)
        machine.reg_write(UC_X86_REG_ECX, 0xC5C60000 | (len(runtime.events) & 0xFFFF))
        machine.reg_write(UC_X86_REG_EDX, 0xD5D60000 | (len(runtime.events) & 0xFFFF))
        machine.reg_write(UC_X86_REG_ESP, esp + 4)
        machine.reg_write(UC_X86_REG_EIP, return_address)

    def on_code(_machine, address, _size, _user):
        if address == CODE_DELETE:
            runtime.private_delete_entries += 1
            return
        names = [name for name, value in STUB_ADDRESSES.items() if value == address]
        if not names:
            return
        name = names[0]
        try:
            if name == "_data_next_index":
                result = runtime.data_next_index(
                    machine_argument(machine, 0), machine_argument(machine, 1)
                )
                return_from_stub(result)
            elif name == "_datum_get":
                result = runtime.datum_get(
                    machine_argument(machine, 0), machine_argument(machine, 1)
                )
                return_from_stub(result)
            elif name == "_tag_get":
                result = runtime.tag_get(
                    machine_argument(machine, 0), machine_argument(machine, 1)
                )
                return_from_stub(result)
            elif name == "_object_get_location":
                result = runtime.object_get_location(
                    machine_argument(machine, 0), machine_argument(machine, 1)
                )
                return_from_stub(result)
            elif name == "_scenario_location_from_point":
                runtime.scenario_location_from_point(
                    machine_argument(machine, 0), machine_argument(machine, 1)
                )
                return_from_stub(0xA5A50000 | len(runtime.events))
            elif name == "_datum_delete":
                runtime.datum_delete(
                    machine_argument(machine, 0), machine_argument(machine, 1)
                )
                return_from_stub(0xB6B60000 | len(runtime.events))
        except AssertionError as error:
            hook_errors.append(f"{name}: {error}")
            machine.emu_stop()

    def on_write(_machine, _access, address, size, value, _user):
        if (
            SYSTEM_BASE <= address < SYSTEM_BASE + ARENA_SIZE
            or PARTICLE_BASE <= address < PARTICLE_BASE + ARENA_SIZE
        ):
            body_writes.append([address, size, value & ((1 << (size * 8)) - 1)])

    machine.hook_add(u.UC_HOOK_CODE, on_code)
    machine.hook_add(u.UC_HOOK_MEM_WRITE, on_write)
    entry = CODE_DELETE if test["entry"] == "delete" else CODE_RECONNECT
    execution_error = None
    try:
        machine.emu_start(entry, STOP, count=20000)
    except u.UcError as error:
        execution_error = str(error)

    actual_snapshot = runtime.snapshot()
    expected_topology = reference.topology()
    actual_topology = None
    topology_error = None
    try:
        actual_topology = runtime.topology()
    except AssertionError as error:
        topology_error = str(error)
    caller_window = bytes(machine.mem_read(ENTRY_SP, CALLER_WINDOW))
    final_nonvolatile = {
        name: machine.reg_read(register) & 0xFFFFFFFF
        for name, register in NONVOLATILE.items()
    }
    validation_errors = []
    if hook_errors:
        validation_errors.append({"code": "hook_error", "detail": hook_errors})
    if execution_error:
        validation_errors.append({"code": "unicorn_error", "detail": execution_error})
    if runtime.events != reference.events:
        validation_errors.append(
            {
                "code": "event_trace",
                "detail": first_event_difference(reference.events, runtime.events),
            }
        )
    if body_writes != reference.body_writes:
        validation_errors.append(
            {
                "code": "body_write_trace",
                "detail": {"expected": reference.body_writes, "actual": body_writes},
            }
        )
    for arena in ("systems", "particles", "definitions"):
        if actual_snapshot[arena] != expected_snapshot[arena]:
            validation_errors.append(
                {
                    "code": "arena_state",
                    "detail": {
                        "arena": arena,
                        "ranges": changed_ranges(expected_snapshot[arena], actual_snapshot[arena]),
                    },
                }
            )
    if runtime.pool_metadata() != reference.pool_metadata():
        validation_errors.append(
            {
                "code": "pool_metadata",
                "detail": {
                    "expected": reference.pool_metadata(),
                    "actual": runtime.pool_metadata(),
                },
            }
        )
    if topology_error is not None or actual_topology != expected_topology:
        validation_errors.append(
            {
                "code": "list_topology",
                "detail": {
                    "expected": expected_topology,
                    "actual": actual_topology,
                    "error": topology_error,
                },
            }
        )
    if runtime.private_delete_entries != reference.private_delete_entries:
        validation_errors.append(
            {
                "code": "private_delete_entries",
                "detail": {
                    "expected": reference.private_delete_entries,
                    "actual": runtime.private_delete_entries,
                },
            }
        )
    if caller_window != expected_caller_window:
        validation_errors.append({"code": "caller_window", "detail": "modified"})
    if machine.reg_read(UC_X86_REG_EIP) != STOP:
        validation_errors.append(
            {"code": "eip", "detail": hex(machine.reg_read(UC_X86_REG_EIP))}
        )
    if machine.reg_read(UC_X86_REG_ESP) != ENTRY_SP + 4:
        validation_errors.append(
            {"code": "esp", "detail": hex(machine.reg_read(UC_X86_REG_ESP))}
        )
    if (machine.reg_read(UC_X86_REG_EFLAGS) & 0x400) != (initial_eflags & 0x400):
        validation_errors.append({"code": "direction_flag", "detail": "changed"})
    if final_nonvolatile != saved:
        validation_errors.append(
            {"code": "nonvolatile", "detail": {"expected": saved, "actual": final_nonvolatile}}
        )
    if not all(
        event["payload_after_identifier_preserved"]
        for event in runtime.events
        if event["api"] == "datum_delete"
    ):
        validation_errors.append({"code": "delete_payload", "detail": "payload changed"})

    return {
        "case": test["name"],
        "entry": test["entry"],
        "events": runtime.events,
        "body_writes": body_writes,
        "arena_sha256": {
            name: sha256_bytes(value) for name, value in actual_snapshot.items()
        },
        "arena_changed_ranges_from_initial": {
            name: changed_ranges(runtime.initial[name], actual_snapshot[name])
            for name in actual_snapshot
        },
        "pool_metadata": runtime.pool_metadata(),
        "surviving_list_topology": actual_topology,
        "private_delete_entries": runtime.private_delete_entries,
        "caller_window_sha256": sha256_bytes(caller_window),
        "eip": machine.reg_read(UC_X86_REG_EIP) & 0xFFFFFFFF,
        "esp": machine.reg_read(UC_X86_REG_ESP) & 0xFFFFFFFF,
        "direction_flag": bool(machine.reg_read(UC_X86_REG_EFLAGS) & 0x400),
        "nonvolatile": final_nonvolatile,
        "volatile": {
            "eax": machine.reg_read(UC_X86_REG_EAX) & 0xFFFFFFFF,
            "ecx": machine.reg_read(UC_X86_REG_ECX) & 0xFFFFFFFF,
            "edx": machine.reg_read(UC_X86_REG_EDX) & 0xFFFFFFFF,
        },
        "validation_errors": validation_errors,
        "pass": not validation_errors,
    }


def canonical_trace(outcome: dict) -> dict:
    return {
        key: outcome[key]
        for key in (
            "entry",
            "events",
            "body_writes",
            "arena_sha256",
            "arena_changed_ranges_from_initial",
            "pool_metadata",
            "surviving_list_topology",
            "private_delete_entries",
            "caller_window_sha256",
            "eip",
            "esp",
            "direction_flag",
            "nonvolatile",
        )
    }


def run_role(prepared: dict, target_reference: dict[str, dict] | None = None) -> tuple[dict, dict]:
    outcomes = {}
    semantic_failures = []
    target_mismatches = []
    for index, test in enumerate(scenarios(), start=1):
        outcome = execute_case(prepared, test, 0x20 + index)
        outcomes[test["name"]] = outcome
        if not outcome["pass"]:
            semantic_failures.append(
                {"case": test["name"], "errors": outcome["validation_errors"]}
            )
        if target_reference is not None:
            trace = canonical_trace(outcome)
            if trace != target_reference[test["name"]]:
                target_mismatches.append(
                    {
                        "case": test["name"],
                        "differing_components": [
                            key
                            for key in trace
                            if trace[key] != target_reference[test["name"]][key]
                        ],
                    }
                )
    reference = {name: canonical_trace(value) for name, value in outcomes.items()}
    digest = sha256_bytes(
        json.dumps(reference, sort_keys=True, separators=(",", ":")).encode("utf-8")
    )
    return {
        "role": prepared["role"],
        "path": str(prepared["path"]),
        "sha256": prepared["sha256"],
        "coff_guards": {
            "delete": prepared["delete"]["checks"],
            "reconnect": prepared["reconnect"]["checks"],
        },
        "function_evidence": {
            kind: {
                "name": prepared[kind]["name"],
                "raw_sha256": prepared[kind]["raw_sha256"],
                "runtime_sha256": prepared[kind]["runtime_sha256"],
                "storage": prepared[kind]["storage"],
                "selection": prepared[kind]["selection"],
                "relocations": prepared[kind]["relocations"],
            }
            for kind in ("delete", "reconnect")
        },
        "function_names": {
            "delete": prepared["delete"]["name"],
            "reconnect": prepared["reconnect"]["name"],
        },
        "trace_sha256": digest,
        "case_count": len(outcomes),
        "semantic_failure_count": len(semantic_failures),
        "target_mismatch_count": len(target_mismatches),
        "semantic_failures": semantic_failures,
        "target_mismatches": target_mismatches,
        "pass": not semantic_failures and not target_mismatches,
        "outcomes": outcomes,
    }, reference


MUTANTS = {
    "delete_next_field_reads_plus_8": {
        "body": "delete",
        "offset": 0x67,
        "from": 0x04,
        "to": 0x08,
        "case": "delete_types_1_chain",
        "expected_code": "event_trace",
        "expected_expected_api": "datum_get",
        "expected_actual_api": "datum_delete",
        "expected_expected_args": [SYSTEM_PARTICLES_POOL, particle_handle(1)],
        "expected_actual_args": [PARTICLE_SYSTEMS_POOL, system_handle(0)],
    },
    "particle_invalid_branch_inverted": {
        "body": "reconnect",
        "offset": 0xB6,
        "from": 0x75,
        "to": 0x74,
        "case": "reconnect_types_4_all_splice_shapes",
        "expected_code": "event_trace",
        "expected_expected_api": "datum_delete",
        "expected_actual_api": "datum_get",
        "expected_expected_args": [SYSTEM_PARTICLES_POOL, particle_handle(0)],
        "expected_actual_args": [SYSTEM_PARTICLES_POOL, particle_handle(1)],
    },
    "delete_system_argument_uses_system_pointer": {
        "body": "delete",
        "offset": 0x97,
        "from": 0x52,
        "to": 0x53,
        "case": "delete_types_0",
        "expected_code": "hook_error",
        "expected_marker": "systems invalid handle",
    },
}


def mutate_prepared(prepared: dict, name: str, mutation: dict) -> dict:
    changed = dict(prepared)
    changed["role"] = "negative_control:" + name
    changed["delete"] = dict(prepared["delete"])
    changed["reconnect"] = dict(prepared["reconnect"])
    body = changed[mutation["body"]]
    raw = bytearray(body["runtime_raw"])
    if raw[mutation["offset"]] != mutation["from"]:
        raise AssertionError(
            (
                name,
                "mutation preimage",
                mutation["offset"],
                raw[mutation["offset"]],
                mutation["from"],
            )
        )
    raw[mutation["offset"]] = mutation["to"]
    body["runtime_raw"] = bytes(raw)
    return changed


def negative_observation_matches(outcome: dict, mutation: dict) -> bool:
    errors = outcome["validation_errors"]
    selected = [error for error in errors if error["code"] == mutation["expected_code"]]
    if len(selected) != 1:
        return False
    error = selected[0]
    if mutation["expected_code"] == "hook_error":
        return any(mutation["expected_marker"] in item for item in error["detail"])
    difference = error["detail"]
    expected = difference.get("expected")
    actual = difference.get("actual")
    return (
        isinstance(expected, dict)
        and isinstance(actual, dict)
        and expected.get("api") == mutation["expected_expected_api"]
        and actual.get("api") == mutation["expected_actual_api"]
        and expected.get("args") == mutation["expected_expected_args"]
        and actual.get("args") == mutation["expected_actual_args"]
    )


def run_negative_controls(target: dict) -> dict:
    by_name = {test["name"]: test for test in scenarios()}
    results = {}
    for index, (name, mutation) in enumerate(MUTANTS.items(), start=1):
        changed = mutate_prepared(target, name, mutation)
        outcome = execute_case(changed, by_name[mutation["case"]], 0x70 + index)
        detected = negative_observation_matches(outcome, mutation)
        results[name] = {
            "mutation": mutation,
            "validation_errors": outcome["validation_errors"],
            "detected_by_precise_expected_observation": detected,
        }
    return {
        "pass": all(
            result["detected_by_precise_expected_observation"]
            for result in results.values()
        ),
        "mutants": results,
    }


def data_semantics_guard() -> dict:
    source_evidence = verify_hash(DATA_SOURCE, PINNED["data_source"], "data_source")
    header_evidence = verify_hash(DATA_HEADER, PINNED["data_header"], "data_header")
    object_evidence = verify_hash(DATA_OBJECT, PINNED["data_object"], "data_object")
    source = DATA_SOURCE.read_text(encoding="utf-8")
    delete_block = source[source.index("void datum_delete(") : source.index("void data_delete_all(")]
    next_block = source[source.index("long data_next_index(") : source.index("long data_prev_index(")]
    source_checks = {
        "identifier_only_record_write": "header->identifier = 0;" in delete_block,
        "updates_first_free": "data->first_free_absolute_index = index;" in delete_block,
        "shrinks_trailing_count": "data->count--;" in delete_block,
        "decrements_actual_count": "data->actual_count--;" in delete_block,
        "no_payload_memset": "memset" not in delete_block,
        "no_free_or_reallocation": all(word not in delete_block for word in ("free(", "realloc", "dispose")),
        "next_uses_signed_low16_plus_one": "short absolute_index = index+1;" in next_block,
        "next_does_not_get_previous_datum": "datum_get(" not in next_block,
    }
    obj = cc.load(DATA_OBJECT)
    function = function_sections(obj)["_datum_delete"]
    symbol = cc.symbol(obj, "_datum_delete")
    object_checks = {
        "size_80": function.info["size"] == 80,
        "one_relocation": function.info["relocation_count"] == 1,
        "normalized_sha256": function.info["normalized_sha256"]
        == "3e87c1fd677ee13ed50e3dc48c09f3187024bce3e57440267a83202c192ee0ce",
        "public_function_owner": symbol["storage"] == 2 and symbol["type"] == 0x20,
        "comdat_noduplicates": _section_aux_selection(obj, symbol["section"]) == 1,
    }
    if not all(source_checks.values()) or not all(object_checks.values()):
        raise AssertionError(("datum_delete provenance guard", source_checks, object_checks))
    return {
        "source": source_evidence,
        "header": header_evidence,
        "object": object_evidence,
        "source_checks": source_checks,
        "object_checks": object_checks,
        "modeled_contract": (
            "clear only identifier +0..1, preserve payload +2 onward, update first-free/count/actual-count; "
            "data_next_index starts at signed low16(previous)+1 without revalidating previous"
        ),
    }


def optional_manifest_pair(
    parser: argparse.ArgumentParser,
    role: str,
    path: Path | None,
    digest: str | None,
) -> tuple[Path, str] | None:
    if (path is None) != (digest is None):
        parser.error(f"--{role}-manifest and --{role}-manifest-sha256 must be supplied together")
    if path is None:
        return None
    return path.resolve(), digest.lower()


def add_direct_object(
    parser: argparse.ArgumentParser,
    role: str,
    object_path: Path | None,
    object_digest: str | None,
    manifest_pair: tuple[Path, str] | None,
) -> tuple[Path, dict] | None:
    if (object_path is None) != (object_digest is None):
        parser.error(f"--{role}-object and --{role}-object-sha256 must be supplied together")
    if object_path is not None and manifest_pair is not None:
        parser.error(f"choose either a pinned {role} object or a pinned {role} manifest")
    if object_path is None:
        return None
    path = object_path.resolve()
    evidence = verify_hash(path, object_digest.lower(), role + "_object")
    return path, {"manifest": None, "phase": "standalone " + role, "object": evidence}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--first-manifest", type=Path)
    parser.add_argument("--first-manifest-sha256")
    parser.add_argument("--first-object", type=Path)
    parser.add_argument("--first-object-sha256")
    parser.add_argument("--actual-manifest", type=Path)
    parser.add_argument("--actual-manifest-sha256")
    parser.add_argument("--actual-object", type=Path)
    parser.add_argument("--actual-object-sha256")
    parser.add_argument("--output", type=Path, default=OUTPUT_DEFAULT)
    args = parser.parse_args()

    output = args.output.resolve()
    if output.exists():
        raise ValueError(f"refusing to overwrite output: {output}")
    if not output.is_relative_to((ROOT / "scratch").resolve()):
        raise ValueError(f"output must remain under scratch: {output}")

    requested = {
        "first": optional_manifest_pair(
            parser, "first", args.first_manifest, args.first_manifest_sha256
        ),
        "actual": optional_manifest_pair(
            parser, "actual", args.actual_manifest, args.actual_manifest_sha256
        ),
    }
    target_path, target_evidence = load_manifest_object(
        BEFORE_MANIFEST, PINNED["before_manifest"], "target"
    )
    paths = {"target": target_path}
    evidence = {"target": target_evidence}
    for role, pair in requested.items():
        direct = add_direct_object(
            parser,
            role,
            getattr(args, role + "_object"),
            getattr(args, role + "_object_sha256"),
            pair,
        )
        if direct is not None:
            paths[role], evidence[role] = direct
        elif pair is not None:
            paths[role], evidence[role] = load_manifest_object(pair[0], pair[1], role)

    prepared = {role: prepare_role(path, role) for role, path in paths.items()}
    target_result, target_reference = run_role(prepared["target"])
    if not target_result["pass"]:
        raise AssertionError(("target semantic oracle failed", target_result["semantic_failures"]))
    objects = {"target": target_result}
    for role in ("first", "actual"):
        if role in prepared:
            objects[role], unused = run_role(prepared[role], target_reference)

    negative_controls = run_negative_controls(prepared["target"])
    datum_contract = data_semantics_guard()
    passed = all(result["pass"] for result in objects.values()) and negative_controls["pass"]
    report = {
        "pass": passed,
        "candidate_status": {
            role: "executed" if role in prepared else "pending pinned manifest"
            for role in ("first", "actual")
        },
        "scope": ["particle_system_delete", "particle_systems_reconnect_to_structure_bsp"],
        "script": {"path": str(SCRIPT), "sha256": sha256_path(SCRIPT)},
        "evidence": evidence,
        "datum_delete_and_iteration_contract": datum_contract,
        "objects": objects,
        "negative_controls": negative_controls,
        "coverage": {
            "case_count": len(scenarios()),
            "entry_points": sorted({test["entry"] for test in scenarios()}),
            "type_counts": sorted(
                {
                    len(spec["types"])
                    for test in scenarios()
                    for spec in test["systems"]
                }
            ),
            "list_splices": ["head", "interior", "tail", "all-invalid"],
            "attached_and_unattached": True,
            "out_of_cluster_system_deletion": True,
            "empty_system_pool": True,
            "full_seeded_system_particle_definition_arenas_bytes_each": ARENA_SIZE,
        },
        "claims": [
            "The two real function sections execute together; reconnect reaches the mapped private delete body rather than a semantic replacement.",
            "Only data/tag/object-location/scenario-location APIs are stubbed, with exact cdecl argument and call-order traces.",
            "The datum_delete stub follows the pinned genuine source and machine owner: only identifier bytes are cleared, payload survives, and pool bookkeeping is updated.",
            "Every deletion records its sequence and payload-retention result; reconnect's machine body performs the post-delete +4 read and linked-list splice.",
            "Every case checks full seeded system, particle, and definition arenas, body write trace, pool metadata, caller window, ESP, EIP, DF, and EBP/EBX/ESI/EDI.",
        ],
        "limits": [
            "Cases stay in the authenticated valid-definition domain with type counts 0 through 4; no invalid-count behavior is claimed.",
            "Datum handles are valid salted modeled handles; the oracle does not generalize to stale or malformed handles.",
            "Location APIs are deterministic typed stubs, not executions of BSP/object subsystems.",
            "System and particle records use guard-separated 0x200/0x100 strides; the pool tokens and count/actual-count/first-free bookkeeping are typed API-stub state, not execution of a contiguous engine data_array.",
            "All modeled slots are low nonnegative indices below 32, where s16(previous)+1 is equivalent to the genuine signed-short conversion of previous+1; no wrap-boundary behavior is claimed.",
            "Any candidate role omitted from a run remains pending until its frozen manifest/object and SHA-256 are supplied; roles listed as executed in this report were pinned and compared.",
            "The report is bounded representative evidence, not an exhaustive state-space proof.",
        ],
    }
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        json.dumps(
            {
                "pass": passed,
                "output": str(output),
                "output_sha256": sha256_path(output),
                "script_sha256": report["script"]["sha256"],
                "cases": len(scenarios()),
                "roles": list(objects),
                "negative_controls": negative_controls["pass"],
            },
            indent=2,
        )
    )
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
