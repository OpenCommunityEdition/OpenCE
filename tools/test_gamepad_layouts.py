"""The game pad layout presets (port/linux/game/gamepad_layouts.h): each gives every control of the game a button of
its own, leaves Start and Back alone, and the names are the ones config.toml, the menus and the launcher write."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HEADER = (ROOT / "port/linux/game/gamepad_layouts.h").read_text()
# input.h's order: A, B, X, Y, black, white, left trigger, right trigger, d-pad up, down, left, right, start, back,
# left stick, right stick
NUMBER_OF_BUTTONS = 16
START, BACK = 12, 13
# input_abstraction.c's controls: jump, switch grenades, action, switch weapons, melee, flashlight, grenade, fire,
# start, back, crouch, zoom
CONTROLS = 12


def layouts():
    found = {}
    for name, buttons in re.findall(r'\{\s*"(\w+)",\s*\{([^}]*)\}', HEADER):
        found[name] = [int(value) for value in buttons.split(",")]
    return found


def test_the_names_are_the_ones_the_settings_use():
    assert list(layouts()) == ["profile", "halo2", "halo3", "halo4", "halo5", "infinite"]
    menu = (ROOT / "tools/port_settings.py").read_text()
    launcher = (ROOT / "port/mac/app/launcher_dialog.c").read_text()
    for name in list(layouts()):
        assert f'"{name}"' in menu and f'"{name}"' in launcher, name


def test_each_layout_gives_every_control_its_own_button():
    for name, buttons in layouts().items():
        if name == "profile":
            continue
        assert len(buttons) == CONTROLS, name
        # Start and Back stay where the game has them
        assert buttons[8] == START and buttons[9] == BACK, name
        controls = [button for index, button in enumerate(buttons) if index not in (8, 9)]
        assert all(0 <= button < NUMBER_OF_BUTTONS and button not in (START, BACK) for button in controls), name
        assert len(set(controls)) == len(controls), f"{name} puts two controls on one button"


def test_halo2_is_the_xboxs_own_layout():
    # A jump, black switch grenades, X action, Y switch weapons, B melee, white flashlight, the triggers, the sticks
    assert layouts()["halo2"] == [0, 4, 2, 3, 1, 5, 6, 7, 12, 13, 14, 15]
