import copy
import json
import struct
import tempfile
import unittest
from pathlib import Path

from tools.coff_compare import build_coff
from tools.regression_gate import (
    BSS_SENTINEL,
    GateError,
    _capture_unit,
    compare_manifests,
    load_baseline,
)


CODE_FLAGS = 0x60001020
BSS_FLAGS = 0xC0000080


class RegressionGateTests(unittest.TestCase):
    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.target_path = self.root / "target.obj"
        self.base_path = self.root / "base.obj"
        self.unit_config = {
            "name": "source/example",
            "target_path": "target.obj",
            "base_path": "base.obj",
        }
        self.meaningful_sizes = {"_first": 3, "_second": 4}

    def tearDown(self):
        self.temporary_directory.cleanup()

    @staticmethod
    def _object(
        first=b"\x31\xc0\xc3",
        second=b"\x40\x48\x90\xc3",
        *,
        bss=True,
        common=False,
        first_storage=2,
    ):
        sections = [
            {
                "name": ".text",
                "size": 16,
                "raw_data": first + b"\x90" * (16 - len(first)),
                "flags": CODE_FLAGS,
            },
            {
                "name": ".text",
                "size": 16,
                "raw_data": second + b"\x90" * (16 - len(second)),
                "flags": CODE_FLAGS,
            },
        ]
        symbols = [
            {
                "name": "_first",
                "value": 0,
                "section": 1,
                "type": 0x20,
                "storage": first_storage,
            },
            {
                "name": "_second",
                "value": 0,
                "section": 2,
                "type": 0x20,
                "storage": 2,
            },
        ]
        if bss:
            sections.append(
                {
                    "name": ".bss",
                    "size": 8,
                    "raw_data": b"\0" * 8,
                    "flags": BSS_FLAGS,
                }
            )
            symbols.append(
                {
                    "name": "_queue",
                    "value": 0,
                    "section": 3,
                    "type": 0,
                    "storage": 3,
                }
            )
        elif common:
            symbols.append(
                {
                    "name": "_queue",
                    "value": 8,
                    "section": 0,
                    "type": 0,
                    "storage": 2,
                }
            )

        result = bytearray(build_coff(sections=sections, symbols=symbols))
        if bss:
            # IMAGE_SCN_CNT_UNINITIALIZED_DATA has a logical size but no raw
            # payload.  Offset zero is the COFF header and must never be hashed.
            bss_header = 20 + 2 * 40
            struct.pack_into("<L", result, bss_header + 20, 0)
        return bytes(result)

    @staticmethod
    def _relocated_object(destination):
        raw = bytearray(b"\x90" * 16)
        struct.pack_into("<i", raw, 4, 0)
        relocation = struct.pack("<LLH", 4, 1, 0x14)
        return build_coff(
            sections=[{
                "name": ".text",
                "size": 16,
                "raw_data": bytes(raw),
                "reloc_count": 1,
                "reloc_data": relocation,
                "flags": CODE_FLAGS,
            }],
            symbols=[
                {
                    "name": "_first",
                    "value": 0,
                    "section": 1,
                    "type": 0x20,
                    "storage": 2,
                },
                {
                    "name": destination,
                    "value": 0,
                    "section": 0,
                    "type": 0,
                    "storage": 2,
                },
            ],
        )

    def _capture(self):
        return _capture_unit(
            self.root,
            self.unit_config,
            self.meaningful_sizes,
            {},
            [],
        )

    @staticmethod
    def _manifest(unit):
        return {
            "schema_version": 1,
            "commit": "a" * 40,
            "build_environment": {
                "compiler_sha256": "compiler",
                "flags_by_unit": {"source/example": "/O2"},
            },
            "semantic_exceptions": [],
            "units": {"source/example": unit},
        }

    def _write_baseline_objects(self):
        self.target_path.write_bytes(self._object())
        self.base_path.write_bytes(self._object())

    def test_stable_no_change_passes(self):
        self._write_baseline_objects()
        baseline = self._manifest(self._capture())
        current = self._manifest(self._capture())

        result = compare_manifests(baseline, current)

        self.assertTrue(result["ok"])
        self.assertEqual(
            result["units"]["source/example"]["still_exact"],
            ["_first", "_second"],
        )

    def test_unrelated_exact_function_regression_fails(self):
        self._write_baseline_objects()
        baseline = self._manifest(self._capture())
        self.base_path.write_bytes(self._object(second=b"\x41\x48\x90\xc3"))
        current = self._manifest(self._capture())

        result = compare_manifests(baseline, current)

        self.assertFalse(result["ok"])
        regressions = [
            item for item in result["failures"] if item["kind"] == "REGRESSED"
        ]
        self.assertEqual([item["item"] for item in regressions], ["_second"])

    def test_silent_bss_to_common_disappearance_fails(self):
        self._write_baseline_objects()
        baseline = self._manifest(self._capture())
        self.base_path.write_bytes(self._object(bss=False, common=True))
        current = self._manifest(self._capture())

        result = compare_manifests(baseline, current)

        self.assertFalse(result["ok"])
        kinds = {item["kind"] for item in result["failures"]}
        self.assertIn("DATA_CHANGED", kinds)
        self.assertIn("SYMBOL_SET_CHANGED", kinds)

    def test_bss_uses_no_raw_sentinel(self):
        self._write_baseline_objects()

        unit = self._capture()

        bss_sections = [
            item
            for item in unit["base"]["non_code_sections"].values()
            if item["kind"] == "BSS"
        ]
        self.assertEqual(len(bss_sections), 1)
        self.assertEqual(bss_sections[0]["normalized_sha256"], BSS_SENTINEL)
        self.assertFalse(bss_sections[0]["raw_present"])

    def test_symbol_storage_change_fails_even_when_bytes_match(self):
        self._write_baseline_objects()
        baseline = self._manifest(self._capture())
        self.base_path.write_bytes(self._object(first_storage=3))
        current = self._manifest(self._capture())

        result = compare_manifests(baseline, current)

        self.assertFalse(result["ok"])
        self.assertIn(
            "SYMBOL_SET_CHANGED",
            {item["kind"] for item in result["failures"]},
        )

    def test_missing_baseline_fails_closed(self):
        missing = self.root / "does-not-exist.json"
        with self.assertRaisesRegex(GateError, "baseline is missing"):
            load_baseline(missing)

    def test_newly_exact_is_warning_not_credit(self):
        self.target_path.write_bytes(self._object())
        self.base_path.write_bytes(self._object(second=b"\x41\x48\x90\xc3"))
        baseline = self._manifest(self._capture())
        self.base_path.write_bytes(self._object())
        current = self._manifest(self._capture())

        result = compare_manifests(baseline, current)

        self.assertTrue(result["ok"])
        self.assertEqual(
            result["units"]["source/example"]["newly_exact"], ["_second"]
        )
        self.assertEqual(result["warnings"][0]["kind"], "NEWLY_EXACT")

    def test_semantic_exception_identity_change_fails(self):
        self._write_baseline_objects()
        baseline = self._manifest(self._capture())
        current = copy.deepcopy(baseline)
        current["semantic_exceptions"] = [
            {
                "ledger": "semantic_matches",
                "unit": "source/example",
                "item": "_first",
                "identity": "changed",
            }
        ]

        result = compare_manifests(baseline, current)

        self.assertFalse(result["ok"])
        self.assertIn("UNKNOWN", {item["kind"] for item in result["failures"]})

    def test_meaningful_and_padded_sizes_are_both_recorded(self):
        self._write_baseline_objects()

        unit = self._capture()

        fingerprint = unit["target"]["functions"]["_first"]
        self.assertEqual(fingerprint["meaningful_size"], 3)
        self.assertEqual(fingerprint["padded_size"], 16)

    def test_ordinary_accepted_strict_mismatch_is_frozen_not_called_exact(self):
        self.target_path.write_bytes(self._relocated_object("_target_a"))
        self.base_path.write_bytes(self._relocated_object("_target_b"))
        unit = _capture_unit(
            self.root,
            self.unit_config,
            {"_first": 3},
            {},
            [],
            {"_first": 100.0},
        )

        function = unit["functions"]["_first"]
        self.assertEqual(
            function["state"], "ORDINARY_ACCEPTED_STRICT_MISMATCH"
        )
        self.assertTrue(function["accepted"])
        relocation = unit["base"]["functions"]["_first"]["relocations"][0]
        self.assertEqual(relocation["address"], 4)
        self.assertEqual(relocation["type"], 0x14)
        self.assertEqual(relocation["addend"], 0)
        self.assertEqual(
            relocation["resolved_destination"], ["symbol", "_target_b", 0]
        )
        self.assertEqual(relocation["target_symbol"]["name"], "_target_b")

    def test_ordinary_accepted_mismatch_destination_change_fails(self):
        self.target_path.write_bytes(self._relocated_object("_target_a"))
        self.base_path.write_bytes(self._relocated_object("_target_b"))
        baseline_unit = _capture_unit(
            self.root,
            self.unit_config,
            {"_first": 3},
            {},
            [],
            {"_first": 100.0},
        )
        baseline = self._manifest(baseline_unit)
        self.base_path.write_bytes(self._relocated_object("_target_c"))
        current_unit = _capture_unit(
            self.root,
            self.unit_config,
            {"_first": 3},
            {},
            [],
            {"_first": 100.0},
        )

        result = compare_manifests(baseline, self._manifest(current_unit))

        self.assertFalse(result["ok"])
        self.assertIn(
            "SYMBOL_SET_CHANGED",
            {item["kind"] for item in result["failures"]},
        )


if __name__ == "__main__":
    unittest.main()
