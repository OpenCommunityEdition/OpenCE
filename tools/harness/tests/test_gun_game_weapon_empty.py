"""The real gun_game_weapon_empty (port/linux/game/gun_game.c): a battery weapon whose magazine block holds no rounds
(Halo CE's plasma rifle and plasma pistol) is spent by its battery, not by its empty magazine; a weapon whose
magazine holds rounds is spent when none is loaded or carried. Taken for spent, Gun Game gave the plasma weapons anew
on every update and they never fired."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["battery-full", "battery-spent", "rounds", "no-magazines"]

RETURN = "return holds_rounds ? TRUE : weapon->weapon.age >= 1.0f;"
SKIP = ("if (magazine_definition->rounds_loaded_maximum <= 0 && magazine_definition->rounds_total_maximum <= 0)\n"
        "\t\t\tcontinue;")

# the fix taken apart (the old "return TRUE", the variable kept used for -Werror), and the case that must catch each
NEGATIVE_CONTROLS = {
    "the-old-return": ((RETURN, "return holds_rounds ? TRUE : TRUE;"), "battery-full"),
    "no-skip": ((SKIP, ";"), "battery-full"),
    "every-magazine-skipped": ((SKIP, "continue;"), "rounds"),
}


def generated(fault=None):
    code = function(read("port/linux/game/gun_game.c"), "gun_game_weapon_empty")
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("gun_game_weapon_empty", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("gun_game_weapon_empty", generated(fault)), case)
    assert status == CHECK_FAILED, \
        f"gun_game_weapon_empty with a fault ({control}) passed '{case}': the test cannot see it"
