"""Fixture-free checks of the solid-box actual-x86 oracle's test contracts."""
import json
from pathlib import Path
import re
import struct

import pytest

pytest.importorskip("unicorn")
from tools.audit import render_debug_box_runtime as oracle
from tools.audit import render_debug_three_runtime as original


def modeled(name):
    case = next(case for case in oracle.cases() if case["name"] == name)
    raw, blocks, locations = oracle.input_image(case)
    before = oracle.b.initial_cache(case)
    return case, raw, blocks, locations, before, oracle.expected(case, before, blocks)


def test_private_loader_configuration_does_not_mutate_original_oracle():
    assert oracle.b is not original
    assert original.BOX == "_render_debug_box_outline"
    assert original.FUNCTIONS == (original.POINT, original.LINE, original.BOX,
                                  original.POLYGON, original.CACHE_ENTRY, original.CACHE_STRING)
    assert "_rasterizer_debug_triangle" not in original.STUBS
    assert oracle.QUAD not in original.FUNCTION_ADDRESSES
    assert oracle.QUAD in oracle.b.FUNCTION_ADDRESSES
    assert oracle.TRIANGLE in oracle.b.FUNCTION_ADDRESSES
    assert oracle.b.CACHE_ENTRY not in oracle.b.STUBS
    assert oracle.b.CACHE_STRING not in oracle.b.STUBS


def test_case_matrix_contains_both_modes_and_six_diagnostic_exits():
    cases = oracle.cases()
    assert len(cases) == len({case["name"] for case in cases}) == 23
    assert all(case["function"] == oracle.BOX for case in cases)
    assert {(case["null"], case["immediate"]) for case in cases if case.get("null")} == {
        (name, immediate) for name in ("bounds", "color", "both") for immediate in (False, True)}
    assert oracle.b.FPCW_MODES == (0x027f, 0x037f)


def test_all_embedded_binary_pins_are_complete_sha256_values():
    pins = [oracle.TARGET_PIN, oracle.BASE_PIN] + [item[2] for item in oracle.TARGET_FUNCTIONS.values()]
    assert all(re.fullmatch("[0-9a-f]{64}", pin) for pin in pins)


def test_ordinary_corner_and_triangle_order_is_fixed():
    _, _, blocks, _, before, (after, events, trace, index) = modeled("immediate_ordinary")
    points = [struct.unpack("<3f", point) for point in oracle.box_points(blocks["bounds"])]
    assert points == [(-3., -5., -7.), (4., -5., -7.), (-3., -5., 8.), (4., -5., 8.),
                      (-3., 6., 8.), (4., 6., 8.), (-3., 6., -7.), (4., 6., -7.)]
    expected_indices = ((0, 2, 4), (0, 4, 6), (1, 3, 5), (1, 5, 7),
                        (0, 1, 3), (0, 3, 2), (4, 5, 7), (4, 7, 6),
                        (0, 1, 7), (0, 7, 6), (2, 3, 5), (2, 5, 4))
    assert [tuple(points.index(struct.unpack("<3f", bytes.fromhex(point))) for point in event["points"])
            for event in events] == list(expected_indices)
    assert all(event["color"] == blocks["color"].hex() for event in events)
    assert trace == [oracle.BOX] + [oracle.QUAD, oracle.TRIANGLE, oracle.TRIANGLE] * 6
    assert after == before and index is None


@pytest.mark.parametrize("name", ("inverted", "plane", "line", "point", "signed_zero"))
def test_degenerate_inverted_and_signed_zero_geometry_is_not_repaired(name):
    _, _, blocks, _, before, (after, events, trace, index) = modeled("immediate_" + name)
    assert len(events) == 12 and len(trace) == 19
    assert after == before and index is None
    if name == "signed_zero":
        assert oracle.box_points(blocks["bounds"])[0] == struct.pack("<III", 0x80000000, 0, 0x80000000)
        assert events[0]["points"][0] == struct.pack("<III", 0x80000000, 0, 0x80000000).hex()


@pytest.mark.parametrize("name", ("immediate_overlapping_inputs", "deferred_overlapping_inputs"))
def test_overlap_inputs_are_modeled_from_one_actual_byte_image(name):
    _, raw, blocks, locations, _, _ = modeled(name)
    assert locations["color"] == locations["bounds"] + 4
    assert blocks["bounds"][4:20] == blocks["color"]
    offset = locations["bounds"] - oracle.b.INPUT
    assert raw[offset:offset + 24] == blocks["bounds"]


def test_deferred_box_tag_six_and_full_payload_preserve_padding():
    case, _, blocks, _, before, (after, events, trace, index) = modeled("deferred_same_tick")
    assert index == case["count"] == 9
    start = oracle.b.POOL_SIZE + index * oracle.b.ENTRY_SIZE
    assert after[start:start + 2] == oracle.b.p16(6)
    assert after[start + 2:start + 4] == before[start + 2:start + 4]
    assert after[start + 4:start + 28] == blocks["bounds"]
    assert after[start + 28:start + 44] == blocks["color"]
    assert after[start + 44:start + oracle.b.ENTRY_SIZE] == before[start + 44:start + oracle.b.ENTRY_SIZE]
    assert oracle.b.s16(after, oracle.b.COUNT_OFFSET) == 10
    assert events == [{"api": "game_time_get", "result": 10}]
    assert trace == [oracle.BOX, oracle.b.CACHE_ENTRY]


def test_last_cache_slot_and_once_only_overflow():
    _, _, _, _, _, (after, events, _, index) = modeled("deferred_last_slot")
    assert index == 511 and oracle.b.s16(after, oracle.b.COUNT_OFFSET) == 512
    assert [event["api"] for event in events] == ["game_time_get"]
    _, _, _, _, before, (after, events, _, index) = modeled("deferred_full_warn")
    assert index is None
    assert [i for i, (old, new) in enumerate(zip(before, after)) if old != new] == [oracle.b.CACHE_WARNING_OFFSET]
    assert events[-1] == {"api": "error", "priority": 2, "message": "render debug cache overflow."}
    _, _, _, _, before, (after, events, _, index) = modeled("deferred_full_already_warned")
    assert after == before and index is None
    assert [event["api"] for event in events] == ["game_time_get"]


def test_new_tick_and_signed_low_word_time_contract():
    _, _, _, _, before, (after, events, _, index) = modeled("deferred_new_tick")
    assert index == 0 and [event["result"] for event in events] == [10, 11]
    assert oracle.b.s16(after, oracle.b.TIME_OFFSET) == 11
    assert oracle.b.s16(after, oracle.b.POOL_OFFSET) == 0
    assert after[0] == 0 and after[1:oracle.b.POOL_SIZE] == before[1:oracle.b.POOL_SIZE]
    assert after[oracle.b.CACHE_WARNING_OFFSET:oracle.b.STRING_WARNING_OFFSET + 1] == b"\x01\x01"
    _, _, _, _, _, (after, events, _, index) = modeled("deferred_signed_tick_wrap")
    assert index == 0 and [event["result"] for event in events] == [0x8000, 0x8001]
    assert oracle.b.s16(after, oracle.b.TIME_OFFSET) == -32767
    _, _, _, _, _, (_, events, _, index) = modeled("deferred_same_low_word_tick")
    assert index == 5 and events == [{"api": "game_time_get", "result": 0x18000}]


@pytest.mark.parametrize("missing,line", (("bounds", 776), ("color", 777), ("both", 776)))
def test_assertions_are_fatal_before_either_immediate_or_cache_path(missing, line):
    for mode in (False, True):
        case, _, _, locations, before, (after, events, trace, index) = modeled(f"null_{missing}_{int(mode)}")
        assert after == before and index is None and trace == [oracle.BOX]
        assert events[0]["line"] == line and events[0]["fatal"] == 1
        assert events[0]["information"] == ("bounds" if missing == "both" else missing)
        assert events[1] == {"api": "system_exit", "code": 0xffffffff}
        args = struct.unpack("<3I", oracle.arguments(case, locations))
        assert args[0] == int(mode)
        assert (args[1] == 0) == (missing in ("bounds", "both"))
        assert (args[2] == 0) == (missing in ("color", "both"))


def test_manifest_rejects_unpinned_candidate_before_execution(monkeypatch):
    fake = {"version": 1, "objects": {role: {"path": str(Path.cwd() / (role + ".obj")), "sha256": "0" * 64}
                                      for role in ("target", "first", "actual")}}
    monkeypatch.setattr(Path, "read_text", lambda *args, **kwargs: json.dumps(fake))
    monkeypatch.setattr(Path, "read_bytes", lambda *args, **kwargs: b"not-the-pinned-object")
    with pytest.raises(AssertionError, match="file pin mismatch"):
        oracle.read_manifest(Path("manifest.json"))


def test_model_checks_need_no_private_binary_fixture(monkeypatch):
    def forbidden(*args, **kwargs):
        raise AssertionError("unexpected private fixture access")
    monkeypatch.setattr(Path, "read_bytes", forbidden)
    monkeypatch.setattr(Path, "read_text", forbidden)
    for case in oracle.cases():
        _, blocks, locations = oracle.input_image(case)
        assert len(oracle.arguments(case, locations)) == 12
        before = oracle.b.initial_cache(case)
        assert len(oracle.expected(case, before, blocks)[0]) == oracle.b.CACHE_SIZE
