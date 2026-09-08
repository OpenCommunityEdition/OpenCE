"""Fixture-free contracts for the pinned actual-x86 Weather deletion oracle."""
from collections import Counter
import copy
import json
from pathlib import Path
import re
import subprocess
import sys

import pytest

pytest.importorskip("unicorn")
from tools.audit import weather_delete_runtime as oracle


def modeled(name):
    case = next(case for case in oracle.cases() if case["name"] == name)
    prepared = oracle.prepare(case)
    return case, prepared, oracle.expected(case, prepared)


def test_embedded_pins_are_full_sha256_values():
    pins = [oracle.TARGET_PIN, oracle.BASELINE_PIN] + [packet[2] for packet in oracle.TARGET_PACKETS.values()]
    assert all(re.fullmatch("[0-9a-f]{64}", pin) for pin in pins)


def test_case_matrix_covers_every_actual_body_and_two_explicit_controls():
    cases = oracle.cases()
    assert len(cases) == len({case["name"] for case in cases}) == 59
    assert Counter(case["function"] for case in cases) == {
        "delete": 28, "get": 9, "get_type": 14, "delete_particle": 8}
    assert oracle.FPCW_MODES == (0x027f, 0x037f)
    assert set(oracle.STUBS) == {"_tag_get", "_datum_get", "_datum_delete", "_display_assert", "_system_exit"}
    assert not set(oracle.STUBS) & {name for names in oracle.NAMES.values() for name in names}


@pytest.mark.parametrize("value,wanted", ((0, 0), (3, 3), (0xabcd0003, 3), (0x8000, -32768),
                                         (0x1234ffff, -1), (-1, -1), (0x7fff, 32767)))
def test_short_arguments_keep_the_observed_low_word_contract(value, wanted):
    assert oracle.signed16(value) == wanted


@pytest.mark.parametrize("player", range(4))
def test_public_delete_preserves_every_unrelated_globals_byte(player):
    case, prepared, (pages, events, result, fatal, final_index) = modeled(f"delete_p{player}_eight")
    before, after = prepared["pages"][oracle.GLOBAL_PAGE], pages[oracle.GLOBAL_PAGE]
    system = prepared["system"] - oracle.GLOBAL_PAGE
    allowed = set(range(oracle.GLOBALS - oracle.GLOBAL_PAGE, oracle.GLOBALS - oracle.GLOBAL_PAGE + 2))
    allowed.update(range(system, system + 4))
    for index in range(8):
        at = system + 0x1c + index * oracle.TYPE_SIZE
        allowed.update(range(at + 8, at + 10))
        allowed.update(range(at + 12, at + 16))
        assert oracle.get16(after, at + 8) == 0
        assert oracle.get32(after, at + 12) == oracle.NONE
    changed = {index for index, (old, new) in enumerate(zip(before, after)) if old != new}
    assert changed <= allowed
    assert oracle.get32(after, system) == oracle.NONE
    assert oracle.get16(after, oracle.GLOBALS - oracle.GLOBAL_PAGE) == 3
    assert not fatal and result is None and final_index == 8
    assert sum(event["api"] == "tag_get" for event in events) == 9
    assert len(prepared["chains"]) == 8 and len(prepared["records"]) == 7


@pytest.mark.parametrize("name", ("delete_p0_no_types", "delete_negative_tag_count"))
def test_empty_or_negative_tag_count_still_clears_system_and_argument_home(name):
    _, prepared, (pages, events, result, fatal, final_index) = modeled(name)
    assert [event["api"] for event in events] == ["tag_get"]
    assert pages[oracle.PARTICLE_PAGE] == prepared["pages"][oracle.PARTICLE_PAGE]
    assert oracle.get32(pages[oracle.GLOBAL_PAGE], prepared["system"] - oracle.GLOBAL_PAGE) == oracle.NONE
    assert final_index == 0 and not fatal and result is None


def test_public_argument_slot_is_a_target_proven_mutable_home():
    case, _, (_, _, _, fatal, final_index) = modeled("delete_salted_short_argument")
    assert case["local_index"] == 0xabcd0003
    assert oracle.TARGET_ABI["delete"]["stack"] == ["local_index"]
    assert oracle.TARGET_ABI["delete"]["argument_write"] == "final_type_index"
    assert final_index == 3 and not fatal
    _, _, (_, _, _, fatal, final_index) = modeled("delete_invalid_4")
    assert fatal and final_index is None


@pytest.mark.parametrize("key", ("delete", "get"))
@pytest.mark.parametrize("index", (-32768, -1, 4, 32767))
def test_invalid_system_index_asserts_91_before_tag_or_data_access(key, index):
    _, prepared, (pages, events, result, fatal, final_index) = modeled(f"{key}_invalid_{index}")
    assert events == oracle.assertion_events(91)
    assert pages == prepared["pages"]
    assert fatal and result is None and final_index is None
    assert events[0]["information"] == (
        "local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS")
    assert events[0]["file"] == "c:\\halo\\SOURCE\\effects\\weather_particle_systems.c"


@pytest.mark.parametrize("player", range(4))
def test_private_get_is_a_read_only_pointer_accessor(player):
    _, prepared, (pages, events, result, fatal, final_index) = modeled(f"get_p{player}")
    assert result == oracle.GLOBALS + 4 + player * 0x9c
    assert pages == prepared["pages"] and events == [] and not fatal and final_index is None


@pytest.mark.parametrize("index,count", ((-32768, 8), (-1, 8), (0, 0), (3, 3), (32767, 8)))
def test_get_type_asserts_after_tag_lookup(index, count):
    _, prepared, (pages, events, result, fatal, _) = modeled(f"get_type_invalid_{index}_{count}")
    assert [event["api"] for event in events] == ["tag_get", "display_assert", "system_exit"]
    assert events[0]["index"] == prepared["definition_index"]
    assert events[1:] == oracle.assertion_events(102)
    assert pages == prepared["pages"] and fatal and result is None


def test_salted_get_type_returns_full_pointer_without_mutating_state():
    _, prepared, (pages, events, result, fatal, _) = modeled("get_type_salted_short")
    assert result == prepared["system"] + 0x1c + 7 * 0x10
    assert pages == prepared["pages"] and not fatal
    assert events == [{"api": "tag_get", "tag": oracle.RAIN,
                       "index": prepared["definition_index"], "result": oracle.DEFINITION}]


def test_salted_handles_are_never_truncated_at_engine_boundaries():
    _, prepared, (pages, events, _, _, _) = modeled("delete_p0_mixed")
    wanted_handles = [handle for chain in prepared["chains"] for handle in chain]
    assert len(wanted_handles) == 4 and all(handle > 0xffff for handle in wanted_handles)
    assert len({handle >> 16 for handle in wanted_handles}) == 4
    assert [event["index"] for event in events if event["api"] == "datum_get"] == wanted_handles
    assert [event["index"] for event in events if event["api"] == "datum_delete"] == wanted_handles
    for handle, address in prepared["records"].items():
        at = address - oracle.PARTICLE_PAGE
        assert pages[oracle.PARTICLE_PAGE][at:at + oracle.PARTICLE_SIZE] == b"\xd5" * oracle.PARTICLE_SIZE
        assert pages[oracle.PARTICLE_PAGE][at + oracle.PARTICLE_SIZE:at + 0x80] == (
            prepared["pages"][oracle.PARTICLE_PAGE][at + oracle.PARTICLE_SIZE:at + 0x80])


@pytest.mark.parametrize("player", range(4))
def test_private_delete_head_saves_next_before_poison_and_frees_only_one(player):
    _, prepared, (pages, events, result, fatal, _) = modeled(f"delete_head_p{player}_1")
    first, second = prepared["chains"][2]
    at = prepared["type"] - oracle.GLOBAL_PAGE
    assert oracle.get32(pages[oracle.GLOBAL_PAGE], at + 12) == second
    assert oracle.get16(pages[oracle.GLOBAL_PAGE], at + 8) == 0
    assert [event["api"] for event in events] == ["datum_get", "datum_delete"]
    assert [event["index"] for event in events] == [first, first]
    freed = prepared["records"][first] - oracle.PARTICLE_PAGE
    live = prepared["records"][second] - oracle.PARTICLE_PAGE
    assert pages[oracle.PARTICLE_PAGE][freed + 0x50:freed + 0x54] == b"\xd5" * 4
    assert pages[oracle.PARTICLE_PAGE][live:live + oracle.PARTICLE_SIZE] == (
        prepared["pages"][oracle.PARTICLE_PAGE][live:live + oracle.PARTICLE_SIZE])
    assert result is None and not fatal


def test_both_short_counter_boundaries_follow_machine_word_stores():
    _, _, (pages, _, _, _, _) = modeled("delete_active_count_wrap")
    assert oracle.get16(pages[oracle.GLOBAL_PAGE], oracle.GLOBALS - oracle.GLOBAL_PAGE) == 0x7fff
    _, prepared, (pages, _, _, _, _) = modeled("delete_type_count_wrap")
    assert oracle.get16(pages[oracle.GLOBAL_PAGE], prepared["type"] - oracle.GLOBAL_PAGE + 8) == 0x7fff


def test_explicit_private_abis_and_no_void_eax_assumption():
    assert oracle.TARGET_ABI["get"]["registers"] == {"esi": "local_index"}
    assert oracle.TARGET_ABI["get_type"]["registers"] == {"edi": "system", "esi": "type_index"}
    assert oracle.TARGET_ABI["delete_particle"]["registers"] == {"esi": "type"}
    assert oracle.TARGET_ABI["delete"]["return"] == oracle.TARGET_ABI["delete_particle"]["return"] == "void"
    for key, abi in oracle.TARGET_ABI.items():
        oracle.validate_abi(key, abi)


@pytest.mark.parametrize("change", ("missing_argument", "duplicate_argument", "stack_register", "wrong_return", "wrong_write"))
def test_abi_descriptors_fail_closed(change):
    abi = copy.deepcopy(oracle.TARGET_ABI["get_type"])
    if change == "missing_argument":
        abi["registers"].pop("esi")
    elif change == "duplicate_argument":
        abi["stack"] = ["system"]
    elif change == "stack_register":
        abi["registers"]["esp"] = abi["registers"].pop("esi")
    elif change == "wrong_return":
        abi["return"] = "void"
    else:
        abi["argument_write"] = "final_type_index"
    with pytest.raises(AssertionError):
        oracle.validate_abi("get_type", abi)


def test_only_authenticated_name_mappings_are_normalized():
    original = {"relocations": [{"target": ["symbol", "_bss_0043d590", 4]},
                                {"target": ["symbol", "_code_00092510", 0]},
                                {"target": ["symbol", "_unrelated", 0]}],
                "normalized_sha256": "00" * 32}
    got = oracle.canonical_packet(original)
    assert got["relocations"][0]["target"] == ["symbol", "_weather_particle_system_globals", 4]
    assert got["relocations"][1]["target"] == ["symbol", "_weather_particle_system_get_type", 0]
    assert got["relocations"][2] == original["relocations"][2]
    assert got["normalized_sha256"] == original["normalized_sha256"]
    assert original["relocations"][0]["target"][1] == "_bss_0043d590"


def test_ambiguous_or_missing_owner_never_silently_selects_one():
    symbols = [{"name": name, "section": 1} for name in oracle.NAMES["get"]]
    with pytest.raises(AssertionError, match="ambiguous"):
        oracle.unique_symbol({"symbols": symbols}, oracle.NAMES["get"])
    with pytest.raises(AssertionError, match="missing"):
        oracle.unique_symbol({"symbols": []}, oracle.NAMES["delete"])


def manifest_stub():
    return {"version": 1, "objects": {role: {"path": str(Path.cwd() / (role + ".obj")), "sha256": "0" * 64}
                                      for role in ("target", "baseline", "first", "ordinary")}}


def test_manifest_requires_all_four_roles_for_candidate_claims(monkeypatch):
    value = manifest_stub()
    value["objects"].pop("baseline")
    monkeypatch.setattr(Path, "read_text", lambda *args, **kwargs: json.dumps(value))
    with pytest.raises(AssertionError, match="manifest roles"):
        oracle.read_manifest("manifest.json")


def test_manifest_rejects_unpinned_candidate_before_loading_or_execution(monkeypatch):
    value = manifest_stub()
    monkeypatch.setattr(Path, "read_text", lambda *args, **kwargs: json.dumps(value))
    monkeypatch.setattr(Path, "read_bytes", lambda *args, **kwargs: b"not the pinned object")
    with pytest.raises(AssertionError, match="file pin mismatch"):
        oracle.read_manifest("manifest.json")


def test_incomplete_hash_and_relative_input_path_are_rejected(monkeypatch):
    value = manifest_stub()
    value["objects"]["target"]["sha256"] = "0" * 62
    monkeypatch.setattr(Path, "read_text", lambda *args, **kwargs: json.dumps(value))
    with pytest.raises(AssertionError, match="complete raw object pin"):
        oracle.read_manifest("manifest.json")
    value["objects"]["target"]["path"] = "target.obj"
    with pytest.raises(AssertionError, match="absolute object path"):
        oracle.read_manifest("manifest.json")


def test_model_corpus_never_reads_private_binary_fixtures(monkeypatch):
    def forbidden(*args, **kwargs):
        raise AssertionError("unexpected private fixture access")
    monkeypatch.setattr(Path, "read_bytes", forbidden)
    monkeypatch.setattr(Path, "read_text", forbidden)
    for case in oracle.cases():
        prepared = oracle.prepare(case)
        pages, _, _, _, _ = oracle.expected(case, prepared)
        assert set(pages) == set(prepared["pages"])
        assert all(len(raw) == oracle.PAGE_SIZE for raw in pages.values())


def test_actual_mutation_controls_are_two_specific_instruction_changes(monkeypatch):
    observed = []
    class Rejected:
        def __init__(self, role, obj, names, abis, patches):
            self.patch = patches
            observed.append(patches)
        def run(self, case, control):
            raise AssertionError("datum contract" if "delete" in self.patch else "arena mismatch")
    monkeypatch.setattr(oracle, "Loader", Rejected)
    result = oracle.negative_controls(None, None, None)
    assert observed == [
        {"delete": [(0xc8, b"\x8b\x58\x50", b"\x8b\x58\x4c")]},
        {"delete_particle": [(0x26, b"\x66\xff\x4e\x08", b"\x66\xff\x4e\x0a")]},
    ]
    assert len(result) == 2 and all(row["rejected"] for row in result)


def test_unrelated_mutation_run_failure_does_not_count_as_a_negative_pass(monkeypatch):
    class Broken:
        def __init__(self, *args, **kwargs):
            pass
        def run(self, case, control):
            raise AssertionError("unrelated setup failure")
    monkeypatch.setattr(oracle, "Loader", Broken)
    with pytest.raises(AssertionError, match="unexpected mutant failure"):
        oracle.negative_controls(None, None, None)


def test_optimized_python_is_rejected_before_argument_or_fixture_loading():
    result = subprocess.run([sys.executable, "-O", "-B", str(Path(oracle.__file__)), "--help"],
                            capture_output=True, text=True, check=False)
    assert result.returncode != 0
    assert "cannot run with Python optimization" in result.stderr


@pytest.mark.parametrize("role", ("manifest", "target", "baseline", "first", "ordinary"))
def test_report_cannot_overwrite_any_input_path(role):
    entries = manifest_stub()["objects"]
    manifest = Path.cwd() / "manifest.json"
    output = manifest if role == "manifest" else Path(entries[role]["path"])
    with pytest.raises(AssertionError, match="pinned input"):
        oracle.validate_output(output, manifest, entries)


def test_report_cannot_overwrite_existing_evidence(monkeypatch):
    entries = manifest_stub()["objects"]
    monkeypatch.setattr(Path, "exists", lambda self: True)
    with pytest.raises(AssertionError, match="evidence already exists"):
        oracle.validate_output(Path.cwd() / "old-report.json", "manifest.json", entries)
