"""The real render_interpolation_blended_camera (render_interpolation.c): the director's perspective changing after a
tick's camera snapshot was taken (b40's opening gives the player the view back one frame into a tick) is a cut, not a
view turned to the player's aim from where the cinematic's camera was."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = ["blended", "handover"]

NEGATIVE_CONTROLS = {
    # the perspective's change not looked at
    "no-handover": (("else if (camera->perspective != director_get_perspective(local_player_index))",
                     "else if (0)"), "handover"),
    # taken again, but still blended from before
    "kept-previous": (("\t\tcamera->has_previous = FALSE;\n\t\tcamera->latest = *observer;",
                       "\t\tcamera->latest = *observer;"), "handover"),
}

HELPERS = ["lerp", "point_lerp", "vector_length", "vector_nlerp", "distance_squared", "correction_significant",
           "correction_drawn", "observer_blend", "observer_cut", "camera_snapshot_carried", "camera_snapshot_blend",
           "render_interpolation_blended_camera"]


def generated(fault=None):
    source = read("port/linux/game/render_interpolation.c").replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define (?:CAMERA_CUT_\w+|CORRECTION_\w+) .*$", source, re.M))
    code = "\n\n".join([defines, structure(source, "interpolated_camera"),
                        "static struct interpolated_camera interpolated_cameras[MAXIMUM_LOCAL_PLAYERS];"] +
                       [function(source, name) for name in HELPERS]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("camera_handover", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("camera_handover", generated(fault)), case)
    assert status == CHECK_FAILED, f"the camera with a fault ({control}) passed '{case}'"
