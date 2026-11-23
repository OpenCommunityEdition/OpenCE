"""Collision's real sphere test over a fake crowded world whose clusters' lists are cluster_partitions.c's real ones
gathers the same features, in the same order, from the objects object_reach_grid.c finds as from the lists gone through.

    python tools/harness/tests/test_object_reach_grid.py
"""

import subprocess
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, inline, mutated, read, run, structure  # noqa: E402

CASES = ["moving", "unheld", "replaced", "full"]

# faults in the grid (each a before and an after), and the case that must catch them
NEGATIVE_CONTROLS = {
    # within a cluster's list, the earliest reference first
    "oldest-first": ([("places[at - 1] == place && times[at - 1] < time",
                       "places[at - 1] == place && times[at - 1] > time")], "moving"),
    # an object's squares only where its center is
    "squares-of-the-center": ([("radius + REACH_GRID_MARGIN;\n\n\t\tx0", "REACH_GRID_MARGIN;\n\n\t\tx0")], "moving"),
    # a cluster whose list holds an object not in the grid answered from the grid
    "ignores-unheld": ([("if (cluster_references_unheld[cluster_index] != 0)\n\t\t\t\treturn NONE;", "")], "unheld"),
    # a replaced game state's lists not read again
    "keeps-replaced-notes": ([("if (partition == object_reach_grid_partition)\n\t\treferences_unknown = TRUE;", ""),
                              ("\tgrid_made = TRUE;\n\treferences_unknown = TRUE;", "\tgrid_made = TRUE;")], "replaced"),
}

ENUMS = [("source/objects/object_types.h", "_object_type_biped"), ("source/objects/objects.h", "_object_invisible_bit"),
         ("source/objects/objects.h", "_object_dead_bit"), ("source/units/bipeds.h", "_biped_airborne_bit"),
         ("source/physics/collision_features.h", "_collision_feature_sphere"),
         ("source/physics/collisions.h", "_collision_test_structure_bit")]

PARTITION_FUNCTIONS = ["cluster_partition_port_forget", "cluster_partition_port_add", "cluster_partition_port_remove",
                       "cluster_partition_get_first_reference", "cluster_partition_reconnect",
                       "cluster_partition_disconnect", "cluster_partition_get_first_datum",
                       "cluster_partition_get_next_datum"]


def after_includes(text):
    return text[text.rindex("#include"):].split("\n", 1)[1]


def under_test(faults=()):
    objects = read("source/objects/objects.h")
    enums = "\n".join(enum_with(read(header), member) for header, member in ENUMS)
    for name, source in (("MAXIMUM_OBJECTS_PER_MAP", objects),
                         ("MAXIMUM_CLUSTERS_PER_STRUCTURE", read("source/structures/structures.h")),
                         ("HALO_PORT_MAXIMUM_CLUSTER_REFERENCES", read("port/linux/include/halo_port_capacity.h"))):
        enums += f"\n#define {name} {constant(source, name)}\n"
    real_math = read("source/math/real_math.h")
    text = "".join(inline(real_math, helper) + "\n"
                   for helper in ("vector_from_points3d", "magnitude_squared3d", "distance_squared3d", "point_in_sphere"))
    text += function(read("source/math/real_math.c"), "fast_vector_intersects_sphere") + "\n"
    text += structure(read("source/objects/reference_lists.h"), "data_reference") + "\n"
    text += structure(read("source/structures/cluster_partitions.h"), "cluster_partition") + "\n"
    text += inline(read("source/objects/reference_lists.h"), "reference_list_get_next_datum_index") + "\n"
    grid = after_includes(read("port/linux/game/object_reach_grid.c"))
    # (the harness's data arrays are always valid)
    grid = mutated(grid, "!partition->data_reference_data->valid ||", "")
    for before, after in faults:
        grid = mutated(grid, before, after)
    text += grid + "\n"
    text += after_includes(read("port/linux/game/object_bounds_cache.c")) + "\n"
    partitions = read("source/structures/cluster_partitions.c")
    hooks = partitions[partitions.index("/* port: object_reach_grid.c's"):]
    text += hooks[:hooks.index("\n\n")] + "\n"
    text += "".join(function(partitions, name) + "\n" for name in PARTITION_FUNCTIONS)
    # the grid's answers counted, or the lists gone through (the test's going_through)
    text += ("static long grid_counted(long count) { if (count != NONE) grid_answers++; return count; }\n"
             "#define object_reach_grid_objects_in_reach(a, b, c, d, e, f) \\\n"
             "\t(going_through ? NONE : grid_counted(object_reach_grid_objects_in_reach(a, b, c, d, e, f)))\n")
    collisions = read("source/physics/collisions.c")
    text += function(collisions, "object_get_features_in_sphere") + "\n"
    text += function(collisions, "collision_get_features_in_sphere") + "\n"
    return (("enums.inc", enums), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("object_reach_grid", under_test()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    faults, case = NEGATIVE_CONTROLS[control]
    # (caught when a check fails, or when the lists, noted wrongly, go round for ever: the cases take a second)
    try:
        status = subprocess.run([str(build("object_reach_grid", under_test(faults))), case], capture_output=True,
                                timeout=20).returncode
    except subprocess.TimeoutExpired:
        return
    assert status == CHECK_FAILED, f"the grid with a fault ({control}) passed '{case}': the test cannot see it"


if __name__ == "__main__":
    sys.exit(pytest.main(["-q", __file__]))
