"""The real cinematic_framing_camera_at_shot, _xbox_width and _vertical_field_of_view (cinematic_framing.c): a10's
close shot of the cryo bay's keyboard looks through the assistant from inside his chest, and the strips a window
wider than 4:3 adds at its sides showed his upper arm and shoulder from inside him; while the scripted camera looks
from that point, the view keeps the Xbox's width. Any other shot, map or window is drawn as it was."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = ["narrowed", "kept"]

SOURCE = "port/linux/game/cinematic_framing.c"

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # any camera point of the map
    "any-point": (("!csstrcmp(point->name, cinematic_framing_shots[shot_index].camera_point_name) &&\n\t\t\t\t", ""),
                  "kept"),
    # any map
    "any-map": (("\t\tif (csstrcmp(scenario_name, cinematic_framing_shots[shot_index].scenario_name))\n"
                 "\t\t{\n\t\t\tcontinue;\n\t\t}\n", ""), "kept"),
    # anywhere near the point
    "anywhere": (("dx * dx + dy * dy + dz * dz <= ", "0.f * dx <= "), "kept"),
    # narrower windows changed too
    "any-window": (("width * 3.f <= height * 4.f ||\n\t\t", ""), "kept"),
    # the wrong way round
    "inverted": (("(4.f * height) / (3.f * width)", "(3.f * width) / (4.f * height)"), "narrowed"),
}


def generated(fault=None):
    source = read(SOURCE).replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define CINEMATIC_FRAMING_\w+ .*$", source, re.M))
    shots = re.search(r"^static struct cinematic_framing_shot const cinematic_framing_shots\[\] =\n\{.*?^\};",
                      source, re.M | re.S).group(0)
    config = "\n".join([defines, structure(source, "cinematic_framing_shot") + ";", shots]) + "\n"
    code = "\n\n".join(function(source, name) for name in
                       ("cinematic_framing_camera_at_shot", "cinematic_framing_xbox_width",
                        "cinematic_framing_vertical_field_of_view")) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


def test_wired():
    """The window's vertical angle passes through it, and the scripted camera says where it looks from."""
    main = read("source/main/main.c").replace("\r\n", "\n")
    camera = function(main, "set_window_camera_values")
    assert "cinematic_framing_vertical_field_of_view(\n\t\t\t\t&window->rasterizer_camera.viewport_bounds," in camera
    scripting = read("source/camera/camera_scripting.c").replace("\r\n", "\n")
    position = function(scripting, "scripted_camera_point_position")
    assert "camera_script_globals.mode != _camera_script_mode_point" in position
    assert "camera_script_globals.relative_object_index != NONE" in position


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("cinematic_framing", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("cinematic_framing", generated(fault)), case)
    assert status == CHECK_FAILED, f"cinematic_framing with a fault ({control}) passed '{case}'"
