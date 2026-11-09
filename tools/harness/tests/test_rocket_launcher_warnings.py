"""The real rocket_launcher_warning_placement (hud_weapon.c): the rocket launcher's warnings ("Reload", the flash of its
last rocket, its click when empty), which it takes from ui\\hud\\master rounds 30 HUD units below its reticle's middle,
were drawn over its ring; they are drawn 7 units lower for it, and every other weapon's and crosshair are where they
were."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["rocket launcher", "other weapons", "other crosshairs"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # not moved (before fix-rocket-reload-text)
    "not-moved": (("placement->offset.y = (short)(placement->offset.y + 7);", ""), "rocket launcher"),
    # "Reload" left out
    "reload-left-out": (("state_index == _crosshair_state_flash_ammo_none_for_reload) &&",
                         "FALSE) &&"), "rocket launcher"),
    # every weapon's moved
    "every-weapon": (('!csstrcmp(tag_get_name(hud_index), "weapons\\\\rocket launcher\\\\rocket_launcher")',
                      "TRUE"), "other weapons"),
    # warnings above the middle moved too
    "above-too": (("placement->offset.y > 0 &&", ""), "other crosshairs"),
}


def generated(fault=None):
    source = read("source/interface/hud_weapon.c").replace("\r\n", "\n")
    code = enum_with(source, "_crosshair_state_flash_ammo_none_for_reload") + "\n"
    code += structure(read("source/interface/hud_definitions.h").replace("\r\n", "\n"), "hud_placement_definition") + "\n"
    code += function(source, "rocket_launcher_warning_placement") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_drawn_with_it():
    """The warnings drawn about the reticle are placed by it, after Custom Edition's placement."""
    source = read("source/interface/hud_weapon.c").replace("\r\n", "\n")
    assert "\t\t\t\t\t\t\t\t\t\t\t\t&placement);\n" \
           "\t\t\t\t\t\t\t\t\t\t\t/* port: clear of the rocket launcher's reticle */\n" \
           "\t\t\t\t\t\t\t\t\t\t\trocket_launcher_warning_placement(definition_indices[0], state_index, &placement);\n" \
           "\t\t\t\t\t\t\t\t\t\t\thud_draw_bitmap(" in source


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("rocket_launcher_warnings", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("rocket_launcher_warnings", generated(fault)), case)
    assert status == CHECK_FAILED, f"rocket_launcher_warning_placement with a fault ({control}) passed '{case}'"
