"""A vehicle state within tolerance keeps the host's velocities on this machine's copy (riding on only)."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure
from test_vehicle_riding import engine, without_includes

OBJECTS = "port/linux/game/network_objects.c"
RIDING = "port/linux/game/vehicle_riding.c"
CASES = [
    "vehicle-state-close-refreshes-velocity", "vehicle-state-close-riding-off-original",
    "vehicle-state-close-own-vehicle-unchanged", "vehicle-state-far-unchanged",
    "vehicle-state-unsteered-path-unchanged", "gun-takeoff-frame-refreshed",
]

REFRESH = ("\t\t\tif (vehicle_riding_enabled() && object->object.type == _object_type_vehicle && !own_vehicle &&\n"
           "\t\t\t\t!distributed_vehicle_unsteered(state->object_index))\n"
           "\t\t\t{\n"
           "\t\t\t\tobject->object.translational_velocity = velocity;\n"
           "\t\t\t\tobject->object.angular_velocity = angular_velocity;\n"
           "\t\t\t}\n")
NEGATIVE_CONTROLS = {
    "vehicle-refresh-dropped": (
        "\t\t\t\tobject->object.translational_velocity = velocity;\n\t\t\t\tobject->object.angular_velocity = angular_velocity;\n",
        "\t\t\t\tobject->object.angular_velocity = angular_velocity;\n", "vehicle-state-close-refreshes-velocity"),
    "vehicle-refresh-angular-dropped": (
        "\t\t\t\tobject->object.angular_velocity = angular_velocity;\n", "", "vehicle-state-close-refreshes-velocity"),
    "vehicle-refresh-ungated": (
        "if (vehicle_riding_enabled() && object->object.type == _object_type_vehicle && !own_vehicle &&",
        "if (object->object.type == _object_type_vehicle && !own_vehicle &&", "vehicle-state-close-riding-off-original"),
    "vehicle-refresh-own-vehicle": (
        "_object_type_vehicle && !own_vehicle &&\n\t\t\t\t!distributed_vehicle_unsteered",
        "_object_type_vehicle &&\n\t\t\t\t!distributed_vehicle_unsteered", "vehicle-state-close-own-vehicle-unchanged"),
    "vehicle-refresh-moves": (
        "\t\t\t\tobject->object.translational_velocity = velocity;\n\t\t\t\tobject->object.angular_velocity = angular_velocity;\n",
        "\t\t\t\tnetwork_objects_reconcile(state->object_index, &state->position, &forward, &up, &velocity,\n"
        "\t\t\t\t\t&angular_velocity, blend_distance, _network_vehicle_write_state);\n",
        "vehicle-state-close-refreshes-velocity"),
}


def generated(fault=None):
    source = read(OBJECTS)
    capacity = read("port/linux/include/halo_port_capacity.h")
    header = read("port/linux/game/network_distributed.h")
    config = re.search(r"#define HALO_PORT_MAXIMUM_OBJECTS_PER_MAP[^\n]+", capacity).group() + "\n"
    config += "#define MAXIMUM_TRACKED_OBJECTS HALO_PORT_MAXIMUM_OBJECTS_PER_MAP\n"
    types = engine() + "\n" + without_includes(read("port/linux/game/vehicle_riding.h")) + "\n"
    types += structure(header, "distributed_vector") + "\n"
    types += structure(source, "distributed_object_state") + "\n"
    types += enum_with(source, "_distributed_object_at_rest_bit") + "\n"
    for name in ("DISTRIBUTED_UNIT_SCALE", "DISTRIBUTED_VELOCITY_SCALE", "DISTRIBUTED_ANGULAR_VELOCITY_SCALE",
                 "REMOTE_BLEND_DISTANCE", "REMOTE_VEHICLE_BLEND_DISTANCE"):
        types += re.search(r"^#define " + name + r" .*$", header, re.M).group(0) + "\n"
    for name in ("REMOTE_OBJECT_TOLERANCE", "REMOTE_OBJECT_ANGLE_TOLERANCE", "LOCAL_VEHICLE_TOLERANCE"):
        types += re.search(r"^#define " + name + r" .*$", source, re.M).group(0) + "\n"
    types += "typedef struct { real_point3d unused; } vehicle_riding_dummy;\n"
    code = "\n".join(function(source, name) for name in (
        "network_objects_reconcile", "distributed_object_state_unpack", "network_objects_handle_states"))
    if fault:
        code = mutated(code, *fault)
    riding = read(RIDING)
    code = function(riding, "riding_period") + "\n" + function(riding, "vehicle_riding_deck_point_velocity") + "\n" + code
    return (("types.inc", types), ("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("vehicle_state_refresh", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    before, after, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("vehicle_state_refresh", generated((before, after))), case)
    assert status == CHECK_FAILED, output
    assert "check failed:" in output, output
