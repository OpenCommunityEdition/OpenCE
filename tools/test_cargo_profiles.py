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


def test_the_release_build_aborts_and_is_not_linked_whole() -> None:
    # (a panic must not unwind into Swift; fat and thin LTO were measured
    # and each made devgulch's frames slower, see the rust_tools report)
    profile = tomllib.loads(CARGO.read_text())["profile"]["release"]
    assert profile["panic"] == "abort"
    assert profile.get("lto", False) is False
