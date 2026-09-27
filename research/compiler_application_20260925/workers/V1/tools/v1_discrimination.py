"""Which adversarial scenarios each verifier (and each V1 mutant) gets wrong.

Run from the scratch repository copy whose tools/ holds the V1 verifier and
the V1 test module:

  python -B <this> --stock <stock.py> --dv2 <dv2.py> --out <dir>

For every scenario in tools/test_semantic_progress.py:V1_SCENARIOS it records
the outcome under the stock verifier (lane HEAD), DV2 (fc53d5f6) and V1, and
whether that outcome meets the scenario's expectation.  It then builds one
mutant of V1 per hardening check (the check disabled by a single textual
edit) and lists the scenarios that detect it.  A mutant no scenario detects
would mean the check is untested.
"""
import argparse
import json
import re
import sys
import tempfile
import types
from pathlib import Path

ROOT = Path.cwd()
sys.path.insert(0, str(ROOT))

from tools import semantic_progress as v1  # noqa: E402
from tools import test_semantic_progress as suite  # noqa: E402

V1_PATH = ROOT / "tools" / "semantic_progress.py"

# (name, old text, new text): each disables exactly one V1 check.
MUTANTS = [
    ("no_coverage_any_path",
     "            touched = _objdiff_group_coverage(\n"
     "                target, target_section_numbers,\n"
     "                f\"{unit_name}:{section_label}\")\n",
     "            touched = {name: sections for name, sections in\n"
     "                       _objdiff_report_data_groups(target).items()\n"
     "                       if {int(s['index']) for s in sections}\n"
     "                       & target_section_numbers}\n"),
    ("no_owner_identity",
     "        if int(target_owner[key]) != int(base_owner[key]):",
     "        if False:"),
    ("no_symbol_table_check",
     "    if _defined_section_symbols(target, target_owner[\"section\"]) \\\n"
     "            != _defined_section_symbols(base, base_owner[\"section\"]):",
     "    if False:"),
    ("no_comdat_selection_check",
     "    if target_selection != base_selection:",
     "    if False:"),
    ("no_image_address_requirement",
     "            if relocation[\"target\"][0] != \"address\":",
     "            if False:"),
    ("no_report_binding",
     "                _require_report_binding(\n"
     "                    target, report_unit, unit_name, section_label)\n",
     ""),
    ("no_surplus_verification",
     "                _verify_declared_surplus(\n"
     "                    target, base, touched, base_section_numbers,\n"
     "                    entry.get(\"surplus\"), unit_name, section_label)\n",
     ""),
    ("no_undeclared_scan",
     "    if undeclared:\n        names = [",
     "    if False:\n        names = ["),
    ("no_one_entry_per_unit",
     "                if unit_entry_counts[unit_name] != 1:",
     "                if False:"),
    ("no_entry_key_whitelist",
     "                if unknown:",
     "                if False:"),
    ("no_member_key_whitelist",
     "                if extent_model is not None and (\n"
     "                        not isinstance(member, dict)",
     "                if False and (\n"
     "                        not isinstance(member, dict)"),
    ("model_sum_instead_of_extent",
     "                credited_size = sum(grouped_sections.values())\n",
     "                credited_size = credited_size\n"),
    ("no_surplus_target_definition_check",
     "        if any(entry[\"name\"] == symbol_name and int(entry[\"section\"]) > 0\n"
     "               for entry in target[\"symbols\"]):",
     "        if False:"),
    ("no_surplus_comdat_check",
     "        if not int(section[\"flags\"]) & _IMAGE_SCN_LNK_COMDAT \\\n"
     "                or selection != _IMAGE_COMDAT_SELECT_ANY:",
     "        if False:"),
    ("no_surplus_single_symbol_check",
     "        if int(owner[\"value\"]) != 0 \\\n"
     "                or _defined_section_symbols(base, number) != expected_symbols:",
     "        if False:"),
    ("no_surplus_snapshot_pin",
     "        if item[\"measurements\"] != {\"base\": snapshot}:",
     "        if False:"),
]


def module_from_source(name, source, path):
    source = source.replace(
        "from .coff_compare import", "from tools.coff_compare import")
    module = types.ModuleType(name)
    module.__file__ = str(path)
    exec(compile(source, str(path), "exec"), module.__dict__)
    return module


def outcome(scenario, verifier):
    with tempfile.TemporaryDirectory() as root:
        try:
            notes, report, before = suite.run_v1_scenario(scenario, verifier, root)
        except Exception as error:  # noqa: BLE001 - recorded
            return {"kind": "reject", "error_type": type(error).__name__,
                    "message": str(error)}
        if not notes:
            return {"kind": "noop" if report == before else "changed-without-note"}
        size = int(re.search(r"\+(\d+) data bytes", notes[-1]).group(1))
        return {"kind": "credit", "bytes": size, "notes": notes}


def meets(expect, result, error_type):
    if expect == "noop":
        return result["kind"] == "noop"
    if expect.startswith("credit "):
        return result["kind"] == "credit" \
            and result["bytes"] == int(expect.split()[1])
    return result["kind"] == "reject" \
        and result.get("error_type") == error_type \
        and re.search(expect, result["message"]) is not None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--stock", type=Path, required=True)
    parser.add_argument("--dv2", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    verifiers = {
        "stock": module_from_source("sp_stock", args.stock.read_text(encoding="utf-8"), args.stock),
        "dv2": module_from_source("sp_dv2", args.dv2.read_text(encoding="utf-8"), args.dv2),
        "v1": v1,
    }
    matrix = {}
    for scenario in suite.V1_SCENARIOS:
        row = {"expect": scenario.expect}
        for name, module in verifiers.items():
            result = outcome(scenario, module)
            result["meets_expectation"] = meets(
                scenario.expect, result, "SemanticProgressError")
            row[name] = result
        matrix[scenario.__name__] = row

    source = V1_PATH.read_text(encoding="utf-8")
    mutants = {}
    for name, old, new in MUTANTS:
        count = source.count(old)
        if count != 1:
            mutants[name] = {"error": f"anchor found {count} times"}
            continue
        module = module_from_source(f"sp_mutant_{name}", source.replace(old, new), V1_PATH)
        killed = []
        for scenario in suite.V1_SCENARIOS:
            result = outcome(scenario, module)
            if not meets(scenario.expect, result, "SemanticProgressError"):
                killed.append(scenario.__name__)
        mutants[name] = {"killed_by": killed, "survived": not killed}

    summary = {
        "scenarios": len(matrix),
        "v1_meets_all": all(row["v1"]["meets_expectation"] for row in matrix.values()),
        "stock_wrong": sorted(k for k, r in matrix.items() if not r["stock"]["meets_expectation"]),
        "dv2_wrong": sorted(k for k, r in matrix.items() if not r["dv2"]["meets_expectation"]),
        "stock_credits_a_negative": sorted(
            k for k, r in matrix.items()
            if not r["expect"].startswith(("credit", "noop"))
            and r["stock"]["kind"] == "credit"),
        "dv2_credits_a_negative": sorted(
            k for k, r in matrix.items()
            if not r["expect"].startswith(("credit", "noop"))
            and r["dv2"]["kind"] == "credit"),
        "mutants_survived": sorted(k for k, m in mutants.items() if m.get("survived") or m.get("error")),
    }
    (args.out / "discrimination.json").write_text(json.dumps(
        {"summary": summary, "matrix": matrix, "mutants": mutants}, indent=1) + "\n",
        encoding="utf-8", newline="\n")
    print(json.dumps(summary, indent=1))
    width = max(len(k) for k in matrix)
    for name, row in matrix.items():
        cells = []
        for verifier in ("stock", "dv2", "v1"):
            result = row[verifier]
            text = result["kind"] + (f" {result['bytes']}" if result["kind"] == "credit" else "")
            cells.append(("ok  " if result["meets_expectation"] else "MISS") + " " + text)
        print(f"{name:{width}s} | {row['expect'][:40]:40s} | " + " | ".join(f"{c:18s}" for c in cells))
    for name, value in mutants.items():
        print(f"mutant {name:36s} {value}")


if __name__ == "__main__":
    main()
