import copy
import json
import struct
import tempfile
import unittest
from pathlib import Path

from tools.coff_compare import build_coff, load, section_info, section_info_resolved
from tools.regression_gate import (
    BSS_SENTINEL,
    GateError,
    XDK_D3DINLINE_RECIPE,
    _capture_unit,
    _exception_records,
    _function_code_evidence,
    _json_hash,
    _object_fingerprint,
    compare_manifests,
    create_manifest,
    load_baseline,
)
from tools.semantic_progress import _semantic_data_member_snapshot


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


DATA_FLAGS = 0xC0300040
RDATA_FLAGS = 0x40300040
UNIT = "source/example"
DEFAULT_DATA_SECTIONS = (
    (".data", DATA_FLAGS, b"ALPHADAT", (("_alpha", 0, 2),)),
    (".data", DATA_FLAGS, b"BETA", (("_beta", 0, 3),)),
    (".rdata", RDATA_FLAGS, b"gamma\0\0\0", (("_gamma", 0, 2),)),
)


class GroupedFixtureMixin:
    """Fixture helpers shared by the grouped semantic-data test classes."""

    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.target_path = self.root / "target.obj"
        self.base_path = self.root / "base.obj"
        self.unit_config = {
            "name": UNIT,
            "target_path": "target.obj",
            "base_path": "base.obj",
        }

    def tearDown(self):
        self.temporary_directory.cleanup()

    @staticmethod
    def _object(data_sections=DEFAULT_DATA_SECTIONS, extra_symbols=()):
        sections = [{
            "name": ".text",
            "size": 16,
            "raw_data": b"\x31\xc0\xc3" + b"\x90" * 13,
            "flags": CODE_FLAGS,
        }]
        symbols = [{
            "name": "_first", "value": 0, "section": 1, "type": 0x20, "storage": 2,
        }]
        for index, (name, flags, raw, owners) in enumerate(data_sections, start=2):
            sections.append(
                {"name": name, "size": len(raw), "raw_data": raw, "flags": flags}
            )
            for owner, value, storage in owners:
                symbols.append({
                    "name": owner, "value": value, "section": index, "type": 0,
                    "storage": storage,
                })
        symbols.extend(extra_symbols)
        return build_coff(sections=sections, symbols=symbols)

    def _write(self, target_sections=DEFAULT_DATA_SECTIONS, base_sections=None,
               target_extra=(), base_extra=()):
        self.target_path.write_bytes(self._object(target_sections, target_extra))
        self.base_path.write_bytes(
            self._object(base_sections or target_sections, base_extra)
        )

    def _snapshot(self, path, name):
        obj = load(path)
        owner = next(
            item for item in obj["symbols"] if item["name"] == name and item["section"] > 0
        )
        section = obj["sections"][owner["section"] - 1]
        return _semantic_data_member_snapshot(owner, section, section_info(obj, name))

    def _member(self, symbol, base_symbol=None):
        member = {
            "symbol": symbol,
            "measurements": {
                "target": self._snapshot(self.target_path, symbol),
                "base": self._snapshot(self.base_path, base_symbol or symbol),
            },
        }
        if base_symbol:
            member["base_symbol"] = base_symbol
        return member

    @staticmethod
    def _group(members, label="static-sections", **extra):
        entry = {"unit": UNIT, "group": label, "members": members, "reason": "fixture"}
        entry.update(extra)
        return entry

    def _capture(self, entries, addresses=None):
        records = _exception_records([UNIT], [], entries)
        unit = _capture_unit(
            self.root, self.unit_config, {"_first": 3}, addresses or {}, records
        )
        return {
            "schema_version": 1,
            "commit": "a" * 40,
            "build_environment": {"compiler_sha256": "compiler"},
            "semantic_exceptions": records,
            "units": {UNIT: unit},
        }

    def _kinds(self, result):
        return {item["kind"] for item in result["failures"]}

    def _failure(self, result, kind):
        return [item for item in result["failures"] if item["kind"] == kind]


class GroupedSemanticDataTests(GroupedFixtureMixin, unittest.TestCase):
    """Grouped semantic-data entries: capture, association and change detection."""

    # --- capture and association -------------------------------------------------

    def test_group_is_one_record_with_whole_entry_identity(self):
        self._write()
        entry = self._group([self._member("_alpha"), self._member("_beta")])

        manifest = self._capture([entry])

        records = manifest["semantic_exceptions"]
        self.assertEqual(len(records), 1)
        self.assertEqual(records[0]["item"], "group:static-sections")
        self.assertEqual(records[0]["kind"], "semantic_data_group")
        self.assertEqual(records[0]["identity"], _json_hash(entry))
        self.assertEqual(records[0]["entry"], entry)
        group = manifest["units"][UNIT]["semantic_data_groups"]["group:static-sections"]
        self.assertEqual(group["identity"], _json_hash(entry))
        self.assertEqual([m["symbol"] for m in group["members"]], ["_alpha", "_beta"])
        sections = manifest["units"][UNIT]["sections"]
        for member in group["members"]:
            self.assertTrue(sections[member["target_section"]]["accepted"])
            self.assertEqual(
                sections[member["target_section"]]["exception_identity"],
                _json_hash(entry),
            )
        # A section outside the group gets no exception identity.
        self.assertIsNone(sections[".rdata|owners=_gamma"]["exception_identity"])

    def test_unnamed_group_uses_the_production_default_label(self):
        self._write()
        entry = self._group([self._member("_alpha")])
        del entry["group"]

        manifest = self._capture([entry])

        self.assertEqual(
            manifest["semantic_exceptions"][0]["item"], "group:data-section-group"
        )

    def test_single_entry_record_keeps_its_historical_shape(self):
        self._write()
        info = section_info_resolved(load(self.target_path), "_alpha", {"_alpha": 0x1000})
        entry = {
            "unit": UNIT,
            "symbol": "_alpha",
            "measurements": {
                key: info[key] for key in ("size", "relocation_count", "normalized_sha256")
            },
            "reason": "fixture",
        }

        manifest = self._capture([entry], {"_alpha": 0x1000})

        record = manifest["semantic_exceptions"][0]
        self.assertEqual(set(record), {"ledger", "unit", "item", "identity", "entry"})
        self.assertEqual(record["item"], "_alpha")
        self.assertEqual(manifest["units"][UNIT]["semantic_data_groups"], {})

    def test_aliased_member_pairs_its_target_and_rebuilt_sections(self):
        target_sections = DEFAULT_DATA_SECTIONS + (
            (".rdata", RDATA_FLAGS, b"table\0\0\0", (("_rdata_00001000", 0, 2),)),
        )
        base_sections = DEFAULT_DATA_SECTIONS + (
            (".rdata", RDATA_FLAGS, b"table\0\0\0", (("$T100", 0, 3),)),
        )
        self._write(target_sections, base_sections)
        entry = self._group([self._member("_rdata_00001000", base_symbol="$T100")])

        manifest = self._capture([entry])

        sections = manifest["units"][UNIT]["sections"]
        target_row = sections[".rdata|owners=_rdata_00001000"]
        base_row = sections[".rdata|owners=$T100"]
        self.assertEqual(target_row["state"], "SEMANTIC_EXACT")
        self.assertEqual(target_row["paired_base_identity"], ".rdata|owners=$T100")
        self.assertEqual(base_row["state"], "SEMANTIC_EXACT")
        self.assertEqual(base_row["paired_target_identity"], ".rdata|owners=_rdata_00001000")
        self.assertEqual(base_row["exception_identity"], _json_hash(entry))

    # --- fail-closed capture -----------------------------------------------------

    def test_changed_member_measurement_fails_capture(self):
        self._write()
        member = self._member("_alpha")
        member["measurements"]["target"]["size"] += 4

        with self.assertRaisesRegex(GateError, "snapshot changed"):
            self._capture([self._group([member])])

    def test_member_bytes_changed_in_rebuilt_object_fails_capture(self):
        self._write()
        entry = self._group([self._member("_alpha")])
        changed = list(DEFAULT_DATA_SECTIONS)
        changed[0] = (".data", DATA_FLAGS, b"ALPHAXXX", (("_alpha", 0, 2),))
        self.base_path.write_bytes(self._object(changed))

        with self.assertRaisesRegex(GateError, "no longer exact"):
            self._capture([entry])

    def test_duplicate_member_fails_capture(self):
        self._write()
        with self.assertRaisesRegex(GateError, "listed more than once"):
            self._capture([self._group([self._member("_alpha"), self._member("_alpha")])])

    def test_two_members_owning_one_section_fail_capture(self):
        sections = (
            (".data", DATA_FLAGS, b"ALPHADAT", (("_alpha", 0, 2), ("_alpha_head", 0, 2))),
        ) + DEFAULT_DATA_SECTIONS[1:]
        self._write(sections)
        with self.assertRaisesRegex(GateError, "already claimed"):
            self._capture([
                self._group([self._member("_alpha"), self._member("_alpha_head")])
            ])

    def test_member_also_claimed_by_single_entry_fails_capture(self):
        self._write()
        info = section_info_resolved(load(self.target_path), "_alpha", {"_alpha": 0x1000})
        single = {
            "unit": UNIT,
            "symbol": "_alpha",
            "measurements": {
                key: info[key] for key in ("size", "relocation_count", "normalized_sha256")
            },
        }
        with self.assertRaisesRegex(GateError, "already claimed"):
            self._capture(
                [single, self._group([self._member("_alpha")])], {"_alpha": 0x1000}
            )

    def test_ambiguous_member_owner_fails_capture(self):
        self._write()
        entry = self._group([self._member("_alpha")])
        duplicate = [{"name": "_alpha", "value": 4, "section": 3, "type": 0, "storage": 2}]
        self.target_path.write_bytes(self._object(extra_symbols=duplicate))

        with self.assertRaisesRegex(GateError, "expected one target owner"):
            self._capture([entry])

    def test_missing_member_owner_fails_capture(self):
        self._write()
        entry = self._group([self._member("_alpha")])
        entry["members"][0]["symbol"] = "_absent"

        with self.assertRaisesRegex(GateError, "expected one target owner"):
            self._capture([entry])

    def test_ambiguous_alias_pairing_fails_capture(self):
        target_sections = DEFAULT_DATA_SECTIONS + (
            (".rdata", RDATA_FLAGS, b"table\0\0\0", (("_rdata_00001000", 0, 2),)),
            (".rdata", RDATA_FLAGS, b"other\0\0\0", (("$T100", 0, 3),)),
        )
        base_sections = DEFAULT_DATA_SECTIONS + (
            (".rdata", RDATA_FLAGS, b"table\0\0\0", (("$T100", 0, 3),)),
        )
        self._write(target_sections, base_sections)
        with self.assertRaisesRegex(GateError, "also exists on the other side"):
            self._capture([self._group([self._member("_rdata_00001000", base_symbol="$T100")])])

    # --- schema: production keys only ---------------------------------------------

    def test_unknown_entry_keys_are_rejected_not_ignored(self):
        self._write()
        # 'surplus' is a model-entry key; on a legacy group it is unsupported.
        for key, value in (("surplus", []), ("note", "x")):
            entry = self._group([self._member("_alpha")], **{key: value})
            with self.subTest(key=key):
                with self.assertRaisesRegex(GateError, "unsupported key"):
                    _exception_records([UNIT], [], [entry])

    def test_unknown_member_key_is_rejected(self):
        self._write()
        member = self._member("_alpha")
        member["padding"] = 0
        with self.assertRaisesRegex(GateError, "unsupported key"):
            _exception_records([UNIT], [], [self._group([member])])

    def test_malformed_entries_are_rejected(self):
        self._write()
        good = self._member("_alpha")
        cases = {
            "mixed": dict(self._group([good]), symbol="_alpha"),
            "empty-members": self._group([]),
            "members-not-list": self._group({"symbol": "_alpha"}),
            "member-not-object": self._group(["_alpha"]),
            "member-missing-measurements": self._group([{"symbol": "_alpha"}]),
            "measurements-shape": self._group([{"symbol": "_alpha", "measurements": {"target": {}}}]),
            "empty-label": self._group([good], label=""),
            "non-bool-flag": self._group([good], credit_raw_size="yes"),
            "base-source-without-source": self._group(
                [dict(good, base_source_function="_main")]
            ),
            "single-without-measurements": {"unit": UNIT, "symbol": "_alpha"},
            "not-an-object": ["unit", UNIT],
        }
        for name, entry in cases.items():
            with self.subTest(case=name):
                with self.assertRaises(GateError):
                    _exception_records([UNIT], [], [entry])

    def test_malformed_entry_for_an_unselected_unit_still_fails(self):
        entry = {"unit": "source/other", "group": "g", "members": []}
        with self.assertRaises(GateError):
            _exception_records([UNIT], [], [entry])

    def test_duplicate_group_label_in_one_unit_is_ambiguous(self):
        self._write()
        first = self._group([self._member("_alpha")])
        second = self._group([self._member("_beta")])
        with self.assertRaisesRegex(GateError, "more than once"):
            _exception_records([UNIT], [], [first, second])

    # --- change detection in both directions ---------------------------------------

    def test_identical_recapture_passes(self):
        self._write()
        entries = [self._group([self._member("_alpha"), self._member("_beta")])]

        result = compare_manifests(self._capture(entries), self._capture(entries))

        self.assertTrue(result["ok"], result["failures"])

    def test_member_added_and_removed_are_named(self):
        self._write()
        small = [self._group([self._member("_alpha")])]
        large = [self._group([self._member("_alpha"), self._member("_beta")])]
        for before, after, key in ((small, large, "added_members"),
                                   (large, small, "removed_members")):
            with self.subTest(direction=key):
                result = compare_manifests(self._capture(before), self._capture(after))
                self.assertFalse(result["ok"])
                changed = self._failure(result, "SEMANTIC_EXCEPTION_CHANGED")
                self.assertEqual(len(changed), 1)
                self.assertEqual(changed[0]["changes"][key], ["_beta"])
                self.assertIn("SEMANTIC_DATA_GROUP_CHANGED", self._kinds(result))
                self.assertIn("UNKNOWN", self._kinds(result))

    def test_changed_measurements_are_named_in_both_directions(self):
        self._write()
        before_entries = [self._group([self._member("_alpha"), self._member("_beta")])]
        baseline = self._capture(before_entries)
        changed = list(DEFAULT_DATA_SECTIONS)
        changed[0] = (".data", DATA_FLAGS, b"ALPHAXXX", (("_alpha", 0, 2),))
        self._write(tuple(changed))
        after_entries = [self._group([self._member("_alpha"), self._member("_beta")])]
        current = self._capture(after_entries)
        for before, after in ((baseline, current), (current, baseline)):
            result = compare_manifests(before, after)
            self.assertFalse(result["ok"])
            changed_records = self._failure(result, "SEMANTIC_EXCEPTION_CHANGED")
            self.assertEqual(changed_records[0]["changes"]["changed_members"], ["_alpha"])
            self.assertIn("DATA_CHANGED", self._kinds(result))

    def test_group_label_change_is_vanished_plus_appeared(self):
        self._write()
        old = self._capture([self._group([self._member("_alpha")], label="old")])
        new = self._capture([self._group([self._member("_alpha")], label="new")])
        for before, after in ((old, new), (new, old)):
            result = compare_manifests(before, after)
            messages = [item["message"] for item in self._failure(result, "SEMANTIC_EXCEPTION_CHANGED")]
            self.assertEqual(len(messages), 2)
            self.assertTrue(any("vanished" in message for message in messages))
            self.assertTrue(any("appeared" in message for message in messages))

    def test_group_flag_change_is_named(self):
        self._write()
        plain = self._capture([self._group([self._member("_alpha")])])
        raw = self._capture([self._group([self._member("_alpha")], credit_raw_size=True)])
        for before, after in ((plain, raw), (raw, plain)):
            result = compare_manifests(before, after)
            changed = self._failure(result, "SEMANTIC_EXCEPTION_CHANGED")
            self.assertEqual(changed[0]["changes"]["changed_keys"], ["credit_raw_size"])

    def test_member_order_change_is_detected(self):
        self._write()
        forward = self._capture([self._group([self._member("_alpha"), self._member("_beta")])])
        reverse = self._capture([self._group([self._member("_beta"), self._member("_alpha")])])
        result = compare_manifests(forward, reverse)
        self.assertFalse(result["ok"])
        changed = self._failure(result, "SEMANTIC_EXCEPTION_CHANGED")
        self.assertTrue(changed[0]["changes"]["member_order_changed"])

    def test_group_vanishing_and_appearing_is_detected(self):
        self._write()
        with_group = self._capture([self._group([self._member("_alpha")])])
        without_group = self._capture([])
        for before, after, word in ((with_group, without_group, "vanished"),
                                    (without_group, with_group, "appeared")):
            result = compare_manifests(before, after)
            self.assertFalse(result["ok"])
            messages = [item["message"] for item in self._failure(result, "SEMANTIC_EXCEPTION_CHANGED")]
            self.assertEqual(len(messages), 1)
            self.assertIn(word, messages[0])
            self.assertIn("SEMANTIC_DATA_GROUP_CHANGED", self._kinds(result))

    def test_rebuilt_section_reassociation_is_detected(self):
        self._write()
        entries = [self._group([self._member("_alpha")])]
        baseline = self._capture(entries)
        extra_owner = [{"name": "_alpha_alias", "value": 0, "section": 2, "type": 0, "storage": 3}]
        self.base_path.write_bytes(self._object(extra_symbols=extra_owner))
        current = self._capture(entries)
        result = compare_manifests(baseline, current)
        self.assertFalse(result["ok"])
        group_changes = self._failure(result, "SEMANTIC_DATA_GROUP_CHANGED")
        self.assertEqual(group_changes[0]["changes"]["changed_members"], ["_alpha"])
        self.assertIn("DATA_CHANGED", self._kinds(result))

    def test_undeclared_rebuilt_only_section_changes_are_detected(self):
        # The production schema declares no surplus: a rebuilt-only (select-any
        # literal) section in a grouped unit stays frozen as BASE_ONLY evidence.
        self._write()
        entries = [self._group([self._member("_alpha")])]
        plain = self._capture(entries)
        surplus = DEFAULT_DATA_SECTIONS + (
            (".rdata", RDATA_FLAGS, b"lit\0", (("??_C@_03LIT@lit?$AA@", 0, 2),)),
        )
        self.base_path.write_bytes(self._object(surplus))
        with_surplus = self._capture(entries)
        row = with_surplus["units"][UNIT]["sections"][".rdata|owners=??_C@_03LIT@lit?$AA@"]
        self.assertEqual(row["state"], "BASE_ONLY")
        self.assertFalse(row["accepted"])
        for before, after in ((plain, with_surplus), (with_surplus, plain)):
            result = compare_manifests(before, after)
            self.assertFalse(result["ok"])
            self.assertIn("DATA_CHANGED", self._kinds(result))
            self.assertIn("SYMBOL_SET_CHANGED", self._kinds(result))

    def test_malformed_function_ledger_entry_is_rejected(self):
        with self.assertRaisesRegex(GateError, "semantic match function"):
            _exception_records([UNIT], [{"unit": UNIT, "reason": "no function"}], [])

    def test_duplicate_exception_records_in_a_manifest_are_ambiguous(self):
        self._write()
        manifest = self._capture([self._group([self._member("_alpha")])])
        current = copy.deepcopy(manifest)
        current["semantic_exceptions"].append(copy.deepcopy(current["semantic_exceptions"][0]))
        result = compare_manifests(manifest, current)
        self.assertFalse(result["ok"])
        self.assertTrue(any(
            "ambiguous semantic-exception record" in item["message"]
            for item in result["failures"]
        ))


class ModelSchemaGateTests(GroupedFixtureMixin, unittest.TestCase):
    """PROPOSAL: extent-model entries (members by identity, declared surplus)."""

    LITERAL = "??_C@_03LIT@lit?$AA@"

    def _surplus_objects(self):
        base_sections = DEFAULT_DATA_SECTIONS + (
            (".rdata", RDATA_FLAGS | 0x1000, b"lit" + bytes(1), ((self.LITERAL, 0, 2),)),
        )
        self._write(DEFAULT_DATA_SECTIONS, base_sections)

    def _model(self, members, surplus, provider="source/provider", **extra):
        entry = self._group(members, extent_model="objdiff-3.3.1-combined", **extra)
        entry["surplus"] = [
            {"symbol": name, "provider": provider,
             "measurements": {"base": self._snapshot(self.base_path, name)}}
            for name in surplus
        ]
        return entry

    def test_declared_surplus_is_frozen_with_zero_credit(self):
        self._surplus_objects()
        manifest = self._capture([self._model([self._member("_alpha")], [self.LITERAL])])
        group = manifest["units"][UNIT]["semantic_data_groups"]["group:static-sections"]
        self.assertEqual(group["extent_model"], "objdiff-3.3.1-combined")
        self.assertEqual([item["provider"] for item in group["surplus"]], ["source/provider"])
        row = manifest["units"][UNIT]["sections"][group["surplus"][0]["base_section"]]
        self.assertEqual(row["state"], "DECLARED_SURPLUS")
        self.assertFalse(row["accepted"])

    def test_surplus_changes_are_named_in_both_directions(self):
        self._surplus_objects()
        member = [self._member("_alpha")]
        without = self._capture([self._model(member, [])])
        with_surplus = self._capture([self._model(member, [self.LITERAL])])
        other_provider = self._capture(
            [self._model(member, [self.LITERAL], provider="source/other")])
        for before, after, key in ((without, with_surplus, "added_surplus"),
                                   (with_surplus, without, "removed_surplus"),
                                   (with_surplus, other_provider, "changed_surplus"),
                                   (other_provider, with_surplus, "changed_surplus")):
            with self.subTest(key=key):
                result = compare_manifests(before, after)
                self.assertFalse(result["ok"])
                changed = self._failure(result, "SEMANTIC_EXCEPTION_CHANGED")
                self.assertEqual(changed[0]["changes"][key], [self.LITERAL])

    def test_stale_surplus_snapshot_fails(self):
        self._surplus_objects()
        entry = self._model([self._member("_alpha")], [self.LITERAL])
        entry["surplus"][0]["measurements"]["base"]["size"] += 1
        with self.assertRaisesRegex(GateError, "snapshot changed"):
            self._capture([entry])

    def test_surplus_defined_by_target_fails(self):
        self._surplus_objects()
        with self.assertRaisesRegex(GateError, "defined by the target"):
            self._capture([self._model([self._member("_alpha")], ["_beta"])])

    def test_surplus_section_claimed_by_a_member_fails(self):
        self._surplus_objects()
        target_sections = DEFAULT_DATA_SECTIONS + (
            (".rdata", RDATA_FLAGS | 0x1000, b"lit" + bytes(1), ((self.LITERAL, 0, 2),)),
        )
        self._write(target_sections, target_sections)
        with self.assertRaises(GateError):
            self._capture([self._model([self._member(self.LITERAL)], [self.LITERAL])])

    def test_model_schema_is_exact(self):
        self._surplus_objects()
        good = self._model([self._member("_alpha")], [self.LITERAL])
        aliased = self._model([self._member("_alpha")], [])
        aliased["members"][0]["base_symbol"] = "_alpha"
        no_reason = dict(good)
        del no_reason["reason"]
        bad_surplus = self._model([self._member("_alpha")], [self.LITERAL])
        del bad_surplus["surplus"][0]["provider"]
        legacy_surplus = self._group([self._member("_alpha")], surplus=[])
        for name, entry in (("aliased-member", aliased), ("no-reason", no_reason),
                            ("surplus-without-provider", bad_surplus),
                            ("surplus-on-legacy", legacy_surplus)):
            with self.subTest(case=name):
                with self.assertRaises(GateError):
                    _exception_records([UNIT], [], [entry])


class LinkAbsoluteZeroRelocationTests(unittest.TestCase):
    """The comparator omits __except_list relocations; the gate must still freeze them."""

    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.path = Path(self.temporary_directory.name) / "seh.obj"

    def tearDown(self):
        self.temporary_directory.cleanup()

    def _write(self, except_address=2, with_except=True):
        raw = bytearray(16)  # every relocation field holds addend 0
        relocations = b""
        count = 0
        if with_except:
            relocations += struct.pack("<LLH", except_address, 1, 6)
            count += 1
        relocations += struct.pack("<LLH", 8, 2, 0x14)
        count += 1
        self.path.write_bytes(build_coff(
            sections=[{
                "name": ".text", "size": 16, "raw_data": bytes(raw),
                "reloc_count": count, "reloc_data": relocations, "flags": CODE_FLAGS,
            }],
            symbols=[
                {"name": "_seh_function", "value": 0, "section": 1, "type": 0x20, "storage": 2},
                {"name": "__except_list", "value": 0, "section": 0, "type": 0, "storage": 2},
                {"name": "_callee", "value": 0, "section": 0, "type": 0x20, "storage": 2},
            ],
        ))

    def _relocations(self):
        fingerprint = _object_fingerprint(self.path, {"_seh_function": 16}, {})
        return fingerprint["functions"]["_seh_function"]["relocations"]

    def test_link_absolute_zero_relocation_is_frozen(self):
        self._write()
        relocations = self._relocations()
        self.assertEqual(len(relocations), 2)
        self.assertEqual(relocations[0]["resolved_destination"], ["link-absolute-zero", "__except_list"])
        self.assertEqual(relocations[0]["address"], 2)
        self.assertEqual(relocations[1]["resolved_destination"], ["symbol", "_callee", 0])

    def test_link_absolute_zero_relocation_changes_are_visible(self):
        self._write()
        original = self._relocations()
        self._write(except_address=12)
        moved = self._relocations()
        self._write(with_except=False)
        removed = self._relocations()
        self.assertNotEqual(original, moved)
        self.assertNotEqual(original, removed)


PROJECT_ROOT = Path(__file__).resolve().parents[1]
PRODUCTION_DATA_MANIFEST = PROJECT_ROOT / "config" / "semantic_data_matches.json"


class ProductionSemanticDataManifestTests(unittest.TestCase):
    """The gate must read, capture and re-verify every entry in the production ledgers."""

    def test_every_production_data_entry_yields_one_identity_record(self):
        entries = json.loads(PRODUCTION_DATA_MANIFEST.read_text(encoding="utf-8"))
        units = sorted({entry["unit"] for entry in entries})

        records = _exception_records(units, [], entries)

        self.assertEqual(len(records), len(entries))
        self.assertEqual(
            sorted(record["identity"] for record in records),
            sorted(_json_hash(entry) for entry in entries),
        )
        grouped = [entry for entry in entries if "members" in entry]
        self.assertEqual(
            sum(record.get("kind") == "semantic_data_group" for record in records),
            len(grouped),
        )

    def test_production_data_units_capture_and_recompare_cleanly(self):
        required = [
            PROJECT_ROOT / "objdiff.json",
            PROJECT_ROOT / "build" / "report.json",
        ]
        missing = [str(path) for path in required if not path.is_file()]
        if missing:
            self.skipTest("SKIPPED: production build inputs are missing: " + ", ".join(missing))
        entries = json.loads(PRODUCTION_DATA_MANIFEST.read_text(encoding="utf-8"))
        config = json.loads((PROJECT_ROOT / "objdiff.json").read_text(encoding="utf-8"))
        units = sorted({entry["unit"] for entry in entries})
        unit_configs = {item["name"]: item for item in config["units"]}
        absent = [
            unit for unit in units
            if not (PROJECT_ROOT / unit_configs[unit]["base_path"]).is_file()
            or not (PROJECT_ROOT / unit_configs[unit]["target_path"]).is_file()
        ]
        if absent:
            self.skipTest("SKIPPED: production objects are not built: " + ", ".join(absent[:5]))
        report = json.loads((PROJECT_ROOT / "build" / "report.json").read_text(encoding="utf-8"))
        semantic_matches = json.loads(
            (PROJECT_ROOT / "config" / "semantic_matches.json").read_text(encoding="utf-8")
        )
        symbols = json.loads((PROJECT_ROOT / "config" / "symbols.json").read_text(encoding="utf-8"))

        first = create_manifest(
            PROJECT_ROOT, "test", units, config, report, semantic_matches, entries,
            symbols, {"test": True},
        )
        second = create_manifest(
            PROJECT_ROOT, "test", units, config, report, semantic_matches, entries,
            symbols, {"test": True},
        )

        result = compare_manifests(first, second)
        self.assertTrue(result["ok"], result["failures"][:3])
        for entry in entries:
            unit = first["units"][entry["unit"]]
            if "members" in entry:
                group = unit["semantic_data_groups"]["group:" + entry["group"]]
                self.assertEqual(len(group["members"]), len(entry["members"]))
                for member in group["members"]:
                    for identity in {member["target_section"], member["base_section"]}:
                        self.assertTrue(unit["sections"][identity]["accepted"], identity)
            else:
                self.assertEqual(unit["semantic_data_groups"], {})


if __name__ == "__main__":
    unittest.main()
