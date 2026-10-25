"""The coverage floors: each crate's lines covered by the workspace's
tests, from cargo llvm-cov's report, against its floor in
engine/coverage.toml."""

from __future__ import annotations

import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ENGINE = ROOT / "engine"

sys.path.insert(0, str(ROOT / "tools"))

import engine_coverage  # noqa: E402


def report(*files: tuple[str, int, int]) -> dict:
    """A report of llvm-cov's JSON form: (file, lines, covered) each."""
    return {
        "data": [
            {
                "files": [
                    {"filename": name, "summary": {"lines": {"count": count, "covered": covered}}}
                    for name, count, covered in files
                ]
            }
        ]
    }


def test_lines_are_counted_by_the_crate_whose_src_holds_them() -> None:
    coverage = engine_coverage.per_crate(
        report(
            (f"{ENGINE}/crates/sim/src/lib.rs", 100, 80),
            (f"{ENGINE}/crates/sim/src/unit/biped.rs", 50, 10),
            (f"{ENGINE}/crates/physics/src/lib.rs", 40, 40),
            # (a test's own lines, and a dependency's, are no crate's)
            (f"{ENGINE}/crates/sim/tests/player.rs", 70, 70),
            ("/nix/store/glam/src/vec3.rs", 30, 0),
        ),
        ENGINE,
    )
    assert coverage == {"sim": (90, 150), "physics": (40, 40)}


def test_a_crate_below_its_floor_fails() -> None:
    coverage = {"sim": (90, 150), "physics": (40, 40)}
    assert engine_coverage.shortfalls(coverage, {"sim": 60.0, "physics": 90.0}) == []
    assert engine_coverage.shortfalls(coverage, {"sim": 60.1, "physics": 90.0}) == [
        "sim: 60.0% of lines covered, below its floor of 60.1%",
    ]


def test_a_crate_with_a_floor_and_no_lines_fails() -> None:
    assert engine_coverage.shortfalls({}, {"net": 50.0}) == ["net: no lines in the report"]


def test_the_floors_are_set_a_point_below_todays_numbers() -> None:
    assert engine_coverage.floors_from({"sim": (905, 1000), "ui": (0, 0)}) == {"sim": 89.0, "ui": 0.0}


def test_every_crate_has_a_floor() -> None:
    floors = tomllib.loads((ENGINE / "coverage.toml").read_text())["floors"]
    crates = {path.name for path in (ENGINE / "crates").iterdir() if (path / "Cargo.toml").exists()}
    assert set(floors) == crates


def test_the_command_runs_the_workspace_under_nextest_in_the_test_profile(tmp_path: Path) -> None:
    command = engine_coverage.coverage_command(tmp_path / "coverage.json")
    assert command[:3] == ["cargo", "llvm-cov", "nextest"]
    assert "--workspace" in command and "--summary-only" in command
    assert command[command.index("--cargo-profile") + 1] == "test-opt"
