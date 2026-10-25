"""The workspace's cargo profiles: the optimised profile the game-data
tests run in keeps the debug checks."""

from __future__ import annotations

import tomllib
from pathlib import Path

CARGO = Path(__file__).resolve().parent.parent / "engine" / "Cargo.toml"


def test_the_optimised_test_profile_keeps_the_checks() -> None:
    profile = tomllib.loads(CARGO.read_text())["profile"]["test-opt"]
    assert profile["inherits"] == "release"
    assert profile["debug-assertions"] is True
    assert profile["overflow-checks"] is True
    assert profile["incremental"] is True


def test_the_debug_builds_dependencies_are_optimised() -> None:
    # (the workspace's own crates stay at opt-level 0, to rebuild fast)
    profile = tomllib.loads(CARGO.read_text())["profile"]["dev"]
    assert profile["package"]["*"]["opt-level"] == 2
    assert "opt-level" not in profile
