"""Negative controls for the bounded ActionObey callback oracle."""
import copy

import pytest

pytest.importorskip("unicorn")
from tools.audit import action_obey_callbacks_runtime_differential as oracle


def advance_outcome():
    case = oracle.advance_cases()[173]
    original = oracle.initial_state("advance", case)
    frame = oracle.callback_frame(case, "advance")
    outcome = {
        "hook_errors": [],
        "execution_error": None,
        "state": oracle.expected_state("advance", case, original),
        "original": original,
        "events": [],
        "terminal": "return",
        "caller_frame": frame,
        "expected_caller_frame": frame,
        "eip": oracle.STOP,
        "esp": oracle.ENTRY_SP + 4,
        "eflags": 0x202,
        "registers": {
            oracle.SAVED_REGISTER_NAMES[register]: value
            for register, value in oracle.SAVED_REGISTERS.items()
        },
    }
    return case, outcome


def test_complete_metadata_domain():
    cases = oracle.advance_cases()
    assert len(cases) == 256
    assert {case["metadata_flags"] for case in cases} == set(range(256))
    for case in cases:
        original = oracle.initial_state("advance", case)
        expected = oracle.expected_state("advance", case, original)
        assert expected["simple"][4] == (case["metadata_flags"] & 0xEF) | 0x08
        assert expected["simple"][:4] == original["simple"][:4]
        assert expected["simple"][5:] == original["simple"][5:]
        for name in ("complex", "user_data", "command_lists"):
            assert expected[name] == original[name]


@pytest.mark.parametrize("fault", ["state", "stack", "df", "nonvolatile", "frame", "calls"])
def test_rejects_bad_outcomes(fault):
    case, outcome = advance_outcome()
    assert oracle.validate_outcome("advance", case, outcome) == []
    if fault == "state":
        data = bytearray(outcome["state"]["simple"])
        data[-1] ^= 1
        outcome["state"]["simple"] = bytes(data)
    elif fault == "stack":
        outcome["esp"] += 4
    elif fault == "df":
        outcome["eflags"] |= 0x400
    elif fault == "nonvolatile":
        outcome["registers"]["ebx"] ^= 1
    elif fault == "frame":
        outcome["caller_frame"] = b"corrupted"
    elif fault == "calls":
        outcome["events"] = [{"api": "unexpected"}]
    assert oracle.validate_outcome("advance", case, outcome)


@pytest.mark.parametrize("callback", list(oracle.CALLBACKS))
def test_target_section_guards_allow_only_proven_aliases_and_fingerprints(callback):
    spec = oracle.CALLBACKS[callback]
    prepared = {
        "callback": callback,
        "symbol": spec["target_symbol"],
        "info": {
            "size": spec["size"],
            "relocation_count": spec["relocation_count"],
            "normalized_sha256": spec["normalized_sha256"],
        },
        "relocation_fingerprint": oracle.EXPECTED_TARGET_RELOCATIONS[callback],
    }
    assert all(oracle.verify_target_function(prepared).values())
    prepared["symbol"] = spec["source_symbol"]
    assert all(oracle.verify_target_function(prepared).values())
    for field in ("symbol", "size", "relocation_count", "normalized_sha256"):
        bad = copy.deepcopy(prepared)
        if field == "symbol":
            bad[field] = "_unproven_alias"
        elif field == "normalized_sha256":
            bad["info"][field] = "0" * 64
        else:
            bad["info"][field] += 1
        with pytest.raises(AssertionError):
            oracle.verify_target_function(bad)
    if spec["relocation_count"]:
        bad = copy.deepcopy(prepared)
        bad["relocation_fingerprint"] = []
        with pytest.raises(AssertionError):
            oracle.verify_target_function(bad)
