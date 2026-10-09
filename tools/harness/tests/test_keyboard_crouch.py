"""Real keyboard movement and player-control crouch gate, including the strict float boundary."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, inline, mutated, read, run  # noqa: E402

CASES = ["cardinals", "diagonals", "maximum-safe-input", "run-and-release",
         "controller-and-mixed-input", "cancelled-keys", "hold-across-frame-rates", "crouch-gate-exceptions"]

# Match portable desktop builds: SSE float arithmetic, with no fused multiply-add.
# Bare -m32 uses x87 excess precision and changes the strict boundary comparison.
FLAGS = ("-march=x86-64", "-ffp-contract=off")

NEGATIVE_CONTROLS = {
    "original-full-speed": (("length *= 0.97999996f;", "length *= 1.f;"), "cardinals"),
    "at-the-strict-cutoff": (("length *= 0.97999996f;", "length *= 0.98f;"), "cardinals"),
    "unnormalized-diagonals": (("x && y ? 0.70710678f : 1.f", "1.f"), "diagonals"),
    "slows-running-too": (("TEST_FLAG(held, HALO_KEYBOARD_CROUCH)", "TRUE"), "run-and-release"),
    "uses-controller-crouch": (("TEST_FLAG(held, HALO_KEYBOARD_CROUCH)",
                               "state->buttons[_game_control_crouch]"), "controller-and-mixed-input"),
}


def generated(fault=None):
    abstraction = read("source/input/input_abstraction.c")
    player = read("source/game/player_control.c")
    enums = "\n".join(enum_with(source, member) for source, member in (
        (abstraction, "_game_control_jump"), (player, "_button_jump"),
        (read("port/linux/include/halo_keyboard.h"), "HALO_KEYBOARD_MOVE_FORWARD"),
        (read("source/units/bipeds.h"), "_biped_airborne_bit"),
        (read("source/units/units.h"), "_unit_control_crouch_modifier_bit")))
    for source, name in (("source/input/input.h", "MAXIMUM_GAMEPADS"),
                         ("source/cseries/cseries.h", "TICKS_PER_SECOND"),
                         ("source/cseries/cseries.h", "UNSIGNED_CHAR_MAX")):
        enums += f"\n#define {name} {constant(read(source), name)}\n"
    start = abstraction.index("struct game_input_state\n{")
    enums += abstraction[start:abstraction.index("};", start) + 2] + "\n"
    start = abstraction.index("static signed char const keyboard_game_controls")
    text = abstraction[start:abstraction.index("static void keyboard_hold_ticks", start)]
    text += function(abstraction, "keyboard_hold_ticks") + "\n"
    update = function(abstraction, "keyboard_controls_update")
    text += (mutated(update, *fault) if fault else update) + "\n"
    text += inline(read("source/math/real_math.h"), "magnitude_squared2d") + "\n"

    # Compile the actual downstream condition and flag write, rather than a copied threshold.
    start = player.index("if (biped &&")
    end = player.index("{", start) + 1
    depth = 1
    while depth:
        depth += (player[end] == "{") - (player[end] == "}")
        end += 1
    gate = "static void apply_crouch(struct biped_datum *biped, struct test_input *input, " \
           "byte const *effective_buttons)\n{\n" + player[start:end] + "\n}\n"
    return (("config.inc", enums), ("under_test.inc", text + gate))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("keyboard_crouch", generated(), FLAGS), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("keyboard_crouch", generated(fault), FLAGS), case)
    assert status == CHECK_FAILED, f"keyboard input with fault {control} passed {case}: the test cannot see it"
