"""objects.c's update loops over a fake object list: riders update after their vehicle, and the update counts.

    python -m pytest -q tools/harness/tests/test_rider_pass.py
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["after-vehicle", "dies-between-loops", "deactivated-between-loops", "parented-between-loops", "double-speed",
         "counts", "capacity", "off-no-deferred-calls"]

# (the list holds a rider only while it has room)
LIMIT = "rider_count < HALO_PORT_MAXIMUM_NETWORK_PLAYERS"
GATE = f"if (riding && {LIMIT} && deferred(object_index))"

NEGATIVE_CONTROLS = {
    "defers-nothing": ((GATE, "if (FALSE)"), "after-vehicle"),
    "passes-gate-inverted": ((GATE, f"if (!riding && {LIMIT} && deferred(object_index))"),
                             "after-vehicle"),
    "deferred-asked-when-off": ((GATE, f"if ({LIMIT} && deferred(object_index))"),
                                "off-no-deferred-calls"),
    "switch-read-per-object": ((GATE, f"if (vehicle_riding_enabled() && {LIMIT} && deferred(object_index))"), "off-no-deferred-calls"),
    "retests-riding": (("if (rider_header &&", "if (rider_header && deferred(object_index) &&"),
                       "dies-between-loops"),
    "no-slot-retest": (("\t\t\tTEST_FLAG(rider_header->flags, _object_header_active_bit) &&\n", ""),
                       "deactivated-between-loops"),
    "no-double-speed-gate": (("(!dont_update_object ||\n\t\t\t\t(TEST_FLAG(_object_mask_unit, rider_header->type)",
                              "(TRUE ||\n\t\t\t\t(TEST_FLAG(_object_mask_unit, rider_header->type)"), "double-speed"),
    "no-parent-retest": (("\t\t\tobject_get(object_index)->object.parent_object_index == NONE &&\n", ""),
                         "parented-between-loops"),
    "riders-overflow-dropped": (("riding && rider_count < HALO_PORT_MAXIMUM_NETWORK_PLAYERS && deferred(object_index))\n"
                                "\t\t\t\t{\n\t\t\t\t\triders[rider_count++] = object_index;\n\t\t\t\t}",
                                "riding && deferred(object_index))\n\t\t\t\t{\n"
                                "\t\t\t\t\tif (rider_count < HALO_PORT_MAXIMUM_NETWORK_PLAYERS)\n"
                                "\t\t\t\t\t\triders[rider_count++] = object_index;\n\t\t\t\t}"), "capacity"),
    "unbounded-count": (("&& absolute_index < MAXIMUM_OBJECTS_PER_MAP)\n\t\t++", ")\n\t\t++"), "counts"),
}


def generated(fault=None):
    objects_h = read("source/objects/objects.h")
    types = read("source/objects/object_types.h")
    # (the object types and their masks are one enum)
    enums = "\n".join([enum_with(objects_h, "_object_header_active_bit"), enum_with(types, "_object_type_biped")])
    enums += f"\n#define MAXIMUM_OBJECTS_PER_MAP {constant(objects_h, 'MAXIMUM_OBJECTS_PER_MAP')}\n"
    limits = read("port/linux/include/halo_port_limits.h")
    enums += f"#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS {constant(limits, 'HALO_PORT_MAXIMUM_NETWORK_PLAYERS')}\n"
    objects = read("port/linux/game/vehicle_riding_bipeds.c")
    text = "".join(function(objects, name) + "\n"
                   for name in ("object_count_update", "object_update_count", "objects_update_passes"))
    if fault:
        text = mutated(text, *fault)
    return (("enums.inc", enums), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("rider_pass", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("rider_pass", generated(fault)), case)
    assert status == CHECK_FAILED, f"objects.c with a fault ({control}) passed '{case}': the test cannot see it"
