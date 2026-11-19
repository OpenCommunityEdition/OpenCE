"""The real loading_time.c (port/linux/game): a level's loading screen held for display.loading_seconds, with the
progress it shows no further than that time has gone; not held with the setting at 0 or in a network game, and at
most 30 seconds. game.c holds the loading screen once the level is ready, and progress_bar.c shows the progress."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["off", "held", "network", "longest", "tick wrap"]
NEGATIVE_CONTROLS = {
    # a network game held: the other machines would wait
    "network-held": (("game_connection()==_game_connection_local && ", ""), "network"),
    # no longest time
    "no-longest": (("if (seconds>LOADING_TIME_MAXIMUM_SECONDS)", "if (0)"), "longest"),
    # the load's progress shown as it comes, not paced
    "not-paced": (("if (gone<progress)", "if (0)"), "held"),
    # still paced after the loading screen ended
    "not-ended": (("\tloading_time.length= 0;\n\n\treturn;\n}", "\treturn;\n}"), "held"),
    # the end time compared, which wraps with the tick count
    "end-time": (("GetTickCount() - loading_time.start<loading_time.length",
                  "GetTickCount()<loading_time.start + loading_time.length"), "tick wrap"),
}


def generated(fault=None):
    code = read("port/linux/game/loading_time.c").replace("\r\n", "\n")
    code = re.sub(r"^#include [^\n]+\n", "", code, flags=re.M)
    if fault:
        code = mutated(code, *fault)
    config = enum_with(read("source/game/game.h").replace("\r\n", "\n"), "_game_connection_local") + "\n"
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("loading_time", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("loading_time", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_wired():
    """game.c reads the setting as the loading screen starts and holds the screen only after a level loaded; the
    loading screen shows the paced progress; the setting is 0 by default."""
    game = function(read("source/game/game.c").replace("\r\n", "\n"), "game_precache_new_map")
    assert "progress_bar_begin(global_scenario_index != NONE);" in game
    begin = game.index("loading_time_begin();")
    assert game.index("progress_bar_begin(") < begin < game.index("while (!map_status);")
    hold = game.index("while (map_status == _cached_map_file_success && loading_time_holding())")
    assert game.index("while (!map_status);") < hold < game.index("loading_time_end();") \
        < game.index("progress_bar_end();")
    display = function(read("source/interface/progress_bar.c").replace("\r\n", "\n"), "progress_bar_display")
    assert display.index("progress= loading_time_progress(progress);") < display.index("progress_bar_render(")
    settings = read("port/linux/src/port_config.c")
    assert re.search(r'\{ "display\.loading_seconds", _config_real, "0\.0", "HALO_LOADING_SECONDS"', settings)
