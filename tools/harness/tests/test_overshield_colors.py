"""The real overshield_effect_green_target, overshield_effect_green_turn and overshield_effect_mix_color
(overshield_effect.c): the overshield's flare is green as it charges and while the shield is over 2 (the shield meter's
green, second overcharge), the powerup's red at 2 or less (the meter's red, first), turning in a quarter of a second."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["picked up", "meter layers", "turn", "colors"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the powerup's color only (before feature-overshield-color-layers)
    "always-red": (("return charging || shield_vitality > OVERSHIELD_EFFECT_GREEN_SHIELD ? 1.f : 0.f;",
                    "return 0.f;"), "picked up"),
    # charging not green
    "charging-not-green": (("return charging || shield_vitality", "return shield_vitality"), "picked up"),
    # green down to the first overcharge
    "green-to-1": (("#define OVERSHIELD_EFFECT_GREEN_SHIELD 2.f", "#define OVERSHIELD_EFFECT_GREEN_SHIELD 1.f"),
                   "meter layers"),
    # turned at once
    "at-once": (("real step = MAX(seconds, 0.f) / OVERSHIELD_EFFECT_COLOR_TURN;", "real step = 1.f;"), "turn"),
}

NAMES = ("overshield_effect_green_target", "overshield_effect_green_turn", "overshield_effect_mix_color")


def generated(fault=None):
    source = read("port/linux/game/overshield_effect.c").replace("\r\n", "\n")
    code = "".join(re.search(r"^#define " + name + r" .*$", source, re.M).group(0) + "\n"
                   for name in ("OVERSHIELD_EFFECT_GREEN_SHIELD", "OVERSHIELD_EFFECT_COLOR_TURN"))
    code += "".join(re.search(r"^static real_rgb_color const " + name + r" = .*$", source, re.M).group(0) + "\n"
                    for name in ("overshield_effect_rgb", "overshield_effect_green_rgb"))
    code += "".join(function(source, name) + "\n" for name in NAMES)
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_drawn_in_them():
    """The flare and its glow are drawn in the overshield's color as green as it is, turned each time it is looked at."""
    source = read("port/linux/game/overshield_effect.c").replace("\r\n", "\n")
    assert "\t\t\tovershield_effect_mix_color(effect->green, color);\n\t\t\treturn TRUE;" in source
    assert "overshield_effect_mix_color(effect ? effect->green : 0.f, color);" in source
    assert "effect->green = overshield_effect_green_turn(effect->green,\n" \
           "\t\tovershield_effect_green_target(charging, unit->object.shield_vitality), " \
           "now - effect->green_time);" in source
    assert "overshield_effect_glow_color(unit_index, color);" in read("port/linux/game/shield_glow.c")


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("overshield_colors", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("overshield_colors", generated(fault)), case)
    assert status == CHECK_FAILED, f"the overshield's colors with a fault ({control}) passed '{case}': the test cannot see it"
