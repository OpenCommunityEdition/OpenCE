"""In-memory contract checks for the bounded inventory runtime oracle."""

import copy
import struct

import pytest

from tools.audit import weapons_inventory_runtime_differential as oracle


def cases_by_name():
    return {case["name"]: case for case in oracle.all_cases()}


def test_case_population_is_bounded_and_distinct():
    cases = oracle.all_cases()
    assert len(cases) == len(cases_by_name()) == 23
    assert {case["magazine_count"] for case in cases} == {0, 1, 2}
    assert sum(case["expected"]["terminal"] == "system_exit" for case in cases) == 2
    assert {case["mode"] for case in cases} == {"same", "equipment"}


@pytest.mark.parametrize("name,field,value", [
    ("same_empty_source_still_handled", "handled", 1),
    ("same_empty_source_still_handled", "rounds_out", 0),
    ("same_capped_source_remains", "source_rounds", [5]),
    ("same_two_magazines_last_write_zero", "rounds_out", 0),
    ("same_two_magazines_repeated_delete_target_behavior", "deletes", 2),
    ("equipment_two_magazines_repeated_delete_target_behavior", "deletes", 2),
    ("equipment_rounds_short_max_local_short_max", "target_rounds", [32767]),
    ("equipment_matching_zero_rounds", "handled", 0),
])
def test_explicit_original_behavior_expectations(name, field, value):
    assert cases_by_name()[name]["expected"][field] == value


def test_contiguous_tag_layout_and_unused_words_are_preserved():
    case = cases_by_name()["equipment_two_magazines_repeated_delete_target_behavior"]
    initial = oracle.build_initial_state(case)
    expected = oracle.expected_state(case, initial)
    assert oracle.MAGAZINE_DEFINITION_STRIDE == oracle.MAGAZINE_DEFINITION_SIZE == 0x70
    assert oracle.AMMUNITION_OBJECT_SIZE == oracle.AMMUNITION_OBJECT_STRIDE == 0x1C
    for index in range(2):
        magazine_offset = index * 0x70
        assert struct.unpack_from("<h", initial["magazine_definitions"], magazine_offset + 8)[0] == 5
        ammunition_offset = index * oracle.AMMUNITION_OBJECT_MAGAZINE_STRIDE
        assert struct.unpack_from("<h", initial["ammunition_objects"], ammunition_offset)[0] == 1
        assert struct.unpack_from("<I", initial["ammunition_objects"], ammunition_offset + 0x18)[0] == oracle.ITEM_DEFINITION_INDEX
    assert expected["ammunition_objects"] == initial["ammunition_objects"]
    assert expected["magazine_definitions"] == initial["magazine_definitions"]


def prepared_contract():
    return {
        "info": {key: oracle.FUNCTION[key] for key in ("size", "relocation_count", "normalized_sha256")},
        "symbol_type": 0x20,
        "storage": 2,
        "selection": 1,
    }


def test_section_contract_accepts_only_authentic_metadata():
    assert all(oracle.verify_section(prepared_contract()).values())


@pytest.mark.parametrize("key,value", [
    ("size", 543),
    ("relocation_count", 18),
    ("normalized_sha256", "0" * 64),
    ("symbol_type", 0),
    ("storage", 3),
    ("selection", 2),
])
def test_section_contract_rejects_mutated_metadata(key, value):
    prepared = prepared_contract()
    destination = prepared["info"] if key in prepared["info"] else prepared
    destination[key] = value
    with pytest.raises(AssertionError, match="section guard"):
        oracle.verify_section(prepared)


def valid_empty_outcome():
    case = cases_by_name()["zero_magazines_sentinel"]
    initial = oracle.build_initial_state(case)
    frame = b"caller-frame-sentinel"
    return case, {
        "hook_errors": [],
        "execution_error": None,
        "terminal": "return",
        "initial": initial,
        "state": oracle.expected_state(case, initial),
        "events": [],
        "al": 0,
        "eip": oracle.STOP,
        "esp": oracle.ENTRY_SP + 4,
        "eflags": 0x202,
        "registers": {oracle.SAVED_REGISTER_NAMES[key]: value for key, value in oracle.SAVED_REGISTERS.items()},
        "caller_frame": frame,
        "expected_caller_frame": frame,
    }


def test_semantic_validator_accepts_explicit_empty_case():
    case, outcome = valid_empty_outcome()
    assert oracle.validate_semantics(case, outcome) == []


@pytest.mark.parametrize("mutation", ["al", "esp", "eip", "df", "esi", "frame", "state", "event"])
def test_semantic_validator_rejects_abi_and_state_mutations(mutation):
    case, outcome = valid_empty_outcome()
    if mutation in ("al", "esp", "eip"):
        outcome[mutation] ^= 1
    elif mutation == "df":
        outcome["eflags"] |= 0x400
    elif mutation == "esi":
        outcome["registers"]["esi"] = 0
    elif mutation == "frame":
        outcome["caller_frame"] = b"changed"
    elif mutation == "state":
        outcome["state"]["rounds_out"] = b"\0" * len(outcome["state"]["rounds_out"])
    else:
        outcome["events"] = [{"api": "_object_delete", "args": [oracle.handle_label(oracle.ITEM_HANDLE)]}]
    assert oracle.validate_semantics(case, outcome)


def test_all_nine_comparison_negative_controls_detect_changes():
    case, outcome = valid_empty_outcome()
    reference = oracle.canonical_outcome(case, outcome)
    reference["events"] = [{"api": "first", "args": ["handle"]}, {"api": "second", "args": []}]
    reference["observed"]["deletes"] = ["item"]
    untouched = copy.deepcopy(reference)
    result = oracle.run_negative_controls({"same_partial_depletes_sound_local": reference})
    assert result["pass"]
    assert len(result["controls"]) == 9
    assert all(result["controls"].values())
    assert reference == untouched
