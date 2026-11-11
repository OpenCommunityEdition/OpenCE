"""The direct camera follows the drawn turn, without undoing a dead player's frozen turn."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run

CASES = ["smooth", "dead", "players", "parented", "no-turn", "render-turn-zero-skipped"]
NEGATIVE_CONTROLS = {
    "wrong-sign": ("-(1.0f - render_interpolation_fraction())", "(1.0f - render_interpolation_fraction())", "smooth"),
    "dead-turn": ("if (!TEST_FLAG(object_get(unit_index)->object.damage_flags, _object_dead_bit))", "if (TRUE)", "dead"),
    "wrong-player": ("vehicle_riding_kept_dyaw(local_player_index)", "vehicle_riding_kept_dyaw(0)", "players"),
    "render-skip-always": ("if (kept_dyaw != 0.f)", "if (kept_dyaw == 0.f)", "smooth"),
    "render-never-skips": ("if (kept_dyaw != 0.f)", "if (TRUE)", "render-turn-zero-skipped"),
}


def generated(fault=None):
    camera = function(read("port/linux/game/render_interpolation.c"), "render_interpolation_direct_camera")
    if fault:
        camera = mutated(camera, *fault)
    turn = function(read("port/linux/game/vehicle_riding.c"), "vehicle_riding_turn_vector")
    counted = "#define vehicle_riding_turn_vector(...) (turn_calls++, vehicle_riding_turn_vector(__VA_ARGS__))\n"
    return (("under_test.inc", turn + "\n" + counted + camera),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("riding_camera", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    before, after, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("riding_camera", generated((before, after))), case)
    assert status == CHECK_FAILED, f"camera fault {control} passed {case}: {output}"
