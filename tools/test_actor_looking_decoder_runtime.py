"""Fixture-free contracts for the bounded ActorLooking runtime oracle."""

from __future__ import annotations

import copy
import hashlib
import re

import pytest

pytest.importorskip("unicorn")
from tools.audit import actor_looking_decoder_runtime as oracle


def cases_by_name() -> dict[str, dict]:
    scenarios = oracle.cases()
    return {scenario["name"]: scenario for scenario in scenarios}


def helper_calls(scenario: dict, role: str) -> list[dict]:
    calls = []
    calls.extend(
        {"name": "_vector_from_points3d", "args": []}
        for _ in range(scenario["vector_calls"])
    )
    calls.extend(
        {"name": "_normalize3d", "args": []}
        for _ in range(scenario["normalize_calls"])
    )
    valid_count = (
        (1 if role == "target" else 2) if scenario["decoder_result_true"] else 0
    )
    calls.extend(
        {"name": "_valid_real_normal3d", "args": []}
        for _ in range(valid_count)
    )
    return calls


def synthetic_normal_run(scenario: dict, role: str = "target", seed: int = 7) -> dict:
    actor_handle = (0xA5100000 | seed) & 0xFFFFFFFF
    is_decoder = scenario["entry"] == "decoder"
    direction_bits = [oracle.f32_bits(value) for value in (0.6, 0.8, 0.0)]
    direction_before = [oracle.f32_bits(value) for value in (9.0, 8.0, 7.0)]
    if is_decoder:
        decoder_entries = [
            {
                "ebx_specification": "specification",
                "edi_direction": "direction",
                "actor_index": actor_handle,
            }
        ]
        nonvolatile = {
            "ebp": oracle.SAVED["ebp"],
            "ebx": oracle.SPEC,
            "esi": oracle.SAVED["esi"],
            "edi": oracle.DIRECTION,
        }
        changes = {
            "actor": [],
            "specification": [],
            "direction": list(range(12)),
            "prop": [],
            "object": [],
            "temporary": [],
            "globals": [],
        }
        writes = [{"region": "direction", "offset": 0, "size": 4, "value": direction_bits[0]}]
        primary_priority = 0
        forced_flag = 0x7D
        forced_direction = direction_before
    else:
        decoder_entries = (
            [
                {
                    "ebx_specification": "actor+0x3ec",
                    "edi_direction": "actor+0x524",
                    "actor_index": actor_handle,
                }
            ]
            if scenario["nested_decoder"]
            else []
        )
        nonvolatile = dict(oracle.SAVED)
        actor_changes = [oracle.FORCED_BY_AIMING]
        if scenario["nested_decoder"]:
            actor_changes.extend(range(oracle.FORCED_DIRECTION, oracle.FORCED_DIRECTION + 12))
        changes = {
            "actor": actor_changes,
            "specification": [],
            "direction": [],
            "prop": [],
            "object": [],
            "temporary": [],
            "globals": [],
        }
        writes = [{"region": "actor", "offset": oracle.FORCED_BY_AIMING, "size": 1, "value": scenario["expected_flag"]}]
        primary_priority = scenario["expected_priority"]
        forced_flag = scenario["expected_flag"]
        forced_direction = (
            direction_before
            if scenario["expected_forced_direction"] == "unchanged"
            else [oracle.f32_bits(value) for value in scenario["expected_forced_direction"]]
        )
    buffer_hashes = {
        name: {"before": name + "-before", "after": name + "-after"}
        for name in changes
    }
    return {
        "role": role,
        "case": scenario["name"],
        "entry": scenario["entry"],
        "fpcw_requested": 0x037F,
        "fpcw_final": 0x037F,
        "fpsw_final": 0,
        "fptag_initial": 0xFFFF,
        "fptag_final": 0xFFFF,
        "x87_top": 0,
        "direction_flag_initial": True,
        "direction_flag_final": True,
        "terminated_by_system_exit": False,
        "eip": oracle.STOP,
        "esp": oracle.ENTRY_SP + 4,
        "al": scenario.get("expected_al", 0) or 0,
        "nonvolatile": nonvolatile,
        "caller_frame_exact": True,
        "api_calls": [{"name": name} for name in scenario["expected_api"]],
        "math_calls": helper_calls(scenario, role),
        "decoder_entries": decoder_entries,
        "writes": writes,
        "invalid_accesses": [],
        "changes": changes,
        "buffer_hashes": buffer_hashes,
        "direction_before_bits": direction_before,
        "direction_after_bits": direction_bits,
        "primary_priority": primary_priority,
        "forced_by_aiming": forced_flag,
        "forced_direction_bits": forced_direction,
    }


def display_call(line: int, fatal: int) -> dict:
    return {
        "name": "_display_assert",
        "info_pointer": "temporary",
        "info": "modeled diagnostic",
        "file": "c:\\halo\\SOURCE\\ai\\actor_looking.c",
        "line": line,
        "fatal": fatal,
    }


def final_assert_call() -> dict:
    return {
        "name": "_csprintf",
        "format": "%s: assert_valid_real_normal3d(%f, %f, %f)",
        "typed_arguments": [
            {"kind": "string", "value": "direction"},
            {"kind": "double", "value": "nan"},
            {"kind": "double", "value": oracle.float_token(0.0)},
            {"kind": "double", "value": oracle.float_token(0.0)},
        ],
    }


def test_case_population_is_bounded_unique_and_covers_requested_paths():
    scenarios = oracle.cases()
    by_name = cases_by_name()
    direct = [scenario for scenario in scenarios if scenario["entry"] == "decoder"]
    public = [scenario for scenario in scenarios if scenario["entry"] == "affect"]
    assert len(scenarios) == len(by_name) == 32
    assert len(direct) == 26
    assert len(public) == 6
    assert {scenario["direction_type"] for scenario in direct if scenario["direction_type"] is not None} >= set(range(7))
    assert oracle.FPCW_VALUES == (0x027F, 0x037F)
    assert {
        "movement_not_moving",
        "prop_missing",
        "target_none",
        "point_zero",
        "danger_none",
        "object_missing",
    } <= by_name.keys()
    assert {
        (scenario["priority"], scenario["nested_decoder"])
        for scenario in public
        if scenario["direction_type"] != oracle.DIRECTION_MOVEMENT
    } == {(2, False), (3, True), (4, True)}


def test_pure_pin_and_coff_contracts_are_complete_without_reading_inputs():
    assert all(re.fullmatch(r"[0-9a-f]{64}", value) for value in oracle.PINNED_HASHES.values())
    assert set(oracle.FUNCTION_FACTS) == {"target", "first", "actual"}
    assert set(oracle.FUNCTION_FACTS["target"]) == {oracle.DECODER, oracle.AFFECT}
    assert oracle.FUNCTION_FACTS["target"][oracle.DECODER][:2] == (1632, 98)
    assert oracle.FUNCTION_FACTS["first"][oracle.DECODER][:2] == (1616, 99)
    assert oracle.FUNCTION_FACTS["actual"] == oracle.FUNCTION_FACTS["first"]
    assert set(oracle.HELPER_FACTS) == set(oracle.HELPERS)
    assert oracle.EXECUTED_HELPERS == {
        "_vector_from_points3d",
        "_normalize3d",
        "_valid_real_normal3d",
    }
    assert set(oracle.APIS) == set(oracle.STUB_ADDRESSES)


def test_four_adjacent_float32_brackets_pin_both_validity_limits():
    by_name = cases_by_name()
    names_and_bits = (
        ("vector_lower_boundary_outside_0x3f7fdf39", 0x3F7FDF39, True),
        ("vector_lower_boundary_inside_0x3f7fdf3a", 0x3F7FDF3A, False),
        ("vector_upper_boundary_inside_0x3f801061", 0x3F801061, False),
        ("vector_upper_boundary_outside_0x3f801062", 0x3F801062, True),
    )
    epsilon = 0.0010000000474974513
    for name, bits, terminated in names_and_bits:
        scenario = by_name[name]
        assert oracle.f32_bits(scenario["spec_vector"][0]) == bits
        assert scenario["spec_vector"][1:] == (0.0, 0.0)
        assert scenario["terminated"] is terminated
    assert 0x3F7FDF3A == 0x3F7FDF39 + 1
    assert 0x3F801062 == 0x3F801061 + 1
    assert abs(oracle.bits_f32(0x3F7FDF39) ** 2 - 1.0) > epsilon
    assert abs(oracle.bits_f32(0x3F7FDF3A) ** 2 - 1.0) < epsilon
    assert abs(oracle.bits_f32(0x3F801061) ** 2 - 1.0) < epsilon
    assert abs(oracle.bits_f32(0x3F801062) ** 2 - 1.0) > epsilon


def test_private_decoder_abi_contract_accepts_ebx_edi_and_stack_actor_then_rejects_drift():
    scenario = cases_by_name()["vector_success"]
    run = synthetic_normal_run(scenario)
    image = object.__new__(oracle.RoleImage)
    image.role = "target"
    image._validate_run(run, scenario, run["decoder_entries"][0]["actor_index"], 0x037F)
    assert run["checks"]["private_entry_abi"]
    assert run["checks"]["decoder_private_registers_preserved"]

    broken = synthetic_normal_run(scenario)
    broken["decoder_entries"][0]["ebx_specification"] = "actor+0x3ec"
    with pytest.raises(AssertionError, match="private_entry_abi"):
        image._validate_run(
            broken,
            scenario,
            broken["decoder_entries"][0]["actor_index"],
            0x037F,
        )


def test_public_caller_contract_requires_real_nested_decoder_abi_and_short_circuits():
    by_name = cases_by_name()
    nested = by_name["affect_vector_priority_3"]
    run = synthetic_normal_run(nested)
    image = object.__new__(oracle.RoleImage)
    image.role = "target"
    image._validate_run(run, nested, run["decoder_entries"][0]["actor_index"], 0x037F)
    assert run["checks"]["public_real_decoder_edge"]
    assert run["checks"]["public_private_abi"]

    broken = synthetic_normal_run(nested)
    actor_handle = broken["decoder_entries"][0]["actor_index"]
    broken["decoder_entries"] = []
    with pytest.raises(AssertionError, match="public_real_decoder_edge"):
        image._validate_run(broken, nested, actor_handle, 0x037F)

    assert not by_name["affect_vector_priority_2"]["nested_decoder"]
    assert not by_name["affect_movement_no_path"]["nested_decoder"]
    assert by_name["affect_movement_no_path"]["expected_priority"] == 0
    assert by_name["affect_movement_has_path"]["expected_priority"] == 4


def test_null_private_arguments_pin_fatal_349_and_350_contracts():
    by_name = cases_by_name()
    specification = by_name["null_specification"]
    direction = by_name["null_direction"]
    assert specification["null_spec"] and not specification["null_direction"]
    assert specification["expected_display"] == [["specification", 349, 1]]
    assert specification["expected_api"] == ["_datum_get", "_display_assert", "_system_exit"]
    assert direction["null_direction"] and not direction["null_spec"]
    assert direction["expected_display"] == [["direction", 350, 1]]
    assert direction["expected_api"] == ["_datum_get", "_display_assert", "_system_exit"]


def test_movement_diagnostic_requires_authentic_x_z_z_arguments():
    scenario = cases_by_name()["movement_infinite_diagnostic"]
    typed = [
        {"kind": "double", "value": "nan"},
        {"kind": "string", "value": ""},
        {"kind": "double", "value": oracle.float_token(11.0)},
        {"kind": "double", "value": oracle.float_token(33.0)},
        {"kind": "double", "value": oracle.float_token(33.0)},
        {"kind": "double", "value": "+inf"},
        {"kind": "double", "value": oracle.float_token(0.0)},
        {"kind": "double", "value": oracle.float_token(0.0)},
    ]
    run = {
        "role": "synthetic",
        "api_calls": [
            {
                "name": "_sprintf",
                "format": "denormalized %f: %smoving (p%f %f %f) (v%f %f %f)",
                "typed_arguments": typed,
            },
            display_call(526, 0),
            final_assert_call(),
            display_call(529, 1),
        ],
    }
    checks = oracle.validate_diagnostic_details(run, scenario, seed=1)
    assert checks["movement_point_x_z_z"]

    broken = copy.deepcopy(run)
    broken["api_calls"][0]["typed_arguments"][3]["value"] = oracle.float_token(22.0)
    with pytest.raises(AssertionError, match="movement_point_x_z_z"):
        oracle.validate_diagnostic_details(broken, scenario, seed=1)


def test_null_prop_diagnostic_requires_invalid_status_and_owner_zero():
    scenario = cases_by_name()["prop_infinite_invalid_second_lookup"]
    seed = 0x23
    actor_handle = (0xA5100000 | seed) & 0xFFFFFFFF
    typed = [
        {"kind": "double", "value": "nan"},
        {"kind": "unsigned", "raw": 0xB6200023, "value": 0xB6200023},
        {"kind": "string", "value": " (invalid)"},
        {"kind": "unsigned", "raw": 0, "value": 0},
        {"kind": "unsigned", "raw": actor_handle, "value": actor_handle},
    ]
    run = {
        "role": "synthetic",
        "api_calls": [
            {
                "name": "_sprintf",
                "format": "denormalized %f: prop 0x%08X%s (actor 0x%08X / us 0x%08X)",
                "typed_arguments": typed,
            },
            display_call(526, 0),
            final_assert_call(),
            display_call(529, 1),
        ],
    }
    checks = oracle.validate_diagnostic_details(run, scenario, seed)
    assert checks["prop_invalid_status"]
    assert checks["prop_null_owner_zero"]
    assert checks["prop_actor_index_forwarded"]

    broken = copy.deepcopy(run)
    broken["api_calls"][0]["typed_arguments"][3]["raw"] = 1
    with pytest.raises(AssertionError, match="prop_null_owner_zero"):
        oracle.validate_diagnostic_details(broken, scenario, seed)


def test_semantic_signature_has_all_required_fields_and_normalizes_observation_only_metadata():
    scenario = cases_by_name()["vector_success"]
    run = synthetic_normal_run(scenario)
    run["api_calls"] = [
        {
            "name": "_sprintf",
            "pointer": "rdata+0x1000",
            "output": "stack+0x20",
            "modeled_rendering": "text",
            "typed_arguments": [
                {
                    "kind": "double",
                    "words": [0, 0x3FF00000],
                    "value": oracle.float_token(1.0),
                }
            ],
        }
    ]
    signature = oracle.semantic_signature(run)
    assert set(signature) == {
        "entry",
        "fpcw",
        "fptag",
        "x87_top",
        "direction_flag",
        "terminated",
        "api_calls",
        "decoder_entries",
        "after_hashes",
        "changes",
        "direction_bits",
        "priority",
        "forced_flag",
        "forced_direction_bits",
        "al",
    }
    assert signature["api_calls"] == [
        {
            "name": "_sprintf",
            "typed_arguments": [
                {"kind": "double", "value": oracle.float_token(1.0)}
            ],
        }
    ]


def test_semantic_comparator_accepts_only_expected_extra_validity_call_and_rejects_state_drift():
    scenario = cases_by_name()["vector_success"]
    target = synthetic_normal_run(scenario, "target")
    candidate = synthetic_normal_run(scenario, "first")
    result = oracle.compare_roles(target, candidate, scenario)
    assert result["semantic_signature_equal"]
    assert result["expected_candidate_extra_validity_call"]
    assert result["target_math_calls"]["_valid_real_normal3d"] == 1
    assert result["candidate_math_calls"]["_valid_real_normal3d"] == 2

    broken = copy.deepcopy(candidate)
    broken["forced_direction_bits"][0] ^= 1
    with pytest.raises(AssertionError, match="semantic differential"):
        oracle.compare_roles(target, broken, scenario)


def test_mutation_helper_enforces_preimage_and_equal_length():
    raw = bytes.fromhex("aa bb cc dd")
    changed = oracle.mutated(raw, 1, bytes.fromhex("bb cc"), bytes.fromhex("11 22"))
    assert changed == bytes.fromhex("aa 11 22 dd")
    assert len(changed) == len(raw)
    with pytest.raises(AssertionError, match="negative-control preimage"):
        oracle.mutated(raw, 1, b"\x00", b"\x11")
    with pytest.raises(AssertionError, match="preserve length"):
        oracle.mutated(raw, 1, b"\xbb", b"\x11\x22")


def test_hash_guard_uses_only_tmp_path_synthetic_fixture_and_rejects_drift(tmp_path):
    path = tmp_path / "synthetic-evidence.bin"
    payload = b"fixture-free actor-looking oracle pin\0"
    path.write_bytes(payload)
    expected = hashlib.sha256(payload).hexdigest()
    result = oracle.verify_hash(path, expected, "synthetic")
    assert result == {"path": str(path), "sha256": expected, "bytes": len(payload)}
    with pytest.raises(AssertionError, match="SHA-256 mismatch"):
        oracle.verify_hash(path, "0" * 64, "synthetic")
