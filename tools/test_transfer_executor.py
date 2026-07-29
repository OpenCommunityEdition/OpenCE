import copy
import hashlib
import json
import re
import tempfile
import unittest
from pathlib import Path

from tools.coff_compare import build_coff, make_section_raw
from tools.transfer_executor import Refusal, emit_candidate, plan_candidate


CODE_FLAGS = 0x60001020
PROOFS = {
    "sizes_equal": True,
    "meaningful_equal": True,
    "reloc_count_equal": True,
    "reloc_addr_type_equal": True,
    "storage_class_equal": True,
    "prologue_equal": True,
    "nontrivial": True,
    "substitution_consistent": True,
}


class TransferExecutorTests(unittest.TestCase):
    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.repo = self.root / "repo"
        self.targets = self.root / "targets"
        self.base = self.root / "base"
        self.output = self.root / "packet"
        self.repo.mkdir()
        (self.repo / ".git").write_text("gitdir: synthetic\n", encoding="utf-8")
        self.targets.mkdir()
        self.base.mkdir()
        self.atlas_path = self.root / "atlas.json"

    def tearDown(self):
        self.temporary_directory.cleanup()

    @staticmethod
    def _unit_path(root, unit, suffix):
        path = root / (unit + suffix)
        path.parent.mkdir(parents=True, exist_ok=True)
        return path

    def _source(self, unit, text):
        if (
            unit == "source/recipient"
            and not re.search(r"\brecipient\s*\(", text)
        ):
            text = "void recipient(void);\n" + text
        path = self._unit_path(self.repo, unit, ".c")
        path.write_text(text, encoding="utf-8", newline="\n")
        return path

    def _object(
        self,
        unit,
        function,
        relocation_targets,
        *,
        addresses=None,
        meaningful=24,
        storage=2,
    ):
        symbols = [
            {
                "name": function,
                "value": 0,
                "section": 1,
                "type": 0x20,
                "storage": storage,
                "aux_count": 0,
            }
        ]
        for target, target_type in relocation_targets:
            symbols.append(
                {
                    "name": target,
                    "value": 0,
                    "section": 0,
                    "type": target_type,
                    "storage": 2,
                    "aux_count": 0,
                }
            )
        addresses = addresses or [
            8 + index * 4 for index in range(len(relocation_targets))
        ]
        relocations = [
            (
                address,
                index + 1,
                0x14 if target_type == 0x20 else 0x06,
                0,
            )
            for index, (address, (_, target_type)) in enumerate(
                zip(addresses, relocation_targets)
            )
        ]
        raw, relocation_data = make_section_raw(32, relocations)
        raw = bytearray(raw)
        raw[:meaningful] = b"\x55" * meaningful
        raw[meaningful:] = b"\x90" * (32 - meaningful)
        for address, _, _, addend in relocations:
            raw[address:address + 4] = int(addend).to_bytes(
                4, "little", signed=True
            )
        data = build_coff(
            sections=[
                {
                    "name": ".text",
                    "size": 32,
                    "raw_data": bytes(raw),
                    "reloc_data": relocation_data,
                    "flags": CODE_FLAGS,
                }
            ],
            symbols=symbols,
        )
        path = self._unit_path(self.targets, unit, ".obj")
        path.write_bytes(data)
        return path

    def _base_object(self, unit="source/recipient"):
        template = self._object("source/base_fixture", "_existing", [])
        path = self._unit_path(self.base, unit, ".obj")
        path.write_bytes(template.read_bytes())
        return path

    def _pair(
        self,
        *,
        donor_unit="source/donor",
        donor_function="_donor",
        recipient_unit="source/recipient",
        recipient_function="_recipient",
        substitutions=None,
        pair_class="MIXED-SUBST",
        meaningful=24,
    ):
        return {
            "class": pair_class,
            "donor": {"unit": donor_unit, "function": donor_function},
            "recipient": {
                "unit": recipient_unit,
                "function": recipient_function,
                "status": "ABSENT",
            },
            "size": 32,
            "meaningful": meaningful,
            "substitutions": substitutions or [],
            "proof": copy.deepcopy(PROOFS),
            "safe": True,
            "donor_source_resolved": True,
        }

    def _write_atlas(self, pairs):
        self.atlas_path.write_text(
            json.dumps(pairs, indent=2), encoding="utf-8", newline="\n"
        )

    def _plan(self, **overrides):
        arguments = {
            "repo_root": self.repo,
            "atlas_path": self.atlas_path,
            "target_root": self.targets,
            "recipient_unit": "source/recipient",
            "recipient_function": "_recipient",
        }
        arguments.update(overrides)
        return plan_candidate(**arguments)

    def _assert_refusal(self, code, **overrides):
        with self.assertRaises(Refusal) as caught:
            self._plan(**overrides)
        self.assertEqual(caught.exception.code, code)

    def test_00_zero_substitution_non_rasterizer_candidate_first(self):
        pair = self._pair(
            substitutions=[],
            pair_class="IDENTICAL-INCLUDING-TARGETS",
        )
        self._write_atlas([pair])
        donor_source = self._source(
            "source/donor",
            "void donor(void)\n{\n\tshared_call();\n}\n",
        )
        recipient_source = self._source(
            "source/recipient",
            "extern void shared_call(void);\n",
        )
        self._object("source/donor", "_donor", [("_shared_call", 0x20)])
        self._object("source/recipient", "_recipient", [("_shared_call", 0x20)])
        before_donor = donor_source.read_bytes()
        before_recipient = recipient_source.read_bytes()

        result = self._plan()

        self.assertIn("void recipient(void)", result["fragment"])
        self.assertNotIn("void donor(void)", result["fragment"])
        self.assertEqual(result["packet"]["derived_substitutions"]["events"], [])
        self.assertEqual(
            result["packet"]["verdict"], "CANDIDATE_GENERATED_NOT_ACCEPTED"
        )
        self.assertEqual(donor_source.read_bytes(), before_donor)
        self.assertEqual(recipient_source.read_bytes(), before_recipient)

    def test_derives_callee_and_data_substitutions_from_recipient_target(self):
        pair = self._pair(
            substitutions=[
                ["_donor_call", "_recipient_call"],
                ["_donor_data", "_recipient_data"],
            ]
        )
        self._write_atlas([pair])
        self._source(
            "source/donor",
            "void donor(void)\n{\n\tdonor_call();\n\tdonor_data = 0;\n}\n",
        )
        self._source(
            "source/recipient",
            "extern void recipient_call(void);\nextern int recipient_data;\n",
        )
        self._object(
            "source/donor",
            "_donor",
            [("_donor_call", 0x20), ("_donor_data", 0)],
        )
        self._object(
            "source/recipient",
            "_recipient",
            [("_recipient_call", 0x20), ("_recipient_data", 0)],
        )

        result = self._plan()

        self.assertIn("recipient_call();", result["fragment"])
        self.assertIn("recipient_data = 0;", result["fragment"])
        self.assertNotIn("donor_call", result["fragment"])
        self.assertFalse(result["packet"]["matching_credit_granted"])
        self.assertEqual(
            result["packet"]["acceptance_gates"][
                "hardened_cross_name_section_infos_equal"
            ],
            "NOT_RUN_REQUIRED",
        )

    def test_allows_unchanged_constant_relocation_without_source_substitution(self):
        pair = self._pair(
            substitutions=[],
            pair_class="IDENTICAL-INCLUDING-TARGETS",
        )
        self._write_atlas([pair])
        self._source(
            "source/donor",
            "void donor(void)\n{\n\tconsume_real(1.0f);\n}\n",
        )
        self._source(
            "source/recipient",
            "extern void consume_real(float value);\n",
        )
        targets = [("__real@3f800000", 0)]
        self._object("source/donor", "_donor", targets)
        self._object("source/recipient", "_recipient", targets)

        result = self._plan()

        self.assertIn("consume_real(1.0f);", result["fragment"])
        self.assertEqual(result["packet"]["derived_substitutions"]["events"], [])

    def test_refuses_wrong_callee_or_stale_atlas_event(self):
        pair = self._pair(
            substitutions=[["_donor_call", "_wrong_recipient_call"]]
        )
        self._write_atlas([pair])
        self._source(
            "source/donor", "void donor(void)\n{\n\tdonor_call();\n}\n"
        )
        self._source(
            "source/recipient", "extern void recipient_call(void);\n"
        )
        self._object("source/donor", "_donor", [("_donor_call", 0x20)])
        self._object(
            "source/recipient", "_recipient", [("_recipient_call", 0x20)]
        )

        self._assert_refusal("wrong-callee-or-stale-atlas")

    def test_refuses_unchanged_target_when_donor_source_calls_wrong_callee(self):
        pair = self._pair(
            substitutions=[],
            pair_class="IDENTICAL-INCLUDING-TARGETS",
        )
        self._write_atlas([pair])
        self._source(
            "source/donor", "void donor(void)\n{\n\twrong_call();\n}\n"
        )
        self._source(
            "source/recipient", "extern void shared_call(void);\n"
        )
        self._object("source/donor", "_donor", [("_shared_call", 0x20)])
        self._object("source/recipient", "_recipient", [("_shared_call", 0x20)])

        self._assert_refusal("wrong-callee-or-source-mismatch")

    def test_refuses_non_injective_substitution(self):
        pair = self._pair(
            substitutions=[
                ["_donor_call", "_recipient_one"],
                ["_donor_call", "_recipient_two"],
            ]
        )
        self._write_atlas([pair])
        self._source(
            "source/donor",
            "void donor(void)\n{\n\tdonor_call();\n\tdonor_call();\n}\n",
        )
        self._source(
            "source/recipient",
            "extern void recipient_one(void);\n"
            "extern void recipient_two(void);\n",
        )
        self._object(
            "source/donor",
            "_donor",
            [("_donor_call", 0x20), ("_donor_call", 0x20)],
        )
        self._object(
            "source/recipient",
            "_recipient",
            [("_recipient_one", 0x20), ("_recipient_two", 0x20)],
        )

        self._assert_refusal("substitution-non-injective")

    def test_refuses_statement_order_relocation_sequence_change(self):
        pair = self._pair(
            substitutions=[
                ["_donor_one", "_recipient_one"],
                ["_donor_two", "_recipient_two"],
            ]
        )
        self._write_atlas([pair])
        self._source(
            "source/donor",
            "void donor(void)\n{\n\tdonor_one();\n\tdonor_two();\n}\n",
        )
        self._source(
            "source/recipient",
            "extern void recipient_one(void);\n"
            "extern void recipient_two(void);\n",
        )
        self._object(
            "source/donor",
            "_donor",
            [("_donor_one", 0x20), ("_donor_two", 0x20)],
            addresses=[8, 12],
        )
        self._object(
            "source/recipient",
            "_recipient",
            [("_recipient_one", 0x20), ("_recipient_two", 0x20)],
            addresses=[12, 8],
        )

        self._assert_refusal("statement-order-mismatch")

    def test_refuses_stub_even_when_other_proofs_claim_safe(self):
        pair = self._pair(meaningful=16)
        self._write_atlas([pair])
        self._source("source/donor", "void donor(void)\n{\n\treturn;\n}\n")
        self._source("source/recipient", "")
        self._object("source/donor", "_donor", [], meaningful=16)
        self._object("source/recipient", "_recipient", [], meaningful=16)

        self._assert_refusal("stub")

    def test_refuses_ambiguous_donor_definitions(self):
        pair = self._pair()
        self._write_atlas([pair])
        self._source(
            "source/donor",
            "void donor(void) { return; }\nvoid donor(void) { return; }\n",
        )
        self._source("source/recipient", "")
        self._object("source/donor", "_donor", [])
        self._object("source/recipient", "_recipient", [])

        self._assert_refusal("donor-source-ambiguous")

    def test_refuses_source_target_relocation_count_disagreement(self):
        pair = self._pair(
            substitutions=[
                ["_donor_call", "_recipient_call"],
                ["_donor_call", "_recipient_call"],
            ]
        )
        self._write_atlas([pair])
        self._source(
            "source/donor", "void donor(void)\n{\n\tdonor_call();\n}\n"
        )
        self._source(
            "source/recipient", "extern void recipient_call(void);\n"
        )
        self._object(
            "source/donor",
            "_donor",
            [("_donor_call", 0x20), ("_donor_call", 0x20)],
        )
        self._object(
            "source/recipient",
            "_recipient",
            [("_recipient_call", 0x20), ("_recipient_call", 0x20)],
        )

        self._assert_refusal("wrong-callee-or-source-mismatch")

    def test_refuses_missing_explicit_recipient_declaration(self):
        pair = self._pair(
            substitutions=[["_donor_call", "_recipient_call"]]
        )
        self._write_atlas([pair])
        self._source(
            "source/donor", "void donor(void)\n{\n\tdonor_call();\n}\n"
        )
        self._source("source/recipient", "")
        self._object("source/donor", "_donor", [("_donor_call", 0x20)])
        self._object(
            "source/recipient", "_recipient", [("_recipient_call", 0x20)]
        )

        self._assert_refusal("recipient-declaration-missing")

    def test_accepts_only_hash_pinned_verbatim_header_declaration(self):
        pair = self._pair(
            substitutions=[["_donor_call", "_recipient_call"]]
        )
        self._write_atlas([pair])
        self._source(
            "source/donor", "void donor(void)\n{\n\tdonor_call();\n}\n"
        )
        self._source("source/recipient", "")
        header = self.repo / "source" / "recipient_api.h"
        header.write_text(
            "extern void recipient_call(void);\n",
            encoding="utf-8",
            newline="\n",
        )
        declaration = "extern void recipient_call(void);"
        manifest = self.root / "declarations.json"
        manifest.write_text(
            json.dumps(
                {
                    "declarations": [
                        {
                            "symbol": "_recipient_call",
                            "text": declaration,
                            "source": "source/recipient_api.h",
                            "sha256": hashlib.sha256(header.read_bytes()).hexdigest(),
                        }
                    ]
                }
            ),
            encoding="utf-8",
            newline="\n",
        )
        self._object("source/donor", "_donor", [("_donor_call", 0x20)])
        self._object(
            "source/recipient", "_recipient", [("_recipient_call", 0x20)]
        )

        result = self._plan(declaration_manifest=manifest)

        self.assertIn(declaration, result["fragment"])
        self.assertEqual(
            result["packet"]["declarations_inserted"][0]["symbol"],
            "_recipient_call",
        )

    def test_auto_resolves_one_unambiguous_repository_header_declaration(self):
        pair = self._pair(
            substitutions=[["_donor_call", "_recipient_call"]]
        )
        self._write_atlas([pair])
        self._source(
            "source/donor", "void donor(void)\n{\n\tdonor_call();\n}\n"
        )
        self._source("source/recipient", "")
        header = self.repo / "source" / "recipient_api.h"
        header.write_text(
            "void recipient_call(\n\tvoid);\n",
            encoding="utf-8",
            newline="\n",
        )
        self._object("source/donor", "_donor", [("_donor_call", 0x20)])
        self._object(
            "source/recipient", "_recipient", [("_recipient_call", 0x20)]
        )

        result = self._plan()

        inserted = result["packet"]["declarations_inserted"][0]
        self.assertEqual(inserted["source"], "source/recipient_api.h")
        self.assertEqual(
            inserted["sha256"], hashlib.sha256(header.read_bytes()).hexdigest()
        )
        self.assertIn("void recipient_call(", result["fragment"])
        self.assertEqual(
            result["packet"]["review_lints"][0]["code"],
            "generated-declaration-redundancy-review",
        )

    def test_flags_donor_domain_parameter_without_rewriting_it(self):
        pair = self._pair(
            donor_function="_antenna_delete",
            recipient_function="_flag_delete",
            substitutions=[],
            pair_class="IDENTICAL-INCLUDING-TARGETS",
        )
        self._write_atlas([pair])
        self._source(
            "source/donor",
            "void antenna_delete(long antenna_index)\n{\n\treturn;\n}\n",
        )
        self._source(
            "source/recipient",
            "void flag_delete(long flag_index);\n",
        )
        self._object("source/donor", "_antenna_delete", [])
        self._object("source/recipient", "_flag_delete", [])

        result = self._plan(
            recipient_function="_flag_delete",
            donor_function="_antenna_delete",
        )

        self.assertIn("long antenna_index", result["fragment"])
        self.assertEqual(
            result["packet"]["review_lints"][0]["code"],
            "donor-domain-parameter-name",
        )

    def test_refuses_real_donor_against_inline_short_recipient_prototype(self):
        pair = self._pair(
            donor_function="_real_local_random_range",
            recipient_function="_local_random_range",
            substitutions=[["_real_seed_random_range", "_seed_random_range"]],
            pair_class="WRAPPER-CALLEE-SUBST",
        )
        self._write_atlas([pair])
        self._source(
            "source/donor",
            "float real_local_random_range(float lower_bound, float upper_bound)\n"
            "{\n"
            "\treturn real_seed_random_range(lower_bound, upper_bound);\n"
            "}\n",
        )
        self._source("source/recipient", '#include "recipient_api.h"\n')
        (self.repo / "source" / "recipient_api.h").write_text(
            "__inline short local_random_range(\n"
            "\tshort lower_bound,\n"
            "\tshort upper_bound)\n"
            "{\n"
            "\treturn seed_random_range(lower_bound, upper_bound);\n"
            "}\n",
            encoding="utf-8",
            newline="\n",
        )
        self._object(
            "source/donor",
            "_real_local_random_range",
            [("_real_seed_random_range", 0x20)],
        )
        self._object(
            "source/recipient",
            "_local_random_range",
            [("_seed_random_range", 0x20)],
        )

        self._assert_refusal(
            "recipient-prototype-incompatible",
            recipient_function="_local_random_range",
            donor_function="_real_local_random_range",
        )

    def test_refuses_recipient_already_defined(self):
        pair = self._pair()
        self._write_atlas([pair])
        self._source("source/donor", "void donor(void) { return; }\n")
        self._source("source/recipient", "void recipient(void) { return; }\n")
        self._object("source/donor", "_donor", [])
        self._object("source/recipient", "_recipient", [])

        self._assert_refusal("recipient-source-already-defines")

    def test_refuses_incomplete_proof(self):
        pair = self._pair()
        pair["proof"]["prologue_equal"] = False
        self._write_atlas([pair])

        self._assert_refusal("incomplete-proof")

    def test_refuses_ambiguous_pair_selection(self):
        pair = self._pair()
        other = copy.deepcopy(pair)
        other["donor"] = {
            "unit": "source/other_donor",
            "function": "_other_donor",
        }
        self._write_atlas([pair, other])

        self._assert_refusal("ambiguous-pair")

    def test_emit_refuses_output_inside_repository(self):
        pair = self._pair(
            substitutions=[],
            pair_class="IDENTICAL-INCLUDING-TARGETS",
        )
        self._write_atlas([pair])
        self._source("source/donor", "void donor(void) { return; }\n")
        self._source("source/recipient", "")
        self._object("source/donor", "_donor", [])
        self._object("source/recipient", "_recipient", [])
        result = self._plan()

        with self.assertRaises(Refusal) as caught:
            emit_candidate(result, self.repo / "review", self.repo)

        self.assertEqual(caught.exception.code, "output-inside-repository")
        self.assertFalse((self.repo / "review").exists())

    def test_emit_refuses_without_frozen_tu_baseline(self):
        pair = self._pair(
            substitutions=[],
            pair_class="IDENTICAL-INCLUDING-TARGETS",
        )
        self._write_atlas([pair])
        self._source("source/donor", "void donor(void) { return; }\n")
        self._source("source/recipient", "")
        self._object("source/donor", "_donor", [])
        self._object("source/recipient", "_recipient", [])
        result = self._plan()

        with self.assertRaises(Refusal) as caught:
            emit_candidate(result, self.output, self.repo)

        self.assertEqual(caught.exception.code, "frozen-tu-baseline-missing")
        self.assertFalse(self.output.exists())

    def test_emit_writes_only_new_external_review_packet(self):
        pair = self._pair(
            substitutions=[],
            pair_class="IDENTICAL-INCLUDING-TARGETS",
        )
        self._write_atlas([pair])
        donor = self._source("source/donor", "void donor(void) { return; }\n")
        recipient = self._source("source/recipient", "")
        self._object("source/donor", "_donor", [])
        self._object("source/recipient", "_recipient", [])
        self._base_object()
        donor_before = donor.read_bytes()
        recipient_before = recipient.read_bytes()
        result = self._plan(base_root=self.base)

        emit_candidate(result, self.output, self.repo)

        self.assertEqual(
            sorted(path.name for path in self.output.iterdir()),
            [
                "REVIEW.md",
                "candidate.c",
                "candidate.patch",
                "frozen_tu_baseline.json",
                "review_packet.json",
            ],
        )
        self.assertEqual(donor.read_bytes(), donor_before)
        self.assertEqual(recipient.read_bytes(), recipient_before)
        packet = json.loads(
            (self.output / "review_packet.json").read_text(encoding="utf-8")
        )
        self.assertFalse(packet["automatic_integration_performed"])
        self.assertFalse(packet["matching_credit_granted"])
        baseline = json.loads(
            (self.output / "frozen_tu_baseline.json").read_text(encoding="utf-8")
        )
        self.assertTrue(baseline["recipient_absent_in_base"])
        self.assertFalse(baseline["matching_credit_granted"])

    def test_emit_refuses_if_an_input_changes_after_planning(self):
        pair = self._pair(
            substitutions=[],
            pair_class="IDENTICAL-INCLUDING-TARGETS",
        )
        self._write_atlas([pair])
        self._source("source/donor", "void donor(void) { return; }\n")
        recipient = self._source("source/recipient", "")
        self._object("source/donor", "_donor", [])
        self._object("source/recipient", "_recipient", [])
        self._base_object()
        result = self._plan(base_root=self.base)
        recipient.write_text("/* changed */\n", encoding="utf-8", newline="\n")

        with self.assertRaises(Refusal) as caught:
            emit_candidate(result, self.output, self.repo)

        self.assertEqual(caught.exception.code, "input-changed-before-emission")
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
