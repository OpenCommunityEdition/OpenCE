"""Client floor contact through delayed prediction echoes, and ordinary corrections after leaving it."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run

SOURCE = "port/linux/game/network_distributed.c"
CASES = ["100ms", "200ms", "100ms-up", "200ms-up", "jump", "walk-off", "other-elevator", "teleport", "unpredicted", "no-history", "on-foot"]


def generated(fault=False):
    source = read(SOURCE)
    correction = function(source, "distributed_correct_own_unit")
    if fault:
        correction = mutated(correction, "own->elevator_index != NONE &&", "FALSE &&")
    config = "#define LOCAL_CORRECTION_TOLERANCE " + re.search(
        r"^#define LOCAL_CORRECTION_TOLERANCE\s+(\S+)", source, re.M)[1] + "\n"
    config += re.search(r"static (struct distributed_own_position\s*\{.*?\n\})", source, re.S)[1] + ";\n"
    return (("config.inc", config), ("under_test.inc", function(source, "distributed_supported_elevator") + "\n" + correction))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("elevator_correction", generated()), case)
    assert status == 0, output


def test_negative_control():
    status, output = run(build("elevator_correction", generated(True)), "200ms")
    assert status == CHECK_FAILED, output
