import json
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
from unittest.mock import patch

from tools.campaign.stable_verdicts import build_snapshot, compare_snapshots, diff_snapshots


class StableVerdictsTests(unittest.TestCase):
    def test_legacy_verdict_map_includes_fastcall_owner(self):
        root = Path(__file__).resolve().parent.parent
        target = root / "build/split/source/math/periodic_functions.obj"
        if not target.is_file():
            self.skipTest("January periodic_functions object is unavailable")

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "verdicts.json"
            subprocess.run(
                [sys.executable, str(root / "tools/campaign/verdicts.py"), str(output)],
                cwd=root, capture_output=True, text=True, check=True,
            )
            verdicts = json.loads(output.read_text(encoding="utf-8"))

        self.assertIn(
            "source/math/periodic_functions::"
            "@periodic_function_build_variable_period_x_table@4",
            verdicts,
        )

    def test_snapshot_uses_coff_function_type_not_name_prefix(self):
        target = {
            "sections": [{"name": ".text"}] * 3,
            "symbols": [
                {"name": ".text", "section": 1, "type": 0,
                 "storage": 3, "value": 0},
                {"name": "@fastcall@4", "section": 1, "type": 0x20,
                 "storage": 3, "value": 0},
                {"name": "_not_a_function", "section": 2, "type": 0,
                 "storage": 3, "value": 0},
                {"name": "_ordinary", "section": 3, "type": 0x20,
                 "storage": 2, "value": 0},
            ],
        }
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "target.obj").touch()
            config = root / "objdiff.json"
            config.write_text(json.dumps({"units": [{
                "name": "unit", "target_path": "target.obj", "base_path": None,
            }]}), encoding="utf-8")
            with patch("tools.campaign.stable_verdicts.coff_compare.load", return_value=target), \
                    patch("tools.campaign.stable_verdicts.coff_compare.section_info",
                          return_value={"size": 32, "normalized_sha256": "hash"}):
                snapshot = build_snapshot(root, config)

        self.assertEqual(set(snapshot), {"unit::section:1", "unit::section:3"})
        self.assertEqual(snapshot["unit::section:1"]["name"], "@fastcall@4")

    def test_semantic_rename_keeps_exact_identity(self):
        before = {
            "unit::section:7": {
                "status": "E",
                "name": "_code_00000010",
                "size": 32,
            }
        }
        after = {
            "unit::section:7": {
                "status": "E",
                "name": "_semantic_name",
                "size": 32,
            }
        }

        self.assertEqual(compare_snapshots(before, after), ([], []))

    def test_exact_loss_is_a_regression(self):
        before = {
            "unit::section:2": {"status": "E", "name": "_owner", "size": 48}
        }
        after = {
            "unit::section:2": {"status": "R", "name": "_owner", "size": 48}
        }

        lost, gained = compare_snapshots(before, after)
        self.assertEqual(lost, ["unit::section:2"])
        self.assertEqual(gained, [])

    def test_diff_reports_gained_bytes_and_fails_on_loss(self):
        before = {
            "unit::section:1": {"status": "E", "name": "_lost", "size": 16},
            "unit::section:2": {"status": "U", "name": "_gain", "size": 24},
        }
        after = {
            "unit::section:1": {"status": "R", "name": "_lost", "size": 16},
            "unit::section:2": {"status": "E", "name": "_gain", "size": 24},
        }

        with tempfile.TemporaryDirectory() as directory:
            before_path = Path(directory) / "before.json"
            after_path = Path(directory) / "after.json"
            before_path.write_text(json.dumps(before), encoding="utf-8")
            after_path.write_text(json.dumps(after), encoding="utf-8")
            output = StringIO()
            with redirect_stdout(output):
                result = diff_snapshots(before_path, after_path)

        self.assertEqual(result, 1)
        self.assertIn("gained 1 24 bytes; regressions 1", output.getvalue())


if __name__ == "__main__":
    unittest.main()
