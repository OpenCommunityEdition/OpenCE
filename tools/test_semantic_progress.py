import copy
import hashlib
import json
import struct
import tempfile
import unittest
from pathlib import Path

from tools import semantic_progress
from tools.coff_compare import build_coff, load, make_section_raw
from tools.semantic_progress import (
    OBJDIFF_331_COMBINED_EXTENT,
    SemanticProgressError,
    apply_semantic_accepted_ledger,
    apply_semantic_data_matches,
    apply_semantic_matches,
    apply_semantic_rejections,
    require_symbol_ownership_snapshots,
    revoke_incomplete_units,
    rescore_unit_with_pinned_scorer,
)


def trust_fixture_report(report):
    """FIXTURE ONLY: stand in for the pinned scorer with the fixture's own report.

    Fixture objects are synthetic and their reports are written by hand, so
    these tests deliberately do not exercise the rescoring binding.  It is
    tested by RescoredReportBindingTests and, on the real build, by
    ProductionSemanticDataManifestTests.  Never use this outside tests.
    """
    def rescore(project_root, objdiff_config, config_unit, extent_model):
        units = [unit for unit in report["units"]
                 if unit["name"] == config_unit["name"]]
        if len(units) != 1:
            raise SemanticProgressError("fixture report lacks the unit")
        return copy.deepcopy(units[0])
    return rescore


class SemanticProgressTests(unittest.TestCase):
    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.target_path = self.root / "target.obj"
        self.base_path = self.root / "base.obj"
        self.manifest_path = self.root / "semantic_matches.json"
        self.config_path = self.root / "objdiff.json"

        self.target_path.write_bytes(self._object(b"\x90" * 16))
        self.base_path.write_bytes(self._object(b"\x90" * 16))
        self.manifest_path.write_text(
            json.dumps([{"unit": "unit", "function": "_fn"}]), encoding="utf-8"
        )
        self.config_path.write_text(
            json.dumps({
                "units": [{
                    "name": "unit",
                    "target_path": "target.obj",
                    "base_path": "base.obj",
                }]
            }),
            encoding="utf-8",
        )
        self.report = {
            "measures": self._measures(100, 20, 10, 2),
            "categories": [{
                "id": "game",
                "name": "game",
                "measures": self._measures(80, 10, 8, 1),
            }],
            "units": [{
                "name": "unit",
                "measures": self._measures(16, 0, 1, 0),
                "functions": [{
                    "name": "_fn",
                    "size": 16,
                    "fuzzy_match_percent": 75.0,
                }],
                "metadata": {"progress_categories": ["game"]},
            }],
        }

    def tearDown(self):
        self.temporary_directory.cleanup()

    @staticmethod
    def _object(raw, function_name="_fn"):
        return build_coff(
            sections=[{"name": ".text", "size": 16, "raw_data": raw}],
            symbols=[{
                "name": function_name,
                "value": 0,
                "section": 1,
                "type": 0x20,
                "storage": 2,
            }],
        )

    @staticmethod
    def _local_label_object(raw, label_name, label_value=8,
                            duplicate_label=False, with_relocation=True):
        symbols = [{
            "name": "_fn",
            "value": 0,
            "section": 1,
            "type": 0x20,
            "storage": 2,
        }, {
            "name": label_name,
            "value": label_value,
            "section": 1,
            "type": 0,
            "storage": 3,
        }]
        if duplicate_label:
            symbols.append({
                "name": "$duplicate",
                "value": label_value,
                "section": 1,
                "type": 0,
                "storage": 3,
            })
        relocations = [(0, 1, 6, 0)] if with_relocation else []
        relocated_raw, relocation_data = make_section_raw(len(raw), relocations)
        relocated_raw = bytes(
            original if index >= 4 else generated
            for index, (original, generated) in enumerate(zip(raw, relocated_raw)))
        return build_coff(
            sections=[{
                "name": ".text",
                "size": len(raw),
                "raw_data": relocated_raw,
                "reloc_data": relocation_data,
            }],
            symbols=symbols,
        )

    @staticmethod
    def _measures(total_code, matched_code, total_functions, matched_functions):
        return {
            "total_code": total_code,
            "matched_code": matched_code,
            "matched_code_percent": 100.0 * matched_code / total_code,
            "total_functions": total_functions,
            "matched_functions": matched_functions,
            "matched_functions_percent": 100.0 * matched_functions / total_functions,
        }

    def _apply(self, report=None):
        if report is None:
            report = self.report
        return apply_semantic_matches(
            report, self.root, self.manifest_path, self.config_path
        )

    def test_verified_false_negative_is_credited_everywhere(self):
        self.report["units"][0]["measures"]["matched_code"] = "0"
        self.report["units"][0]["measures"]["total_code"] = "16"
        notes = self._apply()

        self.assertEqual(len(notes), 1)
        self.assertEqual(self.report["measures"]["matched_code"], 36)
        self.assertEqual(self.report["measures"]["matched_functions"], 3)
        self.assertEqual(self.report["units"][0]["measures"]["matched_code"], 16)
        self.assertEqual(self.report["categories"][0]["measures"]["matched_code"], 26)

    def test_objdiff_exact_function_is_not_double_counted(self):
        report = copy.deepcopy(self.report)
        report["units"][0]["functions"][0]["fuzzy_match_percent"] = 100.0

        notes = self._apply(report)

        self.assertEqual(notes, [])
        self.assertEqual(report["measures"]["matched_code"], 20)
        self.assertEqual(report["measures"]["matched_functions"], 2)

    def test_generated_semantic_ledger_credits_false_negative_once(self):
        semantic_report = self.root / "semantic_report.json"
        semantic_report.write_text(json.dumps({
            "summary": {"accepted_exact": 1},
            "ordinary_rejected": [],
            "accepted_ledger": [{
                "unit": "unit",
                "function": "_fn",
                "code_bytes": 16,
                "padded_bytes": 16,
                "proof_sources": ["semantic-coff"],
            }],
        }), encoding="utf-8")

        notes = apply_semantic_accepted_ledger(self.report, semantic_report)

        self.assertEqual(len(notes), 1)
        self.assertEqual(self.report["units"][0]["measures"]["matched_code"], 16)
        self.assertEqual(
            self.report["units"][0]["functions"][0]["fuzzy_match_percent"],
            100.0,
        )
        self.assertEqual(
            apply_semantic_accepted_ledger(self.report, semantic_report), [])

    def test_generated_semantic_ledger_does_not_recredit_manifest_match(self):
        self._apply()
        before = copy.deepcopy(self.report)
        semantic_report = self.root / "semantic_report.json"
        semantic_report.write_text(json.dumps({
            "summary": {"accepted_exact": 1},
            "ordinary_rejected": [],
            "accepted_ledger": [{
                "unit": "unit",
                "function": "_fn",
                "code_bytes": 16,
                "padded_bytes": 16,
                "proof_sources": ["semantic-coff"],
            }],
        }), encoding="utf-8")

        self.assertEqual(
            apply_semantic_accepted_ledger(self.report, semantic_report), [])
        self.assertEqual(self.report, before)

    def test_generated_semantic_ledger_rejects_duplicates_and_veto_overlap(self):
        entry = {
            "unit": "unit",
            "function": "_fn",
            "code_bytes": 16,
            "padded_bytes": 16,
            "proof_sources": ["semantic-coff"],
        }
        semantic_report = self.root / "semantic_report.json"
        semantic_report.write_text(json.dumps({
            "summary": {"accepted_exact": 2},
            "ordinary_rejected": [],
            "accepted_ledger": [entry, entry],
        }), encoding="utf-8")
        with self.assertRaisesRegex(
                SemanticProgressError, "duplicate semantic accepted"):
            apply_semantic_accepted_ledger(self.report, semantic_report)

        semantic_report.write_text(json.dumps({
            "summary": {"accepted_exact": 1},
            "ordinary_rejected": [{"unit": "unit", "function": "_fn"}],
            "accepted_ledger": [entry],
        }), encoding="utf-8")
        with self.assertRaisesRegex(
                SemanticProgressError, "overlaps rejection"):
            apply_semantic_accepted_ledger(self.report, semantic_report)

    def test_generated_semantic_ledger_requires_matching_report_size(self):
        semantic_report = self.root / "semantic_report.json"
        semantic_report.write_text(json.dumps({
            "summary": {"accepted_exact": 1},
            "ordinary_rejected": [],
            "accepted_ledger": [{
                "unit": "unit",
                "function": "_fn",
                "code_bytes": 12,
                "padded_bytes": 16,
                "proof_sources": ["semantic-coff"],
            }],
        }), encoding="utf-8")

        with self.assertRaisesRegex(
                SemanticProgressError, "accepted size differs"):
            apply_semantic_accepted_ledger(self.report, semantic_report)

    def test_changed_object_refuses_credit(self):
        self.base_path.write_bytes(self._object(b"\xcc" * 16))

        with self.assertRaisesRegex(SemanticProgressError, "no longer exact"):
            self._apply()

    def test_unique_cross_name_exact_function_is_credited(self):
        self.base_path.write_bytes(self._object(b"\x90" * 16, "_sdk_fn"))
        self.manifest_path.write_text(json.dumps([{
            "unit": "unit",
            "function": "_fn",
            "base_function": "_sdk_fn",
        }]), encoding="utf-8")

        notes = self._apply()

        self.assertEqual(len(notes), 1)
        self.assertEqual(self.report["units"][0]["measures"]["matched_code"], 16)

    def test_cross_name_alias_still_requires_exact_evidence(self):
        self.base_path.write_bytes(self._object(b"\xcc" * 16, "_sdk_fn"))
        self.manifest_path.write_text(json.dumps([{
            "unit": "unit",
            "function": "_fn",
            "base_function": "_sdk_fn",
        }]), encoding="utf-8")

        with self.assertRaisesRegex(SemanticProgressError, "no longer exact"):
            self._apply()

    def test_missing_report_function_refuses_credit(self):
        self.report["units"][0]["functions"] = []

        with self.assertRaisesRegex(SemanticProgressError, "expected one"):
            self._apply()

    def test_local_label_continuation_is_credited_by_exact_owner(self):
        raw = b"\0\0\0\0" + b"\x90" * 12
        self.target_path.write_bytes(
            self._local_label_object(raw, "$Ltarget"))
        self.base_path.write_bytes(
            self._local_label_object(raw, "$Lbase"))
        self.manifest_path.write_text(json.dumps([{
            "unit": "unit",
            "function": "$Ltarget",
            "owner_function": "_fn",
        }]), encoding="utf-8")
        self.report["units"][0]["functions"][0].update({
            "name": "$Ltarget",
            "size": 8,
        })

        notes = self._apply()

        self.assertEqual(len(notes), 1)
        self.assertEqual(self.report["units"][0]["measures"]["matched_code"], 8)

    def test_local_label_continuation_refuses_ambiguous_base_offset(self):
        raw = b"\0\0\0\0" + b"\x90" * 12
        self.target_path.write_bytes(
            self._local_label_object(raw, "$Ltarget"))
        self.base_path.write_bytes(
            self._local_label_object(raw, "$Lbase", duplicate_label=True))
        self.manifest_path.write_text(json.dumps([{
            "unit": "unit",
            "function": "$Ltarget",
            "owner_function": "_fn",
        }]), encoding="utf-8")
        self.report["units"][0]["functions"][0]["name"] = "$Ltarget"

        with self.assertRaisesRegex(
                SemanticProgressError, "expected one base local destination"):
            self._apply()

    def test_local_label_continuation_requires_internal_relocation(self):
        raw = b"\x90" * 16
        self.target_path.write_bytes(
            self._local_label_object(raw, "$Ltarget", with_relocation=False))
        self.base_path.write_bytes(
            self._local_label_object(raw, "$Lbase", with_relocation=False))
        self.manifest_path.write_text(json.dumps([{
            "unit": "unit",
            "function": "$Ltarget",
            "owner_function": "_fn",
        }]), encoding="utf-8")
        self.report["units"][0]["functions"][0]["name"] = "$Ltarget"

        with self.assertRaisesRegex(
                SemanticProgressError, "no proven internal relocation"):
            self._apply()

    def test_structurally_rejected_objdiff_match_is_debited_everywhere(self):
        report = copy.deepcopy(self.report)
        report["units"][0]["functions"][0]["fuzzy_match_percent"] = 100.0
        report["units"][0]["measures"] = self._measures(16, 16, 1, 1)
        report["categories"][0]["measures"] = self._measures(80, 24, 8, 2)
        semantic_report_path = self.root / "semantic_report.json"
        semantic_report_path.write_text(
            json.dumps({
                "ordinary_rejected": [{
                    "unit": "unit",
                    "function": "_fn",
                }]
            }),
            encoding="utf-8",
        )

        notes = apply_semantic_rejections(report, semantic_report_path)

        self.assertEqual(len(notes), 1)
        self.assertEqual(report["measures"]["matched_code"], 4)
        self.assertEqual(report["measures"]["matched_functions"], 1)
        self.assertEqual(report["units"][0]["measures"]["matched_code"], 0)
        self.assertEqual(report["categories"][0]["measures"]["matched_code"], 8)

    def test_rejection_revokes_completed_object_everywhere(self):
        report = copy.deepcopy(self.report)
        report["units"][0]["functions"][0]["fuzzy_match_percent"] = 100.0
        report["units"][0]["metadata"]["complete"] = True
        report["units"][0]["measures"].update({
            "matched_code": 16,
            "matched_functions": 1,
            "complete_code": 16,
            "complete_data": 4,
            "complete_units": 1,
        })
        report["measures"].update({
            "total_data": 40,
            "complete_code": 36,
            "complete_data": 12,
            "complete_units": 2,
        })
        report["categories"][0]["measures"].update({
            "matched_code": 24,
            "matched_functions": 2,
            "total_data": 20,
            "complete_code": 24,
            "complete_data": 8,
            "complete_units": 2,
        })
        semantic_report_path = self.root / "semantic_report.json"
        semantic_report_path.write_text(json.dumps({
            "ordinary_rejected": [{"unit": "unit", "function": "_fn"}]
        }), encoding="utf-8")

        apply_semantic_rejections(report, semantic_report_path)

        self.assertFalse(report["units"][0]["metadata"]["complete"])
        self.assertEqual(report["units"][0]["measures"]["complete_units"], 0)
        self.assertEqual(report["measures"]["complete_units"], 1)
        self.assertEqual(report["measures"]["complete_code"], 20)
        self.assertEqual(report["measures"]["complete_data"], 8)
        self.assertEqual(report["categories"][0]["measures"]["complete_units"], 1)

    def test_visible_mismatch_revokes_premature_complete_label(self):
        report = copy.deepcopy(self.report)
        report["units"][0]["metadata"]["complete"] = True
        report["units"][0]["measures"].update({
            "total_data": 4,
            "matched_data": 4,
            "complete_code": 16,
            "complete_data": 4,
            "complete_units": 1,
        })
        report["measures"].update({
            "total_data": 40,
            "complete_code": 36,
            "complete_data": 12,
            "complete_units": 2,
        })
        report["categories"][0]["measures"].update({
            "total_data": 20,
            "complete_code": 24,
            "complete_data": 8,
            "complete_units": 2,
        })

        notes = revoke_incomplete_units(report)

        self.assertEqual(notes, ["unit (1 unmatched functions, 0 unmatched data bytes)"])
        self.assertFalse(report["units"][0]["metadata"]["complete"])
        self.assertEqual(report["units"][0]["measures"]["complete_units"], 0)
        self.assertEqual(report["measures"]["complete_units"], 1)
        self.assertEqual(report["measures"]["complete_code"], 20)
        self.assertEqual(report["measures"]["complete_data"], 8)
        self.assertEqual(report["categories"][0]["measures"]["complete_units"], 1)

    def test_fully_matched_complete_unit_remains_complete(self):
        report = copy.deepcopy(self.report)
        report["units"][0]["metadata"]["complete"] = True
        report["units"][0]["measures"] = {
            "total_code": 16,
            "matched_code": 16,
            "total_functions": 1,
            "matched_functions": 1,
            "total_data": 4,
            "matched_data": 4,
            "complete_code": 16,
            "complete_data": 4,
            "complete_units": 1,
        }

        self.assertEqual(revoke_incomplete_units(report), [])
        self.assertTrue(report["units"][0]["metadata"]["complete"])


class SemanticDataProgressTests(unittest.TestCase):
    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.target_path = self.root / "target.obj"
        self.base_path = self.root / "base.obj"
        self.manifest_path = self.root / "semantic_data_matches.json"
        self.config_path = self.root / "objdiff.json"
        self.symbols_path = self.root / "symbols.json"
        raw = b"\x00" * 16
        self.target_path.write_bytes(self._object(raw))
        self.base_path.write_bytes(self._object(raw))
        self.manifest_path.write_text(json.dumps([{
            "unit": "data_unit",
            "symbol": "_data",
            "measurements": {
                "size": 16,
                "relocation_count": 0,
                "normalized_sha256":
                    "374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb",
            },
        }]), encoding="utf-8")
        self.config_path.write_text(json.dumps({
            "units": [{
                "name": "data_unit",
                "target_path": "target.obj",
                "base_path": "base.obj",
                "metadata": {"complete": True},
            }],
        }), encoding="utf-8")
        self.symbols_path.write_text(json.dumps([{
            "name": "_data", "file_offset": 0x1000,
        }]), encoding="utf-8")
        self.report = {
            "measures": self._measures(32, 8),
            "categories": [{
                "id": "game",
                "name": "game",
                "measures": self._measures(24, 4),
            }],
            "units": [{
                "name": "data_unit",
                "measures": self._measures(16, 0),
                "sections": [{
                    "name": ".data",
                    "size": 16,
                    "fuzzy_match_percent": 0.0,
                }],
                "metadata": {"progress_categories": ["game"]},
            }],
        }

    def tearDown(self):
        self.temporary_directory.cleanup()

    @staticmethod
    def _object(raw):
        return build_coff(
            sections=[{"name": ".data", "size": 16, "raw_data": raw,
                       "flags": 0xC0300040}],
            symbols=[{
                "name": "_data", "value": 0, "section": 1,
                "type": 0, "storage": 2,
            }],
        )

    def test_single_entry_naming_a_matched_section_is_refused(self):
        # Worker M counterexample A1: the named .data is identical and
        # already matched; the unit gap is a different, unverified .rdata of
        # the same size.  Equal size must not transfer the credit.
        def two_sections(other):
            return build_coff(
                sections=[
                    {"name": ".data", "size": 16, "raw_data": b"\x00" * 16,
                     "flags": 0xC0300040},
                    {"name": ".rdata", "size": 16, "raw_data": other,
                     "flags": 0x40300040},
                ],
                symbols=[
                    {"name": "_data", "value": 0, "section": 1,
                     "type": 0, "storage": 2},
                    {"name": "_other", "value": 0, "section": 2,
                     "type": 0, "storage": 2},
                ],
            )
        self.target_path.write_bytes(two_sections(b"\x11" * 16))
        self.base_path.write_bytes(two_sections(b"\x22" * 16))
        self.report["units"][0]["measures"] = self._measures(32, 16)
        self.report["units"][0]["sections"] = [
            {"name": ".data", "size": 16, "fuzzy_match_percent": 100.0},
            {"name": ".rdata", "size": 16, "fuzzy_match_percent": 0.0},
        ]
        with self.assertRaisesRegex(
                SemanticProgressError,
                "not the unit's unmatched report section"):
            self._apply()

    def test_embedded_nul_data_section_name_is_refused(self):
        # Worker M M4: the loader keeps bytes after an embedded NUL, objdiff
        # stops at it, so the two group the section differently.
        from tools.semantic_progress import _objdiff_base_name
        with self.assertRaisesRegex(SemanticProgressError, "embedded NUL"):
            _objdiff_base_name({"name": ".data\x00xy"})

    @staticmethod
    def _measures(total_data, matched_data):
        return {
            "total_data": total_data,
            "matched_data": matched_data,
            "matched_data_percent": 100.0 * matched_data / total_data,
        }

    def _apply(self):
        return apply_semantic_data_matches(
            self.report,
            self.root,
            self.manifest_path,
            self.config_path,
            self.symbols_path,
            rescore_report_unit=trust_fixture_report(self.report),
        )

    def test_verified_data_false_negative_is_credited_everywhere(self):
        notes = self._apply()
        self.assertEqual(len(notes), 1)
        self.assertEqual(self.report["measures"]["matched_data"], 24)
        self.assertEqual(
            self.report["units"][0]["measures"]["matched_data"], 16)
        self.assertEqual(
            self.report["categories"][0]["measures"]["matched_data"], 20)

    def test_changed_data_refuses_credit(self):
        self.base_path.write_bytes(self._object(b"\x01" + b"\x00" * 15))
        with self.assertRaisesRegex(SemanticProgressError, "no longer exact"):
            self._apply()

    def test_noncomplete_unit_refuses_credit(self):
        config = json.loads(self.config_path.read_text(encoding="utf-8"))
        config["units"][0]["metadata"]["complete"] = False
        self.config_path.write_text(json.dumps(config), encoding="utf-8")
        with self.assertRaisesRegex(SemanticProgressError, "not marked complete"):
            self._apply()

    def test_explicit_full_span_allows_incomplete_unit(self):
        config = json.loads(self.config_path.read_text(encoding="utf-8"))
        config["units"][0]["metadata"]["complete"] = False
        self.config_path.write_text(json.dumps(config), encoding="utf-8")
        entries = json.loads(self.manifest_path.read_text(encoding="utf-8"))
        entries[0]["allow_incomplete_unit"] = True
        self.manifest_path.write_text(json.dumps(entries), encoding="utf-8")

        notes = self._apply()

        self.assertEqual(len(notes), 1)
        self.assertEqual(self.report["units"][0]["measures"]["matched_data"], 16)

    @staticmethod
    def _group_object(first=b"1234567", second=b"abcdefghijkl"):
        return build_coff(
            sections=[
                {
                    "name": ".rdata",
                    "size": len(first),
                    "raw_data": first,
                    "flags": 0x40301040,
                },
                {
                    "name": ".rdata",
                    "size": len(second),
                    "raw_data": second,
                    "flags": 0x40401040,
                },
            ],
            symbols=[
                {
                    "name": "_first", "value": 0, "section": 1,
                    "type": 0, "storage": 2,
                },
                {
                    "name": "_second", "value": 0, "section": 2,
                    "type": 0, "storage": 2,
                },
            ],
        )

    @staticmethod
    def _group_member(symbol, raw, flags, padded_size):
        snapshot = {
            "section": ".rdata",
            "size": len(raw),
            "padded_size": padded_size,
            "flags": flags,
            "relocation_count": 0,
            "normalized_sha256": hashlib.sha256(raw).hexdigest(),
            "owner": {"value": 0, "type": 0, "storage": 2},
        }
        return {
            "symbol": symbol,
            "measurements": {"target": snapshot, "base": snapshot},
        }

    def test_grouped_aligned_data_sections_credit_full_report_span(self):
        first = b"1234567"
        second = b"abcdefghijkl"
        self.target_path.write_bytes(self._group_object(first, second))
        self.base_path.write_bytes(self._group_object(first, second))
        self.manifest_path.write_text(json.dumps([{
            "unit": "data_unit",
            "group": "aligned-rdata",
            "members": [
                self._group_member(
                    "_first", first, 0x40301040, 8),
                self._group_member(
                    "_second", second, 0x40401040, 16),
            ],
        }]), encoding="utf-8")
        self.report["units"][0]["measures"] = self._measures(24, 0)
        self.report["units"][0]["sections"] = [{
            "name": ".rdata",
            "size": 24,
            "fuzzy_match_percent": 50.0,
        }]
        self.report["categories"][0]["measures"] = self._measures(32, 8)

        notes = self._apply()

        self.assertEqual(
            notes, ["data_unit:aligned-rdata (+24 data bytes)"])
        self.assertEqual(
            self.report["units"][0]["measures"]["matched_data"], 24)

    def test_grouped_raw_sections_credit_reported_unpadded_span(self):
        first = b"1234567"
        second = b"abcdefghijkl"
        self.target_path.write_bytes(self._group_object(first, second))
        self.base_path.write_bytes(self._group_object(first, second))
        self.manifest_path.write_text(json.dumps([{
            "unit": "data_unit",
            "group": "raw-sections",
            "credit_raw_size": True,
            "members": [
                self._group_member("_first", first, 0x40301040, 8),
                self._group_member("_second", second, 0x40401040, 16),
            ],
        }]), encoding="utf-8")
        self.report["units"][0]["measures"] = self._measures(19, 0)
        self.report["units"][0]["sections"] = [{
            "name": ".rdata", "size": 19, "fuzzy_match_percent": 50.0,
        }]

        notes = self._apply()

        self.assertEqual(notes, ["data_unit:raw-sections (+19 data bytes)"])
        self.assertEqual(self.report["units"][0]["measures"]["matched_data"], 19)

    def test_grouped_data_member_change_refuses_credit(self):
        first = b"1234567"
        second = b"abcdefghijkl"
        self.target_path.write_bytes(self._group_object(first, second))
        self.base_path.write_bytes(
            self._group_object(first, b"X" + second[1:]))
        self.manifest_path.write_text(json.dumps([{
            "unit": "data_unit",
            "group": "aligned-rdata",
            "members": [
                self._group_member(
                    "_first", first, 0x40301040, 8),
                self._group_member(
                    "_second", second, 0x40401040, 16),
            ],
        }]), encoding="utf-8")
        self.report["units"][0]["measures"] = self._measures(24, 0)
        self.report["units"][0]["sections"] = [{
            "name": ".rdata",
            "size": 24,
            "fuzzy_match_percent": 50.0,
        }]

        with self.assertRaisesRegex(
            SemanticProgressError, "group member is no longer exact"
        ):
            self._apply()

    def test_grouped_data_repeated_section_refuses_credit(self):
        first = b"1234567"
        second = b"abcdefghijkl"
        self.target_path.write_bytes(self._group_object(first, second))
        self.base_path.write_bytes(self._group_object(first, second))
        self.manifest_path.write_text(json.dumps([{
            "unit": "data_unit",
            "group": "aligned-rdata",
            "members": [
                self._group_member(
                    "_first", first, 0x40301040, 8),
                self._group_member(
                    "_first", first, 0x40301040, 8),
            ],
        }]), encoding="utf-8")
        self.report["units"][0]["measures"] = self._measures(16, 0)
        self.report["units"][0]["sections"] = [{
            "name": ".rdata",
            "size": 16,
            "fuzzy_match_percent": 50.0,
        }]

        with self.assertRaisesRegex(
            SemanticProgressError, "repeats a section"
        ):
            self._apply()

    def test_grouped_data_requires_complete_report_section_coverage(self):
        first = b"1234567"
        second = b"abcdefghijkl"
        self.target_path.write_bytes(self._group_object(first, second))
        self.base_path.write_bytes(self._group_object(first, second))
        self.manifest_path.write_text(json.dumps([{
            "unit": "data_unit",
            "group": "aligned-rdata",
            "members": [
                self._group_member(
                    "_first", first, 0x40301040, 8),
                self._group_member(
                    "_second", second, 0x40401040, 16),
            ],
        }]), encoding="utf-8")
        self.report["units"][0]["measures"] = self._measures(28, 0)
        self.report["units"][0]["sections"] = [
            {
                "name": ".rdata",
                "size": 24,
                "fuzzy_match_percent": 50.0,
            },
            {
                "name": ".data",
                "size": 4,
                "fuzzy_match_percent": 50.0,
            },
        ]

        with self.assertRaisesRegex(
            SemanticProgressError, "does not cover the reported"
        ):
            self._apply()


_EXTENT_KIND_FLAGS = {"data": 0xC0000040, "rdata": 0x40000040, "bss": 0xC0000080}


def _extent_section(name, raw, code, kind, symbol, relocs=(), storage=2,
                    flags=None):
    return {
        "name": name, "raw": raw, "code": code, "kind": kind,
        "symbol": symbol, "relocs": list(relocs), "storage": storage,
        "flags": flags if flags is not None
        else _EXTENT_KIND_FLAGS[kind] | code << 20,
    }


def _extent_object(sections, externals=("_ext", "_other")):
    names = [section["symbol"] for section in sections] + list(externals)
    specs = []
    for section in sections:
        raw = bytearray(section["raw"])
        relocation_data = bytearray()
        for address, destination, relocation_type, addend in section["relocs"]:
            struct.pack_into("<i", raw, address, addend)
            relocation_data += struct.pack(
                "<LLH", address, names.index(destination), relocation_type)
        specs.append({
            "name": section["name"], "size": len(raw),
            "raw_data": bytes(raw), "reloc_data": bytes(relocation_data),
            "flags": section["flags"],
        })
    symbols = [{
        "name": section["symbol"], "value": 0, "section": index + 1,
        "type": 0, "storage": section["storage"],
    } for index, section in enumerate(sections)]
    symbols += [{
        "name": name, "value": 0, "section": 0, "type": 0, "storage": 2,
    } for name in externals]
    return build_coff(sections=specs, symbols=symbols)


def _extent_snapshot(section):
    """Independently recompute a member snapshot (legacy padded_size rule)."""
    raw = bytearray(section["raw"])
    for address, _, _, _ in section["relocs"]:
        raw[address:address + 4] = b"\0\0\0\0"
    code = (section["flags"] >> 20) & 0xF
    alignment = 1 << (code - 1) if code else 1
    return {
        "section": section["name"],
        "size": len(raw),
        "padded_size": (len(raw) + alignment - 1) & ~(alignment - 1),
        "flags": section["flags"],
        "relocation_count": len(section["relocs"]),
        "normalized_sha256": hashlib.sha256(bytes(raw)).hexdigest(),
        "owner": {"value": 0, "type": 0, "storage": section["storage"]},
    }


def _mixed_sections():
    """Three report sections, alignment codes 0-5, an empty and a lone one."""
    return [
        _extent_section(".rdata", b"abc", 0, "rdata", "_r0"),
        _extent_section(".data", b"xyz", 3, "data", "_d0"),
        _extent_section(".rdata", b"12345", 1, "rdata", "_r1"),
        _extent_section(".rdata", b"123456", 2, "rdata", "_r2"),
        _extent_section(".bss", b"\0" * 3, 1, "bss", "_b0"),
        _extent_section(".rdata", b"123456789", 3, "rdata", "_r3",
                        relocs=[(4, "_ext", 6, 0x10)]),
        _extent_section(".rdata", b"m" * 13, 4, "rdata", "_r4"),
        _extent_section(".bss", b"\0" * 10, 4, "bss", "_b1"),
        _extent_section(".rdata", b"n" * 17, 5, "rdata", "_r5"),
        _extent_section(".rdata", b"zz", 3, "rdata", "_r6"),
        _extent_section(".rdata", b"", 5, "rdata", "_r7"),
    ]


class SemanticDataExtentModelTests(unittest.TestCase):
    """Grouped entries pinned to the objdiff 3.3.1 combined-extent model.

    Every expected report size here was measured by running the frozen
    objdiff-cli 3.3.1 (sha1 3130e4288d483d259d1588092c8159f8e0230e08) on
    the same section tables: research/opus_data_verifier_20260925/
    extent_lab.py, cases ``test_*``.  The mixed fixture is 131 bytes to
    objdiff, 101 to the legacy per-member padded sum and 100 to the
    unpinned running-offset proposal, so it discriminates all three.
    """

    PIN = True
    MIXED_REPORT = {".bss": 16, ".data": 3, ".rdata": 112}

    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.target_path = self.root / "target.obj"
        self.base_path = self.root / "base.obj"
        self.manifest_path = self.root / "semantic_data_matches.json"
        self.config_path = self.root / "objdiff.json"
        self.symbols_path = self.root / "symbols.json"
        self.symbols_path.write_text("[]", encoding="utf-8")
        self.set_complete(False)
        self.use(_mixed_sections())

    def tearDown(self):
        self.temporary_directory.cleanup()

    def set_complete(self, complete):
        self.config_path.write_text(json.dumps({"units": [{
            "name": "data_unit",
            "target_path": "target.obj",
            "base_path": "base.obj",
            "metadata": {"complete": complete},
        }]}), encoding="utf-8")

    def use(self, target_sections, base_sections=None):
        self.target_sections = target_sections
        self.base_sections = (
            base_sections if base_sections is not None
            else copy.deepcopy(target_sections))
        self.target_path.write_bytes(_extent_object(self.target_sections))
        self.base_path.write_bytes(_extent_object(self.base_sections))
        names = sorted(
            {section["symbol"] for section in
             self.target_sections + self.base_sections}
            | {"_ext", "_other"})
        self.symbols_path.write_text(json.dumps([
            {"name": name, "file_offset": 0x1000 + 0x100 * index}
            for index, name in enumerate(names)]), encoding="utf-8")

    def entry(self, symbols=None, **extra):
        target = {section["symbol"]: section for section in self.target_sections}
        base = {section["symbol"]: section for section in self.base_sections}
        if symbols is None:
            symbols = [section["symbol"] for section in self.target_sections]
        entry = {
            "unit": "data_unit",
            "group": "lab-group",
            "allow_incomplete_unit": True,
            "reason": "test",
            "members": [{
                "symbol": symbol,
                "measurements": {
                    "target": _extent_snapshot(target[symbol]),
                    "base": _extent_snapshot(base[symbol]),
                },
            } for symbol in symbols],
        }
        if self.PIN:
            entry["extent_model"] = OBJDIFF_331_COMBINED_EXTENT
        entry.update(extra)
        return entry

    def write(self, *entries):
        self.manifest_path.write_text(json.dumps(list(entries)), encoding="utf-8")

    def report(self, sections, matched=0, fuzzy=None, total=None):
        fuzzy = fuzzy or {}
        if total is None:
            total = str(sum(sections.values()))
        if isinstance(matched, int) and not isinstance(matched, bool):
            matched = str(matched)  # objdiff writes unit byte counts as strings
        unit_sections = [{
            "name": name, "size": str(size),
            "fuzzy_match_percent": fuzzy.get(name, 50.0), "metadata": {},
        } for name, size in sorted(sections.items())]
        unit_sections.append({
            "name": ".text", "size": "40", "fuzzy_match_percent": 90.0,
            "metadata": {},
        })
        self.report_data = {
            "measures": {"total_data": 5000, "matched_data": 500,
                         "matched_code": 7, "matched_functions": 3},
            "categories": [{"id": "game", "name": "game", "measures": {
                "total_data": 1000, "matched_data": 50,
                "matched_code": 7, "matched_functions": 3}}],
            "units": [{
                "name": "data_unit",
                "measures": {
                    "total_data": total,
                    "matched_data": matched,
                    "total_functions": 2, "matched_functions": 1,
                    "total_code": "40", "matched_code": "7",
                },
                "sections": unit_sections,
                "metadata": {"progress_categories": ["game"]},
            }],
        }
        return self.report_data

    def apply(self):
        return apply_semantic_data_matches(
            self.report_data, self.root, self.manifest_path,
            self.config_path, self.symbols_path,
            rescore_report_unit=trust_fixture_report(self.report_data))

    def credit(self, entry, sections, expected):
        self.write(entry)
        self.report(sections)
        self.assertEqual(
            self.apply(), [f"data_unit:lab-group (+{expected} data bytes)"])

    def reject(self, entry, sections, pattern, **report):
        self.write(entry)
        self.report(sections, **report)
        with self.assertRaisesRegex(SemanticProgressError, pattern):
            self.apply()

    # -- the model -------------------------------------------------------

    def test_mixed_alignment_group_credits_measured_objdiff_span(self):
        self.credit(self.entry(), self.MIXED_REPORT, 131)
        unit = self.report_data["units"][0]["measures"]
        self.assertEqual(unit["matched_data"], 131)
        self.assertEqual(self.report_data["measures"]["matched_data"], 631)
        self.assertEqual(
            self.report_data["categories"][0]["measures"]["matched_data"], 181)
        # data-only credit: no code, function or completion change
        self.assertEqual(
            (unit["matched_code"], unit["matched_functions"]), ("7", 1))
        self.assertEqual(
            (self.report_data["measures"]["matched_code"],
             self.report_data["measures"]["matched_functions"]), (7, 3))
        self.assertNotIn("complete_units", unit)
        self.assertEqual(
            sum(_extent_snapshot(s)["padded_size"] for s in self.target_sections),
            101)

    def test_legacy_group_without_model_keeps_per_member_sum(self):
        entry = self.entry()
        entry.pop("extent_model")
        self.reject(entry, self.MIXED_REPORT, "does not cover the reported")
        self.credit(entry, {".bss": 19, ".data": 4, ".rdata": 78}, 101)

    def test_member_list_order_does_not_matter(self):
        entry = self.entry()
        entry["members"].reverse()
        self.credit(entry, self.MIXED_REPORT, 131)

    def test_extent_follows_section_table_order(self):
        wide = _extent_section(".rdata", b"a", 5, "rdata", "_wide")
        narrow = _extent_section(".rdata", b"b", 3, "rdata", "_narrow")
        self.use([wide, narrow])
        entry = self.entry()
        self.credit(entry, {".rdata": 20}, 20)
        self.use([copy.deepcopy(narrow), copy.deepcopy(wide)])
        self.reject(entry, {".rdata": 20}, "does not cover the reported")
        self.credit(entry, {".rdata": 16}, 16)

    def test_alignment_codes_0_1_2_4_8_16(self):
        measured = {0: 20, 1: 8, 2: 8, 3: 8, 4: 12, 5: 20}
        for code, size in measured.items():
            with self.subTest(code=code):
                self.use([
                    _extent_section(".rdata", b"abc", code, "rdata", "_x"),
                    _extent_section(".rdata", b"d", 3, "rdata", "_y"),
                ])
                self.credit(self.entry(), {".rdata": size}, size)

    def test_lone_section_reports_raw_size(self):
        self.use([_extent_section(".data", b"xyz", 3, "data", "_d")])
        self.credit(self.entry(), {".data": 3}, 3)
        self.reject(self.entry(), {".data": 4}, "does not cover the reported")

    def test_empty_section_still_aligns_running_offset(self):
        self.use([
            _extent_section(".rdata", b"12345", 3, "rdata", "_a"),
            _extent_section(".rdata", b"", 5, "rdata", "_empty"),
            _extent_section(".rdata", b"12345", 3, "rdata", "_b"),
        ])
        self.credit(self.entry(), {".rdata": 24}, 24)

    def test_dollar_sections_fold_into_base_report_section(self):
        self.use([
            _extent_section(".rdata", b"a", 5, "rdata", "_plain"),
            _extent_section(".rdata$z", b"b", 3, "rdata", "_suffix"),
        ])
        self.credit(self.entry(), {".rdata": 16}, 16)
        self.use([_extent_section(".rdata$z", b"abc", 3, "rdata", "_suffix")])
        self.credit(self.entry(), {".rdata$z": 3}, 3)

    def test_malformed_alignment_code_is_rejected(self):
        sections = _mixed_sections()
        sections[2]["flags"] = _EXTENT_KIND_FLAGS["rdata"] | 15 << 20
        self.use(sections)
        self.reject(self.entry(), self.MIXED_REPORT,
                    "invalid COFF section alignment code 15")

    def test_long_section_name_is_rejected(self):
        self.use([
            _extent_section("/4", b"abc", 3, "rdata", "_long"),
            _extent_section(".rdata", b"abcd", 3, "rdata", "_short"),
        ])
        self.reject(self.entry(), {".rdata": 4, ".rdata$x": 3},
                    "unsupported long COFF data section name")

    def test_member_outside_objdiff_data_is_rejected(self):
        self.use([
            _extent_section(".rdata", b"abcd", 3, "rdata", "_data"),
            _extent_section(".debug$S", b"abcd", 1, "rdata", "_debug",
                            flags=0x42100040),
        ])
        self.reject(self.entry(), {".rdata": 4},
                    "not an objdiff data section")

    # -- mutations that must fail closed ---------------------------------

    def test_omitted_member_is_rejected(self):
        symbols = [s["symbol"] for s in self.target_sections if s["symbol"] != "_r4"]
        self.reject(self.entry(symbols), self.MIXED_REPORT,
                    "does not cover every target .rdata section")

    def test_duplicate_member_is_rejected(self):
        entry = self.entry()
        entry["members"].append(copy.deepcopy(entry["members"][0]))
        self.reject(entry, self.MIXED_REPORT, "repeats a section")

    def test_changed_byte_is_rejected(self):
        base = _mixed_sections()
        base[6]["raw"] = b"M" + b"m" * 12
        entry = self.entry()
        self.use(_mixed_sections(), base)
        self.reject(entry, self.MIXED_REPORT, "no longer exact")

    def test_changed_relocation_type_destination_or_addend_is_rejected(self):
        entry = self.entry()
        for label, relocation in (
                ("type", (4, "_ext", 7, 0x10)),
                ("destination", (4, "_other", 6, 0x10)),
                ("addend", (4, "_ext", 6, 0x14))):
            with self.subTest(label=label):
                base = _mixed_sections()
                base[5]["relocs"] = [relocation]
                self.use(_mixed_sections(), base)
                self.reject(entry, self.MIXED_REPORT, "no longer exact")

    def test_stale_snapshot_is_rejected(self):
        entry = self.entry()
        changed = _mixed_sections()
        changed[6]["raw"] = b"q" * 13
        self.use(changed)
        self.reject(entry, self.MIXED_REPORT, "snapshot changed")

    def test_owner_change_is_rejected(self):
        entry = self.entry()
        base = _mixed_sections()
        base[2]["storage"] = 3
        self.use(_mixed_sections(), base)
        self.reject(entry, self.MIXED_REPORT, "snapshot changed")

    def test_layout_change_is_rejected(self):
        entry = self.entry()
        base = _mixed_sections()
        base[6]["flags"] = _EXTENT_KIND_FLAGS["rdata"] | 5 << 20
        self.use(_mixed_sections(), base)
        self.reject(entry, self.MIXED_REPORT, r"layout differs.*\(flags\)")

    def test_wrong_report_span_is_rejected(self):
        for label, sections in (
                ("short .rdata", {".bss": 16, ".data": 3, ".rdata": 108}),
                ("legacy sum", {".bss": 19, ".data": 4, ".rdata": 78}),
                ("unpinned proposal", {".bss": 16, ".data": 4, ".rdata": 80}),
                ("missing .bss", {".data": 3, ".rdata": 112}),
                ("extra section", {".bss": 16, ".data": 3, ".rdata": 112,
                                   ".CRT": 4})):
            with self.subTest(label=label):
                self.reject(self.entry(), sections,
                            "does not cover the reported")

    def test_incomplete_unit_requires_explicit_opt_in(self):
        entry = self.entry()
        del entry["allow_incomplete_unit"]
        self.reject(entry, self.MIXED_REPORT, "not marked complete")
        self.set_complete(True)
        self.credit(entry, self.MIXED_REPORT, 131)

    def test_unknown_extent_model_is_rejected(self):
        self.reject(self.entry(extent_model="objdiff-3.6.0-guess"),
                    self.MIXED_REPORT, "unknown semantic data extent model")

    def test_extent_model_must_be_a_string_when_present(self):
        # Only an ABSENT key selects the legacy path; a present key must be a
        # known model string.  Null, empty and non-string JSON values fail
        # cleanly with SemanticProgressError (never TypeError, never legacy).
        for value in (None, "", [], {}, True, False, 0, 1, 3.3,
                      [OBJDIFF_331_COMBINED_EXTENT],
                      {"model": OBJDIFF_331_COMBINED_EXTENT}):
            with self.subTest(value=value):
                self.reject(self.entry(extent_model=value), self.MIXED_REPORT,
                            "extent.model")

    def test_explicit_null_extent_model_never_selects_legacy(self):
        # Review reproduction: a null pin plus a legacy-sized report used to
        # credit 101 bytes through the legacy per-member path.
        self.reject(self.entry(extent_model=None),
                    {".bss": 19, ".data": 4, ".rdata": 78},
                    "must be a string when present")

    def test_invalid_extent_model_on_single_entry_is_rejected(self):
        section = self.target_sections[0]
        snapshot = _extent_snapshot(section)
        for value in (None, [], {}):
            with self.subTest(value=value):
                self.reject({
                    "unit": "data_unit", "symbol": section["symbol"],
                    "allow_incomplete_unit": True,
                    "extent_model": value,
                    "measurements": {key: snapshot[key] for key in (
                        "size", "relocation_count", "normalized_sha256")},
                }, {".rdata": 112}, "must be a string when present")

    def test_extent_model_conflicts_with_raw_size(self):
        self.reject(
            self.entry(extent_model=OBJDIFF_331_COMBINED_EXTENT,
                       credit_raw_size=True),
            self.MIXED_REPORT, "conflicts with credit_raw_size")

    def test_extent_model_requires_grouped_entry(self):
        section = self.target_sections[0]
        snapshot = _extent_snapshot(section)
        self.reject({
            "unit": "data_unit", "symbol": section["symbol"],
            "allow_incomplete_unit": True,
            "extent_model": OBJDIFF_331_COMBINED_EXTENT,
            "measurements": {key: snapshot[key] for key in (
                "size", "relocation_count", "normalized_sha256")},
        }, {".rdata": 112}, "needs a grouped entry")

    # -- report states ---------------------------------------------------

    def test_fully_matched_report_is_zero_credit_noop(self):
        self.write(self.entry())
        self.report(self.MIXED_REPORT, matched=131,
                    fuzzy={name: 100.0 for name in self.MIXED_REPORT})
        before = copy.deepcopy(self.report_data)
        self.assertEqual(self.apply(), [])
        self.assertEqual(self.report_data, before)

    def test_fully_matched_report_still_verifies_members(self):
        base = _mixed_sections()
        base[6]["raw"] = b"M" + b"m" * 12
        entry = self.entry()
        self.use(_mixed_sections(), base)
        self.reject(entry, self.MIXED_REPORT, "no longer exact", matched=131,
                    fuzzy={name: 100.0 for name in self.MIXED_REPORT})

    def test_repeated_application_credits_once(self):
        self.credit(self.entry(), self.MIXED_REPORT, 131)
        after_first = copy.deepcopy(self.report_data)
        self.assertEqual(self.apply(), [])
        self.assertEqual(self.report_data, after_first)
        self.assertEqual(self.report_data["measures"]["matched_data"], 631)

    def test_objdiff_partially_matched_unit_credits_only_remaining_sections(self):
        partial = {".data": 100.0}
        self.reject(self.entry(), self.MIXED_REPORT,
                    "does not cover the reported", matched=3, fuzzy=partial)
        remaining = [s["symbol"] for s in self.target_sections if s["symbol"] != "_d0"]
        self.write(self.entry(remaining))
        self.report(self.MIXED_REPORT, matched=3, fuzzy=partial)
        self.assertEqual(self.apply(), ["data_unit:lab-group (+128 data bytes)"])
        self.assertEqual(self.report_data["units"][0]["measures"]["matched_data"], 131)

    def test_partially_credited_totals_are_rejected(self):
        self.reject(self.entry(), self.MIXED_REPORT,
                    "does not cover all unmatched data", matched=50)

    def test_malformed_totals_are_rejected(self):
        for label, report in (
                ("matched above total", {"matched": 132}),
                ("negative matched", {"matched": -1}),
                ("non-numeric total", {"total": "13x"}),
                ("empty total", {"total": ""}),
                ("non-ASCII digit total", {"total": "ï¼‘ï¼“ï¼‘"}),
                ("float total", {"total": 131.0}),
                ("boolean matched", {"matched": True})):
            with self.subTest(label=label):
                self.reject(self.entry(), self.MIXED_REPORT,
                            "malformed report", **report)

    def test_raw_size_group_is_unchanged(self):
        entry = self.entry(credit_raw_size=True)
        entry.pop("extent_model", None)
        self.credit(entry, {".bss": 13, ".data": 3, ".rdata": 55}, 71)
        self.reject(entry, self.MIXED_REPORT, "does not cover the reported")


# -- V1 adversarial fixtures -------------------------------------------------
#
# A small January/rebuilt object pair shaped like hs.obj: a lone .data section
# with several symbols, relocations and alignment padding; a combined .rdata
# report section made of a non-COMDAT table and select-any string literals;
# a .bss section the report already matches; a literal January only
# references (undefined) and the rebuilt object defines (declared surplus).
# Every expected size below is derived by hand in the comments.

_SCN_DATA = 0xC0000040
_SCN_RDATA = 0x40000040
_SCN_BSS = 0xC0000080
_SCN_COMDAT = 0x00001000
_SELECT_NODUPLICATES = 1
_SELECT_ANY = 2
_REL32 = 0x14
_DIR32 = 0x06
_DIR32NB = 0x07

_ABC = "??_C@_03ABC@abc?$AA@"
_HELLO = "??_C@_05HELLO@hello?$AA@"
_W = "??_C@_01W@w?$AA@"
_XYZ = "??_C@_03XYZ@xyz?$AA@"
_REAL = "__real@3f800000"


def _align(code):
    return code << 20


def _v1_section(name, raw, flags, symbols, relocs=(), selection=0):
    return {
        "name": name, "raw": bytes(raw), "flags": flags,
        "symbols": [list(item) for item in symbols],
        "relocs": [list(item) for item in relocs], "selection": selection,
    }


def _v1_coff(sections, externals):
    """COFF-i386 object with section-definition symbols and aux records."""
    table = []
    for number, section in enumerate(sections, 1):
        auxiliary = struct.pack(
            "<LHHLHB3x", len(section["raw"]), len(section["relocs"]), 0, 0,
            0, section["selection"])
        table.append((section["name"], 0, number, 0, 3, auxiliary))
        for name, value, storage in section["symbols"]:
            table.append((name, value, number, 0, storage, None))
    for name in externals:
        table.append((name, 0, 0, 0, 2, None))
    index, position = {}, 0
    for name, _, _, _, _, auxiliary in table:
        if auxiliary is None:
            index.setdefault(name, position)
        position += 2 if auxiliary is not None else 1

    header_size = 20 + 40 * len(sections)
    payload, relocations, headers = bytearray(), bytearray(), []
    raw_offsets = []
    for section in sections:
        raw = bytearray(section["raw"])
        for address, destination, kind, addend in section["relocs"]:
            struct.pack_into("<i", raw, address, addend)
        if section["flags"] & _SCN_BSS == _SCN_BSS and not any(raw):
            raw_offsets.append(None)
        else:
            raw_offsets.append(len(payload))
            payload += raw
    relocation_base = header_size + len(payload)
    relocation_offsets = []
    for section in sections:
        relocation_offsets.append(relocation_base + len(relocations))
        for address, destination, kind, addend in section["relocs"]:
            relocations += struct.pack("<LLH", address, index[destination], kind)
    symbol_offset = relocation_base + len(relocations)
    strings = bytearray()
    symbol_bytes = bytearray()
    for name, value, number, kind, storage, auxiliary in table:
        encoded = name.encode("ascii")
        if len(encoded) <= 8:
            symbol_bytes += encoded.ljust(8, b"\0")
        else:
            symbol_bytes += struct.pack("<LL", 0, 4 + len(strings))
            strings += encoded + b"\0"
        symbol_bytes += struct.pack(
            "<LhHBB", value, number, kind, storage, 1 if auxiliary else 0)
        if auxiliary:
            symbol_bytes += auxiliary
    string_table = struct.pack("<L", 4 + len(strings)) + bytes(strings)
    for number, section in enumerate(sections):
        raw_pointer = (0 if raw_offsets[number] is None
                       else header_size + raw_offsets[number])
        headers.append(struct.pack(
            "<8sLLLLLLHHL", section["name"].encode("ascii").ljust(8, b"\0"),
            0, 0, len(section["raw"]), raw_pointer,
            relocation_offsets[number] if section["relocs"] else 0, 0,
            len(section["relocs"]), 0, section["flags"]))
    header = struct.pack(
        "<HHLLLHH", 0x14C, len(sections), 0, symbol_offset,
        position, 0, 0)
    return (header + b"".join(headers) + bytes(payload) + bytes(relocations)
            + bytes(symbol_bytes) + string_table)


def _v1_data_section(storage_static=3):
    # 28 bytes: four relocated pointers, a short, 2 bytes of alignment padding
    # and two longs holding the same value (so they can be swapped unseen).
    raw = bytearray(28)
    raw[16:18] = b"\x34\x12"
    raw[20:24] = struct.pack("<L", 5)
    raw[24:28] = struct.pack("<L", 5)
    return _v1_section(
        ".data", raw, _SCN_DATA | _align(3),
        [("_table", 0, 2), ("_static_names", 8, storage_static),
         ("_counter", 16, 2), ("_min", 20, 2), ("_max", 24, 2)],
        [(0, _ABC, _DIR32, 0), (4, _XYZ, _DIR32, 0),
         (8, "_table", _DIR32, 16), (12, "_ext_function", _DIR32, 0)])


def _v1_literal(name, text, code=3, selection=_SELECT_ANY, flags=None):
    return _v1_section(
        ".rdata", text, flags if flags is not None
        else _SCN_RDATA | _SCN_COMDAT | _align(code),
        [(name, 0, 2)], selection=selection)


def _v1_target_sections():
    # .rdata in table order: abc (4, align 4), table (12, align 8),
    # hello (6, align 4), w (2, align 4).
    # objdiff: 4 -> 4; +12 = 16 -> 16; +6 = 22 -> 24; +2 = 26 -> 28 = 28.
    # legacy per-member padded sum: 4 + 16 + 8 + 4 = 32.  raw sum 24.
    table = bytearray(12)
    table[0:4] = struct.pack("<L", 7)
    table[8:12] = struct.pack("<L", 9)
    return [
        _v1_data_section(),
        _v1_literal(_ABC, b"abc\0"),
        _v1_section(".rdata", table, _SCN_RDATA | _align(4),
                    [("_const_table", 0, 2)], [(4, _HELLO, _DIR32, 0)]),
        _v1_literal(_HELLO, b"hello\0"),
        _v1_literal(_W, b"w\0"),
        _v1_section(".bss", bytes(8), _SCN_BSS | _align(3),
                    [("_bss_var", 0, 2)]),
    ]


def _v1_base_sections():
    # The rebuilt object defines the literal January only references, as a
    # select-any COMDAT placed before the others (declared surplus).
    sections = _v1_target_sections()
    sections.insert(1, _v1_literal(_XYZ, b"xyz\0"))
    return sections


_V1_ADDRESSES = {
    "_table": 0x2000, "_static_names": 0x2008, "_counter": 0x2010,
    "_min": 0x2014, "_max": 0x2018, _ABC: 0x3000, "_const_table": 0x3008,
    _HELLO: 0x3018, _W: 0x3020, _XYZ: 0x5000, "_ext_function": 0x6000,
    "_other_function": 0x6100, "_bss_var": 0x7000, _REAL: 0x5100,
}


def _v1_normalized_snapshot(section, owner):
    """Recompute a member snapshot independently of the verifier."""
    raw = bytearray(section["raw"])
    for address, _, _, _ in section["relocs"]:
        raw[address:address + 4] = b"\0\0\0\0"
    code = (section["flags"] >> 20) & 0xF
    alignment = 1 << (code - 1) if code else 1
    name, value, storage = owner
    return {
        "section": section["name"],
        "size": len(raw),
        "padded_size": (len(raw) + alignment - 1) & ~(alignment - 1),
        "flags": section["flags"],
        "relocation_count": len(section["relocs"]),
        "normalized_sha256": hashlib.sha256(bytes(raw)).hexdigest(),
        "owner": {"value": value, "type": 0, "storage": storage},
    }


def _v1_find(sections, symbol):
    for section in sections:
        for owner in section["symbols"]:
            if owner[0] == symbol:
                return section, owner
    raise KeyError(symbol)


class V1Fixture:
    """Materialises one January/rebuilt pair, its report and its manifest."""

    MEMBERS = ["_table", _ABC, "_const_table", _HELLO, _W]
    SURPLUS = [_XYZ]

    def __init__(self):
        self.target = _v1_target_sections()
        self.base = _v1_base_sections()
        self.target_externals = [_XYZ, "_ext_function"]
        self.base_externals = ["_ext_function"]
        # The surplus literal's selected provider: another unit whose January
        # and rebuilt objects define it as a single-symbol select-any COMDAT.
        self.provider_target = [_v1_literal(_XYZ, b"xyz\0")]
        self.provider_base = [_v1_literal(_XYZ, b"xyz\0")]
        self.provider = "provider_unit"
        self.addresses = dict(_V1_ADDRESSES)
        # report: .bss 8 (100%), .data 28, .rdata 28, .text (code) 16
        self.report_sections = {".bss": 8, ".data": 28, ".rdata": 28}
        self.matched_sections = {".bss"}
        self.total_data = None
        self.matched_data = None
        self.complete = False
        self.entries = None

    def entry(self, members=None, surplus=None, model=True, **extra):
        members = self.MEMBERS if members is None else members
        surplus = self.SURPLUS if surplus is None else surplus
        entry = {
            "unit": "data_unit",
            "group": "v1-group",
            "allow_incomplete_unit": True,
            "reason": "fixture",
            "members": [{
                "symbol": symbol,
                "measurements": {
                    "target": _v1_normalized_snapshot(*_v1_find(self.target, symbol)),
                    "base": _v1_normalized_snapshot(*_v1_find(self.base, symbol)),
                },
            } for symbol in members],
        }
        if model:
            entry["extent_model"] = OBJDIFF_331_COMBINED_EXTENT
            entry["surplus"] = [{
                "symbol": symbol,
                "provider": self.provider,
                "measurements": {
                    "base": _v1_normalized_snapshot(*_v1_find(self.base, symbol))},
            } for symbol in surplus]
        entry.update(extra)
        return entry

    def report(self):
        total = self.total_data
        if total is None:
            total = sum(self.report_sections.values())
        matched = self.matched_data
        if matched is None:
            matched = sum(size for name, size in self.report_sections.items()
                          if name in self.matched_sections)
        sections = [{
            "name": name, "size": str(size),
            "fuzzy_match_percent": 100.0 if name in self.matched_sections else 97.5,
            "metadata": {},
        } for name, size in sorted(self.report_sections.items())]
        sections.append({"name": ".text", "size": "16",
                         "fuzzy_match_percent": 90.0, "metadata": {}})
        other = {"name": "other_unit", "measures": {
            "total_data": "8", "matched_data": "0"},
            "sections": [{"name": ".bss", "size": "8",
                          "fuzzy_match_percent": 0.0, "metadata": {}}],
            "metadata": {"progress_categories": ["game"]}}
        return {
            "measures": {"total_data": 5000, "matched_data": 500},
            "categories": [{"id": "game", "name": "game", "measures": {
                "total_data": 1000, "matched_data": 50}}],
            "units": [{
                "name": "data_unit",
                "measures": {"total_data": str(total), "matched_data": str(matched),
                             "total_functions": 2, "matched_functions": 1},
                "sections": sections,
                "metadata": {"progress_categories": ["game"]},
            }, other],
        }

    def materialize(self, root):
        root = Path(root)
        (root / "target.obj").write_bytes(
            _v1_coff(self.target, self.target_externals))
        (root / "base.obj").write_bytes(_v1_coff(self.base, self.base_externals))
        other = getattr(self, "other_sections", None) or [
            _v1_section(".bss", bytes(8), _SCN_BSS | _align(3),
                        [("_bss_var", 0, 2)])]
        (root / "other.obj").write_bytes(_v1_coff(other, []))
        (root / "provider_target.obj").write_bytes(
            _v1_coff(self.provider_target, []))
        (root / "provider_base.obj").write_bytes(
            _v1_coff(self.provider_base, []))
        (root / "objdiff.json").write_text(json.dumps({"units": [
            {"name": "data_unit", "target_path": "target.obj",
             "base_path": "base.obj", "metadata": {"complete": self.complete}},
            {"name": "other_unit", "target_path": "other.obj",
             "base_path": "other.obj", "metadata": {"complete": False}},
            {"name": "provider_unit", "target_path": "provider_target.obj",
             "base_path": "provider_base.obj", "metadata": {"complete": True}},
        ]}), encoding="utf-8")
        (root / "symbols.json").write_text(json.dumps([
            {"name": name, "file_offset": address}
            for name, address in sorted(self.addresses.items())]),
            encoding="utf-8")
        entries = self.entries if self.entries is not None else [self.entry()]
        (root / "semantic_data_matches.json").write_text(
            json.dumps(entries), encoding="utf-8")
        return self.report()


def _swap_symbol_offsets(section, first, second):
    values = {name: value for name, value, _ in section["symbols"]}
    for item in section["symbols"]:
        if item[0] == first:
            item[1] = values[second]
        elif item[0] == second:
            item[1] = values[first]


def _set_symbol(sections, symbol, **fields):
    section, owner = _v1_find(sections, symbol)
    position = {"name": 0, "value": 1, "storage": 2}
    for key, value in fields.items():
        owner[position[key]] = value


def _base_reloc(fixture, section_symbol, address, **fields):
    section, _ = _v1_find(fixture.base, section_symbol)
    for relocation in section["relocs"]:
        if relocation[0] == address:
            if "destination" in fields:
                relocation[1] = fields["destination"]
            if "kind" in fields:
                relocation[2] = fields["kind"]


def _scenario(expect):
    def decorate(function):
        function.expect = expect
        return function
    return decorate


# Each scenario mutates a fresh V1Fixture.  ``credit N`` must credit exactly N
# bytes; ``noop`` must credit nothing and leave the report unchanged; any
# other string is the rejection message the V1 verifier must raise.  Unless a
# scenario says "pinned before", the entry is (re)generated AFTER the mutation,
# so a rejection never rests on a stale snapshot.

@_scenario("credit 56")
def control_rebuilt_object(fixture):
    pass


@_scenario("credit 56")
def control_january_vs_january(fixture):
    fixture.base = copy.deepcopy(fixture.target)
    fixture.base_externals = list(fixture.target_externals)
    fixture.entries = [fixture.entry(surplus=[])]


@_scenario("credit 56")
def control_member_list_reordered(fixture):
    fixture.entries = [fixture.entry(members=list(reversed(fixture.MEMBERS)))]


@_scenario("credit 56")
def control_rebuilt_comdat_order_differs(fixture):
    # Documented design: COMDAT section ORDER in the rebuilt object is not
    # graded (each COMDAT is compared by identity); January's order sizes
    # the credit.
    hello = fixture.base.pop(4)
    fixture.base.insert(2, hello)


@_scenario("noop")
def control_already_complete_unit(fixture):
    fixture.matched_sections = {".bss", ".data", ".rdata"}


@_scenario("credit 28")
def control_partially_matched_unit_credits_only_rest(fixture):
    fixture.matched_sections = {".bss", ".data"}
    fixture.entries = [fixture.entry(members=[_ABC, "_const_table", _HELLO, _W])]


@_scenario("no longer exact")
def member_byte_changed(fixture):
    section, _ = _v1_find(fixture.base, _HELLO)
    section["raw"] = b"hellp\0"


@_scenario("no longer exact")
def relocation_retargeted_outside(fixture):
    fixture.base_externals.append("_other_function")
    _base_reloc(fixture, "_table", 12, destination="_other_function")


@_scenario("no longer exact")
def relocation_retargeted_inside_group(fixture):
    _base_reloc(fixture, "_table", 0, destination=_HELLO)


@_scenario("no longer exact")
def relocation_type_changed(fixture):
    _base_reloc(fixture, "_table", 12, kind=_DIR32NB)


@_scenario("has no image address")
def relocation_destination_unresolvable(fixture):
    del fixture.addresses["_ext_function"]


@_scenario("cannot verify semantic data group member")
def owner_renamed_pinned_before(fixture):
    fixture.entries = [fixture.entry()]
    _set_symbol(fixture.base, "_const_table", name="_const_table_renamed")


@_scenario("malformed semantic data extent-model member")
def owner_renamed_with_base_alias(fixture):
    fixture.entries = [fixture.entry()]
    _set_symbol(fixture.base, "_const_table", name="_const_table_renamed")
    fixture.entries[0]["members"][2]["base_symbol"] = "_const_table_renamed"


@_scenario("snapshot changed")
def owner_external_to_static_pinned_before(fixture):
    fixture.entries = [fixture.entry()]
    _set_symbol(fixture.base, "_const_table", storage=3)


@_scenario("owner differs")
def owner_external_to_static_repinned(fixture):
    _set_symbol(fixture.base, "_const_table", storage=3)


@_scenario("owner differs")
def owner_static_to_external_repinned(fixture):
    _set_symbol(fixture.target, "_const_table", storage=3)


@_scenario("symbol table differs")
def non_owner_symbol_storage_changed(fixture):
    _set_symbol(fixture.base, "_static_names", storage=2)


@_scenario("does not cover every target .rdata section")
def missing_member(fixture):
    fixture.entries = [fixture.entry(members=["_table", _ABC, "_const_table", _HELLO])]


@_scenario("does not cover every target .rdata section")
def subset_summing_to_extent_legacy(fixture):
    # Legacy per-member padded sizes of table + hello + w = 16 + 8 + 4 = 28,
    # exactly objdiff's .rdata extent: the stock grouped check credits this
    # subset although the abc literal is never compared.
    fixture.entries = [fixture.entry(
        members=["_table", "_const_table", _HELLO, _W], model=False)]


@_scenario("does not cover every target .rdata section")
def subset_summing_to_extent_model(fixture):
    fixture.entries = [fixture.entry(members=["_table", "_const_table", _HELLO, _W])]


@_scenario("no longer exact")
def padding_bytes_nonzero(fixture):
    section, _ = _v1_find(fixture.base, "_table")
    raw = bytearray(section["raw"])
    raw[18] = 0xCC  # the alignment padding after the 2-byte _counter
    section["raw"] = bytes(raw)


@_scenario("no longer exact")
def padding_raw_size_differs_padded_size_equal(fixture):
    section, _ = _v1_find(fixture.base, _HELLO)
    section["raw"] = b"hello\0\0\0"  # padded size 8 either way


@_scenario("does not cover every target .rdata section")
def extra_unlisted_section_in_target(fixture):
    fixture.target.insert(5, _v1_literal("??_C@_01Q@q?$AA@", b"q\0"))
    fixture.base.insert(6, _v1_literal("??_C@_01Q@q?$AA@", b"q\0"))
    fixture.addresses["??_C@_01Q@q?$AA@"] = 0x3024


@_scenario("leaves rebuilt sections undeclared")
def extra_undeclared_section_in_rebuilt(fixture):
    fixture.base.insert(3, _v1_literal(_REAL, struct.pack("<f", 1.0)))


@_scenario("symbol table differs")
def extra_symbol_inside_member_section(fixture):
    section, _ = _v1_find(fixture.base, "_table")
    section["symbols"].append(["_table_alias", 8, 2])


@_scenario("symbol table differs")
def reordered_symbols_inside_member_section(fixture):
    # _min and _max hold the same bytes: swapping them changes no byte and no
    # relocation, only which name owns which offset.
    section, _ = _v1_find(fixture.base, "_table")
    _swap_symbol_offsets(section, "_min", "_max")


@_scenario(r"layout differs.*\(flags\)")
def section_flags_changed_writable(fixture):
    section, _ = _v1_find(fixture.base, _ABC)
    section["flags"] = _SCN_DATA | _SCN_COMDAT | _align(3)


@_scenario("layout differs")
def section_alignment_changed(fixture):
    section, _ = _v1_find(fixture.base, "_const_table")
    section["flags"] = _SCN_RDATA | _align(3)


@_scenario("COMDAT selection differs")
def comdat_selection_changed(fixture):
    section, _ = _v1_find(fixture.base, _ABC)
    section["selection"] = _SELECT_NODUPLICATES


@_scenario("repeats a section")
def duplicate_member(fixture):
    entry = fixture.entry()
    entry["members"].append(copy.deepcopy(entry["members"][1]))
    fixture.entries = [entry]


@_scenario("repeats a section")
def duplicate_member_same_section(fixture):
    fixture.entries = [fixture.entry(members=fixture.MEMBERS + ["_min"])]


@_scenario("more than one entry")
def duplicate_entry_for_unit(fixture):
    fixture.entries = [fixture.entry(), fixture.entry()]


@_scenario("cannot verify semantic data group member")
def wrong_unit(fixture):
    entry = fixture.entry()
    entry["unit"] = "other_unit"
    fixture.entries = [entry]


@_scenario("does not cover the reported unmatched sections")
def extent_mismatch_plus_one(fixture):
    fixture.report_sections[".rdata"] = 29


@_scenario("does not cover the reported unmatched sections")
def extent_mismatch_minus_one(fixture):
    fixture.report_sections[".rdata"] = 27


@_scenario("report total_data is not the target's modelled data extent")
def total_data_off_by_one(fixture):
    fixture.total_data = 65
    fixture.matched_data = 9


@_scenario("report section .bss is not the target's modelled extent")
def matched_section_missized(fixture):
    fixture.report_sections[".bss"] = 9


@_scenario("does not cover the reported unmatched sections")
def already_complete_section_included(fixture):
    fixture.matched_sections = {".bss", ".data"}


@_scenario("does not cover the reported unmatched sections")
def only_already_complete_section(fixture):
    fixture.matched_sections = {".bss", ".data"}
    fixture.entries = [fixture.entry(members=["_table"])]


@_scenario("defined by the target")
def surplus_defined_by_target(fixture):
    fixture.entries = [fixture.entry(surplus=[_XYZ, _ABC])]


@_scenario("not a select-any COMDAT")
def surplus_not_comdat(fixture):
    section, _ = _v1_find(fixture.base, _XYZ)
    section["flags"] = _SCN_RDATA | _align(3)


@_scenario("not a select-any COMDAT")
def surplus_not_select_any(fixture):
    section, _ = _v1_find(fixture.base, _XYZ)
    section["selection"] = _SELECT_NODUPLICATES


@_scenario("holds other symbols")
def surplus_holds_other_symbols(fixture):
    section, _ = _v1_find(fixture.base, _XYZ)
    section["symbols"].append(["_hidden", 2, 2])


@_scenario("surplus snapshot changed")
def surplus_stale_snapshot(fixture):
    fixture.entries = [fixture.entry()]
    section, _ = _v1_find(fixture.base, _XYZ)
    section["raw"] = b"xyw\0"


@_scenario("outside the covered report sections")
def surplus_outside_covered_sections(fixture):
    fixture.base.append(_v1_section(
        ".CRT$XCU", bytes(4), _SCN_RDATA | _SCN_COMDAT | _align(3),
        [("_initializer", 0, 2)], selection=_SELECT_ANY))
    fixture.entries = [fixture.entry(surplus=[_XYZ, "_initializer"])]


@_scenario("repeats a section")
def surplus_duplicate(fixture):
    fixture.entries = [fixture.entry(surplus=[_XYZ, _XYZ])]


@_scenario("surplus needs an extent_model entry")
def surplus_on_legacy_entry(fixture):
    entry = fixture.entry()
    del entry["extent_model"]
    fixture.entries = [entry]


@_scenario("unknown semantic data entry keys")
def entry_unknown_key(fixture):
    fixture.entries = [fixture.entry(allow_incomplete_units=True)]


@_scenario("malformed semantic data extent-model entry")
def entry_empty_reason(fixture):
    fixture.entries = [fixture.entry(reason=" ")]


@_scenario("malformed semantic data extent-model entry")
def entry_non_boolean_opt_in(fixture):
    fixture.entries = [fixture.entry(allow_incomplete_unit="yes")]


V1_SCENARIOS = [
    value for name, value in sorted(globals().items())
    if callable(value) and hasattr(value, "expect")
    and getattr(value, "__module__", None) == __name__
]


def run_v1_scenario(scenario, verifier, root):
    """Apply one scenario with ``verifier`` (a semantic_progress module)."""
    fixture = V1Fixture()
    scenario(fixture)
    report = fixture.materialize(root)
    before = copy.deepcopy(report)
    notes = verifier.apply_semantic_data_matches(
        report, Path(root), Path(root) / "semantic_data_matches.json",
        Path(root) / "objdiff.json", Path(root) / "symbols.json",
        rescore_report_unit=trust_fixture_report(report))
    return notes, report, before


class SemanticDataExtentModelAdversarialTests(unittest.TestCase):
    """Adversarial fixtures for the pinned objdiff-3.3.1 extent model (V1)."""

    def run_scenario(self, scenario):
        with tempfile.TemporaryDirectory() as root:
            expect = scenario.expect
            if expect.startswith("credit ") or expect == "noop":
                notes, report, before = run_v1_scenario(
                    scenario, semantic_progress, root)
                if expect == "noop":
                    self.assertEqual(notes, [])
                    self.assertEqual(report, before)
                    return report
                size = int(expect.split()[1])
                self.assertEqual(notes, [f"data_unit:v1-group (+{size} data bytes)"])
                unit = report["units"][0]["measures"]
                self.assertEqual(
                    unit["matched_data"],
                    int(before["units"][0]["measures"]["matched_data"]) + size)
                self.assertEqual(unit["matched_data"], 64)
                self.assertEqual(report["measures"]["matched_data"], 500 + size)
                self.assertEqual(
                    report["categories"][0]["measures"]["matched_data"], 50 + size)
                return report
            with self.assertRaisesRegex(SemanticProgressError, expect):
                run_v1_scenario(scenario, semantic_progress, root)
            return None

    def test_every_scenario_has_a_test(self):
        names = {scenario.__name__ for scenario in V1_SCENARIOS}
        tests = {name[len("test_"):] for name in dir(self) if name.startswith("test_")}
        self.assertEqual(names - tests, set())

    def test_fixture_arithmetic(self):
        # Hand arithmetic behind the expected sizes (see _v1_target_sections).
        fixture = V1Fixture()
        rdata = [s for s in fixture.target if s["name"] == ".rdata"]
        self.assertEqual(sum(len(s["raw"]) for s in rdata), 24)
        legacy = sum(_v1_normalized_snapshot(s, s["symbols"][0])["padded_size"]
                     for s in rdata)
        self.assertEqual(legacy, 32)
        subset = [s for s in rdata if s["symbols"][0][0] != _ABC]
        self.assertEqual(
            sum(_v1_normalized_snapshot(s, s["symbols"][0])["padded_size"]
                for s in subset), 28)

    def test_subset_loophole_shape_passes_the_legacy_sum(self):
        # The subset's legacy per-member sums equal the report exactly, so
        # only complete section coverage can reject it.
        fixture = V1Fixture()
        subset_legacy_members = ["_table", "_const_table", _HELLO, _W]
        sums = {}
        for symbol in subset_legacy_members:
            snapshot = _v1_normalized_snapshot(*_v1_find(fixture.target, symbol))
            sums[snapshot["section"]] = sums.get(snapshot["section"], 0) \
                + snapshot["padded_size"]
        self.assertEqual(sums, {".data": 28, ".rdata": 28})
        self.assertEqual(sums, {k: v for k, v in fixture.report_sections.items()
                                if k not in fixture.matched_sections})

    def test_repeated_application_credits_once(self):
        with tempfile.TemporaryDirectory() as root:
            fixture = V1Fixture()
            report = fixture.materialize(root)
            arguments = (Path(root), Path(root) / "semantic_data_matches.json",
                         Path(root) / "objdiff.json", Path(root) / "symbols.json")
            trusted = trust_fixture_report(report)
            self.assertEqual(apply_semantic_data_matches(
                report, *arguments, rescore_report_unit=trusted),
                ["data_unit:v1-group (+56 data bytes)"])
            after = copy.deepcopy(report)
            self.assertEqual(apply_semantic_data_matches(
                report, *arguments, rescore_report_unit=trusted), [])
            self.assertEqual(report, after)


def _add_scenario_test(scenario):
    def test(self):
        self.run_scenario(scenario)
    test.__name__ = f"test_{scenario.__name__}"
    test.__doc__ = f"{scenario.__name__}: expect {scenario.expect}"
    setattr(SemanticDataExtentModelAdversarialTests, test.__name__, test)


for _scenario_function in V1_SCENARIOS:
    _add_scenario_test(_scenario_function)


def _materialize(fixture, root):
    report = fixture.materialize(root)
    arguments = (Path(root), Path(root) / "semantic_data_matches.json",
                 Path(root) / "objdiff.json", Path(root) / "symbols.json")
    return report, arguments


_SCN_CODE_ALIGN16 = 0x60000020 | _align(5)


def _aux_record_offset(data, obj, symbol):
    """File offset of the section-definition aux record of ``symbol``'s section."""
    number = [item for item in obj["symbols"]
              if item["name"] == symbol and item["section"] > 0][0]["section"]
    name = obj["sections"][number - 1]["name"]
    definition = [item for item in obj["symbols"]
                  if item["section"] == number and item["name"] == name
                  and item["storage"] == 3 and item["value"] == 0][0]
    symbol_offset = struct.unpack_from("<L", data, 8)[0]
    return symbol_offset + 18 * definition["index"] + 18


def _run_fixture(fixture, post_target=None, post_base=None):
    """Materialise a V1 fixture, optionally byte-patch its objects, verify."""
    with tempfile.TemporaryDirectory() as root:
        report = fixture.materialize(root)
        for name, post in (("target.obj", post_target), ("base.obj", post_base)):
            if post:
                path = Path(root) / name
                data = bytearray(path.read_bytes())
                post(data, load(bytes(data)))
                path.write_bytes(bytes(data))
        return apply_semantic_data_matches(
            report, Path(root), Path(root) / "semantic_data_matches.json",
            Path(root) / "objdiff.json", Path(root) / "symbols.json",
            rescore_report_unit=trust_fixture_report(report))


class IdentityCounterexampleTests(unittest.TestCase):
    """Real identity differences the submitted V1 credited (review worker I)."""

    def test_imported_target_resolved_to_local_static_is_rejected(self):
        fixture = V1Fixture()
        fixture.base.append(_v1_section(
            ".text", b"\xc3" + b"\xcc" * 15, _SCN_CODE_ALIGN16,
            [("_ext_function", 0, 3)]))
        fixture.base_externals = [name for name in fixture.base_externals
                                  if name != "_ext_function"]
        with self.assertRaisesRegex(SemanticProgressError, "target linkage differs"):
            _run_fixture(fixture)

    def test_local_static_target_resolved_to_import_is_rejected(self):
        fixture = V1Fixture()
        fixture.target.append(_v1_section(
            ".text", b"\xc3" + b"\xcc" * 15, _SCN_CODE_ALIGN16,
            [("_ext_function", 0, 3)]))
        fixture.target_externals = [name for name in fixture.target_externals
                                    if name != "_ext_function"]
        with self.assertRaisesRegex(SemanticProgressError, "target linkage differs"):
            _run_fixture(fixture)

    def test_static_surplus_owner_is_rejected(self):
        fixture = V1Fixture()
        _, owner = _v1_find(fixture.base, _XYZ)
        owner[2] = 3
        fixture.entries = [fixture.entry()]
        with self.assertRaisesRegex(SemanticProgressError,
                                    "not an external|target linkage"):
            _run_fixture(fixture)

    def test_surplus_bytes_differing_from_january_provider_are_rejected(self):
        fixture = V1Fixture()
        section, _ = _v1_find(fixture.base, _XYZ)
        section["raw"] = b"xyw\0"
        fixture.entries = [fixture.entry()]
        with self.assertRaisesRegex(SemanticProgressError, "January provider"):
            _run_fixture(fixture)

    def test_associative_member_is_rejected(self):
        fixture = V1Fixture()
        for sections in (fixture.target, fixture.base):
            section, _ = _v1_find(sections, _ABC)
            section["selection"] = 5

        def associated(value):
            def post(data, obj):
                struct.pack_into(
                    "<H", data, _aux_record_offset(data, obj, _ABC) + 12, value)
            return post
        with self.assertRaisesRegex(SemanticProgressError,
                                    "selection is not supported"):
            _run_fixture(fixture, associated(1), associated(2))

    def test_exact_match_member_with_other_checksum_is_rejected(self):
        fixture = V1Fixture()
        for sections in (fixture.target, fixture.base):
            section, _ = _v1_find(sections, _ABC)
            section["selection"] = 4

        def checksum(value):
            def post(data, obj):
                struct.pack_into(
                    "<L", data, _aux_record_offset(data, obj, _ABC) + 8, value)
            return post
        with self.assertRaisesRegex(SemanticProgressError,
                                    "selection is not supported"):
            _run_fixture(fixture, checksum(1), checksum(2))

    def test_comdat_key_symbol_order_is_rejected(self):
        fixture = V1Fixture()
        target, _ = _v1_find(fixture.target, _ABC)
        target["symbols"] = [[_ABC, 0, 2], ["_abc_alias", 0, 2]]
        base, _ = _v1_find(fixture.base, _ABC)
        base["symbols"] = [["_abc_alias", 0, 2], [_ABC, 0, 2]]
        fixture.entries = [fixture.entry()]
        with self.assertRaisesRegex(SemanticProgressError,
                                    "COMDAT key symbol differs"):
            _run_fixture(fixture)

    def test_non_ascii_name_collapse_is_rejected(self):
        fixture = V1Fixture()

        def rename(byte):
            def post(data, obj):
                at = data.rfind(b"_static_names\0")
                data[at + len(b"_static_names") - 1] = byte
            return post
        with self.assertRaisesRegex(SemanticProgressError, "undecodable COFF name"):
            _run_fixture(fixture, rename(0x80), rename(0x81))


class PreviouslyUntestedCheckTests(unittest.TestCase):
    """V1 checks whose single-check mutants survived V1's own suite (worker I)."""

    def test_resolved_list_mismatch_is_rejected(self):
        fixture = V1Fixture()
        for sections in (fixture.target, fixture.base):
            section, _ = _v1_find(sections, _ABC)
            section["symbols"] = [[_ABC, 0, 2], ["_abc_end", 4, 2]]
        target, _ = _v1_find(fixture.target, "_const_table")
        target["relocs"] = [[4, _ABC, _DIR32, 4]]
        base, _ = _v1_find(fixture.base, "_const_table")
        base["relocs"] = [[4, "_abc_end", _DIR32, 0]]
        fixture.addresses["_abc_end"] = fixture.addresses[_ABC] + 5
        fixture.entries = [fixture.entry()]
        with self.assertRaisesRegex(SemanticProgressError,
                                    "resolved relocations differ"):
            _run_fixture(fixture)
        fixture.addresses["_abc_end"] = fixture.addresses[_ABC] + 4
        self.assertEqual(_run_fixture(fixture),
                         ["data_unit:v1-group (+56 data bytes)"])

    def test_duplicate_report_section_is_rejected(self):
        fixture = V1Fixture()
        with tempfile.TemporaryDirectory() as root:
            report = fixture.materialize(root)
            report["units"][0]["sections"].append(
                copy.deepcopy(report["units"][0]["sections"][0]))
            with self.assertRaisesRegex(SemanticProgressError,
                                        "duplicate report section"):
                apply_semantic_data_matches(
                    report, Path(root), Path(root) / "semantic_data_matches.json",
                    Path(root) / "objdiff.json", Path(root) / "symbols.json",
                    rescore_report_unit=trust_fixture_report(report))

    def test_ambiguous_report_name_is_rejected(self):
        flags = 0x40000040 | _align(3)
        sections = [{"index": index + 1, "name": name, "size": 4, "flags": flags}
                    for index, name in enumerate(
                        ["foo$bar", "foo$bar$1", "foo$bar$2"])]
        with self.assertRaisesRegex(SemanticProgressError,
                                    "ambiguous objdiff data section name"):
            semantic_progress._objdiff_report_data_groups({"sections": sections})

    def test_extent_rejects_alignment_code_15(self):
        sections = [
            {"index": 1, "name": ".rdata", "size": 4, "flags": 0x40000040 | (15 << 20)},
            {"index": 2, "name": ".rdata", "size": 4, "flags": 0x40000040 | (3 << 20)}]
        with self.assertRaisesRegex(SemanticProgressError,
                                    "invalid COFF section alignment code 15"):
            semantic_progress._objdiff_report_extent(sections)

    def test_surplus_must_be_a_list(self):
        fixture = V1Fixture()
        entry = fixture.entry()
        entry["surplus"] = {}
        fixture.entries = [entry]
        with self.assertRaisesRegex(SemanticProgressError, "surplus must be a list"):
            _run_fixture(fixture)

    def test_model_entry_must_name_its_group(self):
        fixture = V1Fixture()
        entry = fixture.entry()
        del entry["group"]
        fixture.entries = [entry]
        with self.assertRaisesRegex(SemanticProgressError, "needs a group"):
            _run_fixture(fixture)

    def test_malformed_manifest_fails_with_the_verifier_error(self):
        for entries in ([{"unit": ["source/hs/hs"]}], ["source/hs/hs"],
                        {"unit": "data_unit"}):
            with self.subTest(entries=repr(entries)):
                fixture = V1Fixture()
                fixture.entries = entries
                with self.assertRaisesRegex(SemanticProgressError,
                                            "must be a list of entries"):
                    _run_fixture(fixture)


class RescoredReportBindingTests(unittest.TestCase):
    """A model entry binds the live report to a fresh report of its objects."""

    def _apply(self, fixture, rescore):
        with tempfile.TemporaryDirectory() as root:
            report, arguments = _materialize(fixture, root)
            return apply_semantic_data_matches(
                report, *arguments, rescore_report_unit=rescore(report))

    def _mutated(self, change):
        def factory(report):
            trusted = trust_fixture_report(report)

            def rescore(*arguments):
                fresh = trusted(*arguments)
                change(fresh)
                return fresh
            return rescore
        return factory

    def test_identical_fresh_report_binds(self):
        self.assertEqual(self._apply(V1Fixture(), trust_fixture_report),
                         ["data_unit:v1-group (+56 data bytes)"])

    def test_stale_or_foreign_reports_fail_closed(self):
        def resize(fresh):
            fresh["sections"][1]["size"] = "29"

        def rematch(fresh):
            fresh["measures"]["matched_data"] = "36"

        def reratio(fresh):
            fresh["sections"][1]["fuzzy_match_percent"] = 99.0

        def other_unit(fresh):
            fresh["name"] = "other_unit"

        def extra_section(fresh):
            fresh["sections"].append(
                {"name": ".tls", "size": "4", "fuzzy_match_percent": 0.0,
                 "metadata": {}})
        for change in (resize, rematch, reratio, other_unit, extra_section):
            with self.subTest(change=change.__name__):
                with self.assertRaisesRegex(
                        SemanticProgressError,
                        "report is not the pinned scorer's report"):
                    self._apply(V1Fixture(), self._mutated(change))

    def test_missing_scorer_fails_closed(self):
        with tempfile.TemporaryDirectory() as root:
            report, arguments = _materialize(V1Fixture(), root)
            with self.assertRaisesRegex(SemanticProgressError,
                                        "pinned scorer is missing"):
                apply_semantic_data_matches(report, *arguments)

    def test_unpinned_scorer_binary_fails_closed(self):
        with tempfile.TemporaryDirectory() as root:
            report, arguments = _materialize(V1Fixture(), root)
            scorer = Path(root) / semantic_progress.DEFAULT_SCORER_PATH
            scorer.parent.mkdir(parents=True)
            scorer.write_bytes(b"not the pinned objdiff-cli 3.3.1")
            with self.assertRaisesRegex(SemanticProgressError,
                                        "is not the binary pinned"):
                apply_semantic_data_matches(report, *arguments)

    def test_unknown_model_has_no_pinned_scorer(self):
        with self.assertRaisesRegex(SemanticProgressError, "no scorer is pinned"):
            rescore_unit_with_pinned_scorer(
                Path("."), {}, {"name": "u"}, "objdiff-9.9.9-combined")

    def test_real_pinned_scorer_rejects_a_hand_written_fixture_report(self):
        scorer = Path(__file__).resolve().parents[1] / \
            semantic_progress.DEFAULT_SCORER_PATH
        if not scorer.is_file():
            self.skipTest(f"SKIPPED: pinned scorer not present at {scorer}")
        with tempfile.TemporaryDirectory() as root:
            report, arguments = _materialize(V1Fixture(), root)

            def real(project_root, objdiff_config, config_unit, extent_model):
                return rescore_unit_with_pinned_scorer(
                    project_root, objdiff_config, config_unit, extent_model,
                    scorer_path=scorer)
            with self.assertRaisesRegex(
                    SemanticProgressError,
                    "report is not the pinned scorer's report"):
                apply_semantic_data_matches(
                    report, *arguments, rescore_report_unit=real)


class SurplusProviderProofTests(unittest.TestCase):
    """Every declared surplus literal is a folded copy of one January provider."""

    def _apply(self, fixture):
        with tempfile.TemporaryDirectory() as root:
            report, arguments = _materialize(fixture, root)
            return apply_semantic_data_matches(
                report, *arguments,
                rescore_report_unit=trust_fixture_report(report))

    def _rejects(self, mutate, pattern):
        fixture = V1Fixture()
        mutate(fixture)
        with self.assertRaisesRegex(SemanticProgressError, pattern):
            self._apply(fixture)

    def test_control_provider_proof_passes(self):
        self.assertEqual(self._apply(V1Fixture()),
                         ["data_unit:v1-group (+56 data bytes)"])

    def test_surplus_not_referenced_by_january_unit(self):
        # The actions shape: a rebuilt-only literal January's object for this
        # unit never references (here pulled in by nothing January has),
        # even though another unit provides it.
        unreferenced = "??_C@_03NEW@new?$AA@"

        def mutate(fixture):
            fixture.base.insert(1, _v1_literal(unreferenced, b"new\0"))
            fixture.provider_target.append(_v1_literal(unreferenced, b"new\0"))
            fixture.provider_base.append(_v1_literal(unreferenced, b"new\0"))
            fixture.SURPLUS = [_XYZ, unreferenced]
        self._rejects(mutate, "not a January undefined reference")

    def test_invalid_or_self_provider(self):
        for provider in ("missing_unit", "data_unit"):
            with self.subTest(provider=provider):
                def mutate(fixture, provider=provider):
                    fixture.provider = provider
                self._rejects(mutate, "invalid provider")

    def test_provider_is_not_the_unique_january_definer(self):
        def wrong_provider(fixture):
            fixture.provider = "other_unit"
        self._rejects(wrong_provider, "not January's unique definer")
        # other_unit's January object defines the literal too: two definers.
        fixture = V1Fixture()
        fixture.other_sections = [
            _v1_section(".bss", bytes(8), _SCN_BSS | _align(3),
                        [("_bss_var", 0, 2)]),
            _v1_literal(_XYZ, b"xyz\0")]
        with self.assertRaisesRegex(SemanticProgressError,
                                    "not January's unique definer"):
            self._apply(fixture)

    def test_provider_copies_must_be_identical_select_any(self):
        cases = {
            "January provider": lambda f: f.provider_target.__setitem__(
                0, _v1_literal(_XYZ, b"xyZ\0")),
            "rebuilt provider": lambda f: f.provider_base.__setitem__(
                0, _v1_literal(_XYZ, b"xyZ\0")),
            "not a select-any": lambda f: f.provider_base.__setitem__(
                0, _v1_literal(_XYZ, b"xyz\0", selection=_SELECT_NODUPLICATES)),
            "alignment": lambda f: f.provider_base.__setitem__(
                0, _v1_literal(_XYZ, b"xyz\0", code=5)),
        }
        patterns = {
            "January provider": "differs from January provider",
            "rebuilt provider": "differs from rebuilt provider",
            "not a select-any": "not a select-any COMDAT",
            "alignment": "differs from rebuilt provider",
        }
        for name, mutate in cases.items():
            with self.subTest(case=name):
                self._rejects(mutate, patterns[name])

    def test_provider_copy_sharing_its_section_fails(self):
        def mutate(fixture):
            fixture.provider_base[0]["symbols"].append(["_neighbour", 0, 2])
        self._rejects(mutate, "shares its section")

    def test_missing_january_object_fails(self):
        fixture = V1Fixture()
        with tempfile.TemporaryDirectory() as root:
            report, arguments = _materialize(fixture, root)
            (Path(root) / "provider_target.obj").unlink()
            with self.assertRaisesRegex(SemanticProgressError,
                                        "January object missing"):
                apply_semantic_data_matches(
                    report, *arguments,
                    rescore_report_unit=trust_fixture_report(report))


# Surplus-helper constants (the source/ai/actions shape).  January's object for
# the unit imports a helper function; the rebuilt object carries its own
# select-any copy of the helper and of the two constants the helper loads,
# which January's object never names.  The single January definer of the
# helper (the provider) holds the identical helper and the identical constants.
_HELPER = "_helper2d"
_CALLER = "_caller"
_C1 = _REAL
_C2 = "__real@3f1a36e2e0000000"
_C1_BYTES = b"\x00\x00\x80\x3f"
_C2_BYTES = bytes.fromhex("000000e0e2361a3f")
_SCN_CODE_COMDAT = _SCN_CODE_ALIGN16 | _SCN_COMDAT
# fld qword [C2]; fmul dword [C1]; ret; padding.
_HELPER_RAW = b"\xdd\x05" + bytes(4) + b"\xd8\x0d" + bytes(4) + b"\xc3" + b"\xcc" * 3
_HELPER_RELOCS = ((2, _C2, _DIR32, 0), (8, _C1, _DIR32, 0))
_SCN_DEBUG = 0x42100040
_WRITE = 0x80000000


def _helper_section(selection=_SELECT_ANY, relocs=_HELPER_RELOCS,
                    raw=_HELPER_RAW, symbols=((_HELPER, 0, 2),),
                    flags=_SCN_CODE_COMDAT):
    return _v1_section(".text", raw, flags, symbols, relocs, selection=selection)


def _caller_section(extra_relocs=()):
    # call _helper2d; ret; padding (bytes 6..15 can hold extra relocations).
    raw = b"\xe8" + bytes(4) + b"\xc3" + b"\xcc" * 10
    return _v1_section(".text", raw, _SCN_CODE_ALIGN16, [(_CALLER, 0, 2)],
                       [(1, _HELPER, _REL32, -4)] + list(extra_relocs))


def _c1(raw=_C1_BYTES, **options):
    return _v1_literal(_C1, raw, **options)


def _c2(raw=_C2_BYTES, **options):
    return _v1_literal(_C2, raw, code=options.pop("code", 4), **options)


class HelperFixture(V1Fixture):
    """V1Fixture plus a surplus helper whose constants January never names.

    base: .data, xyz, C1, C2, abc, table, hello, w, .bss, caller, helper.
    provider (January and rebuilt): xyz, helper, C1, C2.
    """

    SURPLUS = [_XYZ, _C1, _C2]

    def __init__(self):
        super().__init__()
        self.target.append(_caller_section())
        self.target_externals = self.target_externals + [_HELPER]
        self.base.insert(2, _c1())
        self.base.insert(3, _c2())
        self.base.append(_caller_section())
        self.base.append(_helper_section())
        self.provider_target += [
            _helper_section(selection=_SELECT_NODUPLICATES), _c1(), _c2()]
        self.provider_base += [_helper_section(), _c1(), _c2()]
        self.addresses.update({_HELPER: 0x6200, _CALLER: 0x6300, _C2: 0x5108})
        self.provider_target_imports = []

    def materialize(self, root):
        # V1Fixture writes provider objects without imports; a provider that
        # imports a name is written again with it (placeholders let the base
        # writer resolve the name first).
        if not self.provider_target_imports:
            return super().materialize(root)
        real = self.provider_target
        self.provider_target = real + [
            _v1_literal(name, bytes(4)) for name in self.provider_target_imports]
        try:
            report = super().materialize(root)
        finally:
            self.provider_target = real
        (Path(root) / "provider_target.obj").write_bytes(
            _v1_coff(real, self.provider_target_imports))
        return report


def _retarget_relocation(data, obj, section_symbol, address, new_index):
    """Byte-patch one relocation of ``section_symbol``'s section to symbol
    index ``new_index`` (a spelling the fixture builder cannot express)."""
    number = [item for item in obj["symbols"]
              if item["name"] == section_symbol and item["section"] > 0][0]["section"]
    section = obj["sections"][number - 1]
    for index in range(section["reloc_count"]):
        offset = section["reloc"] + 10 * index
        if struct.unpack_from("<L", data, offset)[0] == address:
            struct.pack_into("<L", data, offset + 4, new_index)
            return
    raise AssertionError(f"no relocation at {address:#x}")


def _section_symbol_index(obj, symbol):
    number = [item for item in obj["symbols"]
              if item["name"] == symbol and item["section"] > 0][0]["section"]
    name = obj["sections"][number - 1]["name"]
    return [item for item in obj["symbols"]
            if item["section"] == number and item["name"] == name
            and item["storage"] == 3][0]["index"]


_HELPER_PATH = (r"is not a January undefined reference of this unit: "
                r"data_unit:__real@[0-9a-f]+ \(nor a verified surplus-helper "
                r"constant: .*")


class SurplusHelperConstantTests(unittest.TestCase):
    """Surplus constants reached only through a surplus helper (RF-CO).

    The controls credit exactly the fixture's report gap; every adversarial
    case fails closed.  ``helper_path=False`` marks a case an unchanged,
    earlier check refuses before the helper path is reached.
    """

    def _rejects(self, mutate, pattern, helper_path=True, post_base=None):
        fixture = HelperFixture()
        mutate(fixture)
        expected = (_HELPER_PATH + pattern) if helper_path else pattern
        with self.assertRaisesRegex(SemanticProgressError, expected):
            _run_fixture(fixture, post_base=post_base)

    # -- controls --------------------------------------------------------
    def test_control_helper_constants_credit_only_the_report_gap(self):
        self.assertEqual(_run_fixture(HelperFixture()),
                         ["data_unit:v1-group (+56 data bytes)"])

    def test_control_section_table_permutation(self):
        # Section numbers are object-local: moving the helper and constants
        # changes no name, byte or relocation, so it is not an identity change.
        fixture = HelperFixture()
        fixture.base.insert(1, fixture.base.pop())
        fixture.base.append(fixture.base.pop(4))
        fixture.provider_target.reverse()
        self.assertEqual(_run_fixture(fixture),
                         ["data_unit:v1-group (+56 data bytes)"])

    def test_january_imported_constant_skips_the_helper_path(self):
        # January imports C1: the ordinary rule accepts it without the helper;
        # C2 still takes the helper path, which now refuses a changed helper.
        fixture = HelperFixture()
        fixture.target_externals = fixture.target_externals + [_C1]
        fixture.base[-1] = _helper_section(raw=b"\x90" + _HELPER_RAW[1:])
        with self.assertRaisesRegex(
                SemanticProgressError,
                _HELPER_PATH.replace("[0-9a-f]+", "3f1a36e2e0000000")
                + "bytes or relocations differ from January provider"):
            _run_fixture(fixture)

    # -- helper byte / relocation mismatch -------------------------------
    def test_helper_byte_differs_from_january_provider(self):
        self._rejects(lambda f: f.provider_target.__setitem__(
            1, _helper_section(selection=_SELECT_NODUPLICATES,
                               raw=_HELPER_RAW[:12] + b"\xc2" + b"\xcc" * 3)),
            "bytes or relocations differ from January provider")

    def test_helper_byte_differs_from_rebuilt_provider(self):
        self._rejects(lambda f: f.provider_base.__setitem__(
            1, _helper_section(raw=_HELPER_RAW[:-1] + b"\x90")),
            "bytes or relocations differ from rebuilt provider")

    def test_our_helper_byte_differs(self):
        self._rejects(lambda f: f.base.__setitem__(
            -1, _helper_section(raw=b"\xd9" + _HELPER_RAW[1:])),
            "bytes or relocations differ from January provider")

    def test_helper_relocation_type_differs(self):
        self._rejects(lambda f: f.base.__setitem__(-1, _helper_section(
            relocs=((2, _C2, _DIR32NB, 0), (8, _C1, _DIR32, 0)))),
            "bytes or relocations differ from January provider")

    def test_helper_relocation_addend_differs(self):
        self._rejects(lambda f: f.base.__setitem__(-1, _helper_section(
            relocs=((2, _C2, _DIR32, 0), (8, _C1, _DIR32, 4)))),
            "bytes or relocations differ from January provider")

    def test_provider_helper_section_name_differs(self):
        self._rejects(lambda f: f.provider_base.__setitem__(1, _v1_section(
            ".text$mn", _HELPER_RAW, _SCN_CODE_COMDAT, [(_HELPER, 0, 2)],
            _HELPER_RELOCS, selection=_SELECT_ANY)),
            "section symbol table differs from rebuilt provider")

    def test_provider_helper_spells_the_constant_through_a_static_alias(self):
        # The strict comparator and the image addresses both accept a static
        # alias at the constant's offset; the relocation rows must not.
        def mutate(fixture):
            fixture.provider_target[1] = _helper_section(
                selection=_SELECT_NODUPLICATES,
                relocs=((2, _C2, _DIR32, 0), (8, "_c1_alias", _DIR32, 0)))
            fixture.provider_target[2]["symbols"].append(["_c1_alias", 0, 3])
            fixture.addresses["_c1_alias"] = 0x5100
        self._rejects(mutate, "relocates through a non-external symbol")

    def test_provider_helper_spells_the_constant_as_another_name_plus_addend(self):
        # csplit-style "other external + addend" reaching the same address:
        # strict comparator and image addresses agree, the names do not.
        def mutate(fixture):
            fixture.provider_target[1] = _helper_section(
                selection=_SELECT_NODUPLICATES,
                relocs=((2, _C2, _DIR32, 0), (8, "_c1_tail", _DIR32, -4)))
            fixture.provider_target[2]["symbols"].append(["_c1_tail", 4, 2])
            fixture.addresses["_c1_tail"] = 0x5104
        self._rejects(mutate, "relocation targets differ from January provider")

    def test_helper_relocation_without_image_address(self):
        self._rejects(lambda f: f.addresses.pop(_C2),
                      "relocation has no image address")

    def test_helper_without_image_address(self):
        self._rejects(lambda f: f.addresses.pop(_HELPER),
                      "image address unavailable")

    def test_helper_section_holds_another_symbol(self):
        self._rejects(lambda f: f.base.__setitem__(-1, _helper_section(
            symbols=((_HELPER, 0, 2), ("$L100", 12, 3)))),
            "not a single-function helper")

    def test_helper_is_static(self):
        self._rejects(lambda f: f.base.__setitem__(-1, _helper_section(
            symbols=((_HELPER, 0, 3),))), "not an external at offset 0")

    def test_our_helper_is_not_select_any(self):
        self._rejects(lambda f: f.base.__setitem__(-1, _helper_section(
            selection=_SELECT_NODUPLICATES)), "has COMDAT selection 1")

    def test_our_helper_is_not_a_comdat(self):
        self._rejects(lambda f: f.base.__setitem__(-1, _helper_section(
            flags=_SCN_CODE_ALIGN16, selection=0)), "is not a code COMDAT")

    def test_rebuilt_provider_helper_is_not_select_any(self):
        self._rejects(lambda f: f.provider_base.__setitem__(1, _helper_section(
            selection=_SELECT_NODUPLICATES)), "has COMDAT selection 1")

    def test_january_provider_helper_other_selection(self):
        self._rejects(lambda f: f.provider_target.__setitem__(1, _helper_section(
            selection=5)), "has COMDAT selection 5")

    def test_provider_helper_alignment_differs(self):
        self._rejects(lambda f: f.provider_base.__setitem__(1, _helper_section(
            flags=0x60000020 | _align(6) | _SCN_COMDAT)),
            "bytes or relocations differ from rebuilt provider")

    def test_helper_reaches_constant_through_its_section_symbol(self):
        def post_base(data, obj):
            _retarget_relocation(data, obj, _HELPER, 8,
                                 _section_symbol_index(obj, _C1))
        self._rejects(lambda f: None, "relocation has no image address",
                      post_base=post_base)

    # -- constant mismatch (complete constant identity) ------------------
    def test_our_constant_differs(self):
        def mutate(fixture):
            fixture.base[2] = _c1(raw=b"\x01\x00\x80\x3f")
        self._rejects(mutate, "constant '__real@3f800000' differs from "
                              "January provider")

    def test_january_provider_constant_differs(self):
        self._rejects(lambda f: f.provider_target.__setitem__(
            2, _c1(raw=b"\x00\x00\x00\x3f")),
            "constant '__real@3f800000' differs from January provider")

    def test_rebuilt_provider_constant_differs(self):
        self._rejects(lambda f: f.provider_base.__setitem__(
            3, _c2(raw=bytes(8))),
            "constant '__real@3f1a36e2e0000000' differs from rebuilt provider")

    def test_constant_section_name_differs(self):
        self._rejects(lambda f: f.base.__setitem__(2, _v1_section(
            ".rdata$r", _C1_BYTES, _SCN_RDATA | _SCN_COMDAT | _align(3),
            [(_C1, 0, 2)], selection=_SELECT_ANY)),
            "constant '__real@3f800000' differs from January provider")

    def test_constant_alignment_differs(self):
        self._rejects(lambda f: f.base.__setitem__(3, _c2(code=5)),
                      "constant '__real@3f1a36e2e0000000' differs from "
                      "January provider")

    def test_provider_helper_imports_the_constant(self):
        # The provider's January helper loads a C1 it does not define (another
        # January object owns it): no identity against the provider exists.
        def mutate(fixture):
            del fixture.provider_target[2]
            fixture.provider_target_imports = [_C1]
        self._rejects(mutate, "constant '__real@3f800000' is not defined by "
                              "the January provider")

    def test_constant_referenced_by_another_name_in_the_provider(self):
        other = "__real@3f800001"

        def mutate(fixture):
            fixture.provider_target[1] = _helper_section(
                selection=_SELECT_NODUPLICATES,
                relocs=((2, _C2, _DIR32, 0), (8, other, _DIR32, 0)))
            fixture.provider_target.append(_v1_literal(other, _C1_BYTES))
        self._rejects(mutate, "bytes or relocations differ from January provider")

    # -- reordered sections / relocations --------------------------------
    def test_swapped_constant_references(self):
        self._rejects(lambda f: f.base.__setitem__(-1, _helper_section(
            relocs=((2, _C1, _DIR32, 0), (8, _C2, _DIR32, 0)))),
            "bytes or relocations differ from January provider")

    def test_reordered_relocation_records(self):
        self._rejects(lambda f: f.base.__setitem__(-1, _helper_section(
            relocs=tuple(reversed(_HELPER_RELOCS)))),
            "bytes or relocations differ from January provider")

    def test_provider_constants_exchanged_between_names(self):
        # Two same-sized constants exchange values in the January provider:
        # names, bytes and relocations of the helpers still agree, only the
        # constant identity check can see it.
        other = "__real@3f000000"
        half = b"\x00\x00\x00\x3f"

        def mutate(fixture):
            relocs = _HELPER_RELOCS + ((12, other, _DIR32, 0),)
            raw = _HELPER_RAW[:12] + bytes(4)
            fixture.base[-1] = _helper_section(relocs=relocs, raw=raw)
            fixture.base.insert(4, _v1_literal(other, half))
            fixture.provider_target[1] = _helper_section(
                selection=_SELECT_NODUPLICATES, relocs=relocs, raw=raw)
            fixture.provider_base[1] = _helper_section(relocs=relocs, raw=raw)
            fixture.provider_target[2] = _c1(raw=half)
            fixture.provider_target.append(_v1_literal(other, _C1_BYTES))
            fixture.provider_base.append(_v1_literal(other, half))
            fixture.addresses[other] = 0x5110
            fixture.SURPLUS = [_XYZ, _C1, _C2, other]
        self._rejects(mutate, "constant '__real@3f800000' differs from "
                              "January provider")

    # -- constant also reached from non-helper sections ------------------
    def test_constant_also_referenced_by_january_defined_code(self):
        self._rejects(lambda f: f.base.__setitem__(-2, _caller_section(
            [(8, _C1, _DIR32, 0)])), "which January's unit defines")

    def test_constant_reached_by_section_symbol_from_other_code(self):
        def post_base(data, obj):
            _retarget_relocation(data, obj, _CALLER, 8,
                                 _section_symbol_index(obj, _C1))
        self._rejects(lambda f: f.base.__setitem__(-2, _caller_section(
            [(8, _C2, _DIR32, 0)])), "which January's unit defines",
            post_base=post_base)

    def test_constant_referenced_from_a_debug_section(self):
        self._rejects(lambda f: f.base.append(_v1_section(
            ".debug$S", bytes(8), _SCN_DEBUG, [], [(4, _C1, 0x0B, 0)])),
            "reached from non-code section")

    def test_constant_referenced_by_a_second_unverified_helper(self):
        # A second rebuilt-only select-any helper January never imports.
        self._rejects(lambda f: f.base.append(_helper_section(
            symbols=(("_helper3d", 0, 2),))),
            "helper '_helper3d' is not a January undefined reference")

    # -- helper not imported by January's unit ---------------------------
    def test_helper_not_named_by_january_unit(self):
        def mutate(fixture):
            fixture.target_externals = [name for name in fixture.target_externals
                                        if name != _HELPER]
            fixture.target[-1] = _v1_section(
                ".text", b"\xc3" + b"\xcc" * 15, _SCN_CODE_ALIGN16,
                [(_CALLER, 0, 2)])
        self._rejects(mutate, "helper '_helper2d' is not a January undefined "
                              "reference of this unit\\)")

    def test_helper_named_but_never_relocated_by_january_unit(self):
        self._rejects(lambda f: f.target.__setitem__(-1, _v1_section(
            ".text", b"\xc3" + b"\xcc" * 15, _SCN_CODE_ALIGN16,
            [(_CALLER, 0, 2)])), "never relocates to helper")

    def test_helper_defined_by_january_unit(self):
        def mutate(fixture):
            fixture.target_externals = [name for name in fixture.target_externals
                                        if name != _HELPER]
            fixture.target.append(_helper_section(
                selection=_SELECT_NODUPLICATES, relocs=()))
        self._rejects(mutate, "'_helper2d', which January's unit defines")

    # -- multiple / unauthenticated providers ----------------------------
    def test_helper_has_two_january_definers(self):
        def mutate(fixture):
            fixture.other_sections = [
                _v1_section(".bss", bytes(8), _SCN_BSS | _align(3),
                            [("_bss_var", 0, 2)]),
                _helper_section(selection=_SELECT_NODUPLICATES, relocs=())]
        self._rejects(mutate, "provider 'provider_unit' is not January's "
                              "unique definer \\(definers \\['other_unit', "
                              "'provider_unit'\\]\\)")

    def test_helper_has_no_january_definer(self):
        self._rejects(lambda f: f.provider_target.pop(1),
                      "not January's unique definer \\(definers \\[\\]\\)")

    def test_constant_names_a_provider_that_does_not_define_the_helper(self):
        def mutate(fixture):
            entry = fixture.entry()
            for item in entry["surplus"]:
                if item["symbol"] == _C1:
                    item["provider"] = "other_unit"
            fixture.entries = [entry]
        self._rejects(mutate, "provider 'other_unit' is not January's unique "
                              "definer")

    def test_constant_names_its_own_unit_or_a_missing_unit(self):
        for provider in ("data_unit", "missing_unit"):
            with self.subTest(provider=provider):
                def mutate(fixture, provider=provider):
                    entry = fixture.entry()
                    for item in entry["surplus"]:
                        if item["symbol"] == _C1:
                            item["provider"] = provider
                    fixture.entries = [entry]
                self._rejects(mutate, "invalid provider")

    def test_missing_january_provider_object(self):
        fixture = HelperFixture()
        with tempfile.TemporaryDirectory() as root:
            report, arguments = _materialize(fixture, root)
            (Path(root) / "provider_target.obj").unlink()
            with self.assertRaisesRegex(SemanticProgressError,
                                        "January object missing"):
                apply_semantic_data_matches(
                    report, *arguments,
                    rescore_report_unit=trust_fixture_report(report))

    def test_constant_has_another_january_definer(self):
        # The helper's provider is unique, but another January object also
        # defines the constant: the unchanged provider check still refuses.
        def mutate(fixture):
            fixture.other_sections = [
                _v1_section(".bss", bytes(8), _SCN_BSS | _align(3),
                            [("_bss_var", 0, 2)]), _c1()]
        self._rejects(mutate, "__real@3f800000 \\(definers "
                              "\\['other_unit', 'provider_unit'\\]\\)",
                      helper_path=False)

    # -- unrelated surplus data riding along -----------------------------
    def _extra_constant(self, fixture, extra):
        fixture.base.insert(4, _v1_literal(extra, b"\x00\x00\x00\x40"))
        fixture.provider_target.append(_v1_literal(extra, b"\x00\x00\x00\x40"))
        fixture.provider_base.append(_v1_literal(extra, b"\x00\x00\x00\x40"))
        fixture.addresses[extra] = 0x5120

    def test_unreferenced_surplus_constant(self):
        def mutate(fixture):
            self._extra_constant(fixture, "__real@40000000")
            fixture.SURPLUS = [_XYZ, "__real@40000000", _C1, _C2]
        self._rejects(mutate, "no rebuilt section references it")

    def test_surplus_constant_used_only_by_january_defined_code(self):
        def mutate(fixture):
            self._extra_constant(fixture, "__real@40000000")
            fixture.base[-2] = _caller_section([(8, "__real@40000000", _DIR32, 0)])
            fixture.SURPLUS = [_XYZ, "__real@40000000", _C1, _C2]
        self._rejects(mutate, "which January's unit defines")

    def test_extra_constant_loaded_only_by_our_helper(self):
        def mutate(fixture):
            self._extra_constant(fixture, "__real@40000000")
            fixture.base[-1] = _helper_section(
                relocs=_HELPER_RELOCS + ((12, "__real@40000000", _DIR32, 0),),
                raw=_HELPER_RAW[:12] + bytes(4))
            fixture.SURPLUS = [_XYZ, _C1, _C2, "__real@40000000"]
        self._rejects(mutate, "bytes or relocations differ from January provider")

    def test_writable_data_reached_only_by_the_helper(self):
        self._rejects(lambda f: f.base.__setitem__(2, _c1(
            flags=_SCN_RDATA | _WRITE | _SCN_COMDAT | _align(3))),
            "not a read-only constant")

    def test_relocated_constant_reached_only_by_the_helper(self):
        self._rejects(lambda f: f.base.__setitem__(2, _v1_section(
            ".rdata", _C1_BYTES, _SCN_RDATA | _SCN_COMDAT | _align(3),
            [(_C1, 0, 2)], [(0, "_ext_function", _DIR32, 0)],
            selection=_SELECT_ANY)), "not a read-only constant")

    def test_helper_loads_code_this_unit_defines(self):
        def mutate(fixture):
            relocs = _HELPER_RELOCS + ((12, _CALLER, _DIR32, 0),)
            raw = _HELPER_RAW[:12] + bytes(4)
            fixture.base[-1] = _helper_section(relocs=relocs, raw=raw)
            fixture.provider_target[1] = _helper_section(
                selection=_SELECT_NODUPLICATES, relocs=relocs, raw=raw)
            fixture.provider_base[1] = _helper_section(relocs=relocs, raw=raw)
            fixture.provider_target.append(_caller_section())
            fixture.provider_base.append(_caller_section())
        self._rejects(mutate, "relocates to '_caller', which is not a data "
                              "constant")

    def test_helper_target_in_a_section_that_does_not_exist(self):
        # A malformed rebuilt symbol (section number past the section table)
        # is refused with the verifier's error, never an IndexError.
        def mutate(fixture):
            relocs = _HELPER_RELOCS + ((12, "_x_far", _DIR32, 0),)
            raw = _HELPER_RAW[:12] + bytes(4)
            fixture.base[-1] = _helper_section(relocs=relocs, raw=raw)
            fixture.base_externals = fixture.base_externals + ["_x_far"]
            fixture.provider_target[1] = _helper_section(
                selection=_SELECT_NODUPLICATES, relocs=relocs, raw=raw)
            fixture.provider_base[1] = _helper_section(relocs=relocs, raw=raw)
            fixture.provider_target.append(_v1_literal("_x_far", bytes(4)))
            fixture.provider_base.append(_v1_literal("_x_far", bytes(4)))
            fixture.addresses["_x_far"] = 0x5130

        def post_base(data, obj):
            index = [item for item in obj["symbols"]
                     if item["name"] == "_x_far"][0]["index"]
            symbol_offset = struct.unpack_from("<L", data, 8)[0]
            struct.pack_into("<h", data, symbol_offset + 18 * index + 12, 999)
        self._rejects(mutate, "relocates to '_x_far', which is not a data "
                              "constant", post_base=post_base)

    def test_helper_constant_outside_the_covered_report_sections(self):
        self._rejects(lambda f: f.base.__setitem__(2, _v1_section(
            ".rdat2", _C1_BYTES, _SCN_RDATA | _SCN_COMDAT | _align(3),
            [(_C1, 0, 2)], selection=_SELECT_ANY)),
            "outside the covered report sections", helper_path=False)

    def test_helper_constant_left_undeclared(self):
        def mutate(fixture):
            fixture.SURPLUS = [_XYZ, _C2]
        self._rejects(mutate, "leaves rebuilt sections undeclared",
                      helper_path=False)

    def test_helper_constant_declared_twice(self):
        def mutate(fixture):
            fixture.SURPLUS = [_XYZ, _C1, _C2, _C1]
        self._rejects(mutate, "repeats a section", helper_path=False)

    # -- extra members ---------------------------------------------------
    @staticmethod
    def _with_member(fixture, symbol):
        entry = fixture.entry()
        section, owner = _v1_find(fixture.base, symbol)
        snapshot = _v1_normalized_snapshot(section, owner)
        entry["members"].append({"symbol": symbol, "measurements": {
            "target": snapshot, "base": snapshot}})
        fixture.entries = [entry]

    def test_helper_constant_listed_as_a_member(self):
        self._rejects(lambda f: self._with_member(f, _C1),
                      "cannot verify semantic data group member",
                      helper_path=False)

    def test_helper_listed_as_a_member(self):
        self._rejects(lambda f: self._with_member(f, _HELPER),
                      "cannot verify semantic data group member",
                      helper_path=False)

    def test_helper_listed_as_surplus(self):
        def mutate(fixture):
            entry = fixture.entry()
            section, owner = _v1_find(fixture.base, _HELPER)
            entry["surplus"].append({
                "symbol": _HELPER, "provider": fixture.provider,
                "measurements": {"base": _v1_normalized_snapshot(section, owner)}})
            fixture.entries = [entry]
        self._rejects(mutate, "cannot verify semantic data surplus",
                      helper_path=False)

    def test_extra_matched_member(self):
        self._rejects(lambda f: self._with_member(f, "_bss_var"),
                      "does not cover the reported unmatched sections",
                      helper_path=False)

    # -- direct call without image addresses -----------------------------
    def test_helper_path_needs_image_addresses(self):
        fixture = HelperFixture()
        with tempfile.TemporaryDirectory() as root:
            fixture.materialize(root)
            config_units = {unit["name"]: unit for unit in json.loads(
                (Path(root) / "objdiff.json").read_text())["units"]}
            item = fixture.entry()["surplus"][1]
            with self.assertRaisesRegex(
                    SemanticProgressError,
                    _HELPER_PATH + "no image symbol addresses"):
                semantic_progress._verify_surplus_providers(
                    Path(root), config_units, load(Path(root) / "target.obj"),
                    load(Path(root) / "base.obj"), "data_unit", [item])


def _append_weak_external(data, name, default_index):
    """Byte-append a weak external (one aux record naming ``default_index``)
    to the symbol table; the string table after it keeps its offsets."""
    symbol_offset, count = struct.unpack_from("<LL", data, 8)
    strings = symbol_offset + 18 * count
    data[strings:strings] = (
        name.encode("ascii").ljust(8, b"\0")
        + struct.pack("<LhHBB", 0, 0, 0, 105, 1)
        + struct.pack("<LL10x", default_index, 3))
    struct.pack_into("<L", data, 12, count + 2)
    return count


class SurplusHelperReviewTests(unittest.TestCase):
    """Independent review (RF-CU) of the surplus-helper constant rule.

    Spellings the reach scan cannot follow fail closed, a helper relocation
    must address a byte of the constant whose identity is proved, and the
    "constant among the helper's constants" check is load-bearing.
    """

    def test_weak_external_to_the_constant_fails_closed(self):
        # _caller (no helper) reaches C1 through a weak external whose default
        # is C1: the relocation names another symbol, the linker binds C1.
        fixture = HelperFixture()
        fixture.base[-2] = _caller_section([(6, _C1, _DIR32, 0)])

        def weak(data, obj):
            default = [item for item in obj["symbols"]
                       if item["name"] == _C1 and item["section"] > 0][0]
            index = _append_weak_external(data, "_w_c1", default["index"])
            _retarget_relocation(data, obj, _CALLER, 6, index)
        with self.assertRaisesRegex(SemanticProgressError,
                                    _HELPER_PATH + "has a weak external"):
            _run_fixture(fixture, post_base=weak)

    def test_alternatename_directive_fails_closed(self):
        fixture = HelperFixture()
        fixture.base[-2] = _caller_section([(6, "_alias_c1", _DIR32, 0)])
        fixture.base_externals = fixture.base_externals + ["_alias_c1"]
        fixture.base.append(_v1_section(
            ".drectve", b" /ALTERNATENAME:_alias_c1=" + _C1.encode() + b" ",
            0x00100A00, []))
        with self.assertRaisesRegex(
                SemanticProgressError,
                _HELPER_PATH + "has an /ALTERNATENAME directive"):
            _run_fixture(fixture)

    def _set_addend(self, fixture, address, addend):
        relocs = [list(item) for item in _HELPER_RELOCS]
        for item in relocs:
            if item[0] == address:
                item[3] = addend
        fixture.base[-1] = _helper_section(relocs=relocs)
        fixture.provider_target[1] = _helper_section(
            selection=_SELECT_NODUPLICATES, relocs=relocs)
        fixture.provider_base[1] = _helper_section(relocs=relocs)

    def test_helper_addressing_past_the_constant_fails_closed(self):
        # all three helpers load C1+4: the four bytes after C1, whose identity
        # nothing proves
        fixture = HelperFixture()
        self._set_addend(fixture, 8, 4)
        with self.assertRaisesRegex(
                SemanticProgressError,
                _HELPER_PATH + "relocation at 0x8 addresses outside constant"):
            _run_fixture(fixture)

    def test_control_helper_addressing_inside_the_constant(self):
        # C2 is 8 bytes: C2+4 (its high dword) is inside the proved bytes
        fixture = HelperFixture()
        self._set_addend(fixture, 2, 4)
        self.assertEqual(_run_fixture(fixture),
                         ["data_unit:v1-group (+56 data bytes)"])

    def test_helper_reaching_the_constant_through_an_undefined_duplicate(self):
        # ours also carries an undefined C1 entry and the helper relocates
        # through it: the reach scan matches the name, the comparator and the
        # image addresses agree, only "C1 is among the helper's constants"
        # refuses (RF-CO's surviving mutant M18 is not implied).
        fixture = HelperFixture()
        fixture.base_externals = fixture.base_externals + [_C1]

        def undefined(data, obj):
            index = [item for item in obj["symbols"]
                     if item["name"] == _C1 and item["section"] == 0][0]["index"]
            _retarget_relocation(data, obj, _HELPER, 8, index)
        with self.assertRaisesRegex(
                SemanticProgressError,
                _HELPER_PATH + "does not reference it by name"):
            _run_fixture(fixture, post_base=undefined)


class ActionsHelperConstantProductionTests(unittest.TestCase):
    """The real source/ai/actions shape on the local build (skips without it)."""

    ROOT = Path(__file__).resolve().parents[1]
    UNIT = "source/ai/actions"
    CONSTANTS = ("__real@3f800000", "__real@3f1a36e2e0000000")

    def setUp(self):
        objdiff = self.ROOT / "objdiff.json"
        symbols = self.ROOT / "config" / "symbols.json"
        if not objdiff.is_file() or not symbols.is_file():
            self.skipTest("SKIPPED: local build inputs are unavailable")
        self.config_units = {unit["name"]: unit for unit in json.loads(
            objdiff.read_text(encoding="utf-8"))["units"]}
        unit = self.config_units.get(self.UNIT)
        paths = [self.ROOT / unit[key] for key in ("target_path", "base_path")] \
            if unit else []
        if not paths or not all(path.is_file() for path in paths):
            self.skipTest("SKIPPED: actions objects are unavailable")
        self.target, self.base = (load(path) for path in paths)
        self.addresses = semantic_progress.image_symbol_addresses(json.loads(
            symbols.read_text(encoding="utf-8")))

    def _verify(self, provider, addresses=True):
        items = [{"symbol": name, "provider": provider, "measurements": {}}
                 for name in self.CONSTANTS]
        semantic_progress._verify_surplus_providers(
            self.ROOT, self.config_units, self.target, self.base, self.UNIT,
            items, self.addresses if addresses else None)

    def test_helper_constants_verify_against_action_charge(self):
        try:
            self._verify("source/ai/action_charge")
        except SemanticProgressError as error:
            self.fail(f"actions helper constants do not verify: {error}")

    def test_helper_constants_refuse_another_provider(self):
        with self.assertRaisesRegex(SemanticProgressError,
                                    "is not January's unique definer"):
            self._verify("source/ai/action_obey")

    def test_helper_constants_refuse_without_image_addresses(self):
        with self.assertRaisesRegex(SemanticProgressError,
                                    "no image symbol addresses"):
            self._verify("source/ai/action_charge", addresses=False)


class ProductionSemanticDataManifestTests(unittest.TestCase):
    """The local build's manifest verifies; a verification failure FAILS.

    Missing build inputs are an explicitly reported skip.  Once they exist,
    any verification error is a test failure, never a skip.
    """

    ROOT = Path(__file__).resolve().parents[1]

    def setUp(self):
        self.paths = {
            "report": self.ROOT / "build" / "report.json",
            "objdiff": self.ROOT / "objdiff.json",
            "manifest": self.ROOT / "config" / "semantic_data_matches.json",
            "symbols": self.ROOT / "config" / "symbols.json",
        }
        missing = [str(path) for path in self.paths.values() if not path.is_file()]
        if missing:
            self.skipTest("SKIPPED: local build inputs are unavailable: "
                          + ", ".join(missing))

    def test_pinned_scorer_reproduces_the_live_report(self):
        scorer = self.ROOT / semantic_progress.DEFAULT_SCORER_PATH
        if not scorer.is_file():
            self.skipTest(f"SKIPPED: pinned scorer not present at {scorer}")
        report = json.loads(self.paths["report"].read_text(encoding="utf-8"))
        objdiff = json.loads(self.paths["objdiff"].read_text(encoding="utf-8"))
        config_units = {unit["name"]: unit for unit in objdiff["units"]}
        live = {unit["name"]: unit for unit in report["units"]}
        for name in ("source/hs/hs", "source/shell/shell_xbox"):
            with self.subTest(unit=name):
                fresh = rescore_unit_with_pinned_scorer(
                    self.ROOT, objdiff, config_units[name],
                    OBJDIFF_331_COMBINED_EXTENT)
                self.assertEqual(
                    semantic_progress._report_data_view(fresh, name),
                    semantic_progress._report_data_view(live[name], name))

    def test_manifest_verifies_and_model_entries_credit_their_gap(self):
        report = json.loads(self.paths["report"].read_text(encoding="utf-8"))
        before = {unit["name"]: copy.deepcopy(unit["measures"])
                  for unit in report["units"]}
        entries = json.loads(self.paths["manifest"].read_text(encoding="utf-8"))
        try:
            notes = apply_semantic_data_matches(
                report, self.ROOT, self.paths["manifest"],
                self.paths["objdiff"], self.paths["symbols"])
        except SemanticProgressError as error:
            self.fail(f"current build does not verify: {error}")
        units = {unit["name"]: unit for unit in report["units"]}
        for entry in entries:
            if "extent_model" not in entry:
                continue
            measures = before[entry["unit"]]
            gap = int(measures.get("total_data", 0)) \
                - int(measures.get("matched_data", 0))
            after = units[entry["unit"]]["measures"]
            self.assertEqual(int(after["matched_data"]),
                             int(measures.get("total_data", 0)))
            if gap:
                self.assertIn(
                    f"{entry['unit']}:{entry['group']} (+{gap} data bytes)", notes)


class SymbolOwnershipSnapshotTests(unittest.TestCase):
    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.target_path = self.root / "target.obj"
        self.base_path = self.root / "base.obj"
        self.manifest_path = self.root / "symbol_ownership.json"
        self.config_path = self.root / "objdiff.json"

        self.target_path.write_bytes(self._object())
        self.base_path.write_bytes(self._object())
        self.config_path.write_text(json.dumps({
            "units": [{
                "name": "unit",
                "target_path": "target.obj",
                "base_path": "base.obj",
            }],
        }), encoding="utf-8")
        self.manifest_path.write_text(json.dumps([{
            "unit": "unit",
            "section": ".bss",
            "snapshot": self._snapshot(),
        }]), encoding="utf-8")

    def tearDown(self):
        self.temporary_directory.cleanup()

    @staticmethod
    def _symbols(globals_storage=3):
        return [
            {"name": "_name", "value": 0, "section": 1,
             "type": 0, "storage": 3},
            {"name": "_pool", "value": 4, "section": 1,
             "type": 0, "storage": 3},
            {"name": "_globals", "value": 8, "section": 1,
             "type": 0, "storage": globals_storage},
            {"name": "_debug", "value": 12, "section": 1,
             "type": 0, "storage": 2},
        ]

    @classmethod
    def _object(cls, globals_storage=3):
        return build_coff(
            sections=[{
                "name": ".bss",
                "size": 13,
                "raw_data": b"\0" * 13,
                "flags": 0xC0300080,
            }],
            symbols=cls._symbols(globals_storage),
        )

    @staticmethod
    def _snapshot():
        return {
            "size": 13,
            "flags": 0xC0300080,
            "relocation_count": 0,
            "normalized_sha256":
                "dd46c3eebb1884ff3b5258c0a2fc9398e560a29e0780d4b53869b6254aa46a96",
            "symbols": [
                {"name": "_name", "value": 0, "type": 0, "storage": 3},
                {"name": "_pool", "value": 4, "type": 0, "storage": 3},
                {"name": "_globals", "value": 8, "type": 0, "storage": 3},
                {"name": "_debug", "value": 12, "type": 0, "storage": 2},
            ],
        }

    def _validate(self):
        return require_symbol_ownership_snapshots(
            self.root, self.manifest_path, self.config_path)

    def test_exact_symbol_ownership_snapshot_passes(self):
        self.assertEqual(self._validate(), ["unit:.bss (4 symbols)"])

    def test_storage_class_drift_fails_closed(self):
        self.base_path.write_bytes(self._object(globals_storage=2))
        with self.assertRaisesRegex(
            SemanticProgressError, "rebuilt ownership snapshot changed"
        ):
            self._validate()


if __name__ == "__main__":
    unittest.main()
