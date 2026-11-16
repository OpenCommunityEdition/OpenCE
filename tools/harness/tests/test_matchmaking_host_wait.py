"""A matchmade host's game whose own player never counts (matchmaking.c)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["never_counted", "counted"]

NEGATIVE_CONTROLS = {
    "never-given-up": (("seconds >= MATCHMAKING_PLAYER_SECONDS + P2P_MM_HOST_WAIT_SECONDS", "0"), "never_counted"),
    "given-up-with-a-player": (("return !players && ", "return "), "counted"),
}


def generated(fault=None):
    source = read("port/linux/game/matchmaking.c")
    seconds = next(line for line in source.splitlines() if line.startswith("#define MATCHMAKING_PLAYER_SECONDS"))
    code = function(source, "matchmaking_host_gave_up")
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", "\n".join([seconds, enum_with(read("port/linux/src/p2p.h"), "P2P_MM_HOST_WAIT_SECONDS"),
                                          code]) + "\n"),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("matchmaking_host_wait", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("matchmaking_host_wait", generated(fault)), case)
    assert status == CHECK_FAILED, f"matchmaking.c with a fault ({control}) passed '{case}': the test cannot see it"
