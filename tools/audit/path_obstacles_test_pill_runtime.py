"""Bounded target-first replay of the PathObstacles pill/helper graph.

Every role executes its real public obstacles_test_pill bytes and the real
private circle_intersect_ray bytes selected by that same object's relocation.
The private helper is never called through an invented standalone ABI.  Its
custom register/stack contract is observed and checked at the public call site.

Only display_assert and nonreturning system_exit are modeled.  The finite
corpus covers signed counts, short/byte argument widths, optional flags,
nearest-hit ordering, the last 24-byte disc, and selected masked-x87 edges.
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

ROOT = next(p for p in Path(__file__).resolve().parents
            if (p / "tools/coff_compare.py").is_file())
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

PUBLIC_FUNCTION = "_obstacles_test_pill"
PUBLIC, HELPER = 0x100000, 0x101000
ASSERT, EXIT, STOP = 0x103000, 0x104000, 0x10F000
DATA, OUTPUT, STACK = 0x200000, 0x210000, 0x300000
DATA_SIZE, OUTPUT_SIZE, STACK_SIZE = 0x4000, 0x1000, 0x10000
SP = STACK + 0x8000
POINT, DIRECTION = DATA + 0x100, DATA + 0x200
OBSTACLES, ZERO_LITERAL = DATA + 0x1000, DATA + 0x2C00
FILE_ADDRESS, EXPRESSION_ADDRESS = DATA + 0x3000, DATA + 0x3400
RESULT = OUTPUT + 0x100

DISC_SIZE = 0x18
CONTAINER_HEADER_SIZE = 8
CAPACITY = 128
CONTAINER_SIZE = CONTAINER_HEADER_SIZE + CAPACITY * DISC_SIZE
RESULT_SIZE = 8
ASSERT_LINE = 0x18C
FPCWS = (0x027F, 0x037F)
DISTANCE_EBP_OFFSET = 0x1C
DISTANCE_STACK_OFFSET = DISTANCE_EBP_OFFSET - 4

PUBLIC_ARGUMENT_OFFSETS = {
    "obstacles": 0x08,
    "ignore_disc_index": 0x0C,
    "point": 0x10,
    "direction": 0x14,
    "radius": 0x18,
    "distance": 0x1C,
    "ignore_optional": 0x20,
    "result": 0x24,
}
PUBLIC_ARGUMENT_BASE = "EBP after prologue"
PUBLIC_RESULT = "boolean in AL"
HELPER_ABI = {
    role: {
        "center": "edx",
        "radius_stack_offset": 4,
        "point": "ecx",
        "direction": "eax",
        "distance": "esi",
        "result": "boolean in AL",
    }
    for role in ("target", "first", "actual")
}
REGISTER_IDS = {
    "eax": UC_X86_REG_EAX,
    "ecx": UC_X86_REG_ECX,
    "edx": UC_X86_REG_EDX,
    "esi": UC_X86_REG_ESI,
}

FILE = "??_C@_0BJ@LMDHFJFI@c?3?2halo?2source?2ai?2path?4h?$AA@"
DISC_ASSERT = (
    "??_C@_0FP@EDEGFDAA@disc_index?$DO?$DN0?5?$CG?$CG?5disc_index?$DMobst@"
)
LITERALS = {
    FILE: (FILE_ADDRESS, b"c:\\halo\\source\\ai\\path.h\0"),
    DISC_ASSERT: (
        EXPRESSION_ADDRESS,
        b"disc_index>=0 && disc_index<obstacles->disc_count && "
        b"obstacles->disc_count<=MAXIMUM_DISC_COUNT\0",
    ),
}

PATHS = {
    "target": ROOT / (
        "scratch/path-obstacles-pill-before-20260906/files/build/split/"
        "source/ai/path_obstacles.obj"
    ),
    "literal_owner": ROOT / (
        "scratch/path-obstacles-pill-before-20260906/files/build/split/"
        "source/ai/path_obstacle_avoidance.obj"
    ),
    "zero_owner": ROOT / (
        "scratch/path-obstacles-pill-january-zero-owner-20260906.obj"
    ),
    "first": ROOT / "scratch/path-obstacles-pill-first-20260906.obj",
    "first_ordinary": ROOT / (
        "scratch/path-obstacles-pill-first-20260906/source/ai/"
        "path_obstacles.obj"
    ),
    "actual": ROOT / (
        "scratch/path-obstacles-pill-named-20260906/source/ai/"
        "path_obstacles.obj"
    ),
    "source": ROOT / (
        "scratch/path-obstacles-pill-named-20260906/files/source/ai/"
        "path_obstacles.c"
    ),
    "header": ROOT / (
        "scratch/path-obstacles-pill-named-20260906/files/source/ai/path.h"
    ),
    "config": ROOT / (
        "scratch/path-obstacles-pill-named-20260906/files/config/symbols.json"
    ),
    "selected_target": ROOT / (
        "scratch/path-obstacles-pill-named-20260906/files/build/split/"
        "source/ai/path_obstacles.obj"
    ),
    "before_manifest": ROOT / "scratch/path-obstacles-pill-before-20260906.json",
    "final_manifest": ROOT / "scratch/path-obstacles-pill-named-20260906.json",
}
PINS = {
    "target": "8d8a2106cce26a6c271939867ccb6088d9a132ac4f7ddf8cd7c749489e329bc3",
    "literal_owner": "773fcf3c4c50847f26051520cb99edd537ecbc8d810752be010d233acc009303",
    "zero_owner": "76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea",
    "first": "9d51c96acd5fce2affd58fc1a83c66af11641d325b1629d9f14391bc7e592743",
    "first_ordinary": "4b6ac1461e309b1297d22ad384d30a983ee2136dea73fed6fc1be9e31ac98a41",
    "actual": "4b6ac1461e309b1297d22ad384d30a983ee2136dea73fed6fc1be9e31ac98a41",
    "source": "9bd08252b0f69445eda8fd6756aa5b9d5b3da58aab65b8b60faeb4f571051fee",
    "header": "c27e7239287d950562dacc7403b80c079e721c82d3771c10357f75d55a7e4913",
    "config": "a0dc47c1b4e853b72c60c2fea3fcfbd464e2bf8eba4e1e012606b6f23c643b29",
    "selected_target": "f438f10d52492b5dd4ae35691a50cb10240d30ad01f61a81be57542b0e224a29",
    "before_manifest": "97fae9cd131e74645b903b5e8602fc7da5bcf657653f11ce6305b07aa3fef222",
    "final_manifest": "75b2a1cbd2ab318d79675362e754cab1ec2f176d0dd412992c862c4f12a7311d",
}

PUBLIC_NORMALIZED = (
    "8524208b95d4c9be122cf038216dadd2a6386839552ee2fe8dcf228719af355c"
)
HELPER_NORMALIZED = (
    "833877701c647f823dcb6876411ba17f81d71dde649f60515850561559a509cf"
)
GRAPH_CONTRACTS = {
    "target": {
        "helper_name": "_code_00050580",
        "helper_storage": 2,
        "public_size": 0xF0,
        "public_relocations": 5,
        "public_normalized": PUBLIC_NORMALIZED,
        "helper_size": 0x90,
        "helper_relocations": 3,
        "helper_normalized": HELPER_NORMALIZED,
    },
    "first": {
        "helper_name": "_circle_intersect_ray",
        "helper_storage": 3,
        "public_size": 0xF0,
        "public_relocations": 5,
        "public_normalized": PUBLIC_NORMALIZED,
        "helper_size": 0x90,
        "helper_relocations": 3,
        "helper_normalized": HELPER_NORMALIZED,
    },
    "actual": {
        "helper_name": "_circle_intersect_ray",
        "helper_storage": 3,
        "public_size": 0xF0,
        "public_relocations": 5,
        "public_normalized": PUBLIC_NORMALIZED,
        "helper_size": 0x90,
        "helper_relocations": 3,
        "helper_normalized": HELPER_NORMALIZED,
    },
}
PUBLIC_FIXED_RELOCATIONS = (
    (0x52, cc.IMAGE_REL_I386_DIR32, FILE),
    (0x57, cc.IMAGE_REL_I386_DIR32, DISC_ASSERT),
    (0x5C, cc.IMAGE_REL_I386_REL32, "_display_assert"),
    (0x63, cc.IMAGE_REL_I386_REL32, "_system_exit"),
)
HELPER_RELOCATIONS = (
    (0x1E, cc.IMAGE_REL_I386_DIR32, "__real@00000000"),
    (0x3F, cc.IMAGE_REL_I386_DIR32, "__real@00000000"),
    (0x62, cc.IMAGE_REL_I386_DIR32, "__real@00000000"),
)
MUTATIONS = (
    {
        "label": "invert_strict_nearest_branch",
        "section": "public",
        "offset": 0xAF,
        "expected": 0x75,
        "replacement": 0x74,
        "case": "single_direct_hit",
        "detected_by": "result_output",
    },
    {
        "label": "reverse_helper_hit_distance_subtraction",
        "section": "helper",
        "offset": 0x72,
        "expected": 0x6D,
        "replacement": 0x65,
        "case": "single_direct_hit",
        "detected_by": "helper_distance",
    },
    {
        "label": "test_optional_bit_one_instead_of_zero",
        "section": "public",
        "offset": 0x7D,
        "expected": 0x01,
        "replacement": 0x02,
        "case": "optional_bit_zero_skipped",
        "detected_by": "helper_call_count",
    },
)
SAVED = {
    UC_X86_REG_EBP: 0x12345678,
    UC_X86_REG_EBX: 0x23456789,
    UC_X86_REG_ESI: 0x3456789A,
    UC_X86_REG_EDI: 0x456789AB,
}

ZERO = 0x00000000
NEGATIVE_ZERO = 0x80000000
MINIMUM_SUBNORMAL = 0x00000001
NEGATIVE_MINIMUM_SUBNORMAL = 0x80000001
HALF = 0x3F000000
ONE = 0x3F800000
NEGATIVE_ONE = 0xBF800000
TWO = 0x40000000
NEGATIVE_TWO = 0xC0000000
THREE = 0x40400000
FOUR = 0x40800000
FIVE = 0x40A00000
TEN = 0x41200000
POSITIVE_INFINITY = 0x7F800000
NEGATIVE_INFINITY = 0xFF800000
QUIET_NAN = 0x7FC12345


def fail(label, *details):
    raise AssertionError((label, *details))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def float_bits(value):
    if math.isnan(value):
        return "nan"
    try:
        return struct.unpack("<I", struct.pack("<f", value))[0]
    except OverflowError:
        return POSITIVE_INFINITY if value > 0 else NEGATIVE_INFINITY


def bits_float(bits):
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def signed_short(raw):
    return struct.unpack("<h", struct.pack("<H", raw & 0xFFFF))[0]


def value_from_contract(contract):
    return math.nan if contract == "nan" else bits_float(contract)


def value_contract(value):
    return float_bits(value)


def assert_value(bits, contract, label):
    if contract == "nan":
        if not math.isnan(bits_float(bits)):
            fail(label, f"0x{bits:08x}", "nan")
    elif bits != contract:
        fail(label, f"0x{bits:08x}", f"0x{contract:08x}")


def disc(x=ZERO, y=ZERO, radius=ZERO, **fields):
    result = {"x": x, "y": y, "radius": radius}
    result.update(fields)
    return result


def make_case(
    name,
    *,
    count=1,
    ignore=0xFFFFFFFF,
    point=(ZERO, ZERO),
    direction=(ONE, ZERO),
    radius=ZERO,
    distance=TEN,
    ignore_optional=0,
    overrides=None,
    expected_disc=-1,
    expected_obstacle=-1,
    expected_distance=None,
    expected_assert=False,
    null_geometry=False,
):
    if expected_distance is None:
        expected_distance = distance
    return {
        "name": name,
        "count": count,
        "ignore": ignore & 0xFFFFFFFF,
        "point": point,
        "direction": direction,
        "radius": radius,
        "distance": distance,
        "ignore_optional": ignore_optional & 0xFFFFFFFF,
        "overrides": dict(overrides or {}),
        "expected": {
            "assert": expected_assert,
            "al": None if expected_assert else int(expected_disc != -1),
            "distance": expected_distance,
            "disc_index": expected_disc,
            "obstacle_index": expected_obstacle,
        },
        "null_geometry": null_geometry,
    }


def cases():
    direct = disc(FIVE, ZERO, ONE, obstacle_index=10)
    near = disc(THREE, ZERO, ONE, obstacle_index=20)
    nearest = disc(TWO, ZERO, ONE, obstacle_index=30)
    return [
        make_case(
            "count_zero", count=0, distance=FIVE, null_geometry=True
        ),
        make_case(
            "count_negative_one", count=-1, distance=FOUR, null_geometry=True
        ),
        make_case(
            "count_minimum_short",
            count=-0x8000,
            distance=THREE,
            null_geometry=True,
        ),
        make_case(
            "single_direct_hit",
            overrides={0: direct},
            expected_disc=0,
            expected_obstacle=10,
            expected_distance=FOUR,
        ),
        make_case(
            "origin_inside_with_positive_projection",
            overrides={0: disc(HALF, ZERO, ONE, obstacle_index=11)},
            expected_disc=0,
            expected_obstacle=11,
            expected_distance=ZERO,
        ),
        make_case(
            "behind_origin_inside_is_rejected",
            overrides={0: disc(0xBF000000, ZERO, ONE, obstacle_index=12)},
        ),
        make_case(
            "zero_projection_inside_is_rejected",
            overrides={0: disc(ZERO, ZERO, ONE, obstacle_index=13)},
        ),
        make_case(
            "negative_zero_projection_is_rejected",
            direction=(NEGATIVE_ZERO, ZERO),
            overrides={0: direct},
        ),
        make_case(
            "minimum_subnormal_projection_hits",
            distance=ONE,
            overrides={
                0: disc(
                    MINIMUM_SUBNORMAL,
                    ZERO,
                    ZERO,
                    obstacle_index=14,
                )
            },
            expected_disc=0,
            expected_obstacle=14,
            expected_distance=MINIMUM_SUBNORMAL,
        ),
        make_case(
            "negative_subnormal_projection_misses",
            distance=ONE,
            overrides={
                0: disc(
                    NEGATIVE_MINIMUM_SUBNORMAL,
                    ZERO,
                    ZERO,
                    obstacle_index=15,
                )
            },
        ),
        make_case(
            "discriminant_zero_tangent",
            overrides={0: disc(FIVE, ONE, ONE, obstacle_index=16)},
            expected_disc=0,
            expected_obstacle=16,
            expected_distance=FIVE,
        ),
        make_case(
            "one_ulp_outside_tangent_misses",
            overrides={0: disc(FIVE, 0x3F800001, ONE, obstacle_index=17)},
        ),
        make_case(
            "maximum_distance_tie_is_excluded",
            distance=FOUR,
            overrides={0: direct},
            expected_distance=FOUR,
        ),
        make_case(
            "hit_beyond_maximum_is_excluded",
            distance=THREE,
            overrides={0: direct},
            expected_distance=THREE,
        ),
        make_case(
            "nearest_second_replaces_first",
            count=2,
            overrides={0: direct, 1: near},
            expected_disc=1,
            expected_obstacle=20,
            expected_distance=TWO,
        ),
        make_case(
            "farther_second_does_not_replace",
            count=2,
            overrides={0: near, 1: direct},
            expected_disc=0,
            expected_obstacle=20,
            expected_distance=TWO,
        ),
        make_case(
            "equal_second_preserves_first",
            count=2,
            overrides={
                0: disc(THREE, ZERO, ONE, obstacle_index=21),
                1: disc(THREE, ZERO, ONE, obstacle_index=22),
            },
            expected_disc=0,
            expected_obstacle=21,
            expected_distance=TWO,
        ),
        make_case(
            "third_strictly_nearest_wins",
            count=3,
            overrides={0: direct, 1: near, 2: nearest},
            expected_disc=2,
            expected_obstacle=30,
            expected_distance=ONE,
        ),
        make_case(
            "ignore_uses_low_zero",
            count=2,
            ignore=0xDEAD0000,
            overrides={0: near, 1: direct},
            expected_disc=1,
            expected_obstacle=10,
            expected_distance=FOUR,
        ),
        make_case(
            "low_one_ignores_second",
            count=2,
            ignore=0xBEEF0001,
            overrides={0: near, 1: nearest},
            expected_disc=0,
            expected_obstacle=20,
            expected_distance=TWO,
        ),
        make_case(
            "minus_one_ignore_sentinel",
            ignore=0x1234FFFF,
            overrides={0: direct},
            expected_disc=0,
            expected_obstacle=10,
            expected_distance=FOUR,
        ),
        make_case(
            "optional_low_byte_zero_does_not_skip",
            ignore_optional=0xDEAD0100,
            overrides={0: disc(FIVE, ZERO, ONE, flags=1, obstacle_index=31)},
            expected_disc=0,
            expected_obstacle=31,
            expected_distance=FOUR,
        ),
        make_case(
            "optional_bit_zero_skipped",
            ignore_optional=0xBEEF0001,
            overrides={0: disc(FIVE, ZERO, ONE, flags=1, obstacle_index=32)},
            null_geometry=True,
        ),
        make_case(
            "optional_nonzero_low_byte_skips",
            ignore_optional=0x12340080,
            overrides={0: disc(FIVE, ZERO, ONE, flags=1, obstacle_index=33)},
        ),
        make_case(
            "optional_policy_ignores_flag_bit_one",
            ignore_optional=1,
            overrides={0: disc(FIVE, ZERO, ONE, flags=2, obstacle_index=34)},
            expected_disc=0,
            expected_obstacle=34,
            expected_distance=FOUR,
        ),
        make_case(
            "optional_policy_skips_high_flag_with_bit_zero",
            ignore_optional=1,
            overrides={0: disc(FIVE, ZERO, ONE, flags=0x8001,
                               obstacle_index=35)},
        ),
        make_case(
            "capacity_last_record_hit",
            count=CAPACITY,
            overrides={CAPACITY - 1: disc(THREE, ZERO, ONE,
                                          obstacle_index=1270)},
            expected_disc=CAPACITY - 1,
            expected_obstacle=1270,
            expected_distance=TWO,
        ),
        make_case(
            "capacity_last_record_ignored",
            count=CAPACITY,
            ignore=0xBEEF007F,
            overrides={CAPACITY - 1: disc(THREE, ZERO, ONE,
                                          obstacle_index=1271)},
        ),
        make_case(
            "count_129_asserts",
            count=CAPACITY + 1,
            expected_assert=True,
            null_geometry=True,
        ),
        make_case(
            "count_129_ignore_zero_still_asserts",
            count=CAPACITY + 1,
            ignore=0,
            expected_assert=True,
            null_geometry=True,
        ),
        make_case(
            "count_maximum_short_asserts",
            count=0x7FFF,
            expected_assert=True,
            null_geometry=True,
        ),
        make_case(
            "nan_direction_projection_misses",
            direction=(QUIET_NAN, ZERO),
            overrides={0: direct},
        ),
        make_case(
            "nan_center_projection_misses",
            overrides={0: disc(QUIET_NAN, ZERO, ONE, obstacle_index=40)},
        ),
        make_case(
            "nan_combined_radius_misses",
            radius=QUIET_NAN,
            overrides={0: direct},
        ),
        make_case(
            "positive_infinite_radius_hits_at_zero",
            radius=POSITIVE_INFINITY,
            overrides={0: direct},
            expected_disc=0,
            expected_obstacle=10,
            expected_distance=ZERO,
        ),
        make_case(
            "negative_infinite_radius_hits_at_zero",
            radius=NEGATIVE_INFINITY,
            overrides={0: direct},
            expected_disc=0,
            expected_obstacle=10,
            expected_distance=ZERO,
        ),
        make_case(
            "infinite_center_with_finite_radius_misses",
            overrides={0: disc(POSITIVE_INFINITY, ZERO, ONE,
                               obstacle_index=41)},
        ),
        make_case(
            "infinite_projection_writes_nan_but_public_rejects",
            direction=(POSITIVE_INFINITY, ZERO),
            overrides={0: direct},
        ),
        make_case(
            "nan_maximum_distance_rejects_finite_hit",
            distance=QUIET_NAN,
            overrides={0: direct},
            expected_distance=QUIET_NAN,
        ),
        make_case(
            "positive_infinite_maximum_accepts_finite_hit",
            distance=POSITIVE_INFINITY,
            overrides={0: direct},
            expected_disc=0,
            expected_obstacle=10,
            expected_distance=FOUR,
        ),
        make_case(
            "negative_infinite_maximum_rejects_finite_hit",
            distance=NEGATIVE_INFINITY,
            overrides={0: direct},
            expected_distance=NEGATIVE_INFINITY,
        ),
        make_case(
            "negative_combined_radius_is_squared",
            overrides={0: disc(FIVE, ZERO, NEGATIVE_ONE,
                               obstacle_index=42)},
            expected_disc=0,
            expected_obstacle=42,
            expected_distance=FOUR,
        ),
        make_case(
            "cancelled_combined_radius_is_point_tangent",
            radius=NEGATIVE_ONE,
            overrides={0: disc(FIVE, ZERO, ONE, obstacle_index=43)},
            expected_disc=0,
            expected_obstacle=43,
            expected_distance=FIVE,
        ),
    ]


def default_disc(index):
    return disc(
        float_bits(100.0 + index),
        float_bits(100.0),
        ZERO,
        flags=(0x4000 | (index << 1)) & 0xFFFF,
        obstacle_index=1000 + index,
        object=(0x71000000 + index) & 0xFFFFFFFF,
        height=float_bits(index + 0.25),
    )


def disc_for(case, index):
    result = default_disc(index)
    result.update(case["overrides"].get(index, {}))
    return result


def pack_disc(record):
    return struct.pack(
        "<HhI4I",
        record.get("flags", 0),
        record.get("obstacle_index", -1),
        record.get("object", 0xFFFFFFFF),
        record["x"],
        record["y"],
        record["radius"],
        record.get("height", 0x7FC0ABCD),
    )


def build_container(case):
    data = bytearray((index * 29 + 11) & 0xFF for index in range(CONTAINER_SIZE))
    struct.pack_into("<hhh", data, 0, 7, case["count"], 3)
    for index in range(CAPACITY):
        start = CONTAINER_HEADER_SIZE + index * DISC_SIZE
        data[start:start + DISC_SIZE] = pack_disc(disc_for(case, index))
    return bytes(data)


def rounded_value(value):
    contract = value_contract(value)
    return value_from_contract(contract), contract


def helper_model(case, record):
    combined, combined_contract = rounded_value(
        bits_float(record["radius"]) + bits_float(case["radius"])
    )
    dx = bits_float(record["x"]) - bits_float(case["point"][0])
    dy = bits_float(record["y"]) - bits_float(case["point"][1])
    projection_extended = (
        dy * bits_float(case["direction"][1])
        + dx * bits_float(case["direction"][0])
    )
    projection, _ = rounded_value(projection_extended)
    if not projection_extended > 0.0:
        return {
            "radius": combined_contract,
            "hit": False,
            "distance": None,
        }
    offset = dx * dx + dy * dy - combined * combined
    if offset <= 0.0:
        return {
            "radius": combined_contract,
            "hit": True,
            "distance": ZERO,
        }
    discriminant = projection * projection - offset
    if not discriminant >= 0.0:
        return {
            "radius": combined_contract,
            "hit": False,
            "distance": None,
        }
    distance = projection - math.sqrt(discriminant)
    return {
        "radius": combined_contract,
        "hit": True,
        "distance": value_contract(distance),
    }


def modeled_case(case):
    calls = []
    result_distance = case["distance"]
    result_disc = -1
    result_obstacle = -1
    scratch = case["distance"]
    count = case["count"]
    ignored = signed_short(case["ignore"])

    if count > 0:
        for index in range(count):
            if index == ignored:
                continue
            if not (0 <= index < count and count <= CAPACITY):
                model = {
                    "assert": True,
                    "calls": calls,
                    "scratch": scratch,
                    "distance": result_distance,
                    "disc_index": result_disc,
                    "obstacle_index": result_obstacle,
                    "al": None,
                }
                verify_expected(case, model)
                return model
            record = disc_for(case, index)
            if (case["ignore_optional"] & 0xFF) and (record["flags"] & 1):
                continue
            helper = helper_model(case, record)
            if helper["hit"]:
                scratch = helper["distance"]
            calls.append({
                "index": index,
                "radius": helper["radius"],
                "return_al": int(helper["hit"]),
                "scratch_after": scratch,
            })
            if helper["hit"]:
                current = bits_float(result_distance)
                candidate = value_from_contract(scratch)
                if current > candidate:
                    if scratch == "nan":
                        fail("model_nan_update", case["name"], index)
                    result_distance = scratch
                    result_disc = index
                    result_obstacle = record["obstacle_index"]

    model = {
        "assert": False,
        "calls": calls,
        "scratch": scratch,
        "distance": result_distance,
        "disc_index": result_disc,
        "obstacle_index": result_obstacle,
        "al": int(result_disc != -1),
    }
    verify_expected(case, model)
    return model


def verify_expected(case, model):
    expected = case["expected"]
    for key in ("assert", "al", "distance", "disc_index", "obstacle_index"):
        if model[key] != expected[key]:
            fail("case_model", case["name"], key, model[key], expected[key])


def expected_result_bytes(model):
    return struct.pack(
        "<Ihh",
        model["distance"],
        model["disc_index"],
        model["obstacle_index"],
    )


def symbolic_relocations(info):
    result = []
    for relocation in info["relocations"]:
        target = relocation.get("symbolic_target", relocation["target"])
        if target[0] != "symbol" or target[2] != 0:
            fail("relocation_shape", target)
        result.append((relocation["address"], relocation["type"], target[1]))
    return tuple(result)


def verify_data_owner(path, name, payload, storage=2):
    obj = cc.load(path)
    symbol = cc.symbol(obj, name)
    if symbol["storage"] != storage:
        fail("data_owner_storage", name, symbol["storage"])
    section = obj["sections"][symbol["section"] - 1]
    raw = bytes(cc._section_bytes(obj, section))
    if raw[symbol["value"]:symbol["value"] + len(payload)] != payload:
        fail("data_owner_payload", name)


def verify_inputs():
    for role, path in PATHS.items():
        if digest(path.read_bytes()) != PINS[role]:
            fail("pin", role)
    for name, (_, payload) in LITERALS.items():
        verify_data_owner(PATHS["literal_owner"], name, payload)
    verify_data_owner(
        PATHS["zero_owner"], "__real@00000000", b"\0\0\0\0"
    )

    result = {}
    for role in ("target", "first", "actual"):
        obj = cc.load(PATHS[role])
        contract = GRAPH_CONTRACTS[role]
        public_symbol = cc.symbol(obj, PUBLIC_FUNCTION)
        helper_symbol = cc.symbol(obj, contract["helper_name"])
        if (public_symbol["storage"], public_symbol["type"]) != (2, 0x20):
            fail("public_symbol", role, public_symbol)
        if (helper_symbol["storage"], helper_symbol["type"]) != (
            contract["helper_storage"],
            0x20,
        ):
            fail("helper_symbol", role, helper_symbol)
        public_info = cc.section_info(obj, PUBLIC_FUNCTION)
        helper_info = cc.section_info(obj, contract["helper_name"])
        actual_public = (
            public_info["size"],
            public_info["relocation_count"],
            public_info["normalized_sha256"],
        )
        expected_public = (
            contract["public_size"],
            contract["public_relocations"],
            contract["public_normalized"],
        )
        actual_helper = (
            helper_info["size"],
            helper_info["relocation_count"],
            helper_info["normalized_sha256"],
        )
        expected_helper = (
            contract["helper_size"],
            contract["helper_relocations"],
            contract["helper_normalized"],
        )
        if actual_public != expected_public:
            fail("public_section", role, actual_public, expected_public)
        if actual_helper != expected_helper:
            fail("helper_section", role, actual_helper, expected_helper)
        expected_public_relocations = PUBLIC_FIXED_RELOCATIONS + (
            (0x97, cc.IMAGE_REL_I386_REL32, contract["helper_name"]),
        )
        if symbolic_relocations(public_info) != expected_public_relocations:
            fail("public_relocations", role, symbolic_relocations(public_info))
        if symbolic_relocations(helper_info) != HELPER_RELOCATIONS:
            fail("helper_relocations", role, symbolic_relocations(helper_info))
        result[role] = {
            "public": {
                "size": public_info["size"],
                "relocation_count": public_info["relocation_count"],
                "normalized_sha256": public_info["normalized_sha256"],
            },
            "helper": {
                "name": contract["helper_name"],
                "storage": helper_symbol["storage"],
                "size": helper_info["size"],
                "relocation_count": helper_info["relocation_count"],
                "normalized_sha256": helper_info["normalized_sha256"],
            },
        }
    return result


def patch_section(raw, info, base, destinations):
    for relocation in info["relocations"]:
        target = relocation.get("symbolic_target", relocation["target"])
        if target[0] != "symbol":
            fail("patch_relocation_shape", target)
        name, addend = target[1:]
        destination = destinations[name] + addend
        if relocation["type"] == cc.IMAGE_REL_I386_REL32:
            destination -= base + relocation["address"] + 4
        elif relocation["type"] != cc.IMAGE_REL_I386_DIR32:
            fail("patch_relocation_type", relocation["type"])
        struct.pack_into(
            "<I", raw, relocation["address"], destination & 0xFFFFFFFF
        )


class Loader:
    def __init__(self, role, patch=None):
        self.role = role
        self.obj = cc.load(PATHS[role])
        self.contract = GRAPH_CONTRACTS[role]
        self.public_info = cc.section_info(self.obj, PUBLIC_FUNCTION)
        self.helper_info = cc.section_info(
            self.obj, self.contract["helper_name"]
        )
        public_symbol = cc.symbol(self.obj, PUBLIC_FUNCTION)
        helper_symbol = cc.symbol(self.obj, self.contract["helper_name"])
        public_raw = cc._section_bytes(
            self.obj, self.obj["sections"][public_symbol["section"] - 1]
        )
        helper_raw = cc._section_bytes(
            self.obj, self.obj["sections"][helper_symbol["section"] - 1]
        )
        if patch is not None:
            section, offset, expected, replacement = patch
            raw = public_raw if section == "public" else helper_raw
            if raw[offset] != expected:
                fail("mutation_guard", section, offset, raw[offset], expected)
            raw[offset] = replacement

        destinations = {
            FILE: FILE_ADDRESS,
            DISC_ASSERT: EXPRESSION_ADDRESS,
            "_display_assert": ASSERT,
            "_system_exit": EXIT,
            self.contract["helper_name"]: HELPER,
        }
        patch_section(public_raw, self.public_info, PUBLIC, destinations)
        patch_section(
            helper_raw,
            self.helper_info,
            HELPER,
            {"__real@00000000": ZERO_LITERAL},
        )
        helper_relocation = next(
            relocation for relocation in self.public_info["relocations"]
            if relocation.get("symbolic_target", relocation["target"])[1]
            == self.contract["helper_name"]
        )
        self.helper_return = PUBLIC + helper_relocation["address"] + 4

        self.cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        for address, size in (
            (PUBLIC, 0x10000),
            (DATA, DATA_SIZE),
            (OUTPUT, OUTPUT_SIZE),
            (STACK, STACK_SIZE),
        ):
            self.cpu.mem_map(address, size)
        self.cpu.mem_write(PUBLIC, bytes(public_raw))
        self.cpu.mem_write(HELPER, bytes(helper_raw))
        self.cpu.mem_write(ASSERT, b"\xC3")
        self.cpu.mem_write(EXIT, b"\xC3")
        self.cpu.mem_protect(PUBLIC, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        self.cpu.mem_protect(DATA, DATA_SIZE, u.UC_PROT_READ)

    def run(self, case, control):
        model = modeled_case(case)
        data = bytearray((index * 17 + 3) & 0xFF for index in range(DATA_SIZE))
        data[POINT - DATA:POINT - DATA + 8] = struct.pack("<2I", *case["point"])
        data[DIRECTION - DATA:DIRECTION - DATA + 8] = \
            struct.pack("<2I", *case["direction"])
        data[OBSTACLES - DATA:OBSTACLES - DATA + CONTAINER_SIZE] = \
            build_container(case)
        struct.pack_into("<I", data, ZERO_LITERAL - DATA, ZERO)
        for address, payload in LITERALS.values():
            data[address - DATA:address - DATA + len(payload)] = payload
        before_data = bytes(data)
        self.cpu.mem_write(DATA, before_data)

        output = bytearray((index * 23 + 5) & 0xFF for index in range(OUTPUT_SIZE))
        before_output = bytes(output)
        self.cpu.mem_write(OUTPUT, before_output)

        stack = bytearray(b"\xA6" * STACK_SIZE)
        point_pointer = 0 if case["null_geometry"] else POINT
        direction_pointer = 0 if case["null_geometry"] else DIRECTION
        struct.pack_into(
            "<9I",
            stack,
            SP - STACK,
            STOP,
            OBSTACLES,
            case["ignore"],
            point_pointer,
            direction_pointer,
            case["radius"],
            case["distance"],
            case["ignore_optional"],
            RESULT,
        )
        before_stack = bytes(stack)
        self.cpu.mem_write(STACK, before_stack)

        calls = []
        events = []
        result_writes = []
        abi = HELPER_ABI[self.role]

        def on_code(cpu, address, size, unused):
            if address == ASSERT:
                arguments = struct.unpack(
                    "<4I", cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 4, 16)
                )
                if not model["assert"]:
                    fail("unexpected_assert", case["name"])
                expected = (
                    EXPRESSION_ADDRESS,
                    FILE_ADDRESS,
                    ASSERT_LINE,
                    1,
                )
                if arguments != expected:
                    fail("assert_arguments", arguments, expected)
                events.append(["assert", ASSERT_LINE])
                return
            if address == EXIT:
                argument = struct.unpack(
                    "<I", cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 4, 4)
                )[0]
                if events != [["assert", ASSERT_LINE]] or argument != 0xFFFFFFFF:
                    fail("exit_contract", events, argument)
                events.append(["exit", -1])
                cpu.emu_stop()
                return
            if address == HELPER:
                ordinal = len(calls)
                if ordinal >= len(model["calls"]):
                    fail("helper_call_count", case["name"], ordinal)
                expected_call = model["calls"][ordinal]
                index = expected_call["index"]
                center = OBSTACLES + CONTAINER_HEADER_SIZE + index * DISC_SIZE + 8
                expected_registers = {
                    abi["center"]: center,
                    abi["point"]: POINT,
                    abi["direction"]: DIRECTION,
                    abi["distance"]: SP + DISTANCE_STACK_OFFSET,
                }
                for name, expected_value in expected_registers.items():
                    actual_value = cpu.reg_read(REGISTER_IDS[name])
                    if actual_value != expected_value:
                        fail(
                            "helper_abi_register",
                            self.role,
                            name,
                            actual_value,
                            expected_value,
                        )
                helper_sp = cpu.reg_read(UC_X86_REG_ESP)
                return_address, radius_bits = struct.unpack(
                    "<2I", cpu.mem_read(helper_sp, 8)
                )
                if return_address != self.helper_return:
                    fail("helper_return_address", return_address, self.helper_return)
                assert_value(radius_bits, expected_call["radius"], "helper_radius")
                calls.append({
                    "index": index,
                    "radius_bits": radius_bits,
                    "return_address": return_address,
                })
                return
            if address == self.helper_return:
                if not calls or "return_al" in calls[-1]:
                    fail("helper_return_order", case["name"], len(calls))
                expected_call = model["calls"][len(calls) - 1]
                actual_al = cpu.reg_read(UC_X86_REG_EAX) & 0xFF
                if actual_al != expected_call["return_al"]:
                    fail(
                        "helper_return_al",
                        case["name"],
                        actual_al,
                        expected_call["return_al"],
                    )
                distance_bits = struct.unpack(
                    "<I",
                    cpu.mem_read(SP + DISTANCE_STACK_OFFSET, 4),
                )[0]
                assert_value(
                    distance_bits,
                    expected_call["scratch_after"],
                    "helper_distance",
                )
                calls[-1].update({
                    "return_al": actual_al,
                    "distance_bits": distance_bits,
                })
                return
            in_public = (
                PUBLIC <= address < PUBLIC + self.public_info["size"]
                and address + size <= PUBLIC + self.public_info["size"]
            )
            in_helper = (
                HELPER <= address < HELPER + self.helper_info["size"]
                and address + size <= HELPER + self.helper_info["size"]
            )
            if not (in_public or in_helper):
                fail("code_boundary", address, size)

        def on_write(cpu, access, address, size, value, unused):
            internal_stack = (
                STACK + 0x100 <= address and address + size <= SP
            )
            distance_scratch = (
                SP + DISTANCE_STACK_OFFSET <= address
                and address + size <= SP + DISTANCE_STACK_OFFSET + 4
            )
            result_write = (
                RESULT <= address and address + size <= RESULT + RESULT_SIZE
            )
            if not (internal_stack or distance_scratch or result_write):
                fail("write_boundary", address, size)
            if result_write:
                result_writes.append((address - RESULT, size, value))

        hooks = [
            self.cpu.hook_add(u.UC_HOOK_CODE, on_code),
            self.cpu.hook_add(u.UC_HOOK_MEM_WRITE, on_write),
        ]
        self.cpu.reg_write(UC_X86_REG_ESP, SP)
        self.cpu.reg_write(UC_X86_REG_EAX, 0xA5A51234)
        self.cpu.reg_write(UC_X86_REG_ECX, 0x56789ABC)
        self.cpu.reg_write(UC_X86_REG_EDX, 0x6789ABCD)
        self.cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
        self.cpu.reg_write(UC_X86_REG_FPCW, control)
        self.cpu.reg_write(UC_X86_REG_FPSW, 0)
        self.cpu.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        initial_tag = self.cpu.reg_read(UC_X86_REG_FPTAG)
        for register, value in SAVED.items():
            self.cpu.reg_write(register, value)
        try:
            self.cpu.emu_start(PUBLIC, STOP, count=100000)
        finally:
            for hook in hooks:
                self.cpu.hook_del(hook)

        if len(calls) != len(model["calls"]):
            fail("helper_call_count", case["name"], len(calls),
                 len(model["calls"]))
        if bytes(self.cpu.mem_read(DATA, DATA_SIZE)) != before_data:
            fail("input_mutation", case["name"])

        expected_output = bytearray(before_output)
        expected_output[RESULT - OUTPUT:RESULT - OUTPUT + RESULT_SIZE] = \
            expected_result_bytes(model)
        actual_output = bytes(self.cpu.mem_read(OUTPUT, OUTPUT_SIZE))
        if actual_output != bytes(expected_output):
            fail("result_output", case["name"])

        actual_stack = bytes(self.cpu.mem_read(STACK, STACK_SIZE))
        if actual_stack[:0x100] != before_stack[:0x100]:
            fail("lower_stack_guard", case["name"])
        start = SP - STACK
        scratch_at = start + DISTANCE_STACK_OFFSET
        if actual_stack[start:scratch_at] != before_stack[start:scratch_at]:
            fail("caller_stack_prefix", case["name"])
        if actual_stack[scratch_at + 4:start + 0x100] != \
                before_stack[scratch_at + 4:start + 0x100]:
            fail("caller_stack_suffix", case["name"])
        scratch_bits = struct.unpack_from("<I", actual_stack, scratch_at)[0]
        assert_value(scratch_bits, model["scratch"], "caller_distance_scratch")

        if self.cpu.reg_read(UC_X86_REG_FPCW) != control:
            fail("fpcw", case["name"])
        if self.cpu.reg_read(UC_X86_REG_FPTAG) != initial_tag:
            fail("fptag", case["name"])
        fpsw = self.cpu.reg_read(UC_X86_REG_FPSW)
        if fpsw & 0x3800:
            fail("x87_top", case["name"], fpsw)
        if self.cpu.reg_read(UC_X86_REG_EFLAGS) & 0x400:
            fail("direction_flag", case["name"])

        if model["assert"]:
            if events != [["assert", ASSERT_LINE], ["exit", -1]]:
                fail("assert_events", case["name"], events)
            if self.cpu.reg_read(UC_X86_REG_EIP) != EXIT:
                fail("assert_eip", case["name"])
        else:
            if events:
                fail("unexpected_events", case["name"], events)
            al = self.cpu.reg_read(UC_X86_REG_EAX) & 0xFF
            if al != model["al"]:
                fail("return_al", case["name"], al, model["al"])
            if self.cpu.reg_read(UC_X86_REG_EIP) != STOP:
                fail("return_eip", case["name"])
            if self.cpu.reg_read(UC_X86_REG_ESP) != SP + 4:
                fail("return_esp", case["name"])
            for register, value in SAVED.items():
                if self.cpu.reg_read(register) != value:
                    fail("nonvolatile", case["name"], register)

        trace_bytes = json.dumps(calls, sort_keys=True).encode()
        return {
            "result_hex": actual_output[
                RESULT - OUTPUT:RESULT - OUTPUT + RESULT_SIZE
            ].hex(),
            "return_al": model["al"],
            "events": events,
            "helper_call_count": len(calls),
            "helper_trace_sha256": digest(trace_bytes),
            "distance_scratch_bits": f"0x{scratch_bits:08x}",
            "result_write_count": len(result_writes),
            "fpsw": fpsw,
            "inputs_immutable": True,
            "write_guards": True,
            "abi_preserved": not model["assert"],
        }


def run_negative_controls():
    corpus = {case["name"]: case for case in cases()}
    results = []
    for mutation in MUTATIONS:
        patch = (
            mutation["section"],
            mutation["offset"],
            mutation["expected"],
            mutation["replacement"],
        )
        loader = Loader("actual", patch)
        try:
            loader.run(corpus[mutation["case"]], 0x037F)
        except AssertionError as error:
            if not error.args or error.args[0][0] != mutation["detected_by"]:
                raise
            results.append({
                **mutation,
                "mutation_kind": "in_memory_actual_function_byte",
            })
        else:
            fail("undetected_mutation", mutation["label"])
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "scratch/path-obstacles-pill-runtime-20260906/report.json",
    )
    args = parser.parse_args()
    if args.output.exists():
        fail("output_exists", str(args.output))

    section_metadata = verify_inputs()
    corpus = cases()
    if len(corpus) != len({case["name"] for case in corpus}):
        fail("duplicate_case_name")
    loaders = {
        role: Loader(role) for role in ("target", "first", "actual")
    }
    results = []
    total_helper_calls = 0
    for case in corpus:
        for control in FPCWS:
            outcomes = {
                role: loader.run(case, control)
                for role, loader in loaders.items()
            }
            if not (
                outcomes["target"] == outcomes["first"] == outcomes["actual"]
            ):
                fail("role_outcome", case["name"], control, outcomes)
            total_helper_calls += sum(
                outcome["helper_call_count"] for outcome in outcomes.values()
            )
            results.append({
                "case": case["name"],
                "fpcw": f"0x{control:04x}",
                "outcome": outcomes["target"],
            })

    negative_controls = run_negative_controls()
    report = {
        "scope": __doc__,
        "pins": PINS,
        "sections": section_metadata,
        "tool_sha256": digest(Path(__file__).read_bytes()),
        "scenario_count": len(corpus),
        "fpcw_modes": [f"0x{control:04x}" for control in FPCWS],
        "execution_count": len(results) * len(loaders),
        "helper_call_count_across_roles": total_helper_calls,
        "negative_control_execution_count": len(negative_controls),
        "all_pass": True,
        "negative_controls": negative_controls,
        "results": results,
        "limitations": [
            "The two x87 control words are disclosed masked conventional "
            "modes; this does not establish the game's process-default FPCW "
            "or cover unmasked invalid/denormal trap timing.",
            "Quiet-NaN, infinity, subnormal, and tangent examples are finite; "
            "signaling NaNs, arbitrary aliasing, and exhaustive floats are not.",
            "The helper is tested only through each object's genuine public "
            "caller and role-specific custom ABI, never as a fabricated API.",
            "Only display_assert and nonreturning system_exit are modeled; "
            "the public/helper graph has no other external call.",
        ],
        "provenance": {
            "january_helper_name": "_code_00050580",
            "authentic_private_name": "circle_intersect_ray",
            "zero_literal_owner":
                "scratch/path-obstacles-pill-january-zero-owner-20260906.obj",
            "zero_literal_selected_provider_census": "1 of 833 January objects",
            "semantic_source":
                "research-cache/halocea-full-blobs-20260830/src/blam/"
                "physics/circle_intersect_ray.c",
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({
        key: report[key]
        for key in (
            "scenario_count",
            "execution_count",
            "helper_call_count_across_roles",
            "negative_control_execution_count",
            "all_pass",
        )
    }))


if __name__ == "__main__":
    main()
