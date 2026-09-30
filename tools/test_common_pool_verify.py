"""Tests for tools/common_pool_verify.py (report-only COMMON-pool verifier; it credits nothing itself).

A small synthetic world (January pool + csplit object + symbols/contribs/config
+ our objects + research ledger + XDK library directory with its recorded
manifest) exercises every failure reason and every fail-closed class; the
positive world must pass, and the real January pool (when present) must
reproduce the reviewed accounting.  Nothing here credits anything.

Every mutation of the RF-DE/RF-DH/RF-DM mutation catalogue
(research/remaining_frontier_20260926/q10/mutation/mutations.json) names the
regression tests below that fail when that mutation is applied.
"""

import copy
import hashlib
import json
import os
import struct
from pathlib import Path

import pytest

from tools.coff_compare import build_coff
from tools.common_pool_verify import (
    EXIT_AMBIGUOUS,
    EXIT_FAILURES,
    EXIT_LIBRARIES,
    EXIT_MALFORMED,
    EXIT_OK,
    EXIT_UNREADABLE,
    LibraryVerifyError,
    MalformedInputError,
    PoolVerifyError,
    TAG_RANK,
    VERDICT_FAIL,
    VERDICT_PASS,
    VERDICT_PASS_UNRESOLVED,
    XDK_MANIFEST_SHA256_LF as PINNED_MANIFEST,
    alignment_from_flags,
    law_alignment,
    main,
    member_symbols,
    read_lib_members,
    render_text,
    tag_ceiling,
    tally,
    verify,
)


REPO = Path(__file__).resolve().parents[1]
Q10 = Path("research") / "remaining_frontier_20260926" / "q10"
REPO_LEDGER = REPO / Q10 / "common_pool_ledger.json"
REPO_MANIFEST = REPO / Q10 / "xdk3911_lib_manifest.json"
MUTATIONS = REPO / Q10 / "mutation" / "mutations.json"
ALIGN_CODE = {1: 1, 2: 2, 4: 3, 8: 4, 16: 5, 32: 6}
LINKER_MODULE = 99
SHA_PLACEHOLDER = "@library-sha256@"


@pytest.fixture(autouse=True)
def _synthetic_manifests_are_not_pinned():
    """Synthetic worlds write their own XDK manifests: the verifier's manifest pin is switched off here and
    tested explicitly (test_*manifest_pin*, test_real_january_pool_accounting).  Not done with the monkeypatch
    fixture, whose undo() inside a test would switch the pin back on."""
    import tools.common_pool_verify as module
    saved, module.XDK_MANIFEST_SHA256_LF = module.XDK_MANIFEST_SHA256_LF, None
    yield
    module.XDK_MANIFEST_SHA256_LF = saved


def bss_flags(alignment):
    return 0xC0000080 | (ALIGN_CODE[alignment] << 20)


RDATA_FLAGS = 0x40000040

# January pool: one linker record, three Halo records, one vendor record.
POOL = [
    # name, rva, size, flags
    ("_rdata_00001000", 0x1000, 28, RDATA_FLAGS),
    ("_alpha", 0x2000, 4, bss_flags(4)),
    ("_beta", 0x2004, 1, bss_flags(1)),
    ("_gamma", 0x2010, 16, bss_flags(16)),
    ("_delta", 0x2020, 4, bss_flags(4)),
]

CONFIG = {
    "projects": [
        {"name": "halobetacache", "objects": [
            {"name": "source/a.c", "index": 10, "status": "NonMatching"},
            {"name": "source/b.c", "index": 20, "status": "NonMatching"},
            {"name": "source/c.c", "index": 30, "status": "Matching"},
            {"name": "source/linker_common.c", "index": LINKER_MODULE, "status": "MISSING"},
        ]},
        {"name": "libcmt", "objects": [
            {"name": "libs/libcmt/crt0dat.c", "index": 50, "status": "Matching"},
        ]},
    ]
}


def readout(channel, owner, names, mapping="direct", counted=True):
    return {"channel": channel, "names": names, "owner": owner, "mapping": mapping,
            "counted": counted, "source": "test"}


def record(name, rva, segment, category, tag, owner, readouts, **extra):
    entry = {"name": name, "rva": rva, "segment": segment, "category": category,
             "name_evidence": "test public", "tag": tag, "owner": owner, "candidate_owner": None,
             "readouts": readouts, "basis": "test", "caveats": [], "sources": ["test"]}
    entry.update(extra)
    return entry


VENDOR_EVIDENCE = {"support": "xdk-library-member-only", "library": "libcmt.lib",
                   "library_sha256": SHA_PLACEHOLDER, "member": "obj\\i386\\crt0dat.obj",
                   "definition": "COMMON", "size": 4, "january_public": False,
                   "january_pdb_module": "test module", "statement": "test"}
XDK_READOUT = {"channel": "XDK", "library": "libcmt.lib", "member": "obj\\i386\\crt0dat.obj",
               "owner": "libs/libcmt/crt0dat.c", "mapping": "direct", "counted": True,
               "source": "test"}
CONTAINMENT = {"kind": "pool-containment", "candidate_owner": "source/c.c",
               "lower_neighbours": ["_beta"], "upper_neighbours": ["_delta"],
               "neighbour_owner_evidence": "test", "ordering_model": "test", "veto_check": "test",
               "ruling": "test", "sources": ["test"]}

LEDGER = {
    "schema": "common-pool-ledger/2",
    "status": "test",
    "vocabulary": {},
    "derivation": ["test"],
    "rulings": [],
    "records": [
        record("_alpha", "0x2000", "halo", "halobetacache", "inferred", "source/a.c",
               [readout("D", "source/a.c", "a"), readout("X", "source/a.c", "a")]),
        record("_beta", "0x2004", "halo", "halobetacache", "probable", "source/b.c",
               [readout("D", "source/b.c", "b")]),
        record("_gamma", "0x2010", "halo", "halobetacache", "unresolved", None, [],
               candidate_owner="source/c.c"),
        record("_delta", "0x2020", "vendor", "libcmt", "probable", "libs/libcmt/crt0dat.c",
               [XDK_READOUT], name_evidence="test lib", vendor_evidence=VENDOR_EVIDENCE),
    ],
}


def common(name, size):
    return {"name": name, "value": size, "section": 0, "storage": 2}


def undef(name):
    return {"name": name, "value": 0, "section": 0, "storage": 2}


def defined(name, storage=2):
    return {"name": name, "value": 0, "section": 1, "storage": storage}


BSS_SECTION = {"name": ".bss", "size": 4, "flags": bss_flags(4)}
DATA_SECTION = {"name": ".data", "size": 4, "flags": 0xC0300040}
TEXT_SECTION = {"name": ".text", "size": 4, "flags": 0x60500020}

OURS = {
    "source/a.obj": ([], [common("_alpha", 4), undef("_beta")]),
    "source/b.obj": ([], [common("_beta", 1)]),
    "source/c.obj": ([], [common("_gamma", 16)]),
    "libs/libcmt/crt0dat.obj": ([], [common("_delta", 4)]),
}


def write_lib(path, members):
    """Minimal COFF archive: empty first linker member, longnames, members."""
    def header(name, size):
        return (name.ljust(16) + "0".ljust(12) + "".ljust(6) + "".ljust(6) + "0".ljust(8)
                + str(size).ljust(10) + "`\n").encode("latin-1")

    longnames = b""
    offsets = []
    for name, _ in members:
        offsets.append(len(longnames))
        longnames += name.encode("latin-1") + b"\0"
    out = bytearray(b"!<arch>\n")
    for name, body in [("/", b"\0\0\0\0"), ("//", longnames)]:
        out += header(name, len(body)) + body
        if len(body) & 1:
            out += b"\n"
    for (name, body), offset in zip(members, offsets):
        out += header(f"/{offset}", len(body)) + body
        if len(body) & 1:
            out += b"\n"
    path.write_bytes(bytes(out))


def delta_member(size=4, extra=()):
    return build_coff(sections=[], symbols=[common("_delta", size)] + list(extra))


def default_libs(delta_size=4, extra=()):
    return {"libcmt.lib": [("obj\\i386\\crt0dat.obj", delta_member(delta_size, extra))]}


def import_member(name):
    """Short import descriptor (sig1 0, sig2 0xFFFF, version 0) for *name*."""
    strings = name.encode("latin-1") + b"\0" + b"test.dll\0"
    return struct.pack("<HHHHLLHH", 0, 0xFFFF, 0, 0x14C, 0, len(strings), 0, 0) + strings


def anonymous_member():
    """Anonymous (LTCG) object header: sig1 0, sig2 0xFFFF, version 1."""
    return struct.pack("<HHHH", 0, 0xFFFF, 1, 0x14C) + b"\0" * 40


def write_world(root, pool=None, config=None, ledger=None, ours=None, symbols_extra=None,
                split_pool=None, libs=None, manifest=None, unlinked=()):
    pool = POOL if pool is None else pool
    config = CONFIG if config is None else config
    ledger = LEDGER if ledger is None else ledger
    ours = OURS if ours is None else ours
    libs = default_libs() if libs is None else libs
    root = Path(root)
    cfg = root / "config"
    cfg.mkdir(parents=True, exist_ok=True)
    (cfg / "config.json").write_text(json.dumps(config), encoding="utf-8")
    contribs = [{"file_offset": rva, "size": size, "flags": flags, "module_index": LINKER_MODULE,
                 "selection": 0} for name, rva, size, flags in pool]
    contribs.append({"file_offset": 0x100, "size": 64, "flags": 0x60000020, "module_index": 10,
                     "selection": 1})
    (cfg / "contribs.json").write_text(json.dumps(contribs), encoding="utf-8")
    symbols = [{"file_offset": rva, "flags": 0, "name": name} for name, rva, size, flags in pool]
    for item in symbols_extra or []:
        for entry in symbols:
            if entry["name"] == item["name"]:
                entry.update(item)
    (cfg / "symbols.json").write_text(json.dumps(symbols), encoding="utf-8")
    split_pool = pool if split_pool is None else split_pool
    sections = [{"name": ".rdata" if not flags & 0x80 else ".bss", "size": size, "flags": flags}
                for name, rva, size, flags in split_pool]
    symbols = [{"name": name, "value": 0, "section": index + 1, "storage": 2}
               for index, (name, rva, size, flags) in enumerate(split_pool)]
    split = root / "build" / "split" / "source" / "linker_common.obj"
    split.parent.mkdir(parents=True, exist_ok=True)
    split.write_bytes(build_coff(sections=sections, symbols=symbols))
    # csplit writes one January object per config unit (RF-DP D7): an empty one, no relocations
    for project in config["projects"]:
        for entry in project["objects"]:
            if entry["name"] == "source/linker_common.c":
                continue
            unit = root / "build" / "split" / Path(entry["name"]).with_suffix(".obj")
            unit.parent.mkdir(parents=True, exist_ok=True)
            unit.write_bytes(build_coff(sections=[], symbols=[]))
    base = root / "build" / "base"
    base.mkdir(parents=True, exist_ok=True)
    for rel, (secs, syms) in ours.items():
        path = base / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(build_coff(sections=secs, symbols=syms))
    lib_dir = root / "xdk"
    lib_dir.mkdir(parents=True, exist_ok=True)
    hashes = {}
    for name, members in libs.items():
        write_lib(lib_dir / name, members)
        hashes[name.lower()] = hashlib.sha256((lib_dir / name).read_bytes()).hexdigest()
    if manifest is None:
        manifest = {"schema": "xdk-lib-manifest/1", "status": "test", "recorded_by": "test",
                    "source_dir": str(lib_dir), "evidence": ["test"],
                    "libraries": [{"file": name, "sha256": hashes[name.lower()],
                                   "size": (lib_dir / name).stat().st_size,
                                   "linked_by_january": name not in unlinked,
                                   "january_pdb_modules": 0 if name in unlinked else 1}
                                  for name in sorted(libs)],
                    "linked_libraries_not_in_extract": []}
    research = root / Q10
    research.mkdir(parents=True, exist_ok=True)
    (research / "xdk_manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
    text = json.dumps(ledger)
    for name, sha in hashes.items():
        text = text.replace(SHA_PLACEHOLDER, sha) if name == "libcmt.lib" else text
    (research / "common_pool_ledger.json").write_text(text, encoding="utf-8")
    return root


def paths(root):
    root = Path(root)
    return {"root": root, "ledger_path": root / Q10 / "common_pool_ledger.json",
            "xdk_lib_dir": root / "xdk", "xdk_manifest_path": root / Q10 / "xdk_manifest.json"}


def cli_args(root, receipt_name="receipt.json", **drop):
    root = Path(root)
    args = {"--root": str(root), "--ledger": str(root / Q10 / "common_pool_ledger.json"),
            "--xdk-lib-dir": str(root / "xdk"),
            "--xdk-lib-manifest": str(root / Q10 / "xdk_manifest.json"),
            "--receipt": str(root / receipt_name), "--text": str(root / "out.txt"),
            "--json": str(root / "out.json")}
    for key in drop:
        args.pop("--" + key.replace("_", "-"))
    return [item for pair in args.items() for item in pair]


def cli(root, receipt_name="receipt.json"):
    return main(cli_args(root, receipt_name))


def receipt_of(root, name="receipt.json"):
    return json.loads((Path(root) / name).read_text(encoding="utf-8"))


def rows_by_name(report):
    return {row["name"]: row for row in report["records"]}


def reason_keys(row):
    return {reason.split(":")[0] for reason in row["reasons"]}


def run(tmp_path, **kwargs):
    write_world(tmp_path, **kwargs)
    return verify(**paths(tmp_path))


def fail_closed_names(report):
    assert report["result"] == "FAIL-CLOSED" and report["exit_code"] == EXIT_AMBIGUOUS
    assert report["accounting"] is None
    return {r["name"] for r in report["fail_closed"]["records"]}


def ledger_with(**changes):
    """Copy LEDGER, replacing fields of named records: ledger_with(_alpha={...}).

    A value of KeyError removes the key; a record set to None is dropped.
    """
    ledger = copy.deepcopy(LEDGER)
    for entry in ledger["records"]:
        if entry["name"] in changes:
            update = changes[entry["name"]]
            if update is None:
                continue
            for key, value in update.items():
                if value is KeyError:
                    entry.pop(key, None)
                else:
                    entry[key] = value
    drop = {name for name, update in changes.items() if update is None}
    ledger["records"] = [e for e in ledger["records"] if e["name"] not in drop]
    return ledger


def ours_with(**changes):
    ours = dict(OURS)
    for key, value in changes.items():
        rel = key.replace("__", "/") + ".obj"
        if value is None:
            ours.pop(rel)
        else:
            ours[rel] = value
    return ours


# -- helpers ---------------------------------------------------------------------

def test_layout_law_and_flag_alignment():
    assert [law_alignment(s) for s in (1, 2, 3, 4, 6, 12, 16, 48, 257, 547628)] == \
        [1, 2, 4, 4, 8, 16, 16, 32, 32, 32]
    assert alignment_from_flags(0xC0100080) == 1
    assert alignment_from_flags(0xC0300080) == 4
    assert alignment_from_flags(0xC0600080) == 32
    assert alignment_from_flags(RDATA_FLAGS) == 0
    with pytest.raises(ValueError):
        law_alignment(0)


def test_lib_reader_round_trip(tmp_path):
    lib = tmp_path / "x.lib"
    write_lib(lib, [("obj\\i386\\crt0dat.obj", b"abc"), ("short.obj", b"de")])
    assert list(read_lib_members(lib)) == [("obj\\i386\\crt0dat.obj", b"abc"), ("short.obj", b"de")]


# -- positive world -------------------------------------------------------------

def test_consistent_world_passes_with_separate_accounting(tmp_path):
    report = run(tmp_path)
    rows = rows_by_name(report)
    assert report["result"] == "COMPLETE" and report["exit_code"] == EXIT_OK
    assert report["global_failures"] == [] and report["fail_closed"] is None
    assert rows["_alpha"]["verdict"] == VERDICT_PASS
    assert rows["_beta"]["verdict"] == VERDICT_PASS
    assert rows["_gamma"]["verdict"] == VERDICT_PASS_UNRESOLVED
    assert rows["_delta"]["verdict"] == VERDICT_PASS
    assert rows["_delta"]["notes"] == ["vendor readout re-verified against the manifest-verified library"]
    assert set(rows) == {"_alpha", "_beta", "_gamma", "_delta"}  # linker record never a row
    acc = report["accounting"]
    assert (acc["halo"]["records"], acc["halo"]["bytes"], acc["halo"]["gap_bytes"]) == (3, 21, 11)
    assert list(acc["vendor"]) == ["libcmt"]
    assert (acc["vendor"]["libcmt"]["records"], acc["vendor"]["libcmt"]["bytes"]) == (1, 4)
    assert "unclassified" not in acc
    assert acc["conservation"] == {"january_bss_span": 36, "counted_bytes_plus_gaps": 36,
                                   "holds": True, "linker_bytes_excluded": 28}
    assert [x["name"] for x in report["excluded_linker_records"]] == ["_rdata_00001000"]
    assert acc["halo"]["by_tag"]["inferred"]["verdicts"] == {VERDICT_PASS: {"records": 1, "bytes": 4}}
    assert acc["halo"]["by_tag"]["unresolved"]["verdicts"] == {
        VERDICT_PASS_UNRESOLVED: {"records": 1, "bytes": 16}}
    assert "re-verified" in acc["segment_evidence"]
    assert cli(tmp_path) == EXIT_OK
    assert receipt_of(tmp_path)["result"] == "COMPLETE"


def test_tags_are_carried_verbatim_and_never_upgraded(tmp_path):
    # _beta has two direct readouts (ceiling inferred) but a recorded caveat keeps it probable
    ledger = ledger_with(_beta={"readouts": [readout("D", "source/b.c", "b"),
                                             readout("X", "source/b.c", "b")],
                                "caveats": ["owner is not a January referencer"]})
    report = run(tmp_path, ledger=ledger)
    rows = rows_by_name(report)
    assert rows["_beta"]["tag"] == "probable"
    assert rows["_beta"]["verdict"] == VERDICT_PASS
    assert tag_ceiling(ledger["records"][1], "halo") == "inferred"
    for entry in ledger["records"]:
        assert rows[entry["name"]]["tag"] == entry["tag"]
    # the inferred label never claims January proof
    assert "not January-proven" in rows["_alpha"]["tag_label"]


def test_report_carries_no_credit_and_writes_nothing(tmp_path):
    write_world(tmp_path)
    before = sorted(p.relative_to(tmp_path) for p in tmp_path.rglob("*"))
    report = verify(**paths(tmp_path))
    assert sorted(p.relative_to(tmp_path) for p in tmp_path.rglob("*")) == before

    def keys(node):
        if isinstance(node, dict):
            for key, value in node.items():
                yield str(key).lower()
                yield from keys(value)
        elif isinstance(node, list):
            for value in node:
                yield from keys(value)

    words = ("credit", "progress", "percent", "fuzzy", "match")
    assert not [k for k in keys(report) if any(w in k for w in words)]
    assert "credits nothing itself" in report["status"] and "never January-proven" in report["status"]
    assert cli(tmp_path) == EXIT_OK
    assert not [k for k in keys(receipt_of(tmp_path)) if any(w in k for w in words)]


# -- negative tests: storage contract ------------------------------------------

def test_wrong_size_fails_with_alignment_class(tmp_path):
    report = run(tmp_path, ours=ours_with(source__a=([], [common("_alpha", 8), undef("_beta")])))
    row = rows_by_name(report)["_alpha"]
    assert row["verdict"] == VERDICT_FAIL
    assert {"size-mismatch", "alignment-class-differs"} <= reason_keys(row)


def test_wrong_size_same_alignment_class_still_fails(tmp_path):
    report = run(tmp_path, ours=ours_with(source__a=([], [common("_alpha", 3)])))
    row = rows_by_name(report)["_alpha"]
    assert reason_keys(row) == {"size-mismatch"}


def test_duplicate_definers_fail(tmp_path):
    report = run(tmp_path, ours=ours_with(source__b=([], [common("_beta", 1), common("_alpha", 4)])))
    row = rows_by_name(report)["_alpha"]
    assert row["verdict"] == VERDICT_FAIL
    assert "duplicate-definers" in reason_keys(row)
    assert row["definers"] == ["source/a.c", "source/b.c"]


@pytest.mark.parametrize("section", [DATA_SECTION, BSS_SECTION])
def test_initialised_or_real_definition_fails(tmp_path, section):
    # `int alpha = 1;` (.data) or `int alpha = 0;` (own .bss): not tentative storage
    report = run(tmp_path, ours=ours_with(source__a=([section], [defined("_alpha"), undef("_beta")])))
    row = rows_by_name(report)["_alpha"]
    assert row["verdict"] == VERDICT_FAIL
    assert {"real-definition", "no-definer"} <= reason_keys(row)


def test_real_definition_beside_a_common_still_fails(tmp_path):
    report = run(tmp_path, ours=ours_with(source__c=([DATA_SECTION], [common("_gamma", 16),
                                                                       defined("_alpha")])))
    assert "real-definition" in reason_keys(rows_by_name(report)["_alpha"])


def test_static_definition_fails(tmp_path):
    report = run(tmp_path, ours=ours_with(source__a=([BSS_SECTION], [defined("_alpha", storage=3),
                                                                      undef("_beta")])))
    row = rows_by_name(report)["_alpha"]
    assert {"static-definition", "no-definer"} <= reason_keys(row)


def test_missing_definer_extern_only_and_absent(tmp_path):
    report = run(tmp_path, ours=ours_with(source__b=([], [undef("_alpha")]),
                                          source__a=([], [common("_beta", 1)])))
    assert "no-definer:extern-only" in rows_by_name(report)["_alpha"]["reasons"]
    report = run(tmp_path / "absent", ours=ours_with(source__a=([], [])))
    assert "no-definer:absent" in rows_by_name(report)["_alpha"]["reasons"]


def test_definer_must_be_the_evidence_owner(tmp_path):
    report = run(tmp_path, ours=ours_with(source__a=([], [undef("_beta")]),
                                          source__c=([], [common("_gamma", 16), common("_alpha", 4)])))
    row = rows_by_name(report)["_alpha"]
    assert reason_keys(row) == {"definer-disagrees-with-evidence-owner"}


def test_unresolved_record_passes_only_as_owner_unresolved(tmp_path):
    report = run(tmp_path)
    row = rows_by_name(report)["_gamma"]
    assert row["verdict"] == VERDICT_PASS_UNRESOLVED and row["owner"] is None
    assert row["candidate_owner"] == "source/c.c"


def test_surplus_common_is_a_global_failure(tmp_path):
    report = run(tmp_path, ours=ours_with(source__c=([], [common("_gamma", 16), common("_zeta", 4)])))
    assert any(g.startswith("surplus-common: _zeta") for g in report["global_failures"])
    assert cli(tmp_path) == EXIT_FAILURES


def test_object_outside_config_fails_closed(tmp_path):
    ours = ours_with(source__c=([], [undef("_gamma")]))
    ours["source/stray.obj"] = ([], [common("_gamma", 16)])
    report = run(tmp_path, ours=ours)
    assert "definer-category-unknown" in reason_keys(rows_by_name(report)["_gamma"])


# -- negative tests: evidence ------------------------------------------------------

def test_missing_evidence_tag_fails(tmp_path):
    report = run(tmp_path, ledger=ledger_with(_alpha={"tag": None}))
    row = rows_by_name(report)["_alpha"]
    assert "missing-provenance-tag" in reason_keys(row)
    assert report["accounting"]["halo"]["by_tag"]["missing"]["records"] == 1


@pytest.mark.parametrize("tag", ["proven", "PROVEN", "confirmed", ""])
def test_proof_vocabulary_is_rejected(tmp_path, tag):
    report = run(tmp_path, ledger=ledger_with(_alpha={"tag": tag}))
    keys = reason_keys(rows_by_name(report)["_alpha"])
    assert keys & {"invalid-provenance-tag", "missing-provenance-tag"}


@pytest.mark.parametrize("readouts", [
    [readout("D", "source/a.c", "a")],                                        # one readout
    [readout("X", "source/a.c", "a"), readout("XD", "source/a.c", "a")],      # one family (HCEX)
    [readout("D", "source/a.c", "a"), readout("OD", "source/a.c", "a", counted=False)],  # strict /Od EDGE
    [readout("D", "source/a.c", "a", "sibling"), readout("X", "source/a.c", "a", "sibling")],
    [readout("D", "source/b.c", "b"), readout("X", "source/b.c", "b")],      # readouts name another owner
    [],
])
def test_inferred_needs_two_direct_readout_families(tmp_path, readouts):
    report = run(tmp_path, ledger=ledger_with(_alpha={"readouts": readouts}))
    assert "tag-exceeds-evidence" in reason_keys(rows_by_name(report)["_alpha"])


def test_probable_needs_a_readout_and_unresolved_carries_no_owner(tmp_path):
    report = run(tmp_path, ledger=ledger_with(_beta={"readouts": []},
                                              _gamma={"owner": "source/c.c"}))
    rows = rows_by_name(report)
    assert "tag-exceeds-evidence" in reason_keys(rows["_beta"])
    assert "unresolved-record-carries-owner" in reason_keys(rows["_gamma"])


def test_missing_owner_and_name_evidence_fail(tmp_path):
    report = run(tmp_path, ledger=ledger_with(_alpha={"owner": None},
                                              _beta={"name_evidence": None},
                                              _gamma={"name_evidence": ""}))
    rows = rows_by_name(report)
    assert "missing-evidence-owner" in reason_keys(rows["_alpha"])
    assert "missing-name-evidence" in reason_keys(rows["_beta"])
    assert "missing-name-evidence" in reason_keys(rows["_gamma"])
    assert report["result"] == "COMPLETE" and report["accounting"]["halo"]["records"] == 3


def test_record_without_a_ledger_entry_fails_closed(tmp_path):
    report = run(tmp_path, ledger=ledger_with(_gamma=None))
    assert "no-ledger-entry" in reason_keys(rows_by_name(report)["_gamma"])
    assert fail_closed_names(report) == {"_gamma"}
    assert cli(tmp_path) == EXIT_AMBIGUOUS


def test_ledger_rva_and_unknown_entries(tmp_path):
    ledger = ledger_with(_alpha={"rva": "0x2001"})
    ledger["records"].append(record("_ghost", "0x9000", "halo", "halobetacache", "unresolved",
                                    None, []))
    report = run(tmp_path, ledger=ledger)
    assert "ledger-rva-mismatch" in reason_keys(rows_by_name(report)["_alpha"])
    assert "ledger-entry-not-in-pool: _ghost" in report["global_failures"]


def test_owner_pool_order_can_refute(tmp_path):
    config = copy.deepcopy(CONFIG)
    config["projects"][0]["objects"][1]["index"] = 5  # b.c now precedes a.c in module order
    report = run(tmp_path, config=config)
    assert "owner-violates-pool-order" in reason_keys(rows_by_name(report)["_beta"])


def test_january_static_flag_on_a_pooled_record_fails(tmp_path):
    report = run(tmp_path, symbols_extra=[{"name": "_beta", "static": True}])
    assert any("static" in r for r in rows_by_name(report)["_beta"]["reasons"])


# -- negative tests: Halo / vendor / linker separation ------------------------------

def test_vendor_record_cannot_enter_the_halo_tally(tmp_path):
    ledger = ledger_with(_delta={"segment": "halo", "category": "halobetacache"})
    report = run(tmp_path, ledger=ledger)
    row = rows_by_name(report)["_delta"]
    assert row["verdict"] == VERDICT_FAIL
    assert {"readout-channel-not-valid-for-segment", "definer-category-mismatch",
            "owner-category-mismatch", "vendor-evidence-on-halo-record"} <= reason_keys(row)
    assert fail_closed_names(report) == {"_delta"}


def test_vendor_library_refutes_a_halo_label_without_readouts(tmp_path):
    ledger = ledger_with(_delta={"segment": "halo", "category": "halobetacache",
                                 "tag": "unresolved", "owner": None, "readouts": [],
                                 "vendor_evidence": KeyError})
    ours = ours_with(libs__libcmt__crt0dat=([], [undef("_delta")]))
    ours["source/c.obj"] = ([], [common("_gamma", 16), common("_delta", 4)])
    report = run(tmp_path, ledger=ledger, ours=ours)
    row = rows_by_name(report)["_delta"]
    assert reason_keys(row) == {"halo-record-defined-by-vendor-library"}
    assert fail_closed_names(report) == {"_delta"}


def test_halo_record_after_vendor_records_conflicts(tmp_path):
    ledger = ledger_with(_beta={"segment": "vendor", "category": "libcmt", "tag": "unresolved",
                                "owner": None, "readouts": []})
    report = run(tmp_path, ledger=ledger)
    assert "segment-order-conflict" in reason_keys(rows_by_name(report)["_gamma"])
    assert fail_closed_names(report) == {"_beta", "_gamma"}


def test_tally_refuses_mixed_linker_or_unclassified_rows():
    good = {"name": "_a", "size": 4, "gap_before": 0, "segment": "halo",
            "category": "halobetacache", "tag": "inferred", "verdict": VERDICT_PASS, "reasons": []}
    tally([good])
    with pytest.raises(ValueError):
        tally([dict(good, category="libcmt")])            # vendor row in the Halo tally
    with pytest.raises(ValueError):
        tally([dict(good, segment="vendor", category="halobetacache")])
    with pytest.raises(ValueError):
        tally([dict(good, segment="linker")])             # linker record anywhere
    with pytest.raises(ValueError):
        tally([dict(good, kind="linker")])
    with pytest.raises(ValueError):
        tally([good, dict(good)])                         # counted twice
    with pytest.raises(ValueError):
        tally([dict(good, segment=None, category=None)])  # ambiguous rows are never tallied


def test_linker_record_in_the_ledger_is_rejected_and_never_counted(tmp_path):
    ledger = copy.deepcopy(LEDGER)
    ledger["records"].append(record("_rdata_00001000", "0x1000", "halo", "halobetacache",
                                    "unresolved", None, []))
    report = run(tmp_path, ledger=ledger)
    assert "linker-record-in-ledger: _rdata_00001000" in report["global_failures"]
    assert "_rdata_00001000" not in rows_by_name(report)
    acc = report["accounting"]
    counted = acc["halo"]["bytes"] + sum(b["bytes"] for b in acc["vendor"].values())
    assert counted == 25 and acc["conservation"]["linker_bytes_excluded"] == 28


def test_vendor_readout_reverified_against_the_library(tmp_path):
    report = run(tmp_path)
    assert rows_by_name(report)["_delta"]["verdict"] == VERDICT_PASS
    assert report["inputs"]["vendor_libraries_reverified"] is True
    report = run(tmp_path / "bad", libs=default_libs(delta_size=8))
    assert "vendor-readout-not-reproduced" in reason_keys(rows_by_name(report)["_delta"])
    assert fail_closed_names(report) == {"_delta"}


def test_vendor_readout_must_name_its_category_library(tmp_path):
    ledger = copy.deepcopy(LEDGER)
    ledger["records"][3]["readouts"][0]["library"] = "xapilib.lib"
    report = run(tmp_path, ledger=ledger)
    assert "vendor-readout-library-not-category" in reason_keys(rows_by_name(report)["_delta"])
    assert fail_closed_names(report) == {"_delta"}


# -- negative tests: January slot / alignment ---------------------------------------

def test_january_alignment_that_breaks_the_layout_law_fails(tmp_path):
    pool = [p if p[0] != "_gamma" else ("_gamma", 0x2010, 16, bss_flags(8)) for p in POOL]
    report = run(tmp_path, pool=pool)
    assert "january-alignment-law" in reason_keys(rows_by_name(report)["_gamma"])


def test_january_slot_that_disagrees_with_the_pool_fails(tmp_path):
    pool = [p if p[0] != "_gamma" else ("_gamma", 0x2020, 16, bss_flags(16)) for p in POOL]
    pool = [p if p[0] != "_delta" else ("_delta", 0x2030, 4, bss_flags(4)) for p in pool]
    report = run(tmp_path, pool=pool, ledger=ledger_with(_gamma={"rva": "0x2020"},
                                                         _delta={"rva": "0x2030"}))
    assert "january-slot-inconsistent" in reason_keys(rows_by_name(report)["_gamma"])
    assert rows_by_name(report)["_delta"]["verdict"] == VERDICT_PASS


def test_misaligned_january_record_fails(tmp_path):
    pool = [p if p[0] != "_beta" else ("_beta", 0x2004, 2, bss_flags(2)) for p in POOL]
    pool = [p if p[0] != "_gamma" else ("_gamma", 0x2008, 16, bss_flags(16)) for p in pool]
    report = run(tmp_path, pool=pool, ledger=ledger_with(_gamma={"rva": "0x2008"}),
                 ours=ours_with(source__b=([], [common("_beta", 2)])))
    assert {"january-misaligned", "january-slot-inconsistent"} <= reason_keys(rows_by_name(report)["_gamma"])


def test_overlapping_january_records_break_conservation(tmp_path):
    # _beta placed inside _alpha's extent: slot inconsistent, and bytes + gaps no longer equal the span
    pool = [p if p[0] != "_beta" else ("_beta", 0x2002, 1, bss_flags(1)) for p in POOL]
    report = run(tmp_path, pool=pool, ledger=ledger_with(_beta={"rva": "0x2002"}))
    assert "january-slot-inconsistent" in reason_keys(rows_by_name(report)["_beta"])
    conservation = report["accounting"]["conservation"]
    assert conservation["holds"] is False
    assert conservation["counted_bytes_plus_gaps"] == 38 and conservation["january_bss_span"] == 36


def test_split_pool_that_disagrees_with_contributions_fails(tmp_path):
    split = [p if p[0] != "_alpha" else ("_alpha", 0x2000, 8, bss_flags(8)) for p in POOL]
    report = run(tmp_path, split_pool=split)
    reasons = rows_by_name(report)["_alpha"]["reasons"]
    assert any(r.startswith("split size 8") for r in reasons)


# -- independent review (RF-DH): accounting soundness regressions ------------------

def test_tree_side_definer_facts_never_move_a_record_out_of_its_segment(tmp_path):
    # a stray COMMON (outside config) and a vendor-object COMMON fail the Halo
    # record, but the January segment is not a property of our tree
    ours = ours_with(libs__libcmt__crt0dat=([], [common("_delta", 4), common("_alpha", 4)]))
    ours["source/stray.obj"] = ([], [common("_beta", 1)])
    report = run(tmp_path, ours=ours)
    rows = rows_by_name(report)
    assert {"duplicate-definers", "definer-category-mismatch"} <= reason_keys(rows["_alpha"])
    assert {"duplicate-definers", "definer-category-unknown"} <= reason_keys(rows["_beta"])
    assert rows["_alpha"]["segment"] == "halo" and rows["_beta"]["segment"] == "halo"
    assert report["result"] == "COMPLETE"
    acc = report["accounting"]
    assert (acc["halo"]["records"], acc["halo"]["bytes"]) == (3, 21)


@pytest.mark.parametrize("symbol", [
    {"name": "_alpha", "value": 0, "section": 1, "storage": 2},     # EXTERNAL function
    {"name": "_alpha", "value": 0, "section": 0, "storage": 105},   # weak external alias
])
def test_function_or_weak_alias_named_like_a_record_fails(tmp_path, symbol):
    ours = ours_with(source__c=([TEXT_SECTION], [common("_gamma", 16), symbol]))
    row = rows_by_name(run(tmp_path, ours=ours))["_alpha"]
    assert row["verdict"] == VERDICT_FAIL and "real-definition" in reason_keys(row)


def test_static_function_of_the_same_name_is_private(tmp_path):
    ours = ours_with(source__c=([TEXT_SECTION], [common("_gamma", 16), defined("_alpha", storage=3)]))
    assert rows_by_name(run(tmp_path, ours=ours))["_alpha"]["verdict"] == VERDICT_PASS


@pytest.mark.parametrize("counted", ["false", "no", 1, 0.5, "EDGE", None])
def test_counted_must_be_the_boolean_true(tmp_path, counted):
    # F4: a non-boolean "counted" is a malformed type: the run fails closed (exit 4)
    ledger = ledger_with(_alpha={"readouts": [readout("D", "source/a.c", "a"),
                                              readout("X", "source/a.c", "a", counted=counted)]})
    write_world(tmp_path, ledger=ledger)
    with pytest.raises(MalformedInputError):
        verify(**paths(tmp_path))
    assert cli(tmp_path) == EXIT_MALFORMED
    # and the ceiling never counts anything but the boolean true
    entry = copy.deepcopy(ledger["records"][0])
    assert tag_ceiling(entry, "halo") == "probable"


def test_direct_readout_must_name_its_owner(tmp_path):
    ledger = ledger_with(_alpha={"readouts": [readout("D", "source/a.c", "b"),
                                              readout("X", "source/a.c", "a")]})
    keys = reason_keys(rows_by_name(run(tmp_path, ledger=ledger))["_alpha"])
    assert "readout-names-disagree-with-owner" in keys


def test_inferred_record_carries_no_caveat(tmp_path):
    ledger = ledger_with(_alpha={"caveats": ["size not independently confirmed"]})
    assert "tag-exceeds-evidence" in reason_keys(rows_by_name(run(tmp_path, ledger=ledger))["_alpha"])


def test_inferred_owner_must_be_a_january_referencer(tmp_path):
    write_world(tmp_path)
    # January's b.obj relocates to _alpha; the ledger's owner a.c never does
    reloc = struct.pack("<LLH", 0, 0, 6)
    split_b = tmp_path / "build" / "split" / "source" / "b.obj"
    split_b.write_bytes(build_coff(sections=[{"name": ".text", "size": 4, "flags": 0x60500020,
                                              "reloc_data": reloc}],
                                   symbols=[undef("_alpha")]))
    row = rows_by_name(verify(**paths(tmp_path)))["_alpha"]
    assert any(r.startswith("tag-exceeds-evidence: inferred owner source/a.c is not a January referencer")
               for r in row["reasons"])
    # the same relocation from a.c itself is consistent
    split_b.replace(tmp_path / "build" / "split" / "source" / "a.obj")
    split_b.write_bytes(build_coff(sections=[], symbols=[]))      # every unit keeps its split object (RF-DP D7)
    assert rows_by_name(verify(**paths(tmp_path)))["_alpha"]["verdict"] == VERDICT_PASS


def test_duplicate_ledger_entry_fails_closed(tmp_path):
    ledger = copy.deepcopy(LEDGER)
    forged = dict(copy.deepcopy(ledger["records"][1]), tag="inferred",
                  readouts=[readout("D", "source/b.c", "b"), readout("L", "source/b.c", "b")])
    ledger["records"].insert(0, forged)
    report = run(tmp_path, ledger=ledger)
    row = rows_by_name(report)["_beta"]
    assert "ledger lists _beta twice" in report["global_failures"]
    assert row["verdict"] == VERDICT_FAIL and "ledger-duplicate-entry" in reason_keys(row)
    assert fail_closed_names(report) == {"_beta"}


def test_vendor_label_needs_a_library_readout_and_order_is_symmetric(tmp_path):
    # the last Halo record relabelled vendor with no library evidence
    ledger = ledger_with(_gamma={"segment": "vendor", "category": "libcmt", "readouts": []})
    report = run(tmp_path, ledger=ledger, ours=ours_with(source__c=([], [undef("_gamma")])))
    row = rows_by_name(report)["_gamma"]
    assert "vendor-segment-without-library-readout" in reason_keys(row)
    assert fail_closed_names(report) == {"_gamma"}
    # a middle Halo record relabelled vendor: every record between the labels conflicts
    ledger = ledger_with(_beta={"segment": "vendor", "category": "libcmt", "tag": "unresolved",
                                "owner": None, "readouts": []})
    report = run(tmp_path / "middle", ledger=ledger)
    rows = rows_by_name(report)
    assert "segment-order-conflict" in reason_keys(rows["_beta"])
    assert fail_closed_names(report) == {"_beta", "_gamma"}


def test_unreproduced_vendor_readout_fails_closed(tmp_path):
    report = run(tmp_path, libs=default_libs(delta_size=8))
    row = rows_by_name(report)["_delta"]
    assert "vendor-readout-not-reproduced" in reason_keys(row) and row["segment"] is None
    assert fail_closed_names(report) == {"_delta"}


def test_xdk_libraries_are_required_no_ledger_only_mode(tmp_path):
    write_world(tmp_path)
    with pytest.raises(LibraryVerifyError, match="--xdk-lib-dir is required"):
        verify(**dict(paths(tmp_path), xdk_lib_dir=None))
    with pytest.raises(LibraryVerifyError, match="--xdk-lib-manifest is required"):
        verify(**dict(paths(tmp_path), xdk_manifest_path=None))
    for flag in ("xdk_lib_dir", "xdk_lib_manifest"):
        assert main(cli_args(tmp_path, f"{flag}.json", **{flag: True})) == EXIT_LIBRARIES
        receipt = receipt_of(tmp_path, f"{flag}.json")
        assert receipt["fail_closed"]["class"] == "libraries-missing-or-unverifiable"
        assert "is required" in receipt["fail_closed"]["message"] and receipt["summary"] is None
    report = verify(**paths(tmp_path))
    assert "LEDGER-ONLY" not in render_text(report)
    assert "re-verified" in report["accounting"]["segment_evidence"]


@pytest.mark.parametrize("change", [
    {"_alpha": {"tag": ["inferred"]}},
    {"_beta": {"readouts": {"D": "b"}}},
    {"_beta": {"readouts": ["D"]}},
    {"_gamma": {"owner": ["source/c.c"]}},
    {"_delta": {"category": ["libcmt"]}},
    {"_delta": {"readouts": [dict(XDK_READOUT, channel=["XDK"])]}},
])
def test_malformed_ledger_values_fail_closed(tmp_path, change):
    write_world(tmp_path, ledger=ledger_with(**change))
    with pytest.raises(MalformedInputError):
        verify(**paths(tmp_path))
    assert cli(tmp_path) == EXIT_MALFORMED
    receipt = receipt_of(tmp_path)
    assert receipt["result"] == "FAIL-CLOSED"
    assert receipt["fail_closed"]["class"] == "malformed-types"
    assert not (tmp_path / "out.txt").exists()  # no report, no accounting


def test_unusable_january_input_fails_closed(tmp_path):
    write_world(tmp_path)
    contribs_path = tmp_path / "config" / "contribs.json"
    contribs = json.loads(contribs_path.read_text(encoding="utf-8"))
    del contribs[0]["size"]
    contribs_path.write_text(json.dumps(contribs), encoding="utf-8")
    assert cli(tmp_path) == EXIT_MALFORMED              # a missing field is a malformed type
    contribs_path.write_text("{not json", encoding="utf-8")
    assert cli(tmp_path) == EXIT_UNREADABLE              # an unreadable file is exit 2
    assert receipt_of(tmp_path)["fail_closed"]["class"] == "unreadable-input"


def test_duplicate_january_name_fails_closed(tmp_path):
    # symbols.json and csplit both name two January records _alpha
    report = run(tmp_path,
                 pool=[p if p[0] != "_gamma" else ("_alpha", 0x2010, 16, bss_flags(16)) for p in POOL],
                 ledger=ledger_with(_gamma=None))
    dup = [r for r in report["records"] if r["name"] == "_alpha"]
    assert len(dup) == 2 and all("duplicate-january-name" in reason_keys(r) for r in dup)
    assert all(r["segment"] is None for r in dup)
    assert fail_closed_names(report) == {"_alpha"}


def test_writable_pool_contribution_is_not_excluded_as_linker(tmp_path):
    pool = POOL + [("_initialised", 0x2030, 4, 0xC0300040)]
    report = run(tmp_path, pool=pool)
    assert any(g.startswith("unexpected-non-bss-pool-contribution: _initialised")
               for g in report["global_failures"])
    assert report["january"]["linker_records"] == 1 and report["january"]["unexpected_records"] == 1
    assert report["result"] == "FAIL-CLOSED" and report["exit_code"] == EXIT_AMBIGUOUS
    assert [x["name"] for x in report["fail_closed"]["unexpected_contributions"]] == ["_initialised"]


# -- integration (RF-DM): libraries fail closed ---------------------------------------

def test_library_hash_mismatch_fails_closed_and_is_receipted(tmp_path):
    write_world(tmp_path)
    lib = tmp_path / "xdk" / "libcmt.lib"
    recorded = hashlib.sha256(lib.read_bytes()).hexdigest()
    write_lib(lib, default_libs(delta_size=8)["libcmt.lib"])     # same size, other bytes
    with pytest.raises(LibraryVerifyError, match="libcmt.lib sha256"):
        verify(**paths(tmp_path))
    assert cli(tmp_path) == EXIT_LIBRARIES
    receipt = receipt_of(tmp_path)
    row = receipt["xdk_libraries"]["libraries"]["libcmt.lib"]
    assert row["recorded_sha256"] == recorded and row["actual_sha256"] != recorded
    assert row["verified"] is False
    assert receipt["fail_closed"]["class"] == "libraries-missing-or-unverifiable"
    assert receipt["summary"] is None


def test_library_size_mismatch_fails_closed(tmp_path):
    write_world(tmp_path)
    manifest_path = tmp_path / Q10 / "xdk_manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest["libraries"][0]["size"] += 1                  # right sha256, wrong recorded size
    manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
    with pytest.raises(LibraryVerifyError, match="libcmt.lib sha256"):
        verify(**paths(tmp_path))


def test_unexpected_type_errors_fail_closed_as_malformed(tmp_path, monkeypatch):
    import tools.common_pool_verify as module
    write_world(tmp_path)

    def broken(*args, **kwargs):
        raise TypeError("unexpected shape")

    monkeypatch.setattr(module, "january_pool", broken)
    assert cli(tmp_path) == EXIT_MALFORMED                  # never 1 ("failures found"), never a crash
    receipt = receipt_of(tmp_path)
    assert receipt["fail_closed"]["class"] == "malformed-types"
    assert receipt["fail_closed"]["message"] == "unexpected shape"


def test_missing_recorded_library_fails_closed(tmp_path):
    write_world(tmp_path)
    (tmp_path / "xdk" / "libcmt.lib").unlink()
    with pytest.raises(LibraryVerifyError, match="missing library libcmt.lib"):
        verify(**paths(tmp_path))


def test_unlisted_library_fails_closed(tmp_path):
    write_world(tmp_path)
    write_lib(tmp_path / "xdk" / "extra.lib", [("x.obj", delta_member())])
    with pytest.raises(LibraryVerifyError, match="unlisted library extra.lib"):
        verify(**paths(tmp_path))


def test_missing_manifest_or_directory_fails_closed(tmp_path):
    write_world(tmp_path)
    with pytest.raises(LibraryVerifyError):
        verify(**dict(paths(tmp_path), xdk_lib_dir=tmp_path / "nowhere"))
    (tmp_path / Q10 / "xdk_manifest.json").unlink()
    with pytest.raises(LibraryVerifyError):
        verify(**paths(tmp_path))
    assert cli(tmp_path) == EXIT_LIBRARIES


def test_vendor_category_needs_a_linked_or_declared_library(tmp_path):
    config = copy.deepcopy(CONFIG)
    config["projects"].append({"name": "binkxbox", "objects": [
        {"name": "libs/binkxbox/binkxbox.c", "index": 60, "status": "MISSING"}]})   # not built (RF-DP D8)
    write_world(tmp_path, config=config)
    with pytest.raises(LibraryVerifyError, match="vendor category binkxbox"):
        verify(**paths(tmp_path))
    # declared (with January evidence) as linked but not in the extract: accepted and disclosed
    manifest = json.loads((tmp_path / Q10 / "xdk_manifest.json").read_text(encoding="utf-8"))
    manifest["linked_libraries_not_in_extract"] = [
        {"library": "binkxbox.lib", "category": "binkxbox", "january_path": "c:\\binkxbox\\binkxbox.lib",
         "january_pdb_modules": 24, "reason": "not XDK"}]
    write_world(tmp_path / "declared", config=config, manifest=manifest)
    report = verify(**paths(tmp_path / "declared"))
    assert report["result"] == "COMPLETE"
    assert report["libraries"]["linked_not_in_extract"][0]["library"] == "binkxbox.lib"
    assert "binkxbox.lib" in report["accounting"]["segment_evidence"]
    # an unlinked library is never used for a category
    write_world(tmp_path / "unlinked", unlinked=("libcmt.lib",))
    with pytest.raises(LibraryVerifyError, match="vendor category libcmt"):
        verify(**paths(tmp_path / "unlinked"))


def test_vendor_records_of_a_library_not_in_the_extract_fail_closed(tmp_path):
    config = copy.deepcopy(CONFIG)
    config["projects"][1]["name"] = "dsstrmh"
    manifest_absent = [{"library": "dsstrmh.lib", "category": "dsstrmh", "january_path": "x",
                        "january_pdb_modules": 1, "reason": "not in the extract"}]
    ledger = ledger_with(_delta={"category": "dsstrmh"})
    write_world(tmp_path, config=config, ledger=ledger)
    manifest = json.loads((tmp_path / Q10 / "xdk_manifest.json").read_text(encoding="utf-8"))
    manifest["linked_libraries_not_in_extract"] = manifest_absent
    (tmp_path / Q10 / "xdk_manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
    with pytest.raises(LibraryVerifyError, match="need a linked library"):
        verify(**paths(tmp_path))


def test_unreadable_member_of_a_linked_library_fails_closed(tmp_path):
    libs = default_libs()
    libs["libcmt.lib"].append(("ltcg.obj", anonymous_member()))
    write_world(tmp_path, libs=libs)
    with pytest.raises(LibraryVerifyError, match="anonymous object"):
        verify(**paths(tmp_path))
    # the same member in a library January did not link is hash-checked only
    libs = default_libs()
    libs["d3d8ltcg.lib"] = [("ltcg.obj", anonymous_member())]
    write_world(tmp_path / "unlinked", libs=libs, unlinked=("d3d8ltcg.lib",))
    assert verify(**paths(tmp_path / "unlinked"))["result"] == "COMPLETE"


@pytest.mark.parametrize("member", [
    build_coff(machine=0, sections=[], symbols=[{"name": "_alpha", "value": 0, "section": 0,
                                                 "storage": 105}]),           # OLDNAMES-style alias
    import_member("_alpha"),                                                  # import descriptor
    build_coff(sections=[{"name": ".rsrc$01", "size": 4, "flags": 0x40000040}],
               symbols=[{"name": "_alpha", "value": 0, "section": 1, "storage": 2}],
               strtab=b"\0\0\0\0"),                                           # CVTRES: strtab length 0
])
def test_every_linked_member_kind_is_scanned_for_halo_names(tmp_path, member):
    libs = default_libs()
    libs["libcmt.lib"].append(("other.obj", member))
    report = run(tmp_path, libs=libs)
    assert "halo-record-defined-by-vendor-library" in reason_keys(rows_by_name(report)["_alpha"])
    assert fail_closed_names(report) == {"_alpha"}


def test_member_reader_is_strict(tmp_path):
    kind, symbols = member_symbols(import_member("_x"))
    assert kind == "import" and [s[0] for s in symbols] == ["_x", "__imp__x"]
    with pytest.raises(LibraryVerifyError):
        member_symbols(b"\x64\x86" + b"\0" * 30)                   # x64 member
    with pytest.raises(LibraryVerifyError):
        member_symbols(build_coff(sections=[], symbols=[common("_a", 4)])[:-6])  # truncated
    lib = tmp_path / "t.lib"
    write_lib(lib, [("a.obj", b"abc")])
    lib.write_bytes(lib.read_bytes()[:-2])
    with pytest.raises(LibraryVerifyError):
        list(read_lib_members(lib))


# -- integration (RF-DM): malformed types and ambiguous classification ------------------

@pytest.mark.parametrize("mutate", [
    lambda l: l["records"][0].update(caveat=["typo'd key hides a caveat"]),    # unknown key
    lambda l: l["records"][0].pop("caveats"),                                  # missing key
    lambda l: l["records"][0].update(name_evidence=3),
    lambda l: l["records"][0].update(caveats="one caveat"),
    lambda l: l["records"][0]["readouts"][0].update(counted="true"),
    lambda l: l["records"][0]["readouts"][0].update(library="libcmt.lib"),     # Halo readout + library
    lambda l: l["records"][3]["readouts"][0].pop("member"),                    # XDK readout, no member
    lambda l: l["records"][3]["vendor_evidence"].update(size="4"),
    lambda l: l["records"][3]["vendor_evidence"].pop("statement"),
    lambda l: l["records"][2].update(containment_evidence=[dict(CONTAINMENT, counted=True)]),
    lambda l: l.update(schema="common-pool-ledger/1"),
    lambda l: l.update(records={}),
    lambda l: l["records"][0].update(name=""),
])
def test_malformed_ledger_types_fail_closed(tmp_path, mutate):
    ledger = copy.deepcopy(LEDGER)
    mutate(ledger)
    write_world(tmp_path, ledger=ledger)
    with pytest.raises(MalformedInputError):
        verify(**paths(tmp_path))
    assert cli(tmp_path) == EXIT_MALFORMED


@pytest.mark.parametrize("mutate", [
    lambda m: m["libraries"][0].update(sha256="ABC"),
    lambda m: m["libraries"].append(dict(m["libraries"][0])),
    lambda m: m["libraries"][0].update(linked_by_january="yes"),
    lambda m: m["libraries"][0].update(file="sub/libcmt.lib"),
    lambda m: m.update(schema="xdk-lib-manifest/0"),
    lambda m: m.pop("evidence"),
])
def test_malformed_manifest_fails_closed(tmp_path, mutate):
    write_world(tmp_path)
    manifest_path = tmp_path / Q10 / "xdk_manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    mutate(manifest)
    manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
    with pytest.raises(MalformedInputError):
        verify(**paths(tmp_path))


def test_ambiguous_classification_fails_closed_without_accounting(tmp_path):
    write_world(tmp_path, ledger=ledger_with(_beta={"segment": "martian"}))
    report = verify(**paths(tmp_path))
    assert "ledger-segment-invalid" in reason_keys(rows_by_name(report)["_beta"])
    assert fail_closed_names(report) == {"_beta"}
    assert "FAIL-CLOSED (ambiguous-classification)" in render_text(report)
    assert cli(tmp_path) == EXIT_AMBIGUOUS
    receipt = receipt_of(tmp_path)
    assert receipt["result"] == "FAIL-CLOSED" and receipt["exit_code"] == EXIT_AMBIGUOUS
    assert [name for name, _ in receipt["fail_closed"]["records"]] == ["_beta"]
    assert receipt["summary"] is None
    assert json.loads((tmp_path / "out.json").read_text(encoding="utf-8"))["accounting"] is None


def test_vendor_category_outside_config_is_ambiguous(tmp_path):
    report = run(tmp_path, ledger=ledger_with(_delta={"category": "martianlib"}))
    assert "ledger-category-invalid" in reason_keys(rows_by_name(report)["_delta"])
    assert fail_closed_names(report) == {"_delta"}


# -- integration (RF-DM): containment never raises a tag (hs_debug_data guard) ---------

HS_LIKE = {"candidate_owner": "source/c.c", "containment_evidence": [CONTAINMENT],
           "caveats": ["candidate owner only"]}


def test_containment_is_recorded_but_never_raises_a_tag(tmp_path):
    report = run(tmp_path, ledger=ledger_with(_gamma=HS_LIKE))
    row = rows_by_name(report)["_gamma"]
    assert (row["tag"], row["owner"], row["verdict"]) == ("unresolved", None, VERDICT_PASS_UNRESOLVED)
    assert row["candidate_owner"] == "source/c.c" and row["containment_evidence"] == 1
    assert "not established" in row["notes"][0] and "sole definer is the candidate" in row["notes"][0]
    entry = ledger_with(_gamma=HS_LIKE)["records"][2]
    for tag in ("probable", "inferred"):
        raised = dict(entry, tag=tag, owner="source/c.c")
        assert tag_ceiling(raised, "halo") == "unresolved"
        report = run(tmp_path / tag, ledger=ledger_with(_gamma=dict(HS_LIKE, tag=tag, owner="source/c.c",
                                                                    caveats=[])))
        reasons = rows_by_name(report)["_gamma"]["reasons"]
        assert any(r.startswith(f"tag-exceeds-evidence: {tag} > unresolved (containment evidence never "
                                f"raises a tag)") for r in reasons)
        assert rows_by_name(report)["_gamma"]["verdict"] == VERDICT_FAIL


def test_containment_cannot_be_smuggled_in_as_a_readout(tmp_path):
    smuggled = dict(readout("D", "source/c.c", "c"), channel="CONTAINMENT")
    ledger = ledger_with(_gamma=dict(HS_LIKE, tag="probable", owner="source/c.c", readouts=[smuggled]))
    row = rows_by_name(run(tmp_path, ledger=ledger))["_gamma"]
    assert {"readout-channel-unknown", "tag-exceeds-evidence"} <= reason_keys(row)


def test_containment_evidence_must_name_the_record_candidate(tmp_path):
    bad = [dict(CONTAINMENT, candidate_owner="source/a.c"), dict(CONTAINMENT, kind="adjacency")]
    row = rows_by_name(run(tmp_path, ledger=ledger_with(_gamma=dict(HS_LIKE, containment_evidence=bad))))["_gamma"]
    assert len([r for r in row["reasons"] if r.startswith("containment-evidence-invalid")]) == 2


# -- integration (RF-DM): vendor evidence is explicit ---------------------------------

def test_vendor_evidence_must_be_recorded(tmp_path):
    report = run(tmp_path, ledger=ledger_with(_delta={"vendor_evidence": KeyError}))
    row = rows_by_name(report)["_delta"]
    assert reason_keys(row) == {"vendor-evidence-missing"}
    assert report["result"] == "COMPLETE" and row["segment"] == "vendor"


@pytest.mark.parametrize("change,key,closed", [
    ({"library_sha256": "0" * 64}, "vendor-evidence-library-hash-mismatch", True),
    ({"member": "obj\\i386\\other.obj"}, "vendor-evidence-disagrees-with-readout", True),
    ({"library": "xapilib.lib"}, "vendor-evidence-disagrees-with-readout", True),
    ({"size": 8}, "vendor-evidence-size-mismatch", False),
    ({"definition": "DEFINED"}, "vendor-evidence-size-mismatch", False),
    ({"support": "january-proven"}, "vendor-evidence-invalid", False),
])
def test_vendor_evidence_is_checked_against_library_and_january(tmp_path, change, key, closed):
    evidence = dict(VENDOR_EVIDENCE, **change)
    report = run(tmp_path, ledger=ledger_with(_delta={"vendor_evidence": evidence}))
    assert key in reason_keys(rows_by_name(report)["_delta"])
    assert (report["result"] == "FAIL-CLOSED") is closed


def test_vendor_record_supported_only_by_a_library_member_stays_probable(tmp_path):
    two = [XDK_READOUT, dict(XDK_READOUT, library="libcmt.lib")]
    report = run(tmp_path, ledger=ledger_with(_delta={"tag": "inferred", "readouts": two}))
    reasons = rows_by_name(report)["_delta"]["reasons"]
    assert any(r.startswith("tag-exceeds-evidence: a vendor record supported only by an XDK library "
                            "member remains probable") for r in reasons)
    assert any(r.startswith("tag-exceeds-evidence: inferred > probable") for r in reasons)


# -- integration (RF-DM): receipts ----------------------------------------------------

def test_every_run_writes_a_receipt(tmp_path):
    import tools.common_pool_verify as module
    write_world(tmp_path)
    assert cli(tmp_path, "r0.json") == EXIT_OK
    receipt = receipt_of(tmp_path, "r0.json")
    source = Path(module.__file__).read_bytes()
    assert receipt["verifier"]["sha256"] == hashlib.sha256(source).hexdigest()
    assert receipt["verifier"]["sha256_lf"] == hashlib.sha256(source.replace(b"\r\n", b"\n")).hexdigest()
    ledger_bytes = (tmp_path / Q10 / "common_pool_ledger.json").read_bytes()
    assert receipt["inputs"]["ledger"]["sha256_lf"] == hashlib.sha256(ledger_bytes).hexdigest()
    assert receipt["inputs"]["xdk_manifest"]["sha256"]
    lib = receipt["xdk_libraries"]["libraries"]["libcmt.lib"]
    assert lib["verified"] and lib["actual_sha256"] == lib["recorded_sha256"]
    snapshot = receipt["build_snapshot"]
    assert snapshot["objects"]["files"] == 4 and len(snapshot["objects"]["manifest_sha256"]) == 64
    assert snapshot["split"]["files"] == 5      # the pool + one January object per config unit (RF-DP D7)
    assert receipt["argv"][:2] == ["--root", str(tmp_path)]
    assert receipt["summary"]["halo"]["verdicts"] == {"PASS": [2, 5], "PASS-OWNER-UNRESOLVED": [1, 16]}
    assert receipt["summary"]["vendor"]["libcmt"]["verdicts"] == {"PASS": [1, 4]}
    # fail-closed runs are receipted too, with every input fingerprint that could be taken
    (tmp_path / "xdk" / "libcmt.lib").write_bytes(b"!<arch>\n")
    assert cli(tmp_path, "r1.json") == EXIT_LIBRARIES
    receipt = receipt_of(tmp_path, "r1.json")
    assert receipt["result"] == "FAIL-CLOSED" and receipt["inputs"]["ledger"]["sha256_lf"]
    assert receipt["build_snapshot"]["objects"]["files"] == 4
    report_json = json.loads((tmp_path / "out.json").read_text(encoding="utf-8"))
    assert report_json["receipt"]["exit_code"] == EXIT_OK      # the r0 run's report, untouched by r1


def test_ledger_hash_is_line_ending_independent(tmp_path):
    write_world(tmp_path)
    ledger_path = tmp_path / Q10 / "common_pool_ledger.json"
    ledger_path.write_bytes(json.dumps(LEDGER, indent=1).replace(SHA_PLACEHOLDER, json.loads(
        (tmp_path / Q10 / "xdk_manifest.json").read_text())["libraries"][0]["sha256"]).encode())
    lf = verify(**paths(tmp_path))["inputs"]
    ledger_path.write_bytes(ledger_path.read_bytes().replace(b"\n", b"\r\n"))
    crlf = verify(**paths(tmp_path))["inputs"]
    assert lf["ledger_sha256_lf"] == crlf["ledger_sha256_lf"]
    assert lf["ledger_sha256"] != crlf["ledger_sha256"]


def test_ledger_path_is_explicit(tmp_path):
    write_world(tmp_path)
    with pytest.raises(PoolVerifyError, match="--ledger is required"):
        verify(**dict(paths(tmp_path), ledger_path=None))
    assert main(cli_args(tmp_path, ledger=True)) == EXIT_UNREADABLE
    assert "--ledger is required" in receipt_of(tmp_path)["fail_closed"]["message"]
    with pytest.raises(SystemExit):                     # nowhere to write the receipt: usage error
        main(cli_args(tmp_path, receipt=True))


# -- the research ledger and the real January pool --------------------------------------

@pytest.mark.skipif(not REPO_LEDGER.is_file() or not REPO_MANIFEST.is_file(),
                    reason="research ledger / XDK manifest not present")
def test_research_ledger_records():
    assert not (REPO / "config" / "common_pool_ledger.json").exists()   # research record, not config
    ledger = json.loads(REPO_LEDGER.read_text(encoding="utf-8"))
    manifest = json.loads(REPO_MANIFEST.read_text(encoding="utf-8"))
    from tools.common_pool_verify import validate_ledger, validate_manifest
    validate_ledger(ledger)
    validate_manifest(manifest)
    sha = {e["file"].lower(): e["sha256"] for e in manifest["libraries"]}
    records = {r["name"]: r for r in ledger["records"]}
    hs = records["_hs_debug_data"]
    assert (hs["tag"], hs["owner"], hs["readouts"]) == ("unresolved", None, [])
    assert hs["candidate_owner"] == "source/hs/hs_runtime.c"
    assert [c["candidate_owner"] for c in hs["containment_evidence"]] == ["source/hs/hs_runtime.c"]
    assert tag_ceiling(dict(hs, owner=hs["candidate_owner"]), "halo") == "unresolved"
    vendor = [r for r in ledger["records"] if r["segment"] == "vendor"]
    assert len(vendor) == 27 and {r["tag"] for r in vendor} == {"probable"}
    for entry in vendor:
        evidence = entry["vendor_evidence"]
        assert evidence["support"] == "xdk-library-member-only"
        assert evidence["library_sha256"] == sha[evidence["library"].lower()]
    assert not any("vendor_evidence" in r for r in ledger["records"] if r["segment"] == "halo")


def _xdk_dir():
    candidates = [os.environ.get("HALO_Q10_XDK_LIB_DIR")]
    if REPO_MANIFEST.is_file():
        candidates.append(json.loads(REPO_MANIFEST.read_text(encoding="utf-8"))["source_dir"])
    for candidate in candidates:
        if candidate and Path(candidate).is_dir():
            return Path(candidate)
    return None


@pytest.mark.skipif(not (REPO / "build/split/source/linker_common.obj").is_file()
                    or not REPO_LEDGER.is_file() or _xdk_dir() is None,
                    reason="January split pool, research ledger or XDK libraries not present")
def test_real_january_pool_accounting(tmp_path, monkeypatch):
    import tools.common_pool_verify as module
    monkeypatch.setattr(module, "XDK_MANIFEST_SHA256_LF", PINNED_MANIFEST)   # the real run is pinned (RF-DX D2)
    # a complete build in which no object defines anything: one empty object per built unit (RF-DP D8)
    empty = tmp_path / "no_objects"
    config = json.loads((REPO / "config" / "config.json").read_text(encoding="utf-8"))
    for project in config["projects"]:
        for entry in project["objects"]:
            if entry.get("status") != "MISSING":
                path = empty / Path(entry["name"]).with_suffix(".obj")
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(build_coff(sections=[], symbols=[]))
    report = verify(REPO, objects_root=empty, ledger_path=REPO_LEDGER, xdk_lib_dir=_xdk_dir(),
                    xdk_manifest_path=REPO_MANIFEST)
    assert report["result"] == "COMPLETE"
    january = report["january"]
    assert (january["records"], january["bss_records"], january["linker_records"]) == (242, 240, 2)
    assert january["layout_reasons"] == 0 and report["global_failures"] == []
    assert sum(x["size"] for x in report["excluded_linker_records"]) == 88
    acc = report["accounting"]
    assert (acc["halo"]["records"], acc["halo"]["bytes"], acc["halo"]["gap_bytes"]) == (213, 1270477, 675)
    vendor = {k: (v["records"], v["bytes"]) for k, v in acc["vendor"].items()}
    assert vendor == {"dsound": (1, 4), "libcmt": (17, 845), "xapilib": (9, 276)}
    assert sum(v["gap_bytes"] for v in acc["vendor"].values()) == 131
    assert acc["conservation"]["holds"] and acc["conservation"]["january_bss_span"] == 1272408
    tags = {t: acc["halo"]["by_tag"][t]["records"] for t in TAG_RANK}
    assert tags == {"unresolved": 83, "probable": 60, "inferred": 70}
    assert sorted(report["libraries"]["scanned_linked"]) == [
        "d3d8.lib", "d3dx8.lib", "dsound.lib", "libcmt.lib", "oldnames.lib", "xapilib.lib", "xbdm.lib",
        "xboxkrnl.lib", "xkbd.lib", "xnet.lib"]
    # with no objects, every record fails for want of a definer - and only for that
    reasons = {r.split(":")[0] for row in report["records"] for r in row["reasons"]}
    assert reasons == {"no-definer", "owner-not-in-config"}


# -- independent re-review (RF-DP): fail-closed gaps found after integration -----------------

def _manifest_path(root):
    return Path(root) / Q10 / "xdk_manifest.json"


def _edit_manifest(root, change):
    manifest = json.loads(_manifest_path(root).read_text(encoding="utf-8"))
    change(manifest)
    _manifest_path(root).write_text(json.dumps(manifest), encoding="utf-8")


def test_manifest_linked_flags_must_match_january_module_counts(tmp_path):
    write_world(tmp_path)
    _edit_manifest(tmp_path, lambda m: m["libraries"][0].update(january_pdb_modules=0))   # linked, 0 modules
    with pytest.raises(MalformedInputError, match="disagrees with january_pdb_modules"):
        verify(**paths(tmp_path))
    write_world(tmp_path / "absent")
    _edit_manifest(tmp_path / "absent", lambda m: m["linked_libraries_not_in_extract"].append(
        {"library": "binkxbox.lib", "category": None, "january_path": "x", "january_pdb_modules": 0,
         "reason": "test"}))
    with pytest.raises(MalformedInputError, match="declared linked by January with 0"):
        verify(**paths(tmp_path / "absent"))


def test_manifest_cannot_hide_a_present_library(tmp_path):
    # RF-DP T02: mark the category's own library unlinked and declare the category absent, so that
    # its records could be relabelled Halo without any library scan
    write_world(tmp_path)

    def hide(manifest):
        manifest["libraries"][0].update(linked_by_january=False, january_pdb_modules=0)
        manifest["linked_libraries_not_in_extract"].append(
            {"library": "libcmt_january.lib", "category": "libcmt", "january_path": "x",
             "january_pdb_modules": 1, "reason": "test"})
    _edit_manifest(tmp_path, hide)
    with pytest.raises(LibraryVerifyError) as caught:
        verify(**paths(tmp_path))
    assert "libcmt.lib is listed but not marked linked" in str(caught.value)
    assert "declared to have no library in the extract" in str(caught.value)
    # a library declared absent must not be sitting in the directory
    write_world(tmp_path / "present")
    (tmp_path / "present" / "xdk" / "binkxbox.lib").write_bytes(b"!<arch>\n")
    _edit_manifest(tmp_path / "present", lambda m: m["linked_libraries_not_in_extract"].append(
        {"library": "binkxbox.lib", "category": None, "january_path": "x", "january_pdb_modules": 24,
         "reason": "test"}))
    with pytest.raises(LibraryVerifyError, match="declared not in the extract but is present"):
        verify(**paths(tmp_path / "present"))


def test_manifest_may_not_declare_a_listed_library_absent(tmp_path):
    write_world(tmp_path)
    _edit_manifest(tmp_path, lambda m: m["linked_libraries_not_in_extract"].append(
        {"library": "LIBCMT.lib", "category": None, "january_path": "x", "january_pdb_modules": 1,
         "reason": "test"}))
    with pytest.raises(MalformedInputError, match="both present and absent"):
        verify(**paths(tmp_path))


def test_upper_case_unlisted_library_fails_closed(tmp_path):
    write_world(tmp_path)
    (tmp_path / "xdk" / "EXTRA.LIB").write_bytes(b"!<arch>\n")
    with pytest.raises(LibraryVerifyError, match="unlisted library EXTRA.LIB"):
        verify(**paths(tmp_path))


def test_library_names_differing_only_in_case_fail_closed(tmp_path, monkeypatch):
    write_world(tmp_path)
    lib_dir = tmp_path / "xdk"
    variant = lib_dir / "LIBCMT.lib"
    if not variant.exists():                    # a case-sensitive file system: a real second file
        variant.write_bytes((lib_dir / "libcmt.lib").read_bytes())
    else:                                       # case-insensitive: list the same file under both names
        real_iterdir = Path.iterdir

        def iterdir(self):
            items = list(real_iterdir(self))
            if self == lib_dir:
                items.append(variant)
            return iter(items)
        monkeypatch.setattr(Path, "iterdir", iterdir)
    with pytest.raises(LibraryVerifyError, match="differ only in case"):
        verify(**paths(tmp_path))


def test_vendor_readout_is_reproduced_only_from_its_own_library(tmp_path):
    # another linked library defines _delta in a member of the same name; libcmt.lib does not
    libs = {"libcmt.lib": [("obj\\i386\\other.obj", build_coff(sections=[], symbols=[]))],
            "other.lib": [("obj\\i386\\crt0dat.obj", delta_member())]}
    report = run(tmp_path, libs=libs)
    assert fail_closed_names(report) == {"_delta"}
    assert "vendor-readout-not-reproduced" in reason_keys(rows_by_name(report)["_delta"])


@pytest.mark.parametrize("which", ["ledger", "manifest", "contribs"])
def test_duplicate_json_keys_fail_closed(tmp_path, which):
    write_world(tmp_path)
    path = {"ledger": tmp_path / Q10 / "common_pool_ledger.json", "manifest": _manifest_path(tmp_path),
            "contribs": tmp_path / "config" / "contribs.json"}[which]
    text = path.read_text(encoding="utf-8")
    brace = text.index("{")                     # the top-level object, or the first contribution
    first = '"size": 1, ' if which == "contribs" else '"schema": "x", '
    path.write_text(text[:brace + 1] + first + text[brace + 1:], encoding="utf-8")
    assert main(cli_args(tmp_path)) == EXIT_MALFORMED
    receipt = receipt_of(tmp_path)
    assert receipt["fail_closed"]["class"] == "malformed-types"
    assert "duplicate JSON key" in receipt["fail_closed"]["message"]


def test_duplicate_caveats_key_cannot_hide_a_caveat(tmp_path):
    write_world(tmp_path)
    path = tmp_path / Q10 / "common_pool_ledger.json"
    text = path.read_text(encoding="utf-8")
    at = text.index('"name": "_alpha"')
    path.write_text(text[:at] + '"caveats": ["hidden"], ' + text[at:], encoding="utf-8")
    with pytest.raises(MalformedInputError, match="duplicate JSON key 'caveats'"):
        verify(**paths(tmp_path))


def test_missing_or_unreadable_january_split_object_fails_closed(tmp_path):
    write_world(tmp_path)
    split_b = tmp_path / "build" / "split" / "source" / "b.obj"
    split_b.unlink()
    assert main(cli_args(tmp_path)) == EXIT_UNREADABLE
    assert "no January split object" in receipt_of(tmp_path)["fail_closed"]["message"]
    split_b.write_bytes(b"\x4c\x01")
    assert main(cli_args(tmp_path)) == EXIT_UNREADABLE
    assert "split object(s) missing or unreadable" in receipt_of(tmp_path)["fail_closed"]["message"]


def test_split_pool_outside_its_split_tree_fails_closed(tmp_path):
    write_world(tmp_path)
    alone = tmp_path / "elsewhere" / "pool" / "linker_common.obj"
    alone.parent.mkdir(parents=True)
    alone.write_bytes((tmp_path / "build" / "split" / "source" / "linker_common.obj").read_bytes())
    with pytest.raises(PoolVerifyError, match="no January split object"):
        verify(**dict(paths(tmp_path), split_path=alone))


def test_absent_object_of_a_built_unit_fails_closed(tmp_path):
    ours = dict(OURS)
    ours.pop("source/b.obj")
    write_world(tmp_path, ours=ours)
    assert main(cli_args(tmp_path)) == EXIT_UNREADABLE
    assert "source/b.c" in receipt_of(tmp_path)["fail_closed"]["message"]


def test_bad_section_number_in_an_object_is_receipted_not_a_crash(tmp_path):
    write_world(tmp_path)
    stray = tmp_path / "build" / "base" / "stray" / "bad.obj"
    stray.parent.mkdir(parents=True)
    stray.write_bytes(build_coff(sections=[], symbols=[{"name": "_alpha", "value": 0, "section": 5,
                                                        "type": 0, "storage": 2, "aux_count": 0}]))
    assert main(cli_args(tmp_path)) == EXIT_UNREADABLE
    assert "names section 5 of 0" in receipt_of(tmp_path)["fail_closed"]["message"]


def test_unreadable_files_are_receipted_not_a_crash(tmp_path, monkeypatch):
    import tools.common_pool_verify as module
    write_world(tmp_path)

    def locked(*_args, **_kwargs):
        raise PermissionError(13, "Permission denied (test)")
    monkeypatch.setattr(module, "tree_manifest", locked)
    assert main(cli_args(tmp_path)) == EXIT_UNREADABLE
    receipt = receipt_of(tmp_path)
    assert receipt["result"] == "FAIL-CLOSED" and "PermissionError" not in receipt["fail_closed"]["message"]
    assert "Permission denied" in receipt["fail_closed"]["message"]
    monkeypatch.undo()
    real_read = Path.read_bytes

    def read_bytes(self):
        if self.name == "libcmt.lib":
            raise PermissionError(13, "Permission denied (test)")
        return real_read(self)
    monkeypatch.setattr(Path, "read_bytes", read_bytes)
    assert main(cli_args(tmp_path, "r2.json")) == EXIT_LIBRARIES
    assert "cannot read library" in receipt_of(tmp_path, "r2.json")["fail_closed"]["message"]


def test_inputs_changed_during_the_run_fail_closed(tmp_path, monkeypatch):
    import tools.common_pool_verify as module
    write_world(tmp_path)
    ledger_path = tmp_path / Q10 / "common_pool_ledger.json"
    real_scan = module.scan_objects

    def scan_and_touch(*args, **kwargs):
        ledger_path.write_bytes(ledger_path.read_bytes() + b"\n")    # a concurrent writer
        return real_scan(*args, **kwargs)
    monkeypatch.setattr(module, "scan_objects", scan_and_touch)
    with pytest.raises(PoolVerifyError, match="inputs changed during the run"):
        verify(**paths(tmp_path))


def test_excluded_record_inconsistency_is_a_global_failure(tmp_path):
    split = [p if p[0] != "_rdata_00001000" else ("_rdata_00001000", 0x1000, 32, RDATA_FLAGS) for p in POOL]
    report = run(tmp_path, split_pool=split)
    assert any(g.startswith("january-excluded-record-inconsistent: _rdata_00001000")
               for g in report["global_failures"])
    assert report["exit_code"] == EXIT_FAILURES


def test_extra_split_section_is_a_global_failure(tmp_path):
    report = run(tmp_path, split_pool=POOL + [("_extra", 0x3000, 4, bss_flags(4))])
    assert any(g.startswith("split pool has 6 sections") for g in report["global_failures"])
    assert report["exit_code"] == EXIT_FAILURES


def test_ledger_only_run_refused_even_when_every_vendor_record_is_relabelled(tmp_path):
    # RF-DH F2 behaviourally: with no vendor record left in the ledger, only the library requirement
    # stands between a relabelled vendor record and the Halo tally
    relabelled = ledger_with(_delta={"segment": "halo", "category": "halobetacache", "tag": "unresolved",
                                     "owner": None, "readouts": [], "vendor_evidence": KeyError})
    write_world(tmp_path, ledger=relabelled)
    assert main(cli_args(tmp_path, xdk_lib_dir=True)) == EXIT_LIBRARIES
    receipt = receipt_of(tmp_path)
    assert receipt["summary"] is None and "--xdk-lib-dir is required" in receipt["fail_closed"]["message"]


# -- independent re-review (RF-DX): D6 completion (every exit path receipted), D11 ------------

def test_unanticipated_exception_is_receipted_never_status_1(tmp_path, monkeypatch):
    import tools.common_pool_verify as module
    write_world(tmp_path)

    def broken(*_args, **_kwargs):
        raise RuntimeError("verifier defect (test)")
    monkeypatch.setattr(module, "january_pool", broken)
    assert main(cli_args(tmp_path)) == EXIT_UNREADABLE
    receipt = receipt_of(tmp_path)
    assert receipt["exit_code"] == EXIT_UNREADABLE and receipt["summary"] is None
    assert receipt["fail_closed"]["class"] == "unreadable-input"
    assert receipt["fail_closed"]["message"] == "unexpected RuntimeError: verifier defect (test)"
    assert not (tmp_path / "out.json").exists()


def test_index_error_is_malformed_input_and_receipted(tmp_path, monkeypatch):
    import tools.common_pool_verify as module
    write_world(tmp_path)

    def broken(*_args, **_kwargs):
        raise IndexError("index out of range (test)")
    monkeypatch.setattr(module, "january_pool", broken)
    assert main(cli_args(tmp_path)) == EXIT_MALFORMED
    receipt = receipt_of(tmp_path)
    assert receipt["fail_closed"]["class"] == "malformed-types"
    assert receipt["fail_closed"]["message"] == "index out of range (test)"


@pytest.mark.parametrize("output", ["text", "json"])
def test_unwritable_report_fails_closed_and_the_receipt_says_so(tmp_path, output):
    write_world(tmp_path)
    args = cli_args(tmp_path)
    args[args.index("--" + output) + 1] = str(tmp_path / "no_such_dir" / f"out.{output}")
    assert main(args) == EXIT_UNREADABLE
    receipt = receipt_of(tmp_path)
    assert receipt["exit_code"] == EXIT_UNREADABLE and receipt["result"] == "FAIL-CLOSED"
    assert receipt["summary"] is None
    assert receipt["fail_closed"]["message"].startswith("cannot write the report: FileNotFoundError")
    # the JSON report is written before the text report: an exit-2 run leaves no accounting behind
    assert not (tmp_path / "out.json").exists()


def test_unwritable_receipt_leaves_no_report_behind(tmp_path, capsys):
    write_world(tmp_path)
    args = cli_args(tmp_path)
    args[args.index("--receipt") + 1] = str(tmp_path / "no_such_dir" / "receipt.json")
    assert main(args) == EXIT_UNREADABLE
    assert "cannot write the receipt" in capsys.readouterr().err
    # the complete run's reports were written first; with no receipt they must not survive
    assert not (tmp_path / "out.json").exists() and not (tmp_path / "out.txt").exists()


def test_unlistable_library_directory_is_a_library_failure(tmp_path, monkeypatch):
    write_world(tmp_path)
    lib_dir = (tmp_path / "xdk").resolve()
    real_iterdir = Path.iterdir

    def iterdir(self):
        if self.resolve() == lib_dir:
            raise PermissionError(13, "Permission denied (test)", str(self))
        return real_iterdir(self)
    monkeypatch.setattr(Path, "iterdir", iterdir)
    assert main(cli_args(tmp_path)) == EXIT_LIBRARIES
    receipt = receipt_of(tmp_path)
    assert receipt["fail_closed"]["class"] == "libraries-missing-or-unverifiable"
    assert "cannot list --xdk-lib-dir" in receipt["fail_closed"]["message"]


@pytest.mark.parametrize("member", [
    build_coff(sections=[], symbols=[common("_delta", 4)]),                     # a second COMMON definer
    build_coff(machine=0, sections=[], symbols=[{"name": "_delta", "value": 0, "section": 0,
                                                 "storage": 105}]),           # a weak alias
])
def test_vendor_record_defined_by_another_linked_library_fails_closed(tmp_path, member):
    libs = default_libs()
    libs["other.lib"] = [("obj\\i386\\other.obj", member)]
    report = run(tmp_path, libs=libs)
    assert fail_closed_names(report) == {"_delta"}
    assert "vendor-record-defined-by-another-library" in reason_keys(rows_by_name(report)["_delta"])


# -- optional (RF-DX, D2 residual): the reviewed XDK manifest is pinned ---------------------

@pytest.mark.skipif(not REPO_MANIFEST.is_file(), reason="research XDK manifest not present")
def test_manifest_pin_is_the_research_manifest():
    data = REPO_MANIFEST.read_bytes().replace(b"\r\n", b"\n")
    assert hashlib.sha256(data).hexdigest() == PINNED_MANIFEST


def test_manifest_pin_refuses_any_other_manifest(tmp_path, monkeypatch):
    import tools.common_pool_verify as module
    write_world(tmp_path)
    monkeypatch.setattr(module, "XDK_MANIFEST_SHA256_LF", PINNED_MANIFEST)   # a well-formed, consistent manifest
    assert main(cli_args(tmp_path)) == EXIT_LIBRARIES
    receipt = receipt_of(tmp_path)
    assert receipt["fail_closed"]["class"] == "libraries-missing-or-unverifiable" and receipt["summary"] is None
    assert "is not the reviewed manifest" in receipt["fail_closed"]["message"]


def test_manifest_pin_accepts_its_own_manifest_with_either_line_end(tmp_path, monkeypatch):
    import tools.common_pool_verify as module
    write_world(tmp_path)
    path = _manifest_path(tmp_path)
    path.write_bytes(json.dumps(json.loads(path.read_text(encoding="utf-8")), indent=1).encode("utf-8"))
    monkeypatch.setattr(module, "XDK_MANIFEST_SHA256_LF", hashlib.sha256(path.read_bytes()).hexdigest())
    assert verify(**paths(tmp_path))["result"] == "COMPLETE"
    path.write_bytes(path.read_bytes().replace(b"\n", b"\r\n"))
    assert verify(**paths(tmp_path))["result"] == "COMPLETE"


# -- the mutation catalogue ----------------------------------------------------------------

@pytest.mark.skipif(not MUTATIONS.is_file(), reason="mutation catalogue not present")
def test_every_mutation_names_existing_regression_tests():
    catalogue = json.loads(MUTATIONS.read_text(encoding="utf-8"))
    tests = {name for name in globals() if name.startswith("test_")}
    ids = [m["id"] for m in catalogue["mutations"]]
    assert len(ids) == len(set(ids))
    origins = {}
    for mutation in catalogue["mutations"]:
        origins[mutation["origin"]] = origins.get(mutation["origin"], 0) + 1
        assert mutation["killers"], mutation["id"]
        assert set(mutation["killers"]) <= tests, (mutation["id"], set(mutation["killers"]) - tests)
    assert origins.get("RF-DE") == 43 and origins.get("RF-DH") == 16
