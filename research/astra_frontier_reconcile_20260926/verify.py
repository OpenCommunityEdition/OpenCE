"""Independent two-TU reconciliation checks; no production verifier changes.

Run from the repository root with the saved pre-edit objects in OUT.
"""
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path.cwd()
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

BASE = "8cda1f91f3a037f52ac2eed551936b03df6796c1"
OUT = ROOT / "scratch/reconcile_frontier_20260926"
UNITS = {
    "source/ai/actor_moving": "_actor_path_refresh",
    "source/interface/ui_widget": "_display_scenario_help",
}


def storage(obj):
    return sorted((s["name"], s["section"], s["value"], s["storage"], s["type"])
                  for s in obj["symbols"] if s["storage"] in (2, 3)
                  and not (s["name"].startswith("$L") and s["storage"] == 3
                           and s["type"] == 0 and s["section"] > 0
                           and obj["sections"][s["section"] - 1]["name"] == ".text"))


def warning_check(unit):
    ninja = (ROOT / "build.ninja").read_text()
    key = "build\\base\\" + unit.replace("/", "\\") + ".obj:"
    start = ninja.index("cflags = ", ninja.index(key)) + len("cflags = ")
    end = ninja.index("\nbuild ", start)
    flags = re.sub(r"\s+", " ", ninja[start:end].replace("$\n", " ")).strip()
    tokens = re.findall(r'/I"[^"]+"|\S+', flags)
    tokens = [("/I" + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in tokens]
    counters = []
    for label in ("before", "after"):
        src = ROOT / (unit + ".c")
        if label == "before":
            src = OUT / ("before_" + Path(unit).name + ".c")
            src.write_bytes(subprocess.check_output(
                ["git", "show", BASE + ":" + unit + ".c"], cwd=ROOT))
        command = [str(ROOT / "xbox/bin/vc7/CL.Exe"), "/nologo", "/c"] + tokens
        command += ["/W3", "/I" + str((ROOT / unit).parent),
                    "/Fo" + str(OUT / (label + "_w3_" + Path(unit).name + ".obj")), str(src)]
        run = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        output = run.stdout + run.stderr
        (OUT / (label + "_w3_" + Path(unit).name + ".log")).write_text(output)
        if run.returncode:
            raise RuntimeError(output)
        warnings = []
        for line in output.splitlines():
            if re.search(r"warning C\d{4}", line):
                line = line.replace(str(src), "<source>").replace(str(ROOT), "<root>")
                warnings.append(re.sub(r"\(\d+\)", "(N)", line).replace("\\", "/"))
        counters.append(Counter(warnings))
    before, after = counters
    new = sorted((after - before).elements())
    if new:
        raise RuntimeError("New warnings: " + repr(new))
    return {"before": sum(before.values()), "after": sum(after.values()), "new": new}


def main():
    results = []
    report = json.loads((ROOT / "build/semantic_report.json").read_text())
    for unit, function in UNITS.items():
        before_path = OUT / ("before_" + Path(unit).name + ".obj")
        after_path = ROOT / "build/base" / (unit + ".obj")
        target_path = ROOT / "build/split" / (unit + ".obj")
        before, after, target = map(cc.load, (before_path, after_path, target_path))
        assert len(before["sections"]) == len(after["sections"])
        assert storage(before) == storage(after), "Storage/ownership changed"
        target_symbol = cc.symbol(after, function)
        changed = []
        for number, (old, new) in enumerate(zip(before["sections"], after["sections"]), 1):
            assert (old["name"], old["flags"]) == (new["name"], new["flags"])
            if old["name"].startswith(".debug"):
                continue
            if not cc.section_infos_equal(cc.section_info_by_number(before, number),
                                          cc.section_info_by_number(after, number)):
                changed.append(number)
        assert changed == [target_symbol["section"]], changed
        january = cc.section_info(target, function)
        assert cc.section_infos_equal(january, cc.section_info(after, function))
        target_section = target["sections"][cc.symbol(target, function)["section"] - 1]
        raw = bytes(cc._section_bytes(target, target_section))
        trailing_nops = len(raw) - len(raw.rstrip(b"\x90"))
        credited = next(r for r in report["accepted_ledger"]
                        if r["unit"] == unit and r["function"] == function)
        results.append({
            "unit": unit, "function": function, "strict_exact": True,
            "padded_bytes": january["size"],
            "target_extent_excluding_trailing_nops": len(raw) - trailing_nops,
            "trailing_nops": trailing_nops, "ledger_code_bytes": credited["code_bytes"],
            "relocations": january["relocation_count"],
            "normalized_sha256": january["normalized_sha256"],
            "sections_compared": len(before["sections"]), "changed_sections": changed,
            "new_or_removed_code_data_common_owners": 0,
            "warnings": warning_check(unit),
            "object_hashes": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                              for p in (before_path, after_path, target_path)},
        })
    pinned_paths = [ROOT / (unit + ".c") for unit in UNITS]
    pinned_paths += [ROOT / p for p in (
        "build/tools/objdiff-cli.exe", "xbox/bin/vc7/CL.Exe", "tools/coff_compare.py",
        "tools/audit_semantic_matches.py", "config/parked.json")]
    pinned_paths += [OUT / p for p in (
        "before.json", "after.json", "before_semantic_report.json",
        "before_build.log", "after_build.log", "before_pytest.log", "after_pytest.log",
        "after_admission.json", "after_parks.json", "after_fake.log")]
    summary = {
        "base": BASE,
        "donor_commits": ["9ed00bf758887d22ee67f9d1e8add23f317b2fc1",
                          "9f0b3af9413e6dc5c1be28786d22f0ea7df6c119"],
        "functions": results,
        "new_non_tail_padding_bytes": sum(r["target_extent_excluding_trailing_nops"] for r in results),
        "new_padded_bytes": sum(r["padded_bytes"] for r in results),
        "new_frozen_ledger_bytes": sum(r["ledger_code_bytes"] for r in results),
        "sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                   for p in pinned_paths},
    }
    (OUT / "verification.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
