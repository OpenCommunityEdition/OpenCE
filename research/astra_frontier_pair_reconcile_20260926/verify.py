"""Recheck the widget/camera-impulse batch; never modify production inputs.

Run from the repository root after saving baseline receipts in OUT.
Uses the previously reviewed section/storage and diagnostic-compile utilities.
"""
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path.cwd()
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc
from research.astra_frontier_reconcile_20260926 import verify as previous

BASE = "cdf1c42ba6687083da2c7ad2730fdce57628d72d"
OUT = ROOT / "scratch/reconcile_frontier_pair_20260926"
UNITS = {
    "source/interface/ui_widget": "_widget_instance_render_recursive",
    "source/effects/player_effects": "_player_effect_update_camera_impulse",
}
DONORS = ["b659a777e4086d34ff3036b30bdf8bc6dcd92ce8",
          "06645ef469cbe43d3617f42e2f79f9b2f925f736"]


def main():
    previous.BASE, previous.OUT = BASE, OUT
    report = json.loads((ROOT / "build/semantic_report.json").read_text())
    results = []
    for unit, function in UNITS.items():
        paths = [OUT / ("before_" + Path(unit).name + ".obj"),
                 ROOT / "build/base" / (unit + ".obj"),
                 ROOT / "build/split" / (unit + ".obj")]
        before, after, target = map(cc.load, paths)
        assert len(before["sections"]) == len(after["sections"])
        assert previous.storage(before) == previous.storage(after)
        changed = []
        for number, (old, new) in enumerate(zip(before["sections"], after["sections"]), 1):
            assert (old["name"], old["flags"]) == (new["name"], new["flags"])
            if old["name"].startswith(".debug"):
                continue
            if not cc.section_infos_equal(cc.section_info_by_number(before, number),
                                          cc.section_info_by_number(after, number)):
                changed.append(number)
        assert changed == [cc.symbol(after, function)["section"]], changed
        january = cc.section_info(target, function)
        assert cc.section_infos_equal(january, cc.section_info(after, function))
        section = target["sections"][cc.symbol(target, function)["section"] - 1]
        raw = bytes(cc._section_bytes(target, section))
        tail = len(raw) - len(raw.rstrip(b"\x90"))
        credited = next(r for r in report["accepted_ledger"]
                        if r["unit"] == unit and r["function"] == function)
        warnings = previous.warning_check(unit)
        # Fresh /W3 compilation must also reproduce every non-debug section.
        for label, saved in (("before", before), ("after", after)):
            fresh = cc.load(OUT / (label + "_w3_" + Path(unit).name + ".obj"))
            assert len(fresh["sections"]) == len(saved["sections"])
            assert previous.storage(fresh) == previous.storage(saved)
            for number, old in enumerate(saved["sections"], 1):
                if old["name"].startswith(".debug"):
                    continue
                assert cc.section_infos_equal(cc.section_info_by_number(saved, number),
                                              cc.section_info_by_number(fresh, number))
        results.append({
            "unit": unit, "function": function, "strict_exact": True,
            "padded_bytes": january["size"],
            "non_tail_padding_bytes": len(raw) - tail, "trailing_nops": tail,
            "ledger_code_bytes": credited["code_bytes"],
            "relocations": january["relocation_count"],
            "normalized_sha256": january["normalized_sha256"],
            "sections_compared": len(before["sections"]), "changed_sections": changed,
            "new_or_removed_code_data_common_owners": 0,
            "warnings": warnings,
            "objects_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in paths},
        })
    assert (ROOT / "README.md").read_bytes() == (OUT / "inherited_README.md").read_bytes()
    snapshots = [json.loads((OUT / (name + ".json")).read_text())
                 for name in ("before", "after")]
    assert snapshots[0].keys() == snapshots[1].keys()
    gains, losses = [], []
    for key, old in snapshots[0].items():
        new = snapshots[1][key]
        if old["status"] != "E" and new["status"] == "E":
            gains.append(new["name"])
        if old["status"] == "E" and new["status"] != "E":
            losses.append(old["name"])
    assert not losses and set(gains) == set(UNITS.values())
    parks = json.loads((OUT / "after_parks.json").read_text())["summary"]
    assert parks == {"active": 74, "stale": 0, "invalid": 0}
    audits = [json.loads((OUT / (stage + "_admission.json")).read_text())["summary"]
              for stage in ("before", "after")]
    assert audits[0] == audits[1] and audits[1]["contradicted_count"] == 0
    scans = [json.loads((OUT / (stage + "_fake.json")).read_text())["findings"]
             for stage in ("before", "after")]
    keys = lambda rows: sorted((r["path"], r["rule"], r["snippet"]) for r in rows)
    assert keys(scans[0]) == keys(scans[1])
    for stage in ("before", "after"):
        assert "1161 passed, 5 skipped, 26 subtests passed" in (
            OUT / (stage + "_pytest.log")).read_text()
    pins = [ROOT / (u + ".c") for u in UNITS]
    pins += [ROOT / p for p in ("config/parked.json", "build/tools/objdiff-cli.exe",
                               "xbox/bin/vc7/CL.Exe", "tools/coff_compare.py", "build.ninja")]
    pins += [OUT / p for p in ("before.json", "after.json", "before_semantic_report.json",
                              "before_build.log", "after_build.log", "before_pytest.log",
                              "after_pytest.log", "after_admission.json", "after_parks.json",
                              "after_fake.json", "stable_diff.log")]
    summary = {
        "base": BASE, "donor_commits": DONORS, "functions": results,
        "new_non_tail_padding_bytes": sum(r["non_tail_padding_bytes"] for r in results),
        "new_padded_bytes": sum(r["padded_bytes"] for r in results),
        "new_ledger_bytes": sum(r["ledger_code_bytes"] for r in results),
        "stable_gains": gains, "regressions": losses, "parks": parks,
        "admission": audits[1], "unchanged_fake_leads": len(scans[1]),
        "pins_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                        for p in pins},
    }
    (OUT / "verification.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
