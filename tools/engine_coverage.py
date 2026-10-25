#!/usr/bin/env python3
"""Checks each of the engine's crates against its coverage floor.

  engine_coverage.py [--write-floors]

Runs the workspace's tests under nextest with cargo llvm-cov (in the
test-opt profile, as the test step runs them), counts the lines each
crate's src/ has and the tests covered, and fails when a crate is below
its floor in engine/coverage.toml. --write-floors sets the floors a point
below today's numbers instead. The floors are measured with the user's
maps in assets/maps, which many tests need: without them the numbers are
printed and not checked. Run it inside nix develop.
"""

from __future__ import annotations

import argparse
import json
import math
import subprocess
import sys
import tempfile
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ENGINE = ROOT / "engine"
FLOORS = ENGINE / "coverage.toml"


def coverage_command(output: Path, profile: str = "test-opt") -> list[str]:
    """The workspace's tests under nextest, instrumented, writing llvm-cov's
    summary of each file to `output`."""
    return [
        "cargo",
        "llvm-cov",
        "nextest",
        "--locked",
        "--workspace",
        "--cargo-profile",
        profile,
        "--json",
        "--summary-only",
        "--output-path",
        str(output),
    ]


def per_crate(report: dict, engine: Path = ENGINE) -> dict[str, tuple[int, int]]:
    """Each crate's (lines covered, lines) from llvm-cov's JSON report:
    the lines of the files in its src/, not its tests' or a dependency's."""
    crates = (engine / "crates").resolve()
    coverage: dict[str, tuple[int, int]] = {}
    for export in report["data"]:
        for file in export["files"]:
            try:
                parts = Path(file["filename"]).resolve().relative_to(crates).parts
            except ValueError:
                continue
            if len(parts) < 3 or parts[1] != "src":
                continue
            lines = file["summary"]["lines"]
            covered, count = coverage.get(parts[0], (0, 0))
            coverage[parts[0]] = (covered + lines["covered"], count + lines["count"])
    return coverage


def percent(covered: int, count: int) -> float:
    return 100.0 * covered / count if count else 0.0


def shortfalls(coverage: dict[str, tuple[int, int]], floors: dict[str, float]) -> list[str]:
    """What is wrong with each crate below its floor, in the floors' order."""
    found = []
    for name, floor in floors.items():
        if name not in coverage:
            found.append(f"{name}: no lines in the report")
            continue
        covered = percent(*coverage[name])
        if covered < floor:
            found.append(f"{name}: {covered:.1f}% of lines covered, below its floor of {floor:.1f}%")
    return found


def floors_from(coverage: dict[str, tuple[int, int]]) -> dict[str, float]:
    """Floors a point below each crate's whole percentage today (a test
    that skips or a line moved should not fail the step)."""
    return {name: max(0.0, math.floor(percent(*lines)) - 1.0) for name, lines in sorted(coverage.items())}


def write_floors(floors: dict[str, float], path: Path = FLOORS) -> None:
    lines = [
        "# Each crate's least share of lines the workspace's tests cover, in",
        "# percent (tools/engine_coverage.py checks it; --write-floors sets it",
        "# a point below the day's numbers, measured with the user's maps).",
        "[floors]",
        *(f"{name} = {floor:.1f}" for name, floor in floors.items()),
    ]
    path.write_text("\n".join(lines) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--write-floors", action="store_true", help="set the floors from today's numbers")
    arguments = parser.parse_args()
    with tempfile.TemporaryDirectory() as folder:
        output = Path(folder) / "coverage.json"
        subprocess.run(coverage_command(output), cwd=ENGINE, check=True)
        coverage = per_crate(json.loads(output.read_text()))
    for name, lines in sorted(coverage.items()):
        print(f"{name:10} {percent(*lines):5.1f}%  ({lines[0]} of {lines[1]} lines)")
    if arguments.write_floors:
        write_floors(floors_from(coverage))
        print(f"wrote {FLOORS}")
        return
    if not (ROOT / "assets" / "maps").is_dir():
        print("no assets/maps: the floors are measured with the user's maps, so they are not checked")
        return
    found = shortfalls(coverage, tomllib.loads(FLOORS.read_text())["floors"])
    for problem in found:
        print(problem, file=sys.stderr)
    if found:
        sys.exit(1)


if __name__ == "__main__":
    main()
