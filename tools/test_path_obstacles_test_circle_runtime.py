"""Fixture-free contract tests for the bounded obstacle-circle replay."""
import hashlib
import struct

from tools.audit import path_obstacles_test_circle_runtime as oracle


def corpus():
    return {case["name"]: case for case in oracle.cases()}


def test_case_population_covers_signed_counts_capacity_and_assertions():
    cases = oracle.cases()
    assert len(cases) == len({case["name"] for case in cases}) == 40
    counts = {case["count"] for case in cases}
    assert {-0x8000, -1, 0, 1, 2, 3, 128, 129, 0x7FFF} <= counts
    assert sum(case["count"] == 128 for case in cases) == 3
    assert sum(case["expected"] == "assert" for case in cases) == 3
    assert all(
        case["null_query"]
        for case in cases
        if case["count"] <= 0 or case["expected"] == "assert"
    )


def test_authentic_disc_stride_last_record_and_full_container_size():
    assert oracle.DISC_SIZE == 0x18
    assert oracle.CONTAINER_HEADER_SIZE + 128 * oracle.DISC_SIZE == \
        oracle.CONTAINER_SIZE == 0xC08
    case = corpus()["capacity_last_record_hit"]
    data = oracle.build_container(case)
    assert len(data) == oracle.CONTAINER_SIZE
    start = oracle.CONTAINER_HEADER_SIZE + 127 * oracle.DISC_SIZE
    record = struct.unpack_from("<HhI4I", data, start)
    assert record[3:6] == (
        case["overrides"][127]["x"],
        case["overrides"][127]["y"],
        case["overrides"][127]["radius"],
    )
    assert start + oracle.DISC_SIZE == len(data)


def test_ignore_argument_uses_only_its_signed_low_short():
    cases = corpus()
    assert oracle.signed_short(cases["ignore_uses_low_zero"]["ignore"]) == 0
    assert oracle.signed_short(
        cases["low_one_does_not_ignore_zero"]["ignore"]
    ) == 1
    assert oracle.signed_short(cases["minus_one_ignore_sentinel"]["ignore"]) == -1
    assert oracle.signed_short(cases["minimum_short_ignore"]["ignore"]) == -0x8000
    assert oracle.modeled_result(cases["ignore_uses_low_zero"]) == -1
    assert oracle.modeled_result(cases["low_one_does_not_ignore_zero"]) == 0


def test_first_hit_ignore_and_tangent_boundaries_are_explicit():
    cases = corpus()
    expected = {
        "first_hit_wins": 0,
        "second_hit_after_first_miss": 1,
        "ignored_first_then_second": 1,
        "ignored_second_first_still_wins": 0,
        "exact_three_four_tangent": 0,
        "one_ulp_inside_tangent": 0,
        "one_ulp_outside_tangent": -1,
        "signed_zero_tangent": 0,
        "capacity_last_record_hit": 127,
        "capacity_last_record_ignored": -1,
    }
    assert {
        name: oracle.modeled_result(cases[name]) for name in expected
    } == expected


def test_selected_nan_infinity_and_negative_radius_semantics_are_not_cleaned():
    cases = corpus()
    for name in (
        "nan_disc_center_misses",
        "nan_query_center_misses",
        "nan_disc_radius_misses",
        "nan_query_radius_misses",
        "infinite_coordinate_finite_radius_misses",
        "infinity_minus_infinity_is_unordered",
        "opposite_infinite_radii_are_unordered",
        "cancelled_radius_subnormal_miss",
    ):
        assert oracle.modeled_result(cases[name]) == -1
    for name in (
        "positive_infinite_disc_radius_hits",
        "negative_infinite_disc_radius_hits",
        "positive_infinite_query_radius_hits",
        "infinite_coordinate_and_radius_compare_equal",
        "negative_radii_square_hit",
        "cancelled_radius_same_center",
    ):
        assert oracle.modeled_result(cases[name]) == 0
    assert struct.unpack("<I", struct.pack("<f", oracle.bits_float(
        oracle.QUIET_NAN)))[0] == oracle.QUIET_NAN


def test_real_assert_contract_and_null_query_avoidance_are_modeled():
    cases = corpus()
    for name in (
        "count_129_asserts",
        "count_129_ignore_zero_still_asserts",
        "count_maximum_short_asserts",
    ):
        assert cases[name]["null_query"]
        assert oracle.modeled_result(cases[name]) == "assert"
    for name in ("count_zero", "count_negative_one", "count_minimum_short"):
        assert cases[name]["null_query"]
        assert oracle.modeled_result(cases[name]) == -1
    assert oracle.ASSERT_LINE == 0x18C
    assert oracle.LITERALS[oracle.FILE][1] == b"c:\\halo\\source\\ai\\path.h\0"
    assert oracle.LITERALS[oracle.DISC_ASSERT][1].endswith(
        b"obstacles->disc_count<=MAXIMUM_DISC_COUNT\0"
    )


def test_input_builders_are_stable_and_modeling_does_not_mutate_them():
    case = corpus()["nan_first_finite_second"]
    before_case = repr(case)
    container = oracle.build_container(case)
    before_hash = hashlib.sha256(container).hexdigest()
    assert oracle.modeled_result(case) == 1
    assert hashlib.sha256(oracle.build_container(case)).hexdigest() == before_hash
    assert repr(case) == before_case


def test_abi_coff_pins_and_in_memory_negative_controls_are_fail_closed():
    assert oracle.ABI_ARGUMENT_OFFSETS == {
        "obstacles": 0x08,
        "ignore_disc_index": 0x0C,
        "center": 0x10,
        "radius": 0x14,
    }
    assert oracle.ABI_RESULT == "signed short in AX"
    assert oracle.FPCWS == (0x027F, 0x037F)
    assert set(oracle.PATHS) == set(oracle.PINS)
    assert all(
        len(value) == 64
        and value == value.lower()
        and not (set(value) - set("0123456789abcdef"))
        for value in oracle.PINS.values()
    )
    assert {
        role: (
            contract["size"],
            contract["relocation_count"],
            len(contract["normalized_sha256"]),
        )
        for role, contract in oracle.SECTION_CONTRACTS.items()
    } == {
        "target": (0xA0, 4, 64),
        "first": (0xA0, 4, 64),
        "actual": (0xA0, 4, 64),
    }
    assert tuple(item[0] for item in oracle.RELOCATION_CONTRACT) == \
        (0x35, 0x3A, 0x3F, 0x46)
    assert [
        (
            mutation["label"],
            mutation["role"],
            mutation["offset"],
            mutation["expected"],
            mutation["replacement"],
            mutation["case"],
        )
        for mutation in oracle.MUTATIONS
    ] == [
        ("invert_hit_branch", "actual", 0x83, 0x74, 0x75, "ordinary_miss"),
        ("subtract_disc_radius", "actual", 0x5B, 0x40, 0x60,
         "radius_sum_only_hit"),
    ]
    cases = corpus()
    assert cases["ordinary_miss"]["expected"] == -1
    assert cases["radius_sum_only_hit"]["expected"] == 0
