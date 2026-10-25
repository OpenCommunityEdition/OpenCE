"""tools/engine_timings.py's parts that need no build: the test binaries
read from cargo's messages, and the table the timings print as."""

from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import engine_timings  # noqa: E402


def artifact(name: str, kind: str, executable: str | None, manifest: str, test: bool = True) -> str:
    return json.dumps(
        {
            "reason": "compiler-artifact",
            "target": {"name": name, "kind": [kind]},
            "profile": {"test": test},
            "executable": executable,
            "manifest_path": manifest,
        }
    )


def test_the_test_binaries_are_read_from_cargos_messages() -> None:
    lines = [
        artifact("opence_sim", "lib", None, "/e/crates/sim/Cargo.toml"),
        artifact("opence_sim", "lib", "/t/debug/deps/opence_sim-1", "/e/crates/sim/Cargo.toml"),
        artifact("opence", "bin", "/t/debug/opence", "/e/crates/cli/Cargo.toml", test=False),
        artifact("ai_reference", "test", "/t/debug/deps/ai_reference-2", "/e/crates/sim/Cargo.toml"),
        json.dumps({"reason": "build-finished", "success": True}),
        "not json",
    ]
    suites = engine_timings.test_binaries(lines)
    assert [suite.name for suite in suites] == ["opence_sim (lib)", "ai_reference"]
    assert suites[1].executable == Path("/t/debug/deps/ai_reference-2")
    assert suites[1].package == Path("/e/crates/sim")


def test_suites_of_the_same_name_are_told_apart_by_their_crate() -> None:
    lines = [
        artifact("scenarios", "test", "/t/debug/deps/scenarios-1", "/e/crates/campaign/Cargo.toml"),
        artifact("scenarios", "test", "/t/debug/deps/scenarios-2", "/e/crates/script/Cargo.toml"),
        artifact("levels", "test", "/t/debug/deps/levels-3", "/e/crates/campaign/Cargo.toml"),
    ]
    names = [suite.name for suite in engine_timings.test_binaries(lines)]
    assert names == ["scenarios (campaign)", "scenarios (script)", "levels"]


def test_the_table_lists_each_step_and_the_slowest_suites() -> None:
    timings = {
        "steps": {"build_dev_cold": 101.25, "build_dev_touch": 9.5},
        "suites": {"a": {"seconds": 3.0, "passed": True}, "b": {"seconds": 900.0, "passed": False}},
    }
    table = engine_timings.table(timings, slowest=1)
    assert "| build_dev_cold | 101.2 s |" in table
    assert "| test suites (2) | 903.0 s |" in table
    assert "| b | 900.0 s | failed |" in table
    assert "| a |" not in table
