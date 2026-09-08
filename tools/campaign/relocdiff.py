"""Compare every relocation in one January/candidate function pair.

By default this tool is fail-closed for relocation-identity residuals: section
size, relocation count, normalized bytes, and relocation address/type layout
must match. Use --allow-structural only when deliberately inspecting a broader
residual; missing rows are then rendered rather than silently truncated.

Usage:

    python -B tools/campaign/relocdiff.py \
        source/hs/hs_compile _hs_parse_object_name
    python -B tools/campaign/relocdiff.py \
        source/hs/hs_compile _hs_parse_object_name scratch/probe.obj
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
from itertools import zip_longest
from pathlib import Path
import sys
from typing import Any, Sequence


REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import (  # noqa: E402
    default_object_path,
    function_sections,
    load_object,
    normalize_unit,
    resolve_path,
)


class RelocationDiffError(Exception):
    """The requested comparison cannot be made truthfully."""


@dataclass(frozen=True)
class RelocationRow:
    """One complete pairwise relocation row."""

    index: int
    target: dict[str, Any] | None
    ours: dict[str, Any] | None
    equal: bool


def validate_info(info: dict[str, Any], label: str) -> None:
    """Require internally consistent comparator section metadata."""

    required = ("size", "relocation_count", "normalized_sha256", "relocations")
    missing = [key for key in required if key not in info]
    if missing:
        raise RelocationDiffError(
            "%s section metadata lacks %s" % (label, ", ".join(missing))
        )
    actual = len(info["relocations"])
    if info["relocation_count"] != actual:
        raise RelocationDiffError(
            "%s reports %d relocations but contains %d records"
            % (label, info["relocation_count"], actual)
        )


def identity_precondition_problems(
    target: dict[str, Any],
    ours: dict[str, Any],
) -> list[str]:
    """Return every reason this is not a pure relocation-identity comparison."""

    problems = []
    if target["size"] != ours["size"]:
        problems.append("size %d != %d" % (target["size"], ours["size"]))
    if target["relocation_count"] != ours["relocation_count"]:
        problems.append(
            "relocation count %d != %d"
            % (target["relocation_count"], ours["relocation_count"])
        )
    if target["normalized_sha256"] != ours["normalized_sha256"]:
        problems.append("normalized bytes differ")

    for index, (target_relocation, ours_relocation) in enumerate(zip(
        target["relocations"],
        ours["relocations"],
    )):
        if target_relocation["address"] != ours_relocation["address"]:
            problems.append(
                "relocation %d address 0x%x != 0x%x"
                % (
                    index,
                    target_relocation["address"],
                    ours_relocation["address"],
                )
            )
        if target_relocation["type"] != ours_relocation["type"]:
            problems.append(
                "relocation %d type 0x%x != 0x%x"
                % (
                    index,
                    target_relocation["type"],
                    ours_relocation["type"],
                )
            )
    return problems


def relocation_rows(
    target: dict[str, Any],
    ours: dict[str, Any],
) -> list[RelocationRow]:
    """Return all rows, retaining unmatched tails on either side."""

    rows = []
    for index, (target_relocation, ours_relocation) in enumerate(zip_longest(
        target["relocations"],
        ours["relocations"],
    )):
        equal = (
            target_relocation is not None
            and ours_relocation is not None
            and cc.relocation_infos_equal(
                [target_relocation],
                [ours_relocation],
            )
        )
        rows.append(RelocationRow(
            index=index,
            target=target_relocation,
            ours=ours_relocation,
            equal=equal,
        ))
    return rows


def target_text(relocation: dict[str, Any] | None) -> str:
    """Render raw and optional symbolic relocation identities."""

    if relocation is None:
        return "<missing>"
    raw = repr(relocation.get("target"))
    symbolic = relocation.get("symbolic_target")
    if symbolic is not None and symbolic != relocation.get("target"):
        return "%s symbolic=%r" % (raw, symbolic)
    return raw


def address_text(relocation: dict[str, Any] | None) -> str:
    if relocation is None:
        return "----"
    return "%04x" % relocation["address"]


def type_text(relocation: dict[str, Any] | None) -> str:
    if relocation is None:
        return "----"
    return "%04x" % relocation["type"]


def relocation_counter(
    relocations: Sequence[dict[str, Any]],
) -> Counter[str]:
    """Count raw relocation identities without dropping addends."""

    return Counter(target_text(relocation) for relocation in relocations)


def render_rows(
    rows: Sequence[RelocationRow],
    only_differences: bool = False,
) -> str:
    output = [
        "%-3s %-5s %-5s %-5s %-5s %-42s %s"
        % ("", "TADDR", "OADDR", "TTYPE", "OTYPE", "TARGET (January)", "OURS"),
    ]
    for row in rows:
        if only_differences and row.equal:
            continue
        output.append(
            "%-3s %-5s %-5s %-5s %-5s %-42s %s"
            % (
                "=" if row.equal else "<",
                address_text(row.target),
                address_text(row.ours),
                type_text(row.target),
                type_text(row.ours),
                target_text(row.target),
                target_text(row.ours),
            )
        )
    return "\n".join(output)


def render_counts(
    target: Sequence[dict[str, Any]],
    ours: Sequence[dict[str, Any]],
) -> str:
    target_counts = relocation_counter(target)
    ours_counts = relocation_counter(ours)
    output = ["%-60s %6s %6s" % ("relocation target", "JAN", "OURS")]
    for identity in sorted(set(target_counts) | set(ours_counts)):
        marker = "" if target_counts[identity] == ours_counts[identity] else "  <"
        output.append(
            "%-60s %6d %6d%s"
            % (identity, target_counts[identity], ours_counts[identity], marker)
        )
    output.append(
        "%-60s %6d %6d"
        % ("TOTAL", sum(target_counts.values()), sum(ours_counts.values()))
    )
    return "\n".join(output)


def function_info(path: Path, function: str) -> dict[str, Any]:
    """Load one unambiguous function-owned section."""

    functions = function_sections(load_object(path))
    if function not in functions:
        raise RelocationDiffError(
            "%s: function %r is missing or ambiguously owned"
            % (path, function)
        )
    return functions[function].info


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Compare complete relocation records for one January/candidate "
            "function pair without modifying either object."
        ),
    )
    parser.add_argument("unit", help="repository-relative unit path")
    parser.add_argument("function", help="decorated COFF function name")
    parser.add_argument(
        "candidate_object",
        nargs="?",
        help="candidate object override (default: build/base/<unit>.obj)",
    )
    parser.add_argument("--target-object", help="January object override")
    parser.add_argument("--ours-object", help="candidate object override")
    parser.add_argument(
        "--allow-structural",
        action="store_true",
        help="render unequal shapes and unmatched relocation tails",
    )
    parser.add_argument(
        "--count-by-target",
        action="store_true",
        help="also summarize complete relocation counts by raw identity",
    )
    parser.add_argument(
        "--only-differences",
        action="store_true",
        help="omit equal relocation rows",
    )
    parser.add_argument(
        "--root",
        default=str(REPO_ROOT),
        help="repository root used for default and relative paths",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.candidate_object and args.ours_object:
        parser.error("candidate_object and --ours-object are mutually exclusive")

    try:
        root = Path(args.root).resolve()
        unit = normalize_unit(args.unit)
        target_path = resolve_path(
            root,
            args.target_object,
            default_object_path(root, "split", unit),
        )
        ours_override = args.ours_object or args.candidate_object
        ours_path = resolve_path(
            root,
            ours_override,
            default_object_path(root, "base", unit),
        )
        target = function_info(target_path, args.function)
        ours = function_info(ours_path, args.function)
        validate_info(target, "January")
        validate_info(ours, "candidate")
        problems = identity_precondition_problems(target, ours)
        if problems and not args.allow_structural:
            raise RelocationDiffError(
                "not a pure relocation-identity residual: %s; "
                "pass --allow-structural to inspect it deliberately"
                % "; ".join(problems)
            )
        rows = relocation_rows(target, ours)
    except (OSError, ValueError, cc.CoffError, RelocationDiffError) as error:
        print("relocdiff: error: %s" % error, file=sys.stderr)
        return 2

    print(
        "target=%s\nours=%s\nshape: size %d/%d, relocations %d/%d, sha %s"
        % (
            target_path,
            ours_path,
            target["size"],
            ours["size"],
            target["relocation_count"],
            ours["relocation_count"],
            (
                "equal"
                if target["normalized_sha256"] == ours["normalized_sha256"]
                else "different"
            ),
        )
    )
    if problems:
        print("structural differences: %s" % "; ".join(problems))
    print(render_rows(rows, only_differences=args.only_differences))
    if args.count_by_target:
        print("\n" + render_counts(target["relocations"], ours["relocations"]))
    print(
        "summary: %d target, %d candidate, %d differing row(s)"
        % (
            len(target["relocations"]),
            len(ours["relocations"]),
            sum(not row.equal for row in rows),
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
