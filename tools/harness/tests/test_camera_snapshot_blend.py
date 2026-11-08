"""The real camera_snapshot_blend, camera_snapshot_carried and observer_cut (render_interpolation.c): a cinematic's
camera that moves every frame is drawn from its snapshots at the moment the objects are drawn at, a tick behind, so that
a steady move is drawn steadily whatever times the frames come at (it sped up and slowed down every tick: a cutscene's
turning camera stuttered), and is cut by a move away from where its velocity carries it, not by a long one (d40's
fly-over, faster than CAMERA_CUT_DISTANCE a tick, was taken for a cut every few frames and jumped back and forth)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run  # noqa: E402

CASES = ["fast frames", "slow frames", "on the tick", "fast flight", "jump in flight", "jump at rest", "not a number"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # blended by the frame's fraction itself (before fix-cutscene-camera-stutter)
    "fraction": (("return (fraction - previous_taken) / span;", "return fraction;"), "fast frames"),
    # the snapshots' times taken as a tick apart
    "a-tick-apart": (("real span = 1.0f + latest_taken - previous_taken;", "real span = 1.0f;"), "slow frames"),
    # cut by a long move, as a player's camera is (before: CAMERA_CUT_DISTANCE alone)
    "not-carried": (("carried->position.x += previous->velocity.i * seconds;\n"
                     "\tcarried->position.y += previous->velocity.j * seconds;\n"
                     "\tcarried->position.z += previous->velocity.k * seconds;\n", ""), "fast flight"),
    # carried for a tick, whenever the snapshots were taken
    "carried-a-tick": (("real seconds = (1.0f + latest_taken - previous_taken) / (real)TICKS_PER_SECOND;",
                        "real seconds = 1.0f / (real)TICKS_PER_SECOND;"), "fast flight"),
}


def generated(fault=None):
    source = read("port/linux/game/render_interpolation.c").replace("\r\n", "\n")
    defines = "".join(re.search(r"^#define " + name + r" .*$", source, re.M).group(0) + "\n"
                      for name in ("CAMERA_CUT_DISTANCE", "CAMERA_CUT_COSINE"))
    code = defines + "#define TICKS_PER_SECOND %d\n" % constant(read("source/cseries/cseries.h"), "TICKS_PER_SECOND")
    code += "".join(function(source, name) + "\n" for name in
                    ("distance_squared", "observer_cut", "camera_snapshot_carried", "camera_snapshot_blend"))
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_cinematic_point_cameras_use_it():
    """Only a scripted camera set to a point is blended and cut so, and only from snapshots a tick apart."""
    source = read("port/linux/game/render_interpolation.c").replace("\r\n", "\n")
    assert "moving_to_point = camera->taken_a_tick_apart &&\n\t\tdirector_get_perspective(local_player_index) == " \
           "_director_perspective_scripted &&\n\t\tscripted_camera_moving_to_point();" in source
    assert "observer_cut(moving_to_point ? &carried : &camera->previous, &camera->latest)" in source
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
