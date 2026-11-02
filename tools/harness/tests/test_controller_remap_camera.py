"""Controller remapping alongside the director's keyboard/mouse debug camera."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read  # noqa: E402
from harness import run  # noqa: E402

CASES = ["stock", "custom", "keyboard", "middle", "blocked", "blocked_middle", "normal_camera"]
NEGATIVE_CONTROLS = {
    "mouse_priority": (("director->debug_controls && !middle &&", "director->debug_controls &&"), "middle"),
    "mouse_menu_guard": (("if (middle && !controls->active &&\n\t\t(!remapped || controller_remap_playing((short)player_index)) &&",
                          "if (middle && !controls->active &&"), "blocked_middle"),
}


def generated(fault=None):
    source = function(read("source/camera/director.c"), "director_update_controls")
    if fault:
        source = mutated(source, *fault)
    return (("under_test.inc", source),
            ("actions.inc", enum_with(read("port/linux/include/controller_remap.h"), "PAD_CAMERA_UP")))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("controller_remap_camera", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("controller_remap_camera", generated(fault)), case)
    assert status == CHECK_FAILED, output
