"""The controller remapping's game context asked for as input_initialize reads the pads, before the game time
globals exist (port/linux/game/controller_remap_game.c), without assets."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["startup", "playing"]

FUNCTIONS = ["remap_game_in_progress", "controller_remap_local_player", "controller_remap_playing",
             "controller_remap_context"]

# a fault in the code, and the case that must catch it
NEGATIVE_CONTROLS = {
    "unchecked": (("return game_time_initialized() && game_in_progress();", "return game_in_progress();"),
                  "startup"),
}


def generated(fault=None):
    source = read("port/linux/game/controller_remap_game.c")
    if fault:
        source = mutated(source, *fault)
    return (("under_test.inc", "\n".join(function(source, name) for name in FUNCTIONS)),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("controller_remap_startup", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("controller_remap_startup", generated(fault)), case)
    assert status == CHECK_FAILED, output
