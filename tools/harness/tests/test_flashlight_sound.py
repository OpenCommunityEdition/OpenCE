"""The real flashlight_sound_toggle_effect (flashlight_sound.c): a player's unit whose definition names no flashlight
toggle effect (the multiplayer cyborg's) plays the campaign cyborg's, weapons\\assault rifle\\effects\\flashlight, when the
map has it, so that a multiplayer flashlight clicks as a campaign one does; any other unit plays its own, or none."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["own effect", "multiplayer player", "none"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # no sound for the multiplayer cyborg (before fix-mp-flashlight-sound)
    "no-fallback": (("return tag_loaded(EFFECT_DEFINITION_TAG, FLASHLIGHT_SOUND_EFFECT_TAG_NAME);", "return NONE;"),
                    "multiplayer player"),
    # an AI's unit too
    "every-unit": (("if (!unit || unit->unit.player_index == NONE)", "if (!unit)"), "none"),
    # the campaign's in place of a definition's own
    "own-replaced": (("if (effect_index != NONE)\n\t\treturn effect_index;\n", ""), "own effect"),
}


def generated(fault=None):
    source = read("port/linux/game/flashlight_sound.c").replace("\r\n", "\n")
    code = re.search(r"^#define FLASHLIGHT_SOUND_EFFECT_TAG_NAME .*$", source, re.M).group(0) + "\n"
    code += function(source, "flashlight_sound_toggle_effect") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_flashlight_plays_it():
    """The flashlight's toggle plays the effect flashlight_sound_toggle_effect gives for the unit."""
    units = read("source/units/units.c").replace("\r\n", "\n")
    assert "\t\t\teffect_new_from_object(\n" \
           "\t\t\t\tflashlight_sound_toggle_effect(unit_index, unit_definition->unit.integrated_light_toggle_effect.index),\n" \
           "\t\t\t\tunit_index, unit_index, NONE, 0.f, 0.f, NULL, NULL);\n" \
           "\t\t\tunit->unit.flags ^= FLAG(_unit_integrated_light_on_bit);" in units


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("flashlight_sound", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("flashlight_sound", generated(fault)), case)
    assert status == CHECK_FAILED, f"flashlight_sound_toggle_effect with a fault ({control}) passed '{case}'"
