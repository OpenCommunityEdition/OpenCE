"""The touchscreen's skip button during a cinematic: the real touch_cinematic.c draws it where the touch controls would
show (SKIP, and in network co-op VOTED once this machine has voted) in the letterbox's bar and says whether a tap
presses A, only one on it while it shows; touch_game.c says a cinematic can be skipped, or voted to skip by a network
co-op client, whose own cinematic never can be."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["solo-button", "no-button-any-tap", "not-over-a-menu", "stale-button", "coop-vote",
         "window-inside-the-screen", "skippable"]
BUTTON = ("touch_cinematic_label", "touch_cinematic_button_enabled", "touch_cinematic_draw", "touch_cinematic_tap")
GAME = ("touch_game_cinematic_skippable",)

# a fault in the code, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "button-takes-any-tap": (("if (!touch_cinematic.drawn ||", "if (TRUE ||"), "touch_cinematic_tap", "solo-button"),
    "no-slack-for-a-finger": (("button.x0 - slack - viewport->x0", "button.x0 - viewport->x0"), "touch_cinematic_draw",
                              "solo-button"),
    "button-under-the-bar": (("window_height * TOUCH_CINEMATIC_TOP / 1000", "window_height / 8"),
                             "touch_cinematic_draw", "solo-button"),
    "button-never-goes": (("system_milliseconds() - touch_cinematic.drawn_at > TOUCH_CINEMATIC_MILLISECONDS", "FALSE"),
                          "touch_cinematic_tap", "stale-button"),
    "button-over-a-menu": ((" || ui_widgets_active()", ""), "touch_cinematic_draw", "not-over-a-menu"),
    "button-before-the-vote": (("if (network_coop_active())", "if (FALSE)"), "touch_cinematic_label", "coop-vote"),
    "voted-not-shown": (('(*voted ? L"VOTED" : L"SKIP")', 'L"SKIP"'), "touch_cinematic_label", "coop-vote"),
    "button-at-the-screen-edge": (("button.x1 = (short)(window->x1 -", "button.x1 = (short)(viewport->x1 -"),
                                  "touch_cinematic_draw", "window-inside-the-screen"),
    # as before: a client's tap could not vote
    "client-cannot-vote": (("(cinematic_can_be_skipped() || network_coop_skip_offered())", "cinematic_can_be_skipped()"),
                           "touch_game_cinematic_skippable", "skippable"),
}


def generated(fault=None, faulty_function=None):
    button = read("port/linux/game/touch_cinematic.c")
    game = read("port/linux/game/touch_game.c")
    state = button[button.index("static struct\n{"):button.index("} touch_cinematic;") + len("} touch_cinematic;")]
    config = enum_with(button, "TOUCH_CINEMATIC_WIDTH") + "\n" + state + "\n"
    text = ""
    for source, names in ((button, BUTTON), (game, GAME)):
        for name in names:
            code = function(source, name)
            if fault and name == faulty_function:
                code = mutated(code, *fault)
            text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("touch_cinematic", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("touch_cinematic", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"


def test_a_tap_presses_a_only_where_the_button_says():
    # touch_input.c presses A for a tap during a skippable cinematic only if the button takes it, and declares the
    # button's function with its definition's exact types (the web build's link refuses a mismatch)
    touch = read("port/linux/src/touch_input.c")
    assert "int touch_cinematic_tap(float x, float y);" in touch
    gamepad = function(touch, "touch_input_gamepad")
    assert "else if (output.clicks && touch_game_cinematic_skippable() && touch_cinematic_tap(x, y))" in gamepad
    assert "int touch_cinematic_tap(\n\tfloat x,\n\tfloat y)" in read("port/linux/game/touch_cinematic.c")


def test_drawn_over_the_letterbox():
    # the screen's overlays draw the button after the letterbox's bars, which are drawn over the players' views
    # (their HUDs too), so that it is in the top bar and nothing covers it
    overlays = function(read("source/interface/interface.c"), "interface_draw_fullscreen_overlays")
    assert overlays.index("cinematic_render();") < overlays.index("touch_cinematic_draw();")


def test_vote_line_names_the_button():
    # the network co-op vote's line says to tap SKIP where the button shows, and the keys elsewhere
    line = function(read("port/linux/game/coop_spectate.c"), "coop_skip_vote_draw")
    assert line.index("touch_cinematic_button_enabled()") < line.index('L"TAP SKIP TO VOTE   %d OF %d"')
    assert 'L"PRESS SPACE OR A TO VOTE TO SKIP   %d OF %d"' in line
