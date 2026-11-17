"""A PS5 DualSense's output bytes (port/linux/src/dualsense_rules.c): the trigger effects, the weapon profiles by the
weapon's state and the trigger strength, the lightbar and player lights, the USB and Bluetooth reports with their CRC,
and the link that writes only what changed (one report a frame, one every 50 ms on Bluetooth)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run  # noqa: E402

CASES = [
    "effects", "kinds", "held", "strength", "events", "left", "lights", "reports", "link-usb",
    "link-bluetooth", "bind",
]

# a fault in dualsense_rules.c and the case that must catch it
NEGATIVE_CONTROLS = {
    "strength-zero-not-off": (("\tif (strength <= 0)\n\t{\n\t\tdualsense_effect_off(effect);\n\t\treturn;\n\t}\n\tmemset(effect, 0, "
                               "DUALSENSE_EFFECT_SIZE);\n\teffect[0] = 0x21;\n\tzones(effect, clamp(position",
                               "\tmemset(effect, 0, DUALSENSE_EFFECT_SIZE);\n\teffect[0] = 0x21;\n\tzones(effect, clamp(position"),
                              "effects"),
    "weapon-end-before-start": (("end = clamp(end, start + 1, 8);", "end = clamp(end, start, 8);"), "effects"),
    "any-maker": (("return vendor == 0x054c && (product", "return (product"), "kinds"),
    "reload-not-free": (("state->readying || (state->reloading && !state->overheated)", "state->readying"), "held"),
    "empty-not-dry": (("\tif (state->empty)\n\t{", "\tif (0)\n\t{"), "held"),
    "readying-not-free": (("shape.mode == OFF || state->readying ||", "shape.mode == OFF ||"), "held"),
    "overheat-not-locked": (("\tif (state->overheated)\n\t{", "\tif (0)\n\t{"), "held"),
    "sniper-needs-no-zoom": (("zoom_levels >= 2 && rate_of_fire < 3.0f", "zoom_levels >= 1 && rate_of_fire < 3.0f"), "kinds"),
    "light-not-hair": (("return percent <= DUALSENSE_STRENGTH_LIGHT;", "return percent < DUALSENSE_STRENGTH_LIGHT;"),
                       "strength"),
    "strong-one-step": (("#define STRONG_STEPS 2", "#define STRONG_STEPS 1"), "strength"),
    "automatic-kicks": (("{ F(3, 2), V(3, 5, 0), NO, 0, 1, 0, 0, \"light hold, a pulse a round\", NO, 0 }",
                         "{ F(3, 2), V(3, 5, 0), F(0, 7), 35, 1, 0, 0, \"light hold, a pulse a round\", NO, 0 }"),
                        "events"),
    "grenade-hair-tick-missing": (("else if (grenades > 0)", "else if (0)"), "left"),
    "player-lights-slow": (("return (unsigned char)(player_lights[player_number - 1] | 0x20);",
                            "return player_lights[player_number - 1];"), "lights"),
    "crc-without-header": (("crc = dualsense_crc32(0, &hidp_header, 1);", "crc = 0;"), "reports"),
    "sequence-dropped": (("report[1] = (unsigned char)((sequence & 0x0f) << 4);", "report[1] = 0;"), "reports"),
    "gone-is-refused": (("\tcase ENODEV:\n", ""), "reports"),
    "changes-not-seen": (("memcmp(link->want + span->at, link->sent + span->at, (size_t)span->size))", "0)"), "link-usb"),
    "bluetooth-no-window": (("link->open_ms = now + (link->bluetooth ? DUALSENSE_BLUETOOTH_INTERVAL_MS : 0);",
                             "link->open_ms = now;"), "link-bluetooth"),
    "busy-retried-at-once": (("link->open_ms = now + (link->bluetooth ? DUALSENSE_BLUETOOTH_INTERVAL_MS : "
                              "DUALSENSE_BUSY_RETRY_MS);", "link->open_ms = now;"), "link-bluetooth"),
    "gone-not-forgotten": (("\tif (result == DUALSENSE_WRITE_GONE)\n\t\tdualsense_link_forget(link);", ""),
                           "link-bluetooth"),
    "all-enable-bits": (("\tif (fields & DUALSENSE_FIELD_LIGHTBAR)\n\t\tbody[DUALSENSE_BODY_ENABLE2] |= "
                         "DUALSENSE_ENABLE2_LIGHTBAR;", "\tbody[DUALSENSE_BODY_ENABLE2] |= DUALSENSE_ENABLE2_LIGHTBAR;"),
                        "link-usb"),
    "pad-keeps-old-port": (("if (other != port && bound[other] == id)", "if (0)"), "bind"),
}


def generated(fault=None):
    code = read("port/linux/src/dualsense_rules.c")
    if fault:
        code = mutated(code, *fault)
    return (("dualsense_rules.h", read("port/linux/src/dualsense_rules.h")), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("dualsense_rules", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("dualsense_rules", generated(fault)), case)
    assert status == CHECK_FAILED, f"dualsense_rules.c with a fault ({control}) passed '{case}': the test cannot see it"
