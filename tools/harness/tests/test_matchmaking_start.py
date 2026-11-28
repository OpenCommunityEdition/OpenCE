"""The countdown of a full small matchmade game (matchmaking.c)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["full_starts", "leave_stops", "not_ready"]

NEGATIVE_CONTROLS = {
    "short-lobby-counts": (("players < most_players", "players < most_players - 1"), "leave_stops"),
    "five-seconds-shortened": (("4999", "499"), "full_starts"),
    "pause-ignored": ((" && !network_game_server_port_countdown_paused(server)", ""), "not_ready"),
}


def generated(fault=None):
    source = read("port/linux/game/matchmaking.c")
    seconds = re.search(r"^#define MATCHMAKING_START_MILLISECONDS .*$", source, re.M).group(0)
    code = function(source, "matchmaking_start_countdown")
    if fault and fault[0] == "4999":
        seconds = mutated(seconds, *fault)
    elif fault:
        code = mutated(code, *fault)
    return (("under_test.inc", seconds + "\n" + code + "\n"),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("matchmaking_start", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("matchmaking_start", generated(fault)), case)
    assert status == CHECK_FAILED, f"matchmaking.c with a fault ({control}) passed '{case}': the test cannot see it"
