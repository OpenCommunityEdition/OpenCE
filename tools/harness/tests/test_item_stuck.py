"""The real item_stuck_count (item_stuck.c): a falling item wedged where it cannot come to rest (a crease, a wall, a
steep slope) hit the ground every tick without getting anywhere, spinning and playing its collision sound with each
hit, until it was picked up. One that hits the ground on 10 ticks in a row, moving no further than 3 cm in any of them,
is put at rest where it is; a tick in the air, a further move, or a hit on anything but the ground starts the count
again, so an item that bounces, slides or falls is never stopped."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["stuck", "in the air", "moving", "not the ground", "any count"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # never put at rest (before fix-stuck-items)
    "never": (("\treturn TRUE;\n}", "\treturn FALSE;\n}"), "stuck"),
    # at rest on the second hit
    "early": (("if (*stuck_ticks < ITEM_STUCK_TICKS)\n", "if (*stuck_ticks < 2)\n"), "in the air"),
    # however far it moves
    "any-distance": (("if (!hit_ground || !(distance_moved <= ITEM_STUCK_DISTANCE))",
                      "if (!hit_ground || distance_moved != distance_moved)"), "moving"),
    # whatever it hits
    "any-hit": (("if (!hit_ground || !(distance_moved <= ITEM_STUCK_DISTANCE))",
                 "if (!(distance_moved <= ITEM_STUCK_DISTANCE))"), "not the ground"),
    # a tick in the air does not start the count again
    "no-restart": (("\t\t*stuck_ticks = 0;\n\t\treturn FALSE;", "\t\treturn FALSE;"), "in the air"),
    # a count that is not one taken as it is
    "any-count": (("if (*stuck_ticks < 0 || *stuck_ticks >= ITEM_STUCK_TICKS)", "if (0)"), "any count"),
}


def generated(fault=None):
    source = read("port/linux/game/item_stuck.c").replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define ITEM_STUCK_\w+ .*$", source, re.M))
    code = defines + "\n\n" + function(source, "item_stuck_count") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_items_count_it():
    """item_update counts each hit where the item does not rest, puts it at rest where it is (as it is, not turned to
    lie on what it hit, and no longer spinning) when it has been stuck, and starts the count again in the air."""
    items = read("source/items/items.c").replace("\r\n", "\n")
    update = function(items, "item_update")
    assert "stuck = !rests && item_stuck_count(\n" \
           "\t\t\t\t\t&item->item.stuck_ticks,\n" \
           "\t\t\t\t\thit_ground,\n" \
           "\t\t\t\t\tdistance3d(&item->object.position, &collision.point));" in update
    assert "if (rests || stuck)" in update
    assert "if (rests)\n\t\t\t\t\t{\n\t\t\t\t\t\titem_align_to_normal_and_point(" in update
    assert "item->object.angular_velocity = *global_zero_vector3d;" in update
    assert "else\n\t\t\t{\n\t\t\t\titem->item.stuck_ticks = 0;\n\t\t\t}" in update


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("item_stuck", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("item_stuck", generated(fault)), case)
    assert status == CHECK_FAILED, f"item_stuck_count with a fault ({control}) passed '{case}'"
