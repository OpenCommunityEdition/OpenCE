"""Propose undecorated January functions whose final instruction is ret N.

This is read-only naming evidence, never proof of an original symbol. By
default only address placeholders such as _code_001b2040 are reported; pass
--all-undecorated to inspect semantic names as well. Confirm every proposal
against source, symbols, and callers before editing config/symbols.json.

Usage:

    python -B tools/campaign/stdcall_scan.py source/hs/hs_compile
    python -B tools/campaign/stdcall_scan.py --all
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
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
)


PLACEHOLDER_NAME = re.compile(r"^_code_[0-9A-Fa-f]{8}$")
MAX_STDCALL_ARGUMENT_BYTES = 4096


@dataclass(frozen=True)
class StdcallProposal:
    """One read-only calling-convention naming proposal."""

    name: str
    argument_bytes: int
    padded_size: int


def stdcall_argument_bytes(body: bytes) -> int | None:
    """Decode a plausible terminal x86 ret-immediate after code padding."""

    trimmed = body.rstrip(b"\x90\xcc")
    if len(trimmed) < 3 or trimmed[-3] != 0xC2:
        return None
    argument_bytes = int.from_bytes(trimmed[-2:], "little")
    if argument_bytes > MAX_STDCALL_ARGUMENT_BYTES:
        return None
    if argument_bytes % 4:
        return None
    return argument_bytes


def scan_object(
    obj: dict[str, Any],
    all_undecorated: bool = False,
) -> list[StdcallProposal]:
    """Return conservative proposals from one already loaded COFF object."""

    proposals = []
    blob = obj["data"]
    for function in function_sections(obj).values():
        if "@" in function.name:
            continue
        if not all_undecorated and not PLACEHOLDER_NAME.fullmatch(function.name):
            continue
        section = obj["sections"][function.section_number - 1]
        offset = section["raw"]
        size = section["size"]
        if not offset or not size:
            continue
        body = bytes(blob[offset:offset + size])
        argument_bytes = stdcall_argument_bytes(body)
        if argument_bytes is None:
            continue
        proposals.append(StdcallProposal(
            name=function.name,
            argument_bytes=argument_bytes,
            padded_size=function.info["size"],
        ))
    return sorted(
        proposals,
        key=lambda proposal: (proposal.name, proposal.argument_bytes),
    )


def discover_units(root: Path) -> list[str]:
    """Discover built January source objects without inventing missing units."""

    source_root = root / "build" / "split" / "source"
    if not source_root.is_dir():
        raise OSError("built target object directory does not exist: %s" % source_root)
    units = []
    for path in sorted(source_root.rglob("*.obj")):
        relative = path.relative_to(root / "build" / "split")
        units.append(relative.as_posix()[:-4])
    if not units:
        raise OSError("no built target objects found beneath %s" % source_root)
    return units


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Print non-authoritative __stdcall naming proposals from terminal "
            "ret-immediate instructions. This command never edits config."
        ),
    )
    parser.add_argument(
        "units",
        nargs="*",
        help="repository-relative unit paths",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="scan every built January source object",
    )
    parser.add_argument(
        "--all-undecorated",
        action="store_true",
        help="include semantic undecorated names, not only _code_XXXXXXXX",
    )
    parser.add_argument(
        "--keep-going",
        action="store_true",
        help="scan remaining units after an error; still exits nonzero",
    )
    parser.add_argument(
        "--root",
        default=str(REPO_ROOT),
        help="repository root used for object paths",
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
        if args.all:
            units = discover_units(root)
        else:
            units = list(dict.fromkeys(
                normalize_unit(unit)
                for unit in args.units
            ))
    except (OSError, ValueError) as error:
        print("stdcall_scan: error: %s" % error, file=sys.stderr)
        return 2

    total = 0
    errors = []
    for unit in units:
        object_path = default_object_path(root, "split", unit).resolve()
        try:
            proposals = scan_object(
                load_object(object_path),
                all_undecorated=args.all_undecorated,
            )
        except (OSError, ValueError, cc.CoffError) as error:
            message = "%s: %s" % (unit, error)
            errors.append(message)
            print("stdcall_scan: error: %s" % message, file=sys.stderr)
            if not args.keep_going:
                return 2
            continue

        if not proposals:
            continue
        print("=== %s" % unit)
        for proposal in proposals:
            print(
                "   %-54s ret %-4d size %-6d propose @%d"
                % (
                    proposal.name,
                    proposal.argument_bytes,
                    proposal.padded_size,
                    proposal.argument_bytes,
                )
            )
            total += 1

    print(
        "\n%d non-authoritative __stdcall naming proposal(s); "
        "manual provenance review required" % total
    )
    if errors:
        print(
            "%d unit(s) failed; result is partial and must not be treated as "
            "a complete scan" % len(errors),
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
