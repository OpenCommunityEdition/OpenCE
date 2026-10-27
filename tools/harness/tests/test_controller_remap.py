"""The real controller button layouts: their text (ten different buttons, else none), swapping and cycling, the
buttons' names on each controller, the settings read again only when one changes; a playing player's pad moved to
their layout (a trigger's pull kept, start, back, the sticks and the d-pad left alone unless a layout uses its left,
a button held as a layout begins doing nothing until let go); the capture of a button (not the press that opened it,
start, back, escape, another controller or a menu gone cancelling, the other players' menus held meanwhile); and the
game's side (the profile's layout, else the standard one; the controller's player; the HUD prompts' buttons)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, mutated, read, run, structure  # noqa: E402

CASES = ["layout-text", "swap-cycle", "names", "settings-cached", "game-profile", "playing-player", "translate",
         "held-at-start", "capture"]

LAYOUTS = "port/linux/src/controller_remap.c"
INPUT = "port/linux/src/controller_remap_input.c"
GAME = "port/linux/game/controller_remap_game.c"

# a fault in the code (its file, the change) and the cases that must catch it
NEGATIVE_CONTROLS = {
    "duplicates-taken": ((LAYOUTS, "(used & (1u << index))", "(used & 0)"), ["layout-text"]),
    "swap-overwrites": ((LAYOUTS, "\t\t\tbuttons[other] = buttons[action];\n", ""), ["swap-cycle"]),
    "never-cached": ((LAYOUTS, "\t\tread_at = config_changes();\n", ""), ["settings-cached"]),
    "bad-profile-used": ((GAME, "!controller_remap_bindable(button) || ", ""), ["game-profile"]),
    "controller-as-player": ((GAME, "return port != NONE ? port : local_player_index;", "return local_player_index;"),
                             ["playing-player"]),
    "player-as-controller": ((GAME, "return local_player_index != NONE ? local_player_index : port;", "return port;"),
                             ["playing-player"]),
    "dpad-left-kept": ((INPUT, "if (layout[action] == _remap_button_dpad_left || targets[action] == _remap_button_dpad_left)",
                        "if (layout[action] == NUMBER_OF_REMAP_BUTTONS || targets[action] == NUMBER_OF_REMAP_BUTTONS)"),
                       ["translate"]),
    "held-acts": ((INPUT, "\t\tremap_ports[port].blocked = held;\n", ""), ["held-at-start"]),
    "opening-press-binds": ((INPUT, "capture.phase == _capture_releasing && !held", "capture.phase == _capture_releasing"),
                            ["capture"]),
    "drain-blocks-others": ((INPUT, "capture.phase != _capture_idle && capture.phase != _capture_draining &&",
                             "capture.phase != _capture_idle &&"), ["capture"]),
}


def without_includes(text):
    return "\n".join(line for line in text.splitlines() if not line.startswith("#include")) + "\n"


def generated(fault=None):
    sources = {LAYOUTS: read(LAYOUTS), INPUT: read(INPUT), GAME: read(GAME)}
    if fault:
        file, before, after = NEGATIVE_CONTROLS[fault][0]
        sources[file] = mutated(sources[file], before, after)
    xbox = read("port/include/xdk/xdk_xbox.h")
    config = "\n".join([
        enum_with(read("source/input/input.h"), "_gamepad_analog_button_a"),
        enum_with(read("source/input/input_abstraction.c"), "_game_control_jump"),
        structure(read("source/input/input_abstraction.h"), "game_input_preferences"),
        structure(read("port/include/xdk/xdk_pdb.h"), "_XINPUT_GAMEPAD"),
        "typedef struct _XINPUT_GAMEPAD XINPUT_GAMEPAD;",
        *re.findall(r"^#define XINPUT_GAMEPAD_\w+ \w+$", xbox, re.M),
        read("port/linux/include/controller_buttons.h"),
        read("port/linux/include/controller_remap.h"),
    ]) + "\n"
    return (("config.inc", config), ("layouts.inc", without_includes(sources[LAYOUTS])),
            ("input.inc", without_includes(sources[INPUT])), ("game.inc", without_includes(sources[GAME])))


@pytest.mark.parametrize("case", CASES)
def test_controller_remap(case):
    status, output = run(build("controller_remap", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("fault,case", [(fault, case) for fault, (_, cases) in NEGATIVE_CONTROLS.items()
                                        for case in cases])
def test_fault_detected(fault, case):
    status, output = run(build("controller_remap", generated(fault)), case)
    assert status == CHECK_FAILED, output
