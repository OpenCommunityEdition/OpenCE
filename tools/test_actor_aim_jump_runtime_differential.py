"""Fixture-free contract tests for the bounded ActorMoving aim oracle."""

from __future__ import annotations

import copy
import hashlib
import tempfile
from pathlib import Path

import pytest

pytest.importorskip("unicorn")
from tools.audit import actor_aim_jump_runtime_differential as oracle


def cases_by_name() -> dict[str, dict]:
    return {scenario["name"]: scenario for scenario in oracle.cases()}


def test_case_population_is_bounded_and_covers_requested_paths_and_precision_modes():
    scenarios = oracle.cases()
    assert len(scenarios) == len(cases_by_name()) == 22
    assert {scenario["expected_path"] for scenario in scenarios} == {
        "vehicle",
        "swarm",
        "idle",
        "unscaled",
        "scaled",
    }
    assert oracle.FPCW_VALUES == (0x027F, 0x037F)
    assert any(scenario["swarm"] == 0x80 for scenario in scenarios)
    assert any(scenario["leap"] == 0x80 for scenario in scenarios)
    assert any(
        scenario["action"] == oracle.ACTION_CHARGE
        and scenario["goal"] == oracle.CHARGE_GOAL_MELEE_LEAPING
        for scenario in scenarios
    )


def test_hash_guard_accepts_exact_bytes_and_rejects_drift_without_game_files():
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "evidence.bin"
        payload = b"bounded actor-aim evidence\0"
        path.write_bytes(payload)
        expected = hashlib.sha256(payload).hexdigest()
        assert oracle.verify_hash(path, expected, "synthetic")["sha256"] == expected
        with pytest.raises(AssertionError, match="SHA-256"):
            oracle.verify_hash(path, "0" * 64, "synthetic")


def test_three_real_byte_mutants_are_pinned_and_change_only_the_requested_byte():
    assert oracle.MUTANTS == {
        "datum_actor_argument_uses_unit_slot": {
            "offset": 0x0D,
            "from": 0x08,
            "to": 0x0C,
            "case": "vehicle_blocks_targeted",
            "expected_marker": "datum args",
        },
        "general_targeted_clear_writes_one": {
            "offset": 0xEC,
            "from": 0x00,
            "to": 0x01,
            "case": "vehicle_blocks_targeted",
            "expected_marker": "actor_full_envelope",
        },
        "general_return_al_false": {
            "offset": 0xEE,
            "from": 0x01,
            "to": 0x00,
            "case": "vehicle_blocks_targeted",
            "expected_marker": "return_al_true",
        },
    }
    raw = bytearray(0x100)
    for mutation in oracle.MUTANTS.values():
        raw[mutation["offset"]] = mutation["from"]
    for mutation in oracle.MUTANTS.values():
        changed = oracle.mutate_runtime(bytes(raw), mutation)
        differences = [
            index for index, (left, right) in enumerate(zip(raw, changed)) if left != right
        ]
        assert differences == [mutation["offset"]]
        assert changed[mutation["offset"]] == mutation["to"]


def test_mutant_preimage_guard_rejects_wrong_machine_byte():
    raw = bytes(0x100)
    mutation = oracle.MUTANTS["datum_actor_argument_uses_unit_slot"]
    with pytest.raises(AssertionError, match="negative-control preimage"):
        oracle.mutate_runtime(raw, mutation)


def synthetic_run() -> dict:
    return {
        "calls": [{"name": "_datum_get", "args": [0x2468ACE0, 0xA5000001]}],
        "writes": [["actor", oracle.TARGETED_OFFSET, 1, 0]],
        "actor_sha256": "a" * 64,
        "vector_bits": [0x7FC12345, 0xFFC54321, 0x00000001],
        "local_magnitude_bits": 0x3F800000,
        "fpsw": 0,
        "nonvolatile": {"ebp": 1, "ebx": 2, "esi": 3, "edi": 4},
    }


def test_differential_accepts_bit_exact_seeded_nan_words_on_no_work_path():
    target = synthetic_run()
    first = copy.deepcopy(target)
    result = oracle.compare_runs(
        target,
        first,
        {"name": "synthetic_idle", "expected_path": "idle"},
    )
    assert result["bit_exact_vector"]
    assert result["vector_bits"]["ulp"] == [0, 0, 0]


@pytest.mark.parametrize(
    ("fault", "message"),
    [
        ("api_path", "call path differs"),
        ("actor", "actor state differs"),
        ("write_shape", "write shape/order differs"),
        ("fpsw", "x87 status differs"),
        ("nonvolatile", "nonvolatile state differs"),
        ("floating", "floating residual exceeds bound"),
    ],
)
def test_differential_rejects_api_state_write_abi_and_numeric_drift(fault, message):
    target = synthetic_run()
    first = copy.deepcopy(target)
    if fault == "api_path":
        first["calls"].append({"name": "unexpected", "args": []})
    elif fault == "actor":
        first["actor_sha256"] = "b" * 64
    elif fault == "write_shape":
        first["writes"][0][2] = 4
    elif fault == "fpsw":
        first["fpsw"] = 1
    elif fault == "nonvolatile":
        first["nonvolatile"]["esi"] ^= 1
    else:
        first["vector_bits"][0] = 0x7FC1234B
    with pytest.raises(AssertionError, match=message):
        oracle.compare_runs(
            target,
            first,
            {"name": "synthetic_idle", "expected_path": "idle"},
        )


def synthetic_negative_target() -> dict:
    raw = bytearray(0x100)
    for mutation in oracle.MUTANTS.values():
        raw[mutation["offset"]] = mutation["from"]
    return {"role": "target", "runtime_raw": bytes(raw)}


def test_negative_control_runner_requires_each_precise_expected_observation(monkeypatch):
    def detected(_prepared, _scale, _scenario, _fpcw, _seed):
        name = _prepared["role"].split(":", 1)[1]
        marker = oracle.MUTANTS[name]["expected_marker"]
        raise AssertionError((name, marker))

    monkeypatch.setattr(oracle, "execute", detected)
    result = oracle.run_negative_controls(synthetic_negative_target(), {})
    assert result["pass"]
    assert all(
        item["detected_by_expected_check"] for item in result["mutants"].values()
    )


def test_negative_control_runner_does_not_credit_an_unrelated_assertion(monkeypatch):
    def unrelated(*_args, **_kwargs):
        raise AssertionError("unrelated failure")

    monkeypatch.setattr(oracle, "execute", unrelated)
    result = oracle.run_negative_controls(synthetic_negative_target(), {})
    assert not result["pass"]
    assert not any(
        item["detected_by_expected_check"] for item in result["mutants"].values()
    )
