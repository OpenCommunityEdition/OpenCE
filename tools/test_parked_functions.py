import copy
import json
import tempfile
import unittest
from pathlib import Path

from tools.coff_compare import build_coff, load, make_section_raw, section_info
from tools.parked_functions import (
    ParkedFunctionsError,
    require_valid_parked_functions,
    validate_parked_functions,
)


class ParkedFunctionsTests(unittest.TestCase):
    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.target_path = self.root / "target.obj"
        self.base_path = self.root / "base.obj"
        self.report_path = self.root / "report.json"
        self.config_path = self.root / "objdiff.json"
        self.manifest_path = self.root / "parked.json"

        self.target_path.write_bytes(self._object(b"\x90" * 16))
        self.base_path.write_bytes(self._object(b"\xcc" * 16))
        self.report = {
            "units": [{
                "name": "unit",
                "functions": [{
                    "name": "_fn",
                    "size": 16,
                    "fuzzy_match_percent": 75.0,
                }],
            }],
        }
        self.config = {
            "units": [{
                "name": "unit",
                "target_path": "target.obj",
                "base_path": "base.obj",
            }],
        }
        self.entry = {
            "unit": "unit",
            "function": "_fn",
            "class": "register-allocation",
            "evidence": "Same control flow and relocations; only registers differ.",
            "measurements": self._measurements(),
        }
        self._write_inputs([self.entry])

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
    def _relocation_object(target_name, addend):
        symbols = [{
            "name": "_fn",
            "value": 0,
            "section": 1,
            "type": 0x20,
            "storage": 2,
        }]
        target_index = 0
        if target_name != "_fn":
            target_index = len(symbols)
            symbols.append({
                "name": target_name,
                "value": 0,
                "section": 0,
                "type": 0,
                "storage": 2,
            })
        raw, relocations = make_section_raw(
            16, [(4, target_index, 6, addend)]
        )
        return build_coff(
            sections=[{
                "name": ".text",
                "size": 16,
                "raw_data": raw,
                "reloc_data": relocations,
            }],
            symbols=symbols,
        )

    @staticmethod
    def _snapshot(info):
        return {
            "size": info["size"],
            "relocation_count": info["relocation_count"],
            "normalized_sha256": info["normalized_sha256"],
        }

    def _measurements(self):
        return {
            "target": self._snapshot(section_info(load(self.target_path), "_fn")),
            "base": self._snapshot(section_info(load(self.base_path), "_fn")),
            "objdiff_percent": 75.0,
        }

    def _write_inputs(self, entries):
        self.report_path.write_text(json.dumps(self.report), encoding="utf-8")
        self.config_path.write_text(json.dumps(self.config), encoding="utf-8")
        self.manifest_path.write_text(
            json.dumps({"version": 1, "entries": entries}), encoding="utf-8"
        )

    def _install_csplit_alias_entry(self, symbol_entries=None):
        self.target_path.write_bytes(
            self._relocation_object("_previous", 6)
        )
        self.base_path.write_bytes(self._relocation_object("_fn", 0))
        self.report["units"][0]["functions"][0]["fuzzy_match_percent"] = 100.0
        entry = {
            "unit": "unit",
            "function": "_fn",
            "class": "csplit-relocation-alias",
            "evidence": (
                "csplit spells the destination as _previous+6 while the "
                "compiler spells the same image address as _fn+0."
            ),
            "measurements": self._measurements(),
        }
        entry["measurements"]["objdiff_percent"] = 100.0
        symbols_path = self.root / "config" / "symbols.json"
        symbols_path.parent.mkdir(parents=True)
        symbols_path.write_text(
            json.dumps(symbol_entries if symbol_entries is not None else [
                {"name": "_previous", "file_offset": 0x1000},
                {"name": "_fn", "file_offset": 0x1006},
            ]),
            encoding="utf-8",
        )
        self._write_inputs([entry])
        return entry

    def _validate(self):
        return validate_parked_functions(
            self.root, self.report_path, self.config_path, self.manifest_path
        )

    def test_current_entry_is_active_and_machine_readable(self):
        result = self._validate()
        self.assertEqual(result["summary"], {"active": 1, "stale": 0, "invalid": 0})
        self.assertEqual(result["active_keys"], ["unit:_fn"])

    def test_unclassified_entry_is_active_without_inventing_a_recipe(self):
        entry = copy.deepcopy(self.entry)
        entry["class"] = "unclassified"
        entry["evidence"] = (
            "The retained source is semantically grounded, but the fail-closed "
            "classifier found no defensible blocker mechanism or recipe."
        )
        self._write_inputs([entry])
        result = self._validate()
        self.assertEqual(result["summary"], {"active": 1, "stale": 0, "invalid": 0})
        self.assertEqual(result["active"][0]["class"], "unclassified")

    def test_changed_object_hash_reopens_entry(self):
        self.base_path.write_bytes(self._object(b"\x00" * 16))
        result = self._validate()
        self.assertEqual(result["summary"]["stale"], 1)
        self.assertEqual(
            result["stale"][0]["differences"][0]["measurement"],
            "base.normalized_sha256",
        )

    def test_changed_relocation_measurement_reopens_entry(self):
        entry = copy.deepcopy(self.entry)
        entry["measurements"]["target"]["relocation_count"] = 1
        self._write_inputs([entry])
        result = self._validate()
        self.assertEqual(result["summary"]["stale"], 1)
        self.assertEqual(
            result["stale"][0]["differences"][0]["measurement"],
            "target.relocation_count",
        )

    def test_changed_objdiff_percent_reopens_entry(self):
        self.report["units"][0]["functions"][0]["fuzzy_match_percent"] = 80.0
        self._write_inputs([self.entry])
        result = self._validate()
        self.assertEqual(result["summary"]["stale"], 1)
        self.assertEqual(
            result["stale"][0]["differences"][0]["measurement"],
            "objdiff_percent",
        )

    def test_function_becoming_exact_invalidates_entry(self):
        self.base_path.write_bytes(self.target_path.read_bytes())
        result = self._validate()
        self.assertEqual(result["summary"]["invalid"], 1)
        self.assertEqual(result["invalid"][0]["reason"], "function is now semantically exact")

    def test_asm_implemented_entry_stays_active_when_exact(self):
        """asm bodies match by construction, so exactness must not unpark them."""
        self.base_path.write_bytes(self.target_path.read_bytes())
        self.report["units"][0]["functions"][0]["fuzzy_match_percent"] = 100.0
        measurements = self._measurements()
        measurements["base"] = dict(measurements["target"])
        measurements["objdiff_percent"] = 100.0
        entry = dict(self.entry)
        entry["class"] = "asm-implemented"
        entry["measurements"] = measurements
        self._write_inputs([entry])
        result = self._validate()
        self.assertEqual(result["summary"]["invalid"], 0)
        self.assertEqual(result["summary"]["stale"], 0)
        self.assertEqual(len(result["active"]), 1)

    def test_asm_implemented_entry_is_invalid_when_not_exact(self):
        """A drifted transcription is the failure mode worth reporting."""
        entry = dict(self.entry)
        entry["class"] = "asm-implemented"
        self._write_inputs([entry])
        result = self._validate()
        self.assertEqual(result["summary"]["invalid"], 1)
        self.assertEqual(
            result["invalid"][0]["reason"],
            "asm-implemented function no longer matches the target",
        )

    def test_csplit_relocation_alias_is_active_when_resolved_exact(self):
        self._install_csplit_alias_entry()
        result = self._validate()
        self.assertEqual(result["summary"], {"active": 1, "stale": 0, "invalid": 0})
        self.assertEqual(result["active_keys"], ["unit:_fn"])

    def test_csplit_relocation_alias_rejects_ordinary_exact_function(self):
        entry = self._install_csplit_alias_entry()
        self.base_path.write_bytes(self.target_path.read_bytes())
        entry["measurements"] = self._measurements()
        entry["measurements"]["objdiff_percent"] = 100.0
        self._write_inputs([entry])
        result = self._validate()
        self.assertEqual(result["summary"]["invalid"], 1)
        self.assertEqual(
            result["invalid"][0]["reason"],
            "csplit-relocation-alias function does not differ under ordinary comparison",
        )

    def test_csplit_relocation_alias_rejects_different_resolved_destination(self):
        self._install_csplit_alias_entry([
            {"name": "_previous", "file_offset": 0x2000},
            {"name": "_fn", "file_offset": 0x1006},
        ])
        result = self._validate()
        self.assertEqual(result["summary"]["invalid"], 1)
        self.assertEqual(
            result["invalid"][0]["reason"],
            (
                "csplit-relocation-alias function is not exact after resolving "
                "image destinations"
            ),
        )

    def test_csplit_relocation_alias_rejects_missing_image_address(self):
        self._install_csplit_alias_entry([
            {"name": "_fn", "file_offset": 0x1006},
        ])
        result = self._validate()
        self.assertEqual(result["summary"]["invalid"], 1)
        self.assertEqual(
            result["invalid"][0]["reason"],
            (
                "csplit-relocation-alias requires an unambiguous image address "
                "for every relocation destination"
            ),
        )

    def test_csplit_relocation_alias_rejects_ambiguous_image_address(self):
        self._install_csplit_alias_entry([
            {"name": "_previous", "file_offset": 0x1000},
            {"name": "_previous", "file_offset": 0x2000},
            {"name": "_fn", "file_offset": 0x1006},
        ])
        result = self._validate()
        self.assertEqual(result["summary"]["invalid"], 1)
        self.assertEqual(
            result["invalid"][0]["reason"],
            (
                "csplit-relocation-alias requires an unambiguous image address "
                "for every relocation destination"
            ),
        )

    def test_duplicate_key_is_invalid(self):
        self._write_inputs([self.entry, self.entry])
        result = self._validate()
        self.assertEqual(result["summary"]["invalid"], 1)

    def test_strict_validation_raises_for_stale_entry(self):
        self.report["units"][0]["functions"][0]["fuzzy_match_percent"] = 80.0
        self._write_inputs([self.entry])
        with self.assertRaises(ParkedFunctionsError):
            require_valid_parked_functions(
                self.root, self.report_path, self.config_path, self.manifest_path
            )


if __name__ == "__main__":
    unittest.main()
