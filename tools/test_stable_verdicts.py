import json
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path

from tools.campaign.stable_verdicts import compare_snapshots, diff_snapshots


class StableVerdictsTests(unittest.TestCase):
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
