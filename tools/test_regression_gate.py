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
    XDK_D3DINLINE_RECIPE,
    _capture_unit,
    _function_code_evidence,
    _json_hash,
    _object_fingerprint,
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

    def test_same_location_alias_inherits_unique_report_size(self):
        self.target_path.write_bytes(build_coff(
            sections=[{
                "name": ".text",
                "size": 16,
                "raw_data": b"\x31\xc0\xc3" + b"\x90" * 13,
                "flags": CODE_FLAGS,
            }],
            symbols=[
                {
                    "name": "_reported",
                    "value": 0,
                    "section": 1,
                    "type": 0x20,
                    "storage": 2,
                },
                {
                    "name": "_legacy_alias",
                    "value": 0,
                    "section": 1,
                    "type": 0x20,
                    "storage": 2,
                },
            ],
        ))

        fingerprint = _object_fingerprint(
            self.target_path,
            {"_reported": 3},
            {},
            require_meaningful_sizes=True,
        )

        self.assertEqual(
            fingerprint["functions"]["_legacy_alias"]["meaningful_size"], 3
        )
        self.assertEqual(
            fingerprint["functions"]["_reported"]["meaningful_size"], 3
        )

    def test_same_location_alias_refuses_ambiguous_report_sizes(self):
        self.target_path.write_bytes(build_coff(
            sections=[{
                "name": ".text",
                "size": 16,
                "raw_data": b"\x31\xc0\xc3" + b"\x90" * 13,
                "flags": CODE_FLAGS,
            }],
            symbols=[
                {
                    "name": "_reported_one",
                    "value": 0,
                    "section": 1,
                    "type": 0x20,
                    "storage": 2,
                },
                {
                    "name": "_reported_two",
                    "value": 0,
                    "section": 1,
                    "type": 0x20,
                    "storage": 2,
                },
                {
                    "name": "_legacy_alias",
                    "value": 0,
                    "section": 1,
                    "type": 0x20,
                    "storage": 2,
                },
            ],
        ))

        with self.assertRaisesRegex(
            GateError, "identical-location report candidates:.*_reported_one.*_reported_two"
        ):
            _object_fingerprint(
                self.target_path,
                {"_reported_one": 3, "_reported_two": 3},
                {},
                require_meaningful_sizes=True,
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

    @staticmethod
    def _wrapper_adjudication(baseline, current, function="_first"):
        unit = "source/example"
        before = baseline["units"][unit]
        after = current["units"][unit]
        before_fingerprint = before["base"]["functions"][function]
        after_fingerprint = after["base"]["functions"][function]
        target_fingerprint = after["target"]["functions"][function]
        return {
            "schema_version": 1,
            "functions": [
                {
                    "unit": unit,
                    "function": function,
                    "recipe": XDK_D3DINLINE_RECIPE,
                    "source_recipe": (
                        "stock XDK D3DINLINE (static __forceinline)"
                    ),
                    "before_comdat_selection": 1,
                    "after_comdat_selection": 2,
                    "before_fingerprint_sha256": _json_hash(before_fingerprint),
                    "after_fingerprint_sha256": _json_hash(after_fingerprint),
                    "target_evidence_sha256": _json_hash(
                        _function_code_evidence(target_fingerprint)
                    ),
                }
            ],
            "debug_sections": [],
        }

    def _comdat_transition(self):
        self._write_baseline_objects()
        baseline = self._manifest(self._capture())
        current = copy.deepcopy(baseline)
        before = baseline["units"]["source/example"]["base"]["functions"]["_first"]
        after = current["units"]["source/example"]["base"]["functions"]["_first"]
        before["comdat_selection"] = 1
        after["comdat_selection"] = 2
        return baseline, current

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

    def test_reviewed_xdk_comdat_transition_passes(self):
        baseline, current = self._comdat_transition()
        adjudications = self._wrapper_adjudication(baseline, current)

        result = compare_manifests(
            baseline, current, adjudications=adjudications
        )

        self.assertTrue(result["ok"])
        self.assertEqual(
            result["units"]["source/example"]["adjudicated_exact"],
            ["_first"],
        )

    def test_exact_recorded_xdk_debug_transition_passes(self):
        baseline, current = self._comdat_transition()
        adjudications = self._wrapper_adjudication(baseline, current)
        unit = "source/example"
        section = ".debug$F|anonymous=0"
        source = next(
            iter(baseline["units"][unit]["base"]["non_code_sections"].values())
        )
        before_debug = copy.deepcopy(source)
        before_debug.update(
            {
                "identity": section,
                "name": ".debug$F",
                "kind": "DEBUG",
                "normalized_sha256": "1" * 64,
            }
        )
        after_debug = copy.deepcopy(before_debug)
        after_debug["normalized_sha256"] = "2" * 64
        baseline["units"][unit]["base"]["non_code_sections"][section] = (
            before_debug
        )
        current["units"][unit]["base"]["non_code_sections"][section] = (
            after_debug
        )
        adjudications["debug_sections"].append(
            {
                "unit": unit,
                "section": section,
                "recipe": XDK_D3DINLINE_RECIPE,
                "before_fingerprint_sha256": _json_hash(before_debug),
                "after_fingerprint_sha256": _json_hash(after_debug),
            }
        )

        result = compare_manifests(
            baseline, current, adjudications=adjudications
        )

        self.assertTrue(result["ok"])
        self.assertIn(
            "ADJUDICATED_DEBUG_CHANGE",
            {item["kind"] for item in result["warnings"]},
        )

    def test_xdk_comdat_adjudication_wrong_hash_fails(self):
        baseline, current = self._comdat_transition()
        adjudications = self._wrapper_adjudication(baseline, current)
        adjudications["functions"][0]["after_fingerprint_sha256"] = "0" * 64

        result = compare_manifests(
            baseline, current, adjudications=adjudications
        )

        self.assertFalse(result["ok"])
        self.assertIn("UNKNOWN", {item["kind"] for item in result["failures"]})

    def test_xdk_comdat_adjudication_wrong_relocation_destination_fails(self):
        self.target_path.write_bytes(self._relocated_object("_target_a"))
        self.base_path.write_bytes(self._relocated_object("_target_a"))
        unit = _capture_unit(
            self.root,
            self.unit_config,
            {"_first": 3},
            {},
            [],
        )
        baseline = self._manifest(unit)
        current = copy.deepcopy(baseline)
        before = baseline["units"]["source/example"]["base"]["functions"]["_first"]
        after = current["units"]["source/example"]["base"]["functions"]["_first"]
        before["comdat_selection"] = 1
        after["comdat_selection"] = 2
        adjudications = self._wrapper_adjudication(baseline, current)
        after["relocations"][0]["resolved_destination"] = [
            "symbol", "_wrong_target", 0
        ]

        result = compare_manifests(
            baseline, current, adjudications=adjudications
        )

        self.assertFalse(result["ok"])
        self.assertIn("UNKNOWN", {item["kind"] for item in result["failures"]})

    def test_unlisted_comdat_function_change_fails(self):
        baseline, current = self._comdat_transition()

        result = compare_manifests(baseline, current)

        self.assertFalse(result["ok"])
        self.assertIn("UNKNOWN", {item["kind"] for item in result["failures"]})

    def test_adjudication_does_not_hide_extra_data_change(self):
        baseline, current = self._comdat_transition()
        adjudications = self._wrapper_adjudication(baseline, current)
        bss = next(
            iter(
                current["units"]["source/example"]["base"]
                ["non_code_sections"].values()
            )
        )
        bss["logical_size"] += 4

        result = compare_manifests(
            baseline, current, adjudications=adjudications
        )

        self.assertFalse(result["ok"])
        self.assertIn(
            "DATA_CHANGED", {item["kind"] for item in result["failures"]}
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

    def test_semantic_alias_compares_explicit_base_function_strictly(self):
        target = build_coff(
            sections=[{
                "name": ".text",
                "size": 16,
                "raw_data": b"\x31\xc0\xc3" + b"\x90" * 13,
                "flags": CODE_FLAGS,
            }],
            symbols=[{
                "name": "_first",
                "value": 0,
                "section": 1,
                "type": 0x20,
                "storage": 2,
            }],
        )
        base = build_coff(
            sections=[{
                "name": ".text",
                "size": 16,
                "raw_data": b"\x31\xc0\xc3" + b"\x90" * 13,
                "flags": CODE_FLAGS,
            }],
            symbols=[{
                "name": "_sdk_first",
                "value": 0,
                "section": 1,
                "type": 0x20,
                "storage": 2,
            }],
        )
        self.target_path.write_bytes(target)
        self.base_path.write_bytes(base)
        exception = {
            "ledger": "semantic_matches",
            "unit": "source/example",
            "item": "_first",
            "identity": "alias-proof",
            "entry": {
                "unit": "source/example",
                "function": "_first",
                "base_function": "_sdk_first",
            },
        }

        unit = _capture_unit(
            self.root,
            self.unit_config,
            {"_first": 3},
            {},
            [exception],
        )

        self.assertEqual(unit["functions"]["_first"]["state"], "STRICT_EXACT")
        self.assertTrue(unit["functions"]["_first"]["accepted"])
        self.assertEqual(
            unit["functions"]["_first"]["exception_identity"], "alias-proof"
        )

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
