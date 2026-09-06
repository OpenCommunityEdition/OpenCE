"""Fixture-free tests for the bounded particle-system natural-init oracle."""

from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path

import pytest

pytest.importorskip("unicorn")
from tools.audit import particle_system_natural_init_runtime as oracle


def cases_by_name() -> dict[str, dict]:
    return {case["name"]: case for case in oracle.cases()}


def record_offset() -> int:
    return oracle.page_offset(oracle.SYSTEM, oracle.SYSTEM_PAGE)


def test_ten_reference_cases_cover_both_entries_and_zero_through_four_types():
    cases = oracle.cases()
    assert len(cases) == len(cases_by_name()) == 10
    assert {case["entry"] for case in cases} == {"initialize", "unattached"}
    assert {len(case["state_counts"]) for case in cases} == {0, 1, 4}
    assert any(case["allocation_none"] for case in cases)
    assert oracle.FPCW_VALUES == (0x027F, 0x037F)


def test_reference_initializer_populates_all_positive_runtime_types_and_updates():
    case = cases_by_name()["initialize_types_4_positive"]
    result = oracle.reference(case, 0x51)
    record = record_offset()

    assert result["return_kind"] == "al"
    assert result["return_value"] == 1
    assert not result["deleted"]
    assert oracle.read_u32(result["system"], record + oracle.SYSTEM_FLAGS) & 2
    for index, duration in enumerate(oracle.DURATION_BITS):
        runtime = record + oracle.SYSTEM_TYPES + index * oracle.RUNTIME_TYPE_STRIDE
        assert oracle.read_u16(result["system"], runtime + oracle.TYPE_STATE_INDEX) == 0
        assert oracle.read_u16(result["system"], runtime + oracle.TYPE_TRANSITION_INDEX) == 0xFFFF
        assert result["system"][runtime + oracle.TYPE_MOVING_FORWARD] == 1
        assert oracle.read_u16(result["system"], runtime + oracle.TYPE_PARTICLE_COUNT) == 0
        assert oracle.read_u32(result["system"], runtime + oracle.TYPE_FIRST_PARTICLE) == 0xFFFFFFFF
        assert oracle.read_u32(result["system"], runtime + oracle.TYPE_TIME_LEFT) == duration
        assert oracle.read_u32(result["system"], runtime + oracle.TYPE_STATE_LENGTH) == duration
    assert result["events"][-1] == {
        "name": "code_0008e7f0",
        "args": [oracle.INITIAL_UPDATE_BITS, oracle.SYSTEM_HANDLE],
    }


def test_reference_initializer_continues_after_zero_state_count_and_returns_false():
    case = cases_by_name()["initialize_mixed_zero_continues"]
    result = oracle.reference(case, 0x52)
    record = record_offset()
    later_runtime = record + oracle.SYSTEM_TYPES + 2 * oracle.RUNTIME_TYPE_STRIDE

    assert result["return_kind"] == "al"
    assert result["return_value"] == 0
    assert not any(event["name"] == "code_0008e7f0" for event in result["events"])
    assert oracle.read_u32(result["system"], later_runtime + oracle.TYPE_TIME_LEFT) == oracle.DURATION_BITS[2]
    assert sum(event["name"] == "real_seed_random_range" for event in result["events"]) == 2


def test_reference_unattached_allocation_none_has_no_hidden_record_mutation():
    case = cases_by_name()["unattached_allocation_none"]
    result = oracle.reference(case, 0x53)

    assert result["return_kind"] == "eax"
    assert result["return_value"] == 0xFFFFFFFF
    assert result["system"] == result["initial"]["system"]
    assert [event["name"] for event in result["events"]] == [
        "body:particle_system_new_unattached",
        "datum_new",
    ]


def test_reference_unattached_success_copies_inputs_lighting_scale_and_both_flags():
    case = cases_by_name()["unattached_one_positive_success"]
    result = oracle.reference(case, 0x54)
    record = record_offset()

    assert result["return_value"] == oracle.SYSTEM_HANDLE
    assert oracle.read_u32(result["system"], record + oracle.SYSTEM_FLAGS) == 3
    assert oracle.read_u32(result["system"], record + oracle.SYSTEM_SCALE) == oracle.SCALE_BITS
    for offset, expected in (
        (oracle.SYSTEM_POSITION, oracle.POSITION_BITS),
        (oracle.SYSTEM_VELOCITY, oracle.VELOCITY_BITS),
        (oracle.SYSTEM_COLOR, oracle.COLOR_BITS),
        (oracle.SYSTEM_LIGHTING, oracle.LIGHT_BITS),
    ):
        actual = tuple(
            oracle.read_u32(result["system"], record + offset + 4 * index)
            for index in range(len(expected))
        )
        assert actual == expected


def test_reference_unattached_failure_deletes_identifier_only_and_returns_none():
    case = cases_by_name()["unattached_mixed_zero_failure_delete"]
    result = oracle.reference(case, 0x55)
    record = record_offset()

    assert result["deleted"]
    assert result["return_kind"] == "eax"
    assert result["return_value"] == 0xFFFFFFFF
    assert oracle.read_u16(result["system"], record) == 0
    assert oracle.read_u32(result["system"], record + oracle.SYSTEM_FLAGS) == 3
    assert result["events"][-1] == {
        "name": "datum_delete",
        "args": ["particle_systems", oracle.SYSTEM_HANDLE],
    }


MUTATIONS = {
    "initializer_wrong_flag_bit": ("initialize", 0x40, 0x02, 0x04),
    "initializer_inverted_zero_state_branch": ("initialize", 0x80, 0x74, 0x75),
    "unattached_skip_failure_delete": ("unattached", 0xAD, 0x75, 0xEB),
}


def synthetic_prepared() -> dict:
    initialize = bytearray(oracle.FUNCTIONS["initialize"]["size"])
    unattached = bytearray(oracle.FUNCTIONS["unattached"]["size"])
    for function, offset, expected, _replacement in MUTATIONS.values():
        body = initialize if function == "initialize" else unattached
        body[offset] = expected
    return {
        "role": "synthetic",
        "initialize": {"runtime_raw": bytes(initialize)},
        "unattached": {"runtime_raw": bytes(unattached)},
    }


@pytest.mark.parametrize("name", sorted(MUTATIONS))
def test_actual_byte_mutations_are_single_byte_and_leave_input_unchanged(name):
    function, offset, expected, replacement = MUTATIONS[name]
    prepared = synthetic_prepared()
    before = copy.deepcopy(prepared)
    changed = oracle.mutated_role(prepared, function, offset, expected, replacement)

    assert prepared == before
    differences = [
        index
        for index, (left, right) in enumerate(
            zip(prepared[function]["runtime_raw"], changed[function]["runtime_raw"])
        )
        if left != right
    ]
    assert differences == [offset]
    assert changed[function]["runtime_raw"][offset] == replacement


def test_actual_byte_mutation_guard_rejects_an_unexpected_preimage():
    prepared = synthetic_prepared()
    with pytest.raises(AssertionError, match="negative control byte guard"):
        oracle.mutated_role(prepared, "initialize", 0x40, 0x03, 0x04)


def test_exact_and_fuzzy_function_pins_are_distinct_and_explicit():
    initialize = oracle.FUNCTIONS["initialize"]
    unattached = oracle.FUNCTIONS["unattached"]

    assert initialize["target_normalized"] != initialize["candidate_normalized"]
    assert initialize["target_addresses"] != initialize["candidate_addresses"]
    assert unattached["target_normalized"] == unattached["candidate_normalized"]
    assert unattached["target_addresses"] == unattached["candidate_addresses"]
    assert oracle.PINNED_HASHES["first"] != oracle.PINNED_HASHES["actual"]


def write_before_manifest(path: Path, target: Path, target_hash: str) -> str:
    path.write_text(
        json.dumps(
            {
                "frozen_files": {
                    "build/split/source/effects/particle_systems.obj": {
                        "path": str(target),
                        "sha256": target_hash,
                    }
                }
            }
        ),
        encoding="utf-8",
    )
    return hashlib.sha256(path.read_bytes()).hexdigest()


def test_before_manifest_enforces_frozen_target_hash_without_private_objects(tmp_path, monkeypatch):
    frozen = tmp_path / "frozen" / "particle_systems.obj"
    frozen.parent.mkdir()
    frozen.write_bytes(b"synthetic frozen target")
    frozen_hash = hashlib.sha256(frozen.read_bytes()).hexdigest()
    manifest = tmp_path / "before.json"
    manifest_hash = write_before_manifest(manifest, frozen, frozen_hash)
    monkeypatch.setitem(oracle.PINNED_HASHES, "target", frozen_hash)

    resolved, expected, guard = oracle.manifest_target(manifest, manifest_hash)
    assert resolved == frozen
    assert expected == frozen_hash
    assert guard["sha256"] == manifest_hash

    frozen.write_bytes(b"drift")
    with pytest.raises(AssertionError, match="sha256"):
        oracle.manifest_target(manifest, manifest_hash)


def test_before_manifest_never_falls_back_to_a_live_object(tmp_path, monkeypatch):
    missing = tmp_path / "capture" / "particle_systems.obj"
    expected_hash = hashlib.sha256(b"expected frozen bytes").hexdigest()
    manifest = tmp_path / "before.json"
    manifest_hash = write_before_manifest(manifest, missing, expected_hash)
    monkeypatch.setitem(oracle.PINNED_HASHES, "target", expected_hash)
    (tmp_path / "particle_systems.obj").write_bytes(b"plausible live fallback")

    with pytest.raises(FileNotFoundError):
        oracle.manifest_target(manifest, manifest_hash)


def test_actual_manifest_enforces_object_record_and_frozen_source(tmp_path, monkeypatch):
    actual = tmp_path / "actual.obj"
    source = tmp_path / "particle_systems.c"
    actual.write_bytes(b"synthetic actual object")
    source.write_bytes(b"synthetic frozen source")
    actual_hash = hashlib.sha256(actual.read_bytes()).hexdigest()
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    monkeypatch.setitem(oracle.PINNED_HASHES, "source", source_hash)
    manifest = tmp_path / "actual.json"
    manifest.write_text(
        json.dumps(
            {
                "objects": {
                    "source/effects/particle_systems": {
                        "path": str(actual),
                        "sha256": actual_hash,
                    }
                },
                "frozen_files": {
                    "source/effects/particle_systems.c": {
                        "path": str(source),
                        "sha256": source_hash,
                    }
                },
            }
        ),
        encoding="utf-8",
    )
    manifest_hash = hashlib.sha256(manifest.read_bytes()).hexdigest()

    guard = oracle.verify_actual_manifest(manifest, manifest_hash, actual, actual_hash)
    assert guard["sha256"] == manifest_hash
    assert guard["source"]["sha256"] == source_hash

    source.write_bytes(b"drift")
    with pytest.raises(AssertionError, match="sha256"):
        oracle.verify_actual_manifest(manifest, manifest_hash, actual, actual_hash)


def test_public_wording_and_stable_default_report_name():
    assert "scratch-only" not in (oracle.__doc__ or "").lower()
    assert oracle.OUTPUT_DEFAULT.name == "particle-system-natural-init-runtime-differential-report.json"
