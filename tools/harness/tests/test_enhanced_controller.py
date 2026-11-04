"""Real goal arbitration across ticks, objective feedback and search fairness."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import read, function, build, run, mutated

def implementation():
    source = read("port/linux/game/enhanced_bots_controller.c")
    code = "\n".join(function(source, name) for name in (
        "bots_distance2d", "bots_distance3d", "bots_set_goal", "bots_goal_failed", "bots_choose_goal", "bots_search_turn"))
    return code + function(read("port/linux/game/enhanced_bots.c"), "enhanced_bots_capture_approach")

@pytest.mark.parametrize("case", ["seat", "objectives", "capture", "fairness"])
def test_controller(case):
    status, output = run(build("enhanced_controller", (("under_test.inc", implementation()),)), case)
    assert status == 0, output

def test_seat_commitment_regression_detected():
    code = mutated(implementation(), "else if (bots_vehicle_seeking(index))", "else if (FALSE)")
    status, output = run(build("enhanced_controller", (("under_test.inc", code),)), "seat")
    assert status == 1, output


def test_capture_does_not_cut_through_wall():
    code = mutated(implementation(),
        "if (collision_test_vector(collision_flags, &start, &delta, player->unit_index, &hit)) return FALSE;",
        "if (collision_test_vector(collision_flags, &start, &delta, player->unit_index, &hit) && FALSE) return FALSE;")
    status, output = run(build("enhanced_controller", (("under_test.inc", code),)), "capture")
    assert status == 1 and "base wall" in output, output
