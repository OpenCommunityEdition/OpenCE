"""Bounded frozen-byte oracle for ActorLooking direction decode + movement caller.

The harness runs the January target, the first natural candidate, and the
frozen ordinary-Ninja candidate.  It enters the genuine private decoder ABI
and the genuine public ``actor_look_affect_movement`` caller.  The reached
``normalize3d``, ``vector_from_points3d``, and ``valid_real_normal3d`` COFF
bodies execute as machine code.  Only genuine outside engine/CRT APIs are
modeled, with their stack arguments decoded and recorded.

This is deliberately finite evidence.  In particular, the January decoder
inlines its first normal-vector validity test while the candidates call the
byte-identical out-of-line helper.  The corpus exercises both x87 control
words but is not exhaustive proof at floating-point decision boundaries.

The script writes only a new JSON path under ``scratch``.  It never compiles,
invokes Ninja, edits production/configuration files, or mutates input objects.
Negative controls alter copies of already-relocated candidate bytes in memory.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import sys

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
    UC_X86_REG_FPTAG,
)


SCRIPT = Path(__file__).resolve()
ROOT = next(parent for parent in SCRIPT.parents if (parent / "tools/coff_compare.py").is_file())
sys.path.insert(0, str(ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import function_sections  # noqa: E402
from tools.regression_gate import _section_aux_selection  # noqa: E402


TARGET = ROOT / "scratch/actor-looking-decoder-before-20260906/files/build/split/source/ai/actor_looking.obj"
FIRST = ROOT / "scratch/actor-looking-decoder-first-20260906.obj"
ACTUAL = ROOT / "scratch/actor-looking-decoder-first-20260906/source/ai/actor_looking.obj"
BEFORE_MANIFEST = ROOT / "scratch/actor-looking-decoder-before-20260906.json"
FIRST_MANIFEST = ROOT / "scratch/actor-looking-decoder-first-20260906.json"
BEFORE_STABLE = ROOT / "scratch/actor-looking-decoder-before-stable-20260906.json"
FIRST_STABLE = ROOT / "scratch/actor-looking-decoder-first-stable-20260906.json"
CACHEBETA = ROOT / "cachebeta.exe"
TARGET_ALERT = ROOT / "build/split/source/ai/action_alert.obj"
TARGET_CHARGE = ROOT / "build/split/source/ai/action_charge.obj"
TARGET_COMBAT = ROOT / "build/split/source/ai/actor_combat.obj"
OUTPUT_DEFAULT = ROOT / "scratch/actor-looking-decoder-runtime-report.json"

PINNED_HASHES = {
    "target": "443e269f49461dcaf5ff3804fc8770cc5579798427289c60f56a1ce533a226de",
    "first": "4dc689d2ca1e996ed9786915663ed0962dfbb8ef3556b6edb36d4bcd42f43387",
    "actual": "0d3378d771f780f909a6d7d29d2c9eec14cfd65f5901c6efbec82a5fd737b175",
    "before_manifest": "5edecbfb16f421a7eedafb89121ef42348cee3ecdbfe45ce8a0f272b6263e3ef",
    "first_manifest": "513b356efb3903cc6f3d0f012e5506e6152601c0d4a09c5843cb89d7346003e8",
    "before_stable": "b13c8826c3323d4a472bf46c94af12b818cc7b8d67fd402e0eb13aafe200b1a6",
    "first_stable": "edbbb60142e71b3132b38bc00563c945c4a06038c998a204c4685c4f25cdff98",
    "cachebeta": "4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520",
    "target_alert": "351ff0de9ddfea9b3c03a9038abab75b0c4b48290443b33e09c33105b41ab61a",
    "target_charge": "76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea",
    "target_combat": "6030e4891bf64d0cb0c41028a6c816ef07c4442f6c573ac70f37fb1e049ca45b",
    "coff_compare": "22f8e11c7b5b5b1e1cb9bc51d2e48c0fa4912def9c1d48bd09db80735ee8204e",
    "coff_tools": "6784bd1a1e241942f301e2691862af596554bf8fbf4333437c0451988660d3c5",
    "regression_gate": "299f15d1569096730508c85eee9271ba4798c8e4f42f51bdccb89f1c1d84ea3f",
}

DECODER = "_actor_look_decode_direction"
AFFECT = "_actor_look_affect_movement"
TARGET_DECODER = "_code_00016c70"
HELPERS = (
    "_square_root",
    "_vector_from_points3d",
    "_scale_vector3d",
    "_magnitude_squared3d",
    "_magnitude3d",
    "_normalize3d",
    "_valid_real",
    "_valid_realcmp",
    "_valid_real_normal3d",
)
EXECUTED_HELPERS = {
    "_vector_from_points3d",
    "_normalize3d",
    "_valid_real_normal3d",
}
APIS = (
    "_datum_get",
    "_datum_try_and_get",
    "_object_try_and_get_and_verify_type",
    "_unit_get_head_position",
    "_object_get_origin",
    "_actor_path_has_path",
    "_display_assert",
    "_system_exit",
    "_sprintf",
    "_csprintf",
)

FUNCTION_FACTS = {
    "target": {
        DECODER: (1632, 98, "4f3714fab510f0c407dc0792b3cbea003ba4d1dfdb4aee74d201d9a9e5dc14a9", 2),
        AFFECT: (128, 4, "5d7d253b925542d2e1c1c21c83c6a2f2049df1a6fe051b6f713b377c47f85dde", 2),
    },
    "first": {
        DECODER: (1616, 99, "bea2ec692aedec4276c07fb076ad2dd53888277627541c42fb56dea1670b3008", 3),
        AFFECT: (128, 4, "d096436ec233acf74823c8ce959e2ded832eb046b0c8b441be976c5e6cf5dafa", 2),
    },
    "actual": {
        DECODER: (1616, 99, "bea2ec692aedec4276c07fb076ad2dd53888277627541c42fb56dea1670b3008", 3),
        AFFECT: (128, 4, "d096436ec233acf74823c8ce959e2ded832eb046b0c8b441be976c5e6cf5dafa", 2),
    },
}
HELPER_FACTS = {
    "_square_root": (16, 0, "0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7"),
    "_vector_from_points3d": (48, 0, "905f56dbe4597d2bacc201456ffc33464b166e0b6392dda24a1a3e109b637aa2"),
    "_scale_vector3d": (48, 0, "6436919fbdc80b5d5fe76d9c92fa146e5ac7d3b4637c9b75aaa62401f0c54a3b"),
    "_magnitude_squared3d": (48, 0, "0d04579096eeab3a1c45f87774e1eb20ce4856dbac219f624a855e8c9ae99725"),
    "_magnitude3d": (48, 0, "114f10e7ee4543446daaaafc859a4ec729ba6c8c74c38825d05bd0f00a42352c"),
    "_normalize3d": (96, 3, "67fbd33f204d0c93cce135fcfd37d4ce5b443642edc6ad99230846b91c893e5b"),
    "_valid_real": (32, 0, "83977ed5f0ae820e1554588c354ae57295d798d933e591ee898dc39d29731d4f"),
    "_valid_realcmp": (64, 1, "af811d6df5f204708d247a7d50e091f7828989672ded12773edd93431f74852c"),
    "_valid_real_normal3d": (96, 2, "0958b20f958923b9a2b2561eebda24ae8838b04180389aaaf3926319b19599c0"),
}

CODE_ADDRESSES = {DECODER: 0x100000, AFFECT: 0x110000}
CODE_ADDRESSES.update({name: 0x120000 + index * 0x2000 for index, name in enumerate(HELPERS)})
STUB_BASE = 0x200000
STUB_ADDRESSES = {name: STUB_BASE + 0x100 * (index + 1) for index, name in enumerate(APIS)}
STOP = 0x20F000
RDATA = 0x300000
RDATA_SIZE = 0x40000
GLOBALS = 0x400000
ACTOR_DATA_SLOT = GLOBALS + 0x100
PROP_DATA_SLOT = GLOBALS + 0x104
ACTOR_DATA_VALUE = 0x2468ACE0
PROP_DATA_VALUE = 0x13579BDF
ACTOR = 0x500000
SPEC = 0x502000
DIRECTION = 0x504000
PROP = 0x506000
OBJECT = 0x508000
TEMPORARY = 0x50A000
STACK = 0x600000
STACK_SIZE = 0x20000
ENTRY_SP = STACK + 0x10000

ACTOR_SIZE = 0x800
SMALL_SIZE = 0x200
TEMPORARY_SIZE = 0x1000
PAGE_SIZE = 0x1000
CALLER_FRAME_SIZE = 0x80
FPCW_VALUES = (0x027F, 0x037F)
NONE = 0xFFFFFFFF

HEAD_POSITION = 0x120
TARGET_PROP_INDEX = 0x270
DANGER_TYPE = 0x280
DANGER_POSITION = 0x2B0
PRIMARY_PRIORITY = 0x3E8
PRIMARY_DIRECTION = 0x3EC
MOVING = 0x504
FORCED_BY_AIMING = 0x505
MOVING_POINT = 0x50C
MOVING_VECTOR = 0x518
FORCED_DIRECTION = 0x524
FIRE_STATE = 0x5F2
FIRE_TARGET_TYPE = 0x60C
AIMING_AT_FIRE_TARGET = 0x628
FIRE_AIM_VECTOR = 0x63C
BURST_AIM_VECTOR = 0x68C

PROP_OWNER = 0x04
PROP_CENTER = 0xC8
PROP_HEAD = 0x104
OBJECT_TYPE = 0x64

DIRECTION_MOVEMENT = 0
DIRECTION_PROP = 1
DIRECTION_TARGET = 2
DIRECTION_POINT = 3
DIRECTION_VECTOR = 4
DIRECTION_DANGER = 5
DIRECTION_OBJECT = 6
FIRE_BURSTING = 2
PRIMARY_AIMING = 3

NONVOLATILE = {
    "ebp": UC_X86_REG_EBP,
    "ebx": UC_X86_REG_EBX,
    "esi": UC_X86_REG_ESI,
    "edi": UC_X86_REG_EDI,
}
SAVED = {
    "ebp": 0x1122AABB,
    "ebx": 0x2233BBCC,
    "esi": 0x3344CCDD,
    "edi": 0x4455DDEE,
}

EXE_CONSTANTS = {
    "??_C@_00CNPNBAHC@?$AA@": (2372623, b""),
    "??_C@_0CL@GJIEKICC@?$CFs?3?5assert_valid_real_normal3d?$CI?$CF@": (
        2377156,
        b"%s: assert_valid_real_normal3d(%f, %f, %f)",
    ),
}
FORMAT_RE = re.compile(r"%(?P<flags>[-+ #0]*)(?P<width>\d*)(?:\.(?P<precision>\d+))?(?P<kind>[fsdXu])")


def sha256_bytes(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def sha256_path(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def verify_hash(path: Path, expected: str, label: str) -> dict:
    actual = sha256_path(path)
    if actual.lower() != expected.lower():
        raise AssertionError((label, "SHA-256 mismatch", str(path), actual, expected))
    return {"path": str(path), "sha256": actual, "bytes": path.stat().st_size}


def p16(value: int) -> bytes:
    return struct.pack("<H", value & 0xFFFF)


def p32(value: int) -> bytes:
    return struct.pack("<I", value & 0xFFFFFFFF)


def r32(machine: u.Uc, address: int) -> int:
    return struct.unpack("<I", machine.mem_read(address, 4))[0]


def s32(value: int) -> int:
    return struct.unpack("<i", p32(value))[0]


def f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


def f32_bits(value: float) -> int:
    return struct.unpack("<I", struct.pack("<f", f32(value)))[0]


def bits_f32(value: int) -> float:
    return struct.unpack("<f", p32(value))[0]


def write16(raw: bytearray, offset: int, value: int) -> None:
    raw[offset : offset + 2] = p16(value)


def write32(raw: bytearray, offset: int, value: int) -> None:
    raw[offset : offset + 4] = p32(value)


def write_float(raw: bytearray, offset: int, value: float) -> None:
    raw[offset : offset + 4] = struct.pack("<f", f32(value))


def write_vector(raw: bytearray, offset: int, value: tuple[float, float, float]) -> None:
    for index, component in enumerate(value):
        write_float(raw, offset + 4 * index, component)


def seeded_bytes(size: int, seed: int) -> bytes:
    return bytes(((index * 37 + seed * 53 + 11) & 0xFF) for index in range(size))


def changed_offsets(before: bytes, after: bytes) -> list[int]:
    if len(before) != len(after):
        raise AssertionError("change comparison length mismatch")
    return [index for index, (left, right) in enumerate(zip(before, after)) if left != right]


def float_token(value: float) -> str:
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "+inf" if value > 0 else "-inf"
    return value.hex()


class Owner:
    def __init__(self, label: str, path: Path, expected_hash: str):
        self.label = label
        self.path = path
        self.input = verify_hash(path, expected_hash, label)
        self.obj = cc.load(path)
        self.functions = function_sections(self.obj)


class ConstantPool:
    def __init__(self, owners: list[Owner], cachebeta: bytes):
        self.owners = owners
        self.cachebeta = cachebeta
        self.section_addresses: dict[tuple[str, int], int] = {}
        self.blocks: dict[int, bytes] = {}
        self.records: list[dict] = []
        self.next_slot = 0

    def _find(self, name: str, preferred: Owner | None) -> tuple[Owner, dict] | None:
        ordered = ([preferred] if preferred is not None else []) + [
            owner for owner in self.owners if owner is not preferred
        ]
        for owner in ordered:
            matches = [
                symbol
                for symbol in owner.obj["symbols"]
                if symbol["name"] == name and symbol["section"] > 0
            ]
            if len(matches) > 1:
                raise AssertionError((name, "ambiguous definition", owner.label, len(matches)))
            if matches:
                symbol = matches[0]
                section = owner.obj["sections"][symbol["section"] - 1]
                if section["flags"] & cc.IMAGE_SCN_CNT_CODE:
                    raise AssertionError((name, "constant resolved to code", owner.label))
                return owner, symbol
        return None

    def address(self, name: str, addend: int, preferred: Owner | None = None) -> int:
        found = self._find(name, preferred)
        if found is None:
            if name not in EXE_CONSTANTS:
                raise AssertionError((name, "no authenticated non-code owner"))
            offset, expected = EXE_CONSTANTS[name]
            end = self.cachebeta.find(b"\0", offset)
            if end < 0:
                raise AssertionError((name, "unterminated executable string"))
            raw = self.cachebeta[offset : end + 1]
            if raw[:-1] != expected:
                raise AssertionError((name, "executable string mismatch", raw[:-1], expected))
            key = ("cachebeta.exe", offset)
            symbol_value = 0
            provenance = {"kind": "cachebeta.exe", "file_offset": offset}
        else:
            owner, symbol = found
            section_number = symbol["section"]
            section = owner.obj["sections"][section_number - 1]
            raw = bytes(cc._section_bytes(owner.obj, section))
            key = (owner.label, section_number)
            symbol_value = symbol["value"]
            provenance = {
                "kind": "coff-section",
                "owner": owner.label,
                "path": str(owner.path),
                "section": section_number,
            }
        if key not in self.section_addresses:
            if len(raw) > 0x1000 or (self.next_slot + 1) * 0x1000 > RDATA_SIZE:
                raise AssertionError((name, "constant arena bound", len(raw), self.next_slot))
            base = RDATA + self.next_slot * 0x1000
            self.next_slot += 1
            self.section_addresses[key] = base
            self.blocks[base] = raw
            self.records.append(
                {
                    "base": base,
                    "bytes": len(raw),
                    "sha256": sha256_bytes(raw),
                    **provenance,
                }
            )
        return self.section_addresses[key] + symbol_value + addend


def direct_case(name: str, direction_type: int | None, expected_al: int | None, **values) -> dict:
    result = {
        "name": name,
        "entry": "decoder",
        "direction_type": direction_type,
        "expected_al": expected_al,
        "moving": 0,
        "movement_vector": (3.0, 4.0, 0.0),
        "movement_point": (11.0, 22.0, 33.0),
        "head": (1.0, 2.0, 3.0),
        "spec_vector": (0.6, 0.8, 0.0),
        "spec_point": (4.0, 6.0, 3.0),
        "prop_present": True,
        "prop_head": (4.0, 6.0, 3.0),
        "prop_center": (4.0, 6.0, 3.0),
        "prop_try_sequence": ["prop"],
        "target_prop": NONE,
        "fire_state": 0,
        "aiming": 0,
        "fire_vector": (0.0, 0.0, 1.0),
        "burst_vector": (0.6, 0.8, 0.0),
        "danger_type": 0,
        "danger_position": (4.0, 6.0, 3.0),
        "object_present": True,
        "object_type": 0,
        "object_point": (4.0, 6.0, 3.0),
        "null_spec": False,
        "null_direction": False,
        "expected_api": ["_datum_get"],
        "vector_calls": 0,
        "normalize_calls": 0,
        "decoder_result_true": expected_al == 1,
        "expected_direction": "unchanged",
        "expected_display": [],
        "terminated": False,
    }
    result.update(values)
    return result


def affect_case(name: str, direction_type: int, priority: int, **values) -> dict:
    result = {
        "name": name,
        "entry": "affect",
        "direction_type": direction_type,
        "priority": priority,
        "path_result": False,
        "head": (1.0, 2.0, 3.0),
        "spec_vector": (0.6, 0.8, 0.0),
        "spec_point": (1.0, 2.0, 3.0),
        "expected_api": ["_datum_get"],
        "vector_calls": 0,
        "normalize_calls": 0,
        "decoder_result_true": False,
        "nested_decoder": False,
        "expected_priority": priority,
        "expected_flag": 0,
        "expected_forced_direction": "unchanged",
        "terminated": False,
        "expected_display": [],
    }
    result.update(values)
    return result


def cases() -> list[dict]:
    diagnostic_tail = ["_sprintf", "_display_assert", "_csprintf", "_display_assert", "_system_exit"]
    direct = [
        direct_case(
            "movement_success", DIRECTION_MOVEMENT, 1, moving=1,
            normalize_calls=1, expected_direction=(0.6, 0.8, 0.0),
        ),
        direct_case("movement_not_moving", DIRECTION_MOVEMENT, 0, moving=0),
        direct_case(
            "prop_success", DIRECTION_PROP, 1,
            expected_api=["_datum_get", "_datum_try_and_get"], vector_calls=1,
            normalize_calls=1, expected_direction=(0.6, 0.8, 0.0),
        ),
        direct_case(
            "prop_missing", DIRECTION_PROP, 0, prop_present=False,
            prop_try_sequence=[None], expected_api=["_datum_get", "_datum_try_and_get"],
        ),
        direct_case(
            "target_burst", DIRECTION_TARGET, 1, fire_state=FIRE_BURSTING,
            expected_direction=(0.6, 0.8, 0.0),
        ),
        direct_case(
            "target_fire_aim", DIRECTION_TARGET, 1, aiming=1,
            expected_direction=(0.0, 0.0, 1.0),
        ),
        direct_case(
            "target_prop", DIRECTION_TARGET, 1, target_prop="prop",
            expected_api=["_datum_get", "_datum_get"], vector_calls=1,
            normalize_calls=1, expected_direction=(0.6, 0.8, 0.0),
        ),
        direct_case("target_none", DIRECTION_TARGET, 0),
        direct_case(
            "point_success", DIRECTION_POINT, 1, vector_calls=0,
            normalize_calls=1, expected_direction=(0.6, 0.8, 0.0),
        ),
        direct_case(
            "point_zero", DIRECTION_POINT, 0, spec_point=(1.0, 2.0, 3.0),
            vector_calls=0, normalize_calls=1, decoder_result_true=False,
            expected_direction=(0.0, 0.0, 0.0),
        ),
        direct_case(
            "vector_success", DIRECTION_VECTOR, 1,
            expected_direction=(0.6, 0.8, 0.0),
        ),
        direct_case(
            "danger_success", DIRECTION_DANGER, 1, danger_type=1,
            vector_calls=1, normalize_calls=1, expected_direction=(0.6, 0.8, 0.0),
        ),
        direct_case("danger_none", DIRECTION_DANGER, 0),
        direct_case(
            "object_unit", DIRECTION_OBJECT, 1, object_type=0,
            expected_api=["_datum_get", "_object_try_and_get_and_verify_type", "_unit_get_head_position"],
            vector_calls=1, normalize_calls=1, expected_direction=(0.6, 0.8, 0.0),
        ),
        direct_case(
            "object_general", DIRECTION_OBJECT, 1, object_type=2,
            expected_api=["_datum_get", "_object_try_and_get_and_verify_type", "_object_get_origin"],
            vector_calls=1, normalize_calls=1, expected_direction=(0.6, 0.8, 0.0),
        ),
        direct_case(
            "object_missing", DIRECTION_OBJECT, 0, object_present=False,
            expected_api=["_datum_get", "_object_try_and_get_and_verify_type"],
        ),
        direct_case(
            "null_specification", None, None, null_spec=True,
            expected_api=["_datum_get", "_display_assert", "_system_exit"],
            expected_display=[["specification", 349, 1]], terminated=True,
        ),
        direct_case(
            "null_direction", DIRECTION_VECTOR, None, null_direction=True,
            expected_api=["_datum_get", "_display_assert", "_system_exit"],
            expected_display=[["direction", 350, 1]], terminated=True,
        ),
        direct_case(
            "invalid_type_7", 7, None,
            expected_api=["_datum_get", "_display_assert", "_system_exit"],
            expected_display=[[None, 449, 1]], terminated=True,
        ),
        direct_case(
            "vector_denormalized", DIRECTION_VECTOR, None, spec_vector=(2.0, 0.0, 0.0),
            expected_api=["_datum_get"] + diagnostic_tail,
            expected_direction=(2.0, 0.0, 0.0), decoder_result_true=True,
            expected_display=[["temporary", 526, 0], ["temporary", 529, 1]], terminated=True,
        ),
        direct_case(
            "vector_lower_boundary_outside_0x3f7fdf39", DIRECTION_VECTOR, None,
            spec_vector=(bits_f32(0x3F7FDF39), 0.0, 0.0),
            expected_api=["_datum_get"] + diagnostic_tail,
            expected_direction=(bits_f32(0x3F7FDF39), 0.0, 0.0), decoder_result_true=True,
            expected_display=[["temporary", 526, 0], ["temporary", 529, 1]], terminated=True,
        ),
        direct_case(
            "vector_lower_boundary_inside_0x3f7fdf3a", DIRECTION_VECTOR, 1,
            spec_vector=(bits_f32(0x3F7FDF3A), 0.0, 0.0),
            expected_direction=(bits_f32(0x3F7FDF3A), 0.0, 0.0),
        ),
        direct_case(
            "vector_upper_boundary_inside_0x3f801061", DIRECTION_VECTOR, 1,
            spec_vector=(bits_f32(0x3F801061), 0.0, 0.0),
            expected_direction=(bits_f32(0x3F801061), 0.0, 0.0),
        ),
        direct_case(
            "vector_upper_boundary_outside_0x3f801062", DIRECTION_VECTOR, None,
            spec_vector=(bits_f32(0x3F801062), 0.0, 0.0),
            expected_api=["_datum_get"] + diagnostic_tail,
            expected_direction=(bits_f32(0x3F801062), 0.0, 0.0), decoder_result_true=True,
            expected_display=[["temporary", 526, 0], ["temporary", 529, 1]], terminated=True,
        ),
        direct_case(
            "movement_infinite_diagnostic", DIRECTION_MOVEMENT, None, moving=1,
            movement_vector=(math.inf, 0.0, 0.0), normalize_calls=1,
            expected_api=["_datum_get"] + diagnostic_tail,
            expected_direction="nonfinite", decoder_result_true=True,
            expected_display=[["temporary", 526, 0], ["temporary", 529, 1]], terminated=True,
        ),
        direct_case(
            "prop_infinite_invalid_second_lookup", DIRECTION_PROP, None,
            head=(0.0, 0.0, 0.0), prop_head=(math.inf, 0.0, 0.0),
            prop_try_sequence=["prop", None], vector_calls=1, normalize_calls=1,
            expected_api=["_datum_get", "_datum_try_and_get", "_datum_try_and_get"] + diagnostic_tail,
            expected_direction="nonfinite", decoder_result_true=True,
            expected_display=[["temporary", 526, 0], ["temporary", 529, 1]], terminated=True,
        ),
    ]
    affect = [
        affect_case(
            "affect_movement_no_path", DIRECTION_MOVEMENT, 4,
            expected_api=["_datum_get", "_actor_path_has_path"],
            expected_priority=0,
        ),
        affect_case(
            "affect_movement_has_path", DIRECTION_MOVEMENT, 4, path_result=True,
            expected_api=["_datum_get", "_actor_path_has_path"],
            expected_priority=4,
        ),
        affect_case("affect_vector_priority_2", DIRECTION_VECTOR, 2),
        affect_case(
            "affect_vector_priority_3", DIRECTION_VECTOR, 3,
            expected_api=["_datum_get", "_datum_get"], nested_decoder=True,
            decoder_result_true=True, expected_flag=1,
            expected_forced_direction=(0.6, 0.8, 0.0),
        ),
        affect_case(
            "affect_vector_priority_4", DIRECTION_VECTOR, 4,
            expected_api=["_datum_get", "_datum_get"], nested_decoder=True,
            decoder_result_true=True, expected_flag=1,
            expected_forced_direction=(0.6, 0.8, 0.0),
        ),
        affect_case(
            "affect_point_zero_priority_3", DIRECTION_POINT, 3,
            expected_api=["_datum_get", "_datum_get"], nested_decoder=True,
            vector_calls=0, normalize_calls=1, decoder_result_true=False,
            expected_flag=0, expected_forced_direction=(0.0, 0.0, 0.0),
        ),
    ]
    return direct + affect


class RoleImage:
    def __init__(self, role: str, primary_path: Path, primary_hash: str):
        self.role = role
        self.primary = Owner(role + ":primary", primary_path, primary_hash)
        self.target_alert = Owner(role + ":target-alert", TARGET_ALERT, PINNED_HASHES["target_alert"])
        self.target_charge = Owner(role + ":target-charge", TARGET_CHARGE, PINNED_HASHES["target_charge"])
        self.target_combat = Owner(role + ":target-combat", TARGET_COMBAT, PINNED_HASHES["target_combat"])
        self.cachebeta = CACHEBETA.read_bytes()
        if sha256_bytes(self.cachebeta) != PINNED_HASHES["cachebeta"]:
            raise AssertionError((role, "cachebeta SHA-256 mismatch"))
        self.constants = ConstantPool(
            [self.primary, self.target_alert, self.target_charge, self.target_combat],
            self.cachebeta,
        )
        if role == "target":
            self.helper_owners = {
                "_square_root": self.target_charge,
                "_vector_from_points3d": self.target_alert,
                "_scale_vector3d": self.target_charge,
                "_magnitude_squared3d": self.target_alert,
                "_magnitude3d": self.target_charge,
                "_normalize3d": self.target_charge,
                "_valid_real": self.target_combat,
                "_valid_realcmp": self.target_combat,
                "_valid_real_normal3d": self.target_combat,
            }
        else:
            self.helper_owners = {name: self.primary for name in HELPERS}

        self.runtime_raw: dict[str, bytes] = {}
        self.section_raw: dict[str, bytes] = {}
        self.section_checks: dict[str, dict] = {}
        self.relocations: dict[str, list[dict]] = {}
        self.function_owner: dict[str, str] = {}
        decoder_owner_name = TARGET_DECODER if role == "target" else DECODER
        self._materialize(DECODER, self.primary, decoder_owner_name)
        self._materialize(AFFECT, self.primary, AFFECT)
        for name in HELPERS:
            self._materialize(name, self.helper_owners[name], name)
        self._cross_guards()

    def _destination(self, name: str, addend: int, owner: Owner) -> int:
        if name == "_actor_data":
            return ACTOR_DATA_SLOT + addend
        if name == "_prop_data":
            return PROP_DATA_SLOT + addend
        if name == "_temporary":
            return TEMPORARY + addend
        if name in STUB_ADDRESSES:
            return STUB_ADDRESSES[name] + addend
        if name in CODE_ADDRESSES:
            return CODE_ADDRESSES[name] + addend
        if name == TARGET_DECODER:
            return CODE_ADDRESSES[DECODER] + addend
        return self.constants.address(name, addend, owner)

    def _materialize(self, canonical_name: str, owner: Owner, owner_name: str) -> None:
        if owner_name not in owner.functions:
            raise AssertionError((self.role, owner.label, "missing function", owner_name))
        function = owner.functions[owner_name]
        symbol = cc.symbol(owner.obj, owner_name)
        section = owner.obj["sections"][function.section_number - 1]
        raw = bytearray(cc._section_bytes(owner.obj, section))
        base = CODE_ADDRESSES[canonical_name]
        relocations = []
        for relocation in function.info["relocations"]:
            target = relocation["target"]
            if target[0] == "internal":
                destination = base + target[1]
                target_name = "$internal"
                addend = target[1]
            elif target[0] == "symbol":
                target_name, addend = target[1], target[2]
                destination = self._destination(target_name, addend, owner)
            elif target[0] == "defined-noncode":
                target_name, addend = target[2], target[3]
                destination = self.constants.address(target_name, addend, owner)
            else:
                raise AssertionError((self.role, canonical_name, "unsupported relocation", target))
            if relocation["type"] == cc.IMAGE_REL_I386_REL32:
                patched = destination - (base + relocation["address"] + 4)
            elif relocation["type"] == cc.IMAGE_REL_I386_DIR32:
                patched = destination
            else:
                raise AssertionError((canonical_name, "unsupported relocation type", relocation))
            raw[relocation["address"] : relocation["address"] + 4] = p32(patched)
            relocations.append(
                {
                    "address": relocation["address"],
                    "type": relocation["type"],
                    "target": target_name,
                    "addend": addend,
                    "destination": destination & 0xFFFFFFFF,
                }
            )

        if canonical_name in (DECODER, AFFECT):
            size, relocation_count, normalized_sha256, storage = FUNCTION_FACTS[self.role][canonical_name]
            expected_selection = 1
        else:
            size, relocation_count, normalized_sha256 = HELPER_FACTS[canonical_name]
            storage = 2
            expected_selection = 1 if self.role == "target" else 2
        checks = {
            "size": function.info["size"] == size == len(raw),
            "relocation_count": function.info["relocation_count"] == relocation_count,
            "normalized_sha256": function.info["normalized_sha256"] == normalized_sha256,
            "storage": symbol["storage"] == storage,
            "function_type": symbol["type"] == 0x20,
            "comdat_selection": _section_aux_selection(owner.obj, symbol["section"])
            == expected_selection,
        }
        if not all(checks.values()):
            raise AssertionError((self.role, canonical_name, "COFF guard", checks))
        self.runtime_raw[canonical_name] = bytes(raw)
        self.section_raw[canonical_name] = bytes(cc._section_bytes(owner.obj, section))
        self.section_checks[canonical_name] = checks
        self.relocations[canonical_name] = relocations
        self.function_owner[canonical_name] = owner.label

    def _cross_guards(self) -> None:
        valid_calls = Counter(item["target"] for item in self.relocations[DECODER])[
            "_valid_real_normal3d"
        ]
        if valid_calls != (1 if self.role == "target" else 2):
            raise AssertionError((self.role, "valid_real_normal3d relocation count", valid_calls))
        affect_decoder_targets = [
            item["target"]
            for item in self.relocations[AFFECT]
            if item["target"] in (TARGET_DECODER, DECODER)
        ]
        expected = [TARGET_DECODER if self.role == "target" else DECODER]
        if affect_decoder_targets != expected:
            raise AssertionError((self.role, "public/private edge", affect_decoder_targets, expected))

    def guards(self) -> dict:
        return {
            "sections": self.section_checks,
            "owners": self.function_owner,
            "relocations": self.relocations,
            "constant_sections": self.constants.records,
            "target_first_validity_boundary": (
                "one out-of-line valid_real_normal3d relocation in target; two in candidate"
            ),
        }

    def run(
        self,
        scenario: dict,
        fpcw: int,
        seed: int,
        overrides: dict[str, bytes] | None = None,
        validate: bool = True,
    ) -> dict:
        machine = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        for address in CODE_ADDRESSES.values():
            machine.mem_map(address, 0x1000)
        machine.mem_map(STUB_BASE, 0x10000)
        machine.mem_map(RDATA, RDATA_SIZE)
        machine.mem_map(GLOBALS, 0x1000)
        for address in (ACTOR, SPEC, DIRECTION, PROP, OBJECT, TEMPORARY):
            machine.mem_map(address, 0x1000)
        machine.mem_map(STACK, STACK_SIZE)
        selected_raw = dict(self.runtime_raw)
        if overrides:
            selected_raw.update(overrides)
        for name, raw in selected_raw.items():
            machine.mem_write(CODE_ADDRESSES[name], raw)
        machine.mem_write(STUB_BASE, b"\xC3" * 0x10000)
        for address, raw in self.constants.blocks.items():
            machine.mem_write(address, raw)
        machine.mem_write(ACTOR_DATA_SLOT, p32(ACTOR_DATA_VALUE))
        machine.mem_write(PROP_DATA_SLOT, p32(PROP_DATA_VALUE))

        actor_handle = (0xA5100000 | seed) & 0xFFFFFFFF
        prop_handle = (0xB6200000 | seed) & 0xFFFFFFFF
        object_handle = (0xC7300000 | seed) & 0xFFFFFFFF
        if scenario.get("target_prop") == "prop":
            target_prop_handle = prop_handle
        else:
            target_prop_handle = int(scenario.get("target_prop", NONE)) & 0xFFFFFFFF

        actor_before = bytearray(seeded_bytes(ACTOR_SIZE, seed))
        write_vector(actor_before, HEAD_POSITION, tuple(scenario.get("head", (1.0, 2.0, 3.0))))
        write32(actor_before, TARGET_PROP_INDEX, target_prop_handle)
        write16(actor_before, DANGER_TYPE, scenario.get("danger_type", 0))
        write_vector(actor_before, DANGER_POSITION, tuple(scenario.get("danger_position", (4.0, 6.0, 3.0))))
        write16(actor_before, PRIMARY_PRIORITY, scenario.get("priority", 0))
        write16(actor_before, PRIMARY_DIRECTION, scenario["direction_type"] or 0)
        primary_value = (
            scenario.get("spec_point", (1.0, 2.0, 3.0))
            if scenario["direction_type"] == DIRECTION_POINT
            else scenario.get("spec_vector", (0.6, 0.8, 0.0))
        )
        write_vector(actor_before, PRIMARY_DIRECTION + 4, tuple(primary_value))
        actor_before[MOVING] = scenario.get("moving", 0) & 0xFF
        actor_before[FORCED_BY_AIMING] = 0x7D
        write_vector(actor_before, MOVING_POINT, tuple(scenario.get("movement_point", (11.0, 22.0, 33.0))))
        write_vector(actor_before, MOVING_VECTOR, tuple(scenario.get("movement_vector", (3.0, 4.0, 0.0))))
        write_vector(actor_before, FORCED_DIRECTION, (9.0, 8.0, 7.0))
        write16(actor_before, FIRE_STATE, scenario.get("fire_state", 0))
        write16(actor_before, FIRE_TARGET_TYPE, 5)
        actor_before[AIMING_AT_FIRE_TARGET] = scenario.get("aiming", 0) & 0xFF
        write_vector(actor_before, FIRE_AIM_VECTOR, tuple(scenario.get("fire_vector", (0.0, 0.0, 1.0))))
        write_vector(actor_before, BURST_AIM_VECTOR, tuple(scenario.get("burst_vector", (0.6, 0.8, 0.0))))

        spec_before = bytearray(seeded_bytes(SMALL_SIZE, seed + 17))
        write16(spec_before, 0, scenario["direction_type"] or 0)
        spec_value = (
            scenario.get("spec_point", (4.0, 6.0, 3.0))
            if scenario["direction_type"] == DIRECTION_POINT
            else scenario.get("spec_vector", (0.6, 0.8, 0.0))
        )
        if scenario["direction_type"] in (DIRECTION_PROP, DIRECTION_OBJECT):
            write32(
                spec_before,
                4,
                prop_handle if scenario["direction_type"] == DIRECTION_PROP else object_handle,
            )
        else:
            write_vector(spec_before, 4, tuple(spec_value))

        direction_before = seeded_bytes(SMALL_SIZE, seed + 31)
        prop_before = bytearray(seeded_bytes(SMALL_SIZE, seed + 43))
        write32(prop_before, PROP_OWNER, 0xD8400000 | seed)
        write_vector(prop_before, PROP_CENTER, tuple(scenario.get("prop_center", (4.0, 6.0, 3.0))))
        write_vector(prop_before, PROP_HEAD, tuple(scenario.get("prop_head", (4.0, 6.0, 3.0))))
        object_before = bytearray(seeded_bytes(SMALL_SIZE, seed + 59))
        write16(object_before, OBJECT_TYPE, scenario.get("object_type", 0))
        temporary_before = seeded_bytes(TEMPORARY_SIZE, seed + 71)
        globals_before = bytes(machine.mem_read(GLOBALS, 0x1000))
        actor_page_before = bytearray(seeded_bytes(PAGE_SIZE, seed + 101))
        actor_page_before[:ACTOR_SIZE] = actor_before
        spec_page_before = bytearray(seeded_bytes(PAGE_SIZE, seed + 103))
        spec_page_before[:SMALL_SIZE] = spec_before
        direction_page_before = bytearray(seeded_bytes(PAGE_SIZE, seed + 107))
        direction_page_before[:SMALL_SIZE] = direction_before
        prop_page_before = bytearray(seeded_bytes(PAGE_SIZE, seed + 109))
        prop_page_before[:SMALL_SIZE] = prop_before
        object_page_before = bytearray(seeded_bytes(PAGE_SIZE, seed + 113))
        object_page_before[:SMALL_SIZE] = object_before
        machine.mem_write(ACTOR, bytes(actor_page_before))
        machine.mem_write(SPEC, bytes(spec_page_before))
        machine.mem_write(DIRECTION, bytes(direction_page_before))
        machine.mem_write(PROP, bytes(prop_page_before))
        machine.mem_write(OBJECT, bytes(object_page_before))
        machine.mem_write(TEMPORARY, temporary_before)

        stack_before = bytearray(seeded_bytes(STACK_SIZE, seed + 89))
        write32(stack_before, ENTRY_SP - STACK, STOP)
        write32(stack_before, ENTRY_SP - STACK + 4, actor_handle)
        caller_before = bytes(
            stack_before[ENTRY_SP - STACK : ENTRY_SP - STACK + CALLER_FRAME_SIZE]
        )
        machine.mem_write(STACK, bytes(stack_before))

        for name, register in NONVOLATILE.items():
            machine.reg_write(register, SAVED[name])
        machine.reg_write(UC_X86_REG_EAX, 0x89ABCDEF)
        machine.reg_write(UC_X86_REG_ECX, 0x76543210)
        machine.reg_write(UC_X86_REG_EDX, 0x0BADF00D)
        machine.reg_write(UC_X86_REG_ESP, ENTRY_SP)
        machine.reg_write(UC_X86_REG_EFLAGS, 0x202 | (0x400 if seed & 1 else 0))
        machine.reg_write(UC_X86_REG_FPCW, fpcw)
        machine.reg_write(UC_X86_REG_FPSW, 0)
        machine.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        initial_direction_flag = bool(machine.reg_read(UC_X86_REG_EFLAGS) & 0x400)
        initial_fptag = machine.reg_read(UC_X86_REG_FPTAG) & 0xFFFF
        if scenario["entry"] == "decoder":
            machine.reg_write(UC_X86_REG_EBX, 0 if scenario["null_spec"] else SPEC)
            machine.reg_write(UC_X86_REG_EDI, 0 if scenario["null_direction"] else DIRECTION)

        api_calls: list[dict] = []
        math_calls: list[dict] = []
        decoder_entries: list[dict] = []
        writes: list[dict] = []
        invalid_accesses: list[dict] = []
        terminated = False
        try_index = 0

        def argument(index: int) -> int:
            return r32(machine, machine.reg_read(UC_X86_REG_ESP) + 4 + 4 * index)

        def pointer_label(address: int) -> str:
            regions = (
                (ACTOR, PAGE_SIZE, "actor"),
                (SPEC, PAGE_SIZE, "specification"),
                (DIRECTION, PAGE_SIZE, "direction"),
                (PROP, PAGE_SIZE, "prop"),
                (OBJECT, PAGE_SIZE, "object"),
                (TEMPORARY, TEMPORARY_SIZE, "temporary"),
                (STACK, STACK_SIZE, "stack"),
                (GLOBALS, 0x1000, "globals"),
                (RDATA, RDATA_SIZE, "rdata"),
            )
            if address == 0:
                return "null"
            for base, size, label in regions:
                if base <= address < base + size:
                    offset = address - base
                    return label if offset == 0 else f"{label}+0x{offset:x}"
            return f"0x{address:08x}"

        def cstring(address: int, limit: int = 4096) -> str | None:
            if address == 0:
                return None
            raw = bytes(machine.mem_read(address, limit))
            end = raw.find(b"\0")
            if end < 0:
                raise AssertionError((self.role, scenario["name"], "unterminated cstring", hex(address)))
            return raw[:end].decode("ascii")

        def return_from_stub(value: int = 0) -> None:
            esp = machine.reg_read(UC_X86_REG_ESP)
            return_address = r32(machine, esp)
            machine.reg_write(UC_X86_REG_EAX, value & 0xFFFFFFFF)
            machine.reg_write(UC_X86_REG_ESP, esp + 4)
            machine.reg_write(UC_X86_REG_EIP, return_address)

        def decode_format(format_address: int) -> tuple[str, list[dict], str]:
            pattern = cstring(format_address)
            assert pattern is not None
            offset_words = 2
            typed: list[dict] = []
            rendered_parts: list[str] = []
            cursor = 0
            for match in FORMAT_RE.finditer(pattern):
                rendered_parts.append(pattern[cursor : match.start()])
                kind = match.group("kind")
                width = int(match.group("width") or 0)
                precision = int(match.group("precision") or 6)
                if kind == "f":
                    low = argument(offset_words)
                    high = argument(offset_words + 1)
                    value = struct.unpack("<d", p32(low) + p32(high))[0]
                    typed.append(
                        {
                            "kind": "double",
                            "words": [low, high],
                            "value": float_token(value),
                        }
                    )
                    rendered_parts.append(format(value, f".{precision}f"))
                    offset_words += 2
                elif kind == "s":
                    pointer = argument(offset_words)
                    text = cstring(pointer)
                    typed.append(
                        {
                            "kind": "string",
                            "pointer": pointer_label(pointer),
                            "value": text,
                        }
                    )
                    rendered_parts.append("(null)" if text is None else text)
                    offset_words += 1
                elif kind == "d":
                    raw_value = argument(offset_words)
                    value = s32(raw_value)
                    typed.append({"kind": "signed", "raw": raw_value, "value": value})
                    rendered_parts.append(str(value))
                    offset_words += 1
                else:
                    raw_value = argument(offset_words)
                    typed.append({"kind": "unsigned", "raw": raw_value, "value": raw_value})
                    text = format(raw_value, "X" if kind == "X" else "x")
                    if width:
                        fill = "0" if "0" in match.group("flags") else " "
                        text = text.rjust(width, fill)
                    rendered_parts.append(text)
                    offset_words += 1
                cursor = match.end()
            rendered_parts.append(pattern[cursor:])
            return pattern, typed, "".join(rendered_parts)

        stub_by_address = {address: name for name, address in STUB_ADDRESSES.items()}
        helper_by_address = {CODE_ADDRESSES[name]: name for name in HELPERS}

        def on_code(_machine, address, _size, _user):
            nonlocal try_index, terminated
            if address == CODE_ADDRESSES[DECODER]:
                decoder_entries.append(
                    {
                        "ebx_specification": pointer_label(machine.reg_read(UC_X86_REG_EBX)),
                        "edi_direction": pointer_label(machine.reg_read(UC_X86_REG_EDI)),
                        "actor_index": argument(0),
                    }
                )
                return
            helper_name = helper_by_address.get(address)
            if helper_name is not None:
                count = {"_normalize3d": 1, "_valid_real_normal3d": 1, "_vector_from_points3d": 3}.get(helper_name, 0)
                args = [pointer_label(argument(index)) for index in range(count)]
                math_calls.append({"name": helper_name, "args": args})
                return
            name = stub_by_address.get(address)
            if name is None:
                return
            if name == "_datum_get":
                data, handle = argument(0), argument(1)
                if data == ACTOR_DATA_VALUE and handle == actor_handle:
                    result, data_label = ACTOR, "actor_data"
                elif data == PROP_DATA_VALUE and handle == prop_handle:
                    result, data_label = PROP, "prop_data"
                else:
                    raise AssertionError((self.role, scenario["name"], name, hex(data), hex(handle)))
                api_calls.append({"name": name, "data": data_label, "handle": handle})
                return_from_stub(result)
            elif name == "_datum_try_and_get":
                data, handle = argument(0), argument(1)
                if data != PROP_DATA_VALUE or handle != prop_handle:
                    raise AssertionError((self.role, scenario["name"], name, hex(data), hex(handle)))
                sequence = scenario.get("prop_try_sequence", ["prop"])
                item = sequence[try_index] if try_index < len(sequence) else sequence[-1]
                try_index += 1
                result = PROP if item == "prop" else 0
                api_calls.append(
                    {"name": name, "data": "prop_data", "handle": handle, "result": pointer_label(result)}
                )
                return_from_stub(result)
            elif name == "_object_try_and_get_and_verify_type":
                handle, mask = argument(0), argument(1)
                if handle != object_handle or mask != 0xFFFFFFFF:
                    raise AssertionError((self.role, scenario["name"], name, hex(handle), hex(mask)))
                result = OBJECT if scenario.get("object_present", True) else 0
                api_calls.append({"name": name, "handle": handle, "mask": mask, "result": pointer_label(result)})
                return_from_stub(result)
            elif name in ("_unit_get_head_position", "_object_get_origin"):
                handle, output = argument(0), argument(1)
                if handle != object_handle or not (STACK <= output <= STACK + STACK_SIZE - 12):
                    raise AssertionError((self.role, scenario["name"], name, hex(handle), hex(output)))
                value = tuple(scenario.get("object_point", (4.0, 6.0, 3.0)))
                machine.mem_write(output, b"".join(struct.pack("<f", f32(component)) for component in value))
                api_calls.append({"name": name, "handle": handle, "output": pointer_label(output), "value": list(value)})
                return_from_stub(0)
            elif name == "_actor_path_has_path":
                handle = argument(0)
                if handle != actor_handle:
                    raise AssertionError((self.role, scenario["name"], name, hex(handle)))
                value = 1 if scenario.get("path_result", False) else 0
                api_calls.append({"name": name, "actor_index": handle, "result": value})
                return_from_stub(value)
            elif name in ("_sprintf", "_csprintf"):
                destination, format_address = argument(0), argument(1)
                if destination != TEMPORARY:
                    raise AssertionError((self.role, scenario["name"], name, pointer_label(destination)))
                pattern, typed, rendered = decode_format(format_address)
                encoded = rendered.encode("ascii") + b"\0"
                if len(encoded) > TEMPORARY_SIZE:
                    raise AssertionError((scenario["name"], "modeled format overflow", len(encoded)))
                machine.mem_write(destination, encoded)
                api_calls.append(
                    {
                        "name": name,
                        "destination": "temporary",
                        "format": pattern,
                        "typed_arguments": typed,
                        "modeled_rendering": rendered,
                    }
                )
                return_from_stub(destination if name == "_csprintf" else len(rendered))
            elif name == "_display_assert":
                info, file_address, line, fatal = (argument(index) for index in range(4))
                api_calls.append(
                    {
                        "name": name,
                        "info_pointer": pointer_label(info),
                        "info": cstring(info),
                        "file": cstring(file_address),
                        "line": line,
                        "fatal": fatal,
                    }
                )
                return_from_stub(0)
            elif name == "_system_exit":
                status = s32(argument(0))
                api_calls.append({"name": name, "status": status})
                terminated = True
                machine.reg_write(UC_X86_REG_EIP, STOP)
                machine.emu_stop()
            else:
                raise AssertionError(name)

        def on_write(_machine, _access, address, size, value, _user):
            for base, bound, label in (
                (ACTOR, PAGE_SIZE, "actor"),
                (SPEC, PAGE_SIZE, "specification"),
                (DIRECTION, PAGE_SIZE, "direction"),
                (PROP, PAGE_SIZE, "prop"),
                (OBJECT, PAGE_SIZE, "object"),
                (TEMPORARY, TEMPORARY_SIZE, "temporary"),
                (GLOBALS, 0x1000, "globals"),
            ):
                if base <= address < base + bound:
                    writes.append(
                        {
                            "region": label,
                            "offset": address - base,
                            "size": size,
                            "value": value & ((1 << min(size * 8, 64)) - 1),
                        }
                    )
                    return

        def on_invalid(_machine, access, address, size, value, _user):
            invalid_accesses.append(
                {"access": access, "address": address, "size": size, "value": value}
            )
            return False

        machine.hook_add(u.UC_HOOK_CODE, on_code)
        machine.hook_add(u.UC_HOOK_MEM_WRITE, on_write)
        machine.hook_add(u.UC_HOOK_MEM_INVALID, on_invalid)
        entry = CODE_ADDRESSES[DECODER if scenario["entry"] == "decoder" else AFFECT]
        try:
            machine.emu_start(entry, STOP, count=100000)
        except u.UcError as error:
            raise AssertionError(
                (
                    self.role,
                    scenario["name"],
                    "emulation error",
                    repr(error),
                    hex(machine.reg_read(UC_X86_REG_EIP)),
                    invalid_accesses,
                )
            ) from error

        buffers_before = {
            "actor": bytes(actor_page_before),
            "specification": bytes(spec_page_before),
            "direction": bytes(direction_page_before),
            "prop": bytes(prop_page_before),
            "object": bytes(object_page_before),
            "temporary": temporary_before,
            "globals": globals_before,
        }
        buffers_after = {
            "actor": bytes(machine.mem_read(ACTOR, PAGE_SIZE)),
            "specification": bytes(machine.mem_read(SPEC, PAGE_SIZE)),
            "direction": bytes(machine.mem_read(DIRECTION, PAGE_SIZE)),
            "prop": bytes(machine.mem_read(PROP, PAGE_SIZE)),
            "object": bytes(machine.mem_read(OBJECT, PAGE_SIZE)),
            "temporary": bytes(machine.mem_read(TEMPORARY, TEMPORARY_SIZE)),
            "globals": bytes(machine.mem_read(GLOBALS, 0x1000)),
        }
        changes = {
            name: changed_offsets(buffers_before[name], buffers_after[name])
            for name in buffers_before
        }
        nonvolatile = {
            name: machine.reg_read(register) & 0xFFFFFFFF
            for name, register in NONVOLATILE.items()
        }
        fpsw = machine.reg_read(UC_X86_REG_FPSW) & 0xFFFF
        fptag = machine.reg_read(UC_X86_REG_FPTAG) & 0xFFFF
        final_direction_flag = bool(machine.reg_read(UC_X86_REG_EFLAGS) & 0x400)
        result = {
            "role": self.role,
            "case": scenario["name"],
            "entry": scenario["entry"],
            "fpcw_requested": fpcw,
            "fpcw_final": machine.reg_read(UC_X86_REG_FPCW) & 0xFFFF,
            "fpsw_final": fpsw,
            "fptag_initial": initial_fptag,
            "fptag_final": fptag,
            "x87_top": (fpsw >> 11) & 7,
            "direction_flag_initial": initial_direction_flag,
            "direction_flag_final": final_direction_flag,
            "terminated_by_system_exit": terminated,
            "eip": machine.reg_read(UC_X86_REG_EIP) & 0xFFFFFFFF,
            "esp": machine.reg_read(UC_X86_REG_ESP) & 0xFFFFFFFF,
            "al": machine.reg_read(UC_X86_REG_EAX) & 0xFF,
            "nonvolatile": nonvolatile,
            "caller_frame_exact": bytes(machine.mem_read(ENTRY_SP, CALLER_FRAME_SIZE)) == caller_before,
            "api_calls": api_calls,
            "math_calls": math_calls,
            "decoder_entries": decoder_entries,
            "writes": writes,
            "invalid_accesses": invalid_accesses,
            "changes": changes,
            "buffer_hashes": {
                name: {"before": sha256_bytes(buffers_before[name]), "after": sha256_bytes(buffers_after[name])}
                for name in buffers_before
            },
            "direction_before_bits": [
                struct.unpack_from("<I", buffers_before["direction"], index * 4)[0]
                for index in range(3)
            ],
            "direction_after_bits": [r32(machine, DIRECTION + index * 4) for index in range(3)],
            "primary_priority": struct.unpack_from("<h", buffers_after["actor"], PRIMARY_PRIORITY)[0],
            "forced_by_aiming": buffers_after["actor"][FORCED_BY_AIMING],
            "forced_direction_bits": [
                struct.unpack_from("<I", buffers_after["actor"], FORCED_DIRECTION + index * 4)[0]
                for index in range(3)
            ],
        }
        if validate:
            self._validate_run(result, scenario, actor_handle, fpcw)
        return result

    def _validate_run(self, result: dict, scenario: dict, actor_handle: int, fpcw: int) -> None:
        api_names = [call["name"] for call in result["api_calls"]]
        math_names = [call["name"] for call in result["math_calls"]]
        displays = [
            [
                "temporary" if call["info_pointer"].startswith("temporary") else call["info"],
                call["line"],
                call["fatal"],
            ]
            for call in result["api_calls"]
            if call["name"] == "_display_assert"
        ]
        expected_valid = (1 if self.role == "target" else 2) if scenario["decoder_result_true"] else 0
        expected_math = Counter(
            {
                "_vector_from_points3d": scenario["vector_calls"],
                "_normalize3d": scenario["normalize_calls"],
                "_valid_real_normal3d": expected_valid,
            }
        )
        expected_math += Counter()
        actual_math = Counter(math_names)
        checks = {
            "api_order": api_names == scenario["expected_api"],
            "display_diagnostics": displays == scenario["expected_display"],
            "termination": result["terminated_by_system_exit"] == scenario["terminated"],
            "fpcw_preserved": result["fpcw_final"] == fpcw,
            "x87_top_balanced": result["x87_top"] == 0,
            "x87_tag_restored": result["fptag_final"] == result["fptag_initial"],
            "direction_flag_preserved": result["direction_flag_final"]
            == result["direction_flag_initial"],
            "no_unmapped_access": not result["invalid_accesses"],
            "math_helper_counts": actual_math == expected_math,
            "only_relevant_helpers_execute": set(math_names) <= EXECUTED_HELPERS,
            "globals_unchanged": not result["changes"]["globals"],
            "specification_unchanged": not result["changes"]["specification"],
            "prop_unchanged": not result["changes"]["prop"],
            "object_unchanged": not result["changes"]["object"],
            "caller_frame_exact": result["caller_frame_exact"],
            "direction_page_tail_unchanged": set(result["changes"]["direction"])
            <= set(range(12)),
            "temporary_page_unchanged_without_format_call": (
                True
                if any(name in ("_sprintf", "_csprintf") for name in api_names)
                else not result["changes"]["temporary"]
            ),
        }
        if scenario["terminated"]:
            system_calls = [call for call in result["api_calls"] if call["name"] == "_system_exit"]
            checks["fatal_system_exit_minus_one"] = system_calls == [{"name": "_system_exit", "status": -1}]
            checks["fatal_eip_stopped"] = result["eip"] == STOP
        else:
            checks["normal_eip_stop"] = result["eip"] == STOP
            checks["cdecl_return_esp"] = result["esp"] == ENTRY_SP + 4
        if scenario["entry"] == "decoder":
            expected_spec = "null" if scenario["null_spec"] else "specification"
            expected_direction = "null" if scenario["null_direction"] else "direction"
            checks["private_entry_abi"] = result["decoder_entries"] == [
                {
                    "ebx_specification": expected_spec,
                    "edi_direction": expected_direction,
                    "actor_index": actor_handle,
                }
            ]
            checks["decoder_actor_read_only"] = not result["changes"]["actor"]
            checks["decoder_nonvolatile"] = (
                result["nonvolatile"]["ebp"] == SAVED["ebp"]
                and result["nonvolatile"]["esi"] == SAVED["esi"]
                if not scenario["terminated"]
                else True
            )
            checks["decoder_write_envelope"] = {
                item["region"] for item in result["writes"]
            } <= {"direction", "temporary"}
            if not scenario["terminated"]:
                checks["decoder_private_registers_preserved"] = (
                    result["nonvolatile"]["ebx"] == (0 if scenario["null_spec"] else SPEC)
                    and result["nonvolatile"]["edi"] == (0 if scenario["null_direction"] else DIRECTION)
                )
                checks["decoder_al"] = result["al"] == scenario["expected_al"]
        else:
            expected_entries = 1 if scenario["nested_decoder"] else 0
            checks["public_real_decoder_edge"] = len(result["decoder_entries"]) == expected_entries
            if expected_entries:
                checks["public_private_abi"] = result["decoder_entries"] == [
                    {
                        "ebx_specification": "actor+0x3ec",
                        "edi_direction": "actor+0x524",
                        "actor_index": actor_handle,
                    }
                ]
            checks["public_nonvolatile_preserved"] = result["nonvolatile"] == SAVED
            checks["public_external_direction_unchanged"] = not result["changes"]["direction"]
            checks["public_write_envelope"] = set(result["changes"]["actor"]) <= (
                {PRIMARY_PRIORITY, PRIMARY_PRIORITY + 1, FORCED_BY_AIMING}
                | set(range(FORCED_DIRECTION, FORCED_DIRECTION + 12))
            )
            checks["public_priority"] = result["primary_priority"] == scenario["expected_priority"]
            checks["public_forced_flag"] = result["forced_by_aiming"] == scenario["expected_flag"]

        failed = [name for name, passed in checks.items() if not passed]
        result["checks"] = checks
        if failed:
            raise AssertionError(
                (
                    self.role,
                    scenario["name"],
                    hex(fpcw),
                    "failed runtime checks",
                    failed,
                    {"api": api_names, "math": math_names, "display": displays},
                )
            )

        if scenario["entry"] == "decoder":
            expected = scenario["expected_direction"]
            actual_bits = result["direction_after_bits"]
            if expected == "unchanged":
                passed = actual_bits == result["direction_before_bits"]
            elif expected == "nonfinite":
                passed = any(not math.isfinite(bits_f32(value)) for value in actual_bits)
            else:
                passed = all(
                    math.isclose(bits_f32(bits), f32(component), rel_tol=2e-6, abs_tol=1e-7)
                    for bits, component in zip(actual_bits, expected)
                )
            result["checks"]["direction_result"] = passed
            if not passed:
                raise AssertionError((self.role, scenario["name"], "direction result", actual_bits, expected))
        else:
            expected = scenario["expected_forced_direction"]
            actual_bits = result["forced_direction_bits"]
            initial_bits = [f32_bits(value) for value in (9.0, 8.0, 7.0)]
            if expected == "unchanged":
                passed = actual_bits == initial_bits
            else:
                passed = all(
                    math.isclose(bits_f32(bits), f32(component), rel_tol=2e-6, abs_tol=1e-7)
                    for bits, component in zip(actual_bits, expected)
                )
            result["checks"]["forced_direction_result"] = passed
            if not passed:
                raise AssertionError((self.role, scenario["name"], "forced direction", actual_bits, expected))


def typed_without_machine_words(value):
    if isinstance(value, list):
        return [typed_without_machine_words(item) for item in value]
    if isinstance(value, dict):
        return {
            key: typed_without_machine_words(item)
            for key, item in value.items()
            if key not in ("words", "modeled_rendering", "output", "pointer")
        }
    return value


def semantic_signature(run: dict) -> dict:
    after_hashes = {
        name: hashes["after"]
        for name, hashes in run["buffer_hashes"].items()
    }
    result = {
        "entry": run["entry"],
        "fpcw": run["fpcw_final"],
        "fptag": run["fptag_final"],
        "x87_top": run["x87_top"],
        "direction_flag": run["direction_flag_final"],
        "terminated": run["terminated_by_system_exit"],
        "api_calls": typed_without_machine_words(run["api_calls"]),
        "decoder_entries": run["decoder_entries"],
        "after_hashes": after_hashes,
        "changes": run["changes"],
        "direction_bits": run["direction_after_bits"],
        "priority": run["primary_priority"],
        "forced_flag": run["forced_by_aiming"],
        "forced_direction_bits": run["forced_direction_bits"],
    }
    if not run["terminated_by_system_exit"] and run["entry"] == "decoder":
        result["al"] = run["al"]
    return result


def exact_candidate_signature(run: dict) -> dict:
    return {
        key: value
        for key, value in run.items()
        if key not in ("role", "checks")
    }


def validate_diagnostic_details(run: dict, scenario: dict, seed: int) -> dict:
    display_calls = [call for call in run["api_calls"] if call["name"] == "_display_assert"]
    checks = {
        "source_path_all_displays": all(
            call["file"] == "c:\\halo\\SOURCE\\ai\\actor_looking.c"
            for call in display_calls
        ),
        "expected_line_fatal_pairs": [
            [call["line"], call["fatal"]] for call in display_calls
        ]
        == [[item[1], item[2]] for item in scenario["expected_display"]],
    }
    if [[item[1], item[2]] for item in scenario["expected_display"]] == [[526, 0], [529, 1]]:
        sprintf_calls = [call for call in run["api_calls"] if call["name"] == "_sprintf"]
        csprintf_calls = [call for call in run["api_calls"] if call["name"] == "_csprintf"]
        checks.update(
            {
                "one_primary_diagnostic_format": len(sprintf_calls) == 1,
                "one_final_assert_format": len(csprintf_calls) == 1,
                "final_assert_format": bool(csprintf_calls)
                and csprintf_calls[0]["format"]
                == "%s: assert_valid_real_normal3d(%f, %f, %f)",
                "final_assert_typed_arity": bool(csprintf_calls)
                and [item["kind"] for item in csprintf_calls[0]["typed_arguments"]]
                == ["string", "double", "double", "double"],
                "diagnostic_sequence_526_then_529": [call["line"] for call in display_calls]
                == [526, 529],
            }
        )
    if scenario["name"] == "movement_infinite_diagnostic":
        call = next(call for call in run["api_calls"] if call["name"] == "_sprintf")
        typed = call["typed_arguments"]
        checks.update(
            {
                "movement_format_selected": "%smoving" in call["format"],
                "movement_typed_arity": len(typed) == 8,
                "movement_status_empty": typed[1]["kind"] == "string" and typed[1]["value"] == "",
                "movement_point_x_z_z": [typed[index]["value"] for index in (2, 3, 4)]
                == [float_token(11.0), float_token(33.0), float_token(33.0)],
            }
        )
    if scenario["name"] == "prop_infinite_invalid_second_lookup":
        call = next(call for call in run["api_calls"] if call["name"] == "_sprintf")
        typed = call["typed_arguments"]
        actor_handle = (0xA5100000 | seed) & 0xFFFFFFFF
        checks.update(
            {
                "prop_format_selected": "prop 0x%08X%s" in call["format"],
                "prop_typed_arity": len(typed) == 5,
                "prop_invalid_status": typed[2]["kind"] == "string"
                and typed[2]["value"] == " (invalid)",
                "prop_null_owner_zero": typed[3]["kind"] == "unsigned"
                and typed[3]["raw"] == 0,
                "prop_actor_index_forwarded": typed[4]["kind"] == "unsigned"
                and typed[4]["raw"] == actor_handle,
            }
        )
    failed = [name for name, passed in checks.items() if not passed]
    if failed:
        raise AssertionError((run["role"], scenario["name"], "diagnostic checks", failed))
    return checks


def compare_roles(target: dict, candidate: dict, scenario: dict) -> dict:
    target_signature = semantic_signature(target)
    candidate_signature = semantic_signature(candidate)
    if target_signature != candidate_signature:
        differing = sorted(
            key
            for key in set(target_signature) | set(candidate_signature)
            if target_signature.get(key) != candidate_signature.get(key)
        )
        raise AssertionError(
            (scenario["name"], hex(target["fpcw_requested"]), "semantic differential", differing)
        )
    target_math = Counter(call["name"] for call in target["math_calls"])
    candidate_math = Counter(call["name"] for call in candidate["math_calls"])
    return {
        "semantic_signature_equal": True,
        "target_fpsw": target["fpsw_final"],
        "candidate_fpsw": candidate["fpsw_final"],
        "fpsw_equal": target["fpsw_final"] == candidate["fpsw_final"],
        "target_math_calls": dict(target_math),
        "candidate_math_calls": dict(candidate_math),
        "expected_candidate_extra_validity_call": (
            candidate_math["_valid_real_normal3d"]
            - target_math["_valid_real_normal3d"]
            == (1 if scenario["decoder_result_true"] else 0)
        ),
    }


def scenario_by_name(name: str) -> dict:
    return next(item for item in cases() if item["name"] == name)


def mutated(raw: bytes, offset: int, before: bytes, after: bytes) -> bytes:
    if raw[offset : offset + len(before)] != before:
        raise AssertionError(("negative-control preimage", hex(offset), raw[offset : offset + len(before)].hex(), before.hex()))
    if len(before) != len(after):
        raise AssertionError("negative-control mutation must preserve length")
    result = bytearray(raw)
    result[offset : offset + len(after)] = after
    return bytes(result)


def run_negative_controls(actual: RoleImage) -> dict:
    results = {}

    threshold_case = scenario_by_name("affect_vector_priority_3")
    threshold_raw = actual.runtime_raw[AFFECT]
    threshold_pattern = bytes.fromhex("66 83 be e8 03 00 00 03")
    threshold_offset = threshold_raw.find(threshold_pattern)
    if threshold_offset < 0 or threshold_raw.find(threshold_pattern, threshold_offset + 1) >= 0:
        raise AssertionError(("threshold mutation pattern cardinality", threshold_offset))
    baseline = actual.run(threshold_case, 0x037F, 0xD1)
    changed = mutated(
        threshold_raw,
        threshold_offset + 7,
        b"\x03",
        b"\x04",
    )
    mutant = actual.run(
        threshold_case,
        0x037F,
        0xD1,
        overrides={AFFECT: changed},
        validate=False,
    )
    detected = (
        baseline["forced_by_aiming"] == 1
        and mutant["forced_by_aiming"] == 0
        and len(baseline["decoder_entries"]) == 1
        and not mutant["decoder_entries"]
        and semantic_signature(baseline) != semantic_signature(mutant)
    )
    results["priority_threshold_3_to_4"] = {
        "function": AFFECT,
        "offset": threshold_offset + 7,
        "before": "03",
        "after": "04",
        "case": threshold_case["name"],
        "detected": detected,
        "observation": {
            "baseline_flag": baseline["forced_by_aiming"],
            "mutant_flag": mutant["forced_by_aiming"],
            "baseline_decoder_entries": len(baseline["decoder_entries"]),
            "mutant_decoder_entries": len(mutant["decoder_entries"]),
        },
    }

    movement_case = scenario_by_name("movement_infinite_diagnostic")
    decoder_raw = actual.runtime_raw[DECODER]
    movement_offset = 0x2F7
    before_instruction = bytes.fromhex("d9 86 14 05 00 00")
    after_instruction = bytes.fromhex("d9 86 10 05 00 00")
    baseline = actual.run(movement_case, 0x037F, 0xD2)
    changed = mutated(decoder_raw, movement_offset, before_instruction, after_instruction)
    mutant = actual.run(
        movement_case,
        0x037F,
        0xD2,
        overrides={DECODER: changed},
        validate=False,
    )
    baseline_typed = next(
        call["typed_arguments"] for call in baseline["api_calls"] if call["name"] == "_sprintf"
    )
    mutant_typed = next(
        call["typed_arguments"] for call in mutant["api_calls"] if call["name"] == "_sprintf"
    )
    baseline_point = [baseline_typed[index]["value"] for index in (2, 3, 4)]
    mutant_point = [mutant_typed[index]["value"] for index in (2, 3, 4)]
    detected = baseline_point == [
        float_token(11.0),
        float_token(33.0),
        float_token(33.0),
    ] and mutant_point == [
        float_token(11.0),
        float_token(22.0),
        float_token(33.0),
    ]
    results["movement_second_z_to_y"] = {
        "function": DECODER,
        "offset": movement_offset,
        "before": before_instruction.hex(),
        "after": after_instruction.hex(),
        "case": movement_case["name"],
        "detected": detected,
        "observation": {"baseline_point": baseline_point, "mutant_point": mutant_point},
    }

    prop_case = scenario_by_name("prop_infinite_invalid_second_lookup")
    owner_offset = 0x34C
    baseline = actual.run(prop_case, 0x037F, 0xD3)
    changed = mutated(decoder_raw, owner_offset, bytes.fromhex("33 c0"), bytes.fromhex("40 90"))
    mutant = actual.run(
        prop_case,
        0x037F,
        0xD3,
        overrides={DECODER: changed},
        validate=False,
    )
    baseline_typed = next(
        call["typed_arguments"] for call in baseline["api_calls"] if call["name"] == "_sprintf"
    )
    mutant_typed = next(
        call["typed_arguments"] for call in mutant["api_calls"] if call["name"] == "_sprintf"
    )
    baseline_owner = baseline_typed[3]["raw"]
    mutant_owner = mutant_typed[3]["raw"]
    detected = baseline_owner == 0 and mutant_owner == 1
    results["null_prop_owner_zero_to_one"] = {
        "function": DECODER,
        "offset": owner_offset,
        "before": "33c0",
        "after": "4090",
        "case": prop_case["name"],
        "detected": detected,
        "observation": {"baseline_owner": baseline_owner, "mutant_owner": mutant_owner},
    }

    if not all(item["detected"] for item in results.values()):
        raise AssertionError(("negative controls", results))
    return {
        "pass": True,
        "mutation_scope": "in-memory copies of relocated actual-candidate bytes only",
        "controls": results,
    }


def inventory_counts(path: Path) -> dict:
    payload = json.loads(path.read_text(encoding="utf-8"))
    values = payload.values() if isinstance(payload, dict) else payload
    counts = Counter(item["status"] for item in values)
    return {"entries": len(payload), "statuses": dict(sorted(counts.items()))}


def main() -> None:
    if not __debug__:
        raise RuntimeError("refusing optimized Python: assertions are part of the oracle")
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=OUTPUT_DEFAULT)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        raise ValueError(f"refusing to overwrite output: {output}")
    if not output.is_relative_to((ROOT / "scratch").resolve()):
        raise ValueError(f"output must remain under scratch: {output}")

    pinned = {
        "target": verify_hash(TARGET, PINNED_HASHES["target"], "target"),
        "first": verify_hash(FIRST, PINNED_HASHES["first"], "first"),
        "actual": verify_hash(ACTUAL, PINNED_HASHES["actual"], "actual"),
        "before_manifest": verify_hash(BEFORE_MANIFEST, PINNED_HASHES["before_manifest"], "before_manifest"),
        "first_manifest": verify_hash(FIRST_MANIFEST, PINNED_HASHES["first_manifest"], "first_manifest"),
        "before_stable": verify_hash(BEFORE_STABLE, PINNED_HASHES["before_stable"], "before_stable"),
        "first_stable": verify_hash(FIRST_STABLE, PINNED_HASHES["first_stable"], "first_stable"),
        "cachebeta": verify_hash(CACHEBETA, PINNED_HASHES["cachebeta"], "cachebeta"),
        "target_alert": verify_hash(TARGET_ALERT, PINNED_HASHES["target_alert"], "target_alert"),
        "target_charge": verify_hash(TARGET_CHARGE, PINNED_HASHES["target_charge"], "target_charge"),
        "target_combat": verify_hash(TARGET_COMBAT, PINNED_HASHES["target_combat"], "target_combat"),
        "coff_compare": verify_hash(ROOT / "tools/coff_compare.py", PINNED_HASHES["coff_compare"], "coff_compare"),
        "coff_tools": verify_hash(ROOT / "tools/campaign/_coff_tools.py", PINNED_HASHES["coff_tools"], "coff_tools"),
        "regression_gate": verify_hash(ROOT / "tools/regression_gate.py", PINNED_HASHES["regression_gate"], "regression_gate"),
    }
    before_manifest = json.loads(BEFORE_MANIFEST.read_text(encoding="utf-8"))
    first_manifest = json.loads(FIRST_MANIFEST.read_text(encoding="utf-8"))
    manifest_checks = {
        "before_frozen_target_hash": before_manifest["frozen_files"][
            "build/split/source/ai/actor_looking.obj"
        ]["sha256"]
        == PINNED_HASHES["target"],
        "first_phase": first_manifest["phase"] == "first",
        "first_head": first_manifest["head"] == "d2c0babb171a51de60fe0cb9ebba4bedab1c6f61",
        "first_capture_sha256": first_manifest["capture_sha256"]
        == "e2e301ff34d075482fdb0a305c0988eed978583e9548ab73c3baec9745625554",
        "first_ninja_deps_sha256": first_manifest["ninja_deps_sha256"]
        == "f3e21bdbea84da8f8edfa534e4ccdbd61077fc28d1a870ee16aaa97e79c132d1",
    }
    if not all(manifest_checks.values()):
        raise AssertionError(("manifest guards", manifest_checks))

    target = RoleImage("target", TARGET, PINNED_HASHES["target"])
    first = RoleImage("first", FIRST, PINNED_HASHES["first"])
    actual = RoleImage("actual", ACTUAL, PINNED_HASHES["actual"])
    if any(first.section_raw[name] != actual.section_raw[name] for name in (DECODER, AFFECT) + HELPERS):
        raise AssertionError("first/actual function or helper raw bytes differ")
    if any(
        not (target.section_raw[name] == first.section_raw[name] == actual.section_raw[name])
        for name in HELPERS
    ):
        raise AssertionError("target/first/actual helper raw bytes differ")

    executions = []
    fpsw_differences = []
    for case_index, scenario in enumerate(cases()):
        for control_index, fpcw in enumerate(FPCW_VALUES):
            seed = case_index * len(FPCW_VALUES) + control_index + 1
            target_run = target.run(scenario, fpcw, seed)
            first_run = first.run(scenario, fpcw, seed)
            actual_run = actual.run(scenario, fpcw, seed)
            diagnostic_checks = {
                role: validate_diagnostic_details(run, scenario, seed)
                for role, run in (
                    ("target", target_run),
                    ("first", first_run),
                    ("actual", actual_run),
                )
            }
            target_first = compare_roles(target_run, first_run, scenario)
            target_actual = compare_roles(target_run, actual_run, scenario)
            if exact_candidate_signature(first_run) != exact_candidate_signature(actual_run):
                raise AssertionError((scenario["name"], hex(fpcw), "first/actual observation differs"))
            if target_first != target_actual:
                raise AssertionError((scenario["name"], hex(fpcw), "target comparison differs for actual"))
            if not target_first["fpsw_equal"]:
                fpsw_differences.append(
                    {
                        "case": scenario["name"],
                        "fpcw": fpcw,
                        "target": target_run["fpsw_final"],
                        "candidate": first_run["fpsw_final"],
                    }
                )
            executions.append(
                {
                    "case": scenario,
                    "fpcw": fpcw,
                    "target": target_run,
                    "first": first_run,
                    "actual": actual_run,
                    "diagnostic_checks": diagnostic_checks,
                    "comparison": target_first,
                }
            )

    negative_controls = run_negative_controls(actual)
    decoder_cases = [item for item in cases() if item["entry"] == "decoder"]
    affect_cases = [item for item in cases() if item["entry"] == "affect"]
    helper_hashes = {
        name: {
            "size": len(target.section_raw[name]),
            "sha256": sha256_bytes(target.section_raw[name]),
            "executed_by_corpus": name in EXECUTED_HELPERS,
            "target_owner": target.function_owner[name],
            "candidate_owner": first.function_owner[name],
            "target_selection": 1,
            "candidate_selection": 2,
        }
        for name in HELPERS
    }
    report = {
        "pass": True,
        "scope": (
            "frozen January target, first natural candidate, and frozen ordinary-Ninja actual; "
            "real private decoder and real public movement caller"
        ),
        "build_or_production_mutation": False,
        "script": {"path": str(SCRIPT), "sha256": sha256_path(SCRIPT)},
        "pinned_inputs": pinned,
        "manifest_checks": manifest_checks,
        "stable_inventory": {
            "before": inventory_counts(BEFORE_STABLE),
            "first": inventory_counts(FIRST_STABLE),
            "expected_preserved_exact_count": 6261,
        },
        "coff_guards": {
            "target": target.guards(),
            "first": first.guards(),
            "actual": actual.guards(),
            "first_actual_all_function_and_helper_raw_exact": True,
            "target_first_actual_all_nine_helper_raw_exact": True,
            "helpers": helper_hashes,
            "physical_target_helper_owner_note": (
                "January physical COMDAT owners are action_alert/action_charge/actor_combat; "
                "real_math.obj is not asserted as the selected owner"
            ),
        },
        "coverage": {
            "direct_decoder_cases": len(decoder_cases),
            "public_affect_cases": len(affect_cases),
            "case_count": len(cases()),
            "fpcw_values": list(FPCW_VALUES),
            "executions_per_role": len(executions),
            "total_real_function_executions": len(executions) * 3,
            "all_seven_direction_types": sorted(
                {item["direction_type"] for item in decoder_cases if isinstance(item["direction_type"], int) and item["direction_type"] <= 6}
            ),
            "false_and_zero_paths": True,
            "priorities_2_3_4_and_path_short_circuits": True,
            "object_unit_general_missing": True,
            "diagnostic_lines": [349, 350, 449, 526, 529],
            "adjacent_float32_validity_brackets": {
                "lower_outside": "0x3f7fdf39",
                "lower_inside": "0x3f7fdf3a",
                "upper_inside": "0x3f801061",
                "upper_outside": "0x3f801062",
                "tested_divergence": False,
            },
            "movement_diagnostic_x_z_z": True,
            "invalid_prop_owner_zero": True,
            "real_public_caller_never_stubs_decoder": True,
            "real_machine_helpers_executed": sorted(EXECUTED_HELPERS),
            "loaded_but_not_reached_helpers": sorted(set(HELPERS) - EXECUTED_HELPERS),
            "modeled_genuine_external_apis": list(APIS),
            "guard_layout": "separate 4 KiB data maps with unmapped 4 KiB gaps; stack centered in bounded 128 KiB map",
        },
        "comparison_summary": {
            "all_tested_semantic_signatures_equal": True,
            "first_actual_all_tested_observations_exact": True,
            "expected_candidate_extra_validity_call_checked": all(
                item["comparison"]["expected_candidate_extra_validity_call"] for item in executions
            ),
            "fpsw_difference_count": len(fpsw_differences),
            "fpsw_differences": fpsw_differences,
            "x87_top_tag_control_and_direction_flag_preserved_all_runs": True,
        },
        "negative_controls": negative_controls,
        "limitations": [
            "Finite authored corpus; not exhaustive input-space or floating-point proof.",
            "The target inlines the first normal-vector validity check while candidates call the real helper. Four adjacent-float32 points bracketing both 0.001 magnitude-squared limits showed no divergence, but the finite corpus does not exclude every boundary case.",
            "sprintf/csprintf text rendering is a stable Python model; machine stack words, parsed types, formats, and downstream display_assert arguments are the evidence, not host CRT byte identity.",
            "Engine data/object/path/head/origin/assert/exit APIs are typed behavioral models; only the three reached inline math helpers execute genuine COFF bytes.",
            "Masked x87 control words 0x027f and 0x037f are covered; unmasked exception modes and every FPSW flag combination are not.",
            "Data buffers are bounded seeded envelopes and do not claim full engine allocation validity or exhaustive pointer aliasing.",
        ],
        "executions": executions,
        "conclusion": (
            "PASS for the bounded corpus: target, first, and actual agree on external API semantics, "
            "diagnostics, final memory, return behavior, private/public ABI observations, and guarded access. "
            "The result is runtime evidence only: the different first-check inline/call placement caused no tested precision divergence."
        ),
    }
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"PASS: {output}")
    print(f"sha256={sha256_path(output)}")
    print(
        f"cases={len(cases())} executions_per_role={len(executions)} "
        f"negative_controls={negative_controls['pass']} fpsw_differences={len(fpsw_differences)}"
    )


if __name__ == "__main__":
    main()
