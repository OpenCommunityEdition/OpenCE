import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
report = json.loads((ROOT / "build/report.json").read_text(encoding="utf-8"))
semantic = json.loads(
    (ROOT / "build/semantic_report.json").read_text(encoding="utf-8")
)
accepted = {
    (entry["unit"], entry["function"])
    for entry in semantic.get("accepted_ledger", [])
}
accepted_count_by_unit = {}
for unit_name, _function_name in accepted:
    accepted_count_by_unit[unit_name] = accepted_count_by_unit.get(unit_name, 0) + 1
ledger_text = "\n".join(
    path.read_text(errors="ignore")
    for path in (ROOT / "docs/object_matching_logs").glob("*.md")
).lower()
roots = [
    ROOT / "build/audit/refs/halocea/src",
    ROOT / "build/audit/refs/stian-halo/src/halo",
]
donor_paths = {}
for root in roots:
    for path in root.rglob("*"):
        if path.suffix.lower() not in (".c", ".cpp"):
            continue
        donor_paths.setdefault(path.name.lower(), []).append(str(path))
rows = []
for unit in report.get("units", []):
    if "halobetacache" not in unit.get("metadata", {}).get(
        "progress_categories", []
    ):
        continue
    for function in unit.get("functions", []):
        key = (unit["name"], function["name"])
        if key in accepted:
            continue
        if accepted_count_by_unit.get(unit["name"], 0) == 0:
            continue
        if function["name"].lower() in ledger_text:
            continue
        basename = function["name"].lstrip("_")
        hits = donor_paths.get((basename + ".c").lower(), [])[:3]
        hits += donor_paths.get((basename + ".cpp").lower(), [])[:3]
        if hits:
            rows.append(
                (int(function.get("size", 0)), unit["name"], function["name"], hits)
            )

print("ROWS", len(rows))
for size, unit_name, function_name, hits in sorted(rows)[:200]:
    print("%5d %s:%s" % (size, unit_name, function_name))
    for hit in hits:
        print("      " + hit)
