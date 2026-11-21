"""The real decision of main.c's main loop whether a frame is drawn: with Smooth Motion every frame, but only once
the map has ticked (before, the original's rule: a cinematic's start showed the sky from an unset camera for a
moment), the original's rule without it, the menus' map always, a multiplayer game from its third tick."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run  # noqa: E402

CASES = ["first tick", "original", "menu", "paused", "multiplayer"]
NEGATIVE_CONTROLS = {
    # every frame drawn with Smooth Motion, the map's first tick or not
    "before-first-tick": (("(main_globals.halt_time_scale && (render_frame || game_time_get()>0));",
                           "main_globals.halt_time_scale;"), "first tick"),
    # with Smooth Motion, only the original's frames
    "no-frames-between": (("(render_frame || game_time_get()>0)", "(render_frame)"), "first tick"),
}


def generated(fault=None):
    source = read("source/main/main.c").replace("\r\n", "\n")
    match = re.search(r"\t*render_frame = main_globals\.main_menu_scenario_loaded \|\|.*?"
                      r"render_frame &= !game_engine_running\(\) \|\| game_time_get\(\)>=3;", source, re.S)
    assert match, "the main loop's render decision is not in main.c"
    code = match.group(0) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("main_render_frame", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("main_render_frame", generated(fault)), case)
    assert status == CHECK_FAILED, output
