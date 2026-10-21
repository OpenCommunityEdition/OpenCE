"""The native engine's reference movement traces (engine/tests/traces,
written by tools/engine_movement_traces.py): every script's file is there,
whole and physical, and the reference game records the same trace again
(the biped settles a few millimetres differently from one run to the next,
so the run is compared from its first tick, within a centimetre)."""

from __future__ import annotations

import csv
import platform
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import engine_movement_traces as traces  # noqa: E402


def read(script: str) -> list[dict[str, float]]:
    with (traces.TRACES / f"{script}.csv").open() as file:
        return [{key: float(value) for key, value in row.items()} for row in csv.DictReader(file)]


@pytest.mark.parametrize("script", traces.SCRIPTS)
def test_each_trace_is_whole(script: str) -> None:
    rows = read(script)
    assert [int(row["tick"]) for row in rows] == list(range(len(rows)))
    assert len(rows) >= 60
    assert list(rows[0]) == list(traces.COLUMNS)
    # (a player's biped moves at most its run speed on the ground, a tick)
    for row in rows:
        if not row["airborne"]:
            assert (row["vx"] ** 2 + row["vy"] ** 2) ** 0.5 < 0.1, row


def test_the_run_reaches_its_speed_and_stops() -> None:
    rows = read("run")
    speed = (rows[30]["vx"] ** 2 + rows[30]["vy"] ** 2) ** 0.5
    assert 0.06 < speed < 0.09
    assert rows[-1]["vx"] == rows[-1]["vy"] == 0


def test_the_jump_rises_and_lands() -> None:
    rows = read("jump")
    peak = max(row["z"] for row in rows)
    assert peak - rows[0]["z"] > 0.5
    assert any(row["airborne"] for row in rows)
    assert not rows[-1]["airborne"]
    assert abs(rows[-1]["z"] - rows[0]["z"]) < 0.05


def test_the_fall_accelerates_by_gravity() -> None:
    rows = read("fall")
    falling = [row for row in rows if row["airborne"] and row["airborne_ticks"] > 2]
    steps = [b["vz"] - a["vz"] for a, b in zip(falling, falling[1:])]
    assert steps and all(abs(step - steps[0]) < 1e-4 for step in steps)
    assert any(row["landing"] >= 0 for row in rows)
    assert not rows[-1]["airborne"] and rows[-1]["body"] > 0


@pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only")
@pytest.mark.skipif(not (traces.HOST.is_file() and traces.IMAGE.is_file()), reason="needs `ninja macos`")
@pytest.mark.skipif(not (traces.MAPS / "bloodgulch.map").is_file(), reason="needs assets/maps/bloodgulch.map")
def test_the_reference_game_records_the_same_jump() -> None:
    recorded = [[float(value) for value in row] for row in traces.record("jump")]
    expected = [[row[key] for key in traces.COLUMNS] for row in read("jump")]
    assert len(recorded) == len(expected)
    for got, want in zip(recorded, expected):
        assert got[0] == want[0]
        for column in (1, 2, 3):
            assert abs((got[column] - recorded[0][column]) - (want[column] - expected[0][column])) < 0.01, (got, want)
        for column in (4, 5, 6):
            assert abs(got[column] - want[column]) < 0.002, (got, want)
        assert got[8:] == want[8:], (got, want)
