"""Idle seated actors keep publishing the host's seat without fresh control."""
from pathlib import Path
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure

SOURCE = "port/linux/game/network_actors.c"
CASES = ["actor-seat-idle-published", "actor-seat-idle-riding-off"]


def generated(control=None):
    source = read(SOURCE)
    if control:
        before, after, _ = NEGATIVE_CONTROLS[control]
        source = mutated(source, before, after)
    limits = read("port/linux/include/halo_port_limits.h")
    capacity = read("port/linux/include/halo_port_capacity.h")
    types = f"#define HALO_PORT_MAXIMUM_NETWORK_MACHINES {constant(limits, 'HALO_PORT_MAXIMUM_NETWORK_MACHINES')}\n"
    types += f"#define MAXIMUM_NETWORK_ACTORS {constant(capacity, 'HALO_PORT_MAXIMUM_ACTORS') + 32}\n"
    types += f"#define MAXIMUM_ENTRIES_PER_MESSAGE {constant(source, 'MAXIMUM_ENTRIES_PER_MESSAGE')}\n"
    types += enum_with(source, "_distributed_actor_rides_bit")
    types += enum_with(source, "_angle_yaw")
    types += structure(read("source/units/unit_control_data.h"), "unit_control_data")
    types += structure(read("port/linux/game/network_distributed.h"), "distributed_vector")
    types += structure(source, "distributed_actor_state")
    types += structure(source, "distributed_actor_state_message")
    types += structure(source, "host_actor")
    code = function(source, "actor_unit_valid")
    code += function(source, "actor_state_from_unit")
    code += function(source, "network_actors_host_tick")
    return (("actor_seat_types.inc", types), ("actor_seat_under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("network_actor_seat", generated()), case)
    assert status == 0, output


NEGATIVE_CONTROLS = {
    "idle-seat-dropped": (
        "(!actor->noted && !TEST_FLAG(unit_get(actor->unit_index)->unit.flags, _unit_running_blindly_bit) &&\n"
        "\t\t\t(!vehicle_riding_enabled() || unit_get(actor->unit_index)->object.parent_object_index == NONE ||\n"
        "\t\t\tunit_get(actor->unit_index)->unit.parent_seat_index == NONE))",
        "(!actor->noted && !TEST_FLAG(unit_get(actor->unit_index)->unit.flags, _unit_running_blindly_bit))",
        "actor-seat-idle-published",
    ),
    "seat-retention-ungated": (
        "(!vehicle_riding_enabled() || unit_get(actor->unit_index)->object.parent_object_index == NONE ||",
        "(unit_get(actor->unit_index)->object.parent_object_index == NONE ||",
        "actor-seat-idle-riding-off",
    ),
}


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    status, output = run(build("network_actor_seat", generated(control)), NEGATIVE_CONTROLS[control][-1])
    assert status == CHECK_FAILED, output
