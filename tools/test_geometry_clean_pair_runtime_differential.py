"""Fixture-free contract tests for the bounded Geometry clean-pair oracle."""

import copy
from pathlib import Path
import struct

import pytest

pytest.importorskip("unicorn")
from tools.audit import geometry_clean_pair_runtime_differential as oracle


def cases_by_name():
    return {case["name"]: case for case in oracle.all_cases()}


def test_case_population_is_bounded_and_distinct():
    cases = oracle.all_cases()
    assert len(cases) == len(cases_by_name()) == 16
    assert sum(case["function"] == "hull" for case in cases) == 8
    assert sum(case["function"] == "point" for case in cases) == 8
    assert {case["name"] for case in cases if case["function"] == "hull"} == {
        "begin_failure",
        "expand_failure",
        "success",
        "zero_point_count",
        "null_points",
        "null_vertices",
        "null_edges",
        "null_surfaces",
    }
    assert {case["name"] for case in cases if case["function"] == "point"} == {
        "zero_surface_count",
        "nonextant_ignored",
        "inside",
        "outside",
        "epsilon_boundary",
        "nan_is_not_greater",
        "positive_infinity",
        "mixed_extant_break",
    }


@pytest.mark.parametrize(
    "name,event_names,expand_indices,expected_return",
    [
        ("begin_failure", ["_convex_hull3d_begin"], [], 0),
        (
            "expand_failure",
            ["_convex_hull3d_begin", "_convex_hull3d_expand", "_convex_hull3d_expand"],
            [0, 1],
            0,
        ),
        (
            "success",
            [
                "_convex_hull3d_begin",
                "_convex_hull3d_expand",
                "_convex_hull3d_expand",
                "_convex_hull3d_expand",
            ],
            [0, 1, 2],
            1,
        ),
        ("zero_point_count", ["_convex_hull3d_begin"], [], 1),
    ],
)
def test_wrapper_expected_call_order_arguments_and_returns(
    name, event_names, expand_indices, expected_return,
):
    case = cases_by_name()[name]
    events = oracle.expected_events(case)
    assert [event["api"] for event in events] == event_names
    assert events[0]["stack_depth"] == 52
    assert events[0]["args"] == [
        case["point_count"],
        "points",
        case["vertex_count"],
        "vertices",
        case["edge_count"],
        "edges",
        case["surface_count"],
        "surfaces",
    ]
    expands = [event for event in events if event["api"] == "_convex_hull3d_expand"]
    assert [event["args"][-1] for event in expands] == expand_indices
    assert all(event["stack_depth"] == 56 for event in expands)
    assert oracle.expected_return_low(case) == expected_return


@pytest.mark.parametrize(
    "name,expression,line,display_depth,exit_depth",
    [
        ("null_points", "points", 2284, 24, 28),
        ("null_vertices", "vertices", 2285, 36, 40),
        ("null_edges", "edges", 2286, 36, 40),
        ("null_surfaces", "surfaces", 2287, 36, 40),
    ],
)
def test_wrapper_assertions_are_explicit_and_noreturn(
    name, expression, line, display_depth, exit_depth,
):
    case = cases_by_name()[name]
    assert oracle.expected_events(case) == [
        {
            "api": "_display_assert",
            "stack_depth": display_depth,
            "args": [expression, oracle.SOURCE_FILE, line, 1],
        },
        {
            "api": "_system_exit",
            "stack_depth": exit_depth,
            "args": [-1],
        },
    ]
    assert oracle.expected_return_low(case) is None


def test_point_expected_semantics_are_explicit():
    expected = {
        "zero_surface_count": 1,
        "nonextant_ignored": 1,
        "inside": 1,
        "outside": 0,
        "epsilon_boundary": 1,
        "nan_is_not_greater": 1,
        "positive_infinity": 0,
        "mixed_extant_break": 0,
    }
    cases = cases_by_name()
    assert {name: oracle.expected_return_low(cases[name]) for name in expected} == expected
    assert all(oracle.expected_events(cases[name]) == [] for name in expected)


def test_surface_schema_is_28_bytes_with_authenticated_offsets():
    records = [
        (1, 1.25, -2.5, 3.75, -4.5),
        (0, -5.25, 6.5, -7.75, 8.0),
    ]
    raw = oracle.make_surface_bytes(records)
    assert len(raw) == 2 * 28
    for index, record in enumerate(records):
        offset = index * 28
        assert raw[offset] == record[0]
        assert raw[offset + 1:offset + 4] == b"\0\0\0"
        assert struct.unpack_from("<4f", raw, offset + 4) == pytest.approx(record[1:])
        assert struct.unpack_from("<ii", raw, offset + 20) == (1000 + index, 2000 + index)

    case = cases_by_name()["outside"]
    memory, arguments = oracle.prepare_case_memory(case)
    surface_offset = oracle.SURFACES - oracle.DATA
    point_offset = oracle.TEST_POINT - oracle.DATA
    assert memory[surface_offset] == 1
    assert struct.unpack_from("<4f", memory, surface_offset + 4) == pytest.approx(
        (1.0, 0.0, 0.0, 0.0),
    )
    assert struct.unpack_from("<3f", memory, point_offset) == pytest.approx((0.002, 0.0, 0.0))
    assert arguments[6:] == [1, oracle.SURFACES, oracle.TEST_POINT]


def prepared_contract(function_key):
    spec = oracle.FUNCTIONS[function_key]
    return {
        "function_key": function_key,
        "symbol": spec["symbol"],
        "info": {
            "size": spec["size"],
            "relocation_count": spec["relocation_count"],
            "normalized_sha256": spec["normalized_sha256"],
        },
        "epsilon": oracle.EPSILON_BITS,
    }


@pytest.mark.parametrize("function_key", ("hull", "point"))
def test_section_contract_accepts_only_pinned_metadata(function_key):
    assert all(oracle.verify_section(prepared_contract(function_key)).values())


@pytest.mark.parametrize(
    "field,wrong",
    [
        ("symbol", "_wrong_owner"),
        ("size", 1),
        ("relocation_count", 999),
        ("normalized_sha256", "0" * 64),
        ("epsilon", b"\0\0\0\0"),
    ],
)
def test_section_contract_rejects_guard_mutations(field, wrong):
    prepared = prepared_contract("point")
    if field in prepared["info"]:
        prepared["info"][field] = wrong
    else:
        prepared[field] = wrong
    with pytest.raises(AssertionError, match="section guard"):
        oracle.verify_section(prepared)


def test_sha256_guard_uses_only_supplied_file(tmp_path):
    evidence = tmp_path / "synthetic.obj"
    evidence.write_bytes(b"fixture-free synthetic guard")
    expected = oracle.sha256_path(evidence)
    assert oracle.verify_hash(evidence, expected, "synthetic")["sha256"] == expected
    with pytest.raises(AssertionError, match="SHA-256 mismatch"):
        oracle.verify_hash(evidence, "0" * 64, "synthetic")


def valid_outcome(case_name):
    case = cases_by_name()[case_name]
    data, arguments = oracle.prepare_case_memory(case)
    frame = oracle.p32(oracle.STOP) + b"".join(oracle.p32(argument) for argument in arguments)
    return case, {
        "hook_errors": [],
        "execution_error": None,
        "terminal": "return",
        "events": oracle.expected_events(case),
        "data": data,
        "initial_data": data,
        "eax": oracle.expected_return_low(case),
        "eip": oracle.STOP,
        "esp": oracle.ENTRY_SP + 4,
        "eflags": 0x202,
        "registers": {
            oracle.SAVED_REGISTER_NAMES[register]: value
            for register, value in oracle.SAVED_REGISTERS.items()
        },
        "caller_frame": frame,
        "expected_caller_frame": frame,
    }


def test_semantic_validator_accepts_explicit_wrapper_and_point_outcomes():
    for name in ("success", "outside"):
        case, outcome = valid_outcome(name)
        assert oracle.validate_semantics(case, outcome) == []


@pytest.mark.parametrize(
    "mutation",
    (
        "hook_error",
        "execution_error",
        "terminal",
        "event_argument",
        "state",
        "frame",
        "eax",
        "eip",
        "esp",
        "direction_flag",
        "callee_saved",
    ),
)
def test_semantic_validator_rejects_state_and_abi_mutations(mutation):
    case, outcome = valid_outcome("success")
    if mutation == "hook_error":
        outcome["hook_errors"] = ["synthetic"]
    elif mutation == "execution_error":
        outcome["execution_error"] = "synthetic"
    elif mutation == "terminal":
        outcome["terminal"] = "other"
    elif mutation == "event_argument":
        outcome["events"][0]["args"][0] += 1
    elif mutation == "state":
        outcome["data"] = bytes([outcome["data"][0] ^ 1]) + outcome["data"][1:]
    elif mutation == "frame":
        outcome["caller_frame"] = b"changed"
    elif mutation in ("eax", "eip", "esp"):
        outcome[mutation] ^= 1
    elif mutation == "direction_flag":
        outcome["eflags"] |= 0x400
    elif mutation == "callee_saved":
        outcome["registers"]["esi"] ^= 1
    assert oracle.validate_semantics(case, outcome)


def test_all_seven_comparison_negative_controls_detect_changes_without_mutation():
    reference = {"hull": {}, "point": {}}
    for function_key, case_name in (("hull", "success"), ("point", "outside")):
        unused_case, outcome = valid_outcome(case_name)
        reference[function_key][case_name] = oracle.canonical_outcome(outcome)
    untouched = copy.deepcopy(reference)
    result = oracle.run_negative_controls(reference)
    assert result["pass"]
    assert set(result["controls"]) == {
        "external_argument",
        "external_call_order",
        "eax_low_byte",
        "esp",
        "callee_saved",
        "modeled_state",
        "caller_frame",
    }
    assert all(result["controls"].values())
    assert reference == untouched


def test_pure_contract_tests_do_not_read_frozen_files(monkeypatch):
    def forbidden(*unused_args, **unused_kwargs):
        raise AssertionError("unexpected frozen-file access")

    monkeypatch.setattr(Path, "read_bytes", forbidden)
    monkeypatch.setattr(Path, "read_text", forbidden)
    monkeypatch.setattr(Path, "rglob", forbidden)
    assert len(oracle.all_cases()) == 16
    assert len(oracle.make_surface_bytes([(1, 1.0, 0.0, 0.0, 0.0)])) == 28
    assert all(oracle.verify_section(prepared_contract("hull")).values())
    case, outcome = valid_outcome("outside")
    assert oracle.validate_semantics(case, outcome) == []
