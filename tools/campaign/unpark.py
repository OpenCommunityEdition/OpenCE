"""Remove ordinary parked entries whose current functions are strict-exact.

The command is a dry run unless ``--write`` is supplied. Assembly-backed
parks remain parked even when their objects compare equal because equality is
true by construction rather than evidence of a C reconstruction.

Examples::

    python -B -m tools.campaign.unpark
    python -B -m tools.campaign.unpark --dry-run
    python -B -m tools.campaign.unpark --write

All report, ledger, object, and function inputs are validated before a write.
When applying removals, the original JSON bytes outside the removed entry
objects are preserved and the ledger is replaced atomically.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import os
from pathlib import Path
import re
import stat
import sys
import tempfile
from typing import Any, Sequence


REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools import coff_compare as cc  # noqa: E402
from tools.campaign._coff_tools import (  # noqa: E402
    function_sections,
    load_object,
    normalize_unit,
    resolve_path,
)


NEVER_UNPARK = frozenset(("asm-implemented", "vendored-assembly"))


class UnparkError(Exception):
    """The parked ledger cannot be updated without guessing."""


@dataclass(frozen=True)
class UnitPaths:
    target: Path | None
    candidate: Path | None


@dataclass(frozen=True)
class EntrySpan:
    start: int
    end: int
    value: dict[str, Any]


def load_json(path: Path, label: str) -> tuple[bytes, dict[str, Any]]:
    try:
        raw = path.read_bytes()
        text = raw.decode("utf-8")
        document = json.loads(text)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise UnparkError("cannot read %s %s: %s" % (label, path, error)) from error
    if not isinstance(document, dict):
        raise UnparkError("%s root must be a JSON object" % label)
    return raw, document


def report_unit_paths(
    document: dict[str, Any],
    root: Path,
) -> dict[str, UnitPaths]:
    units = document.get("units")
    if not isinstance(units, list):
        raise UnparkError("object report must contain a units list")

    result = {}
    for index, entry in enumerate(units):
        if not isinstance(entry, dict):
            raise UnparkError("report unit %d is not an object" % index)
        name = entry.get("name")
        target = entry.get("target_path")
        candidate = entry.get("base_path")
        if not isinstance(name, str) or not name:
            raise UnparkError("report unit %d lacks a nonempty name" % index)
        if target is not None and (not isinstance(target, str) or not target):
            raise UnparkError("report unit %d has an invalid target_path" % index)
        if candidate is not None and (not isinstance(candidate, str) or not candidate):
            raise UnparkError("report unit %d has an invalid base_path" % index)
        try:
            unit = normalize_unit(name)
        except ValueError as error:
            raise UnparkError("invalid report unit %r: %s" % (name, error)) from error
        if unit in result:
            raise UnparkError("duplicate report unit %r" % unit)
        result[unit] = UnitPaths(
            target=(resolve_path(root, target, root / target) if target else None),
            candidate=(
                resolve_path(root, candidate, root / candidate)
                if candidate
                else None
            ),
        )
    return result


def parked_entries(document: dict[str, Any]) -> list[dict[str, Any]]:
    entries = document.get("entries")
    if not isinstance(entries, list):
        raise UnparkError("parked ledger must contain an entries list")

    seen = set()
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            raise UnparkError("parked entry %d is not an object" % index)
        unit = entry.get("unit")
        function = entry.get("function")
        classification = entry.get("class")
        if not isinstance(unit, str) or not unit.strip():
            raise UnparkError("parked entry %d lacks a nonempty unit" % index)
        if not isinstance(function, str) or not function:
            raise UnparkError("parked entry %d lacks a nonempty function" % index)
        if not isinstance(classification, str) or not classification:
            raise UnparkError("parked entry %d lacks a nonempty class" % index)
        try:
            key = (normalize_unit(unit.strip()), function)
        except ValueError as error:
            raise UnparkError("invalid parked unit %r: %s" % (unit, error)) from error
        if key in seen:
            raise UnparkError("duplicate parked entry %s::%s" % key)
        seen.add(key)
    return entries


def exact_entry_keys(
    entries: Sequence[dict[str, Any]],
    paths: dict[str, UnitPaths],
) -> list[tuple[str, str]]:
    """Return removable keys after validating every inspected object/function."""

    cache: dict[str, tuple[dict[str, Any], dict[str, Any]]] = {}
    removable = []
    for entry in entries:
        unit = normalize_unit(entry["unit"].strip())
        function = entry["function"]
        if entry["class"] in NEVER_UNPARK:
            continue
        if unit not in paths:
            raise UnparkError("parked unit is absent from object report: %s" % unit)

        if unit not in cache:
            unit_paths = paths[unit]
            if unit_paths.target is None:
                raise UnparkError("parked unit has no target path: %s" % unit)
            if unit_paths.candidate is None:
                raise UnparkError("parked unit has no candidate path: %s" % unit)
            if not unit_paths.target.is_file():
                raise UnparkError("target object does not exist: %s" % unit_paths.target)
            if not unit_paths.candidate.is_file():
                raise UnparkError(
                    "candidate object does not exist: %s" % unit_paths.candidate
                )
            try:
                cache[unit] = (
                    load_object(unit_paths.target),
                    load_object(unit_paths.candidate),
                )
            except (OSError, cc.CoffError) as error:
                raise UnparkError("cannot load objects for %s: %s" % (unit, error)) from error

        target, candidate = cache[unit]
        target_functions = function_sections(target)
        candidate_functions = function_sections(candidate)
        if function not in target_functions:
            raise UnparkError(
                "target function is missing or ambiguously owned: %s::%s"
                % (unit, function)
            )
        candidate_function = candidate_functions.get(function)
        if candidate_function is not None and cc.section_infos_equal(
            target_functions[function].info,
            candidate_function.info,
        ):
            removable.append((unit, function))
    return removable


def _scan_json_value(text: str, start: int) -> int:
    """Return the exclusive end of one object/array JSON value."""

    opening = text[start]
    if opening not in "{[":
        raise UnparkError("entries array contains a non-container value")
    expected = ["}" if opening == "{" else "]"]
    in_string = False
    escaped = False
    for index in range(start + 1, len(text)):
        character = text[index]
        if in_string:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                in_string = False
            continue
        if character == '"':
            in_string = True
        elif character in "{[":
            expected.append("}" if character == "{" else "]")
        elif character in "}]":
            if not expected or character != expected.pop():
                raise UnparkError("mismatched delimiter in entries array")
            if not expected:
                return index + 1
    raise UnparkError("unterminated value in entries array")


def entry_spans(text: str, expected: Sequence[dict[str, Any]]) -> list[EntrySpan]:
    """Locate every entry object while retaining its exact textual span."""

    matches = list(re.finditer(r'(?m)^[ \t]*"entries"[ \t]*:[ \t]*\[', text))
    if len(matches) != 1:
        raise UnparkError("expected exactly one top-level entries array")
    array_start = text.find("[", matches[0].start(), matches[0].end())
    position = array_start + 1
    spans = []

    while True:
        while position < len(text) and text[position].isspace():
            position += 1
        if position >= len(text):
            raise UnparkError("unterminated entries array")
        if text[position] == "]":
            break
        if text[position] != "{":
            raise UnparkError("entries array element is not an object")
        end = _scan_json_value(text, position)
        try:
            value = json.loads(text[position:end])
        except json.JSONDecodeError as error:
            raise UnparkError("cannot parse parked entry span: %s" % error) from error
        if not isinstance(value, dict):
            raise UnparkError("entries array element is not an object")
        spans.append(EntrySpan(position, end, value))
        position = end
        while position < len(text) and text[position].isspace():
            position += 1
        if position >= len(text):
            raise UnparkError("unterminated entries array")
        if text[position] == ",":
            position += 1
            continue
        if text[position] != "]":
            raise UnparkError("entries array elements are not comma-separated")
        break

    if [span.value for span in spans] != list(expected):
        raise UnparkError("textual entry spans do not match parsed ledger order")
    return spans


def remove_entries_preserving_format(
    raw: bytes,
    document: dict[str, Any],
    keys: set[tuple[str, str]],
) -> bytes:
    """Remove selected entry objects without reserializing retained content."""

    if not keys:
        return raw
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as error:
        raise UnparkError("parked ledger is not UTF-8: %s" % error) from error
    entries = parked_entries(document)
    spans = entry_spans(text, entries)
    entry_keys = [
        (normalize_unit(entry["unit"].strip()), entry["function"])
        for entry in entries
    ]
    missing = keys - set(entry_keys)
    if missing:
        unit, function = sorted(missing)[0]
        raise UnparkError("requested entry is absent: %s::%s" % (unit, function))

    drop = {index for index, key in enumerate(entry_keys) if key in keys}
    ranges = []
    leading = 0
    while leading < len(spans) and leading in drop:
        leading += 1
    if leading:
        end = spans[leading].start if leading < len(spans) else spans[-1].end
        ranges.append((spans[0].start, end))

    for index in sorted(drop):
        if index < leading:
            continue
        separator = text[spans[index - 1].end:spans[index].start]
        if separator.count(",") != 1:
            raise UnparkError("cannot identify unique separator before entry %d" % index)
        start = spans[index - 1].end + separator.index(",")
        ranges.append((start, spans[index].end))

    ordered_ranges = sorted(ranges)
    for left, right in zip(ordered_ranges, ordered_ranges[1:]):
        if left[1] > right[0]:
            raise UnparkError("computed removal ranges overlap")
    for start, end in reversed(ordered_ranges):
        text = text[:start] + text[end:]

    try:
        result = json.loads(text)
    except json.JSONDecodeError as error:
        raise UnparkError("format-preserving removal produced invalid JSON: %s" % error) from error
    expected_entries = [
        entry for entry, key in zip(entries, entry_keys)
        if key not in keys
    ]
    expected_document = dict(document)
    expected_document["entries"] = expected_entries
    if result != expected_document:
        raise UnparkError("format-preserving removal changed retained JSON content")
    return text.encode("utf-8")


def atomic_replace(path: Path, expected: bytes, replacement: bytes) -> None:
    """Replace *path* atomically, refusing a concurrent ledger change."""

    if path.read_bytes() != expected:
        raise UnparkError("parked ledger changed during the audit; refusing to write")
    temporary_path = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb",
            dir=str(path.parent),
            prefix=".%s." % path.name,
            suffix=".tmp",
            delete=False,
        ) as stream:
            temporary_path = Path(stream.name)
            stream.write(replacement)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temporary_path, stat.S_IMODE(path.stat().st_mode))
        os.replace(temporary_path, path)
        temporary_path = None
    except OSError as error:
        raise UnparkError("cannot atomically replace %s: %s" % (path, error)) from error
    finally:
        if temporary_path is not None:
            try:
                temporary_path.unlink()
            except OSError:
                pass


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Remove non-assembly parked entries whose built candidate functions "
            "strictly match January; dry-run is the default."
        ),
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--dry-run",
        action="store_true",
        help="report removals without writing (default)",
    )
    mode.add_argument(
        "--write",
        action="store_true",
        help="atomically remove verified entries from the parked ledger",
    )
    parser.add_argument(
        "--root",
        default=str(REPO_ROOT),
        help="repository root used to resolve relative paths",
    )
    parser.add_argument("--report", help="object report override")
    parser.add_argument("--parked", help="parked ledger override")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    report_path = resolve_path(root, args.report, root / "objdiff.json")
    parked_path = resolve_path(root, args.parked, root / "config" / "parked.json")

    try:
        _, report = load_json(report_path, "object report")
        parked_raw, parked_document = load_json(parked_path, "parked ledger")
        paths = report_unit_paths(report, root)
        entries = parked_entries(parked_document)
        removable = exact_entry_keys(entries, paths)
        replacement = remove_entries_preserving_format(
            parked_raw,
            parked_document,
            set(removable),
        )
        if args.write and removable:
            atomic_replace(parked_path, parked_raw, replacement)
    except (OSError, ValueError, UnparkError) as error:
        print("unpark: error: %s" % error, file=sys.stderr)
        return 2

    for unit, function in removable:
        print("now exact, unpark: %s::%s" % (unit, function))
    print("== %d removable, %d retained" % (len(removable), len(entries) - len(removable)))
    if args.write:
        if removable:
            print("wrote %s atomically" % parked_path)
        else:
            print("no ledger change")
    else:
        print("dry run; pass --write to apply")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
