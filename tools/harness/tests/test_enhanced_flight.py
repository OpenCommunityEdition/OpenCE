"""Exercise the actual bounded aircraft planner against geometric obstacles."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import read, build, run

@pytest.mark.parametrize("case", ["clear", "wall", "sealed", "landing", "takeoff", "clearance", "retarget", "braking", "shape", "partial", "recovery", "mass_budget", "vertical", "forward_flight", "fighter_landing", "dynamic_hull", "terrain", "hold", "winged", "lookahead", "climb_progress", "landing_site", "low_approach"])
def test_flight(case):
    source = read("port/linux/game/enhanced_bots_flight.c")
    code = source[source.index("enum\n{"):]
    status, output = run(build("enhanced_flight", (("under_test.inc", code),)), case)
    assert status == 0, output
