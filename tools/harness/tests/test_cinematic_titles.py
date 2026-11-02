"""Exercise the real cinematic_render over a fake scenario.

Titles draw in the game, not over menus; their time advances under menus.
Widescreen placement follows alignment, and MCC non-wrapping titles avoid clipping.
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run, structure  # noqa: E402

CASES = [
    "title-screen-anchors", "default-title-anchor", "mcc-title-screen-anchor",
    "mcc-title-bounds", "mcc-title-anchors", "drawn-in-the-game",
    "not-over-a-menu", "time-goes-on-under-a-menu",
]

# a fault in cinematic_render and the case that must catch it
NEGATIVE_CONTROLS = {
    "right-title-uses-box-center": (("case 1: anchor = title_bounds->x1; break;",
                                     "case 1: anchor = (short)((title_bounds->x0 + title_bounds->x1) / 2); break;"),
                                    "default-title-anchor"),
    "mcc-title-clipped": (("mcc_hud_title_bounds(&wide_bounds, title->justification, title->text_flags);",
                           "(void)0;"), "mcc-title-bounds"),
    "drawn-over-a-menu": (("if (!ui_widgets_active())", "if (TRUE)"), "not-over-a-menu"),
    "time-stops-under-a-menu": (("active_title->time += game_time_get_paused()",
                                 "active_title->time += ui_widgets_active() || game_time_get_paused()"),
                                "time-goes-on-under-a-menu"),
}


def generated(fault=None):
    cinematics = read("source/cutscene/cinematics.c")
    header = read("source/cutscene/cinematics.h")
    config = "enum { MAXIMUM_QUEUED_CINEMATIC_TITLES = %d };\n" % constant(cinematics,
                                                                          "MAXIMUM_QUEUED_CINEMATIC_TITLES")
    for source, name in ((cinematics, "scenario_cutscene_title"), (header, "cinematic_title"),
                         (header, "cinematic_global_data")):
        config += structure(source, name) + "\n"
    code = function(cinematics, "cinematic_render")
    if fault:
        code = mutated(code, *fault)
    helper = function(read("port/linux/game/mcc_hud_draw.c"), "mcc_hud_title_bounds")
    return (("config.inc", config), ("under_test.inc", helper + "\n" + code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("cinematic_titles", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("cinematic_titles", generated(fault)), case)
    assert status == CHECK_FAILED, f"cinematics.c with a fault ({control}) passed '{case}': the test cannot see it"
