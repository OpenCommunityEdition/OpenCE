"""Read-only COFF object inspector for campaign target and candidate files."""

from __future__ import annotations

import argparse
import json
import string
import sys
from pathlib import Path
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
    relocation_text,
    resolve_path,
)


def function_records(obj: dict[str, Any]) -> list[dict[str, Any]]:
    """Return stable, size-sorted records for unambiguous code sections."""

    records = []
    for function in function_sections(obj).values():
        records.append({
            "name": function.name,
            "section_number": function.section_number,
            "size": function.info["size"],
            "relocation_count": function.info["relocation_count"],
            "normalized_sha256": function.info["normalized_sha256"],
            "relocations": function.info["relocations"],
        })
    return sorted(records, key=lambda record: (record["size"], record["name"]))


def function_record(
    obj: dict[str, Any],
    function_name: str,
) -> dict[str, Any] | None:
    """Return one function record, or ``None`` if no section owns the name."""

    try:
        owner = cc.symbol(obj, function_name)
    except cc.CoffError:
        return None
    section = obj["sections"][owner["section"] - 1]
    if not section["flags"] & cc.IMAGE_SCN_CNT_CODE:
        return None
    info = cc.section_info_by_number(obj, owner["section"])
    return {
        "name": function_name,
        "section_number": owner["section"],
        "size": info["size"],
        "relocation_count": info["relocation_count"],
        "normalized_sha256": info["normalized_sha256"],
        "relocations": info["relocations"],
    }


def _preview_bytes(
    obj: dict[str, Any],
    section: dict[str, Any],
    limit: int,
) -> bytes | None:
    """Read at most *limit* file-backed bytes; never materialize BSS."""

    if section["flags"] & cc.IMAGE_SCN_CNT_UNINITIALIZED_DATA \
            and section["raw"] == 0:
        return None
    preview_size = min(section["size"], limit)
    start = section["raw"]
    return bytes(obj["data"][start:start + preview_size])


def data_records(
    obj: dict[str, Any],
    preview_limit: int = 512,
) -> list[dict[str, Any]]:
    """Describe non-code sections with bounded, file-backed previews."""

    records = []
    for section in obj["sections"]:
        if section["flags"] & cc.IMAGE_SCN_CNT_CODE:
            continue
        preview = _preview_bytes(obj, section, preview_limit)
        owners = sorted({
            symbol["name"]
            for symbol in obj["symbols"]
            if symbol["section"] == section["index"]
            and symbol["storage"] in (2, 3)
        })
        records.append({
            "section_number": section["index"],
            "name": section["name"],
            "size": section["size"],
            "flags": section["flags"],
            "owners": owners,
            "uninitialized": preview is None,
            "preview": None if preview is None else preview.hex(),
            "preview_size": 0 if preview is None else len(preview),
            "truncated": preview is not None and len(preview) < section["size"],
        })
    return records


def _ascii(data: bytes) -> str:
    printable = set(string.printable.encode("ascii"))
    return "".join(chr(byte) if byte in printable and byte >= 32 else "." for byte in data)


def render_functions(records: Sequence[dict[str, Any]], detail: bool) -> str:
    output = []
    for record in records:
        output.append(
            "%-56s size=%-6d relocs=%-4d sha=%s"
            % (
                record["name"],
                record["size"],
                record["relocation_count"],
                record["normalized_sha256"][:16],
            )
        )
        if detail:
            for relocation in record["relocations"]:
                output.append(
                    "    @%04x %s"
                    % (relocation["address"], relocation_text(relocation))
                )
    return "\n".join(output)


def render_data(records: Sequence[dict[str, Any]]) -> str:
    output = []
    for record in records:
        output.append(
            "== [%d] %s size=%d flags=0x%x owners=%s"
            % (
                record["section_number"],
                record["name"],
                record["size"],
                record["flags"],
                record["owners"],
            )
        )
        if record["uninitialized"]:
            output.append("    <uninitialized; preview not materialized>")
            continue
        preview = bytes.fromhex(record["preview"])
        for offset in range(0, len(preview), 16):
            chunk = preview[offset:offset + 16]
            output.append(
                "    %04x  %-47s  %s"
                % (
                    offset,
                    " ".join("%02x" % byte for byte in chunk),
                    _ascii(chunk),
                )
            )
        if record["truncated"]:
            output.append(
                "    ... (%d bytes not shown)"
                % (record["size"] - record["preview_size"])
            )
    return "\n".join(output)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Inspect an i386 COFF object without modifying it. Relative paths "
            "are resolved from --root; data previews are bounded and BSS is "
            "never materialized."
        ),
    )
    parser.add_argument(
        "unit",
        nargs="?",
        help="repository-relative unit path used for the default object",
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--function", "--fn", help="show one function in detail")
    mode.add_argument(
        "--data",
        action="store_true",
        help="show non-code section metadata and bounded previews",
    )
    parser.add_argument(
        "--object",
        help="object to inspect instead of build/split/<unit>.obj",
    )
    parser.add_argument(
        "--base",
        action="store_true",
        help="inspect build/base/<unit>.obj rather than build/split/<unit>.obj",
    )
    parser.add_argument(
        "--root",
        default=str(REPO_ROOT),
        help="repository root used for default and relative paths",
    )
    parser.add_argument(
        "--preview-bytes",
        type=int,
        default=512,
        help="maximum bytes previewed per data section (default: 512)",
    )
    parser.add_argument("--json", action="store_true", help="emit JSON to stdout")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.object is None and args.unit is None:
        parser.error("unit is required unless --object is supplied")
    if args.object is not None and args.base:
        parser.error("--object and --base are mutually exclusive")
    if args.preview_bytes < 0 or args.preview_bytes > 1024 * 1024:
        parser.error("--preview-bytes must be between 0 and 1048576")

    try:
        root = Path(args.root).resolve()
        if args.object is not None:
            object_path = resolve_path(root, args.object, root)
        else:
            unit = normalize_unit(args.unit)
            object_path = default_object_path(
                root,
                "base" if args.base else "split",
                unit,
            ).resolve()
        obj = load_object(object_path)
        if args.data:
            records = data_records(obj, args.preview_bytes)
            mode = "data"
        elif args.function:
            record = function_record(obj, args.function)
            if record is None:
                print("function not found: %s" % args.function, file=sys.stderr)
                return 1
            records = [record]
            mode = "function"
        else:
            records = function_records(obj)
            mode = "functions"
    except (OSError, ValueError, cc.CoffError) as error:
        parser.error(str(error))

    if args.json:
        print(json.dumps({
            "object": str(object_path),
            "mode": mode,
            "records": records,
        }, indent=2, sort_keys=True))
    elif mode == "data":
        print(render_data(records))
    else:
        print(render_functions(records, detail=mode == "function"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
