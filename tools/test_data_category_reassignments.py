"""Tests for apply_data_category_reassignments (accounting only, no credit)."""

import copy
import json
import random
import struct
import tempfile
import unittest
from pathlib import Path

from tools import semantic_progress
from tools.coff_compare import build_coff
from tools.semantic_progress import (
    OBJDIFF_331_COMBINED_EXTENT,
    SemanticProgressError,
    apply_data_category_reassignments,
)

BSS_ALIGN1 = 0xC0100080
BSS_ALIGN4 = 0xC0300080
BSS_ALIGN32 = 0xC0600080
RDATA_DEFAULT = 0x40000040
RDATA_ALIGN4 = 0x40300040
LONG_NAME = b".rdata$debug"


def _pool_object(extra_symbols=(), long_name="/4"):
    """Five-record pool: .bss 5/1/48, .rdata 28 and a long-named .rdata$debug 60."""
    sections = [
        {"name": ".bss", "size": 5, "flags": BSS_ALIGN4},
        {"name": ".bss", "size": 1, "flags": BSS_ALIGN1},
        {"name": ".bss", "size": 48, "flags": BSS_ALIGN32},
        {"name": ".rdata", "size": 28, "flags": RDATA_DEFAULT},
        {"name": long_name, "size": 60, "flags": RDATA_ALIGN4},
    ]
    symbols = [
        {"name": ".bss", "section": 1, "storage": 3},
        {"name": "_halo_a", "section": 1},
        {"name": "_vend_b", "section": 2},
        {"name": "_halo_c", "section": 3},
        {"name": "_rd_a", "section": 4},
        {"name": "_rd_b", "section": 5},
        {"name": "_undef", "section": 0},
    ]
    symbols.extend(extra_symbols)
    strtab = struct.pack("<L", 4 + len(LONG_NAME) + 1) + LONG_NAME + b"\0"
    return build_coff(sections=sections, symbols=symbols, strtab=strtab)


def _report():
    return {
        "measures": {"total_data": 1150, "matched_data": 510,
                     "complete_data": 100},
        "categories": [
            {"id": "halo", "name": "halo",
             "measures": {"total_data": 1000, "matched_data": 500,
                          "matched_data_percent": 50.0,
                          "complete_data": 100,
                          "complete_data_percent": 10.0}},
            {"id": "vend", "name": "vend",
             "measures": {"total_data": 150, "matched_data": 10,
                          "matched_data_percent": 10 / 1.5}},
        ],
        "units": [{
            "name": "pool",
            "measures": {"total_data": "160"},
            "sections": [{"name": ".bss", "size": "64"},
                         {"name": ".rdata", "size": "96"}],
            "metadata": {"progress_categories": ["halo"]},
        }],
    }


def _manifest():
    return {
        "categories": [{"id": "linker", "name": "linker"}],
        "entries": [{
            "unit": "pool",
            "from_category": "halo",
            "extent_model": OBJDIFF_331_COMBINED_EXTENT,
            "unit_total_data": 160,
            "moved": {"linker": 96, "vend": 4},
            "reason": "fixture",
            "records": [
                {"symbol": "_vend_b", "section": ".bss", "size": 1,
                 "to_category": "vend", "evidence": "fixture vendor COMMON"},
                {"symbol": "_rd_a", "section": ".rdata", "size": 28,
                 "to_category": "linker", "evidence": "fixture debug dir"},
                {"symbol": "_rd_b", "section": ".rdata$debug", "size": 60,
                 "to_category": "linker", "evidence": "fixture CodeView"},
            ],
        }],
    }


class DataCategoryReassignmentTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / "target.obj").write_bytes(_pool_object())
        self.objdiff = {"units": [{
            "name": "pool", "target_path": "target.obj",
            "metadata": {"progress_categories": ["halo"]}}]}
        self.manifest = _manifest()

    def tearDown(self):
        self.tmp.cleanup()

    def _run(self, report=None):
        report = _report() if report is None else report
        (self.root / "objdiff.json").write_text(
            json.dumps(self.objdiff), encoding="utf-8")
        (self.root / "manifest.json").write_text(
            json.dumps(self.manifest), encoding="utf-8")
        notes = apply_data_category_reassignments(
            report, self.root, self.root / "manifest.json",
            self.root / "objdiff.json")
        return report, notes

    def _fails(self, message, report=None):
        report = _report() if report is None else report
        before = copy.deepcopy(report)
        with self.assertRaisesRegex(SemanticProgressError, message):
            self._run(report)
        self.assertEqual(report, before, "a failure must not change measures")

    def test_moves_only_the_listed_shares(self):
        report, notes = self._run()
        before = _report()
        categories = {c["id"]: c["measures"] for c in report["categories"]}
        self.assertEqual(categories["halo"]["total_data"], 900)
        self.assertEqual(categories["vend"]["total_data"], 154)
        self.assertEqual(categories["linker"]["total_data"], 96)
        self.assertEqual(categories["linker"]["matched_data"], 0)
        self.assertEqual(categories["linker"]["total_units"], 0)
        self.assertAlmostEqual(categories["halo"]["matched_data_percent"],
                               100.0 * 500 / 900)
        self.assertAlmostEqual(categories["halo"]["complete_data_percent"],
                               100.0 * 100 / 900)
        # matched and complete bytes, All and the unit row never move
        self.assertEqual(categories["halo"]["matched_data"], 500)
        self.assertEqual(categories["vend"]["matched_data"], 10)
        self.assertEqual(report["measures"], before["measures"])
        self.assertEqual(report["units"], before["units"])
        # the categories still partition the same total
        self.assertEqual(sum(c["measures"]["total_data"]
                             for c in report["categories"]), 1150)
        self.assertEqual(notes, [
            "pool: halo -100 -> linker +96, vend +4 "
            "(3 records: 89 raw + 11 objdiff padding bytes)"])

    def test_missing_manifest_is_a_no_op(self):
        report = _report()
        notes = apply_data_category_reassignments(
            report, self.root, self.root / "absent.json",
            self.root / "objdiff.json")
        self.assertEqual(notes, [])
        self.assertEqual(report, _report())

    def test_shares_sum_to_the_pinned_extent_model(self):
        rng = random.Random(20260928)
        for _ in range(300):
            sections = []
            for index in range(1, rng.randint(1, 12)):
                name = rng.choice([".bss", ".data", ".data$r", ".data$z"])
                sections.append({
                    "index": index, "name": name,
                    "size": rng.randint(0, 300),
                    "flags": 0xC0000040 | (rng.randint(0, 7) << 20)})
            if not sections:
                continue
            shares = semantic_progress._objdiff_report_shares(sections)
            self.assertEqual(
                sum(shares.values()),
                semantic_progress._objdiff_report_extent(sections))
            self.assertEqual(set(shares),
                             {s["index"] for s in sections})

    def test_long_section_name_is_resolved(self):
        named = semantic_progress._objdiff_named_sections(
            semantic_progress.load(self.root / "target.obj"), "pool")
        self.assertEqual([s["name"] for s in named["sections"]],
                         [".bss", ".bss", ".bss", ".rdata", ".rdata$debug"])

    def test_dangling_long_section_name_fails(self):
        (self.root / "target.obj").write_bytes(_pool_object(long_name="/99"))
        self._fails("unresolvable long section name")

    def test_malformed_long_section_name_fails(self):
        (self.root / "target.obj").write_bytes(_pool_object(long_name="/x1"))
        self._fails("malformed long section name")

    def test_wrong_size_fails(self):
        self.manifest["entries"][0]["records"][0]["size"] = 2
        self._fails("does not match the target")

    def test_wrong_section_name_fails(self):
        self.manifest["entries"][0]["records"][2]["section"] = ".rdata"
        self._fails("does not match the target")

    def test_absent_symbol_fails(self):
        self.manifest["entries"][0]["records"][0]["symbol"] = "_nobody"
        self._fails("not one defined external symbol")

    def test_undefined_symbol_is_not_a_record(self):
        self.manifest["entries"][0]["records"][0]["symbol"] = "_undef"
        self._fails("not one defined external symbol")

    def test_duplicate_record_fails(self):
        records = self.manifest["entries"][0]["records"]
        records.append(copy.deepcopy(records[0]))
        self._fails("listed twice")

    def test_shared_section_fails(self):
        (self.root / "target.obj").write_bytes(_pool_object(
            extra_symbols=[{"name": "_alias", "section": 2, "value": 0}]))
        self._fails("does not own its whole section")

    def test_offset_symbol_fails(self):
        (self.root / "target.obj").write_bytes(_pool_object(
            extra_symbols=[{"name": "_inner", "section": 3, "value": 8}]))
        self.manifest["entries"][0]["records"][0] = {
            "symbol": "_inner", "section": ".bss", "size": 48,
            "to_category": "vend", "evidence": "fixture"}
        self._fails("does not own its whole section")

    def test_moved_pin_fails(self):
        self.manifest["entries"][0]["moved"] = {"linker": 96, "vend": 1}
        self._fails("totals changed")

    def test_raw_size_pin_is_not_the_moved_share(self):
        self.manifest["entries"][0]["moved"] = {"linker": 88, "vend": 1}
        self._fails("totals changed")

    def test_unit_total_pin_fails(self):
        self.manifest["entries"][0]["unit_total_data"] = 161
        self._fails("unit total changed")

    def test_report_not_bound_to_target_fails(self):
        report = _report()
        report["units"][0]["sections"][0]["size"] = "60"
        report["units"][0]["measures"]["total_data"] = "156"
        self._fails("modelled extent", report)

    def test_unit_with_matched_data_fails(self):
        report = _report()
        report["units"][0]["measures"]["matched_data"] = "4"
        self._fails("no matched or complete data", report)

    def test_complete_unit_fails(self):
        self.objdiff["units"][0]["metadata"]["complete"] = True
        self._fails("no matched or complete data")

    def test_unit_in_two_categories_fails(self):
        report = _report()
        report["units"][0]["metadata"]["progress_categories"] = [
            "halo", "vend"]
        self._fails("not solely", report)

    def test_unknown_target_category_fails(self):
        self.manifest["entries"][0]["records"][0]["to_category"] = "nowhere"
        self._fails("invalid data category reassignment target")

    def test_target_equal_to_source_fails(self):
        self.manifest["entries"][0]["records"][0]["to_category"] = "halo"
        self._fails("invalid data category reassignment target")

    def test_declared_category_may_not_shadow(self):
        self.manifest["categories"].append({"id": "vend", "name": "vend"})
        self._fails("already exists")

    def test_unused_declared_category_fails(self):
        self.manifest["categories"].append({"id": "spare", "name": "spare"})
        self._fails("receives no data")

    def test_unknown_keys_fail(self):
        for where in ("manifest", "entry", "record", "category"):
            with self.subTest(where=where):
                self.manifest = _manifest()
                target = {"manifest": self.manifest,
                          "entry": self.manifest["entries"][0],
                          "record": self.manifest["entries"][0]["records"][0],
                          "category": self.manifest["categories"][0]}[where]
                target["credit"] = 1
                self._fails("")

    def test_missing_keys_fail(self):
        for key in ("reason", "moved", "unit_total_data", "extent_model"):
            with self.subTest(key=key):
                self.manifest = _manifest()
                del self.manifest["entries"][0][key]
                self._fails("exactly the keys")
        self.manifest = _manifest()
        del self.manifest["entries"][0]["records"][1]["evidence"]
        self._fails("exactly the keys")

    def test_blank_evidence_fails(self):
        self.manifest["entries"][0]["records"][1]["evidence"] = "  "
        self._fails("malformed data category reassignment record")

    def test_unknown_extent_model_fails(self):
        self.manifest["entries"][0]["extent_model"] = "objdiff-9"
        self._fails("unknown data category reassignment extent model")

    def test_boolean_is_not_a_count(self):
        self.manifest["entries"][0]["records"][0]["size"] = True
        self._fails("malformed size")

    def test_unit_listed_twice_fails_before_any_change(self):
        self.manifest["entries"].append(copy.deepcopy(
            self.manifest["entries"][0]))
        self._fails("listed twice")

    def test_unknown_unit_fails(self):
        self.manifest["entries"][0]["unit"] = "elsewhere"
        self._fails("unit not found")


class ProductionDataCategoryReassignmentTests(unittest.TestCase):
    """The local build's manifest verifies; a verification failure FAILS.

    Missing build inputs are an explicitly reported skip.
    """

    ROOT = Path(__file__).resolve().parents[1]

    def setUp(self):
        self.paths = {
            "report": self.ROOT / "build" / "report.json",
            "objdiff": self.ROOT / "objdiff.json",
            "manifest": self.ROOT / "config" / "data_category_reassignments.json",
            "target": self.ROOT / "build" / "split" / "source" / "linker_common.obj",
        }
        missing = [str(p) for p in self.paths.values() if not p.is_file()]
        if missing:
            self.skipTest("SKIPPED: local build inputs are unavailable: "
                          + ", ".join(missing))

    def test_linker_common_non_halo_records_leave_the_halo_denominator(self):
        report = json.loads(self.paths["report"].read_text(encoding="utf-8"))
        before = copy.deepcopy(report)
        notes = apply_data_category_reassignments(
            report, self.ROOT, self.paths["manifest"], self.paths["objdiff"])
        self.assertEqual(notes, [
            "source/linker_common: halobetacache -1288 -> dsound +4, "
            "libcmt +880, linker +96, xapilib +308 "
            "(29 records: 1213 raw + 75 objdiff padding bytes)"])
        old = {c["id"]: int(c["measures"].get("total_data", 0))
               for c in before["categories"]}
        new = {c["id"]: int(c["measures"].get("total_data", 0))
               for c in report["categories"]}
        self.assertEqual(new["halobetacache"], old["halobetacache"] - 1288)
        self.assertEqual(new["libcmt"], old["libcmt"] + 880)
        self.assertEqual(new["xapilib"], old["xapilib"] + 308)
        self.assertEqual(new["dsound"], old["dsound"] + 4)
        self.assertEqual(new["linker"], 96)
        self.assertEqual(sum(new.values()), sum(old.values()))
        self.assertEqual(sum(new.values()),
                         int(before["measures"]["total_data"]))
        self.assertEqual(report["measures"], before["measures"])
        self.assertEqual(report["units"], before["units"])
        for category in before["categories"]:
            for key in ("matched_data", "complete_data", "matched_code",
                        "matched_functions", "total_units", "complete_units"):
                self.assertEqual(
                    int(next(c for c in report["categories"]
                             if c["id"] == category["id"])["measures"]
                        .get(key, 0)),
                    int(category["measures"].get(key, 0)))


if __name__ == "__main__":
    unittest.main()
