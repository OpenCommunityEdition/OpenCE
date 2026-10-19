"""Current-transform carry, including starts, stops, reversals and network phase snaps."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run


def generated(fault=False):
    code = function(read("source/devices/device_machines.c"), "machine_move_elevator_riders")
    if fault:
        code = mutated(code, "node_position->z - previous_position->z",
                       "node_position->z - machine->machine.elevator_position.z")
    return (("under_test.inc", code),)


@pytest.mark.parametrize("case", ["up", "down", "start", "stop", "reverse", "snap", "other-rider", "door"])
def test_case(case):
    status, output = run(build("elevator_carry", generated()), case)
    assert status == 0, output


def test_negative_control():
    status, output = run(build("elevator_carry", generated(True)), "reverse")
    assert status == CHECK_FAILED, output


def test_carry_uses_new_collision_nodes():
    update = function(read("source/objects/objects.c"), "object_update")
    before = update.index("machine_get_elevator_position(object_index, &elevator_position)")
    motion = update.index("object_type_update(object_index)")
    collision = update.index("object_compute_node_matrices(object_index)")
    carry = update.index("machine_move_elevator_riders(object_index, &elevator_position)")
    assert before < motion < collision < carry
    assert "machine_move_elevator_riders(" not in function(read("source/devices/device_machines.c"), "machine_update")
