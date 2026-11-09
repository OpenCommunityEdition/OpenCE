"""The real atmospheric_fog_settle_fraction and atmospheric_fog_settle_state (atmospheric_fog_settle.c): a local
player's atmospheric fog, which moves toward the fog where the camera is only as the camera moves (scenario.c), also
settles there over time, so that a camera standing still has it a second or two after it arrives, at any frame rate
(in b40, a player who stopped on the bridges kept the tunnels' dark fog until they walked on)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["standing still", "on the bridge", "no time"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # only as the camera moves (before fix-atmospheric-fog-settle)
    "no-settling": (("return seconds / (ATMOSPHERIC_FOG_SETTLE_TIME + seconds);", "return 0.f;"), "standing still"),
    # at once
    "at-once": (("return seconds / (ATMOSPHERIC_FOG_SETTLE_TIME + seconds);", "return 1.f;"), "standing still"),
    # a step a frame, whatever the frame's time
    "per-frame": (("return seconds / (ATMOSPHERIC_FOG_SETTLE_TIME + seconds);", "return 0.03f;"), "standing still"),
    # a time gone backward taken as time
    "backward": (("if (!(seconds > 0.f))\n\t\treturn 0.f;\n", ""), "no time"),
}


def generated(fault=None):
    source = read("port/linux/game/atmospheric_fog_settle.c").replace("\r\n", "\n")
    code = re.search(r"^#define ATMOSPHERIC_FOG_SETTLE_TIME .*$", source, re.M).group(0) + "\n"
    code += "".join(function(source, name) + "\n" for name in (
        "atmospheric_fog_settle_fraction", "atmospheric_fog_settle_value", "atmospheric_fog_settle_state"))
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_fog_settled_with_it():
    """scenario_get_atmospheric_fog settles a local player's fog after moving it, and takes the time it settles from when
    it sets it at once; the fields the test's structures name are the game's."""
    scenario = read("source/scenario/scenario.c").replace("\r\n", "\n")
    blend = function(scenario, "scenario_get_atmospheric_fog")
    assert blend.count("atmospheric_fog_settle(local_player_index, fog_state, fog, indoor_fog_scale);") == 2
    assert blend.index("interpolate_scalar(&fog_state->indoor_fog_scale, indoor_fog_scale, distance);") < \
        blend.index("atmospheric_fog_settle(local_player_index, fog_state, fog, indoor_fog_scale);")
    header = read("source/scenario/scenario.h")
    for field in ("atmospheric_start_distance", "atmospheric_opaque_distance", "atmospheric_maximum_density",
                  "real_rgb_color atmospheric_color", "indoor_fog_scale"):
        assert field in header
    sky = read("source/scenario/sky_definitions.h")
    for field in ("real_rgb_color color", "maximum_density", "start_distance", "opaque_distance"):
        assert field in sky


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("atmospheric_fog_settle", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("atmospheric_fog_settle", generated(fault)), case)
    assert status == CHECK_FAILED, f"the fog's settling with a fault ({control}) passed '{case}': the test cannot see it"
