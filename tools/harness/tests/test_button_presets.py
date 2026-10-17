"""The real button settings of player_ui.c: every layout keeps start and back and gives each control a button of its
own, Bumper Jumper is the Master Chief Collection's, and a layout this build does not know is the standard one."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["every-preset", "bumper-jumper", "unknown-preset"]

# a fault in player_ui.c and the cases that must catch it
NEGATIVE_CONTROLS = {
    "jump-on-black": (("[0] = _gamepad_analog_button_white;", "[0] = _gamepad_analog_button_black;"),
                      ["every-preset", "bumper-jumper"]),
    "no-default": (("\t\tdefault:\n", ""), ["unknown-preset"]),
}


def generated(fault=None):
    profile = read("source/saved games/player_profile.h")
    config = "\n".join([
        enum_with(read("source/input/input.h"), "_gamepad_analog_button_a"),
        enum_with(read("source/input/input_abstraction.c"), "_game_control_jump"),
        enum_with(profile, "NUMBER_OF_LOOK_SENSITIVITY_SETTINGS"),
        enum_with(profile, "_button_preset_standard"),
        enum_with(profile, "_joystick_preset_standard"),
        structure(profile, "player_profile_controller_settings"),
        structure(read("source/input/input_abstraction.h"), "game_input_preferences"),
    ]) + "\n"
    code = function(read("source/interface/player_ui.c"), "set_local_player_controls_from_player_profile")
    if fault:
        code = mutated(code, *NEGATIVE_CONTROLS[fault][0])
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_button_presets(case):
    status, output = run(build("button_presets", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("fault,case", [(fault, case) for fault, (_, cases) in NEGATIVE_CONTROLS.items()
                                        for case in cases])
def test_fault_detected(fault, case):
    status, output = run(build("button_presets", generated(fault)), case)
    assert status == CHECK_FAILED, output
