"""Build a strict exact-function ledger and quantify objdiff false negatives."""

import argparse
import json
from pathlib import Path

from tools.coff_compare import CoffError, load, section_info


def relocation_shape_matches(target_info, base_info):
    """Return whether objdiff's exact result has an identical COFF shape.

    Symbol ownership can legitimately differ between csplit output and the
    rebuilt object, but section size, normalized bytes, relocation count,
    relocation addresses, and relocation types must still agree.  This keeps
    objdiff from admitting code whose special relocations it did not model.
    """
    if target_info["size"] != base_info["size"]:
        return False
    if target_info["normalized_sha256"] != base_info["normalized_sha256"]:
        return False
    target_relocations = target_info["relocations"]
    base_relocations = base_info["relocations"]
    if len(target_relocations) != len(base_relocations):
        return False
    return all(
        target["address"] == base["address"] and target["type"] == base["type"]
        for target, base in zip(target_relocations, base_relocations)
    )


def audit(project_root, report_path, config_path):
    report = json.loads(report_path.read_text(encoding="utf-8"))
    config = json.loads(config_path.read_text(encoding="utf-8"))
    config_units = {unit["name"]: unit for unit in config.get("units", [])}

    exact = []
    hidden = []
    ordinary_only = []
    ordinary_rejected = []
    missing_base_symbol = 0
    unit_errors = []
    functions_evaluated = 0
    units_scanned = 0
    local_symbols_skipped = 0
    accepted_ledger = {}

    for unit in report.get("units", []):
        for function in unit.get("functions", []):
            if function.get("fuzzy_match_percent") == 100.0:
                key = f"{unit['name']}:{function['name']}"
                accepted_ledger[key] = {
                    "unit": unit["name"],
                    "function": function["name"],
                    "code_bytes": int(function.get("size", 0)),
                    "proof_sources": ["objdiff"],
                }

    for report_unit in report.get("units", []):
        unit_name = report_unit["name"]
        config_unit = config_units.get(unit_name)
        if not config_unit or "base_path" not in config_unit:
            continue
        functions = report_unit.get("functions", [])
        if not functions:
            continue
        try:
            target = load(project_root / config_unit["target_path"])
            base = load(project_root / config_unit["base_path"])
        except (CoffError, OSError) as error:
            unit_errors.append({"unit": unit_name, "error": str(error)})
            continue
        units_scanned += 1

        for function in functions:
            function_name = function["name"]
            if function_name.startswith("$"):
                local_symbols_skipped += 1
                continue
            ordinary_exact = function.get("fuzzy_match_percent") == 100.0
            try:
                target_info = section_info(target, function_name)
                base_info = section_info(base, function_name)
            except CoffError:
                missing_base_symbol += 1
                continue
            functions_evaluated += 1
            semantic_exact = target_info == base_info
            item = {
                "unit": unit_name,
                "function": function_name,
                "code_bytes": int(function.get("size", 0)),
                "padded_bytes": target_info["size"],
                "relocations": target_info["relocation_count"],
                "normalized_sha256": target_info["normalized_sha256"],
                "objdiff_percent": function.get("fuzzy_match_percent", 0.0),
            }
            key = f"{unit_name}:{function_name}"
            if semantic_exact:
                exact.append(item)
                if key in accepted_ledger:
                    accepted_ledger[key]["proof_sources"].append("semantic-coff")
                    accepted_ledger[key].update({
                        "padded_bytes": target_info["size"],
                        "relocations": target_info["relocation_count"],
                        "normalized_sha256": target_info["normalized_sha256"],
                    })
                else:
                    accepted_ledger[key] = dict(item, proof_sources=["semantic-coff"])
                if not ordinary_exact:
                    hidden.append(item)
            elif ordinary_exact:
                item["target"] = target_info
                item["base"] = base_info
                ordinary_only.append(item)
                if relocation_shape_matches(target_info, base_info):
                    accepted_ledger[key]["proof_sources"].append(
                        "objdiff-coff-shape"
                    )
                else:
                    ordinary_rejected.append(item)
                    accepted_ledger.pop(key, None)

    return {
        "summary": {
            "units_scanned": units_scanned,
            "functions_evaluated": functions_evaluated,
            "missing_base_symbol": missing_base_symbol,
            "semantic_exact": len(exact),
            "hidden_exact": len(hidden),
            "hidden_code_bytes": sum(item["code_bytes"] for item in hidden),
            "ordinary_only": len(ordinary_only),
            "ordinary_structural": len(ordinary_only) - len(ordinary_rejected),
            "ordinary_rejected": len(ordinary_rejected),
            "unit_errors": len(unit_errors),
            "local_symbols_skipped": local_symbols_skipped,
            "accepted_exact": len(accepted_ledger),
        },
        "hidden_exact": hidden,
        "ordinary_only": ordinary_only,
        "ordinary_rejected": ordinary_rejected,
        "unit_errors": unit_errors,
        "semantic_exact": exact,
        "accepted_ledger": sorted(
            accepted_ledger.values(), key=lambda item: (item["unit"], item["function"])
        ),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path.cwd())
    parser.add_argument("--report", type=Path, default=Path("build/report.json"))
    parser.add_argument("--config", type=Path, default=Path("objdiff.json"))
    parser.add_argument("--output", type=Path, default=Path("build/semantic_report.json"))
    args = parser.parse_args()

    result = audit(
        args.project,
        args.project / args.report,
        args.project / args.config,
    )
    output_path = args.project / args.output
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result["summary"], indent=2))


if __name__ == "__main__":
    main()
