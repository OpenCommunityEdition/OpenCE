"""Show an alignment-aware, relocation-normalized disassembly diff.

Unlike ``gate.py --disas``, this tool aligns complete instruction streams with
``difflib.SequenceMatcher`` before displaying differences.  Inserted or
deleted instructions therefore do not shift every later comparison.
"""

from __future__ import annotations

import argparse
import difflib
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Sequence


REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import (  # noqa: E402
    default_object_path,
    load_object,
    normalize_unit,
    relocation_text,
    resolve_path,
)


_CONTROL_FLOW = re.compile(r"^(?:j[a-z]+|call|loop[a-z]*)$")
_LEADING_ADDRESS = re.compile(r"^(?:0x[0-9a-fA-F]+|-?[0-9]+)(?=$|,)")


@dataclass(frozen=True)
class Instruction:
    """Rendered and normalized views of one instruction."""

    offset: int
    rendered: str
    normalized: str


def normalize_instruction(
    mnemonic: str,
    operands: str,
    relocation_annotations: Iterable[str] = (),
) -> str:
    """Normalize unstable branch destinations while preserving semantics."""

    normalized_operands = operands
    if _CONTROL_FLOW.match(mnemonic):
        normalized_operands = _LEADING_ADDRESS.sub("<branch-target>", operands)
    text = mnemonic
    if normalized_operands:
        text += " " + normalized_operands
    annotations = tuple(relocation_annotations)
    if annotations:
        text += " ; " + " ; ".join(annotations)
    return text


def disassemble_function(
    obj: dict[str, Any],
    function_name: str,
) -> list[Instruction]:
    """Disassemble exactly the section owned by *function_name*.

    Relocated dwords are zeroed before disassembly and their fail-closed COFF
    identities are retained as annotations.  This prevents link-time operand
    values from creating false instruction differences.
    """

    try:
        import capstone
    except ImportError as error:  # pragma: no cover - dependency is in CI
        raise RuntimeError("Capstone is required for alndiff.py") from error

    owner = cc.symbol(obj, function_name)
    section = obj["sections"][owner["section"] - 1]
    if not section["flags"] & cc.IMAGE_SCN_CNT_CODE:
        raise cc.CoffError("%r does not own a code section" % function_name)

    info = cc.section_info_by_number(obj, owner["section"])
    raw = cc._section_bytes(obj, section)
    relocations: list[dict[str, Any]] = info["relocations"]
    for relocation in relocations:
        address = relocation["address"]
        raw[address:address + 4] = b"\0\0\0\0"

    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoded_instructions = list(decoder.disasm(bytes(raw), 0))
    result: list[Instruction] = []
    for decoded in decoded_instructions:
        annotations = [
            "rel+0x%x %s" % (
                relocation["address"],
                relocation_text(relocation),
            )
            for relocation in relocations
            if decoded.address <= relocation["address"]
            < decoded.address + decoded.size
        ]
        rendered = decoded.mnemonic
        if decoded.op_str:
            rendered += " " + decoded.op_str
        if annotations:
            rendered += " ; " + " ; ".join(annotations)
        result.append(Instruction(
            offset=decoded.address,
            rendered=rendered,
            normalized=normalize_instruction(
                decoded.mnemonic,
                decoded.op_str,
                annotations,
            ),
        ))

    consumed = sum(instruction.size for instruction in decoded_instructions)
    if consumed != len(raw):
        raise cc.CoffError(
            "%r disassembled %d of %d bytes" % (
                function_name,
                consumed,
                len(raw),
            ))
    return result


def aligned_opcodes(
    target: Sequence[Instruction],
    ours: Sequence[Instruction],
) -> list[tuple[str, int, int, int, int]]:
    """Return difflib alignment opcodes for two normalized streams."""

    matcher = difflib.SequenceMatcher(
        None,
        [instruction.normalized for instruction in target],
        [instruction.normalized for instruction in ours],
        autojunk=False,
    )
    return matcher.get_opcodes()


def _offset(instructions: Sequence[Instruction], index: int) -> str:
    if index >= len(instructions):
        return "end"
    return "0x%x" % instructions[index].offset


def render_diff(
    target: Sequence[Instruction],
    ours: Sequence[Instruction],
    *,
    include_equal: bool = False,
    max_lines: int = 40,
) -> str:
    """Render aligned edit blocks without falling back to index zipping."""

    output = [
        "target %d instructions, ours %d instructions"
        % (len(target), len(ours))
    ]
    changed = False
    for tag, target_start, target_end, ours_start, ours_end in aligned_opcodes(
        target,
        ours,
    ):
        if tag == "equal" and not include_equal:
            continue
        if tag != "equal":
            changed = True
        output.append(
            "--- %s target[%d:%d] ours[%d:%d] @T %s @O %s"
            % (
                tag,
                target_start,
                target_end,
                ours_start,
                ours_end,
                _offset(target, target_start),
                _offset(ours, ours_start),
            )
        )
        target_rows = target[target_start:target_end]
        ours_rows = ours[ours_start:ours_end]
        if max_lines:
            target_rows = target_rows[:max_lines]
            ours_rows = ours_rows[:max_lines]
        for instruction in target_rows:
            output.append("   T %5x  %s" % (
                instruction.offset,
                instruction.rendered,
            ))
        if max_lines and target_end - target_start > max_lines:
            output.append("   T ... %d more" % (
                target_end - target_start - max_lines,
            ))
        for instruction in ours_rows:
            output.append("   O %5x  %s" % (
                instruction.offset,
                instruction.rendered,
            ))
        if max_lines and ours_end - ours_start > max_lines:
            output.append("   O ... %d more" % (
                ours_end - ours_start - max_lines,
            ))
    if not changed:
        output.append("no normalized instruction differences")
    return "\n".join(output)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Align and compare normalized Capstone streams for one COFF "
            "function. All paths are read-only; relative paths are resolved "
            "from --root."
        ),
    )
    parser.add_argument("unit", help="repository-relative unit path, without .obj")
    parser.add_argument("function", help="target COFF function symbol")
    parser.add_argument(
        "--ours-function",
        help="candidate symbol when it differs from the target symbol",
    )
    parser.add_argument(
        "--target-object",
        help="target object (default: build/split/<unit>.obj)",
    )
    parser.add_argument(
        "--ours-object",
        help="candidate object (default: build/base/<unit>.obj)",
    )
    parser.add_argument(
        "--root",
        default=str(REPO_ROOT),
        help="repository root used for default and relative paths",
    )
    parser.add_argument(
        "--include-equal",
        action="store_true",
        help="also print aligned equal blocks",
    )
    parser.add_argument(
        "--max-lines",
        type=int,
        default=40,
        help="maximum instructions shown per side of each block (0 = unlimited)",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.max_lines < 0:
        parser.error("--max-lines must be non-negative")
    try:
        unit = normalize_unit(args.unit)
        root = Path(args.root).resolve()
        target_path = resolve_path(
            root,
            args.target_object,
            default_object_path(root, "split", unit),
        )
        ours_path = resolve_path(
            root,
            args.ours_object,
            default_object_path(root, "base", unit),
        )
        target = disassemble_function(load_object(target_path), args.function)
        ours = disassemble_function(
            load_object(ours_path),
            args.ours_function or args.function,
        )
    except (OSError, ValueError, RuntimeError, cc.CoffError) as error:
        parser.error(str(error))
    print(render_diff(
        target,
        ours,
        include_equal=args.include_equal,
        max_lines=args.max_lines,
    ))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
