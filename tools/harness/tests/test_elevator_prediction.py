"""The host's taking of a co-op client's player where the client says (network_distributed.c's
distributed_take_prediction) on a moving elevator or standing on its floor at rest: across only, so the host's copy
rides its own elevator; after leaving a stopped one, the client's height is taken normally."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

SOURCE = "port/linux/game/network_distributed.c"
CASES = ["riding-up", "elevator-at-rest", "on-foot", "jump-at-rest", "left-platform",
         "reversals-100ms", "reversals-200ms"]
FUNCTIONS = ("distributed_supported_elevator", "distributed_riding_elevator", "distributed_take_prediction")

# a fault in network_distributed.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "height-taken-riding": (("if (distributed_riding_elevator(unit_index))\n\t\tposition.z",
                             "if (0)\n\t\tposition.z"), "distributed_take_prediction", "riding-up"),
    "stopped-floor-not-protected": (("distributed_supported_elevator(unit_index) != NONE",
                                    "FALSE"), "distributed_riding_elevator",
                               "elevator-at-rest"),
    "airborne-is-supported": (("TEST_FLAG(biped->biped.flags, _biped_airborne_bit)", "FALSE"),
                              "distributed_supported_elevator", "jump-at-rest"),
}


def divergence(source):
    """network_distributed.c's distributed_divergence, an unnamed structure's static variable"""
    end = source.find("} distributed_divergence;")
    assert end >= 0, f"distributed_divergence not found in {SOURCE}"
    start = source.rindex("static struct", 0, end)
    return source[start:end] + "} distributed_divergence;"


def generated(fault=None, faulty_function=None):
    source = read(SOURCE)
    tolerance = re.search(r"^#define HOST_ACCEPT_TOLERANCE\s+(\S+)", source, re.M)
    assert tolerance, f"HOST_ACCEPT_TOLERANCE not found in {SOURCE}"
    config = f"#define HOST_ACCEPT_TOLERANCE {tolerance.group(1)}\n"
    config += structure(source, "distributed_on_foot_bound") + "\n"
    # (the tests' drift figures, which distributed_take_prediction notes:
    # the source's own definitions)
    apart = re.search(r"^#define DIVERGENCE_APART_DISTANCE\s+(\S+)", source, re.M)
    assert apart, f"DIVERGENCE_APART_DISTANCE not found in {SOURCE}"
    config += f"#define DIVERGENCE_APART_DISTANCE {apart.group(1)}\n"
    config += divergence(source) + "\n"
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
