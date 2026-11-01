"""Real link planner, measured shortcuts, traversal and lifecycle in a tiny graph."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import read, function, build, run, mutated


def implementation():
    source = read("port/linux/game/bots_navigation.inl")
    code = source[source.index("#define BOTS_NAV_LINKS"):source.index("static void bots_navigation_reset")]
    code += "\n".join(function(source, name) for name in (
        "bots_navigation_reset", "bots_navigation_reset_bot", "bots_navigation_climbing", "bots_navigation_pays",
        "bots_navigation_cost", "bots_navigation_remaining", "bots_navigation_drop", "bots_navigation_claimers",
        "bots_navigation_query", "bots_navigation_answer", "bots_navigation_update", "bots_navigation_follow",
        "bots_navigation_action"))
    code += function(read("port/linux/game/bots.c"), "bots_bot_unstick")
    return code


@pytest.mark.parametrize("case", ["measure", "reject", "reservation", "ladder", "climb_recovery", "tall_ladder", "timeout", "lifecycle"])
def test_navigation(case):
    status, output = run(build("bots_navigation", (("under_test.inc", implementation()),)), case)
    assert status == 0, output


def test_measured_cost_is_required():
    code = mutated(implementation(), "route->entrance_cost + beyond", "route->entrance_cost")
    status, output = run(build("bots_navigation", (("under_test.inc", code),)), "reject")
    assert status == 1 and "expensive exit" in output, output


def test_empty_cleanup_respects_engine_allocator():
    code = mutated(implementation(), "if (bots_navigation.vehicle_walkable) free(bots_navigation.vehicle_walkable - 1)",
                   "free(bots_navigation.vehicle_walkable ? bots_navigation.vehicle_walkable - 1 : NULL)")
    status, output = run(build("bots_navigation", (("under_test.inc", code),)), "lifecycle")
    assert status == 1 and "engine free received NULL" in output, output


def test_vertical_climb_is_not_stuck():
    code = mutated(implementation(), "route->climb_tick != 0", "route->climb_tick == 0")
    status, output = run(build("bots_navigation", (("under_test.inc", code),)), "climb_recovery")
    assert status == 1 and "climb mistaken for stuck movement" in output, output
