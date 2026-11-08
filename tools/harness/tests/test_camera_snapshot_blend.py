"""The real camera_snapshot_blend (render_interpolation.c): a cinematic's camera that moves every frame is drawn from
its snapshots at the moment the objects are drawn at, a tick behind, so that a steady move is drawn steadily whatever
times the frames come at (it sped up and slowed down every tick: a cutscene's turning camera stuttered)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["fast frames", "slow frames", "on the tick"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # blended by the frame's fraction itself (before fix-cutscene-camera-stutter)
    "fraction": (("return (fraction - previous_taken) / span;", "return fraction;"), "fast frames"),
    # the snapshots' times taken as a tick apart
    "a-tick-apart": (("real span = 1.0f + latest_taken - previous_taken;", "real span = 1.0f;"), "slow frames"),
}


def generated(fault=None):
    code = function(read("port/linux/game/render_interpolation.c"), "camera_snapshot_blend") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_cinematic_point_cameras_use_it():
    """Only a scripted camera set to a point is blended so, and only from snapshots a tick apart."""
    source = read("port/linux/game/render_interpolation.c")
    assert "camera->taken_a_tick_apart &&\n\t\tdirector_get_perspective(local_player_index) == " \
           "_director_perspective_scripted &&\n\t\tscripted_camera_moving_to_point()" in source
    assert "return camera_script_globals.mode == _camera_script_mode_point;" in read("source/camera/camera_scripting.c")


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("camera_snapshot_blend", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("camera_snapshot_blend", generated(fault)), case)
    assert status == CHECK_FAILED, f"camera_snapshot_blend with a fault ({control}) passed '{case}': the test cannot see it"
