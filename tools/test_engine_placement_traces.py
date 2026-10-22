"""The reference game's placement traces (tools/engine_placement_traces.py):
the recorder's lines are read whole, another level's are left out, an
unfinished trace is refused, and a small map's trace is complete (needs
`ninja macos` and assets/maps)."""

from __future__ import annotations

import csv
import platform
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import engine_placement_traces as traces  # noqa: E402

LOG = """\
halo-linux: placement trace: begin levels\\ui\\ui
halo-linux: placement trace: 6 0 12.30000 -1.80000 15.50000 0.913545 -0.400557 0.070629 -0.000000 0.173648 0.984808 0.0.0.0.0.0.0.0 -1 scenery\\halo\\halo
halo-linux: placement trace: done 0 1 1
halo-linux: network test: host:wizard
halo-linux: placement trace: begin levels\\test\\wizard\\wizard
halo-linux: placement trace: 6 3 1.00000 2.00000 3.00000 1.000000 0.000000 0.000000 0.000000 0.000000 1.000000 0.1.0.0.0.0.0.0 -1 scenery\\big tree\\big tree
halo-linux: placement trace: 7 -1 4.00000 5.00000 -6.50000 0.000000 1.000000 0.000000 0.000000 0.000000 1.000000 0.0.0.0.0.0.0.0 12 levels\\test\\wizard\\devices\\door
halo-linux: placement trace: done 0 1 2
halo-linux: placement trace: 2 0 0.00000 0.00000 0.00000 1.000000 0.000000 0.000000 0.000000 0.000000 1.000000 0.0.0.0.0.0.0.0 -1 weapons\\pistol\\pistol
"""


def test_the_lines_are_read_whole() -> None:
    rows, end = traces.parse(LOG, "levels\\test\\wizard\\wizard")
    assert end == {"bsp": 0, "difficulty": 1, "count": 2}
    assert len(rows) == 2 and all(len(row) == len(traces.COLUMNS) for row in rows)
    assert rows[0][:2] == ["6", "3"] and rows[0][-3:] == ["0.1.0.0.0.0.0.0", "-1", "scenery\\big tree\\big tree"]
    assert rows[1][1] == "-1" and rows[1][4] == "-6.50000" and rows[1][-2] == "12"


def test_another_levels_and_later_placements_are_left_out() -> None:
    rows, end = traces.parse(LOG, "levels\\ui\\ui")
    assert end["count"] == 1 and rows[0][-1] == "scenery\\halo\\halo"


def test_an_unfinished_or_short_trace_is_refused() -> None:
    with pytest.raises(ValueError):
        traces.parse(LOG.split("done 0 1 2")[0], "levels\\test\\wizard\\wizard")
    with pytest.raises(ValueError):
        traces.parse(LOG.replace("done 0 1 2", "done 0 1 3"), "levels\\test\\wizard\\wizard")
    with pytest.raises(ValueError):
        traces.parse(LOG, "levels\\a10\\a10")


def test_the_maps_games_and_scenarios() -> None:
    assert traces.network_test("a10") == "host:levels\\a10\\a10:coop"
    assert traces.network_test("wizard") == "host:wizard"
    assert traces.scenario("a10") == "levels\\a10\\a10"
    assert traces.scenario("wizard") == "levels\\test\\wizard\\wizard"
    assert len(set(traces.MULTIPLAYER + traces.CAMPAIGN)) == 23


def test_a_written_trace_reads_back(tmp_path: Path) -> None:
    rows, _ = traces.parse(LOG.replace("big tree", 'big, "tree"'), "levels\\test\\wizard\\wizard")
    path = tmp_path / "wizard.csv"
    traces.write(path, rows, 3)
    with path.open() as file:
        read = list(csv.DictReader(file))
    assert (tmp_path / "wizard.bsp").read_text() == "3\n"
    assert list(read[0]) == list(traces.COLUMNS)
    assert read[0]["tag"] == 'scenery\\big, "tree"\\big, "tree"' and read[1]["parent"] == "12"


@pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only")
@pytest.mark.skipif(not (traces.HOST.is_file() and traces.IMAGE.is_file()), reason="needs `ninja macos`")
@pytest.mark.skipif(not (traces.MAPS / "wizard.map").is_file(), reason="needs assets/maps/wizard.map")
def test_a_small_maps_trace_is_complete() -> None:
    rows, end = traces.record("wizard")
    assert end["count"] == len(rows) > 0 and end["bsp"] == 0
    assert all(0 <= int(row[0]) < 12 and int(row[1]) >= 0 for row in rows)
    assert all(row[-1] and row[-1] != "?" for row in rows)
    # (each placement once)
    assert len({(row[0], row[1]) for row in rows}) == len(rows)
