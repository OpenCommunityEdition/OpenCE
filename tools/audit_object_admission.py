"""Find object-completion labels that need strict admission review.

This tool never grants matching credit.  It applies the same semantic
function/data transforms used by ``configure.py progress`` and then reports:

* unmarked units whose reported function and data gaps are both zero; and
* units whose raw completion label is contradicted by the strict report.

The first class is only a review queue.  COFF data ownership still has to be
proved before changing config.json.
"""

import argparse
import copy
import json
from pathlib import Path

from tools.semantic_progress import (
    apply_semantic_data_matches,
    apply_semantic_matches,
    apply_semantic_rejections,
    revoke_incomplete_units,
)


def _integer(value):
    if isinstance(value, str) and value.isdigit():
        return int(value)
    return value


def _normalize_measures(report):
    for container in [report, *report.get("categories", []), *report.get("units", [])]:
        measures = container.get("measures", {})
        for key, value in list(measures.items()):
            measures[key] = _integer(value)


def classify_units(raw_report, strict_report):
    """Return fail-closed completion candidates and contradicted labels."""
    raw_units = {unit["name"]: unit for unit in raw_report.get("units", [])}
    candidates = []
    contradicted = []

    for unit in strict_report.get("units", []):
        measures = unit.get("measures", {})
        total_functions = int(measures.get("total_functions", 0))
        matched_functions = int(measures.get("matched_functions", 0))
        total_data = int(measures.get("total_data", 0))
        matched_data = int(measures.get("matched_data", 0))
        function_gap = total_functions - matched_functions
        data_gap = total_data - matched_data
        fully_reported = function_gap == 0 and data_gap == 0
        has_reported_content = total_functions > 0 or total_data > 0
        raw_complete = bool(
            raw_units.get(unit["name"], {}).get("metadata", {}).get("complete", False)
        )

        item = {
            "unit": unit["name"],
            "total_functions": total_functions,
            "function_gap": function_gap,
            "total_data": total_data,
            "data_gap": data_gap,
        }
        if fully_reported and has_reported_content and not raw_complete:
            item["decision"] = "audit-coff-ownership-before-admission"
            candidates.append(item)
        elif raw_complete and not fully_reported:
            item["decision"] = "completion-label-contradicted"
            contradicted.append(item)

    candidates.sort(key=lambda item: (item["data_gap"], item["function_gap"], item["unit"]))
    contradicted.sort(key=lambda item: item["unit"])
    return {"candidates": candidates, "contradicted": contradicted}


def audit(
    project_root,
    report_path,
    config_path,
    semantic_report_path,
    semantic_matches_path,
    semantic_data_matches_path,
    symbol_manifest_path,
):
    raw_report = json.loads(report_path.read_text(encoding="utf-8"))
    strict_report = copy.deepcopy(raw_report)
    _normalize_measures(strict_report)

    apply_semantic_rejections(strict_report, semantic_report_path)
    apply_semantic_matches(
        strict_report,
        project_root,
        semantic_matches_path,
        config_path,
    )
    apply_semantic_data_matches(
        strict_report,
        project_root,
        semantic_data_matches_path,
        config_path,
        symbol_manifest_path,
    )
    revoked = revoke_incomplete_units(strict_report)
    result = classify_units(raw_report, strict_report)
    result["summary"] = {
        "candidate_count": len(result["candidates"]),
        "contradicted_count": len(result["contradicted"]),
        "revoked_count": len(revoked),
    }
    result["revoked"] = revoked
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Find finished-but-unmarked objects without granting credit."
    )
    parser.add_argument("--project", type=Path, default=Path("."))
    parser.add_argument("--report", type=Path, default=Path("build/report.json"))
    parser.add_argument("--config", type=Path, default=Path("objdiff.json"))
    parser.add_argument(
        "--semantic-report", type=Path, default=Path("build/semantic_report.json")
    )
    parser.add_argument(
        "--semantic-matches", type=Path, default=Path("config/semantic_matches.json")
    )
    parser.add_argument(
        "--semantic-data-matches",
        type=Path,
        default=Path("config/semantic_data_matches.json"),
    )
    parser.add_argument("--symbols", type=Path, default=Path("config/symbols.json"))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)

    project_root = args.project.resolve()

    def resolve(path):
        return path if path.is_absolute() else project_root / path

    result = audit(
        project_root,
        resolve(args.report),
        resolve(args.config),
        resolve(args.semantic_report),
        resolve(args.semantic_matches),
        resolve(args.semantic_data_matches),
        resolve(args.symbols),
    )
    rendered = json.dumps(result, indent=2) + "\n"
    if args.output:
        output = resolve(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
