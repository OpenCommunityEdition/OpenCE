"""A matchmade game's lobby (matchmaking.c)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["leave_to_multiplayer_then_main_menu"]

NEGATIVE_CONTROLS = {
    "no-back-target": (("tag_loaded(UI_WIDGET_DEFINITION_TAG, behind[index])", "NONE"),
                       "leave_to_multiplayer_then_main_menu"),
    "no-close": (("ui_widgets_close_all();", ""), "leave_to_multiplayer_then_main_menu"),
    "no-main-menu-behind": (("\"pc\\\\main_menu\\\\main_menu\",", ""), "leave_to_multiplayer_then_main_menu"),
}


def generated(fault=None):
    code = function(read("port/linux/game/matchmaking.c"), "matchmaking_open_lobby")
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code + "\n"),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("matchmaking_lobby", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("matchmaking_lobby", generated(fault)), case)
    assert status == CHECK_FAILED, f"matchmaking.c with a fault ({control}) passed '{case}': the test cannot see it"
