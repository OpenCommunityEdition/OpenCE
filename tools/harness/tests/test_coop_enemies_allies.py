"""Co-op enemy counts treat humans as the players' allies before any script allies them."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["start", "allied"]


def generated(fault=None):
    code = function(read("port/linux/game/coop_enemies.c"), "coop_enemies_team_against_players")
    if fault:
        code = mutated(code, *fault)
    return (
        ("teams.inc", enum_with(read("source/game/game.h"), "_game_team_human,")),
        ("under_test.inc", code),
    )


@pytest.mark.parametrize("case", CASES)
def test_allies(case):
    status, output = run(build("coop_enemies_allies", generated()), case)
    assert status == 0, output


def test_negative_control():
    # only the players' own team excluded
    fault = ("team_index == _game_team_player || team_index == _game_team_human", "team_index == _game_team_player")
    status, output = run(build("coop_enemies_allies", generated(fault)), "start")
    assert status == CHECK_FAILED, output
