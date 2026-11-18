"""The real cinematic_letterbox.c (display.cinematic_framing): a shot of the scripted camera's with the letterbox shows
the Xbox's picture between the bars (its 4:3 width, 16:9 tall) across the window, and the bars are only as high as keep
it 16:9: none at 16:9. Off, the player's own view under the bars and a shot without them keep the stock view and
bars."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["xbox picture", "bars going", "stock view", "bars"]

SOURCE = "port/linux/game/cinematic_letterbox.c"

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the player's own view under the bars narrowed too
    "player-view": (("!director_camera_scripted || !*director_camera_scripted ||\n\t\t", ""), "stock view"),
    # the setting's value ignored
    "always-on": (("!cinematic_letterbox_enabled() || !director_camera_scripted", "!director_camera_scripted"),
                  "stock view"),
    # a window wider than 16:9 cropped to the Xbox's width too
    "wide-cropped": (("MIN(width / height, CINEMATIC_LETTERBOX_XBOX_PICTURE_ASPECT)", "(width / height)"),
                     "xbox picture"),
    # the view narrowed as the bars come, not with the cut
    "with-the-bars": (("\tif (cinematic_globals->show_letterbox)\n\t{\n\t\treturn 1.f;\n\t}\n", ""),
                      "xbox picture"),
    # the view snapped back while the shot stays as the bars go
    "snapped-back": (("return PIN(cinematic_globals->letterbox_amount, 0.f, 1.f);", "return 0.f;"), "bars going"),
    # the wrong way round
    "inverted": (("CINEMATIC_LETTERBOX_XBOX_WINDOW_ASPECT / MIN(",
                  "1.f / CINEMATIC_LETTERBOX_XBOX_WINDOW_ASPECT * MIN("), "xbox picture"),
    # the stock bars at any shape
    "bars-stock": (("0.5f * (1.f - (width / height) / CINEMATIC_LETTERBOX_XBOX_PICTURE_ASPECT)",
                    "CINEMATIC_LETTERBOX_STOCK_BAR_HEIGHT"), "bars"),
    # the setting read once
    "read-once": (("unsigned long changes = config_changes();", "unsigned long changes = 0;"), "bars"),
}


def generated(fault=None):
    source = read(SOURCE).replace("\r\n", "\n")
    config = "\n".join(re.findall(r"^#define CINEMATIC_LETTERBOX_\w+ .*$", source, re.M)) + "\n"
    code = "\n\n".join(function(source, name) for name in
                       ("cinematic_letterbox_enabled", "cinematic_letterbox_xbox_picture_amount",
                        "cinematic_letterbox_xbox_picture", "cinematic_letterbox_vertical_field_of_view",
                        "cinematic_letterbox_bar_height")) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


def test_wired():
    """The window's vertical angle passes through it, the letterbox's bars are as high as it says, and the setting is
    off by default."""
    camera = function(read("source/main/main.c").replace("\r\n", "\n"), "set_window_camera_values")
    assert "cinematic_letterbox_vertical_field_of_view(\n\t\t\t\t&window->rasterizer_camera.viewport_bounds," in camera
    cinematics = function(read("source/cutscene/cinematics.c").replace("\r\n", "\n"), "cinematic_render")
    assert "0.125f" not in cinematics
    assert re.search(r"bar_height = [\w>-]+ \*\n\t+"
                     r"cinematic_letterbox_bar_height\(&render\.camera\.viewport_bounds\);", cinematics)
    assert "cinematic_letterbox_bar_height(&render.camera.viewport_bounds) > 0.0f)" in cinematics
    assert '{ "display.cinematic_framing", _config_boolean, "false",' in read("port/linux/src/port_config.c")


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("cinematic_letterbox", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("cinematic_letterbox", generated(fault)), case)
    assert status == CHECK_FAILED, f"cinematic_letterbox with a fault ({control}) passed '{case}'"
