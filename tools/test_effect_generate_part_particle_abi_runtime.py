"""Fixture-free tests for the bounded effect particle-call ABI oracle."""

import copy
import hashlib
import json
from pathlib import Path
from types import SimpleNamespace

import pytest

pytest.importorskip("unicorn")
from tools.audit import effect_generate_part_particle_abi_runtime as oracle


def test_target_callee_uses_historical_frozen_capture_not_live_split():
    assert oracle.TARGET_CALLEE.relative_to(oracle.ROOT).as_posix() == (
        "scratch/particle-systems-fable-pair-final-20260906/files/"
        "build/split/source/effects/particle_systems.obj"
    )


def valid_run(role="schema", scale_bits=0x3F800000):
    pointer_bias = 0x100 if role == "target" else 0
    nonvolatile = {
        "ebp": 0x1122AABB,
        "ebx": 0x2233BBCC,
        "esi": oracle.PART,
        "edi": 0x4455DDEE,
    }
    return {
        "role": role,
        "calls": [
            {"name": "_get_global_local_random_seed_address", "staged_args": []},
            {
                "name": "_effect_random_translational_velocity",
                "effect_register": oracle.EFFECT,
                "args": [
                    oracle.SEED_ADDRESS,
                    oracle.FORWARD,
                    oracle.FRAME - 0xB4 - pointer_bias,
                    oracle.FRAME - 0x10 - pointer_bias,
                    oracle.LOWER_BITS,
                    oracle.UPPER_BITS,
                    oracle.CONE_BITS,
                    oracle.SCALE_A_FLAGS,
                    oracle.SCALE_B_FLAGS,
                ],
            },
            {
                "name": oracle.CALLEE,
                "args_first_six_words": [
                    oracle.DEFINITION_INDEX,
                    oracle.WORLD,
                    oracle.FRAME - 0x10 - pointer_bias,
                    oracle.FRAME - 0xA8 - pointer_bias,
                    scale_bits,
                    oracle.SEED_ADDRESS,
                ],
                "velocity_bits": list(oracle.EXPECTED_VELOCITY_BITS),
                "color_bits": list(oracle.EXPECTED_COLOR_BITS),
            },
        ],
        "correct_real32_predicate": True,
        "expected_promoted_double_words": oracle.promoted_double_words(scale_bits),
        "final_esp": oracle.FRAME + 8,
        "final_eip": oracle.STOP,
        "final_eax": oracle.STUB_RETURN,
        "final_fpcw": 0x027F,
        "final_fpsw": 0,
        "final_nonvolatile": nonvolatile,
    }


def baseline_run(scale_bits=0x3F800000):
    result = valid_run("before", scale_bits)
    words = oracle.promoted_double_words(scale_bits)
    result["calls"][2]["args_first_six_words"][4:6] = words
    result["correct_real32_predicate"] = words[0] == scale_bits
    return result


def test_case_population_is_bounded_nonzero_and_has_two_disclosed_fpcw_modes():
    assert len(oracle.SCALE_CASES) == len({name for name, unused in oracle.SCALE_CASES}) == 6
    assert all(bits != 0 for unused, bits in oracle.SCALE_CASES)
    assert oracle.FPCW_VALUES == (0x027F, 0x037F)
    assert {name for name, unused in oracle.SCALE_CASES} == {
        "one",
        "negative_two_point_five",
        "point_one",
        "minimum_normal",
        "maximum_finite",
        "negative_zero",
    }


@pytest.mark.parametrize("unused_name,bits", oracle.SCALE_CASES)
def test_promoted_double_negative_witness_is_exact_and_not_real32(unused_name, bits):
    words = oracle.promoted_double_words(bits)
    assert len(words) == 2
    assert words[0] != bits
    assert baseline_run(bits)["calls"][2]["args_first_six_words"][4:6] == words


def test_role_metadata_pins_exact_abi_routes_and_tail_relocations():
    assert oracle.FUNCTION_FACTS["before"]["abi"] == "promoted_double_negative_witness"
    assert oracle.FUNCTION_FACTS["schema"]["abi"] == "real32"
    assert oracle.FUNCTION_FACTS["target"]["abi"] == "real32"
    assert oracle.FUNCTION_FACTS["before"]["tail_relocations"][-1] == [0x3C0, oracle.CALLEE]
    assert oracle.FUNCTION_FACTS["schema"]["tail_relocations"][-1] == [0x3BB, oracle.CALLEE]
    assert oracle.FUNCTION_FACTS["target"]["tail_relocations"][-1] == [0x3CF, oracle.CALLEE]
    assert oracle.CALLEE_FACTS["scale_copy_offset"] == 0x86
    assert oracle.CALLEE_FACTS["scale_copy_bytes"] == "8b5518895014"


def fake_prepared(role, *, normalized_sha256=None, selection=1, abi_bytes=None):
    facts = oracle.FUNCTION_FACTS[role]
    raw = bytearray(facts["size"])
    offsets = {
        "before": (0x3B5, bytes.fromhex("d94510dd1c24")),
        "schema": (0x391, bytes.fromhex("8b4d1051")),
        "target": (0x3A5, bytes.fromhex("8b451050")),
    }
    offset, expected = offsets[role]
    expected = expected if abi_bytes is None else abi_bytes
    raw[offset : offset + len(expected)] = expected
    relocations = [
        {
            "address": address,
            "type": oracle.cc.IMAGE_REL_I386_REL32,
            "target": ["symbol", name, 0],
            "symbolic_target": ["symbol", name, 0],
        }
        for address, name in facts["tail_relocations"]
    ]
    info = {
        "size": facts["size"],
        "relocation_count": facts["relocations"],
        "normalized_sha256": normalized_sha256 or facts["normalized_sha256"],
        "relocations": relocations,
    }
    return {
        "path": Path("synthetic.obj"),
        "object": {},
        "function": SimpleNamespace(info=info),
        "symbol": {"section": facts["section"], "storage": 3, "type": 0x20},
        "raw": bytes(raw),
        "selection": selection,
    }


@pytest.mark.parametrize("role", ("before", "schema", "target"))
def test_prepare_accepts_synthetic_exact_coff_contract(monkeypatch, role):
    monkeypatch.setattr(oracle, "function_section", lambda *unused: fake_prepared(role))
    prepared = oracle.prepare(role, Path("unused.obj"), "unused-hash")
    assert all(prepared["checks"].values())
    assert prepared["tail_relocations"] == oracle.FUNCTION_FACTS[role]["tail_relocations"]


@pytest.mark.parametrize(
    "mutation",
    ("normalized_sha256", "selection", "abi_instruction"),
)
def test_prepare_rejects_synthetic_coff_guard_mutations(monkeypatch, mutation):
    kwargs = {}
    if mutation == "normalized_sha256":
        kwargs["normalized_sha256"] = "0" * 64
    elif mutation == "selection":
        kwargs["selection"] = 2
    else:
        kwargs["abi_bytes"] = b"\x90\x90\x90\x90"
    monkeypatch.setattr(oracle, "function_section", lambda *unused: fake_prepared("schema", **kwargs))
    with pytest.raises(AssertionError):
        oracle.prepare("schema", Path("unused.obj"), "unused-hash")


def fake_callee(raw_mutation=False):
    raw = bytearray(oracle.CALLEE_FACTS["size"])
    offset = oracle.CALLEE_FACTS["scale_copy_offset"]
    value = bytearray.fromhex(oracle.CALLEE_FACTS["scale_copy_bytes"])
    if raw_mutation:
        value[0] ^= 1
    raw[offset : offset + len(value)] = value
    return {
        "function": SimpleNamespace(
            info={
                "size": oracle.CALLEE_FACTS["size"],
                "relocation_count": oracle.CALLEE_FACTS["relocations"],
                "normalized_sha256": oracle.CALLEE_FACTS["normalized_sha256"],
            }
        ),
        "symbol": {"storage": 2, "type": 0x20},
        "raw": bytes(raw),
    }


def test_callee_guard_accepts_dword_scale_load_store_and_rejects_byte_mutation(monkeypatch):
    monkeypatch.setattr(oracle, "function_section", lambda *unused: fake_callee())
    assert all(oracle.verify_callee()["checks"].values())
    monkeypatch.setattr(oracle, "function_section", lambda *unused: fake_callee(True))
    with pytest.raises(AssertionError, match=oracle.CALLEE):
        oracle.verify_callee()


def test_relocation_name_requires_symbol_and_zero_addend():
    valid = {"target": ["symbol", oracle.CALLEE, 0]}
    assert oracle.relocation_name(valid) == oracle.CALLEE
    with pytest.raises(AssertionError, match="unexpected addend"):
        oracle.relocation_name({"target": ["symbol", oracle.CALLEE, 4]})
    with pytest.raises(AssertionError, match="unresolved relocation"):
        oracle.relocation_name({"target": ["internal", 12]})


def test_hash_guard_uses_only_supplied_synthetic_file(tmp_path):
    path = tmp_path / "synthetic.obj"
    path.write_bytes(b"fixture-free effect ABI guard")
    expected = hashlib.sha256(path.read_bytes()).hexdigest()
    assert oracle.verify_hash(path, expected, "synthetic")["sha256"] == expected
    with pytest.raises(AssertionError, match="SHA-256"):
        oracle.verify_hash(path, "0" * 64, "synthetic")


def synthetic_context_files(tmp_path, monkeypatch):
    anchor = '#include "effects/particle_system_definitions.h"\n'
    include = '#include "effects/particle_systems.h"\n'
    prototype = (
        "long particle_system_new_unattached(\n"
        "\tlong definition_index,\n"
        "\treal_point3d const *position,\n"
        "\treal_vector3d const *velocity,\n"
        "\treal_argb_color const *color,\n"
        "\treal scale);"
    )
    content = {
        "before_manifest": "{}",
        "schema_manifest": "{}",
        "before_source": anchor,
        "schema_source": anchor + include,
        "before_header": prototype,
        "schema_header": prototype,
        "runtime_summary": json.dumps(
            {
                "unit_count": 5,
                "section_classification_totals": {
                    "compiler_local_label_spelling_only": 5,
                    "runtime_or_owner_changed_REVIEW": 1,
                    "unchanged": 618,
                },
                "review_required_units": ["source/effects/effects"],
                "forbidden_point_new_units": [],
            }
        ),
        "effects_runtime": json.dumps(
            {
                "section_classification_counts": {
                    "compiler_local_label_spelling_only": 2,
                    "runtime_or_owner_changed_REVIEW": 1,
                    "unchanged": 103,
                },
                "issues": {
                    "runtime_changed": [".text|owners=_effect_generate_part"],
                    "runtime_added": [],
                    "runtime_lost": [],
                    "program_symbol_delta_after_local_names_only": {"added": [], "removed": []},
                    "common_delta": {"added": [], "removed": []},
                    "definition_transition_leads": [],
                },
                "all_sections_added": [],
                "all_sections_lost": [],
                "runtime_changes": {
                    ".text|owners=_effect_allowed_by_environment": {
                        "classification": "compiler_local_label_spelling_only"
                    },
                    ".text|owners=_effect_evaluate_function_integral": {
                        "classification": "compiler_local_label_spelling_only"
                    },
                    ".text|owners=_effect_generate_part": {
                        "classification": "runtime_or_owner_changed_REVIEW"
                    },
                },
                "raw_symbol_inventory_delta": {
                    "added": [{"name": "$L%d" % index} for index in range(11)],
                    "removed": [{"name": "$Lx%d" % index} for index in range(11)],
                },
            }
        ),
    }
    for name, text in content.items():
        path = tmp_path / (name + ".txt")
        path.write_text(text, encoding="utf-8")
        monkeypatch.setattr(oracle, name.upper(), path)
        monkeypatch.setitem(oracle.PINS, name, oracle.sha256_path(path))
    return content


def test_frozen_context_contract_accepts_synthetic_complete_classification(tmp_path, monkeypatch):
    synthetic_context_files(tmp_path, monkeypatch)
    result = oracle.verify_frozen_context()
    assert all(result["source_checks"].values())
    assert all(result["summary_checks"].values())


def test_frozen_context_contract_rejects_classification_mutation(tmp_path, monkeypatch):
    synthetic_context_files(tmp_path, monkeypatch)
    summary = json.loads(oracle.RUNTIME_SUMMARY.read_text(encoding="utf-8"))
    summary["section_classification_totals"]["unchanged"] = 617
    oracle.RUNTIME_SUMMARY.write_text(json.dumps(summary), encoding="utf-8")
    monkeypatch.setitem(oracle.PINS, "runtime_summary", oracle.sha256_path(oracle.RUNTIME_SUMMARY))
    with pytest.raises(AssertionError, match="frozen runtime classification"):
        oracle.verify_frozen_context()


def test_correct_pair_comparator_ignores_local_addresses_but_checks_full_semantics():
    target = valid_run("target")
    schema = valid_run("schema")
    result = oracle.compare_correct(target, schema)
    assert result["equal"]
    assert result["signature"]["scale_bits"] == 0x3F800000


@pytest.mark.parametrize(
    "mutation",
    (
        "call_order",
        "helper_register",
        "helper_scalar",
        "definition",
        "world",
        "velocity",
        "color",
        "scale",
        "eax",
        "esp",
        "eip",
        "nonvolatile",
        "fpcw",
        "x87_top",
    ),
)
def test_correct_pair_comparator_rejects_semantic_state_and_abi_mutations(mutation):
    target = valid_run("target")
    schema = valid_run("schema")
    if mutation == "call_order":
        schema["calls"][0]["name"] = "_synthetic_wrong_first_call"
    elif mutation == "helper_register":
        schema["calls"][1]["effect_register"] ^= 1
    elif mutation == "helper_scalar":
        schema["calls"][1]["args"][4] ^= 1
    elif mutation == "definition":
        schema["calls"][2]["args_first_six_words"][0] ^= 1
    elif mutation == "world":
        schema["calls"][2]["args_first_six_words"][1] ^= 1
    elif mutation == "velocity":
        schema["calls"][2]["velocity_bits"][0] ^= 1
    elif mutation == "color":
        schema["calls"][2]["color_bits"][0] ^= 1
    elif mutation == "scale":
        schema["calls"][2]["args_first_six_words"][4] ^= 1
    elif mutation == "eax":
        schema["final_eax"] ^= 1
    elif mutation == "esp":
        schema["final_esp"] ^= 1
    elif mutation == "eip":
        schema["final_eip"] ^= 1
    elif mutation == "nonvolatile":
        schema["final_nonvolatile"]["ebx"] ^= 1
    elif mutation == "fpcw":
        schema["final_fpcw"] ^= 0x100
    else:
        schema["final_fpsw"] ^= 0x0800
    with pytest.raises(AssertionError, match="semantic signature"):
        oracle.compare_correct(target, schema)


@pytest.mark.parametrize("unused_name,scale_bits", oracle.SCALE_CASES)
def test_baseline_comparator_requires_exact_promoted_double_negative_witness(
    unused_name, scale_bits,
):
    before = baseline_run(scale_bits)
    schema = valid_run("schema", scale_bits)
    result = oracle.compare_baseline_negative(before, schema)
    assert result["pass"]
    assert result["checks"]["baseline_fails_real32_contract"]
    assert result["checks"]["schema_passes_real32_contract"]


def test_five_synthetic_controls_detect_outcome_mutations_without_mutating_input():
    target = valid_run("target")
    schema = valid_run("schema")
    untouched = copy.deepcopy(schema)
    result = oracle.synthetic_negative_controls(target, schema)
    assert result["pass"] and result["count"] == 5
    assert set(result["results"]) == {
        "scale_word",
        "velocity_component",
        "color_component",
        "stack_pointer",
        "saved_ebx",
    }
    assert all(item["detected"] for item in result["results"].values())
    assert all(
        item["classification"] == "synthetic outcome mutation, not a binary mutation"
        for item in result["results"].values()
    )
    assert schema == untouched


def test_pure_contract_checks_do_not_read_frozen_campaign_files(monkeypatch):
    def forbidden(*unused_args, **unused_kwargs):
        raise AssertionError("unexpected frozen-file access")

    monkeypatch.setattr(Path, "read_bytes", forbidden)
    monkeypatch.setattr(Path, "read_text", forbidden)
    assert oracle.promoted_double_words(0x3F800000) == [0, 0x3FF00000]
    assert oracle.compare_correct(valid_run("target"), valid_run("schema"))["equal"]
    assert oracle.compare_baseline_negative(baseline_run(), valid_run())["pass"]
