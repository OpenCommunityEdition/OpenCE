import copy
import json
import tempfile
import unittest
from pathlib import Path

from tools.coff_compare import build_coff, make_section_raw
from tools.semantic_progress import (
    SemanticProgressError,
    apply_semantic_data_matches,
    apply_semantic_matches,
    apply_semantic_rejections,
)


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
    def _object(raw):
        return build_coff(
            sections=[{"name": ".text", "size": 16, "raw_data": raw}],
            symbols=[{
                "name": "_fn",
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

    def test_changed_object_refuses_credit(self):
        self.base_path.write_bytes(self._object(b"\xcc" * 16))

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
                "metadata": {"progress_categories": ["game"]},
            }],
        }

    def tearDown(self):
        self.temporary_directory.cleanup()

    @staticmethod
    def _object(raw):
        return build_coff(
            sections=[{"name": ".data", "size": 16, "raw_data": raw}],
            symbols=[{
                "name": "_data", "value": 0, "section": 1,
                "type": 0, "storage": 2,
            }],
        )

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


if __name__ == "__main__":
    unittest.main()
