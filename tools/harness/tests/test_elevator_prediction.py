"""The host's taking of a co-op client's player where the client says (network_distributed.c's
distributed_take_prediction) on a moving elevator: across only, so the host's copy rides its own elevator rather than
falling through its floor going up; off one, or on one at rest, as the client says."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

SOURCE = "port/linux/game/network_distributed.c"
CASES = ["riding-up", "elevator-at-rest", "on-foot"]
FUNCTIONS = ("distributed_riding_moving_elevator", "distributed_take_prediction")

# a fault in network_distributed.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "height-taken-riding": (("if (distributed_riding_moving_elevator(unit_index))\n\t\tposition.z",
                             "if (0)\n\t\tposition.z"), "distributed_take_prediction", "riding-up"),
    "elevator-at-rest-rides": (("return elevator && elevator->device.position_velocity != 0.0f;",
                                "return elevator != NULL;"), "distributed_riding_moving_elevator",
                               "elevator-at-rest"),
}


def generated(fault=None, faulty_function=None):
    source = read(SOURCE)
    tolerance = re.search(r"^#define HOST_ACCEPT_TOLERANCE\s+(\S+)", source, re.M)
    assert tolerance, f"HOST_ACCEPT_TOLERANCE not found in {SOURCE}"
    config = f"#define HOST_ACCEPT_TOLERANCE {tolerance.group(1)}\n"
    config += structure(source, "distributed_on_foot_bound") + "\n"
    text = ""
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("elevator_prediction", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("elevator_prediction", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, \
        f"network_distributed.c with a fault ({control}) passed '{case}': the test cannot see it"
