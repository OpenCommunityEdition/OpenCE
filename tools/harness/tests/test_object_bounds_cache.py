"""Collision's real object loop over a fake crowded world gathers the same features with the object bounds cache.

    python tools/harness/tests/test_object_bounds_cache.py     # the cases, then how fast the cache is
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, inline, mutated, read, run  # noqa: E402

CASES = ["identical", "moving", "detects-stale", "invalidated"]

# a fault in the cache, and the case that must catch it
NEGATIVE_CONTROLS = {
    "ignores-invalidation": (("bounds->epoch == object_bounds_epoch &&", ""), "invalidated"),
    "drops-the-query-radius": (("bounds->radius + radius", "bounds->radius"), "identical"),
}


# the game's enums the fake world and the code under test use: (header, a member of the enum)
ENUMS = [("source/objects/object_types.h", "_object_type_biped"), ("source/objects/objects.h", "_object_invisible_bit"),
         ("source/objects/objects.h", "_object_dead_bit"), ("source/units/bipeds.h", "_biped_airborne_bit"),
         ("source/physics/collision_features.h", "_collision_feature_sphere"),
         ("source/physics/collisions.h", "_collision_test_structure_bit")]


def under_test(fault=None):
    enums = "\n".join(enum_with(read(header), member) for header, member in ENUMS)
    enums += f"\n#define MAXIMUM_OBJECTS_PER_MAP {constant(read('source/objects/objects.h'), 'MAXIMUM_OBJECTS_PER_MAP')}\n"
    real_math = read("source/math/real_math.h")
    text = "".join(inline(real_math, helper) + "\n"
                   for helper in ("vector_from_points3d", "magnitude_squared3d", "distance_squared3d", "point_in_sphere"))
    cache = read("port/linux/game/object_bounds_cache.c")
    cache = cache[cache.rindex("#include"):].split("\n", 1)[1]
    text += (mutated(cache, *fault) if fault else cache) + "\n"
    collisions = read("source/physics/collisions.c")
    text += function(collisions, "object_get_features_in_sphere") + "\n"
    text += function(collisions, "collision_get_features_in_sphere") + "\n"
    return (("enums.inc", enums), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("object_bounds_cache", under_test()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("object_bounds_cache", under_test(fault)), case)
    assert status == CHECK_FAILED, f"the cache with a fault ({control}) passed '{case}': the test cannot see it"


if __name__ == "__main__":
    status = pytest.main(["-q", __file__])
    print(run(build("object_bounds_cache", under_test()), "benchmark")[1])
    sys.exit(status)
