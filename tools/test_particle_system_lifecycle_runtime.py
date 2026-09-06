"""Fixture-free tests for the bounded particle-system lifecycle oracle."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import tempfile
from pathlib import Path

import pytest

pytest.importorskip("unicorn")
from tools.audit import particle_system_lifecycle_runtime as oracle


def cases_by_name() -> dict[str, dict]:
    return {case["name"]: case for case in oracle.scenarios()}


def test_five_case_domain_covers_both_entries_and_type_counts_zero_through_four():
    cases = oracle.scenarios()
    assert len(cases) == len(cases_by_name()) == 5
    assert {case["entry"] for case in cases} == {"delete", "reconnect"}
    assert sorted(
        {
            len(system["types"])
            for case in cases
            for system in case["systems"]
        }
    ) == [0, 1, 2, 3, 4]
    assert cases_by_name()["reconnect_empty"]["systems"] == []


def test_reference_delete_chain_preserves_payload_and_orders_every_deletion():
    case = cases_by_name()["delete_types_1_chain"]
    world = oracle.make_reference(case, 0x41)
    deletions = [
        (event["kind"], event["args"][1])
        for event in world.events
        if event["api"] == "datum_delete"
    ]
    assert deletions == [
        ("particles", oracle.particle_handle(0)),
        ("particles", oracle.particle_handle(1)),
        ("particles", oracle.particle_handle(2)),
        ("systems", oracle.system_handle(0)),
    ]
    assert all(
        event["payload_after_identifier_preserved"]
        for event in world.events
        if event["api"] == "datum_delete"
    )
    assert world.pool_metadata() == {
        "systems": {"count": 0, "actual_count": 0, "first_free_absolute_index": 0},
        "particles": {"count": 0, "actual_count": 0, "first_free_absolute_index": 0},
    }


def test_reconnect_count_four_has_exact_head_interior_tail_and_all_invalid_topology():
    case = cases_by_name()["reconnect_types_4_all_splice_shapes"]
    world = oracle.make_reference(case, 0x42)
    assert world.topology() == {
        f"0x{oracle.system_handle(0):08x}": [
            [oracle.particle_handle(1)],
            [oracle.particle_handle(2), oracle.particle_handle(4)],
            [oracle.particle_handle(5)],
            [],
        ]
    }
    assert [
        event["args"][1]
        for event in world.events
        if event["api"] == "datum_delete"
    ] == [
        oracle.particle_handle(0),
        oracle.particle_handle(3),
        oracle.particle_handle(6),
        oracle.particle_handle(7),
        oracle.particle_handle(8),
    ]
    assert world.body_writes == [
        [oracle.SYSTEM_BASE + oracle.SYSTEM_TYPE_HEAD_OFFSET, 4, oracle.particle_handle(1)],
        [oracle.PARTICLE_BASE + 2 * oracle.PARTICLE_STRIDE + oracle.PARTICLE_NEXT_OFFSET, 4, oracle.particle_handle(4)],
        [oracle.PARTICLE_BASE + 5 * oracle.PARTICLE_STRIDE + oracle.PARTICLE_NEXT_OFFSET, 4, oracle.NONE],
        [oracle.SYSTEM_BASE + oracle.SYSTEM_TYPE_HEAD_OFFSET + 3 * oracle.PARTICLE_TYPE_SIZE, 4, oracle.particle_handle(8)],
        [oracle.SYSTEM_BASE + oracle.SYSTEM_TYPE_HEAD_OFFSET + 3 * oracle.PARTICLE_TYPE_SIZE, 4, oracle.NONE],
    ]


def test_deleted_outer_system_handle_advances_to_later_attached_system():
    case = cases_by_name()["reconnect_delete_then_continue_types_3_and_2"]
    world = oracle.make_reference(case, 0x43)
    next_events = [event for event in world.events if event["api"] == "data_next_index"]
    assert next_events[1] == {
        "api": "data_next_index",
        "args": [oracle.PARTICLE_SYSTEMS_POOL, oracle.system_handle(0)],
        "result": oracle.system_handle(2),
    }
    assert world.topology() == {
        f"0x{oracle.system_handle(2):08x}": [
            [oracle.particle_handle(3)],
            [oracle.particle_handle(4)],
        ]
    }
    assert any(event["api"] == "object_get_location" for event in world.events)


def test_datum_delete_model_clears_identifier_only_and_retains_next_handle():
    case = cases_by_name()["delete_types_1_chain"]
    world = oracle.World(case, 0x44)
    address = world.datum_address("particles", oracle.particle_handle(0))
    before = world.read(address, oracle.PARTICLE_SIZE)
    assert world.read_u32(address + oracle.PARTICLE_NEXT_OFFSET) == oracle.particle_handle(1)
    world.datum_delete(oracle.SYSTEM_PARTICLES_POOL, oracle.particle_handle(0))
    after = world.read(address, oracle.PARTICLE_SIZE)
    assert after[:2] == b"\0\0"
    assert after[2:] == before[2:]
    assert world.read_u32(address + oracle.PARTICLE_NEXT_OFFSET) == oracle.particle_handle(1)
    assert world.pool_metadata()["particles"] == {
        "count": 3,
        "actual_count": 2,
        "first_free_absolute_index": 0,
    }


def test_world_rejects_duplicate_slots_and_out_of_domain_type_count():
    duplicate = oracle.scenario(
        "duplicate",
        "reconnect",
        [oracle.system(0, [[oracle.particle(0)], [oracle.particle(0)]])],
    )
    with pytest.raises(AssertionError, match="duplicate particle slot"):
        oracle.World(duplicate, 1)
    too_many = oracle.scenario(
        "too_many",
        "reconnect",
        [oracle.system(0, [[], [], [], [], []])],
    )
    with pytest.raises(AssertionError, match="invalid type count"):
        oracle.World(too_many, 1)


def synthetic_prepared() -> dict:
    delete = bytearray(oracle.FUNCTION_FACTS["delete"]["size"])
    reconnect = bytearray(oracle.FUNCTION_FACTS["reconnect"]["size"])
    for mutation in oracle.MUTANTS.values():
        target = delete if mutation["body"] == "delete" else reconnect
        target[mutation["offset"]] = mutation["from"]
    return {
        "role": "synthetic",
        "delete": {"runtime_raw": bytes(delete)},
        "reconnect": {"runtime_raw": bytes(reconnect)},
    }


@pytest.mark.parametrize("name", sorted(oracle.MUTANTS))
def test_each_mutant_is_one_precise_real_byte_and_preserves_input(name):
    prepared = synthetic_prepared()
    before = copy.deepcopy(prepared)
    mutation = oracle.MUTANTS[name]
    changed = oracle.mutate_prepared(prepared, name, mutation)
    assert prepared == before
    body = mutation["body"]
    differences = [
        index
        for index, (left, right) in enumerate(
            zip(prepared[body]["runtime_raw"], changed[body]["runtime_raw"])
        )
        if left != right
    ]
    assert differences == [mutation["offset"]]
    assert changed[body]["runtime_raw"][mutation["offset"]] == mutation["to"]


def test_mutant_preimage_guard_rejects_unexpected_machine_byte():
    prepared = synthetic_prepared()
    mutation = oracle.MUTANTS["delete_next_field_reads_plus_8"]
    raw = bytearray(prepared["delete"]["runtime_raw"])
    raw[mutation["offset"]] ^= 0xFF
    prepared["delete"]["runtime_raw"] = bytes(raw)
    with pytest.raises(AssertionError, match="mutation preimage"):
        oracle.mutate_prepared(prepared, "bad", mutation)


@pytest.mark.parametrize(
    "name",
    ["delete_next_field_reads_plus_8", "particle_invalid_branch_inverted"],
)
def test_event_mutant_requires_exact_api_and_pool_handle_arguments(name):
    mutation = oracle.MUTANTS[name]
    exact = {
        "validation_errors": [
            {
                "code": mutation["expected_code"],
                "detail": {
                    "expected": {
                        "api": mutation["expected_expected_api"],
                        "args": mutation["expected_expected_args"],
                    },
                    "actual": {
                        "api": mutation["expected_actual_api"],
                        "args": mutation["expected_actual_args"],
                    },
                },
            }
        ]
    }
    assert oracle.negative_observation_matches(exact, mutation)
    wrong = copy.deepcopy(exact)
    wrong["validation_errors"][0]["detail"]["actual"]["args"][1] ^= 1
    assert not oracle.negative_observation_matches(wrong, mutation)
    unrelated = {"validation_errors": [{"code": "arena_state", "detail": {}}]}
    assert not oracle.negative_observation_matches(unrelated, mutation)


def test_deletion_target_mutant_requires_its_specific_hook_message():
    mutation = oracle.MUTANTS["delete_system_argument_uses_system_pointer"]
    exact = {
        "validation_errors": [
            {
                "code": "hook_error",
                "detail": ["_datum_delete: systems invalid handle 0x00400000"],
            }
        ]
    }
    assert oracle.negative_observation_matches(exact, mutation)
    exact["validation_errors"][0]["detail"] = ["unrelated assertion"]
    assert not oracle.negative_observation_matches(exact, mutation)


def test_manifest_and_object_hashes_are_both_required_without_game_fixtures():
    with tempfile.TemporaryDirectory() as directory_name:
        directory = Path(directory_name)
        object_path = directory / "particle_systems.obj"
        object_path.write_bytes(b"synthetic object bytes")
        object_hash = hashlib.sha256(object_path.read_bytes()).hexdigest()
        manifest_path = directory / "capture.json"
        manifest_path.write_text(
            json.dumps(
                {
                    "phase": "synthetic",
                    "objects": {
                        oracle.UNIT: {"path": str(object_path), "sha256": object_hash}
                    },
                }
            ),
            encoding="utf-8",
        )
        manifest_hash = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
        resolved, evidence = oracle.load_manifest_object(
            manifest_path, manifest_hash, "first"
        )
        assert resolved == object_path
        assert evidence["object"]["sha256"] == object_hash
        object_path.write_bytes(b"drift")
        with pytest.raises(AssertionError, match="SHA-256"):
            oracle.load_manifest_object(manifest_path, manifest_hash, "first")


def test_void_function_differential_trace_excludes_volatile_register_observation():
    outcome = {
        "entry": "delete",
        "events": [],
        "body_writes": [],
        "arena_sha256": {},
        "arena_changed_ranges_from_initial": {},
        "pool_metadata": {},
        "surviving_list_topology": {},
        "private_delete_entries": 1,
        "caller_window_sha256": "a" * 64,
        "eip": oracle.STOP,
        "esp": oracle.ENTRY_SP + 4,
        "direction_flag": False,
        "nonvolatile": {},
        "volatile": {"eax": 1, "ecx": 2, "edx": 3},
    }
    first = oracle.canonical_trace(outcome)
    outcome["volatile"] = {"eax": 4, "ecx": 5, "edx": 6}
    assert oracle.canonical_trace(outcome) == first


def test_hash_guard_rejects_drift_without_game_files():
    with tempfile.TemporaryDirectory() as directory_name:
        path = Path(directory_name) / "evidence.bin"
        path.write_bytes(b"particle lifecycle evidence")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        assert oracle.verify_hash(path, digest, "synthetic")["sha256"] == digest
        with pytest.raises(AssertionError, match="SHA-256"):
            oracle.verify_hash(path, "0" * 64, "synthetic")
