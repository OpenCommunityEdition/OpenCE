#!/usr/bin/env python3
"""Writes a mutator repository's mutator-index.toml (docs/mutators.md,
"Mutator repositories"): the list of its mutators and of the files each one
is played from, with their sizes and SHA-256 checksums.

    python tools/mutator_index.py REPOSITORY [--name NAME] [--description TEXT]
    python tools/mutator_index.py REPOSITORY --check

REPOSITORY is a folder of mutators, a folder each with a mutator.toml, as the
data root's mutators folder is. The index lists each one's mutator.toml and
the add-on it names, and nothing else of its folder, and the SHA-256 of
those files' SHA-256s one after the other: the game's for the mutator, of
which an id two mutators have gives one the first hex digits
(port_mutators.c's mutator_add). Its name and description
are kept from the index there is, unless given. --check writes nothing, and
fails if the index is not the one this would write (for a commit hook or CI).

The checks are those port/linux/src/port_mutators.c makes that need no game:
the folders' and ids' lengths and characters, the file sizes, the format,
and the add-on's name; an id two mutators have is said, not refused (the
game plays the second by another). The game makes them all again.
"""

import argparse
import hashlib
import sys
import tomllib
from pathlib import Path
from typing import Any, Dict, List

INDEX = "mutator-index.toml"
# the index's format, which the game reads
INDEX_FORMAT = 1
# port_mutators.c's: MUTATORS_FORMAT, MAXIMUM_MUTATORS,
# MAXIMUM_MUTATOR_FOLDER_LENGTH (ended), MAXIMUM_MUTATOR_FILE_SIZE and
# MAXIMUM_ADDON_FILE_SIZE
MUTATORS_FORMAT = 7
MAXIMUM_MUTATORS = 64
MAXIMUM_FOLDER_BYTES = 63
MAXIMUM_MUTATOR_FILE_SIZE = 65536
MAXIMUM_ADDON_FILE_SIZE = 64 * 1024 * 1024


class RefusedError(Exception):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def mutator_sha256(files: List[Dict[str, Any]]) -> str:
    """the SHA-256 of the files' SHA-256s, in their order"""
    return hashlib.sha256(b"".join(bytes.fromhex(entry["sha256"]) for entry in files)).hexdigest()


def text(table: Dict[str, Any], key: str, default: str, where: str) -> str:
    value = table.get(key, default)
    if not isinstance(value, str):
        raise RefusedError(f"{where}: {key} is not a string")
    return value


def file_entry(folder: Path, name: str, maximum: int) -> Dict[str, Any]:
    path = folder / name
    if not path.is_file():
        raise RefusedError(f"{folder.name}: there is no {name}")
    size = path.stat().st_size
    if size > maximum:
        raise RefusedError(f"{folder.name}: {name} is larger than {maximum} bytes")
    return {"path": name, "size": size, "sha256": sha256(path)}


def mutator_entry(folder: Path) -> Dict[str, Any]:
    name = folder.name
    where = f"{name}/mutator.toml"
    if len(name.encode("utf-8")) > MAXIMUM_FOLDER_BYTES:
        raise RefusedError(f"{name}: a folder name has {MAXIMUM_FOLDER_BYTES} bytes at most")
    if any(c in name for c in "/\\\"',") or name.startswith("."):
        raise RefusedError(f"{name}: a folder name has no slash, quote or comma, and no leading dot")
    files = [file_entry(folder, "mutator.toml", MAXIMUM_MUTATOR_FILE_SIZE)]
    try:
        table = tomllib.loads((folder / "mutator.toml").read_text(encoding="utf-8"))
    except (tomllib.TOMLDecodeError, UnicodeDecodeError) as error:
        raise RefusedError(f"{where}: it is not TOML ({error})") from None
    mutator_id = text(table, "id", name, where)
    # (port_mutators.c's id_valid; the game cuts a longer id short, which
    # could make two the same, so it is refused here)
    if (not mutator_id or any(c in ",\"\\@\x7f" or ord(c) < 0x20 for c in mutator_id)
            or len(mutator_id.encode("utf-8")) > MAXIMUM_FOLDER_BYTES):
        raise RefusedError(f"{where}: its id is empty, too long or has a comma, quote, backslash, @ or control character")
    mutator_format = table.get("format", 1)
    if not isinstance(mutator_format, int) or isinstance(mutator_format, bool):
        raise RefusedError(f"{where}: its format is not a number")
    if mutator_format > MUTATORS_FORMAT:
        raise RefusedError(f"{where}: format {mutator_format} is later than this tool's {MUTATORS_FORMAT}")
    if "addon" in table:
        addon = text(table, "addon", "", where)
        if not addon or "/" in addon or "\\" in addon or addon in (".", ".."):
            raise RefusedError(f"{where}: its addon is a file's name, with no folder")
        files.append(file_entry(folder, addon, MAXIMUM_ADDON_FILE_SIZE))
    if "script" in table:
        section = table["script"]
        if not isinstance(section, dict):
            raise RefusedError(f"{where}: its script is a table, with its file")
        script = text(section, "file", "", where)
        if not script.endswith(".lua") or "/" in script or "\\" in script or script.startswith("."):
            raise RefusedError(f"{where}: its script is a .lua file's name, with no folder")
        files.append(file_entry(folder, script, 0x40000))
    authors = table.get("authors", [])
    if isinstance(authors, str):
        authors = [authors]
    if not isinstance(authors, list) or not all(isinstance(author, str) for author in authors):
        raise RefusedError(f"{where}: authors is not a string or a list of them")
    return {
        "folder": name,
        "id": mutator_id,
        "name": text(table, "name", name, where),
        "version": text(table, "version", "", where),
        "authors": authors,
        "description": text(table, "description", "", where),
        "format": mutator_format,
        "size": sum(entry["size"] for entry in files),
        "sha256": mutator_sha256(files),
        "files": files,
    }


def quoted(value: str) -> str:
    """value as a TOML basic string"""
    escapes = {"\\": "\\\\", '"': '\\"', "\b": "\\b", "\t": "\\t", "\n": "\\n", "\f": "\\f", "\r": "\\r"}
    out = []
    for c in value:
        if c in escapes:
            out.append(escapes[c])
        elif ord(c) < 0x20 or ord(c) == 0x7F:
            out.append(f"\\u{ord(c):04X}")
        else:
            out.append(c)
    return '"' + "".join(out) + '"'


def index_text(name: str, description: str, mutators: List[Dict[str, Any]]) -> str:
    lines = [
        "# A mutator repository's index (docs/mutators.md, \"Mutator repositories\"),",
        "# written by tools/mutator_index.py: do not edit it by hand.",
        "",
        f"format = {INDEX_FORMAT}",
        f"name = {quoted(name)}",
        f"description = {quoted(description)}",
    ]
    for mutator in mutators:
        lines += [
            "",
            "[[mutator]]",
            f"folder = {quoted(mutator['folder'])}",
            f"id = {quoted(mutator['id'])}",
            f"name = {quoted(mutator['name'])}",
            f"version = {quoted(mutator['version'])}",
            "authors = [" + ", ".join(quoted(author) for author in mutator["authors"]) + "]",
            f"description = {quoted(mutator['description'])}",
            f"format = {mutator['format']}",
            f"size = {mutator['size']}",
            f"sha256 = {quoted(mutator['sha256'])}",
        ]
        for entry in mutator["files"]:
            lines += [
                "",
                "[[mutator.file]]",
                f"path = {quoted(entry['path'])}",
                f"size = {entry['size']}",
                f"sha256 = {quoted(entry['sha256'])}",
            ]
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Writes a mutator repository's mutator-index.toml.")
    parser.add_argument("repository", type=Path)
    parser.add_argument("--name", help="the repository's name (else the index's, else the folder's)")
    parser.add_argument("--description", help="what the repository is (else the index's)")
    parser.add_argument("--check", action="store_true", help="write nothing; fail if the index is not up to date")
    arguments = parser.parse_args()

    repository: Path = arguments.repository
    index_path = repository / INDEX
    previous: Dict[str, Any] = {}
    if index_path.is_file():
        try:
            previous = tomllib.loads(index_path.read_text(encoding="utf-8"))
        except (tomllib.TOMLDecodeError, UnicodeDecodeError):
            previous = {}
    name = arguments.name or previous.get("name") or repository.resolve().name
    description = arguments.description if arguments.description is not None else previous.get("description", "")

    folders = sorted(
        (folder for folder in repository.iterdir() if folder.is_dir() and (folder / "mutator.toml").is_file()),
        key=lambda folder: folder.name,
    )
    mutators = []
    ids: Dict[str, str] = {}
    try:
        if len(folders) > MAXIMUM_MUTATORS:
            raise RefusedError(f"more than {MAXIMUM_MUTATORS} mutators")
        for folder in folders:
            mutator = mutator_entry(folder)
            other = ids.get(mutator["id"].lower())
            if other:
                print(f"mutator_index: {folder.name} has the id {mutator['id']}, as {other} has: the game plays it by "
                      f"another", file=sys.stderr)
            else:
                ids[mutator["id"].lower()] = folder.name
            mutators.append(mutator)
    except RefusedError as error:
        print(f"mutator_index: {error}", file=sys.stderr)
        return 1

    written = index_text(name, description, mutators)
    current = index_path.read_text(encoding="utf-8") if index_path.is_file() else None
    if arguments.check:
        if current != written:
            print(f"mutator_index: {index_path} is not up to date: run tools/mutator_index.py {repository}",
                  file=sys.stderr)
            return 1
        return 0
    if current != written:
        index_path.write_text(written, encoding="utf-8")
    print(f"mutator_index: {index_path}: {len(mutators)} mutators")
    return 0


if __name__ == "__main__":
    sys.exit(main())
