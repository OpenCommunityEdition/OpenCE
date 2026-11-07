"""The real weapon_build_weapon_interface_state (weapons.c), the state the hud's meters read: while a weapon's first
trigger charges, its heat meter shows the charged shot's heat as far as it has charged (a plasma pistol's meter fills
with the charge, then is full charged), and the weapon's own heat is not changed."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["idle", "charging", "charged", "bottomless"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the charge not shown (before fix-charge-heat-meter)
    "not-shown": (("if (charged>0.0f)", "if (0)"), "charged"),
    # the whole charged shot's heat shown from the charge's first tick
    "all-at-once": (("state->heat+charged*TAG_BLOCK_GET_ELEMENT(", "state->heat+1.0f*TAG_BLOCK_GET_ELEMENT("), "charging"),
    # past full
    "past-full": (("state->heat = MIN(1.0f, ", "state->heat = ("), "charged"),
    # the shot's heat shown with a bottomless clip, which adds none
    "bottomless": ((" && !cheat.bottomless_clip", ""), "bottomless"),
}


def generated(fault=None):
    source = read("source/items/weapons.c")
    config = "\n".join([
        enum_with(read("source/items/weapons.h"), "_trigger_charging,"),
        enum_with(read("source/items/weapons.h"), "_magazine_idle = 0,"),
        structure(read("source/interface/hud_weapon.c"), "weapon_interface_magazine_state"),
        structure(read("source/interface/hud_weapon.c"), "weapon_interface_state"),
    ]) + "\n"
    code = "\n".join(function(source, name) for name in ("weapon_trigger_get_charged_fraction",
                                                         "weapon_build_weapon_interface_state")) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("charge_heat_meter", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("charge_heat_meter", generated(fault)), case)
    assert status == CHECK_FAILED, f"the meter with a fault ({control}) passed '{case}': the test cannot see it"
