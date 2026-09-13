"""Build a strict exact-function ledger and quantify objdiff false negatives."""

import argparse
import hashlib
import json
from pathlib import Path

from tools.coff_compare import CoffError, load, section_info, section_infos_equal


class SemanticAuditError(ValueError):
    """Raised when pinned semantic-credit policy is missing or stale."""


def relocation_sha256(info):
    """Return a stable digest of complete ordered relocation evidence."""
    payload = json.dumps(
        info["relocations"],
        ensure_ascii=True,
        separators=(",", ":"),
        sort_keys=True,
    ).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def section_fingerprint(info):
    """Reduce a COFF section description to manifest-pinnable evidence."""
    return {
        "normalized_sha256": info["normalized_sha256"],
        "padded_bytes": int(info["size"]),
        "relocations": int(info["relocation_count"]),
        "relocation_sha256": relocation_sha256(info),
    }


def _require_string(entry, field, label):
    value = entry.get(field)
    if not isinstance(value, str) or not value.strip():
        raise SemanticAuditError(f"{label} requires a non-empty {field}")
    return value


def _validate_fingerprint(value, label):
    fields = {
        "normalized_sha256",
        "padded_bytes",
        "relocations",
        "relocation_sha256",
    }
    if not isinstance(value, dict) or set(value) != fields:
        raise SemanticAuditError(
            f"{label} must contain exactly {', '.join(sorted(fields))}"
        )
    for field in ("normalized_sha256", "relocation_sha256"):
        digest = value[field]
        if not isinstance(digest, str) or len(digest) != 64 \
                or any(character not in "0123456789abcdef" for character in digest):
            raise SemanticAuditError(f"{label}.{field} is not a lowercase SHA-256")
    for field in ("padded_bytes", "relocations"):
        if not isinstance(value[field], int) or isinstance(value[field], bool) \
                or value[field] < 0:
            raise SemanticAuditError(f"{label}.{field} must be a non-negative integer")
    return value


def load_manual_rejections(project_root, manifest_path):
    """Load source-quality vetoes whose target and candidate evidence is pinned."""
    if manifest_path is None:
        return {}
    if not manifest_path.is_file():
        raise SemanticAuditError(
            f"semantic-credit rejection manifest does not exist: {manifest_path}"
        )

    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise SemanticAuditError(
            f"cannot read semantic-credit rejection manifest {manifest_path}: {error}"
        ) from error
    if not isinstance(manifest, dict) or set(manifest) != {"version", "entries"}:
        raise SemanticAuditError(
            "semantic-credit rejection manifest requires only version and entries"
        )
    if manifest["version"] != 1 or not isinstance(manifest["entries"], list):
        raise SemanticAuditError(
            "semantic-credit rejection manifest requires version 1 and an entries list"
        )

    project_root = project_root.resolve()
    rejections = {}
    required_fields = {
        "unit",
        "function",
        "target",
        "candidate",
        "code_bytes",
        "reason",
        "evidence",
    }
    for index, entry in enumerate(manifest["entries"]):
        label = f"semantic-credit rejection entry {index}"
        if not isinstance(entry, dict) or set(entry) != required_fields:
            raise SemanticAuditError(
                f"{label} must contain exactly {', '.join(sorted(required_fields))}"
            )
        unit_name = _require_string(entry, "unit", label)
        function_name = _require_string(entry, "function", label)
        reason = _require_string(entry, "reason", label)
        evidence = _require_string(entry, "evidence", label)
        target = _validate_fingerprint(entry["target"], f"{label}.target")
        candidate = _validate_fingerprint(entry["candidate"], f"{label}.candidate")
        code_bytes = entry["code_bytes"]
        if not isinstance(code_bytes, int) or isinstance(code_bytes, bool) \
                or code_bytes < 0:
            raise SemanticAuditError(
                f"{label}.code_bytes must be a non-negative integer"
            )

        evidence_file = evidence.split("#", 1)[0]
        evidence_path = (project_root / evidence_file).resolve()
        try:
            evidence_path.relative_to(project_root)
        except ValueError as error:
            raise SemanticAuditError(
                f"{label}.evidence escapes the project root: {evidence}"
            ) from error
        if not evidence_file or not evidence_path.is_file():
            raise SemanticAuditError(
                f"{label}.evidence does not name an existing file: {evidence}"
            )

        key = f"{unit_name}:{function_name}"
        if key in rejections:
            raise SemanticAuditError(f"duplicate semantic-credit rejection: {key}")
        rejections[key] = {
            "unit": unit_name,
            "function": function_name,
            "target": target,
            "candidate": candidate,
            "code_bytes": code_bytes,
            "reason": reason,
            "evidence": evidence,
        }
    return rejections


def require_pinned_rejection(key, rejection, target_info, base_info):
    """Fail if either live COFF section has drifted from a manual veto."""
    actual = {
        "target": section_fingerprint(target_info),
        "candidate": section_fingerprint(base_info),
    }
    for side in ("target", "candidate"):
        if actual[side] != rejection[side]:
            raise SemanticAuditError(
                f"stale semantic-credit rejection {key}: {side} fingerprint "
                f"expected {rejection[side]!r}, found {actual[side]!r}"
            )


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


def audit(project_root, report_path, config_path, rejection_path=None):
    report = json.loads(report_path.read_text(encoding="utf-8"))
    config = json.loads(config_path.read_text(encoding="utf-8"))
    config_units = {unit["name"]: unit for unit in config.get("units", [])}
    manual_rejections = load_manual_rejections(project_root, rejection_path)
    applied_manual_rejections = set()

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
            semantic_exact = section_infos_equal(target_info, base_info)
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
            manual_rejection = manual_rejections.get(key)
            if manual_rejection is not None:
                if not ordinary_exact:
                    raise SemanticAuditError(
                        f"stale semantic-credit rejection {key}: "
                        "function is no longer objdiff exact"
                    )
                if item["code_bytes"] != manual_rejection["code_bytes"]:
                    raise SemanticAuditError(
                        f"stale semantic-credit rejection {key}: code bytes "
                        f"expected {manual_rejection['code_bytes']}, "
                        f"found {item['code_bytes']}"
                    )
                require_pinned_rejection(
                    key,
                    manual_rejection,
                    target_info,
                    base_info,
                )
                item["target"] = target_info
                item["base"] = base_info
                item["rejection"] = {
                    "source": "manual-semantic-credit-veto",
                    "reason": manual_rejection["reason"],
                    "evidence": manual_rejection["evidence"],
                }
                if not semantic_exact:
                    ordinary_only.append(item)
                ordinary_rejected.append(item)
                accepted_ledger.pop(key, None)
                applied_manual_rejections.add(key)
                continue
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

    unapplied = sorted(set(manual_rejections) - applied_manual_rejections)
    if unapplied:
        raise SemanticAuditError(
            "stale semantic-credit rejection(s) not found in audited COFF input: "
            + ", ".join(unapplied)
        )

    rejected_keys = {
        f"{item['unit']}:{item['function']}" for item in ordinary_rejected
    }
    ordinary_structural = sum(
        f"{item['unit']}:{item['function']}" not in rejected_keys
        for item in ordinary_only
    )

    return {
        "summary": {
            "units_scanned": units_scanned,
            "functions_evaluated": functions_evaluated,
            "missing_base_symbol": missing_base_symbol,
            "semantic_exact": len(exact),
            "hidden_exact": len(hidden),
            "hidden_code_bytes": sum(item["code_bytes"] for item in hidden),
            "ordinary_only": len(ordinary_only),
            "ordinary_structural": ordinary_structural,
            "ordinary_rejected": len(ordinary_rejected),
            "manual_rejected": len(applied_manual_rejections),
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
    parser.add_argument(
        "--rejections",
        type=Path,
        default=Path("config/semantic_credit_rejections.json"),
    )
    parser.add_argument("--output", type=Path, default=Path("build/semantic_report.json"))
    args = parser.parse_args()

    try:
        result = audit(
            args.project,
            args.project / args.report,
            args.project / args.config,
            args.project / args.rejections,
        )
    except SemanticAuditError as error:
        raise SystemExit(f"Semantic audit failed: {error}") from error
    output_path = args.project / args.output
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result["summary"], indent=2))


if __name__ == "__main__":
    main()
