"""Tests for apply_common_pool_credit (owner ruling Q10: pooled COMMON record data credit).

Two groups:
- CommonPoolCreditTests: a synthetic pool unit and a fake verifier run (the verifier's own behaviour is covered by
  tools/test_common_pool_verify.py); they cover the hook's pins, receipts, qualifying set, extent shares, the
  no-double-counting guards and the no-Matching-claim guards.
- RealPoolCreditTests: the real verifier as its command line (a subprocess) on a copy of this tree's build, the pinned
  entry and the XDK libraries it names; skipped when the build or the libraries are absent. They cover the credit the
  entry pins and the verifier's fail-closed classes as progress sees them (pin mismatch, missing library directory,
  a manifest other than the reviewed one, an unreadable object, a malformed ledger).
The reader-pin tests (Q10 addition a) cover a changed, missing or CR-damaged tools/coff_compare.py, a reader loaded
from elsewhere by this process, and (real tree) a reader substituted through sys.path in the verifier's process.
The disclosure tests (Q10 addition b) cover bytes by tag, raw versus padding and the largest record in progress, the
missing-library list, scope and environment sentence in every receipt, and (CommonPoolAllowlistTests) the frozen
Q10 FINAL allowlist and amount, and the EXT1 allowlist extension after it, in the real entry.
"""

import copy
import hashlib
import importlib.util
import inspect
import json
import os
import shutil
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from tools import project_x86, semantic_progress
from tools.coff_compare import build_coff
from tools.semantic_progress import (
    COMMON_POOL_CREDIT_SCHEMA,
    OBJDIFF_331_COMBINED_EXTENT,
    SemanticProgressError,
    apply_common_pool_credit,
    apply_data_category_reassignments,
    clear_common_pool_outputs,
)

BSS_ALIGN1 = 0xC0100080
BSS_ALIGN2 = 0xC0200080
BSS_ALIGN4 = 0xC0300080
BSS_ALIGN8 = 0xC0400080
RDATA_DEFAULT = 0x40000040

VERIFIER = "tools/common_pool_verify.py"
READER = "tools/coff_compare.py"
LEDGER = "research/q10/ledger.json"
MANIFEST = "research/q10/manifest.json"
ENTRY = "config/common_pool_credit.json"
OUT = "build/common_pool"
ABSENT = ["binkxbox.lib", "dsstrmh.lib"]
# the reader this test process loaded, LF (the hook checks the reader it loaded by content, CRLF -> LF)
READER_BYTES = Path(semantic_progress.load.__code__.co_filename).read_bytes().replace(b"\r\n", b"\n")


def _lf_sha(data):
    return hashlib.sha256(data.replace(b"\r\n", b"\n")).hexdigest()


def _load_reader_copy(path, data, name):
    """Load *data* as a COFF reader module from *path* (a reader found elsewhere on sys.path)."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _pool_object(section_changes=None, symbol_changes=None, extra_symbols=()):
    """Five .bss records (shares 8/4/4/4/4 under the objdiff 3.3.1 model) and one .rdata linker record (28).

    The optional arguments change fields of a section (by 1-based number) or of a symbol (by name), or add symbols,
    for the split-object shape checks."""
    sections = [
        {"name": ".bss", "size": 6, "flags": BSS_ALIGN8},
        {"name": ".bss", "size": 4, "flags": BSS_ALIGN4},
        {"name": ".bss", "size": 2, "flags": BSS_ALIGN2},
        {"name": ".bss", "size": 1, "flags": BSS_ALIGN1},
        {"name": ".bss", "size": 4, "flags": BSS_ALIGN4},
        {"name": ".rdata", "size": 28, "flags": RDATA_DEFAULT},
    ]
    symbols = [
        {"name": "_halo_a", "section": 1},
        {"name": "_halo_b", "section": 2},
        {"name": "_halo_c", "section": 3},
        {"name": "_halo_d", "section": 4},
        {"name": "_vend_v", "section": 5},
        {"name": "_rd_l", "section": 6},
        {"name": "_undef", "section": 0},
    ]
    for number, changes in (section_changes or {}).items():
        sections[number - 1].update(changes)
    for symbol in symbols:
        symbol.update((symbol_changes or {}).get(symbol["name"], {}))
    symbols[-1:-1] = list(extra_symbols)
    return build_coff(sections=sections, symbols=symbols)


def _row(index, name, rva, size, segment, category, tag, owner, verdict, reasons=()):
    return {"pool_index": index, "name": name, "rva": rva, "size": size, "segment": segment,
            "category": category, "tag": tag, "owner": owner, "verdict": verdict,
            "reasons": list(reasons)}


def _report():
    """The progress report as the hook sees it: after the 38f82c59-style reassignment of _vend_v and _rd_l."""
    return {
        "measures": {"total_data": 1000, "matched_data": 400, "complete_data": 100,
                     "matched_data_percent": 40.0},
        "categories": [
            {"id": "halo", "name": "halo",
             "measures": {"total_data": 900, "matched_data": 300, "matched_data_percent": 100 / 3,
                          "complete_data": 100}},
            {"id": "vend", "name": "vend",
             "measures": {"total_data": 72, "matched_data": 100 - 30, "matched_data_percent": 70 / 0.72}},
            {"id": "linker", "name": "linker",
             "measures": {"total_data": 28, "matched_data": 0, "matched_data_percent": 0.0}},
        ],
        "units": [
            {"name": "pool", "measures": {"total_data": "52"},
             "sections": [{"name": ".bss", "size": "24"}, {"name": ".rdata", "size": "28"}],
             "metadata": {"progress_categories": ["halo"]}},
            {"name": "other", "measures": {"total_data": "8", "matched_data": "8"},
             "sections": [{"name": ".data", "size": "8"}],
             "metadata": {"progress_categories": ["halo"]}},
        ],
    }


class CommonPoolCreditTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self._write(VERIFIER, b"# fixture verifier\n")
        self._write(READER, READER_BYTES)
        self._write(LEDGER, b'{"fixture": "ledger"}\n')
        self._write(MANIFEST, b'{"fixture": "manifest"}\n')
        self.target = _pool_object()
        self._write("build/split/pool.obj", self.target)
        self._json("objdiff.json", {"units": [
            {"name": "pool", "target_path": "build/split/pool.obj",
             "metadata": {"progress_categories": ["halo"]}},
            {"name": "other", "target_path": "build/split/other.obj", "base_path": "build/base/other.obj",
             "metadata": {"progress_categories": ["halo"]}}]})
        self._json("config/config.json", {"projects": [{"name": "halo", "objects": [
            {"name": "pool.c", "index": 1, "status": "MISSING"},
            {"name": "other.c", "index": 0, "status": "Matching"}]}]})
        self._json("config/semantic_data_matches.json", [
            {"unit": "other", "symbol": "_other_data", "measurements": {}}])
        self._json("config/data_category_reassignments.json", {"entries": [{"unit": "pool", "records": [
            {"symbol": "_vend_v", "section": ".bss", "size": 4, "to_category": "vend", "evidence": "fixture"},
            {"symbol": "_rd_l", "section": ".rdata", "size": 28, "to_category": "linker",
             "evidence": "fixture"}]}]})
        self.entry = self._entry()
        self.calls = []
        self.mutate = None
        self.status = 1

    def tearDown(self):
        self.tmp.cleanup()

    # -- fixture helpers --------------------------------------------------------------------------------------

    def _write(self, relative, data):
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def _json(self, relative, value):
        return self._write(relative, (json.dumps(value, indent=1) + "\n").encode("utf-8"))

    def _pin(self, relative):
        return _lf_sha((self.root / relative).read_bytes())

    def _entry(self):
        return {
            "schema": COMMON_POOL_CREDIT_SCHEMA, "unit": "pool", "category": "halo",
            "extent_model": OBJDIFF_331_COMBINED_EXTENT, "unit_total_data": 52,
            "verifier": {"path": VERIFIER, "sha256_lf": self._pin(VERIFIER)},
            "reader": {"path": READER, "sha256_lf": self._pin(READER)},
            "ledger": {"path": LEDGER, "sha256_lf": self._pin(LEDGER)},
            "xdk_manifest": {"path": MANIFEST, "sha256_lf": self._pin(MANIFEST)},
            "xdk_lib_dir": "C:/fixture/lib", "linked_libraries_not_in_extract": list(ABSENT),
            "credit": {"records": 2, "raw": 10, "padding": 2, "total": 12},
            "reason": "fixture", "scope": "fixture scope", "limitation": "fixture L1",
            "records": [
                {"symbol": "_halo_a", "rva": "0x100", "size": 6, "share": 8, "tag": "inferred",
                 "owner": "src/a.c"},
                {"symbol": "_halo_b", "rva": "0x108", "size": 4, "share": 4, "tag": "probable",
                 "owner": "src/b.c"},
            ],
        }

    def _outputs(self, status):
        receipt = {
            "schema": "common-pool-verify-receipt/1", "exit_code": status,
            "result": "COMPLETE" if status in (0, 1) else "FAIL-CLOSED",
            "fail_closed": None if status in (0, 1) else {
                "class": {2: "unreadable-input", 3: "libraries-missing-or-unverifiable", 4: "malformed-types",
                          5: "ambiguous-classification"}[status],
                "exit_code": status, "message": "fixture failure"},
            "verifier": {"path": VERIFIER, "sha256_lf": self._pin(VERIFIER),
                         "coff_compare_sha256_lf": self._pin(READER)},
            "inputs": {"ledger": {"sha256_lf": self._pin(LEDGER)},
                       "xdk_manifest": {"sha256_lf": self._pin(MANIFEST)},
                       "split_pool": {"sha256": hashlib.sha256(
                           (self.root / "build/split/pool.obj").read_bytes()).hexdigest()}},
            "xdk_libraries": {"libraries": {"a.lib": {"verified": True}, "b.lib": {"verified": True}},
                              "linked_not_in_extract": list(ABSENT)},
        }
        report = {
            "result": receipt["result"], "exit_code": status, "global_failures": [],
            "accounting": {"conservation": {"holds": True}},
            "records": [
                _row(0, "_halo_a", "0x100", 6, "halo", "halo", "inferred", "src/a.c", "PASS"),
                _row(1, "_halo_b", "0x108", 4, "halo", "halo", "probable", "src/b.c", "PASS"),
                _row(2, "_halo_c", "0x10c", 2, "halo", "halo", "unresolved", None, "PASS-OWNER-UNRESOLVED"),
                _row(3, "_halo_d", "0x10e", 1, "halo", "halo", "probable", "src/d.c", "FAIL",
                     ["no-definer:absent"]),
                _row(4, "_vend_v", "0x110", 4, "vendor", "vend", "probable", "vend/pool.obj", "FAIL",
                     ["no-definer:absent"]),
            ],
            "excluded_linker_records": [{"name": "_rd_l", "size": 28}],
        }
        return receipt, report

    def _runner(self, project_root, argv):
        self.calls.append(list(argv))
        args = dict(zip(argv[::2], argv[1::2]))
        receipt, report = self._outputs(self.status)
        if self.mutate:
            self.mutate(receipt, report)
        report.setdefault("receipt", receipt)
        if report.get("write", True) is not False:
            report.pop("write", None)
            Path(args["--json"]).write_text(json.dumps(report), encoding="utf-8")
        if receipt.pop("write", True) is not False:
            Path(args["--receipt"]).write_text(json.dumps(receipt), encoding="utf-8")
        Path(args["--text"]).write_text("fixture text report\n", encoding="utf-8")
        return self.status, "", "fixture stderr"

    def _run(self, report=None, entry=None, runner=None):
        report = _report() if report is None else report
        if entry is not False:
            self._json(ENTRY, self.entry if entry is None else entry)
        notes = apply_common_pool_credit(
            report, self.root, ENTRY, "objdiff.json", "config/semantic_data_matches.json",
            "config/data_category_reassignments.json", Path("build"), runner or self._runner)
        return report, notes

    def _receipt(self):
        return json.loads((self.root / OUT / "credit_receipt.json").read_text(encoding="utf-8"))

    def _fails(self, pattern, report=None, entry=None, runner=None):
        report = _report() if report is None else report
        before = copy.deepcopy(report)
        with self.assertRaisesRegex(SemanticProgressError, pattern):
            self._run(report, entry, runner)
        self.assertEqual(report, before, "a failure must not change any measure")
        receipt = self._receipt()
        self.assertEqual(receipt["outcome"], "FAIL-CLOSED")
        self.assertRegex(receipt["reason"], pattern)
        self.assertIsNone(receipt["credit"])
        return receipt

    # -- the credit -------------------------------------------------------------------------------------------

    def test_credits_exactly_the_listed_pass_shares(self):
        report, notes = self._run()
        before = _report()
        self.assertEqual(report["measures"]["matched_data"], 400 + 12)
        categories = {c["id"]: c["measures"] for c in report["categories"]}
        self.assertEqual(categories["halo"]["matched_data"], 300 + 12)
        self.assertAlmostEqual(categories["halo"]["matched_data_percent"], 100.0 * 312 / 900)
        unit = report["units"][0]["measures"]
        self.assertEqual(unit["matched_data"], 12)
        # denominators, completion, vendor and linker categories, other units: untouched
        self.assertEqual(report["measures"]["total_data"], 1000)
        self.assertEqual(report["measures"]["complete_data"], 100)
        self.assertEqual(categories["halo"]["total_data"], 900)
        self.assertEqual(categories["halo"]["complete_data"], 100)
        self.assertEqual(report["categories"][1:], before["categories"][1:])
        self.assertEqual(report["units"][1], before["units"][1])
        self.assertEqual(report["units"][0]["sections"], before["units"][0]["sections"])
        self.assertNotIn("complete_data", unit)
        self.assertEqual(len(notes), 2)
        self.assertIn("pool: +12 halo data bytes (2 PASS Halo records: 10 raw + 2 objdiff padding bytes); "
                      "uncredited: ", notes[0])
        self.assertIn("1 PASS-OWNER-UNRESOLVED, 1 FAIL, 0 unlisted PASS, 1 vendor and 1 linker records",
                      notes[0])
        self.assertIn("stays MISSING (no Matching claim)", notes[0])

    def test_progress_shows_bytes_by_tag_raw_padding_and_the_largest_record(self):
        # owner, Q10 additions: inferred/probable bytes, raw versus padding, and the largest record's share
        _, notes = self._run()
        self.assertEqual(notes[1], "pool by tag: inferred 1 records +8 (6 raw + 2 padding), probable 1 records +4 "
                                   "(4 raw + 0 padding); largest record _halo_a (inferred) +8 (6 raw + 2 padding) "
                                   "= 66.7% of the credit")

    def test_receipt_records_raw_padding_and_every_uncredited_class(self):
        self._run()
        receipt = self._receipt()
        self.assertEqual(receipt["outcome"], "CREDITED")
        credit = receipt["credit"]
        self.assertEqual((credit["records"], credit["raw"], credit["padding"], credit["total"]), (2, 10, 2, 12))
        self.assertEqual(credit["by_tag"], {"inferred": {"records": 1, "raw": 6, "padding": 2, "share": 8},
                                            "probable": {"records": 1, "raw": 4, "padding": 0, "share": 4}})
        self.assertEqual(credit["largest_record"], {"symbol": "_halo_a", "tag": "inferred", "raw": 6,
                                                    "padding": 2, "share": 8})
        self.assertEqual(receipt["linked_libraries_not_in_extract"], ABSENT)
        self.assertEqual(receipt["scope"], "fixture scope")
        self.assertEqual(receipt["environment"], semantic_progress.COMMON_POOL_ENVIRONMENT)
        self.assertTrue(receipt["environment"].startswith(
            "Pins are not protection against a compromised execution environment."))
        self.assertEqual(credit["rows"], [["_halo_a", "inferred", 6, 8], ["_halo_b", "probable", 4, 4]])
        self.assertEqual(credit["uncredited"], {
            "halo_pass_owner_unresolved": [1, 2], "halo_fail": [1, 1], "halo_pass_not_listed": [0, 0],
            "vendor": [1, 4], "linker_excluded": [1, 28]})
        self.assertEqual(credit["matched_data_before"], {"overall": 400, "category": 300, "unit": 0})
        self.assertEqual(credit["matched_data_after"], {"overall": 412, "category": 312, "unit": 12})
        self.assertEqual(set(receipt["pins"]), {"verifier", "reader", "ledger", "xdk_manifest"})
        for pin in receipt["pins"].values():
            self.assertEqual(pin["expected"], pin["actual"])
        reader = receipt["pins"]["reader"]
        self.assertEqual(reader["path"], READER)
        self.assertEqual(reader["expected"], _lf_sha(READER_BYTES))
        self.assertEqual(reader["loaded_by_hook"]["actual"], reader["expected"])
        self.assertEqual(Path(reader["loaded_by_hook"]["path"]), Path(semantic_progress.load.__code__.co_filename))
        self.assertEqual(reader["verifier_receipt"], reader["expected"])
        self.assertEqual(receipt["verifier_run"]["exit_code"], 1)
        self.assertIn("L1", receipt["limitation"])
        self.assertEqual(receipt["split_pool"]["sha256"], hashlib.sha256(self.target).hexdigest())

    def test_runs_the_verifier_with_the_lib_dir_and_the_pinned_manifest(self):
        self._run()
        self.assertEqual(len(self.calls), 1)
        args = dict(zip(self.calls[0][::2], self.calls[0][1::2]))
        self.assertEqual(args["--xdk-lib-dir"], "C:/fixture/lib")
        self.assertEqual(Path(args["--xdk-lib-manifest"]), self.root / MANIFEST)
        self.assertEqual(Path(args["--ledger"]), self.root / LEDGER)
        self.assertEqual(Path(args["--split-pool"]), self.root / "build/split/pool.obj")
        self.assertEqual(Path(args["--objects"]), self.root / "build/base")
        for key in ("--receipt", "--json", "--text"):
            self.assertEqual(Path(args[key]).parent, self.root / OUT)

    def test_an_exit_0_run_credits_too(self):
        self.status = 0
        report, _ = self._run()
        self.assertEqual(report["measures"]["matched_data"], 412)

    def test_an_unlisted_pass_record_earns_nothing(self):
        self.entry["records"] = self.entry["records"][:1]
        self.entry["credit"] = {"records": 1, "raw": 6, "padding": 2, "total": 8}
        report, notes = self._run()
        self.assertEqual(report["measures"]["matched_data"], 408)
        self.assertIn("1 unlisted PASS", notes[0])
        self.assertEqual(self._receipt()["credit"]["not_listed"], ["_halo_b"])

    def test_a_vendor_pass_record_is_never_halo_credit(self):
        def vendor_passes(receipt, report):
            report["records"][4].update(verdict="PASS", reasons=[])
        self.mutate = vendor_passes
        report, notes = self._run()
        self.assertEqual(report["measures"]["matched_data"], 412)
        self.assertIn("0 unlisted PASS, 1 vendor", notes[0])

    def test_absent_entry_is_a_no_op_and_clears_stale_outputs(self):
        self._write(OUT + "/credit_receipt.json", b'{"outcome": "CREDITED"}')
        self._write(OUT + "/verify_receipt.json", b"{}")
        report = _report()
        report_out, notes = self._run(report=report, entry=False)
        self.assertEqual(notes, [])
        self.assertEqual(report_out, _report())
        self.assertFalse((self.root / OUT / "credit_receipt.json").exists())
        self.assertFalse((self.root / OUT / "verify_receipt.json").exists())
        self.assertEqual(self.calls, [])

    # -- fail closed: pins ----------------------------------------------------------------------------------

    def test_verifier_pin_mismatch_fails_before_the_run(self):
        self._write(VERIFIER, b"# a changed verifier\n")
        receipt = self._fails(r"pin mismatch: verifier")
        self.assertEqual(self.calls, [])
        self.assertNotEqual(receipt["pins"]["verifier"]["expected"], receipt["pins"]["verifier"]["actual"])

    def test_ledger_pin_mismatch_fails_before_the_run(self):
        self._write(LEDGER, b'{"fixture": "edited ledger"}\n')
        self._fails(r"pin mismatch: ledger")
        self.assertEqual(self.calls, [])

    def test_manifest_pin_mismatch_fails_before_the_run(self):
        self._write(MANIFEST, b'{"fixture": "another manifest"}\n')
        self._fails(r"pin mismatch: xdk_manifest")
        self.assertEqual(self.calls, [])

    # -- fail closed: the reader pin (Q10 addition a) -----------------------------------------------------------

    def test_reader_pin_mismatch_fails_before_the_run(self):
        self._write(READER, READER_BYTES + b"\n# a changed reader\n")
        receipt = self._fails(r"pin mismatch: reader tools/coff_compare\.py")
        self.assertEqual(self.calls, [])
        self.assertNotEqual(receipt["pins"]["reader"]["expected"], receipt["pins"]["reader"]["actual"])
        self.assertIsNone(receipt["verifier_run"])

    def test_a_missing_reader_fails_before_the_run(self):
        (self.root / READER).unlink()
        receipt = self._fails(r"cannot read pinned reader")
        self.assertEqual(self.calls, [])
        self.assertIsNone(receipt["verifier_run"])

    def test_a_lone_cr_in_the_reader_is_a_pin_mismatch(self):
        self._write(READER, READER_BYTES.replace(b"\n", b"\r", 1))
        self._fails(r"pin mismatch: reader")
        self.assertEqual(self.calls, [])

    def test_a_changed_reader_loaded_by_this_process_fails_before_the_run(self):
        # the tree's file is the pinned one, but the reader this process computes the shares with came from
        # elsewhere (a regular 'tools' package earlier on sys.path, say) and differs
        module = _load_reader_copy(self.root / "elsewhere" / "coff_compare.py",
                                   READER_BYTES + b"\n# a changed reader\n", "q10_changed_reader")
        with mock.patch.object(semantic_progress, "load", module.load):
            receipt = self._fails(r"pin mismatch: the COFF reader this process loaded")
        self.assertEqual(self.calls, [])
        loaded = receipt["pins"]["reader"]["loaded_by_hook"]
        self.assertEqual(Path(loaded["path"]), self.root / "elsewhere" / "coff_compare.py")
        self.assertNotEqual(loaded["actual"], receipt["pins"]["reader"]["expected"])
        self.assertEqual(receipt["pins"]["reader"]["actual"], receipt["pins"]["reader"]["expected"])

    def test_the_loaded_reader_is_checked_by_content_not_location(self):
        module = _load_reader_copy(self.root / "elsewhere" / "coff_compare.py", READER_BYTES,
                                   "q10_identical_reader")
        with mock.patch.object(semantic_progress, "load", module.load):
            report, _ = self._run()
        self.assertEqual(report["measures"]["matched_data"], 412)

    def test_an_unidentifiable_loaded_reader_fails_closed(self):
        with mock.patch.object(semantic_progress, "load", len):   # a builtin names no source file
            self._fails(r"cannot identify the COFF reader this process loaded")
        self.assertEqual(self.calls, [])

    def test_receipt_must_name_the_pinned_reader(self):
        receipt = self._bound_fails(r"not bound to the pins: reader sha256_lf",
                                    lambda r, p: r["verifier"].update(coff_compare_sha256_lf="4" * 64))
        self.assertEqual(receipt["pins"]["reader"]["verifier_receipt"], "4" * 64)

    def test_a_verifier_that_found_no_reader_fails_closed(self):
        self._bound_fails(r"not bound to the pins: reader sha256_lf",
                          lambda r, p: r["verifier"].update(coff_compare_sha256_lf=None))

    def test_a_verifier_receipt_without_a_reader_hash_is_malformed(self):
        self._bound_fails(r"malformed COMMON pool verifier receipt or report: KeyError",
                          lambda r, p: r["verifier"].pop("coff_compare_sha256_lf"))

    def test_a_crlf_checkout_of_the_pinned_files_passes(self):
        for relative in (VERIFIER, READER, LEDGER, MANIFEST):
            path = self.root / relative
            path.write_bytes(path.read_bytes().replace(b"\n", b"\r\n"))
        report, _ = self._run()
        self.assertEqual(report["measures"]["matched_data"], 412)

    def test_missing_pinned_file_fails_closed(self):
        (self.root / LEDGER).unlink()
        self._fails(r"cannot read pinned ledger")
        self.assertEqual(self.calls, [])

    # -- disclosures every receipt carries (Q10 addition b) -------------------------------------------------

    def test_a_fail_closed_receipt_of_a_valid_entry_carries_the_disclosures(self):
        self._write(LEDGER, b'{"fixture": "edited ledger"}\n')
        receipt = self._fails(r"pin mismatch: ledger")
        self.assertEqual(receipt["linked_libraries_not_in_extract"], ABSENT)
        self.assertEqual(receipt["scope"], "fixture scope")
        self.assertEqual(receipt["limitation"], semantic_progress.COMMON_POOL_L1)
        self.assertEqual(receipt["environment"], semantic_progress.COMMON_POOL_ENVIRONMENT)

    def test_a_malformed_entry_leaves_the_entry_disclosures_empty(self):
        self._write(ENTRY, b"{")
        with self.assertRaisesRegex(SemanticProgressError, r"malformed COMMON pool credit entry"):
            self._run(entry=False)
        receipt = self._receipt()
        self.assertEqual(receipt["outcome"], "FAIL-CLOSED")
        self.assertIsNone(receipt["linked_libraries_not_in_extract"])
        self.assertIsNone(receipt["scope"])
        self.assertEqual(receipt["environment"], semantic_progress.COMMON_POOL_ENVIRONMENT)

    # -- fail closed: the verifier's own classes ------------------------------------------------------------

    def test_every_fail_closed_verifier_exit_fails_progress(self):
        for status, name in ((2, "unreadable-input"), (3, "libraries-missing-or-unverifiable"),
                             (4, "malformed-types"), (5, "ambiguous-classification")):
            with self.subTest(status=status):
                self.status = status
                receipt = self._fails(rf"verifier failed closed: exit {status} \({name}\): fixture failure")
                self.assertEqual(receipt["verifier_run"]["exit_code"], status)

    def test_a_crash_without_a_receipt_fails_closed(self):
        self.status = 1

        def crash(project_root, argv):
            return 1, "", "Traceback (most recent call last): boom"
        self._fails(r"exited 1 without its receipt and report", runner=crash)

    def test_an_unknown_exit_code_fails_closed(self):
        # an exit status outside the verifier's contract, with a receipt that carries no class
        def runner(project_root, argv):
            args = dict(zip(argv[::2], argv[1::2]))
            Path(args["--receipt"]).write_text("{}", encoding="utf-8")
            return 7, "", "odd"
        self._fails(r"verifier failed closed: exit 7 \(no receipt\)", runner=runner)

    def test_a_runner_exception_is_receipted(self):
        def runner(project_root, argv):
            raise RuntimeError("the interpreter vanished")
        self._fails(r"unexpected RuntimeError: the interpreter vanished", runner=runner)

    # -- fail closed: the run must be bound to the pins and complete ----------------------------------------

    def _bound_fails(self, pattern, mutate):
        self.mutate = mutate
        return self._fails(pattern)

    def test_receipt_exit_code_must_equal_the_process_status(self):
        self._bound_fails(r"receipt exit code", lambda r, p: r.update(exit_code=0))

    def test_receipt_must_name_the_pinned_verifier(self):
        self._bound_fails(r"verifier sha256_lf", lambda r, p: r["verifier"].update(sha256_lf="0" * 64))

    def test_receipt_must_name_the_pinned_ledger_and_manifest(self):
        self._bound_fails(r"ledger sha256_lf", lambda r, p: r["inputs"]["ledger"].update(sha256_lf="1" * 64))
        self._bound_fails(r"manifest sha256_lf",
                          lambda r, p: r["inputs"]["xdk_manifest"].update(sha256_lf="2" * 64))

    def test_receipt_must_name_the_split_pool_the_hook_reads(self):
        self._bound_fails(r"split pool sha256", lambda r, p: r["inputs"]["split_pool"].update(sha256="3" * 64))

    def test_report_must_embed_the_same_receipt(self):
        self._bound_fails(r"report receipt", lambda r, p: p.update(receipt={"other": True}))

    def test_undisclosed_library_gap_fails_closed(self):
        self._bound_fails(r"libraries not in the extract",
                          lambda r, p: r["xdk_libraries"].update(linked_not_in_extract=["binkxbox.lib"]))

    def test_unverified_library_fails_closed(self):
        self._bound_fails(r"unverified libraries",
                          lambda r, p: r["xdk_libraries"]["libraries"]["b.lib"].update(verified=False))

    def test_global_failures_fail_closed(self):
        self._bound_fails(r"global failures", lambda r, p: p.update(global_failures=["surplus-common: _x"]))

    def test_broken_conservation_fails_closed(self):
        self._bound_fails(r"conservation", lambda r, p: p["accounting"]["conservation"].update(holds=False))

    def test_malformed_verifier_report_fails_closed(self):
        self._bound_fails(r"malformed COMMON pool verifier receipt or report",
                          lambda r, p: p.pop("excluded_linker_records"))

    def test_verifier_report_that_is_not_json_fails_closed(self):
        def runner(project_root, argv):
            self._runner(project_root, argv)
            args = dict(zip(argv[::2], argv[1::2]))
            Path(args["--json"]).write_text("{not json", encoding="utf-8")
            return 1, "", ""
        self._fails(r"malformed COMMON pool verifier report", runner=runner)

    def test_duplicate_keys_in_the_verifier_receipt_fail_closed(self):
        def runner(project_root, argv):
            self._runner(project_root, argv)
            args = dict(zip(argv[::2], argv[1::2]))
            text = Path(args["--receipt"]).read_text(encoding="utf-8")
            Path(args["--receipt"]).write_text(text[:-1] + ', "exit_code": 1}', encoding="utf-8")
            return 1, "", ""
        self._fails(r"repeats JSON key", runner=runner)

    # -- fail closed: the listed records --------------------------------------------------------------------

    def test_listed_record_that_fails_now_fails_closed(self):
        self._bound_fails(r"not a qualifying PASS: _halo_b",
                          lambda r, p: p["records"][1].update(verdict="FAIL", reasons=["no-definer:absent"]))

    def test_listed_record_that_became_owner_unresolved_fails_closed(self):
        self._bound_fails(r"not a qualifying PASS: _halo_a", lambda r, p: p["records"][0].update(
            verdict="PASS-OWNER-UNRESOLVED", tag="unresolved"))

    def test_a_non_pass_verdict_is_refused_even_without_reasons(self):
        # an inconsistent report (verdict FAIL, no reasons, a qualifying tag) must not qualify
        self._bound_fails(r"not a qualifying PASS: _halo_b",
                          lambda r, p: p["records"][1].update(verdict="FAIL", reasons=[]))

    def test_a_non_halo_segment_is_refused_even_in_the_halo_category(self):
        # an inconsistent report (vendor segment, Halo category) must not qualify
        self._bound_fails(r"not a qualifying PASS: _halo_b",
                          lambda r, p: p["records"][1].update(segment="vendor"))

    def test_listed_record_missing_from_the_run_fails_closed(self):
        self._bound_fails(r"not a qualifying PASS: _halo_a", lambda r, p: p["records"].pop(0))

    def test_listed_record_with_another_tag_owner_size_or_rva_fails_closed(self):
        for key, value in (("tag", "inferred"), ("owner", "src/z.c"), ("size", 5), ("rva", "0x104")):
            with self.subTest(key=key):
                self._bound_fails(r"record changed: _halo_b",
                                  lambda r, p, key=key, value=value: p["records"][1].update({key: value}))

    def test_a_pinned_unresolved_or_fail_record_is_rejected(self):
        entry = self._entry()
        entry["records"][1]["tag"] = "unresolved"
        self._fails(r"malformed COMMON pool credit record", entry=entry)
        entry = self._entry()
        entry["records"].append({"symbol": "_halo_d", "rva": "0x10e", "size": 1, "share": 4,
                                 "tag": "probable", "owner": "src/d.c"})
        entry["credit"] = {"records": 3, "raw": 11, "padding": 5, "total": 16}
        self._fails(r"not a qualifying PASS: _halo_d", entry=entry)

    def test_a_wrong_share_fails_closed(self):
        entry = self._entry()
        entry["records"][0]["share"] = 6
        entry["credit"] = {"records": 2, "raw": 10, "padding": 0, "total": 10}
        self._fails(r"does not match the split object: _halo_a", entry=entry)

    def test_a_record_the_split_object_does_not_define_fails_closed(self):
        def ghost(receipt, report):
            report["records"].append(_row(9, "_ghost", "0x200", 4, "halo", "halo", "inferred", "src/g.c",
                                          "PASS"))
        self.mutate = ghost
        entry = self._entry()
        entry["records"].append({"symbol": "_ghost", "rva": "0x200", "size": 4, "share": 4, "tag": "inferred",
                                 "owner": "src/g.c"})
        entry["credit"] = {"records": 3, "raw": 14, "padding": 2, "total": 16}
        self._fails(r"not one external symbol at the start of its section: _ghost", entry=entry)

    # -- fail closed: malformed entry ----------------------------------------------------------------------

    def test_malformed_entries_fail_closed(self):
        cases = [
            ("unknown key", lambda e: e.update(extra=1), r"exactly the keys"),
            ("missing key", lambda e: e.pop("limitation"), r"exactly the keys"),
            ("schema", lambda e: e.update(schema="common-pool-credit/0"), r"schema"),
            ("model", lambda e: e.update(extent_model="objdiff-3.6.0"), r"extent model"),
            ("empty reason", lambda e: e.update(reason=" "), r"reason must be"),
            ("bool total", lambda e: e.update(unit_total_data=True), r"unit_total_data"),
            ("short sha", lambda e: e["ledger"].update(sha256_lf="abc"), r"ledger pin"),
            ("upper sha", lambda e: e["ledger"].update(sha256_lf="A" * 64), r"ledger pin"),
            ("sha + newline", lambda e: e["ledger"].update(sha256_lf="a" * 64 + "\n"), r"ledger pin"),
            ("rva + newline", lambda e: e["records"][0].update(rva="0x100\n"),
             r"malformed COMMON pool credit record"),
            ("absolute ledger", lambda e: e["ledger"].update(path="C:/ledger.json"), r"repository-relative"),
            ("dotdot ledger", lambda e: e["ledger"].update(path="../ledger.json"), r"repository-relative"),
            ("backslash", lambda e: e["xdk_manifest"].update(path="research\\q10\\manifest.json"),
             r"repository-relative"),
            ("verifier path", lambda e: e["verifier"].update(path="tools/other_verify.py"),
             r"verifier path must be"),
            ("reader path", lambda e: e["reader"].update(path="tools/other_reader.py"),
             r"reader path must be tools/coff_compare\.py"),
            ("no reader pin", lambda e: e.pop("reader"), r"exactly the keys"),
            ("reader sha", lambda e: e["reader"].update(sha256_lf="B" * 64), r"reader pin"),
            ("reader pin keys", lambda e: e["reader"].update(size=1), r"reader pin"),
            ("empty scope", lambda e: e.update(scope=" "), r"scope must be"),
            ("no scope", lambda e: e.pop("scope"), r"exactly the keys"),
            ("absent not list", lambda e: e.update(linked_libraries_not_in_extract="binkxbox.lib"),
             r"linked_libraries_not_in_extract"),
            ("no records", lambda e: e.update(records=[]), r"needs records"),
            ("record key", lambda e: e["records"][0].update(section=".bss"), r"record must have exactly"),
            ("zero size", lambda e: e["records"][0].update(size=0), r"size of _halo_a"),
            ("share < size", lambda e: e["records"][0].update(share=5), r"below its size"),
            ("rva", lambda e: e["records"][0].update(rva="256"), r"malformed COMMON pool credit record"),
            ("duplicate record", lambda e: e["records"].append(dict(e["records"][0])), r"lists a record twice"),
            ("totals", lambda e: e["credit"].update(total=13), r"totals do not add up"),
        ]
        for name, change, pattern in cases:
            with self.subTest(name=name):
                entry = self._entry()
                change(entry)
                self._fails(pattern, entry=entry)
        self.assertEqual(self.calls, [], "a malformed entry must stop before the verifier runs")

    def test_duplicate_keys_in_the_entry_fail_closed(self):
        text = json.dumps(self.entry)
        self._write(ENTRY, (text[:-1] + ', "unit": "pool"}').encode("utf-8"))
        with self.assertRaisesRegex(SemanticProgressError, r"repeats JSON key"):
            self._run(entry=False)
        self.assertEqual(self._receipt()["outcome"], "FAIL-CLOSED")

    def test_entry_that_is_not_json_fails_closed(self):
        self._write(ENTRY, b"{")
        with self.assertRaisesRegex(SemanticProgressError, r"malformed COMMON pool credit entry"):
            self._run(entry=False)

    # -- no double counting --------------------------------------------------------------------------------

    def test_a_reassigned_vendor_record_can_never_be_listed(self):
        entry = self._entry()
        entry["records"].append({"symbol": "_vend_v", "rva": "0x110", "size": 4, "share": 4,
                                 "tag": "probable", "owner": "vend/pool.obj"})
        entry["credit"] = {"records": 3, "raw": 14, "padding": 2, "total": 16}
        self._fails(r"reassigned to another category: \['_vend_v'\]", entry=entry)
        self.assertEqual(self.calls, [])

    def test_a_semantic_data_entry_for_the_unit_fails_closed(self):
        self._json("config/semantic_data_matches.json", [{"unit": "pool", "symbol": "_other"}])
        self._fails(r"also has a semantic data entry: pool")

    def test_a_record_named_by_a_semantic_data_entry_fails_closed(self):
        self._json("config/semantic_data_matches.json", [
            {"unit": "other", "group": "g", "members": [{"symbol": "_halo_b", "measurements": {}}]}])
        self._fails(r"also named by a semantic data entry: \['_halo_b'\]")

    def test_a_unit_with_matched_data_fails_closed(self):
        report = _report()
        report["units"][0]["measures"]["matched_data"] = "4"
        self._fails(r"no matched or complete data \(matched_data\)", report=report)

    def test_the_hook_never_credits_twice(self):
        report, _ = self._run()
        credited = copy.deepcopy(report)
        with self.assertRaisesRegex(SemanticProgressError, r"no matched or complete data"):
            self._run(report=report)
        self.assertEqual(report, credited)
        self.assertEqual(report["measures"]["matched_data"], 412)

    def test_credit_beyond_the_category_total_fails_closed(self):
        report = _report()
        report["categories"][0]["measures"]["matched_data"] = 895
        self._fails(r"exceed the halo data total", report=report)

    def test_a_listed_record_later_reassigned_to_another_category_fails_closed(self):
        # a record moved out of the Halo denominator can no longer earn Halo credit
        self._json("config/data_category_reassignments.json", {"entries": [{"unit": "pool", "records": [
            {"symbol": "_halo_c"}, {"symbol": "_halo_d"}, {"symbol": "_vend_v"}, {"symbol": "_rd_l"},
            {"symbol": "_halo_b"}]}]})
        self._fails(r"reassigned to another category: \['_halo_b'\]")

    # -- no whole-pool Matching claim ----------------------------------------------------------------------

    def test_a_complete_unit_is_refused(self):
        report = _report()
        report["units"][0]["metadata"]["complete"] = True
        self._fails(r"marked complete \(report\)", report=report)

    def test_a_matching_config_status_is_refused(self):
        self._json("config/config.json", {"projects": [{"name": "halo", "objects": [
            {"name": "pool.c", "index": 1, "status": "Matching"}]}]})
        self._fails(r"one MISSING config object")

    def test_a_rebuilt_object_for_the_unit_is_refused(self):
        objdiff = json.loads((self.root / "objdiff.json").read_text(encoding="utf-8"))
        objdiff["units"][0]["base_path"] = "build/base/pool.obj"
        self._json("objdiff.json", objdiff)
        self._fails(r"no rebuilt object")

    def test_a_unit_in_two_categories_is_refused(self):
        report = _report()
        report["units"][0]["metadata"]["progress_categories"] = ["halo", "vend"]
        self._fails(r"not solely in 'halo'", report=report)

    def test_a_changed_unit_total_fails_closed(self):
        entry = self._entry()
        entry["unit_total_data"] = 53
        self._fails(r"unit total changed", entry=entry)

    def test_a_report_not_bound_to_the_split_object_fails_closed(self):
        report = _report()
        report["units"][0]["sections"][0]["size"] = "20"
        self._fails(r"not the target's modelled extent", report=report)

    def test_a_credit_never_moves_completion_or_denominators(self):
        report, _ = self._run()
        before = _report()
        for key in ("total_data", "complete_data"):
            self.assertEqual(report["measures"][key], before["measures"][key])
            self.assertEqual(report["categories"][0]["measures"][key], before["categories"][0]["measures"][key])
        self.assertNotIn("complete", report["units"][0]["metadata"])

    # -- receipts ------------------------------------------------------------------------------------------

    def test_an_unwritable_receipt_fails_closed_without_credit(self):
        self._write("build/common_pool", b"a file where the output directory should be")
        report = _report()
        before = copy.deepcopy(report)
        with self.assertRaisesRegex(SemanticProgressError,
                                    r"failed closed: unexpected FileExistsError.*; and cannot write the "
                                    r"COMMON pool credit receipt"):
            self._run(report=report)
        self.assertEqual(report, before)

    def test_a_stale_receipt_never_survives_a_failed_run(self):
        self._run()
        self.assertEqual(self._receipt()["outcome"], "CREDITED")
        self._write(LEDGER, b'{"fixture": "edited"}\n')
        self._fails(r"pin mismatch: ledger")
        self.assertFalse((self.root / OUT / "verify_receipt.json").exists())
        self.assertFalse((self.root / OUT / "verify_report.json").exists())

    # -- RF-EI v1.4: stale outputs (D1) --------------------------------------------------------------------

    def _block(self, name):
        """Replace an output of the previous (credited) run by a directory, which no unlink can remove."""
        blocker = self.root / OUT / name
        blocker.unlink()
        blocker.mkdir()
        return blocker

    def test_an_unremovable_verifier_output_is_receipted_and_the_old_receipt_is_gone(self):
        self._run()
        self._block("verify_receipt.json")
        receipt = self._fails(r"cannot remove the stale COMMON pool output verify_receipt\.json")
        self.assertIsNone(receipt["verifier_run"])
        self.assertEqual(len(self.calls), 1, "the verifier must not run after a stale-output failure")

    def test_an_unremovable_receipt_fails_closed_and_says_it_is_stale(self):
        self._run()
        self._block("credit_receipt.json")
        report = _report()
        before = copy.deepcopy(report)
        with self.assertRaisesRegex(SemanticProgressError,
                                    r"cannot remove the stale COMMON pool output credit_receipt\.json: .*does not "
                                    r"describe this run; and cannot write the COMMON pool credit receipt"):
            self._run(report=report)
        self.assertEqual(report, before)
        self.assertEqual(len(self.calls), 1)

    def test_the_absent_entry_path_still_fails_on_an_unremovable_output(self):
        self._run()
        (self.root / ENTRY).unlink()
        self._block("verify_report.json")
        with self.assertRaisesRegex(SemanticProgressError,
                                    r"cannot remove the stale COMMON pool output verify_report\.json"):
            self._run(entry=False)
        receipt = self._receipt()
        self.assertEqual(receipt["outcome"], "FAIL-CLOSED")
        self.assertRegex(receipt["reason"], r"verify_report\.json: .*does not describe this run")

    def test_clear_records_an_unremovable_verifier_output_in_a_fail_closed_receipt(self):
        self._run()
        self._block("verify_report.txt")
        with self.assertRaisesRegex(SemanticProgressError,
                                    r"cannot remove the stale COMMON pool output verify_report\.txt: .*does not "
                                    r"describe this run"):
            clear_common_pool_outputs(Path("build"), self.root)
        receipt = self._receipt()
        self.assertEqual(receipt["outcome"], "FAIL-CLOSED")
        self.assertIsNone(receipt["credit"])
        self.assertEqual(receipt["limitation"], "L1: pinning prevents unnoticed changes to reviewed assumptions; it "
                                                "does not prove those assumptions or January ownership.")
        # the same receipt schema as the hook's own (Q10 addition b)
        self.assertEqual(receipt["environment"], semantic_progress.COMMON_POOL_ENVIRONMENT)
        self.assertIsNone(receipt["scope"])
        self.assertIsNone(receipt["linked_libraries_not_in_extract"])

    def test_clear_common_pool_outputs_removes_exactly_the_four_outputs(self):
        out = self.root / OUT
        out.mkdir(parents=True)
        for name in ("credit_receipt.json", "verify_receipt.json", "verify_report.json", "verify_report.txt",
                     "unrelated.txt"):
            (out / name).write_text("x", encoding="utf-8")
        self.assertEqual(clear_common_pool_outputs(Path("build"), self.root), out)
        self.assertEqual(sorted(path.name for path in out.iterdir()), ["unrelated.txt"])

    def test_the_credit_receipt_is_written_before_any_measure_changes(self):
        from tools import semantic_progress
        original = semantic_progress._write_pool_receipt

        def refuse_credited(path, receipt):
            if receipt.get("outcome") == "CREDITED":
                raise SemanticProgressError("cannot write the COMMON pool credit receipt (fixture)")
            original(path, receipt)
        report = _report()
        before = copy.deepcopy(report)
        with mock.patch.object(semantic_progress, "_write_pool_receipt", refuse_credited):
            with self.assertRaisesRegex(SemanticProgressError, r"credit receipt \(fixture\)"):
                self._run(report=report)
        self.assertEqual(report, before)

    # -- RF-EI v1.4: pins, inconsistent verifier output and split-object shape (D2) ------------------------

    def test_a_lone_cr_in_a_pinned_file_is_a_pin_mismatch(self):
        # only CRLF -> LF is normalised; a CR that is not a line end changes the pinned content
        self._write(LEDGER, b'{\r"fixture": "ledger"}\n')
        self._fails(r"pin mismatch: ledger")
        self.assertEqual(self.calls, [])

    def test_inconsistent_receipt_or_report_fields_are_refused(self):
        cases = (
            (r"receipt result", lambda r, p: r.update(result="FAIL-CLOSED")),
            (r"receipt fail_closed", lambda r, p: r.update(fail_closed={"class": "unreadable-input"})),
            (r"receipt schema", lambda r, p: r.update(schema="common-pool-verify-receipt/2")),
            (r"report result", lambda r, p: p.update(result="FAIL-CLOSED")),
            (r"report exit code", lambda r, p: p.update(exit_code=0)),
            (r"unverified libraries", lambda r, p: r["xdk_libraries"].update(libraries={})),
            (r"rows must name distinct records", lambda r, p: p["records"].append(dict(p["records"][0]))),
            (r"not a qualifying PASS: _halo_a", lambda r, p: p["records"][0].update(reasons=["no-definer:absent"])),
            (r"not a qualifying PASS: _halo_a", lambda r, p: p["records"][0].update(category="vend")),
        )
        for pattern, mutate in cases:
            with self.subTest(pattern=pattern):
                self._bound_fails(pattern, mutate)

    def test_split_objects_outside_the_one_record_per_section_shape_are_refused(self):
        cases = (
            ("symbol not at offset 0", {"symbol_changes": {"_halo_a": {"value": 4}}},
             r"not one external symbol at the start of its section: _halo_a"),
            ("two externals in one section", {"extra_symbols": [{"name": "_halo_x", "section": 1, "value": 2}]},
             r"does not own one reported data section: _halo_a"),
            ("initialised data section", {"section_changes": {1: {"flags": 0xC0400040}}},
             r"does not match the split object: _halo_a"),
            ("split size differs, same share", {"section_changes": {1: {"size": 5}}},
             r"does not match the split object: _halo_a"),
        )
        for name, variant, pattern in cases:
            with self.subTest(name=name):
                self._write("build/split/pool.obj", _pool_object(**variant))
                self._fails(pattern)
        self._write("build/split/pool.obj", self.target)


class CommonPoolWiringTests(unittest.TestCase):
    """RF-EI v1.4: the progress wiring (tools/project_x86.py) that the hook tests cannot see."""

    def test_progress_clears_first_and_credits_once_after_the_reassignment(self):
        source = inspect.getsource(project_x86.calculate_progress)
        order = [source.index(name + "(") for name in (
            "clear_common_pool_outputs", "apply_data_category_reassignments", "apply_common_pool_credit")]
        self.assertEqual(order, sorted(order))
        self.assertEqual(source.count("apply_common_pool_credit("), 1)

    def test_the_progress_edge_does_not_depend_on_the_entry_or_the_verifier(self):
        source = inspect.getsource(project_x86.generate_build_ninja)
        edge = source[source.index('outputs="progress"'):]
        edge = edge[:edge.index("n.newline()")]
        for name in ('"common_pool_credit.json"', '"common_pool_verify.py"'):
            self.assertNotIn(name, edge)

    def test_progress_prints_every_credit_note(self):
        # Q10 addition b: the credit is two notes (the total, then bytes by tag and the largest record); progress
        # must print each one (RF-EK mutation K20: printing only the first survived every other test)
        source = inspect.getsource(project_x86.calculate_progress)
        self.assertIn("    for common_pool_credit in common_pool_credits:\n", source)


ROOT = Path(__file__).resolve().parents[1]
REAL_ENTRY = ROOT / ENTRY

# Owner ruling Q10 FINAL (2026-09-28): the reviewed 85-record allowlist is frozen. Records that pass later earn nothing
# until a separate approval lists them; changing the list or the amount is a reviewed change to this test as well as to
# the entry. The frozen 85 stay the first 85 entry records, unchanged.
Q10_FINAL_ALLOWLIST_SHA256 = "62b51a00121e7445f836c4b453f15d9feb50f6b728beb09d657b487c639a943f"
# Allowlist extension EXT1 (separate owner approval; RF-EV PACKET_ALLOWLIST_EXT.md): exactly these records follow the
# frozen 85, in pool order, and nothing else is listed.
Q10_EXT1_SYMBOLS = ("_sound_class_data", "_debug_sound_cache", "_assertion_count", "_debug_sound_reference_counts",
                    "_wind_globals", "_build_sprite_globals", "_light_data", "_glow_globals")
Q10_EXT1_RECORDS_SHA256 = "9d7655b062d93139a91fae1402e217cb3df999ef9b9c1b0d89f5de684027828f"
# Qualifying PASS records that stay unlisted by their own rulings:
# - _debug_sound_channels (Q-B2: "Keep it outside Q10's credit allowlist")
# - _director_camera_scripted (B7: "remain outside the Q10 allowlist")
Q10_UNLISTED_BY_RULING = ("_debug_sound_channels", "_director_camera_scripted")


def _canonical_sha256(records):
    return hashlib.sha256(json.dumps(records, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()


def _by_tag(records):
    by_tag = {}
    for record in records:
        item = by_tag.setdefault(record["tag"], [0, 0, 0])
        item[0] += 1
        item[1] += record["size"]
        item[2] += record["share"]
    return by_tag


@unittest.skipIf(not REAL_ENTRY.is_file(), "no config/common_pool_credit.json")
class CommonPoolAllowlistTests(unittest.TestCase):
    """The entry lists exactly the approved allowlist and amount (Q10 FINAL, then EXT1); no build is needed."""

    def setUp(self):
        self.entry = json.loads(REAL_ENTRY.read_text(encoding="utf-8"))
        self.records = self.entry["records"]

    def test_the_first_85_records_are_the_frozen_q10_final_allowlist(self):
        final = self.records[:85]
        self.assertEqual(_canonical_sha256(final), Q10_FINAL_ALLOWLIST_SHA256)
        self.assertEqual(_by_tag(final), {"inferred": [60, 8220, 8444], "probable": [25, 649025, 649112]})
        self.assertEqual((len(final), sum(r["size"] for r in final), sum(r["share"] for r in final)),
                         (85, 657245, 657556))

    def test_ext1_lists_exactly_the_approved_records_after_them(self):
        extension = self.records[85:]
        self.assertEqual(tuple(record["symbol"] for record in extension), Q10_EXT1_SYMBOLS)
        self.assertEqual(_canonical_sha256(extension), Q10_EXT1_RECORDS_SHA256)
        self.assertEqual(_by_tag(extension), {"inferred": [4, 3392, 3440], "probable": [4, 8, 16]})

    def test_the_entry_amount_and_disclosures(self):
        entry, records = self.entry, self.records
        self.assertEqual(entry["credit"], {"records": 93, "raw": 660645, "padding": 367, "total": 661012})
        self.assertEqual(_by_tag(records), {"inferred": [64, 11612, 11884], "probable": [29, 649033, 649128]})
        largest = max(records, key=lambda record: record["share"])
        self.assertEqual((largest["symbol"], largest["tag"], largest["size"], largest["share"]),
                         ("_render", "probable", 643732, 643744))
        listed = {record["symbol"] for record in records}
        self.assertEqual(sorted(listed & set(Q10_UNLISTED_BY_RULING)), [])
        self.assertEqual(entry["linked_libraries_not_in_extract"], ["binkxbox.lib", "dsstrmh.lib"])
        self.assertEqual(entry["scope"], "This lifts Q10 only for those 85 records and the 8 records of allowlist "
                                         "extension EXT1 under this verification contract. It does not certify "
                                         "January ownership, complete pool layout, or a matching executable link.")


def _real_prerequisites():
    if not REAL_ENTRY.is_file():
        return "no config/common_pool_credit.json"
    for relative in ("build/report.json", "build/base", "build/split/source/linker_common.obj", "objdiff.json"):
        if not (ROOT / relative).exists():
            return f"no {relative} (build first)"
    entry = json.loads(REAL_ENTRY.read_text(encoding="utf-8"))
    if not Path(entry["xdk_lib_dir"]).is_dir():
        return f"XDK library directory {entry['xdk_lib_dir']} is absent"
    return None


def _convert_numbers(report):
    for measures in [report["measures"]] + [c["measures"] for c in report.get("categories", [])]:
        for key, value in list(measures.items()):
            if isinstance(value, str) and value.isdigit():
                measures[key] = int(value)
    return report


@unittest.skipIf(_real_prerequisites(), _real_prerequisites() or "")
class RealPoolCreditTests(unittest.TestCase):
    """The real verifier (subprocess) on a private copy of this tree's build and pinned inputs."""

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.mini = Path(cls.tmp.name)
        cls.entry = json.loads(REAL_ENTRY.read_text(encoding="utf-8"))
        (cls.mini / "tools").mkdir()
        for name in ("common_pool_verify.py", "coff_compare.py"):
            shutil.copy2(ROOT / "tools" / name, cls.mini / "tools" / name)
        shutil.copytree(ROOT / "config", cls.mini / "config",
                        ignore=lambda d, names: [n for n in names if not n.endswith(".json")])
        for key in ("ledger", "xdk_manifest"):
            target = cls.mini / cls.entry[key]["path"]
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / cls.entry[key]["path"], target)
        (cls.mini / "build").mkdir()
        shutil.copytree(ROOT / "build" / "base", cls.mini / "build" / "base")
        shutil.copytree(ROOT / "build" / "split", cls.mini / "build" / "split")
        shutil.copy2(ROOT / "objdiff.json", cls.mini / "objdiff.json")
        cls.report_text = (ROOT / "build" / "report.json").read_text(encoding="utf-8")
        cls.pristine = {}
        for relative in (ENTRY, cls.entry["ledger"]["path"], cls.entry["xdk_manifest"]["path"], READER):
            cls.pristine[relative] = (cls.mini / relative).read_bytes()

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def setUp(self):
        for relative, data in self.pristine.items():
            (self.mini / relative).write_bytes(data)
        self.restore = []

    def tearDown(self):
        for path, data in self.restore:
            path.write_bytes(data)

    def _report(self):
        report = _convert_numbers(json.loads(self.report_text))
        apply_data_category_reassignments(report, self.mini, self.mini / "config/data_category_reassignments.json",
                                          self.mini / "objdiff.json")
        return report

    def _run(self, report):
        return apply_common_pool_credit(
            report, self.mini, ENTRY, "objdiff.json", "config/semantic_data_matches.json",
            "config/data_category_reassignments.json", Path("build"))

    def _write_entry(self, entry):
        (self.mini / ENTRY).write_text(json.dumps(entry, indent=2) + "\n", encoding="utf-8")

    def _receipts(self):
        out = self.mini / OUT
        credit = json.loads((out / "credit_receipt.json").read_text(encoding="utf-8"))
        verify = out / "verify_receipt.json"
        return credit, (json.loads(verify.read_text(encoding="utf-8")) if verify.is_file() else None)

    def _fails(self, pattern):
        report = self._report()
        before = copy.deepcopy(report)
        with self.assertRaisesRegex(SemanticProgressError, pattern):
            self._run(report)
        self.assertEqual(report, before)
        credit, verify = self._receipts()
        self.assertEqual(credit["outcome"], "FAIL-CLOSED")
        return credit, verify

    def test_real_pool_credit_equals_the_entry(self):
        report = self._report()
        before = copy.deepcopy(report)
        notes = self._run(report)
        total = self.entry["credit"]["total"]
        self.assertEqual(report["measures"]["matched_data"], before["measures"]["matched_data"] + total)
        self.assertEqual(report["measures"]["total_data"], before["measures"]["total_data"])
        after = {c["id"]: c["measures"] for c in report["categories"]}
        prior = {c["id"]: c["measures"] for c in before["categories"]}
        halo = self.entry["category"]
        self.assertEqual(after[halo]["matched_data"], prior[halo]["matched_data"] + total)
        for category_id in after:
            self.assertEqual(after[category_id]["total_data"], prior[category_id]["total_data"])
            if category_id != halo:
                self.assertEqual(after[category_id], prior[category_id])
        credit, verify = self._receipts()
        self.assertEqual(credit["outcome"], "CREDITED")
        self.assertEqual([credit["credit"][k] for k in ("records", "raw", "padding", "total")],
                         [self.entry["credit"][k] for k in ("records", "raw", "padding", "total")])
        self.assertIn(verify["exit_code"], (0, 1))
        self.assertEqual(verify["result"], "COMPLETE")
        self.assertIn(f"+{total} {halo} data bytes", notes[0])
        # the reader pin: the tree's file, the reader this process loaded and the verifier's reader agree
        pin = self.entry["reader"]["sha256_lf"]
        reader = credit["pins"]["reader"]
        self.assertEqual((reader["expected"], reader["actual"], reader["loaded_by_hook"]["actual"],
                          reader["verifier_receipt"]), (pin, pin, pin, pin))
        self.assertEqual(verify["verifier"]["coff_compare_sha256_lf"], pin)
        self.assertEqual(_lf_sha((ROOT / READER).read_bytes()), pin)
        # bytes by tag and the largest record, as the entry lists them (Q10 addition b)
        by_tag = {}
        for record in self.entry["records"]:
            item = by_tag.setdefault(record["tag"], [0, 0, 0])
            item[0] += 1
            item[1] += record["share"]
            item[2] += record["size"]
        tags = ", ".join(f"{tag} {n} records +{share} ({raw} raw + {share - raw} padding)"
                         for tag, (n, share, raw) in sorted(by_tag.items()))
        largest = max(self.entry["records"], key=lambda record: record["share"])
        self.assertEqual(notes[1], f"{self.entry['unit']} by tag: {tags}; largest record {largest['symbol']} "
                                   f"({largest['tag']}) +{largest['share']} ({largest['size']} raw + "
                                   f"{largest['share'] - largest['size']} padding) = "
                                   f"{100.0 * largest['share'] / total:.1f}% of the credit")
        self.assertEqual(credit["linked_libraries_not_in_extract"], self.entry["linked_libraries_not_in_extract"])
        self.assertEqual(credit["linked_libraries_not_in_extract"], verify["xdk_libraries"]["linked_not_in_extract"])
        self.assertEqual(credit["linked_libraries_not_in_extract"], ["binkxbox.lib", "dsstrmh.lib"])
        self.assertEqual(credit["scope"], self.entry["scope"])

    def test_hook_pin_mismatch_stops_before_the_verifier(self):
        path = self.mini / self.entry["ledger"]["path"]
        path.write_bytes(path.read_bytes() + b" ")
        credit, verify = self._fails(r"pin mismatch: ledger")
        self.assertIsNone(verify)
        self.assertIsNone(credit["verifier_run"])

    def test_a_changed_reader_stops_before_the_verifier(self):
        path = self.mini / READER
        path.write_bytes(path.read_bytes() + b"\n# a changed reader\n")
        credit, verify = self._fails(r"pin mismatch: reader tools/coff_compare\.py")
        self.assertIsNone(verify)
        self.assertIsNone(credit["verifier_run"])

    def test_a_missing_reader_stops_before_the_verifier(self):
        (self.mini / READER).unlink()
        credit, verify = self._fails(r"cannot read pinned reader")
        self.assertIsNone(verify)
        self.assertIsNone(credit["verifier_run"])

    def test_a_reader_substituted_through_sys_path_in_the_verifier_fails_closed(self):
        # A regular 'tools' package anywhere on the search path takes precedence over the tree's namespace package
        # for `python -B -m tools.common_pool_verify`. Here it holds a byte-identical verifier and a changed reader:
        # every file the hook hashes is the pinned one, and only the verifier receipt's reader hash shows the swap.
        substitute = self.mini / "substitute"
        package = substitute / "tools"
        package.mkdir(parents=True)
        (package / "__init__.py").write_bytes(b"")
        shutil.copy2(self.mini / VERIFIER, package / "common_pool_verify.py")
        changed = (self.mini / READER).read_bytes() + b"\n# a substituted reader\n"
        (package / "coff_compare.py").write_bytes(changed)
        try:
            with mock.patch.dict(os.environ, {"PYTHONPATH": str(substitute)}):
                credit, verify = self._fails(r"not bound to the pins: reader sha256_lf")
        finally:
            shutil.rmtree(substitute)
        self.assertEqual(verify["verifier"]["coff_compare_sha256_lf"], _lf_sha(changed))
        self.assertEqual(verify["verifier"]["sha256_lf"], self.entry["verifier"]["sha256_lf"])
        self.assertEqual(credit["pins"]["reader"]["verifier_receipt"], _lf_sha(changed))
        self.assertEqual(credit["pins"]["reader"]["actual"], self.entry["reader"]["sha256_lf"])

    def test_missing_xdk_lib_dir_fails_closed(self):
        entry = copy.deepcopy(self.entry)
        entry["xdk_lib_dir"] = str(self.mini / "no_such_xdk_lib_dir")
        self._write_entry(entry)
        credit, verify = self._fails(r"exit 3 \(libraries-missing-or-unverifiable\)")
        self.assertEqual(verify["exit_code"], 3)
        self.assertEqual(credit["verifier_run"]["exit_code"], 3)

    def test_a_manifest_other_than_the_reviewed_one_fails_closed(self):
        # the entry's pin follows the edited manifest, so only the verifier's own mandatory pin can stop it
        path = self.mini / self.entry["xdk_manifest"]["path"]
        manifest = json.loads(path.read_text(encoding="utf-8"))
        manifest["status"] += " (edited)"
        path.write_text(json.dumps(manifest, indent=1) + "\n", encoding="utf-8")
        entry = copy.deepcopy(self.entry)
        entry["xdk_manifest"]["sha256_lf"] = _lf_sha(path.read_bytes())
        self._write_entry(entry)
        credit, verify = self._fails(r"exit 3 \(libraries-missing-or-unverifiable\).*not the reviewed manifest")
        self.assertEqual(verify["exit_code"], 3)

    def test_an_unreadable_object_fails_closed(self):
        victim = sorted((self.mini / "build" / "base").rglob("*.obj"))[0]
        self.restore.append((victim, victim.read_bytes()))
        victim.write_bytes(b"\x4c\x01\x01")
        credit, verify = self._fails(r"exit 2 \(unreadable-input\)")
        self.assertEqual(verify["exit_code"], 2)

    def test_a_malformed_ledger_fails_closed(self):
        path = self.mini / self.entry["ledger"]["path"]
        ledger = json.loads(path.read_text(encoding="utf-8"))
        ledger["records"][0]["tag"] = 5
        path.write_text(json.dumps(ledger, indent=1) + "\n", encoding="utf-8")
        entry = copy.deepcopy(self.entry)
        entry["ledger"]["sha256_lf"] = _lf_sha(path.read_bytes())
        self._write_entry(entry)
        credit, verify = self._fails(r"exit 4 \(malformed-types\)")
        self.assertEqual(verify["exit_code"], 4)

    def test_real_pool_is_never_credited_twice(self):
        report = self._report()
        self._run(report)
        credited = copy.deepcopy(report)
        with self.assertRaisesRegex(SemanticProgressError, r"no matched or complete data"):
            self._run(report)
        self.assertEqual(report, credited)


if __name__ == "__main__":
    unittest.main()
