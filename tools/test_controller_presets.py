"""Asset-free controller profile and menu preview checks against the engine's code."""

import os
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

from harness import enum_with, function, read, structure
from port_settings import PE, settings_files

ROOT = Path(__file__).resolve().parents[1]


class ControllerPresets(unittest.TestCase):
    def test_profile_controls_and_preview(self):
        profile_header = read("source/saved games/player_profile.h")
        inputs = read("source/input/input.h")
        menu = read("port/linux/game/menu_functions.c")
        constants = "\n".join(line for line in menu.splitlines() if line.startswith("#define PROFILE_"))
        declarations = "\n".join([
            enum_with(profile_header, "_button_preset_standard"),
            enum_with(profile_header, "_joystick_preset_standard"),
            enum_with(inputs, "_gamepad_analog_button_a"),
            structure(profile_header, "player_profile_controller_settings"),
            structure(read("source/input/input_abstraction.h"), "game_input_preferences"),
        ])
        code = """
typedef unsigned char byte;
typedef unsigned char boolean;
typedef float real;
#define NONE (-1)
#define MAXIMUM_NUMBER_OF_LOCAL_PLAYERS 4
#define NUMBER_OF_LOOK_SENSITIVITY_SETTINGS 10
#define match_assert(file, line, condition) ((void)0)
""" + declarations + """
struct player_profile { struct player_profile_controller_settings controller_settings; };
static struct {
    struct { struct player_profile profile; } local_players[4];
    short single_player_controller[4];
} player_ui_globals;
static struct game_input_preferences captured;
static short captured_controller;
static void input_abstraction_update_local_player_preferences(short controller,
    struct game_input_preferences const *preferences)
{
    captured = *preferences;
    captured_controller = controller;
}
struct widget_instance { struct { short current_frame_index; } animation; };
static struct player_profile *edited;
static struct player_profile *player_ui_get_edit_player_profile(void) { return edited; }
""" + constants + "\n" + function(read("source/interface/player_ui.c"),
                                     "set_local_player_controls_from_player_profile")
        code += "\n" + function(menu, "profile_gamepad_layout") + """
int main(void)
{
    /* Physical Xbox button indices, in game-action order. Old presets must
       retain their saved ordinals; later-game layouts follow them. */
    static byte const expected[10][12] = {
        {0,4,2,3,1,5,6,7,12,13,14,15},
        {0,4,2,3,1,5,7,6,12,13,14,15},
        {6,4,2,3,1,5,0,7,12,13,14,15},
        {0,4,2,3,6,5,1,7,12,13,14,15},
        {0,4,2,3,15,5,6,7,12,13,14,1},
        {5,0,1,3,4,2,6,7,12,13,14,15},
        {0,5,4,3,1,2,6,7,12,13,14,15},
        {0,2,1,3,4,5,6,7,12,13,14,15},
        {0,5,2,3,15,14,4,7,12,13,1,6},
        {5,0,1,3,15,2,4,7,12,13,14,6}
    };
    struct player_profile_controller_settings *controls;
    struct widget_instance picture;
    int preset, player, action;
    if (NUMBER_OF_BUTTON_PRESETS != 10 || _button_preset_bumper_jumper != 5 ||
        _button_preset_recon != 6 || _button_preset_walkie_talkie != 7 ||
        _button_preset_fishstick != 8 || _button_preset_helljumper != 9) return 1;
    for (player = 0; player < 4; player++) {
        controls = &player_ui_globals.local_players[player].profile.controller_settings;
        controls->look_sensitivity = 3;
        controls->joystick_preset = _joystick_preset_south_paw;
        controls->invert_look = 1;
        controls->flight_stick_aircraft_controls = 1;
        player_ui_globals.single_player_controller[player] = NONE;
        for (preset = 0; preset < 10; preset++) {
            controls->button_preset = preset;
            set_local_player_controls_from_player_profile(player);
            if (captured_controller != player) return 2;
            for (action = 0; action < 12; action++)
                if (captured.game_control_to_xbox_buttons[action] != expected[preset][action]) return 3;
            /* Every action must remain reachable on a distinct physical button. */
            for (action = 0; action < 12; action++) {
                int other;
                for (other = action + 1; other < 12; other++)
                    if (captured.game_control_to_xbox_buttons[action] ==
                        captured.game_control_to_xbox_buttons[other]) return 9;
            }
            if (captured.pitch_rate != 60 || captured.yaw_rate != 120 ||
                captured.joystick_controls != _joystick_preset_south_paw ||
                !captured.invert_look || !captured.invert_look_aircraft_control) return 4;
        }
        player_ui_globals.single_player_controller[player] = (player + 1) % 4;
        set_local_player_controls_from_player_profile(player);
        if (captured_controller != (player + 1) % 4) return 5;
    }
    edited = &player_ui_globals.local_players[0].profile;
    picture.animation.current_frame_index = PROFILE_GAMEPAD_FRAME;
    for (preset = 0; preset < 10; preset++) {
        edited->controller_settings.button_preset = preset;
        profile_gamepad_layout(&picture);
        if (picture.animation.current_frame_index !=
            (preset >= 5 ? PROFILE_GAMEPAD_FRAME : PROFILE_FIRST_LAYOUT_FRAME + preset)) return 6;
    }
    edited->controller_settings.button_preset = 0;
    profile_gamepad_layout(&picture);
    if (picture.animation.current_frame_index != PROFILE_FIRST_LAYOUT_FRAME) return 7;
    picture.animation.current_frame_index = 1;
    profile_gamepad_layout(&picture);
    if (picture.animation.current_frame_index != 1) return 8;
    return 0;
}
"""
        with tempfile.TemporaryDirectory(prefix="halo-controller-") as directory:
            source = Path(directory) / "controls.c"
            executable = Path(directory) / ("controls.exe" if os.name == "nt" else "controls")
            source.write_text(code, encoding="utf-8")
            command = [os.environ.get("CC", "clang"), "-std=gnu99", "-fuse-ld=lld"]
            if os.name != "nt":
                command += ["--no-default-config", "-m32"]
                sysroot = ROOT / "build/linux/third_party/sysroot"
                if sysroot.is_dir():
                    command.append(f"--sysroot={sysroot}")
            result = subprocess.run(command + [str(source), "-o", str(executable)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run([str(executable)], timeout=30)
            self.assertEqual(result.returncode, 0)

    def test_generated_menu_matches_generator(self):
        filename = f"{PE}/gamepad_setup".replace("/", ".") + ".xml"
        generated = "\n".join(settings_files()[filename])
        self.assertEqual((ROOT / "port/assets/menus/ce" / filename).read_text(), generated)
        root = ET.fromstring(generated)
        spinner = next(widget for widget in root.iter("widget")
                       if widget.get("setting") == "profile.button_preset")
        choices = dict(zip(spinner.get("strings").split("|"), spinner.get("values").split("|")))
        for name, value in [("BOXER", "3"), ("GREEN THUMB", "4"), ("BUMPER JUMPER", "5"),
                            ("RECON", "6"), ("WALKIE TALKIE", "7"), ("FISHSTICK", "8"), ("HELLJUMPER", "9")]:
            self.assertEqual(choices[name], value)
        self.assertIn('event="a" run="port settings save"', generated)


if __name__ == "__main__":
    unittest.main()
