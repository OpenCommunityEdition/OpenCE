"""The real scenario_leaf_index_from_point_cached (scenario.c), bsp3d_test_point_clearance (bsp3d.c) and
collision_bsp_test_vector (collision_bsp.c) over random bsps: a point within the clearance of one whose leaf was kept
is given that leaf, the one bsp3d_test_point gives, without the bsp being walked; a vector within it is given what the
walk of the collision bsp gives, one leaf and no surface, and another short one is given what the walk gives, keeping
its start as bsp3d_test_point_clearance gives it; a new bsp keeps nothing of the old one's."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["same-leaf", "reuses", "vectors", "vectors-without-walk", "new-bsp", "empty-entry", "malformed-bsp"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the clearance the greatest distance from a plane, not the least
    "greatest-distance": (("if (!(margin >= least))", "if (!(margin <= least))"), "same-leaf"),
    # a point on a plane's negative side given no clearance
    "signed-distance": (("(real)fabs(distance) * 0.999f", "distance * 0.999f"), "reuses"),
    # a vector given its start's leaf wherever it ends
    "end-unchecked": (("scenario_leaf_cache_entry_contains(entry, point0) &&\n\t\t\tscenario_leaf_cache_entry_contains("
                       "entry, point1)", "scenario_leaf_cache_entry_contains(entry, point0)"), "vectors"),
    # a vector given without the walk given no leaf
    "no-leaf-given": (("result->leaf_count = 1;", "result->leaf_count = 0;"), "vectors"),
    # the leaves of the old bsp kept
    "kept-across-bsps": (("csmemset(scenario_leaf_cache, 0, sizeof(scenario_leaf_cache));", ""), "new-bsp"),
    # an entry with nothing kept giving its leaf 0 to a point where it lies
    "empty-entry-used": (("< entry->clearance * entry->clearance", "<= entry->clearance * entry->clearance"),
                         "empty-entry"),
    # a vector's walk that keeps a start nearer a plane than nothing
    "kept-below-nothing": (("node_index != NONE && least > 0.f ? least : 0.f", "least"), "vectors"),
    # a vector divided by a plane taken down the side of its start
    "divided-undivided": (("distance0 = dot*0.f + distance;\n\t\tdistance1 = dot*t1 + distance;",
                           "distance0 = dot*0.f + distance;\n\t\tdistance1 = distance;"), "vectors"),
    # the node where a vector is divided counted twice against the bsp's nodes
    "node-counted-twice": (("\t\t\tdata->nodes_left++;\n", ""), "malformed-bsp"),
    # each cube's points replacing one entry, not its ways in turn
    "one-way": (("% SCENARIO_LEAF_CACHE_WAYS]", "% 1]"), "reuses"),
    # a vector's walk that keeps nothing
    "nothing-kept": (("\tscenario_leaf_cache_keep(", "\tif (0) scenario_leaf_cache_keep("), "vectors-without-walk"),
}


def generated(fault=None):
    bsp3d = read("source/physics/bsp3d.c")
    bsp3d_header = read("source/physics/bsp3d.h")
    scenario = read("source/scenario/scenario.c")
    collision_bsp = read("source/physics/collision_bsp.c")
    collision_bsp_header = read("source/physics/collision_bsp.h")
    collision_bsp_definitions = read("source/physics/collision_bsp_definitions.h")
    collisions_header = read("source/physics/collisions.h")
    cache = re.search(r"^static struct scenario_leaf_cache_entry scenario_leaf_cache\[[^;]*;", scenario, re.M)
    next_way = re.search(r"^static unsigned long scenario_leaf_cache_next_way;", scenario, re.M)
    config = "\n".join([
        enum_with(bsp3d_header, "MAXIMUM_BSP3D_TRAVERSAL_DEPTH"),
        structure(bsp3d_header, "bsp3d"),
        structure(bsp3d_header, "bsp3d_node"),
        enum_with(scenario, "SCENARIO_LEAF_CACHE_SIZE"),
        structure(scenario, "scenario_leaf_cache_entry"),
        cache.group(0),
        next_way.group(0),
        enum_with(collisions_header, "_collision_test_front_facing_surfaces_bit"),
        enum_with(collision_bsp, "_collision_surface_invisible_bit"),
        enum_with(collision_bsp, "_collision_leaf_contains_two_sided_bit"),
        enum_with(collision_bsp, "_contents_unknown"),
        structure(collision_bsp, "collision_leaf"),
        structure(collision_bsp_definitions, "collision_surface"),
        structure(collision_bsp_header, "collision_bsp_test_vector_result"),
        structure(collision_bsp, "collision_bsp_test_vector_context"),
    ]) + "\n"
    code = "\n".join([
        function(bsp3d, "bsp3d_test_point"),
        function(bsp3d, "bsp3d_clearance_least"),
        function(bsp3d, "bsp3d_test_point_clearance"),
        function(scenario, "scenario_leaf_cache_clear"),
        function(scenario, "scenario_leaf_cache_entry_contains"),
        function(scenario, "scenario_leaf_cache_cube"),
        function(scenario, "scenario_leaf_cache_find"),
        function(scenario, "scenario_leaf_cache_replaced"),
        function(scenario, "scenario_leaf_cache_get"),
        function(scenario, "scenario_leaf_index_from_point_cached"),
        function(scenario, "scenario_leaf_cache_test_segment"),
        function(scenario, "scenario_leaf_cache_keep"),
        function(collision_bsp, "collision_bsp3d_enter_node"),
        function(collision_bsp, "collision_bsp_test_vector_in_one_leaf"),
        function(collision_bsp, "collision_bsp_test_vector_short"),
        function(collision_bsp, "collision_bsp_test_vector_descend"),
        function(collision_bsp, "collision_bsp_test_vector"),
        function(collision_bsp, "collision_bsp_test_vector_recursive"),
    ]).replace("union real_point3d", "real_point3d") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("leaf_cache", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("leaf_cache", generated(fault)), case)
    assert status == CHECK_FAILED, f"the leaf cache with a fault ({control}) passed '{case}': the test cannot see it"
