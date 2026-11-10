"""MATCHMAKING's SEARCH refused when player 1's profile cannot play online (menu_functions.c)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["invalid_name_refused", "no_profile_refused", "valid_name_searches"]

NEGATIVE_CONTROLS = {
    "no-name-check": (("if (!ui_widget_port_multiplayer_name_valid(0, &profile))\n\t\treturn FALSE;", ""),
                      "invalid_name_refused"),
    "no-profile-check": (("if (!campaign_profile(0, &profile))\n\t\treturn campaign_fail();",
                          "campaign_profile(0, &profile);"), "no_profile_refused"),
    "every-name-valid": (("if (player_name_valid(profile->player_name, NUMBEROF(profile->player_name)))", "if (1)"),
                         "invalid_name_refused"),
}


def generated(fault=None):
    menus = read("port/linux/game/menu_functions.c")
    code = "\n".join([function(read("source/interface/ui_widget_event_handler_functions.c"),
                               "ui_widget_port_multiplayer_name_valid")] +
                     [function(menus, name) for name in
                      ("campaign_fail", "matchmaking_search_allowed", "matchmaking_screen_search")])
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", enum_with(menus, "MATCHMAKING_SEARCH") + "\n" + code + "\n"),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("matchmaking_search_profile", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("matchmaking_search_profile", generated(fault)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
