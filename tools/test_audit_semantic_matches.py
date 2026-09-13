import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.audit_semantic_matches import (
    SemanticAuditError,
    audit,
    load_manual_rejections,
    relocation_shape_matches,
    section_fingerprint,
)


def info(size=16, digest="same", relocations=None):
    relocations = [] if relocations is None else relocations
    return {
        "size": size,
        "normalized_sha256": digest,
        "relocation_count": len(relocations),
        "relocations": relocations,
    }


class RelocationShapeTests(unittest.TestCase):
    def test_accepts_symbol_ownership_difference(self):
        target = info(relocations=[{
            "address": 4,
            "type": 6,
            "target": ["symbol", "_target_owner", 12],
        }])
        base = info(relocations=[{
            "address": 4,
            "type": 6,
            "target": ["symbol", "_candidate_owner", 0],
        }])
        self.assertTrue(relocation_shape_matches(target, base))

    def test_rejects_relocation_count_difference(self):
        target = info(relocations=[])
        base = info(relocations=[{
            "address": 4,
            "type": 6,
            "target": ["symbol", "__except_list", 0],
        }])
        self.assertFalse(relocation_shape_matches(target, base))

    def test_rejects_relocation_address_or_type_difference(self):
        target = info(relocations=[{
            "address": 4,
            "type": 6,
            "target": ["symbol", "_value", 0],
        }])
        moved = info(relocations=[{
            "address": 8,
            "type": 6,
            "target": ["symbol", "_value", 0],
        }])
        retyped = info(relocations=[{
            "address": 4,
            "type": 20,
            "target": ["symbol", "_value", 0],
        }])
        self.assertFalse(relocation_shape_matches(target, moved))
        self.assertFalse(relocation_shape_matches(target, retyped))

    def test_rejects_size_or_normalized_bytes_difference(self):
        target = info()
        self.assertFalse(relocation_shape_matches(target, info(size=32)))
        self.assertFalse(relocation_shape_matches(target, info(digest="different")))


class ManualSemanticCreditRejectionTests(unittest.TestCase):
    def make_fixture(self, root, mutate_manifest=None):
        target = info(
            digest="1" * 64,
            relocations=[{
                "address": 4,
                "type": 6,
                "target": ["symbol", "_target_owner", 5432],
            }],
        )
        candidate = info(
            digest="1" * 64,
            relocations=[{
                "address": 4,
                "type": 6,
                "target": ["symbol", "_candidate_owner", 0],
            }],
        )
        report_path = root / "report.json"
        report_path.write_text(json.dumps({
            "units": [{
                "name": "source/example",
                "functions": [{
                    "name": "_example",
                    "size": 16,
                    "fuzzy_match_percent": 100.0,
                }],
            }],
        }), encoding="utf-8")
        config_path = root / "objdiff.json"
        config_path.write_text(json.dumps({
            "units": [{
                "name": "source/example",
                "target_path": "target.obj",
                "base_path": "candidate.obj",
            }],
        }), encoding="utf-8")
        evidence_path = root / "docs" / "evidence.md"
        evidence_path.parent.mkdir()
        evidence_path.write_text("# Evidence\n", encoding="utf-8")
        manifest = {
            "version": 1,
            "entries": [{
                "unit": "source/example",
                "function": "_example",
                "code_bytes": 16,
                "target": section_fingerprint(target),
                "candidate": section_fingerprint(candidate),
                "reason": "The source-authentic owner differs from the split artifact.",
                "evidence": "docs/evidence.md#evidence",
            }],
        }
        if mutate_manifest is not None:
            mutate_manifest(manifest)
        manifest_path = root / "semantic_credit_rejections.json"
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        return report_path, config_path, manifest_path, target, candidate

    def run_fixture(self, root, fixture):
        report_path, config_path, manifest_path, target, candidate = fixture
        with patch(
            "tools.audit_semantic_matches.load",
            side_effect=["target-object", "candidate-object"],
        ), patch(
            "tools.audit_semantic_matches.section_info",
            side_effect=lambda obj, _name: (
                target if obj == "target-object" else candidate
            ),
        ):
            return audit(root, report_path, config_path, manifest_path)

    def test_manual_veto_removes_structural_objdiff_credit(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            result = self.run_fixture(root, self.make_fixture(root))

        self.assertEqual(result["summary"]["ordinary_only"], 1)
        self.assertEqual(result["summary"]["ordinary_structural"], 0)
        self.assertEqual(result["summary"]["ordinary_rejected"], 1)
        self.assertEqual(result["summary"]["manual_rejected"], 1)
        self.assertEqual(result["summary"]["accepted_exact"], 0)
        self.assertEqual(result["accepted_ledger"], [])
        rejection = result["ordinary_rejected"][0]
        self.assertEqual(
            rejection["rejection"]["source"],
            "manual-semantic-credit-veto",
        )
        self.assertEqual(rejection["code_bytes"], 16)

    def test_candidate_relocation_owner_drift_fails_closed(self):
        def mutate(manifest):
            manifest["entries"][0]["candidate"]["relocation_sha256"] = "0" * 64

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            fixture = self.make_fixture(root, mutate)
            with self.assertRaisesRegex(
                SemanticAuditError,
                "stale semantic-credit rejection.*candidate fingerprint",
            ):
                self.run_fixture(root, fixture)

    def test_missing_manifest_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing.json"
            with self.assertRaisesRegex(
                SemanticAuditError,
                "manifest does not exist",
            ):
                load_manual_rejections(Path(directory), missing)


if __name__ == "__main__":
    unittest.main()
