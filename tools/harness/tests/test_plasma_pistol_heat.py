"""The real plasma_pistol_heat_meter_value (plasma_pistol_heat.c): with display.plasma_pistol_heat, the plasma pistol's
gauge shows the larger of its charge and the pistol's heat, so that firing it fast fills it; any other weapon's meters,
and every meter with the setting off, are as they were."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["firing fast", "charging", "other meters", "setting off"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the stock gauge whatever the setting (before feature-plasma-pistol-heat-gauge)
    "stock": (("return plasma_pistol_heat_gauge_value(value, weapon->weapon.heat);", "return value;"), "firing fast"),
    # the heat in place of the charge
    "heat-only": (("\tif (!(heat > value))\n\t\treturn value;\n", ""), "charging"),
    # every weapon's meters
    "every-weapon": (("!plasma_pistol_definition(weapon->definition_index)", "FALSE"), "other meters"),
    # whatever the setting
    "always-on": (("if (!plasma_pistol_heat_enabled() || object_index == NONE)", "if (object_index == NONE)"),
                  "setting off"),
}


def generated(fault=None):
    source = read("port/linux/game/plasma_pistol_heat.c").replace("\r\n", "\n")
    code = re.search(r"^#define PLASMA_PISTOL_TAG_NAME .*$", source, re.M).group(0) + "\n"
    code += "".join(function(source, name) + "\n" for name in (
        "plasma_pistol_heat_enabled", "plasma_pistol_definition", "plasma_pistol_heat_gauge_value",
        "plasma_pistol_heat_meter_value"))
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_meters_drawn_with_it():
    """A meter shader drawn on an object shows plasma_pistol_heat_meter_value's value, before the debug overrides."""
    source = read("source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c").replace("\r\n", "\n")
    hook = "meter_value = plasma_pistol_heat_meter_value(group->object_index, meter_value);"
    assert hook in source
    assert source.index(hook) < source.index("if (rasterizer_debug_options.debug_meter_shader_enabled)")


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("plasma_pistol_heat", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("plasma_pistol_heat", generated(fault)), case)
    assert status == CHECK_FAILED, f"plasma_pistol_heat_meter_value with a fault ({control}) passed '{case}'"
