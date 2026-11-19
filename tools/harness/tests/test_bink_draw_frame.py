"""The real bink_draw_frame (source/bink/bink_playback.c), without its debug_bink statistics: a movie at its own
size in the middle of the screen, as the Xbox; with the credits' flag (display.widescreen_credits) their 16:9
picture as large as the screen holds it, as before on a 4:3 screen. game_end_credits_start passes the flag."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["4:3", "16:9", "21:9"]
NEGATIVE_CONTROLS = {
    # the credits drawn as any movie
    "no-flag": (("\telse if (TEST_FLAG(bink_globals.flags, _bink_playback_widescreen_picture_bit))",
                 "\telse if (0)"), "16:9"),
    # the screen's width filled whatever its height
    "width-only": (("\t\treal scale= MIN((real)screen_width/(real)bink_globals.width,\n"
                    "\t\t\t(real)screen_height*16.0f/(9.0f*(real)bink_globals.width));",
                    "\t\treal scale= (real)screen_width/(real)bink_globals.width;"), "21:9"),
}


def generated(fault=None):
    source = read("source/bink/bink_playback.c").replace("\r\n", "\n")
    code = function(source, "bink_draw_frame")
    # (the statistics debug_bink draws need the text and Bink's own)
    code = code[:code.index("\tif (debug_bink)")] + "\treturn;\n}\n"
    if fault:
        code = mutated(code, *fault)
    config = enum_with(read("source/bink/bink_playback.h").replace("\r\n", "\n"),
                       "_bink_playback_widescreen_picture_bit") + "\n"
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("bink_draw_frame", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("bink_draw_frame", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_credits_pass_the_flag():
    credits = function(read("source/interface/attract_mode.c").replace("\r\n", "\n"), "game_end_credits_start")
    assert ('(config_boolean("display.widescreen_credits") ? FLAG(_bink_playback_widescreen_picture_bit) : 0)'
            in credits)
