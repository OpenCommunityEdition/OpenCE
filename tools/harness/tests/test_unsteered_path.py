"""Upstream's path of an unsteered vehicle (network_objects.c): its replaced object and its teleport."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import CHECK_FAILED, build, function, mutated, read, run

OBJECTS = "port/linux/game/network_objects.c"
CASES = ["path-replaced-object-kept-still", "path-carries-its-own-object", "path-teleport-restarts"]
# a fault in the path, and the case that must fail a CHECK
NEGATIVE_CONTROLS = {
    "path-ignores-slot-identity": (
        "\t\t\tobjects_client_unsteered_path[absolute_index].object_index != iterator.index)",
        "\t\t\tFALSE)", "path-replaced-object-kept-still"),
    "path-never-carries": (
        "\t\tif (object->object.parent_object_index != NONE || !distributed_vehicle_unsteered(iterator.index))",
        "\t\tif (TRUE)", "path-carries-its-own-object"),
    "path-teleport-glided": (
        "if (!same || !(step.i * step.i + step.j * step.j + step.k * step.k <= limit * limit))",
        "if (!same || !(step.i * step.i + step.j * step.j + step.k * step.k <= 1.0e12f * limit * limit))", "path-teleport-restarts"),
}


def generated(fault=None):
    source = read(OBJECTS)
    types = "#define MAXIMUM_OBJECT_PERIOD_TICKS 4\n"
    for name in ("UNSTEERED_PATH_STEP", "UNSTEERED_PATH_MAXIMUM_TICKS", "UNSTEERED_PATH_LATE_TICKS"):
        types += re.search(r"^#define " + name + r" .*$", source, re.M).group(0) + "\n"
    types += re.search(r"^static struct\n\{\n\tlong object_index;\n\tlong time;\n\tlong host_time;.*?^\} objects_client_unsteered_path\[[^\]]+\];",
                       source, re.M | re.S).group(0) + "\n"
    code = "\n".join(function(source, name) for name in (
        "distributed_client_note_unsteered_path", "distributed_client_carry_unsteered_vehicles"))
    if fault:
        code = mutated(code, *fault)
    return (("types.inc", types), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("unsteered_path", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    before, after, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("unsteered_path", generated((before, after))), case)
    assert status == CHECK_FAILED, output
    assert "check failed:" in output, output
