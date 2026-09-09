"""List non-exact functions that do not already have a parked verdict.

The command is read-only and inspects existing January/candidate COFF objects;
it does not compile source or edit ``config/parked.json``.  Explicit unit paths
may name ``.c`` or ``.obj`` files.  Use ``--all`` to inspect every built source
object.

Examples::

    python -B -m tools.campaign.unparked source/ai/actors
    python -B -m tools.campaign.unparked --all --json

Because this reads ``build/base``, rebuild or use ``gate.py`` first when source
has changed since the last build.
"""

from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import json
from pathlib import Path
import sys
from typing import Any, Iterable, Sequence


REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import (  # noqa: E402
    default_object_path,
    function_sections,
    load_object,
    normalize_unit,
)


class OpenPoolError(Exception):
    """The requested open-pool census cannot be completed reliably."""


@dataclass(frozen=True)
class OpenFunction:
    unit: str
    function: str
    status: str
    padded_size: int
    difference: str


def load_parked(path: Path) -> set[tuple[str, str]]:
    """Load normalized ``(unit, function)`` keys from the parked ledger."""

    document = json.loads(path.read_text(encoding="utf-8"))
    entries = document.get("entries")
    if not isinstance(entries, list):
        raise OpenPoolError("parked ledger must contain an entries list")

    parked = set()
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            raise OpenPoolError("parked entry %d is not an object" % index)
        unit = entry.get("unit")
        function = entry.get("function")
        if not isinstance(unit, str) or not isinstance(function, str):
            raise OpenPoolError(
                "parked entry %d lacks string unit/function fields" % index
            )
        parked.add((normalize_unit(unit.strip()), function))
    return parked


def difference_text(
    target: dict[str, Any],
    candidate: dict[str, Any],
) -> str:
    """Describe a residual using the same fail-closed fields as ``gate.py``."""

    differences = []
    if candidate["size"] != target["size"]:
        differences.append("size %d!=%d" % (candidate["size"], target["size"]))
    if candidate["relocation_count"] != target["relocation_count"]:
        differences.append(
            "relocs %d!=%d"
            % (candidate["relocation_count"], target["relocation_count"])
        )
    if candidate["normalized_sha256"] != target["normalized_sha256"]:
        differences.append("sha")
    if not differences:
        differences.append("reloc-identity")
    return ", ".join(differences)


def classify_unit(
    unit: str,
    target: dict[str, Any],
    candidate: dict[str, Any] | None,
    parked: set[tuple[str, str]],
) -> list[OpenFunction]:
    """Return every unparked residual/unwritten function in one unit."""

    target_functions = function_sections(target)
    candidate_functions = function_sections(candidate) if candidate else {}
    rows = []

    for function, target_section in target_functions.items():
        if (unit, function) in parked:
            continue
        candidate_section = candidate_functions.get(function)
        if candidate_section is None:
            rows.append(OpenFunction(
                unit=unit,
                function=function,
                status="UNWRITTEN",
                padded_size=target_section.info["size"],
                difference="candidate function absent",
            ))
            continue
        if cc.section_infos_equal(target_section.info, candidate_section.info):
            continue
        rows.append(OpenFunction(
            unit=unit,
            function=function,
            status="residual",
            padded_size=target_section.info["size"],
            difference=difference_text(target_section.info, candidate_section.info),
        ))

    return sorted(rows, key=lambda row: (-row.padded_size, row.function))


def discover_units(root: Path) -> list[str]:
    """Return all built source units beneath ``build/split``."""

    split_root = root / "build" / "split"
    source_root = split_root / "source"
    if not source_root.is_dir():
        raise OpenPoolError("built target directory does not exist: %s" % source_root)
    units = [
        path.relative_to(split_root).as_posix()[:-4]
        for path in sorted(source_root.rglob("*.obj"))
    ]
    if not units:
        raise OpenPoolError("no built target objects found beneath %s" % source_root)
    return units


def inspect_units(
    root: Path,
    units: Iterable[str],
    parked: set[tuple[str, str]],
    keep_going: bool = False,
) -> tuple[list[OpenFunction], list[str]]:
    """Inspect units and return ``(rows, errors)`` without mutating the tree."""

    rows = []
    errors = []
    for unit in units:
        target_path = default_object_path(root, "split", unit)
        candidate_path = default_object_path(root, "base", unit)
        try:
            target = load_object(target_path)
            candidate = load_object(candidate_path) if candidate_path.is_file() else None
            rows.extend(classify_unit(unit, target, candidate, parked))
        except (OSError, ValueError, cc.CoffError) as error:
            errors.append("%s: %s" % (unit, error))
            if not keep_going:
                break
    return rows, errors


def render_text(rows: Sequence[OpenFunction]) -> str:
    output = []
    by_unit: dict[str, list[OpenFunction]] = {}
    for row in rows:
        by_unit.setdefault(row.unit, []).append(row)
    for unit in sorted(by_unit):
        output.append("=== %s ===" % unit)
        for row in by_unit[unit]:
            output.append(
                "  %-9s %6d  %-56s [%s]"
                % (row.status, row.padded_size, row.function, row.difference)
            )

    residuals = sum(row.status == "residual" for row in rows)
    unwritten = sum(row.status == "UNWRITTEN" for row in rows)
    output.append(
        "OPEN: %d padded bytes in %d function(s): %d residual, %d unwritten"
        % (
            sum(row.padded_size for row in rows),
            len(rows),
            residuals,
            unwritten,
        )
    )
    return "\n".join(output)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "List built residual/unwritten functions after subtracting the "
            "parked ledger; this command never edits files."
        ),
    )
    parser.add_argument("units", nargs="*", help="repository-relative unit paths")
    parser.add_argument("--all", action="store_true", help="inspect all built source units")
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    parser.add_argument(
        "--keep-going",
        action="store_true",
        help="continue after unreadable units; the command still exits nonzero",
    )
    parser.add_argument(
        "--root",
        default=str(REPO_ROOT),
        help="repository root used for object and ledger paths",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.all and args.units:
        parser.error("--all and explicit units are mutually exclusive")
    if not args.all and not args.units:
        parser.error("provide at least one unit or pass --all")

    root = Path(args.root).resolve()
    try:
        units = discover_units(root) if args.all else list(dict.fromkeys(
            normalize_unit(unit) for unit in args.units
        ))
        parked = load_parked(root / "config" / "parked.json")
        rows, errors = inspect_units(root, units, parked, args.keep_going)
    except (OSError, ValueError, json.JSONDecodeError, OpenPoolError) as error:
        print("unparked: error: %s" % error, file=sys.stderr)
        return 2

    if args.json:
        print(json.dumps({
            "rows": [asdict(row) for row in rows],
            "summary": {
                "padded_bytes": sum(row.padded_size for row in rows),
                "functions": len(rows),
                "residual": sum(row.status == "residual" for row in rows),
                "unwritten": sum(row.status == "UNWRITTEN" for row in rows),
            },
            "errors": errors,
        }, indent=2))
    else:
        print(render_text(rows))
        for error in errors:
            print("unparked: error: %s" % error, file=sys.stderr)

    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
