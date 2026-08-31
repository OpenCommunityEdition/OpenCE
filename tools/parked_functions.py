"""Validate and list functions parked on documented credible residuals.

The manifest is deliberately evidence-only: it does not decide which function
should be worked next.  Queue tooling can consume the validated ``active``
keys, while stale measurements fail validation and therefore reopen the
function for investigation.
"""

import argparse
import json
from pathlib import Path

from .coff_compare import (
    CoffError,
    image_symbol_addresses,
    load,
    section_info,
    section_info_resolved,
    section_infos_equal,
)


ALLOWED_CLASSES = {
    # The fail-closed classifier found no defensible mechanism.  This class is
    # intentionally recipe-free: it permits an honest fuzzy park without
    # laundering an unknown source/codegen gap into a more specific claim.
    "unclassified",
    "instruction-scheduling",
    "private-register-convention",
    "register-allocation",
    "tu-context-optimization",
    # Not compiler output at all (e.g. vendored __asm bodies); the evidence
    # must identify the external origin.  Added for matrix4x3_multiply,
    # parked in 2fdddb42 with this class but without the allowlist entry.
    "vendored-assembly",
    # Our source implements the function in inline or __declspec(naked)
    # __asm, so it is byte-identical by construction rather than by
    # reconstruction.  See ASM_IMPLEMENTED_CLASS below: entries in this class
    # are expected to compare exact, which is precisely why they are parked.
    "asm-implemented",
    # The target split and rebuilt object spell one or more relocation targets
    # differently even though independently recovered image addresses prove
    # identical destinations.  See CSPLIT_RELOCATION_ALIAS_CLASS below: these
    # entries must differ ordinarily and compare exact only after every
    # relocation destination is resolved through unambiguous image evidence.
    "csplit-relocation-alias",
}

# The ordinary manifest records functions we cannot yet match.  Membership is
# therefore invalidated when a function starts matching.  ``asm-implemented``
# inverts that: the body is transcribed assembly, so it always matches, and the
# park records provenance rather than a codegen gap.  Exactness must not
# invalidate those entries, and non-exactness must, because a non-matching
# asm body means the transcription has drifted from the target.
ASM_IMPLEMENTED_CLASS = "asm-implemented"
CSPLIT_RELOCATION_ALIAS_CLASS = "csplit-relocation-alias"


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


def _has_unresolved_relocations(info):
    return any(
        relocation.get("target", [None])[0] != "address"
        for relocation in info.get("relocations", [])
    )


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
            target_object = load(project_root / config_unit["target_path"])
            base_object = load(project_root / config_unit["base_path"])
            target_info = section_info(target_object, function_name)
            base_info = section_info(base_object, function_name)
        except (CoffError, KeyError, OSError) as error:
            invalid.append({"key": key, "reason": f"cannot measure function: {error}"})
            continue

        is_exact = section_infos_equal(target_info, base_info)
        if entry.get("class") == CSPLIT_RELOCATION_ALIAS_CLASS:
            if is_exact:
                invalid.append({
                    "key": key,
                    "reason": (
                        "csplit-relocation-alias function does not differ under "
                        "ordinary comparison"
                    ),
                })
                continue

            try:
                symbol_entries = json.loads(
                    (project_root / "config" / "symbols.json").read_text(
                        encoding="utf-8"
                    )
                )
                if not isinstance(symbol_entries, list):
                    raise ValueError("symbol manifest must be a list")
                symbol_addresses = image_symbol_addresses(symbol_entries)
                target_resolved = section_info_resolved(
                    target_object, function_name, symbol_addresses
                )
                base_resolved = section_info_resolved(
                    base_object, function_name, symbol_addresses
                )
            except (CoffError, KeyError, OSError, TypeError, ValueError) as error:
                invalid.append({
                    "key": key,
                    "reason": f"cannot resolve csplit relocation alias: {error}",
                })
                continue

            if _has_unresolved_relocations(target_resolved) \
                    or _has_unresolved_relocations(base_resolved):
                invalid.append({
                    "key": key,
                    "reason": (
                        "csplit-relocation-alias requires an unambiguous image "
                        "address for every relocation destination"
                    ),
                })
                continue
            if not section_infos_equal(target_resolved, base_resolved):
                invalid.append({
                    "key": key,
                    "reason": (
                        "csplit-relocation-alias function is not exact after "
                        "resolving image destinations"
                    ),
                })
                continue
        elif entry.get("class") == ASM_IMPLEMENTED_CLASS:
            # Inverted expectation: an asm body matches by construction, so
            # only a *non*-matching one is news (the transcription drifted).
            if not is_exact:
                invalid.append({
                    "key": key,
                    "reason": "asm-implemented function no longer matches the target",
                })
                continue
        elif is_exact:
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
