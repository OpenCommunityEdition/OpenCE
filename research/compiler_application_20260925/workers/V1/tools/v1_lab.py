"""V1 review lab: the extent-model verifier on the real build, stage by stage.

Run from the SCRATCH repository copy (scratch/campaign/workers/V1/repo), whose
tools/semantic_progress.py is the V1 verifier and whose build/ and config/ are
the snapshot of lane HEAD.  Nothing outside --out is written.

Verifiers:  stock = lane HEAD blob 9a7a5129 (LF sha256 aea695ea...),
            dv2   = fc53d5f6:tools/semantic_progress.py (LF sha256 fa196fa5...),
            v1    = ./tools/semantic_progress.py.
Every run mirrors tools/project_x86.py calculate_progress: rejections ->
semantic matches -> accepted ledger -> DATA MATCHES -> ownership snapshots ->
revocations -> parked validation, on a fresh copy of build/report.json.

  python -B <this> --stock <stock.py> --dv2 <dv2.py> --entries-dir <dir>
         --proposed-hs <B1 json> --proposed-actions <fc53d5f6 json> --out <dir>
"""
import argparse
import copy
import hashlib
import json
import shutil
import sys
import traceback
import types
from pathlib import Path

ROOT = Path.cwd()
sys.path.insert(0, str(ROOT))

from tools import semantic_progress as v1  # noqa: E402
from tools import audit_object_admission  # noqa: E402
from tools import regression_gate  # noqa: E402
from tools.coff_compare import load  # noqa: E402
from tools.parked_functions import require_valid_parked_functions  # noqa: E402

MEASURE_KEYS = ("total_code", "matched_code", "total_data", "matched_data",
                "total_functions", "matched_functions", "complete_code",
                "complete_data", "complete_units", "total_units")
HS = "source/hs/hs"
ACTIONS = "source/ai/actions"


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def sha256_file(path):
    return sha256_bytes(Path(path).read_bytes())


def module_from_file(name, path):
    source = Path(path).read_text(encoding="utf-8").replace(
        "from .coff_compare import", "from tools.coff_compare import")
    module = types.ModuleType(name)
    module.__file__ = str(path)
    exec(compile(source, str(path), "exec"), module.__dict__)
    return module


def pipeline(module, manifest_path, objdiff_path=None, report_path=None):
    report_path = report_path or ROOT / "build" / "report.json"
    objdiff_path = objdiff_path or ROOT / "objdiff.json"
    report = json.loads(report_path.read_text(encoding="utf-8"))

    def convert(data):
        for key, value in data.items():
            if isinstance(value, str) and value.isdigit():
                data[key] = int(value)
    convert(report["measures"])
    for category in report.get("categories", []):
        convert(category["measures"])
    notes = {
        "rejections": module.apply_semantic_rejections(
            report, ROOT / "build" / "semantic_report.json"),
        "matches": module.apply_semantic_matches(
            report, ROOT, ROOT / "config" / "semantic_matches.json",
            objdiff_path),
        "accepted": module.apply_semantic_accepted_ledger(
            report, ROOT / "build" / "semantic_report.json"),
        "data_matches": module.apply_semantic_data_matches(
            report, ROOT, manifest_path, objdiff_path,
            ROOT / "config" / "symbols.json"),
        "ownership": module.require_symbol_ownership_snapshots(
            ROOT, ROOT / "config" / "symbol_ownership.json", objdiff_path),
        "revoked": module.revoke_incomplete_units(report),
    }
    parked = require_valid_parked_functions(
        ROOT, report_path, objdiff_path, ROOT / "config" / "parked.json")
    notes["parked_active"] = len(parked["active"])
    return report, notes


def totals(report):
    halo = {c["id"]: c for c in report["categories"]}["halobetacache"]["measures"]
    pick = lambda m: {k: int(m.get(k, 0) or 0) for k in MEASURE_KEYS}  # noqa: E731
    return {"all": pick(report["measures"]), "halobetacache": pick(halo)}


def unit_measures(report):
    return {u["name"]: {k: int(u.get("measures", {}).get(k, 0) or 0)
                        for k in MEASURE_KEYS}
            for u in report["units"]}


def run(label, module, manifest_path, results, **kwargs):
    try:
        report, notes = pipeline(module, manifest_path, **kwargs)
    except Exception as error:  # noqa: BLE001 - recorded, not hidden
        results[label] = {
            "status": "REJECTED",
            "error_type": type(error).__name__,
            "error": str(error),
        }
        return None
    results[label] = {
        "status": "OK",
        "totals": totals(report),
        "data_notes": notes["data_matches"],
        "other_notes": {k: v for k, v in notes.items() if k != "data_matches"},
    }
    return report


def compare(results, left, right, reports):
    if results[left]["status"] != "OK" or results[right]["status"] != "OK":
        return None
    a, b = unit_measures(reports[left]), unit_measures(reports[right])
    changed = {name: {k: (a[name][k], b[name][k]) for k in MEASURE_KEYS
                      if a[name][k] != b[name][k]}
               for name in a if a[name] != b[name]}
    ta, tb = results[left]["totals"], results[right]["totals"]
    delta = {scope: {k: tb[scope][k] - ta[scope][k] for k in MEASURE_KEYS
                     if tb[scope][k] != ta[scope][k]} for scope in ta}
    return {
        "changed_units": changed,
        "total_delta": delta,
        "other_notes_identical":
            results[left]["other_notes"] == results[right]["other_notes"],
        "data_notes_added": [n for n in results[right]["data_notes"]
                             if n not in results[left]["data_notes"]],
        "data_notes_removed": [n for n in results[left]["data_notes"]
                               if n not in results[right]["data_notes"]],
        "prior_data_notes_in_order":
            results[right]["data_notes"][:len(results[left]["data_notes"])]
            == results[left]["data_notes"],
    }


def write_manifest(path, entries):
    path.write_text(json.dumps(entries, indent=1) + "\n", encoding="utf-8",
                    newline="\n")
    return path


def subset_entry(entry):
    """Drop .rdata members whose LEGACY padded sizes sum to exactly 44."""
    subset = copy.deepcopy(entry)
    subset.pop("extent_model", None)
    subset.pop("surplus", None)
    dropped, total = [], 0
    for member in reversed(subset["members"]):
        snap = member["measurements"]["target"]
        if snap["section"] == ".rdata" and snap["padded_size"] == 4 \
                and total < 44:
            dropped.append(member["symbol"])
            total += 4
    subset["members"] = [m for m in subset["members"]
                         if m["symbol"] not in dropped]
    subset["group"] = "LAB-ONLY-subset-exploit"
    subset["reason"] = "LAB ONLY: loophole demonstration, never a proposal"
    return subset, dropped, total


def corrupt_member(obj_path, symbol, out_path):
    data = bytearray(Path(obj_path).read_bytes())
    obj = load(bytes(data))
    owner = [s for s in obj["symbols"] if s["name"] == symbol and s["section"] > 0][0]
    section = obj["sections"][owner["section"] - 1]
    data[section["raw"]] ^= 0x20  # flip one letter's case
    Path(out_path).write_bytes(bytes(data))


def objdiff_with(out, name, overrides):
    config = json.loads((ROOT / "objdiff.json").read_text(encoding="utf-8"))
    for unit in config["units"]:
        if unit["name"] in overrides:
            unit.update(overrides[unit["name"]])
    path = out / name
    path.write_text(json.dumps(config), encoding="utf-8")
    return path


def board_binding(out):
    report = json.loads((ROOT / "build" / "report.json").read_text(encoding="utf-8"))
    config = {u["name"]: u for u in json.loads(
        (ROOT / "objdiff.json").read_text(encoding="utf-8"))["units"]}
    ok, failures, skipped = 0, [], []
    for unit in report["units"]:
        cfg = config.get(unit["name"])
        if not cfg or not cfg.get("target_path") \
                or not (ROOT / cfg["target_path"]).is_file():
            skipped.append(unit["name"])
            continue
        try:
            v1._require_report_binding(
                load(ROOT / cfg["target_path"]), unit, unit["name"], "board")
            ok += 1
        except Exception as error:  # noqa: BLE001
            failures.append([unit["name"], str(error)])
    return {"units_bound": ok, "failures": failures, "skipped": len(skipped)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--stock", type=Path, required=True)
    parser.add_argument("--dv2", type=Path, required=True)
    parser.add_argument("--entries-dir", type=Path, required=True)
    parser.add_argument("--proposed-hs", type=Path, required=True)
    parser.add_argument("--proposed-actions", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    out = args.out
    out.mkdir(parents=True, exist_ok=True)

    stock = module_from_file("sp_stock", args.stock)
    dv2 = module_from_file("sp_dv2", args.dv2)
    production = json.loads(
        (ROOT / "config" / "semantic_data_matches.json").read_text(encoding="utf-8"))
    hs_v1 = json.loads((args.entries_dir / "hs_data_group_v1.json").read_text(encoding="utf-8"))
    actions_v1 = json.loads((args.entries_dir / "actions_data_group_v1.json").read_text(encoding="utf-8"))
    hs_prop = [e for e in json.loads(args.proposed_hs.read_text(encoding="utf-8"))
               if e["unit"] == HS]
    act_value = json.loads(args.proposed_actions.read_text(encoding="utf-8"))
    act_prop = act_value if isinstance(act_value, list) else [act_value]

    manifests = {
        "production": ROOT / "config" / "semantic_data_matches.json",
        "combined_v1": write_manifest(out / "combined_v1.json",
                                      production + hs_v1 + actions_v1),
        "hs_v1_only": write_manifest(out / "hs_v1_only.json", production + hs_v1),
        "actions_v1_only": write_manifest(out / "actions_v1_only.json",
                                          production + actions_v1),
        "combined_proposed": write_manifest(out / "combined_proposed.json",
                                            production + hs_prop + act_prop),
    }
    subset, dropped, dropped_total = subset_entry(hs_v1[0])
    subset_model = copy.deepcopy(subset)
    subset_model["extent_model"] = v1.OBJDIFF_331_COMBINED_EXTENT
    subset_model["surplus"] = hs_v1[0]["surplus"]
    manifests["subset_legacy"] = write_manifest(out / "subset_legacy.json",
                                                production + [subset])
    manifests["subset_model"] = write_manifest(out / "subset_model.json",
                                               production + [subset_model])
    corrupted = out / "hs_corrupted_dropped_literal.obj"
    corrupt_member(ROOT / "build" / "base" / "source" / "hs" / "hs.obj",
                   dropped[0], corrupted)
    corrupt_config = objdiff_with(out, "objdiff_corrupt.json", {
        HS: {"base_path": str(corrupted.resolve())}})

    results, reports = {}, {}
    plan = [
        ("A_stock_production", stock, "production", {}),
        ("B_v1_production", v1, "production", {}),
        ("B2_dv2_production", dv2, "production", {}),
        ("D_v1_combined", v1, "combined_v1", {}),
        ("Dh_v1_hs_only", v1, "hs_v1_only", {}),
        ("Da_v1_actions_only", v1, "actions_v1_only", {}),
        ("E_stock_combined_v1", stock, "combined_v1", {}),
        ("E2_stock_combined_proposed", stock, "combined_proposed", {}),
        ("F_dv2_combined_proposed", dv2, "combined_proposed", {}),
        ("F2_dv2_combined_v1", dv2, "combined_v1", {}),
        ("G_v1_combined_proposed", v1, "combined_proposed", {}),
        ("H1_stock_subset_legacy", stock, "subset_legacy", {}),
        ("H1c_stock_subset_legacy_corrupted", stock, "subset_legacy",
         {"objdiff_path": corrupt_config}),
        ("H2_dv2_subset_legacy", dv2, "subset_legacy", {}),
        ("H2c_dv2_subset_legacy_corrupted", dv2, "subset_legacy",
         {"objdiff_path": corrupt_config}),
        ("H2m_dv2_subset_model", dv2, "subset_model", {}),
        ("H3_v1_subset_legacy", v1, "subset_legacy", {}),
        ("H3c_v1_subset_legacy_corrupted", v1, "subset_legacy",
         {"objdiff_path": corrupt_config}),
        ("H3m_v1_subset_model", v1, "subset_model", {}),
    ]
    for label, module, manifest, kwargs in plan:
        reports[label] = run(label, module, manifests[manifest], results, **kwargs)

    comparisons = {
        "B_vs_A": compare(results, "A_stock_production", "B_v1_production", reports),
        "B2_vs_A": compare(results, "A_stock_production", "B2_dv2_production", reports),
        "D_vs_A": compare(results, "A_stock_production", "D_v1_combined", reports),
        "Dh_vs_A": compare(results, "A_stock_production", "Dh_v1_hs_only", reports),
        "Da_vs_A": compare(results, "A_stock_production", "Da_v1_actions_only", reports),
        "F_vs_A": compare(results, "A_stock_production", "F_dv2_combined_proposed", reports),
        "F2_vs_D": compare(results, "D_v1_combined", "F2_dv2_combined_v1", reports),
        "H1_vs_A": compare(results, "A_stock_production", "H1_stock_subset_legacy", reports),
        "H1c_vs_A": compare(results, "A_stock_production", "H1c_stock_subset_legacy_corrupted", reports),
        "H2_vs_A": compare(results, "A_stock_production", "H2_dv2_subset_legacy", reports),
        "H2c_vs_A": compare(results, "A_stock_production", "H2c_dv2_subset_legacy_corrupted", reports),
    }

    # January-vs-January control: base := January for hs and actions, no
    # surplus.  Data stage only (the parked/code stages are not meaningful
    # when the rebuilt object is replaced by the target).
    jj_config = objdiff_with(out, "objdiff_jj.json", {
        HS: {"base_path": "build/split/source/hs/hs.obj"},
        ACTIONS: {"base_path": "build/split/source/ai/actions.obj"}})
    jj_entries = []
    for entry in hs_v1 + actions_v1:
        item = copy.deepcopy(entry)
        item["surplus"] = []
        jj_entries.append(item)
    jj_manifest = write_manifest(out / "jj_manifest.json", jj_entries)
    report = json.loads((ROOT / "build" / "report.json").read_text(encoding="utf-8"))
    try:
        jj_notes = v1.apply_semantic_data_matches(
            report, ROOT, jj_manifest, jj_config, ROOT / "config" / "symbols.json")
        jj = {"status": "OK", "notes": jj_notes}
    except Exception as error:  # noqa: BLE001
        jj = {"status": "REJECTED", "error": str(error)}
    # The same control with the rebuilt-object surplus lists kept must fail:
    # January does not contain the declared surplus sections.
    report = json.loads((ROOT / "build" / "report.json").read_text(encoding="utf-8"))
    try:
        v1.apply_semantic_data_matches(
            report, ROOT, write_manifest(out / "jj_with_surplus.json", hs_v1 + actions_v1),
            jj_config, ROOT / "config" / "symbols.json")
        jj_surplus = {"status": "OK (UNEXPECTED)"}
    except Exception as error:  # noqa: BLE001
        jj_surplus = {"status": "REJECTED", "error": str(error)}

    # Admission audit on production vs combined manifests (V1 in tools/).
    audits = {}
    for name in ("production", "combined_v1"):
        result = audit_object_admission.audit(
            ROOT, ROOT / "build" / "report.json", ROOT / "objdiff.json",
            ROOT / "build" / "semantic_report.json",
            ROOT / "config" / "semantic_matches.json", manifests[name],
            ROOT / "config" / "symbols.json",
            ROOT / "config" / "object_admission_rejections.json")
        audits[name] = {"summary": result["summary"],
                        "candidates": [c["unit"] for c in result["candidates"]],
                        "revoked": result["revoked"]}

    # regression_gate exception records for grouped entries.
    gate = {}
    for unit in (HS, ACTIONS, "source/shell/shell_xbox"):
        try:
            regression_gate._exception_records(
                [unit], [], production + hs_v1 + actions_v1)
            gate[unit] = "OK"
        except Exception as error:  # noqa: BLE001
            gate[unit] = f"{type(error).__name__}: {error}"

    # Padding accounting for the credited sections.
    accounting = {}
    config = {u["name"]: u for u in json.loads(
        (ROOT / "objdiff.json").read_text(encoding="utf-8"))["units"]}
    for entry in hs_v1 + actions_v1:
        target = load(ROOT / config[entry["unit"]]["target_path"])
        numbers = {int(v1._unique_defined_symbol(target, m["symbol"], "t")["section"])
                   for m in entry["members"]}
        touched = v1._objdiff_group_coverage(target, numbers, entry["unit"])
        accounting[entry["unit"]] = {
            name: {"extent": v1._objdiff_report_extent(sections),
                   "sections": len(sections),
                   "raw_bytes": sum(int(s["size"]) for s in sections),
                   "padding_bytes": v1._objdiff_report_extent(sections)
                   - sum(int(s["size"]) for s in sections)}
            for name, sections in touched.items()}
        accounting[entry["unit"]]["surplus_sections"] = len(entry["surplus"])
        accounting[entry["unit"]]["surplus_bytes_not_credited"] = sum(
            s["measurements"]["base"]["size"] for s in entry["surplus"])

    summary = {
        "root": str(ROOT),
        "inputs": {
            "report.json": sha256_file(ROOT / "build" / "report.json"),
            "semantic_report.json": sha256_file(ROOT / "build" / "semantic_report.json"),
            "objdiff.json": sha256_file(ROOT / "objdiff.json"),
            "production_manifest": sha256_file(manifests["production"]),
            "symbols.json": sha256_file(ROOT / "config" / "symbols.json"),
            "stock_verifier": sha256_file(args.stock),
            "dv2_verifier": sha256_file(args.dv2),
            "v1_verifier": sha256_file(ROOT / "tools" / "semantic_progress.py"),
            "hs_v1_entry": sha256_file(args.entries_dir / "hs_data_group_v1.json"),
            "actions_v1_entry": sha256_file(args.entries_dir / "actions_data_group_v1.json"),
            "proposed_hs": sha256_file(args.proposed_hs),
            "proposed_actions": sha256_file(args.proposed_actions),
            "combined_v1_manifest": sha256_file(manifests["combined_v1"]),
            "hs.obj split/base": [sha256_file(ROOT / "build/split/source/hs/hs.obj"),
                                  sha256_file(ROOT / "build/base/source/hs/hs.obj")],
            "actions.obj split/base": [sha256_file(ROOT / "build/split/source/ai/actions.obj"),
                                       sha256_file(ROOT / "build/base/source/ai/actions.obj")],
        },
        "subset_exploit": {"dropped_members": dropped,
                           "dropped_legacy_padded_bytes": dropped_total,
                           "corrupted_member_in_scratch_copy": dropped[0]},
        "runs": results,
        "comparisons": comparisons,
        "jan_vs_jan_control": jj,
        "jan_vs_jan_with_rebuilt_surplus": jj_surplus,
        "admission_audit": audits,
        "regression_gate_exception_records": gate,
        "padding_accounting": accounting,
        "board_binding": board_binding(out),
    }
    (out / "lab_summary.json").write_text(json.dumps(summary, indent=1) + "\n",
                                          encoding="utf-8", newline="\n")
    for label, value in results.items():
        line = value["status"]
        if value["status"] == "OK":
            line += f"  halo data {value['totals']['halobetacache']['matched_data']}"
        else:
            line += f"  {value['error'][:150]}"
        print(f"{label:40s} {line}")
    for name, value in comparisons.items():
        if value is None:
            print(f"{name:12s} n/a")
            continue
        print(f"{name:12s} delta={value['total_delta']} units={list(value['changed_units'])} "
              f"other_notes_identical={value['other_notes_identical']} "
              f"prior_notes_in_order={value['prior_data_notes_in_order']}")
    print("J-vs-J", jj)
    print("J-vs-J with rebuilt surplus", jj_surplus)
    print("audits", json.dumps(audits))
    print("gate", gate)
    print("accounting", json.dumps(accounting))
    print("binding", json.dumps(summary["board_binding"])[:2000])


if __name__ == "__main__":
    main()
