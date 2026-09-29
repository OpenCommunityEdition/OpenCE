#!/usr/bin/env python3
"""Report-only per-record verifier for January's pooled COMMON block.

REPORT ONLY (owner rulings "Q10 mechanism", "Q10 fixes", "Q10 ledger home" and
"Q10 FINAL"): this tool itself never credits anything or writes progress.  The
progress hook (semantic_progress.apply_common_pool_credit) runs it on every run
and credits only the reviewed allowlist in config/common_pool_credit.json.

January's linker gathered every uninitialised, non-static file-scope global
(a *tentative* definition, COFF "COMMON") into one pool, which csplit exposes as
``source/linker_common``.  For every pooled record this tool checks the tree
under test against January, against a reviewed provenance ledger (a research
record, passed explicitly) and against the XDK libraries January linked:

* name     - csplit symbol == symbols.json name at the RVA == ledger name; the
             record is public (no ``"static": true``); name evidence recorded;
* size     - every COMMON definer requests exactly January's extent;
* slot     - January's alignment/slot obey the pool layout law, and the
             alignment our size would request equals January's;
* class    - the name exists in our objects only as COMMON (tentative) and
             UNDEF; any real (initialised/const/.bss), code, weak or static
             definition fails;
* definer  - exactly one COMMON definer object, in the record's category;
* provenance - the ledger's tag (inferred / probable / unresolved) is carried
             verbatim and may never exceed the recorded definer readouts;
             containment evidence and candidate owners never raise a tag; for
             inferred/probable records the definer must be the evidence owner
             and owners must respect January's pool order (a constraint that can
             refute, never prove);
* segment  - Halo / vendor classification is re-verified against the XDK
             libraries, whose bytes must match a recorded manifest.

Accounting keeps Halo (halobetacache) and each vendor category separate
(house rule 5) and excludes linker-generated records from every denominator.
Nothing here proves January's owner: "inferred" means "owner inferred from
later first-party builds (>=2 readouts), consistent with January".

Fail closed (owner ruling 2026-09-27, "Q10 fixes"): there is no ledger-only
mode.  Exit status: 0 complete, no failing record; 1 complete, failing records
or global failures; 2 unreadable input or usage error; 3 XDK libraries missing
or unverifiable; 4 malformed types; 5 ambiguous classification.  Runs 2-5
carry no accounting.  Every run writes a receipt (--receipt).  Independent
review RF-DP added: a repeated JSON key is malformed (4); a manifest whose
linked flags disagree with its January module counts, or that declares a
present library (or a listed library's category) absent, fails closed (4/3);
a missing or unreadable January split object, an absent object of a built
(non-MISSING) unit, an unreadable file anywhere, and inputs that change during
the run are unreadable input (2).  Independent review RF-DX added: any other
exception, and a report that cannot be written, end in exit 2 with a receipt
(the receipt is written last and always states the process exit status); an
unlistable library directory is exit 3; a vendor record whose name another
linked library also defines is ambiguous (5).  The XDK manifest is pinned
(XDK_MANIFEST_SHA256_LF): any other manifest is exit 3.
"""

import argparse
import datetime
import hashlib
import json
import re
import struct
import sys
from collections import Counter, OrderedDict
from pathlib import Path

try:  # package use (python -m tools.common_pool_verify, pytest)
    from .coff_compare import CoffError, load
except ImportError:  # pragma: no cover - direct script use
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from coff_compare import CoffError, load  # type: ignore


STATUS = ("report only; credits nothing itself (progress credits only the reviewed Q10 allowlist); "
          "provenance is inferred or weaker, never January-proven")

LEDGER_SCHEMA = "common-pool-ledger/2"
MANIFEST_SCHEMA = "xdk-lib-manifest/1"
RECEIPT_SCHEMA = "common-pool-verify-receipt/1"

EXIT_OK = 0
EXIT_FAILURES = 1
EXIT_UNREADABLE = 2
EXIT_LIBRARIES = 3
EXIT_MALFORMED = 4
EXIT_AMBIGUOUS = 5
FAIL_CLASSES = OrderedDict((
    (EXIT_UNREADABLE, "unreadable-input"),
    (EXIT_LIBRARIES, "libraries-missing-or-unverifiable"),
    (EXIT_MALFORMED, "malformed-types"),
    (EXIT_AMBIGUOUS, "ambiguous-classification"),
))

IMAGE_SCN_CNT_CODE = 0x00000020
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_SYM_CLASS_EXTERNAL = 2
IMAGE_SYM_CLASS_STATIC = 3

LINKER_SOURCE = "source/linker_common.c"
HALO_CATEGORY = "halobetacache"
SEGMENTS = ("halo", "vendor")

TAG_RANK = OrderedDict((("unresolved", 0), ("probable", 1), ("inferred", 2)))
TAG_LABELS = {
    "inferred": ("owner inferred from later first-party builds (>=2 readouts), "
                 "consistent with January; not January-proven"),
    "probable": ("one definer readout, or readouts with a recorded caveat; "
                 "not January-proven"),
    "unresolved": ("no usable definer readout, or readouts disagree; "
                   "ownership not established"),
}
# Readout channels and the evidence family each belongs to.  Two readouts from
# one family (HCEX GlobalRefs singleton X and HCEX real definition XD) are one
# observation.  XDK = the vendor library member that defines the COMMON.
# Pool containment (house rule 48) is NOT a channel: it is recorded separately
# ("containment_evidence") and never counts toward a tag.
CHANNEL_FAMILY = {"D": "D", "X": "X", "XD": "X", "OD": "OD", "L": "L",
                  "XDK": "XDK"}
SEGMENT_CHANNELS = {"halo": {"D", "X", "XD", "OD", "L"}, "vendor": {"XDK"}}
MAPPINGS = ("direct", "sibling")
VENDOR_SUPPORT = ("xdk-library-member-only",)
CONTAINMENT_KIND = "pool-containment"

# Failure reasons that contradict the ledger's Halo/vendor segment: such a
# record cannot be classified, and the whole run fails closed (exit 5).  Only
# January-side, library-side or ledger-side evidence can contradict a segment:
# facts about the tree under test (e.g. a definer in the wrong project or
# outside config.json) fail the record inside its segment and never move it
# out of a denominator.
SEGMENT_CONFLICTS = frozenset({
    "ledger-segment-invalid", "ledger-category-invalid", "ledger-duplicate-entry",
    "readout-channel-not-valid-for-segment", "owner-category-mismatch",
    "vendor-readout-library-not-category", "vendor-readout-not-reproduced",
    "vendor-segment-without-library-readout", "halo-record-defined-by-vendor-library",
    "segment-order-conflict", "duplicate-january-name",
    "vendor-evidence-disagrees-with-readout", "vendor-evidence-library-hash-mismatch",
    "vendor-evidence-on-halo-record",
    "vendor-record-defined-by-another-library",
})
IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105
IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040
IMAGE_SCN_MEM_EXECUTE = 0x20000000
IMAGE_SCN_MEM_WRITE = 0x80000000

VERDICT_PASS = "PASS"
VERDICT_PASS_UNRESOLVED = "PASS-OWNER-UNRESOLVED"
VERDICT_FAIL = "FAIL"

SHA256_RE = re.compile(r"^[0-9a-f]{64}$")

# The reviewed XDK manifest (RF-DM C02; recomputed 38/38 by RF-DP C02) records fixed facts: the XDK 3911
# extract's library hashes and January's PDB module table.  Its linked flags and absent declarations decide
# which libraries are scanned for Halo names and which vendor categories may lack a library, so a consistent
# edit could move the Halo/vendor boundary (RF-DX C05 T05/T06).  Its content is therefore pinned here: any
# other manifest is unverifiable library evidence (exit 3), and changing it is a verifier change (RF-DX, D2).
XDK_MANIFEST_SHA256_LF = "1b8a8350bd456bd61efea345970b793ca7ae8b554f9f63d5a0111571a47ebe3b"


class PoolVerifyError(RuntimeError):
    """Input that cannot be read (exit 2)."""
    exit_code = EXIT_UNREADABLE


class LibraryVerifyError(PoolVerifyError):
    """XDK libraries missing, unlisted, or not matching the recorded manifest (exit 3)."""
    exit_code = EXIT_LIBRARIES


class MalformedInputError(PoolVerifyError):
    """A JSON input with a wrong type, a missing required key or an unknown key (exit 4)."""
    exit_code = EXIT_MALFORMED


# -- small helpers -----------------------------------------------------------

def alignment_from_flags(flags):
    """COFF section alignment (IMAGE_SCN_ALIGN_*) in bytes; 0 if unspecified."""
    code = (int(flags) >> 20) & 0xF
    return 1 << (code - 1) if code else 0


def law_alignment(size):
    """Alignment VC7's linker gives a COMMON of *size* bytes in January's pool.

    min(32, smallest power of two >= size); holds for all 240 January records.
    """
    size = int(size)
    if size <= 0:
        raise ValueError("COMMON size must be positive")
    power = 1
    while power < size:
        power <<= 1
    return min(32, power)


def align_up(value, alignment):
    return (value + alignment - 1) // alignment * alignment if alignment else value


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def sha256_file(path):
    return sha256_bytes(Path(path).read_bytes())


def lf_normalised(data):
    return data.replace(b"\r\n", b"\n")


def sha256_file_lf(path):
    """sha256 of the file with CRLF normalised to LF (checkout-independent)."""
    return sha256_bytes(lf_normalised(Path(path).read_bytes()))


def _stem(path):
    return Path(str(path).replace("\\", "/")).stem.lower()


class DuplicateKeyError(ValueError):
    """A JSON object names the same key twice (Python would keep the last value silently)."""


def _no_duplicate_keys(pairs):
    seen = set()
    for key, _value in pairs:
        if key in seen:
            raise DuplicateKeyError(f"duplicate JSON key {key!r}")
        seen.add(key)
    return dict(pairs)


def _loads(text, description):
    """json.loads that fails closed on a repeated key (exit 4): a reviewer, another parser and
    this verifier must all read the same value (RF-DP D3)."""
    try:
        return json.loads(text, object_pairs_hook=_no_duplicate_keys)
    except DuplicateKeyError as exc:
        raise MalformedInputError(f"{description}: {exc}") from exc


def _read_json(path, description, error=PoolVerifyError):
    try:
        text = Path(path).read_text(encoding="utf-8")
    except (OSError, ValueError) as exc:
        raise error(f"cannot read {description} {path}: {exc}") from exc
    try:
        return _loads(text, f"{description} {path}")
    except MalformedInputError:
        raise
    except ValueError as exc:
        raise error(f"cannot read {description} {path}: {exc}") from exc


def file_fingerprint(path):
    """{path, sha256, sha256_lf, size} of a file, or {path, missing: true}."""
    path = Path(path)
    try:
        data = path.read_bytes()
    except OSError:
        return {"path": str(path), "missing": True}
    return {"path": str(path), "sha256": sha256_bytes(data),
            "sha256_lf": sha256_bytes(lf_normalised(data)), "size": len(data)}


def tree_manifest(root, pattern="*.obj"):
    """sha256sum-style manifest of every *pattern* file under *root* (sorted by
    relative POSIX path) and the sha256 of that manifest text."""
    root = Path(root)
    if not root.is_dir():
        return {"root": str(root), "missing": True}
    lines = []
    for path in sorted(root.rglob(pattern), key=lambda p: p.relative_to(root).as_posix()):
        if path.is_file():
            lines.append(f"{sha256_file(path)} *{path.relative_to(root).as_posix()}\n")
    text = "".join(lines).encode("utf-8")
    return {"root": str(root), "files": len(lines), "manifest_sha256": sha256_bytes(text),
            "format": "sha256sum lines '<sha256> *<relative path>', sorted by relative path"}


# -- JSON schema (keys + JSON types; fail closed) -----------------------------

def _kind_ok(value, kind):
    if kind == "str":
        return isinstance(value, str)
    if kind == "str?":
        return value is None or isinstance(value, str)
    if kind == "bool":
        return type(value) is bool
    if kind == "int":
        return type(value) is int
    if kind == "list":
        return isinstance(value, list)
    if kind == "list[str]":
        return isinstance(value, list) and all(isinstance(v, str) for v in value)
    if kind == "obj":
        return isinstance(value, dict)
    raise AssertionError(kind)


def _check_object(where, value, required, optional=None):
    """Keys and JSON types of one object: missing, unknown or mistyped keys are malformed."""
    optional = optional or {}
    if not isinstance(value, dict):
        raise MalformedInputError(f"{where}: expected an object, got {type(value).__name__}")
    missing = [key for key in required if key not in value]
    if missing:
        raise MalformedInputError(f"{where}: missing key(s) {missing}")
    unknown = sorted(set(value) - set(required) - set(optional))
    if unknown:
        raise MalformedInputError(f"{where}: unknown key(s) {unknown}")
    for key, kind in list(required.items()) + list(optional.items()):
        if key in value and not _kind_ok(value[key], kind):
            raise MalformedInputError(f"{where}: {key} must be {kind}, got "
                                      f"{type(value[key]).__name__} {value[key]!r:.60}")


LEDGER_TOP = {"schema": "str", "status": "str", "vocabulary": "obj", "derivation": "list[str]",
              "rulings": "list[str]", "records": "list"}
LEDGER_RECORD = {"name": "str", "rva": "str", "segment": "str", "category": "str",
                 "name_evidence": "str?", "tag": "str?", "owner": "str?",
                 "candidate_owner": "str?", "readouts": "list", "basis": "str",
                 "caveats": "list[str]", "sources": "list[str]"}
LEDGER_RECORD_OPTIONAL = {"containment_evidence": "list", "vendor_evidence": "obj"}
READOUT = {"channel": "str", "owner": "str", "mapping": "str", "counted": "bool", "source": "str"}
READOUT_OPTIONAL = {"names": "str", "library": "str", "member": "str", "note": "str"}
CONTAINMENT = {"kind": "str", "candidate_owner": "str", "lower_neighbours": "list[str]",
               "upper_neighbours": "list[str]", "neighbour_owner_evidence": "str",
               "ordering_model": "str", "veto_check": "str", "ruling": "str",
               "sources": "list[str]"}
VENDOR_EVIDENCE = {"support": "str", "library": "str", "library_sha256": "str", "member": "str",
                   "definition": "str", "size": "int", "january_public": "bool",
                   "january_pdb_module": "str?", "statement": "str"}
MANIFEST_TOP = {"schema": "str", "status": "str", "recorded_by": "str", "source_dir": "str",
                "evidence": "list[str]", "libraries": "list",
                "linked_libraries_not_in_extract": "list"}
MANIFEST_LIBRARY = {"file": "str", "sha256": "str", "size": "int", "linked_by_january": "bool",
                    "january_pdb_modules": "int"}
MANIFEST_ABSENT = {"library": "str", "category": "str?", "january_path": "str",
                   "january_pdb_modules": "int", "reason": "str"}


def validate_ledger(ledger):
    """Keys and JSON types of the whole ledger (MalformedInputError on the first defect).

    Vocabulary (tag / channel / mapping values, segment names) is NOT checked
    here: a value outside the vocabulary is a per-record failure reason.
    """
    _check_object("ledger", ledger, LEDGER_TOP)
    if ledger["schema"] != LEDGER_SCHEMA:
        raise MalformedInputError(f"ledger schema {ledger['schema']!r} != {LEDGER_SCHEMA!r}")
    for index, entry in enumerate(ledger["records"]):
        where = f"ledger record #{index}"
        _check_object(where, entry, LEDGER_RECORD, LEDGER_RECORD_OPTIONAL)
        where = f"ledger record #{index} ({entry['name']})"
        if not entry["name"]:
            raise MalformedInputError(f"{where}: empty name")
        for number, readout in enumerate(entry["readouts"]):
            _check_object(f"{where} readout #{number}", readout, READOUT, READOUT_OPTIONAL)
            if readout["channel"] == "XDK":
                if "library" not in readout or "member" not in readout or "names" in readout:
                    raise MalformedInputError(f"{where} readout #{number}: an XDK readout has "
                                              f"library + member and no names")
            elif readout["channel"] in CHANNEL_FAMILY:
                if "names" not in readout or "library" in readout or "member" in readout:
                    raise MalformedInputError(f"{where} readout #{number}: a {readout['channel']} "
                                              f"readout has names and no library/member")
        for number, item in enumerate(entry.get("containment_evidence", [])):
            _check_object(f"{where} containment #{number}", item, CONTAINMENT)
        if "vendor_evidence" in entry:
            _check_object(f"{where} vendor_evidence", entry["vendor_evidence"], VENDOR_EVIDENCE)


def validate_manifest(manifest):
    _check_object("XDK manifest", manifest, MANIFEST_TOP)
    if manifest["schema"] != MANIFEST_SCHEMA:
        raise MalformedInputError(f"XDK manifest schema {manifest['schema']!r} != {MANIFEST_SCHEMA!r}")
    seen = set()
    for index, entry in enumerate(manifest["libraries"]):
        _check_object(f"XDK manifest library #{index}", entry, MANIFEST_LIBRARY)
        if not SHA256_RE.match(entry["sha256"]):
            raise MalformedInputError(f"XDK manifest library {entry['file']}: sha256 must be 64 "
                                      f"lowercase hex digits")
        key = entry["file"].lower()
        if not key.endswith(".lib") or "/" in key or "\\" in key:
            raise MalformedInputError(f"XDK manifest library {entry['file']!r}: a bare .lib file name")
        if key in seen:
            raise MalformedInputError(f"XDK manifest lists {entry['file']} twice")
        seen.add(key)
        # the linked flag is January's PDB module table in another form (RF-DP D2): a library is
        # linked exactly when January's PDB lists modules from it
        # (types are the schema's job above; this compares values only)
        linked, modules = entry["linked_by_january"], entry["january_pdb_modules"]
        if type(linked) is bool and type(modules) is int and (linked != (modules > 0) or modules < 0):
            raise MalformedInputError(f"XDK manifest library {entry['file']}: linked_by_january "
                                      f"{entry['linked_by_january']} disagrees with january_pdb_modules "
                                      f"{entry['january_pdb_modules']}")
    for index, entry in enumerate(manifest["linked_libraries_not_in_extract"]):
        _check_object(f"XDK manifest absent library #{index}", entry, MANIFEST_ABSENT)
        if entry["library"].lower() in seen:
            raise MalformedInputError(f"XDK manifest: {entry['library']} is both present and absent")
        if entry["january_pdb_modules"] <= 0:
            raise MalformedInputError(f"XDK manifest: absent library {entry['library']} is declared linked by "
                                      f"January with {entry['january_pdb_modules']} PDB modules")


# -- configuration -------------------------------------------------------------

def load_config(config_path):
    """Map every config.json object to (project, January module index, status)."""
    config = _read_json(config_path, "config")
    objects = {}
    if not isinstance(config, dict) or not isinstance(config.get("projects"), list):
        raise MalformedInputError("config: 'projects' must be a list")
    for project in config["projects"]:
        if not isinstance(project, dict) or not isinstance(project.get("name"), str) \
                or not isinstance(project.get("objects"), list):
            raise MalformedInputError("config: every project needs a name and an objects list")
        for entry in project["objects"]:
            if not isinstance(entry, dict) or not isinstance(entry.get("name"), str) \
                    or type(entry.get("index")) is not int:
                raise MalformedInputError(f"config: malformed object entry in {project['name']}")
            name = entry["name"]
            if name in objects:
                raise MalformedInputError(f"config lists {name} twice")
            objects[name] = {"category": project["name"],
                             "index": entry["index"],
                             "status": entry.get("status")}
    linker = objects.get(LINKER_SOURCE)
    if linker is None:
        raise MalformedInputError(f"config does not list {LINKER_SOURCE}")
    return objects, linker["index"]


# -- January side ----------------------------------------------------------------

def _int_field(item, key, where):
    value = item.get(key) if isinstance(item, dict) else None
    if type(value) is not int:
        raise MalformedInputError(f"{where}: {key} must be an integer, got {value!r:.40}")
    return value


def january_pool(contribs_path, symbols_path, split_path, pool_module):
    """Read January's pool from contribs.json and cross-check csplit + symbols.json.

    Returns (records, global_failures).  Every record carries its own January
    self-consistency failures in ``january_reasons``.
    """
    contribs = _read_json(contribs_path, "contribs")
    symbols = _read_json(symbols_path, "symbols")
    if not isinstance(contribs, list) or not isinstance(symbols, list):
        raise MalformedInputError("contribs.json and symbols.json must be lists")
    by_offset = {}
    for item in symbols:
        offset = _int_field(item, "file_offset", "symbols.json")
        if not isinstance(item.get("name"), str):
            raise MalformedInputError(f"symbols.json entry at {offset:#x} has no string name")
        by_offset.setdefault(offset, []).append(item)
    for item in contribs:
        for key in ("file_offset", "size", "flags", "module_index"):
            _int_field(item, key, "contribs.json")
    pool = sorted((c for c in contribs if c["module_index"] == pool_module),
                  key=lambda c: c["file_offset"])
    if not pool:
        raise MalformedInputError(f"no contributions for the pool module {pool_module}")

    try:
        split = load(Path(split_path).read_bytes())
    except (OSError, CoffError) as error:
        raise PoolVerifyError(f"cannot read the split pool {split_path}: {error}") from error
    split_names = {}
    for item in split["symbols"]:
        if item["section"] > 0 and item["storage"] == IMAGE_SYM_CLASS_EXTERNAL:
            split_names.setdefault(item["section"], []).append(item["name"])

    failures = []
    if len(split["sections"]) != len(pool):
        failures.append(f"split pool has {len(split['sections'])} sections, "
                        f"January contributions {len(pool)}")
    records = []
    previous_bss = None
    for index, contribution in enumerate(pool):
        rva = contribution["file_offset"]
        size = contribution["size"]
        flags = contribution["flags"]
        reasons = []
        entries = by_offset.get(rva, [])
        names = [entry["name"] for entry in entries]
        if len(names) != 1:
            reasons.append(f"symbols.json names at rva: {names}")
        name = names[0] if names else None
        static = any(entry.get("static") for entry in entries)
        section = split["sections"][index] if index < len(split["sections"]) else None
        if section is None:
            reasons.append("no split section")
        else:
            if section["size"] != size:
                reasons.append(f"split size {section['size']} != contribution {size}")
            if section["flags"] != flags:
                reasons.append(f"split flags {section['flags']:#x} != contribution {flags:#x}")
            if split_names.get(section["index"]) != [name]:
                reasons.append(f"split symbol {split_names.get(section['index'])} != symbols.json {name}")
        bss = bool(flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
        alignment = alignment_from_flags(flags)
        # Only read-only initialised data (January: the PE debug directory and
        # its CodeView record) is excluded as linker-generated; any other
        # non-.bss pool contribution cannot be classified and fails the run
        # closed instead of being silently dropped from every denominator.
        read_only_data = (flags & IMAGE_SCN_CNT_INITIALIZED_DATA
                          and not flags & (IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE
                                           | IMAGE_SCN_CNT_CODE))
        kind = "bss" if bss else ("linker" if read_only_data else "unexpected")
        if kind == "unexpected":
            failures.append(f"unexpected-non-bss-pool-contribution: {name} rva {rva:#x} "
                            f"flags {flags:#x}")
        record = {"pool_index": index, "rva": rva, "size": size, "flags": flags,
                  "alignment": alignment, "name": name, "static_in_symbols": static,
                  "kind": kind, "gap_before": 0,
                  "january_reasons": reasons}
        if bss:
            if static:
                reasons.append("symbols.json marks a pooled record static")
            if size <= 0:
                reasons.append("non-positive size")
            else:
                if alignment != law_alignment(size):
                    reasons.append(f"january-alignment-law: flags align {alignment} "
                                   f"!= law {law_alignment(size)}")
                if not alignment or rva % alignment:
                    reasons.append(f"january-misaligned: rva {rva:#x} align {alignment}")
                if previous_bss is not None:
                    end = previous_bss["rva"] + previous_bss["size"]
                    expected = align_up(end, alignment)
                    if rva != expected:
                        reasons.append(f"january-slot-inconsistent: rva {rva:#x} != "
                                       f"align_up(previous end {end:#x}, {alignment}) {expected:#x}")
                    record["gap_before"] = max(0, rva - end)
            previous_bss = record
        records.append(record)
    counts = Counter(r["name"] for r in records if r["name"])
    for record in records:
        if counts.get(record["name"], 0) > 1:
            record["january_reasons"].append(
                f"duplicate-january-name: {record['name']} names {counts[record['name']]} pool records")
    # an excluded (linker) record is in no tally, but a csplit/contribs disagreement about it is
    # still a January-side inconsistency and must not pass silently (RF-DP D9)
    for record in records:
        if record["kind"] != "bss" and record["january_reasons"]:
            failures.append(f"january-excluded-record-inconsistent: {record['name']}: "
                            f"{'; '.join(record['january_reasons'])}")
    return records, failures


# -- our build ---------------------------------------------------------------------

def _object_path(objects_root, name):
    return Path(objects_root) / Path(name).with_suffix(".obj")


def scan_objects(objects_root, config_objects, pool_names):
    """Collect every appearance of a pooled name, and every COMMON, in our objects."""
    objects_root = Path(objects_root)
    if not objects_root.is_dir():
        raise PoolVerifyError(f"objects root {objects_root} is not a directory")
    known = {}
    stale, stale_paths = [], set()
    absent = []
    for name, info in sorted(config_objects.items()):
        path = _object_path(objects_root, name)
        if not path.is_file():
            # a unit that is not MISSING must have been built: its absent object could hide a
            # duplicate or real definition of a pooled name (RF-DP D8)
            if info["status"] != "MISSING":
                absent.append(name)
            continue
        if info["status"] == "MISSING":
            stale.append(name)
            stale_paths.add(path.resolve())
            continue
        known[path.resolve()] = (name, info["category"])
    if absent:
        raise PoolVerifyError(f"{len(absent)} built (non-MISSING) config unit(s) have no object under "
                              f"{objects_root}: {absent[:8]}{' ...' if len(absent) > 8 else ''}; an incomplete "
                              f"build cannot be verified")
    for path in sorted(objects_root.rglob("*.obj")):
        resolved = path.resolve()
        if resolved in known or resolved in stale_paths:
            continue
        rel = path.relative_to(objects_root).as_posix()
        known[resolved] = (rel, None)  # unknown category: fails closed if it defines

    commons, reals, statics, undefs = {}, {}, {}, {}
    all_commons = {}
    scanned = 0
    for path, (name, category) in sorted(known.items(), key=lambda kv: kv[1][0]):
        try:
            obj = load(path.read_bytes())
        except (OSError, CoffError) as error:
            raise PoolVerifyError(f"cannot read our object {path}: {error}") from error
        scanned += 1
        for item in obj["symbols"]:
            symbol_name = item["name"]
            section_no = item["section"]
            storage = item["storage"]
            if section_no == 0 and storage == IMAGE_SYM_CLASS_EXTERNAL:
                if item["value"] > 0:
                    all_commons.setdefault(symbol_name, []).append((name, category, item["value"]))
                    if symbol_name in pool_names:
                        commons.setdefault(symbol_name, []).append((name, category, item["value"]))
                elif symbol_name in pool_names:
                    undefs.setdefault(symbol_name, set()).add(name)
                continue
            if symbol_name not in pool_names:
                continue
            if storage == IMAGE_SYM_CLASS_WEAK_EXTERNAL:
                # an alias the linker may bind the pooled name to: never tentative storage
                reals.setdefault(symbol_name, []).append((name, category, "weak external"))
                continue
            if section_no <= 0:
                continue
            if section_no > len(obj["sections"]):
                raise PoolVerifyError(f"cannot read our object {path}: symbol {symbol_name} names section "
                                      f"{section_no} of {len(obj['sections'])}")
            section = obj["sections"][section_no - 1]
            where = (name, category, section["name"])
            if section["flags"] & IMAGE_SCN_CNT_CODE:
                # an EXTERNAL function of the pooled name is a real definition the
                # link would bind the name to; a static function is TU-private
                if storage == IMAGE_SYM_CLASS_EXTERNAL:
                    reals.setdefault(symbol_name, []).append(where)
                continue
            if storage == IMAGE_SYM_CLASS_EXTERNAL:
                reals.setdefault(symbol_name, []).append(where)
            elif storage == IMAGE_SYM_CLASS_STATIC:
                statics.setdefault(symbol_name, []).append(where)
    return {"commons": commons, "reals": reals, "statics": statics,
            "undefs": undefs, "all_commons": all_commons, "scanned": scanned,
            "stale_missing_units": stale}


def january_referencers(split_root, config_objects, pool_names):
    """{pooled name: set(config object)} of January TUs whose csplit object relocates to it.

    January-internal evidence (like pool order): it can refute an "inferred"
    owner that never references its own record, never prove one.  csplit writes
    one object per config unit, so a missing or unreadable one is unreadable
    January input (PoolVerifyError, exit 2): skipping it would silently drop the
    refutation it carries (RF-DP D7).
    """
    split_root = Path(split_root)
    found, unreadable = {}, []
    for name in sorted(config_objects):
        if name == LINKER_SOURCE:
            continue
        path = _object_path(split_root, name)
        if not path.is_file():
            unreadable.append(f"{name}: no January split object {path}")
            continue
        try:
            obj = load(path.read_bytes())
        except (OSError, CoffError) as error:
            unreadable.append(f"{name}: {error}")
            continue
        data = obj["data"]
        for section in obj["sections"]:
            for index in range(section["reloc_count"]):
                _address, target, _type = struct.unpack_from(
                    "<LLH", data, section["reloc"] + index * 10)
                symbol = obj["by_index"].get(target)
                if symbol is not None and symbol["name"] in pool_names:
                    found.setdefault(symbol["name"], set()).add(name)
    if unreadable:
        raise PoolVerifyError(f"{len(unreadable)} January split object(s) missing or unreadable under "
                              f"{split_root}: {unreadable[:5]}{' ...' if len(unreadable) > 5 else ''}")
    return found, unreadable


# -- XDK libraries (required; bytes pinned by the recorded manifest) ------------

def read_lib_members(path_or_bytes, library="archive"):
    """Yield (member name, bytes) of every member of a COFF archive (strict)."""
    data = path_or_bytes if isinstance(path_or_bytes, (bytes, bytearray)) \
        else Path(path_or_bytes).read_bytes()
    if data[:8] != b"!<arch>\n":
        raise LibraryVerifyError(f"{library} is not a COFF archive")
    offset, longnames = 8, b""
    while offset < len(data):
        if offset + 60 > len(data) or data[offset + 58:offset + 60] != b"`\n":
            raise LibraryVerifyError(f"{library}: truncated or corrupt member header at {offset:#x}")
        header = data[offset:offset + 60]
        raw_name = header[:16].decode("latin-1").rstrip()
        try:
            size = int(header[48:58].decode("ascii").strip())
        except ValueError as error:
            raise LibraryVerifyError(f"{library}: bad member size at {offset:#x}") from error
        if offset + 60 + size > len(data):
            raise LibraryVerifyError(f"{library}: member at {offset:#x} extends past the archive end")
        body = data[offset + 60:offset + 60 + size]
        offset += 60 + size + (size & 1)
        if raw_name == "//":
            longnames = body
            continue
        if raw_name == "/" or raw_name == "/SYM64/":
            continue
        if raw_name.startswith("/") and raw_name[1:].isdigit():
            start = int(raw_name[1:])
            if start >= len(longnames):
                raise LibraryVerifyError(f"{library}: long member name offset {start} out of range")
            end = start
            while end < len(longnames) and longnames[end] not in (0, 10):
                end += 1
            member = longnames[start:end].decode("latin-1")
        else:
            member = raw_name.rstrip("/")
        yield member, body


def member_symbols(body, where="member"):
    """(kind, [(name, section, value, storage)]) of one archive member.

    Reads i386 and machine-independent (IMAGE_FILE_MACHINE_UNKNOWN, e.g.
    OLDNAMES aliases) COFF objects and short import descriptors.  A string
    table whose length field is 0 (CVTRES resource objects) is read as empty.
    Anything else (e.g. an anonymous LTCG object) is unreadable: LibraryVerifyError.
    """
    if len(body) >= 20 and body[:4] == b"\x00\x00\xff\xff":
        version = struct.unpack_from("<H", body, 4)[0]
        if version != 0:
            raise LibraryVerifyError(f"{where}: anonymous object version {version}: symbols unreadable")
        data_size = struct.unpack_from("<L", body, 12)[0]
        strings = body[20:20 + data_size]
        name = strings.split(b"\0", 1)[0].decode("latin-1")
        if not name:
            raise LibraryVerifyError(f"{where}: import descriptor without a name")
        return "import", [(name, -3, 0, IMAGE_SYM_CLASS_EXTERNAL),
                          ("__imp_" + name, -3, 0, IMAGE_SYM_CLASS_EXTERNAL)]
    if len(body) < 20:
        raise LibraryVerifyError(f"{where}: truncated COFF header")
    machine, _sections, _stamp, symbol_offset, symbol_count, optional_size, _chars = \
        struct.unpack_from("<HHLLLHH", body, 0)
    if machine not in (0x014C, 0x0000) or optional_size != 0:
        raise LibraryVerifyError(f"{where}: machine {machine:#06x} / optional header "
                                 f"{optional_size}: not a readable object")
    string_offset = symbol_offset + symbol_count * 18
    if string_offset > len(body):
        raise LibraryVerifyError(f"{where}: symbol table truncated")
    strtab = b""
    if string_offset + 4 <= len(body):
        length = struct.unpack_from("<L", body, string_offset)[0]
        if length >= 4:
            if string_offset + length > len(body):
                raise LibraryVerifyError(f"{where}: string table truncated")
            strtab = body[string_offset:string_offset + length]
        elif length != 0:
            raise LibraryVerifyError(f"{where}: invalid string table length {length}")
    elif string_offset != len(body):
        raise LibraryVerifyError(f"{where}: string table length field truncated")
    symbols = []
    index = 0
    while index < symbol_count:
        entry = symbol_offset + index * 18
        zeroes, name_offset = struct.unpack_from("<LL", body, entry)
        if zeroes == 0:
            if name_offset < 4 or name_offset >= len(strtab):
                raise LibraryVerifyError(f"{where}: symbol {index} long name offset out of range")
            end = strtab.find(b"\0", name_offset)
            if end < 0:
                raise LibraryVerifyError(f"{where}: symbol {index} name not terminated")
            name = strtab[name_offset:end].decode("latin-1")
        else:
            name = body[entry:entry + 8].rstrip(b"\0").decode("latin-1")
        value, section, _type, storage, aux = struct.unpack_from("<LhHBB", body, entry + 8)
        symbols.append((name, section, value, storage))
        index += 1 + aux
    return "coff", symbols


def verify_libraries(lib_dir, manifest_path, vendor_categories, trace):
    """Check the XDK library directory against the recorded manifest.

    Returns (linked library bytes {file: bytes}, manifest, sha256 by lower-case
    file name).  Raises LibraryVerifyError (exit 3) when the directory or the
    manifest is missing, a recorded library is missing or differs in sha256 or
    size, the directory holds an unlisted .lib, or a vendor category of
    config.json has no library in the manifest.
    """
    if lib_dir is None:
        raise LibraryVerifyError("--xdk-lib-dir is required for every accounting run "
                                 "(owner ruling 2026-09-27: no ledger-only fallback)")
    if manifest_path is None:
        raise LibraryVerifyError("--xdk-lib-manifest is required: library bytes are verified "
                                 "against a recorded manifest")
    manifest_path, lib_dir = Path(manifest_path), Path(lib_dir)
    try:
        raw = manifest_path.read_bytes()
    except OSError as error:
        raise LibraryVerifyError(f"cannot read the XDK manifest {manifest_path}: {error}") from error
    try:
        manifest = _loads(raw.decode("utf-8"), f"the XDK manifest {manifest_path}")
    except ValueError as error:
        raise LibraryVerifyError(f"the XDK manifest {manifest_path} is not JSON: {error}") from error
    validate_manifest(manifest)
    pinned, actual = XDK_MANIFEST_SHA256_LF, sha256_bytes(lf_normalised(raw))
    if pinned is not None and actual != pinned:
        raise LibraryVerifyError(f"the XDK manifest {manifest_path} is not the reviewed manifest: LF sha256 "
                                 f"{actual[:16]}.. != pinned {pinned[:16]}.. (RF-DX D2 pin)")
    if not lib_dir.is_dir():
        raise LibraryVerifyError(f"--xdk-lib-dir {lib_dir} is not a directory")
    present = {}
    try:
        for path in lib_dir.iterdir():
            if path.is_file() and path.suffix.lower() == ".lib":
                if path.name.lower() in present:
                    # on a case-sensitive file system one of the two would never be hashed
                    raise LibraryVerifyError(f"{lib_dir} holds {present[path.name.lower()].name} and "
                                             f"{path.name}: library names that differ only in case")
                present[path.name.lower()] = path
    except OSError as error:
        # listing the library directory is library verification: exit 3, never 2 (RF-DX, D6)
        raise LibraryVerifyError(f"cannot list --xdk-lib-dir {lib_dir}: {error}") from error
    status, problems, linked, hashes = OrderedDict(), [], OrderedDict(), {}
    for entry in sorted(manifest["libraries"], key=lambda e: e["file"].lower()):
        key = entry["file"].lower()
        row = {"recorded_sha256": entry["sha256"], "recorded_size": entry["size"],
               "linked_by_january": entry["linked_by_january"]}
        path = present.get(key)
        if path is None:
            row.update(actual_sha256=None, verified=False)
            problems.append(f"missing library {entry['file']}")
        else:
            try:
                data = path.read_bytes()
            except OSError as error:
                raise LibraryVerifyError(f"cannot read library {path}: {error}") from error
            actual = sha256_bytes(data)
            row.update(actual_sha256=actual, actual_size=len(data),
                       verified=actual == entry["sha256"] and len(data) == entry["size"])
            if not row["verified"]:
                problems.append(f"library {entry['file']} sha256 {actual[:16]}.. size {len(data)} "
                                f"!= recorded {entry['sha256'][:16]}.. size {entry['size']}")
            elif entry["linked_by_january"]:
                linked[entry["file"]] = data
            hashes[key] = actual
        status[entry["file"]] = row
    recorded = {entry["file"].lower() for entry in manifest["libraries"]}
    for key in sorted(set(present) - recorded):
        try:
            unlisted_sha = sha256_file(present[key])
        except OSError as error:
            unlisted_sha = f"unreadable: {error}"
        status[present[key].name] = {"recorded_sha256": None,
                                     "actual_sha256": unlisted_sha, "verified": False}
        problems.append(f"unlisted library {present[key].name}: no recorded sha256")
    trace["xdk_libraries"] = {"dir": str(lib_dir), "libraries": status,
                              "linked_not_in_extract": [e["library"] for e in
                                                        manifest["linked_libraries_not_in_extract"]]}
    linked_stems = {entry["file"].lower()[:-4] for entry in manifest["libraries"]
                    if entry["linked_by_january"]}
    absent = {entry["category"] for entry in manifest["linked_libraries_not_in_extract"]
              if entry["category"]}
    for category in sorted(vendor_categories):
        if category.lower() not in linked_stems and category not in absent:
            problems.append(f"vendor category {category}: no linked library recorded in the manifest")
    # The linked set decides which libraries are scanned for Halo names, and a declared-absent
    # category may lack a library.  Neither may be used to hide a library that is present (RF-DP D2):
    # config.json's objects of a vendor category are January PDB modules of that library.
    listed_stems = {entry["file"].lower()[:-4] for entry in manifest["libraries"]}
    for category in sorted(vendor_categories):
        if category.lower() in listed_stems and category.lower() not in linked_stems:
            problems.append(f"vendor category {category}: {category.lower()}.lib is listed but not marked "
                            f"linked, although config.json lists January modules of this category")
    for entry in manifest["linked_libraries_not_in_extract"]:
        if entry["library"].lower() in present:
            problems.append(f"library {entry['library']} is declared not in the extract but is present "
                            f"in {lib_dir}")
        if entry["category"] and entry["category"].lower() in listed_stems:
            problems.append(f"category {entry['category']} is declared to have no library in the extract, "
                            f"but {entry['category'].lower()}.lib is listed in the manifest")
    if problems:
        raise LibraryVerifyError("; ".join(problems))
    return linked, manifest, hashes


def library_definers(libraries, names):
    """{name: [(library, member, kind, size)]} over the given library bytes.

    Every member must be readable (member_symbols); an unreadable member of a
    linked library fails the run closed (it could hide a definition).
    """
    found = {}
    members = {}
    for library, data in sorted(libraries.items(), key=lambda kv: kv[0].lower()):
        lib = library.lower()
        count = Counter()
        for member, body in read_lib_members(data, library):
            kind, symbols = member_symbols(body, f"{library}({member})")
            count[kind] += 1
            for name, section, value, storage in symbols:
                if name not in names:
                    continue
                if storage == IMAGE_SYM_CLASS_WEAK_EXTERNAL:
                    found.setdefault(name, []).append((lib, member, "WEAK", None))
                    continue
                if storage != IMAGE_SYM_CLASS_EXTERNAL:
                    continue
                if section == 0 and value > 0:
                    found.setdefault(name, []).append((lib, member, "COMMON", value))
                elif section > 0 or section in (-1, -3):  # section, absolute, import
                    found.setdefault(name, []).append((lib, member, "DEFINED", None))
        members[library] = dict(sorted(count.items()))
    return found, members


# -- ledger --------------------------------------------------------------------

def load_ledger(ledger_path):
    if ledger_path is None:
        raise PoolVerifyError("--ledger is required (the research ledger is passed explicitly)")
    ledger = _read_json(ledger_path, "ledger")
    validate_ledger(ledger)
    by_name, failures = {}, []
    duplicates = Counter()
    for entry in ledger["records"]:
        name = entry["name"]
        if name in by_name:
            # every entry of a duplicated name is distrusted: the record is
            # failed and cannot be classified (the run fails closed)
            failures.append(f"ledger lists {name} twice")
            duplicates[name] += 1
            continue
        by_name[name] = entry
    return ledger, by_name, failures, set(duplicates)


def _readouts(entry):
    """The entry's readouts (validated as a list of objects by validate_ledger)."""
    return entry.get("readouts") or []


def tag_ceiling(entry, segment):
    """Highest tag the recorded readouts can carry (never used to raise a tag).

    Only definer readouts count.  containment_evidence and candidate_owner are
    never read here: pool containment supports a candidate, never a tag.
    """
    owner = entry.get("owner")
    direct, any_family = set(), set()
    for readout in _readouts(entry):
        if readout.get("counted") is not True:  # the boolean true only
            continue
        channel = readout.get("channel")
        if not isinstance(channel, str) or channel not in SEGMENT_CHANNELS.get(segment, set()):
            continue
        if owner is None or readout.get("owner") != owner:
            continue
        family = CHANNEL_FAMILY[channel]
        any_family.add(family)
        if readout.get("mapping") == "direct":
            direct.add(family)
    if len(direct) >= 2:
        return "inferred"
    if any_family:
        return "probable"
    return "unresolved"


def _ledger_reasons(entry, record, config_objects, vendor_categories):
    reasons = []
    segment = entry.get("segment")
    category = entry.get("category")
    if segment not in SEGMENTS:
        reasons.append(f"ledger-segment-invalid: {segment!r}")
    elif segment == "halo" and category != HALO_CATEGORY:
        reasons.append(f"ledger-category-invalid: halo record in {category!r}")
    elif segment == "vendor" and category not in vendor_categories:
        reasons.append(f"ledger-category-invalid: vendor record in {category!r}")
    try:
        rva = int(entry.get("rva"), 0)
    except ValueError:
        rva = None
    if rva != record["rva"]:
        reasons.append(f"ledger-rva-mismatch: {entry.get('rva')} != {record['rva']:#x}")
    if not entry.get("name_evidence"):
        reasons.append("missing-name-evidence")
    tag = entry.get("tag")
    if tag is None:
        reasons.append("missing-provenance-tag")
    elif not isinstance(tag, str) or tag not in TAG_RANK:
        reasons.append(f"invalid-provenance-tag: {tag!r}")
        tag = None
    readouts = _readouts(entry)
    for readout in readouts:
        channel = readout.get("channel")
        if channel not in CHANNEL_FAMILY:
            reasons.append(f"readout-channel-unknown: {channel!r}")
            channel = None
        elif segment in SEGMENTS and channel not in SEGMENT_CHANNELS[segment]:
            reasons.append(f"readout-channel-not-valid-for-segment: {channel} on {segment}")
        mapping = readout.get("mapping")
        if mapping not in MAPPINGS:
            reasons.append(f"readout-mapping-invalid: {mapping!r}")
        # a direct readout names its owner itself: the raw name (Halo channels) or
        # the library member (XDK) must be the owner's own file
        if mapping == "direct" and channel and readout.get("owner"):
            raw = readout.get("member") if channel == "XDK" else readout.get("names")
            if not isinstance(raw, str) or _stem(raw) != _stem(readout["owner"]):
                reasons.append(f"readout-names-disagree-with-owner: {channel} {raw!r} "
                               f"-> {readout.get('owner')}")
    if segment == "vendor" and not any(r.get("channel") == "XDK" for r in readouts):
        reasons.append("vendor-segment-without-library-readout: a vendor record needs the library "
                       "member that defines it")
    if tag == "inferred" and entry.get("caveats"):
        reasons.append("tag-exceeds-evidence: an inferred record carries caveats")
    owner = entry.get("owner")
    if tag in ("inferred", "probable"):
        if not owner:
            reasons.append("missing-evidence-owner")
        elif owner not in config_objects:
            reasons.append(f"owner-not-in-config: {owner}")
        elif config_objects[owner]["category"] != category:
            reasons.append(f"owner-category-mismatch: {owner} is "
                           f"{config_objects[owner]['category']}, record {category}")
    if tag == "unresolved" and owner:
        reasons.append("unresolved-record-carries-owner")
    if tag in TAG_RANK and segment in SEGMENTS:
        ceiling = tag_ceiling(entry, segment)
        if TAG_RANK[tag] > TAG_RANK[ceiling]:
            reasons.append(f"tag-exceeds-evidence: {tag} > {ceiling}"
                           + (" (containment evidence never raises a tag)"
                              if entry.get("containment_evidence") else ""))
    for item in entry.get("containment_evidence", []):
        if item["kind"] != CONTAINMENT_KIND:
            reasons.append(f"containment-evidence-invalid: kind {item['kind']!r}")
        if item["candidate_owner"] != entry.get("candidate_owner"):
            reasons.append(f"containment-evidence-invalid: candidate {item['candidate_owner']} != "
                           f"record candidate {entry.get('candidate_owner')}")
    return reasons


def _vendor_evidence_reasons(entry, record, library_hashes):
    """Vendor records must record their library-member evidence explicitly
    (owner ruling 2026-09-27, "Q10 ledger home"); a Halo record carries none."""
    evidence = entry.get("vendor_evidence")
    if entry.get("segment") != "vendor":
        return ["vendor-evidence-on-halo-record: only vendor records carry library evidence"] \
            if evidence is not None else []
    if evidence is None:
        return ["vendor-evidence-missing: the XDK library member evidence must be recorded"]
    reasons = []
    if evidence["support"] not in VENDOR_SUPPORT:
        reasons.append(f"vendor-evidence-invalid: support {evidence['support']!r}")
    elif entry.get("tag") == "inferred":
        reasons.append("tag-exceeds-evidence: a vendor record supported only by an XDK library "
                       "member remains probable")
    xdk = [(str(r.get("library")).lower(), r.get("member"))
           for r in _readouts(entry) if r.get("channel") == "XDK"]
    if xdk != [(evidence["library"].lower(), evidence["member"])]:
        reasons.append(f"vendor-evidence-disagrees-with-readout: {evidence['library']} "
                       f"{evidence['member']} vs readouts {xdk}")
    recorded = library_hashes.get(evidence["library"].lower())
    if recorded != evidence["library_sha256"]:
        reasons.append(f"vendor-evidence-library-hash-mismatch: evidence {evidence['library_sha256'][:16]}.. "
                       f"verified {str(recorded)[:16]}..")
    if evidence["definition"] != "COMMON" or evidence["size"] != record["size"]:
        reasons.append(f"vendor-evidence-size-mismatch: {evidence['definition']}({evidence['size']}) "
                       f"!= January COMMON({record['size']})")
    return reasons


# -- verification ------------------------------------------------------------------

def _trace_inputs(trace, config_dir, split_path, ledger_path, manifest_path, objects_root):
    trace["inputs"] = {
        "config": file_fingerprint(config_dir / "config.json"),
        "contribs": file_fingerprint(config_dir / "contribs.json"),
        "symbols": file_fingerprint(config_dir / "symbols.json"),
        "split_pool": file_fingerprint(split_path),
        "ledger": file_fingerprint(ledger_path) if ledger_path else None,
        "xdk_manifest": file_fingerprint(manifest_path) if manifest_path else None,
    }
    trace["build_snapshot"] = {"objects": tree_manifest(objects_root),
                               "split": tree_manifest(split_path.parent.parent)}


def verify(root=".", objects_root=None, ledger_path=None, split_path=None,
           config_dir=None, xdk_lib_dir=None, xdk_manifest_path=None, trace=None):
    """Verify the tree against January, the ledger and the XDK libraries.

    Returns the report (result COMPLETE or FAIL-CLOSED for ambiguous
    classification).  Raises PoolVerifyError subclasses for unreadable input
    (exit 2), missing/unverifiable libraries (3) and malformed types (4).
    *trace* (a dict) receives the input fingerprints even when it raises.
    """
    trace = {} if trace is None else trace
    root = Path(root)
    config_dir = Path(config_dir) if config_dir else root / "config"
    objects_root = Path(objects_root) if objects_root else root / "build" / "base"
    split_path = Path(split_path) if split_path else root / "build" / "split" / "source" / "linker_common.obj"
    _trace_inputs(trace, config_dir, split_path, ledger_path, xdk_manifest_path, objects_root)

    config_objects, pool_module = load_config(config_dir / "config.json")
    vendor_categories = {info["category"] for info in config_objects.values()} - {HALO_CATEGORY}
    ledger, ledger_by_name, ledger_failures, ledger_duplicates = load_ledger(ledger_path)
    linked, manifest, library_hashes = verify_libraries(xdk_lib_dir, xdk_manifest_path,
                                                        vendor_categories, trace)
    records, global_failures = january_pool(config_dir / "contribs.json",
                                            config_dir / "symbols.json",
                                            split_path, pool_module)
    global_failures.extend(ledger_failures)

    pool_names = {r["name"] for r in records if r["name"]}
    bss_names = {r["name"] for r in records if r["kind"] == "bss" and r["name"]}
    linker_names = {r["name"] for r in records if r["kind"] == "linker" and r["name"]}
    for name in sorted(set(ledger_by_name) & linker_names):
        global_failures.append(f"linker-record-in-ledger: {name}")
    for name in sorted(set(ledger_by_name) - pool_names):
        global_failures.append(f"ledger-entry-not-in-pool: {name}")
    unknown_vendor = sorted({e["category"] for e in ledger_by_name.values()
                             if e["segment"] == "vendor" and e["category"] in vendor_categories
                             and e["category"].lower() + ".lib" not in {f.lower() for f in linked}})
    if unknown_vendor:
        raise LibraryVerifyError(f"ledger vendor records of {unknown_vendor} need a linked library "
                                 f"present in the XDK extract")

    ours = scan_objects(objects_root, config_objects, pool_names)
    for name, where in sorted(ours["all_commons"].items()):
        if name not in bss_names:
            global_failures.append(f"surplus-common: {name} in {[w[0] for w in where]}")

    library, library_members = library_definers(linked, bss_names)

    # January referencers (csplit relocations) refute an "inferred" owner that
    # never references its own record (RF-CP's PROVEN condition); they never prove one.
    # A missing or unreadable split object raises (exit 2) instead of becoming a global failure.
    referencers, _unreadable = january_referencers(split_path.parent.parent, config_objects, bss_names)

    # January links Halo objects before the vendor libraries, so its pool holds
    # every Halo record before the first vendor record.  Checked symmetrically:
    # every record between the first vendor label and the last Halo label is in
    # conflict, whichever of the two labels is wrong.
    labels = {}
    for record in records:
        if record["kind"] == "bss" and isinstance(ledger_by_name.get(record["name"]), dict):
            labels[record["pool_index"]] = ledger_by_name[record["name"]].get("segment")
    halo_at = [i for i, s in labels.items() if s == "halo"]
    vendor_at = [i for i, s in labels.items() if s == "vendor"]
    order_conflict = (min(vendor_at), max(halo_at)) \
        if halo_at and vendor_at and min(vendor_at) < max(halo_at) else None

    rows, excluded = [], []
    last_owner_index, last_owner_name = -1, None
    for record in records:
        if record["kind"] != "bss":
            excluded.append({"pool_index": record["pool_index"], "name": record["name"],
                             "rva": f"{record['rva']:#x}", "size": record["size"],
                             "reason": ("linker-generated (not a COMMON record); excluded "
                                        "from every denominator" if record["kind"] == "linker" else
                                        "UNEXPECTED non-.bss pool contribution (cannot be classified)"),
                             "january_reasons": record["january_reasons"]})
            continue
        name = record["name"]
        entry = ledger_by_name.get(name)
        reasons = list(record["january_reasons"])
        notes = []
        if entry is None:
            reasons.append("no-ledger-entry")
            entry = {}
            segment, category, tag, owner = None, None, None, None
        else:
            segment, category = entry["segment"], entry["category"]
            tag, owner = entry["tag"], entry["owner"]
            reasons.extend(_ledger_reasons(entry, record, config_objects, vendor_categories))
            reasons.extend(_vendor_evidence_reasons(entry, record, library_hashes))
        if name in ledger_duplicates:
            reasons.append("ledger-duplicate-entry: more than one ledger entry names this record")
        if segment == "halo" and tag == "inferred" and owner and referencers.get(name) \
                and owner not in referencers[name]:
            reasons.append(f"tag-exceeds-evidence: inferred owner {owner} is not a January referencer "
                           f"({len(referencers[name])} referencing TUs)")

        definers = ours["commons"].get(name, [])
        real = ours["reals"].get(name, [])
        static = ours["statics"].get(name, [])
        undef = sorted(ours["undefs"].get(name, set()))
        for obj, obj_category, section in real:
            reasons.append(f"real-definition: {obj} ({section}); an initialised or real "
                           f"definition is not tentative storage")
        for obj, obj_category, section in static:
            reasons.append(f"static-definition: {obj} ({section})")
        if len(definers) == 0:
            reasons.append("no-definer:extern-only" if undef else "no-definer:absent")
        elif len(definers) > 1:
            reasons.append(f"duplicate-definers: {[d[0] for d in definers]}")
        for obj, obj_category, size in definers:
            if size != record["size"]:
                reasons.append(f"size-mismatch: {obj} COMMON({size}) != January {record['size']}")
                if law_alignment(size) != record["alignment"]:
                    reasons.append(f"alignment-class-differs: {obj} would align "
                                   f"{law_alignment(size)}, January {record['alignment']}")
            if obj_category is None:
                reasons.append(f"definer-category-unknown: {obj}")
            elif category and obj_category != category:
                reasons.append(f"definer-category-mismatch: {obj} is {obj_category}, "
                               f"record {category}")
        if tag in ("inferred", "probable") and owner and len(definers) == 1 \
                and definers[0][0] != owner:
            reasons.append(f"definer-disagrees-with-evidence-owner: {definers[0][0]} != {owner}")

        # pool order may refute an owner, never prove one (house rule 48)
        if segment == "halo" and tag in ("inferred", "probable") and owner in config_objects:
            owner_index = config_objects[owner]["index"]
            if owner_index < last_owner_index:
                reasons.append(f"owner-violates-pool-order: {owner} (module {owner_index}) after "
                               f"{last_owner_name} (module {last_owner_index})")
            else:
                last_owner_index, last_owner_name = owner_index, owner

        if segment == "vendor":
            xdk = [r for r in _readouts(entry) if r.get("channel") == "XDK"]
            for readout in xdk:
                lib = str(readout.get("library", "")).lower()
                if lib[:-4] != str(category).lower() or not lib.endswith(".lib"):
                    reasons.append(f"vendor-readout-library-not-category: {readout.get('library')} "
                                   f"for {category}")
            hits = library.get(name, [])
            for readout in xdk:
                lib = str(readout.get("library", "")).lower()
                in_lib = [h for h in hits if h[0] == lib]
                if [(h[1], h[2], h[3]) for h in in_lib] != [(readout.get("member"), "COMMON", record["size"])]:
                    reasons.append(f"vendor-readout-not-reproduced: {lib} gives {in_lib}")
            # another linked library that also defines the name (COMMON, real or weak) makes the
            # record's library - its vendor category - ambiguous (RF-DX D11)
            own = {str(readout.get("library", "")).lower() for readout in xdk}
            others = [h for h in hits if h[0] not in own]
            if others:
                reasons.append(f"vendor-record-defined-by-another-library: {others}")
            notes.append("vendor readout re-verified against the manifest-verified library")
        elif segment == "halo" and library is not None and library.get(name):
            reasons.append(f"halo-record-defined-by-vendor-library: {library[name]}")

        if order_conflict and order_conflict[0] <= record["pool_index"] <= order_conflict[1]:
            reasons.append(f"segment-order-conflict: Halo and vendor labels interleave in January's "
                           f"pool (pool {order_conflict[0]}..{order_conflict[1]})")

        candidate = entry.get("candidate_owner")
        if tag == "unresolved" and candidate:
            notes.append(f"candidate owner {candidate} (not established; the tag stays unresolved); "
                         f"sole definer {'is' if [d[0] for d in definers] == [candidate] else 'is not'} "
                         f"the candidate")

        if reasons:
            verdict = VERDICT_FAIL
        elif tag == "unresolved":
            verdict = VERDICT_PASS_UNRESOLVED
        else:
            verdict = VERDICT_PASS
        # A record whose segment is invalid or contradicted by evidence cannot
        # be classified: it enters no tally and the run fails closed (exit 5).
        conflicted = any(reason.split(":")[0] in SEGMENT_CONFLICTS for reason in reasons)
        valid = ((segment == "halo" and category == HALO_CATEGORY)
                 or (segment == "vendor" and category in vendor_categories))
        accounted = segment if valid and not conflicted else None
        rows.append({
            "pool_index": record["pool_index"], "name": name, "rva": f"{record['rva']:#x}",
            "size": record["size"], "alignment": record["alignment"],
            "gap_before": record["gap_before"], "segment": accounted,
            "ledger_segment": segment, "category": category if accounted else None,
            "ledger_category": category,
            "tag": tag, "tag_label": TAG_LABELS.get(tag), "owner": owner,
            "candidate_owner": entry.get("candidate_owner"),
            "containment_evidence": len(entry.get("containment_evidence", [])),
            "verdict": verdict, "reasons": reasons, "notes": notes,
            "definers": [d[0] for d in definers], "referencers": len(undef),
        })

    ambiguous = [{"pool_index": row["pool_index"], "name": row["name"], "reasons": row["reasons"]}
                 for row in rows if row["segment"] is None]
    unexpected = [item for item in excluded if item["reason"].startswith("UNEXPECTED")]
    report = {
        "tool": "tools/common_pool_verify.py", "schema": 2, "status": STATUS,
        "inputs": {
            "config": str(config_dir / "config.json"),
            "contribs_sha256": trace["inputs"]["contribs"].get("sha256"),
            "symbols_sha256": trace["inputs"]["symbols"].get("sha256"),
            "split_pool": str(split_path), "split_pool_sha256": trace["inputs"]["split_pool"].get("sha256"),
            "ledger": str(ledger_path), "ledger_sha256": trace["inputs"]["ledger"]["sha256"],
            "ledger_sha256_lf": trace["inputs"]["ledger"]["sha256_lf"],
            "ledger_schema": ledger["schema"], "ledger_status": ledger["status"],
            "xdk_manifest": str(xdk_manifest_path),
            "xdk_manifest_sha256_lf": trace["inputs"]["xdk_manifest"]["sha256_lf"],
            "objects_root": str(objects_root), "objects_scanned": ours["scanned"],
            "stale_missing_units_ignored": ours["stale_missing_units"],
            "vendor_libraries_reverified": True,
        },
        "libraries": {
            "scanned_linked": library_members,
            "linked_not_in_extract": [
                {"library": e["library"], "category": e["category"], "reason": e["reason"]}
                for e in manifest["linked_libraries_not_in_extract"]],
        },
        "january": {
            "pool_module_index": pool_module, "records": len(records),
            "bss_records": sum(1 for r in records if r["kind"] == "bss"),
            "linker_records": sum(1 for r in records if r["kind"] == "linker"),
            "unexpected_records": sum(1 for r in records if r["kind"] == "unexpected"),
            "layout_reasons": sum(1 for r in records if r["january_reasons"]),
        },
        "excluded_linker_records": excluded,
        "global_failures": global_failures,
        "records": rows,
    }
    # the receipt fingerprints the inputs before they are parsed; a file replaced meanwhile (e.g. by
    # a concurrent build) would leave a receipt that does not describe the data used (RF-DP D5)
    again = {}
    _trace_inputs(again, config_dir, split_path, ledger_path, xdk_manifest_path, objects_root)
    changed = [key for key in ("inputs", "build_snapshot") if again[key] != trace[key]]
    if changed:
        raise PoolVerifyError(f"inputs changed during the run ({', '.join(changed)}); rerun on a quiet tree")
    if ambiguous or unexpected:
        report["result"] = "FAIL-CLOSED"
        report["exit_code"] = EXIT_AMBIGUOUS
        report["fail_closed"] = {"class": FAIL_CLASSES[EXIT_AMBIGUOUS], "exit_code": EXIT_AMBIGUOUS,
                                 "records": ambiguous, "unexpected_contributions": unexpected}
        report["accounting"] = None
        return report
    report["result"] = "COMPLETE"
    report["fail_closed"] = None
    report["accounting"] = tally(rows, records)
    report["accounting"]["segment_evidence"] = (
        "Halo/vendor segments re-verified against the XDK libraries January linked (bytes == recorded "
        f"manifest); linked libraries not in the extract: "
        f"{[e['library'] for e in manifest['linked_libraries_not_in_extract']] or 'none'}")
    failed = global_failures or any(r["verdict"] == VERDICT_FAIL for r in rows)
    report["exit_code"] = EXIT_FAILURES if failed else EXIT_OK
    return report


# -- accounting (house rule 5; linker records never counted) -------------------

def _bucket():
    return {"records": 0, "bytes": 0, "gap_bytes": 0,
            "by_tag": {tag: {"records": 0, "bytes": 0, "verdicts": {}} for tag in
                       list(TAG_RANK) + ["missing"]},
            "by_verdict": {}, "failure_reasons": {}}


def _add(bucket, row):
    bucket["records"] += 1
    bucket["bytes"] += row["size"]
    bucket["gap_bytes"] += row["gap_before"]
    tag = row["tag"] if isinstance(row["tag"], str) and row["tag"] in TAG_RANK else "missing"
    by_tag = bucket["by_tag"][tag]
    by_tag["records"] += 1
    by_tag["bytes"] += row["size"]
    verdict = by_tag["verdicts"].setdefault(row["verdict"], {"records": 0, "bytes": 0})
    verdict["records"] += 1
    verdict["bytes"] += row["size"]
    overall = bucket["by_verdict"].setdefault(row["verdict"], {"records": 0, "bytes": 0})
    overall["records"] += 1
    overall["bytes"] += row["size"]
    for reason in row["reasons"]:
        key = reason.split(":")[0]
        bucket["failure_reasons"][key] = bucket["failure_reasons"].get(key, 0) + 1


def tally(rows, january_records=None):
    """Separate Halo / vendor-category tallies.

    Raises ValueError if a linker record, an unclassified row, or a row whose
    segment and category disagree would enter a tally: such a row can never be
    counted anywhere (the run fails closed instead).
    """
    halo, vendor = _bucket(), {}
    seen = set()
    for row in rows:
        if row.get("kind") == "linker" or row.get("segment") == "linker":
            raise ValueError(f"linker record {row['name']} must not enter any tally")
        # one January pool record = one row (two records may share a name only
        # in a corrupt symbols.json, which fails them as duplicate-january-name)
        key = row.get("pool_index", row["name"])
        if key in seen:
            raise ValueError(f"{row['name']} would be counted twice")
        seen.add(key)
        segment, category = row.get("segment"), row.get("category")
        if segment == "halo" and category == HALO_CATEGORY:
            _add(halo, row)
        elif segment == "vendor" and category and category != HALO_CATEGORY:
            _add(vendor.setdefault(category, _bucket()), row)
        elif segment is None:
            raise ValueError(f"{row['name']} is unclassified; an ambiguous classification "
                             f"fails the run closed and is never tallied")
        else:
            raise ValueError(f"{row['name']}: segment {segment!r} and category "
                             f"{category!r} disagree; refusing to count it")
    accounting = {"halo": halo, "vendor": dict(sorted(vendor.items())),
                  "note": "bytes = January COMMON extents; gap_bytes = alignment padding "
                          "before each record; no credit, no progress percentage"}
    if january_records is not None:
        bss = [r for r in january_records if r["kind"] == "bss"]
        span = (bss[-1]["rva"] + bss[-1]["size"] - bss[0]["rva"]) if bss else 0
        counted = (halo["bytes"] + halo["gap_bytes"]
                   + sum(b["bytes"] + b["gap_bytes"] for b in vendor.values()))
        accounting["conservation"] = {
            "january_bss_span": span, "counted_bytes_plus_gaps": counted,
            "holds": span == counted,
            "linker_bytes_excluded": sum(r["size"] for r in january_records
                                         if r["kind"] == "linker"),
        }
    return accounting


# -- receipts ----------------------------------------------------------------------

def verifier_version():
    """sha256 of this verifier's source (raw and LF-normalised) and of its COFF reader."""
    own = Path(__file__).resolve()
    reader = own.parent / "coff_compare.py"
    return {"path": "tools/common_pool_verify.py", "sha256": sha256_file(own),
            "sha256_lf": sha256_file_lf(own),
            "coff_compare_sha256_lf": sha256_file_lf(reader) if reader.is_file() else None}


def summary(report):
    """Per-segment verdict counts of a complete report (records / January bytes)."""
    acc = report.get("accounting")
    if not acc:
        return None

    def verdicts(bucket):
        return {v: [d["records"], d["bytes"]] for v, d in sorted(bucket["by_verdict"].items())}

    return {"halo": {"records": acc["halo"]["records"], "bytes": acc["halo"]["bytes"],
                     "verdicts": verdicts(acc["halo"])},
            "vendor": {c: {"records": b["records"], "bytes": b["bytes"], "verdicts": verdicts(b)}
                       for c, b in acc["vendor"].items()},
            "linker_excluded": [[x["name"], x["size"]] for x in report["excluded_linker_records"]],
            "global_failures": len(report["global_failures"]),
            "conservation_holds": acc["conservation"]["holds"]}


def build_receipt(argv, trace, exit_code, report=None, error=None):
    fail = None
    if exit_code in FAIL_CLASSES:
        fail = {"class": FAIL_CLASSES[exit_code], "exit_code": exit_code,
                "message": str(error) if error is not None else None}
        if report is not None and report.get("fail_closed"):
            fail["records"] = [[r["name"], r["reasons"]] for r in report["fail_closed"]["records"]]
    return {
        "schema": RECEIPT_SCHEMA, "status": STATUS,
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
        "verifier": verifier_version(),
        "python": sys.version.split()[0],
        "argv": list(argv),
        "result": "COMPLETE" if exit_code in (EXIT_OK, EXIT_FAILURES) else "FAIL-CLOSED",
        "exit_code": exit_code,
        "fail_closed": fail,
        "inputs": trace.get("inputs"),
        "build_snapshot": trace.get("build_snapshot"),
        "xdk_libraries": trace.get("xdk_libraries"),
        "summary": summary(report) if report is not None else None,
    }


# -- text rendering ----------------------------------------------------------------

def _block_lines(title, bucket):
    lines = [f"{title}: {bucket['records']} records, {bucket['bytes']:,} B "
             f"(+{bucket['gap_bytes']:,} gap B)"]
    for tag, info in bucket["by_tag"].items():
        if not info["records"]:
            continue
        verdicts = ", ".join(f"{v} {d['records']}/{d['bytes']:,} B"
                             for v, d in sorted(info["verdicts"].items()))
        lines.append(f"  {tag:<10} {info['records']:>3} records {info['bytes']:>10,} B : {verdicts}")
    if bucket["failure_reasons"]:
        reasons = ", ".join(f"{k} {v}" for k, v in sorted(bucket["failure_reasons"].items()))
        lines.append(f"  failure reasons: {reasons}")
    return lines


def render_text(report, show_records=True):
    lines = [f"common_pool_verify - {report['status']}",
             f"result: {report['result']} (exit {report['exit_code']})",
             f"pool module {report['january']['pool_module_index']}: "
             f"{report['january']['records']} contributions = {report['january']['bss_records']} "
             f"COMMON records + {report['january']['linker_records']} linker records (excluded)",
             f"objects scanned {report['inputs']['objects_scanned']} under "
             f"{report['inputs']['objects_root']}; vendor libraries re-verified: "
             f"{report['inputs']['vendor_libraries_reverified']}"]
    acc = report["accounting"]
    if report["fail_closed"]:
        fail = report["fail_closed"]
        lines.append(f"FAIL-CLOSED ({fail['class']}): {len(fail['records'])} record(s) cannot be "
                     f"classified, {len(fail['unexpected_contributions'])} unexpected contribution(s); "
                     f"no accounting")
        lines += [f"  {r['name']}: {'; '.join(r['reasons'])}" for r in fail["records"]]
    else:
        lines.append(f"segment evidence: {acc.get('segment_evidence')}")
        lines += _block_lines("HALO (halobetacache)", acc["halo"])
        for category, bucket in acc["vendor"].items():
            lines += _block_lines(f"VENDOR {category}", bucket)
        c = acc["conservation"]
        lines.append(f"conservation: January .bss pool span {c['january_bss_span']:,} B == counted "
                     f"{c['counted_bytes_plus_gaps']:,} B: {c['holds']}; linker bytes excluded "
                     f"{c['linker_bytes_excluded']}")
    for item in report["excluded_linker_records"]:
        lines.append(f"EXCLUDED {item['name']} {item['rva']} {item['size']} B: {item['reason']}")
    lines.append(f"global failures: {len(report['global_failures'])}")
    lines += [f"  {g}" for g in report["global_failures"]]
    if show_records:
        for row in report["records"]:
            lines.append(f"{row['pool_index']:>3} {row['name']:<48} {row['size']:>7} "
                         f"{str(row['segment']):<6} {str(row['category']):<13} "
                         f"{str(row['tag']):<10} {row['verdict']:<21} "
                         f"{'; '.join(row['reasons'])}")
    return "\n".join(lines) + "\n"


def _remove_outputs(paths):
    """Best effort: a run that ends in exit 2 leaves no report (no accounting) behind."""
    for path in paths:
        try:
            path.unlink()
        except OSError:
            pass


def _write_reports(report, args, written):
    """Write the JSON report (receipt embedded) and the text report (or stdout); *written*
    collects every path this run has written."""
    if args.json:
        Path(args.json).write_text(json.dumps(report, indent=1) + "\n", encoding="utf-8")
        written.append(Path(args.json))
    text = render_text(report)
    if args.text:
        Path(args.text).write_text(text, encoding="utf-8")
        written.append(Path(args.text))
    else:
        sys.stdout.write(text)


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--root", default=".")
    parser.add_argument("--objects", help="object root under test (default <root>/build/base)")
    # --ledger, --xdk-lib-dir and --xdk-lib-manifest are required, but checked by
    # verify() rather than argparse so that a run without them still writes its
    # receipt and exits with its own fail-closed code (2 ledger, 3 libraries).
    parser.add_argument("--ledger",
                        help="REQUIRED: research provenance ledger (e.g. research/"
                             "remaining_frontier_20260926/q10/common_pool_ledger.json)")
    parser.add_argument("--split-pool", help="January split pool object")
    parser.add_argument("--config-dir", help="config directory (default <root>/config)")
    parser.add_argument("--xdk-lib-dir",
                        help="REQUIRED: XDK 3911 xbox/lib directory (no ledger-only mode)")
    parser.add_argument("--xdk-lib-manifest",
                        help="REQUIRED: recorded sha256/size manifest of that directory")
    parser.add_argument("--receipt", required=True, help="write the run receipt (JSON) here")
    parser.add_argument("--json", help="write the JSON report here")
    parser.add_argument("--text", help="write the text report here (default stdout)")
    args = parser.parse_args(argv)
    trace, report, error = {}, None, None
    try:
        report = verify(args.root, args.objects, args.ledger, args.split_pool,
                        args.config_dir, args.xdk_lib_dir, args.xdk_lib_manifest, trace=trace)
        exit_code = report["exit_code"]
    except PoolVerifyError as caught:
        error, exit_code = caught, caught.exit_code
    except (ValueError, TypeError, KeyError, AttributeError, struct.error) as caught:
        # malformed input must never pass for "failures found" (1) or be half-reported
        error, exit_code = caught, EXIT_MALFORMED
    except IndexError as caught:
        # a malformed structure indexed out of range: malformed input, receipted (RF-DP D6)
        exit_code, error = EXIT_MALFORMED, caught
    except OSError as caught:
        # a file that vanished, is locked or unreadable outside an explicit wrapper: unreadable
        # input (2), never the uncaught traceback exit status 1 = "complete, failing records" (RF-DP D6)
        error, exit_code = caught, EXIT_UNREADABLE
    except Exception as caught:  # deliberately total (RF-DX, D6)
        # any other exception (a verifier defect or an input shape nobody anticipated) is not a
        # complete run either: fail closed (2) with a receipt that names the type
        error = PoolVerifyError(f"unexpected {type(caught).__name__}: {caught}")
        exit_code = EXIT_UNREADABLE
    receipt = build_receipt(argv, trace, exit_code, report, error)
    written = []
    if report is not None:
        report["receipt"] = receipt
        try:
            _write_reports(report, args, written)
        except Exception as unwritable:  # deliberately total (RF-DX, D6)
            # a report that cannot be written (missing directory, unencodable text, ...) leaves the
            # run unreported: fail closed (2), drop what this run wrote (an exit-2 run carries no
            # accounting) and let the receipt, written last, say so
            _remove_outputs(written)
            error = PoolVerifyError(f"cannot write the report: {type(unwritable).__name__}: {unwritable}")
            exit_code, report = EXIT_UNREADABLE, None
            receipt = build_receipt(argv, trace, exit_code, None, error)
    if error is not None:
        print(f"common_pool_verify: FAIL-CLOSED ({FAIL_CLASSES[exit_code]}): "
              f"{type(error).__name__}: {error}", file=sys.stderr)
    try:
        Path(args.receipt).write_text(json.dumps(receipt, indent=1) + "\n", encoding="utf-8")
    except OSError as caught:
        _remove_outputs(written)  # no report outlives a run whose receipt could not be written (RF-DX)
        print(f"common_pool_verify: cannot write the receipt {args.receipt}: {caught}", file=sys.stderr)
        return EXIT_UNREADABLE
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
