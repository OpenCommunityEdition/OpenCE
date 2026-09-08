"""Safely merge disjoint candidate edits against one common base file.

Each candidate must be a complete copy derived from the same base. The tool
computes candidate-to-base hunks, rejects every ambiguous overlap (including
two insertions at the same base position), preserves one consistent line-ending
style, and writes atomically beneath scratch/.

Usage:

    python -B tools/campaign/merge_candidates.py \
        scratch/base.c scratch/merged.c scratch/candidate-a.c \
        scratch/candidate-b.c

The output is only a review artifact. Diff and gate it before applying any
change to source/.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import difflib
import os
from pathlib import Path
import re
import sys
import tempfile
from typing import Sequence


REPO_ROOT = Path(__file__).resolve().parents[2]


class MergeError(Exception):
    """A candidate set cannot be merged without guessing."""


@dataclass(frozen=True)
class Hunk:
    """One candidate replacement of a half-open base-line interval."""

    start: int
    end: int
    replacement: tuple[str, ...]
    source: str


def line_ending_style(text: str, label: str) -> str | None:
    """Return the sole newline spelling in *text*, rejecting mixed styles."""

    styles = set(re.findall(r"\r\n|\r|\n", text))
    if len(styles) > 1:
        rendered = ", ".join(repr(style) for style in sorted(styles))
        raise MergeError("%s has mixed line endings: %s" % (label, rendered))
    return next(iter(styles), None)


def candidate_hunks(
    base_lines: Sequence[str],
    candidate_lines: Sequence[str],
    source: str,
) -> list[Hunk]:
    """Return every non-equal candidate opcode as a base-relative hunk."""

    matcher = difflib.SequenceMatcher(
        None,
        base_lines,
        candidate_lines,
        autojunk=False,
    )
    return [
        Hunk(start, end, tuple(candidate_lines[candidate_start:candidate_end]), source)
        for opcode, start, end, candidate_start, candidate_end in matcher.get_opcodes()
        if opcode != "equal"
    ]


def hunks_conflict(left: Hunk, right: Hunk) -> bool:
    """Return whether combining two base-relative hunks is ambiguous.

    Insertions are points rather than intervals. A point conflicts with
    another insertion at that point and with either boundary of a replacement;
    accepting those cases would impose an arbitrary ordering absent from both
    independently edited candidates.
    """

    left_insert = left.start == left.end
    right_insert = right.start == right.end
    if left_insert and right_insert:
        return left.start == right.start
    if left_insert:
        return right.start <= left.start <= right.end
    if right_insert:
        return left.start <= right.start <= left.end
    return max(left.start, right.start) < min(left.end, right.end)


def merge_candidate_texts(
    base: str,
    candidates: Sequence[tuple[str, str]],
    base_label: str = "base",
) -> tuple[str, list[Hunk]]:
    """Merge candidate texts or raise MergeError fail-closed."""

    base_style = line_ending_style(base, base_label)
    base_lines = base.splitlines(keepends=True)
    all_hunks: list[Hunk] = []

    for source, candidate in candidates:
        candidate_style = line_ending_style(candidate, source)
        if candidate_style != base_style:
            raise MergeError(
                "%s line endings %r differ from %s %r"
                % (source, candidate_style, base_label, base_style)
            )
        all_hunks.extend(candidate_hunks(
            base_lines,
            candidate.splitlines(keepends=True),
            source,
        ))

    if not all_hunks:
        raise MergeError("no candidate changes to merge")

    all_hunks.sort(key=lambda hunk: (hunk.start, hunk.end, hunk.source))
    for left, right in zip(all_hunks, all_hunks[1:]):
        if hunks_conflict(left, right):
            raise MergeError(
                "overlapping edits: %s touches %s and %s touches %s"
                % (
                    left.source,
                    render_span(left),
                    right.source,
                    render_span(right),
                )
            )

    merged: list[str] = []
    cursor = 0
    for hunk in all_hunks:
        merged.extend(base_lines[cursor:hunk.start])
        merged.extend(hunk.replacement)
        cursor = hunk.end
    merged.extend(base_lines[cursor:])
    return "".join(merged), all_hunks


def render_span(hunk: Hunk) -> str:
    """Render a human-oriented base position for one hunk."""

    if hunk.start == hunk.end:
        return "insertion point before line %d" % (hunk.start + 1)
    return "base lines %d-%d" % (hunk.start + 1, hunk.end)


def resolve_cli_path(root: Path, value: str) -> Path:
    """Resolve a CLI path relative to the selected repository root."""

    path = Path(value)
    if not path.is_absolute():
        path = root / path
    return path.resolve()


def require_scratch_output(root: Path, output: Path) -> None:
    """Reject an output path outside this worktree's scratch directory."""

    scratch = (root / "scratch").resolve()
    try:
        output.relative_to(scratch)
    except ValueError as error:
        raise MergeError(
            "output must be beneath %s; source/config output is forbidden"
            % scratch
        ) from error


def atomic_write(path: Path, data: bytes) -> None:
    """Atomically replace *path* from a closed sibling temporary file."""

    path.parent.mkdir(parents=True, exist_ok=True)
    temporary_path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb",
            dir=str(path.parent),
            prefix=".%s." % path.name,
            suffix=".tmp",
            delete=False,
        ) as stream:
            temporary_path = Path(stream.name)
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        temporary_path.replace(path)
    finally:
        if temporary_path is not None and temporary_path.exists():
            temporary_path.unlink()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Merge non-overlapping whole-file candidate copies against one "
            "common base and atomically write a scratch review artifact."
        ),
    )
    parser.add_argument("base", help="common base file")
    parser.add_argument("output", help="output path beneath <root>/scratch")
    parser.add_argument("candidates", nargs="+", help="candidate copies")
    parser.add_argument(
        "--root",
        default=str(REPO_ROOT),
        help="repository root used for relative paths (default: %(default)s)",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    base_path = resolve_cli_path(root, args.base)
    output_path = resolve_cli_path(root, args.output)
    candidate_paths = [
        resolve_cli_path(root, candidate)
        for candidate in args.candidates
    ]

    try:
        require_scratch_output(root, output_path)
        inputs = [base_path, *candidate_paths]
        if output_path in inputs:
            raise MergeError("output must not alias the base or a candidate")
        base = base_path.read_bytes().decode("latin-1")
        candidates = [
            (str(path), path.read_bytes().decode("latin-1"))
            for path in candidate_paths
        ]
        merged, hunks = merge_candidate_texts(
            base,
            candidates,
            base_label=str(base_path),
        )
        atomic_write(output_path, merged.encode("latin-1"))
    except (MergeError, OSError) as error:
        print("merge_candidates: error: %s" % error, file=sys.stderr)
        return 1

    by_source: dict[str, int] = {}
    for hunk in hunks:
        by_source[hunk.source] = by_source.get(hunk.source, 0) + 1
        print("take   %-48s %s" % (hunk.source, render_span(hunk)))
    print(
        "wrote %s atomically: %d hunk(s) from %d changed candidate(s)"
        % (output_path, len(hunks), len(by_source))
    )
    print("Review the diff and gate every affected function before installation.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
