"""Validate and list functions parked on documented compiler-codegen ties.

The manifest is deliberately evidence-only: it does not decide which function
should be worked next.  Queue tooling can consume the validated ``active``
keys, while stale measurements fail validation and therefore reopen the
function for investigation.
"""

import argparse
import json
from pathlib import Path

from .coff_compare import CoffError, load, section_info


ALLOWED_CLASSES = {
    "instruction-scheduling",
    "private-register-convention",
    "register-allocation",
    "tu-context-optimization",
}


class ParkedFunctionsError(RuntimeError):
    pass


def _snapshot(info):
    return {
        "size": info["size"],
        "relocation_count": info["relocation_count"],
        "normalized_sha256": info["normalized_sha256"],
    }


def _differences(expected, current, prefix):
    differences = []
    for key in ("size", "relocation_count", "normalized_sha256"):
        if expected.get(key) != current.get(key):
            differences.append({
                "measurement": f"{prefix}.{key}",
                "expected": expected.get(key),
                "current": current.get(key),
            })
    return differences


def validate_parked_functions(project_root, report_path, config_path, manifest_path):
    project_root = Path(project_root)
    report = json.loads(Path(report_path).read_text(encoding="utf-8"))
    config = json.loads(Path(config_path).read_text(encoding="utf-8"))
    manifest = json.loads(Path(manifest_path).read_text(encoding="utf-8"))

    if manifest.get("version") != 1 or not isinstance(manifest.get("entries"), list):
        raise ParkedFunctionsError("parked manifest must have version 1 and an entries list")

    report_units = {unit["name"]: unit for unit in report.get("units", [])}
    config_units = {unit["name"]: unit for unit in config.get("units", [])}
    seen = set()
    active = []
    stale = []
    invalid = []

    for entry in manifest["entries"]:
        unit_name = entry.get("unit")
        function_name = entry.get("function")
        key = f"{unit_name}:{function_name}"
        reason = None

        if not isinstance(unit_name, str) or not isinstance(function_name, str):
            reason = "unit and function must be strings"
        elif key in seen:
            reason = "duplicate parked-function key"
        elif entry.get("class") not in ALLOWED_CLASSES:
            reason = "unknown blocker class"
        elif not isinstance(entry.get("evidence"), str) or not entry["evidence"].strip():
            reason = "non-empty evidence is required"
        elif unit_name not in report_units or unit_name not in config_units:
            reason = "unit missing from report or objdiff config"
        seen.add(key)

        if reason:
            invalid.append({"key": key, "reason": reason})
            continue

        functions = [
            function for function in report_units[unit_name].get("functions", [])
            if function.get("name") == function_name
        ]
        if len(functions) != 1:
            invalid.append({
                "key": key,
                "reason": f"expected one report function, found {len(functions)}",
            })
            continue

        config_unit = config_units[unit_name]
        try:
            target_info = section_info(
                load(project_root / config_unit["target_path"]), function_name
            )
            base_info = section_info(
                load(project_root / config_unit["base_path"]), function_name
            )
        except (CoffError, KeyError, OSError) as error:
            invalid.append({"key": key, "reason": f"cannot measure function: {error}"})
            continue

        if target_info == base_info:
            invalid.append({"key": key, "reason": "function is now semantically exact"})
            continue

        current = {
            "target": _snapshot(target_info),
            "base": _snapshot(base_info),
            "objdiff_percent": functions[0].get("fuzzy_match_percent", 0.0),
        }
        expected = entry.get("measurements", {})
        differences = []
        differences.extend(_differences(expected.get("target", {}), current["target"], "target"))
        differences.extend(_differences(expected.get("base", {}), current["base"], "base"))
        if expected.get("objdiff_percent") != current["objdiff_percent"]:
            differences.append({
                "measurement": "objdiff_percent",
                "expected": expected.get("objdiff_percent"),
                "current": current["objdiff_percent"],
            })

        item = {
            "key": key,
            "unit": unit_name,
            "function": function_name,
            "class": entry["class"],
            "evidence": entry["evidence"],
            "measurements": current,
        }
        if differences:
            item["differences"] = differences
            stale.append(item)
        else:
            active.append(item)

    return {
        "summary": {
            "active": len(active),
            "stale": len(stale),
            "invalid": len(invalid),
        },
        "active": active,
        "active_keys": [item["key"] for item in active],
        "stale": stale,
        "invalid": invalid,
    }


def require_valid_parked_functions(project_root, report_path, config_path, manifest_path):
    result = validate_parked_functions(project_root, report_path, config_path, manifest_path)
    if result["stale"] or result["invalid"]:
        details = [item["key"] for item in result["stale"] + result["invalid"]]
        raise ParkedFunctionsError(
            "parked-function evidence is stale or invalid: " + ", ".join(details)
        )
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path.cwd())
    parser.add_argument("--report", type=Path, default=Path("build/report.json"))
    parser.add_argument("--config", type=Path, default=Path("objdiff.json"))
    parser.add_argument("--manifest", type=Path, default=Path("config/parked.json"))
    parser.add_argument("--output", type=Path)
    parser.add_argument(
        "--allow-stale",
        action="store_true",
        help="report stale/invalid entries without returning a failure status",
    )
    args = parser.parse_args()

    root = args.project.resolve()
    result = validate_parked_functions(
        root,
        root / args.report,
        root / args.config,
        root / args.manifest,
    )
    serialized = json.dumps(result, indent=2) + "\n"
    if args.output:
        output = root / args.output
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(serialized, encoding="utf-8")
    print(serialized, end="")
    if not args.allow_stale and (result["stale"] or result["invalid"]):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
