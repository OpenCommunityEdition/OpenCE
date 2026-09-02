from pathlib import Path
import subprocess
import sys
import unittest

from tools.audit_object_admission import classify_units


def _unit(name, complete, total_functions, matched_functions, total_data, matched_data):
    return {
        "name": name,
        "metadata": {"complete": complete},
        "measures": {
            "total_functions": total_functions,
            "matched_functions": matched_functions,
            "total_data": total_data,
            "matched_data": matched_data,
        },
    }


class ObjectAdmissionAuditTests(unittest.TestCase):
    def test_direct_script_entrypoint_imports_repository_tools(self):
        repository_root = Path(__file__).resolve().parents[1]
        result = subprocess.run(
            [
                sys.executable,
                str(repository_root / "tools" / "audit_object_admission.py"),
                "--help",
            ],
            cwd=repository_root,
            capture_output=True,
            text=True,
            check=False,
        )

        self.assertEqual(result.returncode, 0, result.stderr)

    def test_lists_unmarked_zero_gap_unit_without_admitting_it(self):
        raw = {"units": [_unit("source/a", False, 2, 1, 8, 8)]}
        strict = {"units": [_unit("source/a", False, 2, 2, 8, 8)]}

        result = classify_units(raw, strict)

        self.assertEqual([item["unit"] for item in result["candidates"]], ["source/a"])
        self.assertEqual(
            result["candidates"][0]["decision"],
            "audit-coff-ownership-before-admission",
        )
        self.assertEqual(result["contradicted"], [])
        self.assertEqual(result["rejected"], [])

    def test_lists_complete_label_with_strict_gap_as_contradicted(self):
        raw = {"units": [_unit("source/b", True, 3, 3, 12, 12)]}
        strict = {"units": [_unit("source/b", False, 3, 2, 12, 12)]}

        result = classify_units(raw, strict)

        self.assertEqual(result["candidates"], [])
        self.assertEqual(
            [item["unit"] for item in result["contradicted"]], ["source/b"]
        )
        self.assertEqual(result["rejected"], [])

    def test_does_not_list_incomplete_or_already_complete_units(self):
        units = [
            _unit("source/incomplete", False, 2, 1, 0, 0),
            _unit("source/complete", True, 2, 2, 0, 0),
        ]

        result = classify_units({"units": units}, {"units": units})

        self.assertEqual(
            result,
            {"candidates": [], "contradicted": [], "rejected": []},
        )

    def test_does_not_list_zero_content_placeholder_as_candidate(self):
        unit = _unit("libs/empty", False, 0, 0, 0, 0)

        result = classify_units({"units": [unit]}, {"units": [unit]})

        self.assertEqual(
            result,
            {"candidates": [], "contradicted": [], "rejected": []},
        )

    def test_reviewed_ownership_failure_is_not_an_admission_candidate(self):
        unit = _unit("source/game/player_rumble", False, 12, 12, 68, 68)
        rejection = {
            "source/game/player_rumble": [
                {
                    "unit": "source/game/player_rumble",
                    "class": "candidate-only-comdat-owner",
                    "symbol": "_fast_ftol",
                    "reason": "candidate-only owner",
                    "evidence": "docs/evidence.md",
                    "reopen": "prove compatible owner",
                }
            ]
        }

        result = classify_units(
            {"units": [unit]},
            {"units": [unit]},
            rejection,
        )

        self.assertEqual(result["candidates"], [])
        self.assertEqual(result["contradicted"], [])
        self.assertEqual(
            result["rejected"][0]["decision"], "object-admission-rejected"
        )
        self.assertEqual(
            result["rejected"][0]["rejections"][0]["symbol"], "_fast_ftol"
        )


if __name__ == "__main__":
    unittest.main()
