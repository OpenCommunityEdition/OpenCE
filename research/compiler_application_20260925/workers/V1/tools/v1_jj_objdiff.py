"""January-vs-January control scored by the frozen objdiff-cli 3.3.1 itself.

Builds an objdiff project in --work whose base objects are byte copies of
January's split hs.obj and actions.obj, runs `objdiff-cli report generate`
(the production binary, sha1 3130e428...), and feeds that report to the V1
verifier with the V1 entries (surplus emptied: January has no surplus).  The
scorer cannot give January 100% against itself (the '$' literal-name defect),
so the verifier must credit exactly the scorer's own gap and nothing else.

Run from the scratch repository copy:
  python -B <this> --objdiff build/tools/objdiff-cli.exe --entries-dir <dir> --work <dir>
"""
import argparse
import copy
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path.cwd()
sys.path.insert(0, str(ROOT))

from tools import semantic_progress as v1  # noqa: E402

UNITS = {
    "source/hs/hs": ("build/split/source/hs/hs.obj", "hs"),
    "source/ai/actions": ("build/split/source/ai/actions.obj", "actions"),
}


def sha1(path):
    return hashlib.sha1(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--objdiff", type=Path, required=True)
    parser.add_argument("--entries-dir", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    args = parser.parse_args()
    work = args.work.resolve()
    if work.exists():
        shutil.rmtree(work)
    (work / "target").mkdir(parents=True)
    (work / "base").mkdir()
    units = []
    for name, (split, short) in UNITS.items():
        shutil.copyfile(ROOT / split, work / "target" / f"{short}.obj")
        shutil.copyfile(ROOT / split, work / "base" / f"{short}.obj")
        units.append({
            "name": name, "target_path": f"target/{short}.obj",
            "base_path": f"base/{short}.obj",
            "metadata": {"progress_categories": ["halobetacache"]}})
    config = {"min_version": "2.0.0-beta.5",
              "progress_categories": [{"id": "halobetacache", "name": "halobetacache"}],
              "units": units}
    (work / "objdiff.json").write_text(json.dumps(config, indent=1), encoding="utf-8")
    subprocess.run([str(args.objdiff.resolve()), "report", "generate", "-p", str(work),
                    "-o", str(work / "report_331.json")], check=True)
    report = json.loads((work / "report_331.json").read_text(encoding="utf-8"))
    for key, value in list(report["measures"].items()):
        if isinstance(value, str) and value.isdigit():
            report["measures"][key] = int(value)
    for category in report.get("categories", []):
        for key, value in list(category["measures"].items()):
            if isinstance(value, str) and value.isdigit():
                category["measures"][key] = int(value)
    pristine = copy.deepcopy(report)
    before = {u["name"]: dict(u["measures"]) for u in report["units"]}
    entries = []
    for file_name in ("hs_data_group_v1.json", "actions_data_group_v1.json"):
        for entry in json.loads((args.entries_dir / file_name).read_text(encoding="utf-8")):
            entry = copy.deepcopy(entry)
            entry["surplus"] = []
            entries.append(entry)
    result = {"objdiff_sha1": sha1(args.objdiff),
              "scorer_units": {name: {"measures": {k: v for k, v in m.items() if "data" in k},
                                      "sections": [(s["name"], s["size"], s.get("fuzzy_match_percent"))
                                                   for s in next(u for u in report["units"] if u["name"] == name)["sections"]]}
                               for name, m in before.items()}}

    def attempt(label, manifest_entries):
        trial = copy.deepcopy(pristine)
        path = work / f"manifest_{label}.json"
        path.write_text(json.dumps(manifest_entries), encoding="utf-8")
        try:
            notes = v1.apply_semantic_data_matches(
                trial, work, path, work / "objdiff.json",
                ROOT / "config" / "symbols.json")
            result[label] = {
                "status": "OK", "notes": notes,
                "after": {u["name"]: {k: v for k, v in u["measures"].items() if "data" in k}
                          for u in trial["units"]}}
        except v1.SemanticProgressError as error:
            result[label] = {"status": "REJECTED", "error": str(error)}

    # 1. the full entries: a section the scorer already matches must not be
    #    covered again, so an entry spanning it is rejected.
    for entry in entries:
        attempt(f"full_{entry['unit'].split('/')[-1]}", [entry])
    # 2. entries scoped to exactly the sections the scorer leaves unmatched.
    from tools.coff_compare import load
    scoped = []
    for entry in entries:
        unit = next(u for u in pristine["units"] if u["name"] == entry["unit"])
        unmatched = {s["name"] for s in unit["sections"]
                     if s["name"] != ".text" and float(s.get("fuzzy_match_percent", 0)) != 100.0}
        target = load(work / next(u for u in units if u["name"] == entry["unit"])["target_path"])
        groups = v1._objdiff_report_data_groups(target)
        keep_numbers = {int(sec["index"]) for name, secs in groups.items()
                        if name in unmatched for sec in secs}
        item = copy.deepcopy(entry)
        item["members"] = [m for m in entry["members"]
                           if int(v1._unique_defined_symbol(target, m["symbol"], "t")["section"])
                           in keep_numbers]
        result.setdefault("scoped_member_counts", {})[entry["unit"]] = len(item["members"])
        scoped.append(item)
    attempt("scoped", scoped)
    (work / "jj_result.json").write_text(json.dumps(result, indent=1) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=1))


if __name__ == "__main__":
    main()
