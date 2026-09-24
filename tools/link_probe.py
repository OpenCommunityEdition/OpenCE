#!/usr/bin/env python3
"""Opt-in Xbox PE link attempt using the already-compiled comparison objects.

This is deliberately separate from configure.py and the byte-matching graph.
It never edits source, object files, objdiff configuration, or matching reports.
The diagnostic pass suppresses unavailable default libraries so the linker can
report unresolved symbols; neither pass uses /FORCE or claims to make an XBE.
"""

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path


DIAGNOSTIC_CODES = ("LNK2001", "LNK2005", "LNK2019", "LNK1104", "LNK1181")


def manifest_objects(
    root: Path,
    excluded_projects: set[str] | None = None,
    excluded_sources: set[str] | None = None,
    replacements: dict[str, Path] | None = None,
) -> tuple[list[Path], list[dict[str, str]]]:
    config = json.loads((root / "config" / "config.json").read_text(encoding="utf-8"))
    objects: list[Path] = []
    missing: list[dict[str, str]] = []
    for project in config["projects"]:
        if excluded_projects and project["name"] in excluded_projects:
            continue
        for entry in project["objects"]:
            if entry["status"] == "MISSING" or (excluded_sources and entry["name"] in excluded_sources):
                continue
            path = (replacements or {}).get(entry["name"])
            if path is None:
                path = root / "build" / "base" / Path(entry["name"]).with_suffix(".obj")
            if path.is_file():
                objects.append(path.resolve())
            else:
                missing.append({"project": project["name"], "source": entry["name"], "object": str(path)})
    return objects, missing


def object_digest(objects: list[Path]) -> str:
    digest = hashlib.sha256()
    for path in objects:
        digest.update(str(path).encode("utf-8"))
        digest.update(b"\0")
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(block)
    return digest.hexdigest()


def diagnostic_summary(output: str) -> dict[str, object]:
    lines = output.splitlines()
    counts = {code: sum(code in line for line in lines) for code in DIAGNOSTIC_CODES}
    samples = [line for line in lines if re.search(r"\bLNK\d{4}\b", line)]
    unresolved = set()
    duplicates = set()
    for line in lines:
        match = re.search(r"unresolved external symbol (.*?)(?: referenced in function|$)", line)
        if match:
            unresolved.add(match.group(1))
        duplicate = re.search(r"LNK2005: (.*?) already defined", line)
        if duplicate:
            duplicates.add(duplicate.group(1))
    summary = re.search(r"LNK1120: (\d+) unresolved externals", output)
    return {
        "counts": counts,
        "unique_unresolved_seen": len(unresolved),
        "unique_duplicate_definitions": len(duplicates),
        "duplicate_symbols": sorted(duplicates),
        "linker_unresolved_count": int(summary.group(1)) if summary else None,
        "first_diagnostics": samples[:30],
    }


def run_link(
    linker: Path,
    objects: list[Path],
    extra_objects: list[Path],
    output_dir: Path,
    label: str,
    library_dirs: list[Path],
    libraries: list[Path],
    suppress_default_libraries: bool,
) -> dict[str, object]:
    image = output_dir / f"{label}.exe"
    map_file = output_dir / f"{label}.map"
    response = output_dir / f"{label}.rsp"
    args = [
        "/NOLOGO",
        "/MACHINE:X86",
        "/SUBSYSTEM:XBOX",
        "/ENTRY:mainCRTStartup",
        "/INCREMENTAL:NO",
        "/OPT:NOREF",
        f"/OUT:{image}",
        f"/MAP:{map_file}",
    ]
    if suppress_default_libraries:
        args.append("/NODEFAULTLIB")
    args.extend(f"/LIBPATH:{path}" for path in library_dirs)
    args.extend(str(path) for path in objects)
    args.extend(str(path) for path in extra_objects)
    args.extend(str(path) for path in libraries)
    response.write_text("\n".join(f'"{arg}"' for arg in args) + "\n", encoding="ascii")
    completed = subprocess.run(
        [str(linker), f"@{response}"],
        cwd=output_dir,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    output = completed.stdout + completed.stderr
    (output_dir / f"{label}.log").write_text(output, encoding="utf-8")
    return {
        "exit_code": completed.returncode,
        "image_exists": image.is_file(),
        "response": str(response),
        "log": str(output_dir / f"{label}.log"),
        **diagnostic_summary(output),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--object-root", type=Path, default=Path.cwd())
    parser.add_argument("--output-dir", type=Path, default=Path("build/link-probe"))
    parser.add_argument("--linker", type=Path)
    parser.add_argument("--library-dir", type=Path, action="append", default=[])
    parser.add_argument("--library", type=Path, action="append", default=[])
    parser.add_argument("--extra-object", type=Path, action="append", default=[])
    parser.add_argument("--replace-object", action="append", default=[], metavar="SOURCE=OBJECT")
    parser.add_argument("--exclude-project", action="append", default=[])
    parser.add_argument("--exclude-source", action="append", default=[])
    args = parser.parse_args()

    root = args.object_root.resolve()
    output_dir = args.output_dir.resolve()
    linker = (args.linker or root / "xbox/bin/vc7/Link.Exe").resolve()
    if not linker.is_file():
        parser.error(f"linker not found: {linker}")
    if output_dir == root or output_dir.is_relative_to(root / "build/base") or output_dir.is_relative_to(root / "build/split"):
        parser.error("output must not overwrite the repository or comparison objects")
    replacements = {}
    for definition in args.replace_object:
        source, separator, object_path = definition.partition("=")
        if not separator or not source or not object_path:
            parser.error(f"invalid replacement: {definition}")
        if source in replacements:
            parser.error(f"duplicate replacement: {source}")
        replacements[source] = Path(object_path).resolve()
    config = json.loads((root / "config" / "config.json").read_text(encoding="utf-8"))
    manifest_names = {entry["name"] for project in config["projects"] for entry in project["objects"] if entry["status"] != "MISSING"}
    unknown_replacements = set(replacements) - manifest_names
    if unknown_replacements:
        parser.error(f"unknown manifest sources: {sorted(unknown_replacements)}")
    unknown_exclusions = set(args.exclude_source) - manifest_names
    if unknown_exclusions:
        parser.error(f"unknown manifest sources: {sorted(unknown_exclusions)}")
    if set(replacements) & set(args.exclude_source):
        parser.error("a source cannot be both replaced and excluded")
    objects, missing = manifest_objects(root, set(args.exclude_project), set(args.exclude_source), replacements)
    if missing:
        print(f"Cannot link: {len(missing)} manifest objects have not been compiled.")
        for entry in missing[:20]:
            print(entry["object"])
        return 2
    library_dirs = [path.resolve() for path in args.library_dir]
    libraries = [path.resolve() for path in args.library]
    extra_objects = [path.resolve() for path in args.extra_object]
    for library_dir in library_dirs:
        if not library_dir.is_dir():
            parser.error(f"library directory not found: {library_dir}")
    for library in libraries:
        if not library.is_file():
            parser.error(f"library not found: {library}")
    for extra_object in extra_objects:
        if not extra_object.is_file():
            parser.error(f"extra object not found: {extra_object}")
    output_dir.mkdir(parents=True, exist_ok=True)

    before = object_digest(objects + extra_objects)
    results = {
        "object_root": str(root),
        "object_count": len(objects),
        "excluded_projects": args.exclude_project,
        "excluded_sources": args.exclude_source,
        "extra_objects": [str(path) for path in extra_objects],
        "replaced_objects": {key: str(value) for key, value in replacements.items()},
        "object_sha256_before": before,
        "missing_manifest_objects": missing,
        "full": run_link(linker, objects, extra_objects, output_dir, "full", library_dirs, libraries, False),
        "diagnostic": run_link(linker, objects, extra_objects, output_dir, "diagnostic", library_dirs, libraries, True),
    }
    results["object_sha256_after"] = object_digest(objects + extra_objects)
    results["input_objects_unchanged"] = before == results["object_sha256_after"]
    (output_dir / "summary.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    print(f"Inputs: {len(objects)} objects; unchanged: {results['input_objects_unchanged']}")
    for label in ("full", "diagnostic"):
        result = results[label]
        print(f"{label}: exit {result['exit_code']}; {result['counts']}; log: {result['log']}")
    print(f"Summary: {output_dir / 'summary.json'}")
    if not results["input_objects_unchanged"]:
        return 3
    return 0 if results["full"]["exit_code"] == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
