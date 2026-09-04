"""Shared, read-only helpers for campaign COFF inspection tools."""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Any

from tools import coff_compare as cc


REPO_ROOT = Path(__file__).resolve().parents[2]
_DRIVE_PATH = re.compile(r"^[A-Za-z]:/")
_SOURCE_SUFFIXES = (".cpp", ".cxx", ".cc", ".c")


@dataclass(frozen=True)
class FunctionSection:
    """One unambiguously owned i386 code section."""

    name: str
    section_number: int
    info: dict[str, Any]


def normalize_unit(value: str) -> str:
    """Normalize a repository-relative unit name and reject path escapes."""

    normalized = value.replace("\\", "/").strip()
    if normalized.endswith(".obj"):
        normalized = normalized[:-4]
    else:
        lowered = normalized.lower()
        for suffix in _SOURCE_SUFFIXES:
            if lowered.endswith(suffix):
                normalized = normalized[:-len(suffix)]
                break

    raw_parts = normalized.split("/")
    path = PurePosixPath(normalized)
    if not normalized or path.is_absolute() or _DRIVE_PATH.match(normalized):
        raise ValueError("unit must be a repository-relative path")
    if any(part in ("", ".", "..") for part in raw_parts):
        raise ValueError("unit must not contain empty, '.' or '..' path components")
    return path.as_posix()


def resolve_path(root: Path, supplied: str | None, default: Path) -> Path:
    """Resolve a CLI path; explicit relative paths are relative to *root*."""

    if supplied is None:
        return default.resolve()
    path = Path(supplied)
    if not path.is_absolute():
        path = root / path
    return path.resolve()


def default_object_path(root: Path, tree: str, unit: str) -> Path:
    """Return ``<root>/build/<tree>/<unit>.obj`` without CWD dependence."""

    base = root / "build" / tree / Path(*unit.split("/"))
    return Path(str(base) + ".obj")


def load_object(path: Path) -> dict[str, Any]:
    """Load a COFF object without opening it for mutation."""

    return cc.load(path.read_bytes())


def function_sections(obj: dict[str, Any]) -> dict[str, FunctionSection]:
    """Return code sections that have exactly one function-like owner.

    COFF permits aliases and duplicate symbol names.  Naming proposals must not
    guess through either case, so ambiguous sections and ambiguous names are
    deliberately omitted.
    """

    owners_by_section: dict[int, set[str]] = {}
    for symbol in obj["symbols"]:
        section_number = symbol["section"]
        if section_number <= 0 or section_number > len(obj["sections"]):
            continue
        section = obj["sections"][section_number - 1]
        if not section["flags"] & cc.IMAGE_SCN_CNT_CODE:
            continue
        name = symbol["name"]
        if symbol["storage"] not in (2, 3) or symbol["value"] != 0:
            continue
        if not name.startswith(("_", "?", "@")):
            continue
        owners_by_section.setdefault(section_number, set()).add(name)

    candidates: list[FunctionSection] = []
    for section_number, names in owners_by_section.items():
        if len(names) != 1:
            continue
        name = next(iter(names))
        candidates.append(FunctionSection(
            name=name,
            section_number=section_number,
            info=cc.section_info_by_number(obj, section_number),
        ))

    counts: dict[str, int] = {}
    for candidate in candidates:
        counts[candidate.name] = counts.get(candidate.name, 0) + 1
    return {
        candidate.name: candidate
        for candidate in candidates
        if counts[candidate.name] == 1
    }


def relocation_text(relocation: dict[str, Any]) -> str:
    """Render the comparator's fail-closed relocation identity compactly."""

    target = relocation["target"]
    if isinstance(target, list):
        target_text = ":".join(str(part) for part in target)
    else:
        target_text = str(target)
    return "type=0x%04x target=%s" % (relocation["type"], target_text)
