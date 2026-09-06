"""Fixture-free contracts for the PathObstacles pill/helper replay."""
import hashlib
import math
import struct

from tools.audit import path_obstacles_test_pill_runtime as oracle


def corpus():
    return {case["name"]: case for case in oracle.cases()}


def models():
    return {case["name"]: oracle.modeled_case(case) for case in oracle.cases()}


def test_corpus_covers_counts_capacity_and_real_assert_paths():
    cases = oracle.cases()
    assert len(cases) == len({case["name"] for case in cases}) == 43
    assert {-0x8000, -1, 0, 1, 2, 3, 128, 129, 0x7FFF} <= {
        case["count"] for case in cases
    }
    assert sum(case["count"] == 128 for case in cases) == 2
    assert sum(case["expected"]["assert"] for case in cases) == 3
    assert all(
        case["null_geometry"]
        for case in cases
        if case["count"] <= 0 or case["expected"]["assert"]
    )


def test_disc_container_and_result_layouts_include_the_last_record():
    assert oracle.DISC_SIZE == 0x18
    assert oracle.CONTAINER_HEADER_SIZE + 128 * oracle.DISC_SIZE == \
        oracle.CONTAINER_SIZE == 0xC08
    assert oracle.RESULT_SIZE == struct.calcsize("<Ihh") == 8
    case = corpus()["capacity_last_record_hit"]
    container = oracle.build_container(case)
    start = 8 + 127 * 0x18
    record = struct.unpack_from("<HhI4I", container, start)
    assert record[1] == 1270
    assert record[3:6] == (
        case["overrides"][127]["x"],
        case["overrides"][127]["y"],
        case["overrides"][127]["radius"],
    )
    assert start + 0x18 == len(container)


def test_nearest_is_strict_ties_preserve_first_and_scratch_keeps_last_hit():
    results = models()
    assert results["nearest_second_replaces_first"]["disc_index"] == 1
    assert results["third_strictly_nearest_wins"]["disc_index"] == 2
    assert results["equal_second_preserves_first"]["disc_index"] == 0
    farther = results["farther_second_does_not_replace"]
    assert farther["disc_index"] == 0
    assert farther["distance"] == oracle.TWO
    assert farther["scratch"] == oracle.FOUR
    tied_cap = results["maximum_distance_tie_is_excluded"]
    assert tied_cap["al"] == 0
    assert tied_cap["disc_index"] == -1
    assert tied_cap["distance"] == tied_cap["scratch"] == oracle.FOUR


def test_ignore_short_optional_byte_and_disc_flag_bit_widths_are_explicit():
    cases = corpus()
    results = models()
    assert oracle.signed_short(cases["ignore_uses_low_zero"]["ignore"]) == 0
    assert oracle.signed_short(cases["low_one_ignores_second"]["ignore"]) == 1
    assert oracle.signed_short(cases["minus_one_ignore_sentinel"]["ignore"]) == -1
    assert cases["optional_low_byte_zero_does_not_skip"][
        "ignore_optional"
    ] & 0xFF == 0
    assert len(results["optional_low_byte_zero_does_not_skip"]["calls"]) == 1
    assert len(results["optional_bit_zero_skipped"]["calls"]) == 0
    assert cases["optional_bit_zero_skipped"]["null_geometry"]
    assert len(results["optional_nonzero_low_byte_skips"]["calls"]) == 0
    assert len(results["optional_policy_ignores_flag_bit_one"]["calls"]) == 1
    assert len(
        results["optional_policy_skips_high_flag_with_bit_zero"]["calls"]
    ) == 0


def test_projection_inside_tangent_and_subnormal_boundaries_are_distinct():
    results = models()
    assert results["origin_inside_with_positive_projection"]["distance"] == \
        oracle.ZERO
    for name in (
        "behind_origin_inside_is_rejected",
        "zero_projection_inside_is_rejected",
        "negative_zero_projection_is_rejected",
        "negative_subnormal_projection_misses",
        "one_ulp_outside_tangent_misses",
    ):
        assert results[name]["al"] == 0
    assert results["minimum_subnormal_projection_hits"]["distance"] == \
        oracle.MINIMUM_SUBNORMAL
    tangent = results["discriminant_zero_tangent"]
    assert tangent["al"] == 1 and tangent["distance"] == oracle.FIVE


def test_nan_and_infinity_paths_preserve_ordered_comparison_semantics():
    results = models()
    for name in (
        "nan_direction_projection_misses",
        "nan_center_projection_misses",
        "nan_combined_radius_misses",
        "infinite_center_with_finite_radius_misses",
        "nan_maximum_distance_rejects_finite_hit",
        "negative_infinite_maximum_rejects_finite_hit",
    ):
        assert results[name]["al"] == 0
    for name in (
        "positive_infinite_radius_hits_at_zero",
        "negative_infinite_radius_hits_at_zero",
    ):
        assert results[name]["al"] == 1
        assert results[name]["distance"] == oracle.ZERO
    infinite_projection = results[
        "infinite_projection_writes_nan_but_public_rejects"
    ]
    assert infinite_projection["al"] == 0
    assert infinite_projection["calls"][0]["return_al"] == 1
    assert infinite_projection["calls"][0]["scratch_after"] == "nan"
    assert results["nan_maximum_distance_rejects_finite_hit"][
        "distance"
    ] == oracle.QUIET_NAN
    assert math.isnan(oracle.bits_float(oracle.QUIET_NAN))


def test_assertions_initialize_result_but_never_call_private_helper():
    cases = corpus()
    results = models()
    for name in (
        "count_129_asserts",
        "count_129_ignore_zero_still_asserts",
        "count_maximum_short_asserts",
    ):
        model = results[name]
        assert model["assert"] and model["calls"] == []
        assert oracle.expected_result_bytes(model) == struct.pack(
            "<Ihh", cases[name]["distance"], -1, -1
        )
    for name in ("count_zero", "count_negative_one", "count_minimum_short"):
        model = results[name]
        assert not model["assert"] and model["al"] == 0 and model["calls"] == []
    assert oracle.ASSERT_LINE == 0x18C
    assert oracle.LITERALS[oracle.FILE][1] == b"c:\\halo\\source\\ai\\path.h\0"


def test_builders_and_semantic_model_do_not_mutate_case_inputs():
    case = corpus()["third_strictly_nearest_wins"]
    before_case = repr(case)
    before_container = oracle.build_container(case)
    before_hash = hashlib.sha256(before_container).hexdigest()
    oracle.modeled_case(case)
    assert repr(case) == before_case
    assert hashlib.sha256(oracle.build_container(case)).hexdigest() == before_hash


def test_public_private_abi_pins_relocations_and_mutations_are_fail_closed():
    assert oracle.PUBLIC_ARGUMENT_OFFSETS == {
        "obstacles": 0x08,
        "ignore_disc_index": 0x0C,
        "point": 0x10,
        "direction": 0x14,
        "radius": 0x18,
        "distance": 0x1C,
        "ignore_optional": 0x20,
        "result": 0x24,
    }
    assert oracle.PUBLIC_RESULT == "boolean in AL"
    assert oracle.PUBLIC_ARGUMENT_BASE == "EBP after prologue"
    assert oracle.DISTANCE_EBP_OFFSET == 0x1C
    assert oracle.DISTANCE_STACK_OFFSET == 0x18
    assert set(oracle.HELPER_ABI) == {"target", "first", "actual"}
    assert all(contract == {
        "center": "edx",
        "radius_stack_offset": 4,
        "point": "ecx",
        "direction": "eax",
        "distance": "esi",
        "result": "boolean in AL",
    } for contract in oracle.HELPER_ABI.values())
    assert set(oracle.PATHS) == set(oracle.PINS)
    assert all(
        len(value) == 64
        and value == value.lower()
        and not set(value) - set("0123456789abcdef")
        for value in oracle.PINS.values()
    )
    assert {
        role: (
            contract["helper_name"],
            contract["helper_storage"],
            contract["public_size"],
            contract["helper_size"],
        )
        for role, contract in oracle.GRAPH_CONTRACTS.items()
    } == {
        "target": ("_code_00050580", 2, 0xF0, 0x90),
        "first": ("_circle_intersect_ray", 3, 0xF0, 0x90),
        "actual": ("_circle_intersect_ray", 3, 0xF0, 0x90),
    }
    assert tuple(item[0] for item in oracle.PUBLIC_FIXED_RELOCATIONS) == \
        (0x52, 0x57, 0x5C, 0x63)
    assert tuple(item[0] for item in oracle.HELPER_RELOCATIONS) == \
        (0x1E, 0x3F, 0x62)
    assert [
        (
            mutation["label"],
            mutation["section"],
            mutation["offset"],
            mutation["expected"],
            mutation["replacement"],
            mutation["detected_by"],
        )
        for mutation in oracle.MUTATIONS
    ] == [
        ("invert_strict_nearest_branch", "public", 0xAF, 0x75, 0x74,
         "result_output"),
        ("reverse_helper_hit_distance_subtraction", "helper", 0x72, 0x6D,
         0x65, "helper_distance"),
        ("test_optional_bit_one_instead_of_zero", "public", 0x7D, 0x01,
         0x02, "helper_call_count"),
    ]
