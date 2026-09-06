"""Bounded target-first replay of January obstacles_test_circle.

The real public function bytes execute for the January target, the first
natural gate object, and the frozen ordinary-build object.  Only the genuine
display_assert and nonreturning system_exit contracts are modeled.  The
complete 0xC08 obstacle container and the query point are immutable inputs.

The corpus covers the signed count and short ignore domains, first-hit order,
the final 24-byte record, tangent equality, and selected IEEE-754 edge cases
under two disclosed conventional x87 control words.  It is finite evidence,
not an exhaustive floating-point or arbitrary-pointer proof.
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

FUNCTION = "_obstacles_test_circle"
CODE, DATA, STACK = 0x100000, 0x200000, 0x300000
DATA_SIZE, STACK_SIZE = 0x4000, 0x10000
ASSERT, EXIT, STOP = CODE + 0x1000, CODE + 0x2000, CODE + 0xF000
SP = STACK + 0x8000
QUERY, OBSTACLES = DATA + 0x100, DATA + 0x1000
FILE_ADDRESS, EXPRESSION_ADDRESS = DATA + 0x3000, DATA + 0x3400

DISC_SIZE = 0x18
CONTAINER_HEADER_SIZE = 8
CAPACITY = 128
CONTAINER_SIZE = CONTAINER_HEADER_SIZE + CAPACITY * DISC_SIZE
ASSERT_LINE = 0x18C
FPCWS = (0x027F, 0x037F)
ABI_ARGUMENT_OFFSETS = {
    "obstacles": 0x08,
    "ignore_disc_index": 0x0C,
    "center": 0x10,
    "radius": 0x14,
}
ABI_RESULT = "signed short in AX"

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
        "scratch/path-obstacles-circle-before-20260906/files/build/split/"
        "source/ai/path_obstacles.obj"
    ),
    "literal_owner": ROOT / (
        "scratch/path-obstacles-circle-before-20260906/files/build/split/"
        "source/ai/path_obstacle_avoidance.obj"
    ),
    "first": ROOT / "scratch/path-obstacles-circle-first-20260906.obj",
    "actual": ROOT / (
        "scratch/path-obstacles-circle-final-20260906/source/ai/"
        "path_obstacles.obj"
    ),
    "source": ROOT / (
        "scratch/path-obstacles-circle-final-20260906/files/source/ai/"
        "path_obstacles.c"
    ),
    "header": ROOT / (
        "scratch/path-obstacles-circle-final-20260906/files/source/ai/path.h"
    ),
    "avoidance": ROOT / (
        "scratch/path-obstacles-circle-final-20260906/files/source/ai/"
        "path_obstacle_avoidance.c"
    ),
    "before_manifest": ROOT / "scratch/path-obstacles-circle-before-20260906.json",
    "final_manifest": ROOT / "scratch/path-obstacles-circle-final-20260906.json",
}
PINS = {
    "target": "8d8a2106cce26a6c271939867ccb6088d9a132ac4f7ddf8cd7c749489e329bc3",
    "literal_owner": "773fcf3c4c50847f26051520cb99edd537ecbc8d810752be010d233acc009303",
    "first": "9eecf47e5bb37f7428f95fc8ea56c3fb0e32ab624867b97361741eaa6f61611f",
    "actual": "ae99335deba8e6cddd594d4add96639b3b7f6b9d60c17058930285964f958a63",
    "source": "cea5c6440f7389a9b2b14b2d32aa75d446f604d5d536b46d7920029cb4b8050c",
    "header": "5b0e569f51633c058c5412b7996ac762b1e5f947381d5a7a4dbb78de02268a15",
    "avoidance": "c2d82a66acf4e223a2c27e9b58fe7fcb771f36c92a0439f718954e0dd5a954f2",
    "before_manifest": "313d047a89c1fed28d63c0aa3377d2031b764888283313e71580c2f44c4e3fc9",
    "final_manifest": "630a9182f06ac19b914400670f298a7aa3b60d01f0828c6453dfd6a099dec148",
}
SECTION_CONTRACTS = {
    "target": {
        "size": 0xA0,
        "relocation_count": 4,
        "normalized_sha256":
            "3a11e2b7f83efa430dfba1728254845c7707cba32e459854f452a47d527bd433",
    },
    "first": {
        "size": 0xA0,
        "relocation_count": 4,
        "normalized_sha256":
            "f07c634dda655b3857287511109246b281ad8f7c784f29868e4e930140d31ef9",
    },
    "actual": {
        "size": 0xA0,
        "relocation_count": 4,
        "normalized_sha256":
            "f07c634dda655b3857287511109246b281ad8f7c784f29868e4e930140d31ef9",
    },
}
RELOCATION_CONTRACT = (
    (0x35, cc.IMAGE_REL_I386_DIR32, FILE),
    (0x3A, cc.IMAGE_REL_I386_DIR32, DISC_ASSERT),
    (0x3F, cc.IMAGE_REL_I386_REL32, "_display_assert"),
    (0x46, cc.IMAGE_REL_I386_REL32, "_system_exit"),
)
MUTATIONS = (
    {
        "label": "invert_hit_branch",
        "role": "actual",
        "offset": 0x83,
        "expected": 0x74,
        "replacement": 0x75,
        "case": "ordinary_miss",
        "detected_by": "return_ax",
    },
    {
        "label": "subtract_disc_radius",
        "role": "actual",
        "offset": 0x5B,
        "expected": 0x40,
        "replacement": 0x60,
        "case": "radius_sum_only_hit",
        "detected_by": "return_ax",
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
ONE = 0x3F800000
NEGATIVE_ONE = 0xBF800000
TWO = 0x40000000
NEGATIVE_TWO = 0xC0000000
THREE = 0x40400000
FOUR = 0x40800000
FIVE = 0x40A00000
POSITIVE_INFINITY = 0x7F800000
NEGATIVE_INFINITY = 0xFF800000
QUIET_NAN = 0x7FC12345
MINIMUM_SUBNORMAL = 0x00000001


def digest(data):
    return hashlib.sha256(data).hexdigest()


def float_bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def bits_float(bits):
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def signed_short(raw):
    return struct.unpack("<h", struct.pack("<H", raw & 0xFFFF))[0]


def disc(x=ZERO, y=ZERO, radius=ZERO, **fields):
    result = {"x": x, "y": y, "radius": radius}
    result.update(fields)
    return result


def make_case(
    name,
    *,
    count=1,
    ignore=0xFFFFFFFF,
    center=(ZERO, ZERO),
    radius=ZERO,
    overrides=None,
    expected=-1,
    null_query=False,
):
    return {
        "name": name,
        "count": count,
        "ignore": ignore & 0xFFFFFFFF,
        "center": center,
        "radius": radius,
        "overrides": dict(overrides or {}),
        "expected": expected,
        "null_query": null_query,
    }


def cases():
    hit = disc(ZERO, ZERO, ONE)
    far = disc(FIVE, ZERO, ONE)
    return [
        make_case("count_zero", count=0, expected=-1, null_query=True),
        make_case("count_negative_one", count=-1, expected=-1, null_query=True),
        make_case(
            "count_minimum_short", count=-0x8000, expected=-1, null_query=True
        ),
        make_case("inside_origin", overrides={0: hit}, expected=0),
        make_case("ordinary_miss", overrides={0: far}, radius=ONE, expected=-1),
        make_case(
            "exact_three_four_tangent",
            overrides={0: disc(ZERO, ZERO, ONE)},
            center=(THREE, FOUR),
            radius=FOUR,
            expected=0,
        ),
        make_case(
            "one_ulp_inside_tangent",
            overrides={0: disc(ZERO, ZERO, ONE)},
            center=(0x3FFFFFFF, ZERO),
            radius=ONE,
            expected=0,
        ),
        make_case(
            "one_ulp_outside_tangent",
            overrides={0: disc(ZERO, ZERO, ONE)},
            center=(0x40000001, ZERO),
            radius=ONE,
            expected=-1,
        ),
        make_case(
            "signed_zero_tangent",
            overrides={0: disc(NEGATIVE_ZERO, ZERO, NEGATIVE_ZERO)},
            center=(ZERO, NEGATIVE_ZERO),
            radius=ZERO,
            expected=0,
        ),
        make_case(
            "radius_sum_only_hit",
            overrides={0: disc(TWO, ZERO, ONE)},
            radius=TWO,
            expected=0,
        ),
        make_case(
            "negative_radii_square_hit",
            overrides={0: disc(ONE, ZERO, NEGATIVE_ONE)},
            radius=NEGATIVE_ONE,
            expected=0,
        ),
        make_case(
            "cancelled_radius_same_center",
            overrides={0: disc(ZERO, ZERO, ONE)},
            radius=NEGATIVE_ONE,
            expected=0,
        ),
        make_case(
            "cancelled_radius_subnormal_miss",
            overrides={0: disc(ZERO, ZERO, ONE)},
            center=(MINIMUM_SUBNORMAL, ZERO),
            radius=NEGATIVE_ONE,
            expected=-1,
        ),
        make_case(
            "ignore_uses_low_zero",
            ignore=0xDEAD0000,
            overrides={0: hit},
            expected=-1,
            null_query=True,
        ),
        make_case(
            "low_one_does_not_ignore_zero",
            ignore=0xBEEF0001,
            overrides={0: hit},
            expected=0,
        ),
        make_case(
            "minus_one_ignore_sentinel",
            ignore=0x1234FFFF,
            overrides={0: hit},
            expected=0,
        ),
        make_case(
            "minimum_short_ignore",
            ignore=0x56788000,
            overrides={0: hit},
            expected=0,
        ),
        make_case(
            "first_hit_wins",
            count=3,
            overrides={0: hit, 1: hit},
            expected=0,
        ),
        make_case(
            "second_hit_after_first_miss",
            count=3,
            overrides={0: far, 1: hit},
            expected=1,
        ),
        make_case(
            "ignored_first_then_second",
            count=3,
            ignore=0xCAFE0000,
            overrides={0: hit, 1: hit},
            expected=1,
        ),
        make_case(
            "ignored_second_first_still_wins",
            count=3,
            ignore=0xCAFE0001,
            overrides={0: hit, 1: hit},
            expected=0,
        ),
        make_case(
            "capacity_last_record_hit",
            count=CAPACITY,
            overrides={CAPACITY - 1: hit},
            expected=CAPACITY - 1,
        ),
        make_case(
            "capacity_last_record_ignored",
            count=CAPACITY,
            ignore=0xBEEF007F,
            overrides={CAPACITY - 1: hit},
            expected=-1,
        ),
        make_case(
            "capacity_first_hit_wins",
            count=CAPACITY,
            overrides={0: hit, CAPACITY - 1: hit},
            expected=0,
        ),
        make_case(
            "nan_disc_center_misses",
            overrides={0: disc(QUIET_NAN, ZERO, ONE)},
            radius=ONE,
            expected=-1,
        ),
        make_case(
            "nan_query_center_misses",
            overrides={0: hit},
            center=(ZERO, QUIET_NAN),
            radius=ONE,
            expected=-1,
        ),
        make_case(
            "nan_disc_radius_misses",
            overrides={0: disc(ZERO, ZERO, QUIET_NAN)},
            expected=-1,
        ),
        make_case(
            "nan_query_radius_misses",
            overrides={0: hit},
            radius=QUIET_NAN,
            expected=-1,
        ),
        make_case(
            "nan_first_finite_second",
            count=2,
            overrides={0: disc(QUIET_NAN, ZERO, ONE), 1: hit},
            expected=1,
        ),
        make_case(
            "ignored_nan_first_finite_second",
            count=2,
            ignore=0xABCD0000,
            overrides={0: disc(QUIET_NAN, ZERO, ONE), 1: hit},
            expected=1,
        ),
        make_case(
            "positive_infinite_disc_radius_hits",
            overrides={0: disc(FIVE, ZERO, POSITIVE_INFINITY)},
            expected=0,
        ),
        make_case(
            "negative_infinite_disc_radius_hits",
            overrides={0: disc(FIVE, ZERO, NEGATIVE_INFINITY)},
            expected=0,
        ),
        make_case(
            "positive_infinite_query_radius_hits",
            overrides={0: far},
            radius=POSITIVE_INFINITY,
            expected=0,
        ),
        make_case(
            "infinite_coordinate_finite_radius_misses",
            overrides={0: disc(POSITIVE_INFINITY, ZERO, ONE)},
            radius=ONE,
            expected=-1,
        ),
        make_case(
            "infinite_coordinate_and_radius_compare_equal",
            overrides={0: disc(POSITIVE_INFINITY, ZERO, POSITIVE_INFINITY)},
            expected=0,
        ),
        make_case(
            "infinity_minus_infinity_is_unordered",
            overrides={0: disc(POSITIVE_INFINITY, ZERO, ONE)},
            center=(POSITIVE_INFINITY, ZERO),
            radius=ONE,
            expected=-1,
        ),
        make_case(
            "opposite_infinite_radii_are_unordered",
            overrides={0: disc(ZERO, ZERO, POSITIVE_INFINITY)},
            radius=NEGATIVE_INFINITY,
            expected=-1,
        ),
        make_case(
            "count_129_asserts",
            count=CAPACITY + 1,
            expected="assert",
            null_query=True,
        ),
        make_case(
            "count_129_ignore_zero_still_asserts",
            count=CAPACITY + 1,
            ignore=0,
            expected="assert",
            null_query=True,
        ),
        make_case(
            "count_maximum_short_asserts",
            count=0x7FFF,
            expected="assert",
            null_query=True,
        ),
    ]


def default_disc(index):
    return disc(
        float_bits(1000.0 + index),
        float_bits(-1000.0),
        ZERO,
        flags=(0x8000 | index) & 0xFFFF,
        obstacle_index=-1 if index & 1 else index,
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
        record.get("flags", 0xA55A),
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


def modeled_result(case):
    count = case["count"]
    if count <= 0:
        return -1
    ignored = signed_short(case["ignore"])
    for index in range(count):
        if index == ignored:
            continue
        if not (0 <= index < count and count <= CAPACITY):
            return "assert"
        record = disc_for(case, index)
        dx = bits_float(record["x"]) - bits_float(case["center"][0])
        dy = bits_float(record["y"]) - bits_float(case["center"][1])
        combined = bits_float(record["radius"]) + bits_float(case["radius"])
        distance_squared = dx * dx + dy * dy
        radius_squared = combined * combined
        if radius_squared >= distance_squared:
            return index
    return -1


def verify_literal_owner():
    owner = cc.load(PATHS["literal_owner"])
    for name, (_, payload) in LITERALS.items():
        symbol = cc.symbol(owner, name)
        section = owner["sections"][symbol["section"] - 1]
        raw = bytes(cc._section_bytes(owner, section))
        assert raw[symbol["value"]:symbol["value"] + len(payload)] == payload, name


def symbolic_relocations(info):
    result = []
    for relocation in info["relocations"]:
        target = relocation.get("symbolic_target", relocation["target"])
        assert target[0] == "symbol" and target[2] == 0, target
        result.append((relocation["address"], relocation["type"], target[1]))
    return tuple(result)


def verify_inputs():
    for role, path in PATHS.items():
        assert digest(path.read_bytes()) == PINS[role], role
    verify_literal_owner()
    metadata = {}
    for role in ("target", "first", "actual"):
        obj = cc.load(PATHS[role])
        symbol = cc.symbol(obj, FUNCTION)
        assert symbol["storage"] == 2 and symbol["type"] == 0x20, role
        info = cc.section_info(obj, FUNCTION)
        contract = SECTION_CONTRACTS[role]
        assert {key: info[key] for key in contract} == contract, role
        assert symbolic_relocations(info) == RELOCATION_CONTRACT, role
        metadata[role] = contract
    return metadata


class Loader:
    def __init__(self, path, patch=None):
        self.obj = cc.load(path)
        self.info = cc.section_info(self.obj, FUNCTION)
        symbol = cc.symbol(self.obj, FUNCTION)
        raw = cc._section_bytes(
            self.obj, self.obj["sections"][symbol["section"] - 1]
        )
        if patch is not None:
            offset, expected, replacement = patch
            assert raw[offset] == expected, (offset, raw[offset], expected)
            raw[offset] = replacement
        destinations = {
            "_display_assert": ASSERT,
            "_system_exit": EXIT,
            FILE: FILE_ADDRESS,
            DISC_ASSERT: EXPRESSION_ADDRESS,
        }
        for relocation in self.info["relocations"]:
            target = relocation.get("symbolic_target", relocation["target"])
            assert target[0] == "symbol", target
            name, addend = target[1:]
            destination = destinations[name] + addend
            if relocation["type"] == cc.IMAGE_REL_I386_REL32:
                destination -= CODE + relocation["address"] + 4
            else:
                assert relocation["type"] == cc.IMAGE_REL_I386_DIR32
            struct.pack_into(
                "<I", raw, relocation["address"], destination & 0xFFFFFFFF
            )
        self.cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
        for address, size in (
            (CODE, 0x10000),
            (DATA, DATA_SIZE),
            (STACK, STACK_SIZE),
        ):
            self.cpu.mem_map(address, size)
        self.cpu.mem_write(CODE, bytes(raw))
        self.cpu.mem_write(ASSERT, b"\xC3")
        self.cpu.mem_write(EXIT, b"\xC3")
        self.cpu.mem_protect(CODE, 0x10000, u.UC_PROT_READ | u.UC_PROT_EXEC)
        self.cpu.mem_protect(DATA, DATA_SIZE, u.UC_PROT_READ)

    def run(self, case, control):
        expected = modeled_result(case)
        assert expected == case["expected"], (case["name"], expected)

        data = bytearray((index * 17 + 3) & 0xFF for index in range(DATA_SIZE))
        data[OBSTACLES - DATA:OBSTACLES - DATA + CONTAINER_SIZE] = \
            build_container(case)
        data[QUERY - DATA:QUERY - DATA + 8] = struct.pack("<2I", *case["center"])
        for address, payload in LITERALS.values():
            data[address - DATA:address - DATA + len(payload)] = payload
        before_data = bytes(data)
        self.cpu.mem_write(DATA, before_data)

        stack = bytearray(b"\xA6" * STACK_SIZE)
        query_pointer = 0 if case["null_query"] else QUERY
        struct.pack_into(
            "<5I",
            stack,
            SP - STACK,
            STOP,
            OBSTACLES,
            case["ignore"],
            query_pointer,
            case["radius"],
        )
        before_stack = bytes(stack)
        self.cpu.mem_write(STACK, before_stack)
        events = []

        def on_code(cpu, address, size, unused):
            if address == ASSERT:
                arguments = struct.unpack(
                    "<4I", cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 4, 16)
                )
                assert expected == "assert"
                assert arguments == (
                    EXPRESSION_ADDRESS,
                    FILE_ADDRESS,
                    ASSERT_LINE,
                    1,
                )
                events.append(["assert", ASSERT_LINE])
            elif address == EXIT:
                assert events == [["assert", ASSERT_LINE]]
                argument = struct.unpack(
                    "<I", cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 4, 4)
                )[0]
                assert argument == 0xFFFFFFFF
                events.append(["exit", -1])
                cpu.emu_stop()
            else:
                assert CODE <= address and address + size <= CODE + self.info["size"]

        def on_write(cpu, access, address, size, value, unused):
            assert STACK + 0x100 <= address and address + size <= SP, (
                "write_boundary",
                address,
                size,
            )

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
            self.cpu.emu_start(CODE, STOP, count=50000)
        finally:
            for hook in hooks:
                self.cpu.hook_del(hook)

        assert bytes(self.cpu.mem_read(DATA, DATA_SIZE)) == before_data
        assert bytes(self.cpu.mem_read(STACK, 0x100)) == before_stack[:0x100]
        assert bytes(self.cpu.mem_read(SP, 0x100)) == \
            before_stack[SP - STACK:SP - STACK + 0x100]
        assert self.cpu.reg_read(UC_X86_REG_FPCW) == control
        assert self.cpu.reg_read(UC_X86_REG_FPTAG) == initial_tag
        fpsw = self.cpu.reg_read(UC_X86_REG_FPSW)
        assert fpsw & 0x3800 == 0
        assert self.cpu.reg_read(UC_X86_REG_EFLAGS) & 0x400 == 0

        if expected == "assert":
            assert events == [["assert", ASSERT_LINE], ["exit", -1]]
            assert self.cpu.reg_read(UC_X86_REG_EIP) == EXIT
            return {
                "result": "nonreturning_assert",
                "events": events,
                "fpsw": fpsw,
                "inputs_immutable": True,
                "write_guards": True,
            }

        assert not events
        ax = self.cpu.reg_read(UC_X86_REG_EAX) & 0xFFFF
        result = signed_short(ax)
        assert result == expected, ("return_ax", case["name"], result, expected)
        assert self.cpu.reg_read(UC_X86_REG_EIP) == STOP
        assert self.cpu.reg_read(UC_X86_REG_ESP) == SP + 4
        assert all(
            self.cpu.reg_read(register) == value
            for register, value in SAVED.items()
        )
        return {
            "result": result,
            "ax": ax,
            "events": events,
            "fpsw": fpsw,
            "stack_delta": 4,
            "nonvolatile_preserved": True,
            "inputs_immutable": True,
            "write_guards": True,
        }


def run_negative_controls():
    corpus = {case["name"]: case for case in cases()}
    results = []
    for mutation in MUTATIONS:
        patch = (
            mutation["offset"],
            mutation["expected"],
            mutation["replacement"],
        )
        loader = Loader(PATHS[mutation["role"]], patch)
        try:
            loader.run(corpus[mutation["case"]], 0x037F)
        except AssertionError as error:
            assert error.args and error.args[0][0] == "return_ax", repr(error)
            results.append({
                **mutation,
                "mutation_kind": "in_memory_actual_function_byte",
            })
        else:
            raise AssertionError(
                "actual-byte mutation was not detected: " + mutation["label"]
            )
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / (
            "scratch/path-obstacles-circle-runtime-20260906/report.json"
        ),
    )
    args = parser.parse_args()
    assert not args.output.exists(), args.output

    section_metadata = verify_inputs()
    corpus = cases()
    assert len(corpus) == len({case["name"] for case in corpus})
    loaders = {
        role: Loader(PATHS[role]) for role in ("target", "first", "actual")
    }
    results = []
    for case in corpus:
        for control in FPCWS:
            outcomes = {
                role: loader.run(case, control)
                for role, loader in loaders.items()
            }
            assert outcomes["target"] == outcomes["first"] == outcomes["actual"], (
                case["name"],
                control,
                outcomes,
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
        "section_contracts": section_metadata,
        "tool_sha256": digest(Path(__file__).read_bytes()),
        "scenario_count": len(corpus),
        "fpcw_modes": [f"0x{control:04x}" for control in FPCWS],
        "execution_count": len(results) * len(loaders),
        "negative_control_execution_count": len(negative_controls),
        "all_pass": True,
        "negative_controls": negative_controls,
        "results": results,
        "limitations": [
            "The two x87 control words are disclosed conventional modes; "
            "this replay does not establish the game's process-default FPCW "
            "or cover unmasked invalid/denormal trap timing.",
            "Quiet-NaN and selected infinity behavior is covered, but "
            "signaling NaNs, arbitrary aliasing, and exhaustive floats are not.",
            "Only display_assert and nonreturning system_exit are modeled; "
            "the tested function contains no external math or private-body call.",
            "The natural radius-load scheduling residual remains fuzzy and "
            "receives no exact-match credit from this behavioral replay.",
        ],
        "provenance": {
            "semantic_source":
                "research-cache/halocea-full-blobs-20260830/src/blam/"
                "bitmaps/obstacles_test_circle.c",
            "january_assertion_owner":
                "scratch/path-obstacles-circle-before-20260906/files/build/"
                "split/source/ai/path_obstacle_avoidance.obj",
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({
        key: report[key]
        for key in (
            "scenario_count",
            "execution_count",
            "negative_control_execution_count",
            "all_pass",
        )
    }))


if __name__ == "__main__":
    main()
