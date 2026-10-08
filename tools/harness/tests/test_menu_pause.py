"""Real widget pause paths and clock/loop expiry over a fake menu scene."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run  # noqa: E402

CASES = ["main-menu", "main-menu-error", "campaign-nested", "campaign-error",
         "coop", "unrequested"]
CONTROLS = {
    "pauses-shell": ("policy", "!we_are_at_the_main_menu &&", "", "main-menu"),
    "pauses-coop": ("policy", "!network_coop_active()", "TRUE", "coop"),
    "error-bypasses-policy": ("error", "widget_pause_game_requested(pause_game_time)",
                              "pause_game_time", "main-menu-error"),
}


def block(source, marker):
    start = source.index(marker)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def generated(control=None):
    ui = read("source/interface/ui_widget.c")
    initializer = function(ui, "widget_instance_initialize")
    start = initializer.index("widget->pause_game_time =")
    assignment = initializer[start:initializer.index(";", start) + 1]
    # This also runs on the unfixed source, to demonstrate the original failure.
    policy = function(ui, "widget_pause_game_requested") if "static boolean widget_pause_game_requested(" in ui else ""
    parts = {
        "policy": policy,
        "init": assignment + block(initializer, "if (widget->pause_game_time == TRUE)"),
        "error": block(function(ui, "display_error"), "if (!widget->pause_game_time)"),
        "delete": block(function(ui, "ui_widget_delete"), "if (widget->pause_game_time == TRUE)"),
    }
    if control:
        part, before, after, _ = CONTROLS[control]
        parts[part] = mutated(parts[part], before, after)
    code = function(read("source/game/game_time.c"), "game_time_set_paused") + "\n" + parts["policy"]
    code += "\nstatic void open_widget(struct widget_instance *widget, struct definition *definition){\n" + parts["init"] + "\n}\n"
    code += "static void error_pause(struct widget_instance *widget, boolean pause_game_time){\n" + parts["error"] + "\n}\n"
    code += "static void close_widget(struct widget_instance *widget){\n" + parts["delete"] + "\n}\n"
    expiry = block(function(read("source/sound/sound_manager.c"), "process_looping_sounds"),
                   "if (looping_sound->flip_flop != sound_manager_globals.flip_flop)")
    code += "static void expire_loop(struct loop *looping_sound){\n" + expiry + "\n}\n"
    return (("config.inc", f"enum {{ _widget_pause_game_time_bit = {constant(ui, '_widget_pause_game_time_bit')} }};\n"),
            ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("menu_pause", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", CONTROLS)
def test_negative_control(control):
    status, output = run(build("menu_pause", generated(control)), CONTROLS[control][3])
    assert status == CHECK_FAILED, f"{control} was not caught: {output}"
