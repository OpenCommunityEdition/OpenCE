"""Object-free contract tests for the bounded ActionVehicle control oracle."""

import copy
import hashlib
import tempfile
from pathlib import Path

import pytest

pytest.importorskip("unicorn")
from tools.audit import action_vehicle_control_runtime_differential as oracle


def cases_by_name():
    return {case["name"]: case for case in oracle.cases()}


def test_case_population_is_bounded_and_covers_boolean_and_forwarding_shapes():
    cases = oracle.cases()
    assert len(cases) == len(cases_by_name()) == 18
    assert {case["lock_facing"] for case in cases} == {
        0x00, 0x01, 0x02, 0x10, 0x40, 0x55,
        0x7F, 0x80, 0xAA, 0xFE, 0xFF,
    }
    assert {
        case["path_byte"] for case in cases if not case["lock_facing"]
    } == {0x00, 0x01, 0x02, 0x7F, 0x80, 0xFE, 0xFF}
    assert len({case["handle"] for case in cases}) == 18
    assert 0xFFFFFFFF in {case["handle"] for case in cases}
    assert len(oracle.VECTOR_PATTERNS) == 6
    assert len(oracle.MUTANTS) == 4


def test_hash_guard_accepts_exact_bytes_and_rejects_drift_without_game_files():
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "evidence.bin"
        payload = b"bounded action-vehicle evidence\0"
        path.write_bytes(payload)
        expected = hashlib.sha256(payload).hexdigest()
        result = oracle.verify_hash(path, expected, "synthetic")
        assert result["sha256"] == expected
        assert result["bytes"] == len(payload)
        with pytest.raises(AssertionError, match="SHA-256"):
            oracle.verify_hash(path, "0" * 64, "synthetic")


def write_owner_triplet(directory):
    source = directory / "action_vehicle.c"
    header = directory / "actions.h"
    consumer = directory / "actions.c"
    source.write_text(
        "void action_vehicle_control(\n"
        "\tlong actor_index)\n"
        "{\n"
        "\tstruct actor_datum *actor = actor_get(actor_index);\n"
        "\tif (actor_path_has_path(actor_index)) return;\n"
        "\treturn;\n"
        "}\n",
        encoding="utf-8",
    )
    header.write_text(
        "void action_vehicle_control(\n\tlong actor_index);\n",
        encoding="utf-8",
    )
    consumer.write_text(
        "static void (*const callbacks[])(long) = { &action_vehicle_control, };\n",
        encoding="utf-8",
    )
    return source, header, consumer


def test_owner_source_guard_accepts_real_owner_shape_and_rejects_facades():
    with tempfile.TemporaryDirectory() as directory_name:
        directory = Path(directory_name)
        source, header, consumer = write_owner_triplet(directory)
        assert all(oracle.verify_owner_sources(source, header, consumer).values())

        header.write_text("/* prototype missing */\n", encoding="utf-8")
        with pytest.raises(AssertionError, match="owner source guard"):
            oracle.verify_owner_sources(source, header, consumer)

        source, header, consumer = write_owner_triplet(directory)
        consumer.write_text(
            "void action_vehicle_control(\n\tlong actor_index);\n"
            "static void (*const callbacks[])(long) = { &action_vehicle_control, };\n",
            encoding="utf-8",
        )
        with pytest.raises(AssertionError, match="owner source guard"):
            oracle.verify_owner_sources(source, header, consumer)


def prepared_contract():
    return {
        "role": "synthetic",
        "info": {
            "size": oracle.FUNCTION["size"],
            "relocation_count": oracle.FUNCTION["relocation_count"],
            "normalized_sha256": oracle.FUNCTION["normalized_sha256"],
        },
        "symbol": {
            "storage": oracle.FUNCTION["storage"],
            "type": oracle.FUNCTION["symbol_type"],
        },
        "selection": oracle.FUNCTION["selection"],
        "relocation_fingerprint": copy.deepcopy(oracle.EXPECTED_RELOCATIONS),
        "undefined_dependencies": {
            name: {
                "section": 0,
                "value": 0,
                "storage": 2,
                "symbol_type": 0 if name == "_actor_data" else 0x20,
            }
            for name in ("_actor_data", "_datum_get", "_actor_path_has_path")
        },
    }


def test_section_guard_accepts_only_exact_public_owner_contract():
    assert all(oracle.section_guard(prepared_contract()).values())


@pytest.mark.parametrize(
    "fault",
    [
        "size", "relocation_count", "normalized_sha256", "relocations",
        "storage", "type", "selection", "undefined_dependency",
    ],
)
def test_section_guard_rejects_metadata_and_owner_drift(fault):
    prepared = prepared_contract()
    if fault == "size":
        prepared["info"]["size"] -= 1
    elif fault == "relocation_count":
        prepared["info"]["relocation_count"] -= 1
    elif fault == "normalized_sha256":
        prepared["info"]["normalized_sha256"] = "0" * 64
    elif fault == "relocations":
        prepared["relocation_fingerprint"].pop()
    elif fault == "storage":
        prepared["symbol"]["storage"] = 3
    elif fault == "type":
        prepared["symbol"]["type"] = 0
    elif fault == "selection":
        prepared["selection"] = 2
    else:
        prepared["undefined_dependencies"]["_datum_get"]["section"] = 1
    with pytest.raises(AssertionError, match="section guard"):
        oracle.section_guard(prepared)


def valid_unlocked_path_outcome():
    case = cases_by_name()["unlocked_path_01_01"]
    original = oracle.initial_actor(case)
    caller_window = b"caller-window-sentinel"
    outcome = {
        "hook_errors": [],
        "execution_error": None,
        "events": oracle.expected_events(case),
        "actor_writes": oracle.expected_actor_writes(case),
        "actor": oracle.expected_actor(case, original),
        "original_actor": original,
        "caller_window": caller_window,
        "expected_caller_window": caller_window,
        "eip": oracle.STOP,
        "esp": oracle.ENTRY_SP + 4,
        "eflags": 0x202,
        "saved_registers": {
            oracle.SAVED_REGISTER_NAMES[register]: value
            for register, value in oracle.SAVED_REGISTERS.items()
        },
        "volatile_registers": {
            "eax": 0xA5B6C701,
            "ecx": 0xC5C6C7C8,
            "edx": 0xD5D6D7D8,
        },
    }
    return case, outcome


def test_semantic_validator_accepts_complete_seeded_envelope_contract():
    case, outcome = valid_unlocked_path_outcome()
    assert oracle.validate_outcome(case, outcome) == []


@pytest.mark.parametrize(
    "fault",
    [
        "actor", "write", "api_argument", "api_order", "caller_window",
        "esp", "eip", "df", "nonvolatile",
    ],
)
def test_semantic_validator_rejects_state_write_api_and_abi_drift(fault):
    case, outcome = valid_unlocked_path_outcome()
    if fault == "actor":
        actor = bytearray(outcome["actor"])
        actor[0x700] ^= 1
        outcome["actor"] = bytes(actor)
    elif fault == "write":
        outcome["actor_writes"] = outcome["actor_writes"][:-1]
    elif fault == "api_argument":
        outcome["events"][0]["args"][1] ^= 1
    elif fault == "api_order":
        outcome["events"].reverse()
    elif fault == "caller_window":
        outcome["caller_window"] = b"changed"
    elif fault == "esp":
        outcome["esp"] += 4
    elif fault == "eip":
        outcome["eip"] ^= 1
    elif fault == "df":
        outcome["eflags"] |= 0x400
    else:
        outcome["saved_registers"]["esi"] ^= 1
    assert oracle.validate_outcome(case, outcome)


def test_locked_copy_and_unlocked_direction_preservation_are_explicit():
    locked = next(case for case in oracle.cases() if case["lock_facing"] == 0x80)
    original = oracle.initial_actor(locked)
    expected = oracle.expected_actor(locked, original)
    assert expected[
        oracle.PRIMARY_DIRECTION_VECTOR_OFFSET:
        oracle.PRIMARY_DIRECTION_VECTOR_OFFSET + 12
    ] == original[
        oracle.DESTINATION_FACING_OFFSET:
        oracle.DESTINATION_FACING_OFFSET + 12
    ]
    assert oracle.expected_events(locked) == [{
        "api": "_datum_get",
        "stack_depth": 28,
        "args": [oracle.ACTOR_DATA_VALUE, locked["handle"]],
    }]

    no_path = cases_by_name()["unlocked_path_00_00"]
    original = oracle.initial_actor(no_path)
    expected = oracle.expected_actor(no_path, original)
    preserved = slice(
        oracle.PRIMARY_DIRECTION_TYPE_OFFSET,
        oracle.PRIMARY_DIRECTION_VECTOR_OFFSET + 12,
    )
    assert expected[preserved] == original[preserved]

    high_path = next(
        case for case in oracle.cases()
        if case["lock_facing"] == 0 and case["path_byte"] == 0x80
    )
    original = oracle.initial_actor(high_path)
    expected = oracle.expected_actor(high_path, original)
    assert expected[
        oracle.PRIMARY_DIRECTION_VECTOR_OFFSET:
        oracle.PRIMARY_DIRECTION_VECTOR_OFFSET + 12
    ] == original[
        oracle.PRIMARY_DIRECTION_VECTOR_OFFSET:
        oracle.PRIMARY_DIRECTION_VECTOR_OFFSET + 12
    ]


def test_four_real_byte_mutants_remain_pinned():
    assert oracle.MUTANTS == {
        "locked_priority_5": {"offset": 0x27, "from": 0x04, "to": 0x05},
        "lock_branch_inverted": {"offset": 0x24, "from": 0x74, "to": 0x75},
        "vector_word0_wrong_register": {"offset": 0x48, "from": 0x02, "to": 0x0A},
        "idle_look_type_5": {"offset": 0xA9, "from": 0x04, "to": 0x05},
    }
