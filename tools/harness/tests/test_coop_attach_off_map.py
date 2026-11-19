"""A co-op client attaches what the host attached, also an item an update called carried, unless it is in an inventory."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["on-map", "carried-scenery", "carried-vehicle", "carried-detach", "carried-own-child", "parented-off-map",
         "attach-detach-inventory", "attach-inventory", "inventory-attach-detach"]


def generated(fault=None):
    coop = read("port/linux/game/network_coop.c")
    objects = read("source/objects/objects.c")
    code = "\n".join([
        function(objects, "objects_scripting_attach"),
        function(objects, "objects_scripting_detach"),
        function(read("source/units/units.c"), "unit_network_add_weapon"),
        function(coop, "client_apply_attach"),
    ])
    if fault:
        code = mutated(code, *fault)
    object_types = read("source/objects/object_types.h")
    declarations = "\n".join([
        "#define MAXIMUM_WEAPONS_PER_UNIT %d" % constant(read("source/units/units.h"), "MAXIMUM_WEAPONS_PER_UNIT"),
        enum_with(read("source/objects/objects.h"), "_object_connected_to_map_bit,"),
        enum_with(object_types, "_object_mask_all ="),
        enum_with(read("source/items/items.h"), "_item_attached_to_unit_bit ="),
        enum_with(coop, "_coop_attach,"),
        structure(coop, "distributed_coop_event"),
    ])
    return (("declarations.inc", declarations + "\n"), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_attach(case):
    status, output = run(build("coop_attach_off_map", generated()), case)
    assert status == 0, output


RECONNECT = "\t\t\tobject_reconnect_to_map(child_index, NULL);\n"
SHOW = "\t\t\tobject_set_visibility(child_index, TRUE);\n"
OFF_MAP = "off_map = event->type == _coop_attach && object_get(child_index)->object.parent_object_index == NONE &&"
IN_INVENTORY = "TEST_FLAG(item_get(child_index)->item.flags, _item_attached_to_unit_bit))"
OWN_CHILDREN = "\t/* (not to one of its own children"


@pytest.mark.parametrize("fault, case", [
    # main's: no check, the attach of an item off the map
    ((OFF_MAP, "off_map = FALSE &&"), "carried-scenery"),
    # skipping every item off the map (the PR's first version)
    ((IN_INVENTORY, "TRUE)"), "carried-scenery"),
    # attaching it off the map
    ((RECONNECT, ""), "carried-scenery"),
    # attaching it hidden
    ((SHOW, ""), "carried-scenery"),
    # attaching one in an inventory
    ((IN_INVENTORY, "FALSE)"), "inventory-attach-detach"),
    # logging the expected skip of one in an inventory
    ((IN_INVENTORY + "\n\t{\n", IN_INVENTORY + "\n\t{\n\t\terror(_error_silent, \"\");\n"), "inventory-attach-detach"),
    # putting it back on the map before the check of the parent's ancestors
    ((OWN_CHILDREN, "\tif (off_map)\n\t{\n" + RECONNECT[1:] + SHOW[1:] + "\t}\n" + OWN_CHILDREN), "carried-own-child"),
    # putting back a child that has a parent
    ((OFF_MAP, "off_map = event->type == _coop_attach &&"), "parented-off-map"),
])
def test_negative_control(fault, case):
    status, output = run(build("coop_attach_off_map", generated(fault)), case)
    assert status == CHECK_FAILED, output
