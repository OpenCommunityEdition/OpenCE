"""Fixture-free contract tests for the bounded RenderDebug runtime oracle."""

import copy
import math
from pathlib import Path
import re
import struct

import pytest

pytest.importorskip("unicorn")
from tools.audit import render_debug_three_runtime as oracle


def cases_by_name():
    cases = oracle.cases()
    return {case["name"]: case for case in cases}


def modeled_success(case_name):
    case = cases_by_name()[case_name]
    unused_input, blocks, unused_locations = oracle.input_image(case)
    before = oracle.initial_cache(case)
    after, events, trace, index = oracle.expected_success(case, before, blocks)
    return case, blocks, before, after, events, trace, index


def synthetic_outcome(case_name):
    case, unused_blocks, unused_before, after, events, trace, index = modeled_success(case_name)
    entry_hex = None
    if index is not None:
        start = oracle.POOL_SIZE + index * oracle.ENTRY_SIZE
        entry_hex = bytes(after[start:start + oracle.ENTRY_SIZE]).hex()
    return {
        "case": case_name,
        "events": copy.deepcopy(events),
        "function_trace": list(trace),
        "cache_entry_hex": entry_hex,
        "cache_sha256": oracle.digest(bytes(after)),
        "abi": {
            "esp_restored": True,
            "nonvolatiles_restored": True,
            "cache_guard": True,
            "fpcw_final": 0x037F,
        },
    }


def test_case_population_and_seven_assertion_exits_are_exact():
    cases = oracle.cases()
    by_name = cases_by_name()
    assert len(cases) == len(by_name) == 22
    assert {case["function"] for case in cases} == {
        oracle.POINT,
        oracle.LINE,
        oracle.BOX,
    }
    assert {case["name"] for case in cases if case.get("null")} == {
        "assert_point_null",
        "assert_point_color_null",
        "assert_line_point0_null",
        "assert_line_point1_null",
        "assert_line_color_null",
        "assert_box_bounds_null",
        "assert_box_color_null",
    }
    assert oracle.ASSERTIONS == {
        (oracle.POINT, "point"): ("point", 327),
        (oracle.POINT, "color"): ("color", 328),
        (oracle.LINE, "point0"): ("point0", 363),
        (oracle.LINE, "point1"): ("point1", 364),
        (oracle.LINE, "color"): ("color", 365),
        (oracle.BOX, "bounds"): ("bounds", 812),
        (oracle.BOX, "color"): ("color", 813),
    }
    assert len(oracle.ASSERTIONS) == 7
    for case in cases:
        if "size" in case:
            assert math.isfinite(case["size"])
        if "bounds" in case:
            assert all(math.isfinite(value) for value in case["bounds"])


def test_pin_contract_is_complete_frozen_and_lowercase_sha256():
    assert set(oracle.PATHS) == set(oracle.PINS) == {
        "target",
        "first",
        "actual",
        "source",
        "cache_reference",
    }
    assert all(re.fullmatch(r"[0-9a-f]{64}", value) for value in oracle.PINS.values())
    relative = {
        label: path.relative_to(oracle.ROOT).as_posix()
        for label, path in oracle.PATHS.items()
    }
    assert relative["target"] == (
        "scratch/render-debug-three-before-20260906/"
        "build/split/source/render/render_debug.obj"
    )
    assert relative["source"] == "scratch/render-debug-three-source-final-20260906.c"
    assert relative["first"] == "scratch/render-debug-three-first-20260906.obj"
    assert relative["actual"] == "scratch/render-debug-three-actual-20260906.obj"
    assert oracle.LITERALS["__real@3f000000"] == struct.pack("<I", 0x3F000000)


@pytest.mark.parametrize(
    "size,expected",
    [
        (
            4.0,
            [
                ((-0.75, -2.5, 3.75), (3.25, -2.5, 3.75)),
                ((1.25, -4.5, 3.75), (1.25, -0.5, 3.75)),
                ((1.25, -2.5, 1.75), (1.25, -2.5, 5.75)),
            ],
        ),
        (
            0.0,
            [
                ((1.25, -2.5, 3.75), (1.25, -2.5, 3.75)),
                ((1.25, -2.5, 3.75), (1.25, -2.5, 3.75)),
                ((1.25, -2.5, 3.75), (1.25, -2.5, 3.75)),
            ],
        ),
        (
            -2.0,
            [
                ((2.25, -2.5, 3.75), (0.25, -2.5, 3.75)),
                ((1.25, -1.5, 3.75), (1.25, -3.5, 3.75)),
                ((1.25, -2.5, 4.75), (1.25, -2.5, 2.75)),
            ],
        ),
    ],
)
def test_point_axis_pairs_cover_positive_zero_and_negative_sizes(size, expected):
    point = oracle.point_bytes((1.25, -2.5, 3.75))
    actual = [
        (struct.unpack("<3f", point0), struct.unpack("<3f", point1))
        for point0, point1 in oracle.point_axis_pairs(point, size)
    ]
    assert actual == expected


def test_inverted_box_preserves_two_ring_and_vertical_edge_order():
    case, blocks, before, after, events, trace, index = modeled_success(
        "box_immediate_inverted"
    )
    points = oracle.box_points(blocks["bounds"])
    expected_edges = [
        (3, 0), (0, 1), (1, 2), (2, 3),
        (7, 4), (4, 5), (5, 6), (6, 7),
        (0, 4), (1, 5), (2, 6), (3, 7),
    ]
    assert index is None
    assert after == before
    assert [
        (bytes.fromhex(event["point0"]), bytes.fromhex(event["point1"]))
        for event in events
    ] == [(points[start], points[end]) for start, end in expected_edges]
    assert trace == (
        [oracle.BOX, oracle.POLYGON]
        + [oracle.LINE] * 4
        + [oracle.POLYGON]
        + [oracle.LINE] * 8
    )
    assert case["bounds"] == (4.0, -3.0, 6.0, -5.0, 8.0, -7.0)


@pytest.mark.parametrize(
    "case_name,tag,written_end",
    [
        ("point_deferred_same_tick", 1, 0x24),
        ("line_deferred_same_tick", 2, 0x2C),
        ("box_deferred_same_tick", 7, 0x2C),
    ],
)
def test_deferred_variadic_payloads_have_typed_cache_layout_and_preserve_padding(
    case_name, tag, written_end
):
    case, blocks, before, after, events, trace, index = modeled_success(case_name)
    assert trace == [case["function"], oracle.CACHE_ENTRY]
    assert [event["api"] for event in events] == ["game_time_get"]
    assert index == case["count"]
    assert oracle.s16(after, oracle.COUNT_OFFSET) == case["count"] + 1

    start = oracle.POOL_SIZE + index * oracle.ENTRY_SIZE
    old_entry = bytes(before[start:start + oracle.ENTRY_SIZE])
    entry = bytes(after[start:start + oracle.ENTRY_SIZE])
    assert struct.unpack_from("<h", entry)[0] == tag
    assert entry[2:4] == old_entry[2:4]
    assert entry[written_end:] == old_entry[written_end:]

    if tag == 1:
        assert entry[4:0x10] == blocks["point"]
        assert entry[0x10:0x14] == oracle.f32(case["size"])
        assert entry[0x14:0x24] == blocks["color"]
    elif tag == 2:
        assert entry[4:0x10] == blocks["point0"]
        assert entry[0x10:0x1C] == blocks["point1"]
        assert entry[0x1C:0x2C] == blocks["color"]
    else:
        assert entry[4:0x1C] == blocks["bounds"]
        assert entry[0x1C:0x2C] == blocks["color"]


@pytest.mark.parametrize(
    "case_name,word_count",
    [
        ("point_deferred_same_tick", 4),
        ("line_deferred_same_tick", 4),
        ("box_deferred_same_tick", 3),
    ],
)
def test_deferred_public_abi_stages_false_and_exact_payload_words(case_name, word_count):
    case = cases_by_name()[case_name]
    unused_input, blocks, locations = oracle.input_image(case)
    raw = oracle.public_arguments(case, locations)
    words = struct.unpack("<%dI" % word_count, raw)
    assert words[0] == 0
    if case["function"] == oracle.POINT:
        assert words[1:] == (
            locations["point"],
            struct.unpack("<I", oracle.f32(case["size"]))[0],
            locations["color"],
        )
    elif case["function"] == oracle.LINE:
        assert words[1:] == (
            locations["point0"],
            locations["point1"],
            locations["color"],
        )
    else:
        assert words[1:] == (locations["bounds"], locations["color"])
    assert blocks


def test_new_tick_restarts_at_slot_zero_but_preserves_stale_pool_and_entry_padding():
    case, unused_blocks, before, after, events, trace, index = modeled_success(
        "point_deferred_new_tick"
    )
    assert [event["api"] for event in events] == ["game_time_get", "game_time_get"]
    assert [event["result"] for event in events] == [10, 11]
    assert trace == [oracle.POINT, oracle.CACHE_ENTRY]
    assert index == 0
    assert oracle.s16(after, oracle.TIME_OFFSET) == 11
    assert oracle.s16(after, oracle.COUNT_OFFSET) == 1
    assert oracle.s16(after, oracle.POOL_OFFSET) == 0
    assert after[0] == 0
    assert after[1:oracle.POOL_SIZE] == before[1:oracle.POOL_SIZE]

    start = oracle.POOL_SIZE
    assert after[start + 2:start + 4] == before[start + 2:start + 4]
    assert after[start + 0x24:start + oracle.ENTRY_SIZE] == before[
        start + 0x24:start + oracle.ENTRY_SIZE
    ]
    assert after[start + oracle.ENTRY_SIZE:oracle.TIME_OFFSET] == before[
        start + oracle.ENTRY_SIZE:oracle.TIME_OFFSET
    ]
    assert list(after[oracle.CACHE_WARNING_OFFSET:oracle.STRING_WARNING_OFFSET + 1]) == [0, 0]
    assert case["pool"] == b"stale\0"


def test_last_slot_is_valid_and_does_not_set_overflow_warning():
    case, unused_blocks, before, after, events, unused_trace, index = modeled_success(
        "box_deferred_last_slot"
    )
    assert index == oracle.ENTRY_LIMIT - 1 == 511
    assert oracle.s16(after, oracle.COUNT_OFFSET) == oracle.ENTRY_LIMIT
    assert [event["api"] for event in events] == ["game_time_get"]
    assert after[oracle.CACHE_WARNING_OFFSET] == before[oracle.CACHE_WARNING_OFFSET] == 0
    start = oracle.POOL_SIZE + index * oracle.ENTRY_SIZE
    assert before[:start] == after[:start]
    assert struct.unpack_from("<h", after, start)[0] == 7
    assert start + oracle.ENTRY_SIZE == oracle.TIME_OFFSET
    assert case["count"] == 511


def test_full_cache_warns_once_without_touching_entries_or_count():
    case, unused_blocks, before, after, events, trace, index = modeled_success(
        "line_deferred_full_cache_warns"
    )
    assert index is None
    assert trace == [oracle.LINE, oracle.CACHE_ENTRY]
    assert [event["api"] for event in events] == ["game_time_get", "error"]
    assert events[-1] == {
        "api": "error",
        "priority": 2,
        "message": "render debug cache overflow.",
    }
    assert oracle.s16(after, oracle.COUNT_OFFSET) == oracle.ENTRY_LIMIT
    changed = [offset for offset, pair in enumerate(zip(before, after)) if pair[0] != pair[1]]
    assert changed == [oracle.CACHE_WARNING_OFFSET]
    assert after[oracle.CACHE_WARNING_OFFSET] == 1

    already_warned = dict(case, cache_warning=1)
    unused_input, blocks, unused_locations = oracle.input_image(already_warned)
    warned_before = oracle.initial_cache(already_warned)
    warned_after, warned_events, warned_index = oracle.cache_model(
        warned_before, already_warned, blocks
    )
    assert warned_index is None
    assert warned_after == warned_before
    assert [event["api"] for event in warned_events] == ["game_time_get"]


def test_synthetic_captured_outcome_controls_detect_all_mutations_without_side_effects():
    outcomes = {
        (name, 0x037F): synthetic_outcome(name)
        for name in (
            "point_immediate_positive",
            "point_deferred_same_tick",
            "box_immediate_ordinary",
            "line_immediate",
        )
    }
    untouched = copy.deepcopy(outcomes)
    controls = oracle.captured_outcome_negative_controls(outcomes)
    assert set(controls) == {
        "wrong_point_axis",
        "wrong_point_sign",
        "wrong_cache_tag",
        "wrong_box_call_order",
        "wrong_stack_result",
        "wrong_nonvolatile_result",
        "wrong_guard_result",
        "wrong_fpcw_result",
    }
    assert all(controls.values())
    assert outcomes == untouched


def test_comparison_ignores_only_case_label():
    outcome = synthetic_outcome("line_immediate")
    renamed = copy.deepcopy(outcome)
    renamed["case"] = "renamed"
    assert oracle.comparable(outcome) == oracle.comparable(renamed)
    renamed["events"][0]["point0"] = "00" * 12
    assert oracle.comparable(outcome) != oracle.comparable(renamed)


def test_pure_contract_checks_do_not_read_frozen_campaign_files(monkeypatch):
    def forbidden(*unused_args, **unused_kwargs):
        raise AssertionError("unexpected frozen-file access")

    monkeypatch.setattr(Path, "read_bytes", forbidden)
    monkeypatch.setattr(Path, "read_text", forbidden)
    assert len(oracle.cases()) == 22
    assert oracle.point_axis_pairs(oracle.point_bytes((1.0, 2.0, 3.0)), 0.0)
    assert modeled_success("box_immediate_inverted")[-2] is not None
    outcomes = {
        (name, 0x037F): synthetic_outcome(name)
        for name in (
            "point_immediate_positive",
            "point_deferred_same_tick",
            "box_immediate_ordinary",
            "line_immediate",
        )
    }
    assert all(oracle.captured_outcome_negative_controls(outcomes).values())
