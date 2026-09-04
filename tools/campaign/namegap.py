"""Propose non-authoritative COFF symbol-name gaps without editing anything.

The strict comparator can prove that two function-owned sections are equal
even when their symbols differ.  That is useful evidence for a later naming
review, but it is not proof of the original name.  This utility therefore
prints proposals only, never edits ``config/symbols.json``, and never reports
an ambiguous pairing as a proposal.
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any, Sequence


REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import (  # noqa: E402
    FunctionSection,
    default_object_path,
    function_sections,
    load_object,
    normalize_unit,
    resolve_path,
)


NOTICE = (
    "NON-AUTHORITATIVE PROPOSALS ONLY: section equality does not prove an "
    "original symbol name. Review January/PDB/source provenance before any "
    "manual config edit."
)


def discover_units(root: Path) -> list[str]:
    """Read source-unit names from config without consulting generated files."""

    config_path = root / "config" / "config.json"
    config = json.loads(config_path.read_text(encoding="utf-8"))
    units = set()
    for project in config.get("projects", []):
        for entry in project.get("objects", []):
            raw_name = str(entry.get("name", "")).replace("\\", "/")
            if not raw_name.startswith("source/"):
                continue
            try:
                units.add(normalize_unit(raw_name))
            except ValueError:
                continue
    return sorted(units)


def _shape(function: FunctionSection) -> tuple[int, int, str]:
    info = function.info
    return (
        info["size"],
        info["relocation_count"],
        info["normalized_sha256"],
    )


def find_name_gaps(
    target_obj: dict[str, Any],
    ours_obj: dict[str, Any],
    unit: str,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Return strict unique proposals and separately listed ambiguities."""

    target_sections = function_sections(target_obj)
    ours_sections = function_sections(ours_obj)
    unmatched_target = {
        name: section
        for name, section in target_sections.items()
        if name not in ours_sections
    }
    unmatched_ours = {
        name: section
        for name, section in ours_sections.items()
        if name not in target_sections
    }

    ours_by_shape: dict[tuple[int, int, str], list[FunctionSection]] = defaultdict(list)
    for section in unmatched_ours.values():
        ours_by_shape[_shape(section)].append(section)

    target_matches: dict[str, list[str]] = {}
    ours_matches: dict[str, list[str]] = defaultdict(list)
    for target_name, target_section in unmatched_target.items():
        matches = []
        for ours_section in ours_by_shape.get(_shape(target_section), []):
            if cc.section_infos_equal(target_section.info, ours_section.info):
                matches.append(ours_section.name)
                ours_matches[ours_section.name].append(target_name)
        if matches:
            target_matches[target_name] = sorted(matches)

    proposals = []
    ambiguities = []
    for target_name, ours_names in sorted(target_matches.items()):
        unique = len(ours_names) == 1 \
            and len(ours_matches[ours_names[0]]) == 1
        target_section = unmatched_target[target_name]
        if not unique:
            ambiguities.append({
                "unit": unit,
                "target_name": target_name,
                "candidate_names": ours_names,
                "size": target_section.info["size"],
                "relocation_count": target_section.info["relocation_count"],
                "reason": "non-bijective strict section equality",
            })
            continue
        proposals.append({
            "unit": unit,
            "target_name": target_name,
            "candidate_name": ours_names[0],
            "size": target_section.info["size"],
            "relocation_count": target_section.info["relocation_count"],
            "confidence": "strict-unique-section-equality",
            "authoritative": False,
            "requires_manual_provenance_review": True,
        })
    return proposals, ambiguities


def scan_unit(
    root: Path,
    unit: str,
    target_object: str | None = None,
    ours_object: str | None = None,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]], dict[str, str]]:
    """Load and scan one unit, returning paths with its diagnostic records."""

    target_path = resolve_path(
        root,
        target_object,
        default_object_path(root, "split", unit),
    )
    ours_path = resolve_path(
        root,
        ours_object,
        default_object_path(root, "base", unit),
    )
    proposals, ambiguities = find_name_gaps(
        load_object(target_path),
        load_object(ours_path),
        unit,
    )
    return proposals, ambiguities, {
        "target": str(target_path),
        "ours": str(ours_path),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Print non-authoritative symbol rename proposals for strictly "
            "equal, uniquely paired COFF code sections. This command never "
            "edits source or config files."
        ),
    )
    parser.add_argument(
        "units",
        nargs="*",
        help="repository-relative unit paths (default: all configured source units)",
    )
    parser.add_argument(
        "--target-object",
        help="target object override; requires exactly one explicit unit",
    )
    parser.add_argument(
        "--ours-object",
        help="candidate object override; requires exactly one explicit unit",
    )
    parser.add_argument(
        "--root",
        default=str(REPO_ROOT),
        help="repository root used for default and relative paths",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="emit a machine-readable proposal report to stdout",
    )
    parser.add_argument(
        "--show-ambiguous",
        action="store_true",
        help="include rejected non-bijective pairings in text output",
    )
    return parser


def _render_text(
    proposals: Sequence[dict[str, Any]],
    ambiguities: Sequence[dict[str, Any]],
    skipped: Sequence[dict[str, str]],
    show_ambiguous: bool,
) -> str:
    output = [NOTICE]
    current_unit = None
    for proposal in proposals:
        if proposal["unit"] != current_unit:
            current_unit = proposal["unit"]
            output.append("== %s" % current_unit)
        output.append(
            "   %5d B %3d rel  %-44s -> %s"
            % (
                proposal["size"],
                proposal["relocation_count"],
                proposal["target_name"],
                proposal["candidate_name"],
            )
        )
    if show_ambiguous and ambiguities:
        output.append("AMBIGUOUS (REJECTED, NOT PROPOSALS)")
        for ambiguity in ambiguities:
            output.append(
                "   %s %s -> %s"
                % (
                    ambiguity["unit"],
                    ambiguity["target_name"],
                    ", ".join(ambiguity["candidate_names"]),
                )
            )
    if skipped:
        output.append("SKIPPED %d unit(s) with missing/unreadable objects" % len(skipped))
    output.append(
        "TOTAL: %d proposal(s), %d bytes; %d ambiguous pairing(s) rejected"
        % (
            len(proposals),
            sum(proposal["size"] for proposal in proposals),
            len(ambiguities),
        )
    )
    return "\n".join(output)


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if (args.target_object or args.ours_object) and len(args.units) != 1:
        parser.error("object overrides require exactly one explicit unit")

    try:
        root = Path(args.root).resolve()
        units = [normalize_unit(unit) for unit in args.units]
        if not units:
            units = discover_units(root)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        parser.error(str(error))

    proposals: list[dict[str, Any]] = []
    ambiguities: list[dict[str, Any]] = []
    skipped: list[dict[str, str]] = []
    paths: dict[str, dict[str, str]] = {}
    for unit in units:
        try:
            unit_proposals, unit_ambiguities, unit_paths = scan_unit(
                root,
                unit,
                target_object=args.target_object,
                ours_object=args.ours_object,
            )
        except (OSError, ValueError, cc.CoffError) as error:
            skipped.append({"unit": unit, "reason": str(error)})
            continue
        proposals.extend(unit_proposals)
        ambiguities.extend(unit_ambiguities)
        paths[unit] = unit_paths

    report = {
        "authoritative": False,
        "notice": NOTICE,
        "proposal_count": len(proposals),
        "proposal_bytes": sum(proposal["size"] for proposal in proposals),
        "proposals": proposals,
        "ambiguities_rejected": ambiguities,
        "skipped": skipped,
        "paths": paths,
    }
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(_render_text(
            proposals,
            ambiguities,
            skipped,
            args.show_ambiguous,
        ))

    if args.units and len(skipped) == len(units):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
