"""The cryo bay cell's collision bsp (port/linux/game/coop_cryo_cell.c), tested by the engine's own bsp code."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, inline, mutated, read, run, structure  # noqa: E402

CASES = ["faces", "points", "rays", "ground", "spheres"]

# a fault in the cell's code, and the case that must catch it
NEGATIVE_CONTROLS = {
    "rings-clockwise": (("angles[b - 1] > angles[b]", "angles[b - 1] < angles[b]"), "faces"),
    "corner-dropped": (("CELL_MAXIMUM_FACE_POINTS = 8,", "CELL_MAXIMUM_FACE_POINTS = 7,"), "faces"),
    "leaves-inside": (("node->children[1] = (long)(LONG_MIN | face);\n\t\t\tnode->children[0] = next;",
                       "node->children[0] = (long)(LONG_MIN | face);\n\t\t\tnode->children[1] = next;"), "points"),
    "separator-in-the-plinth": (("{ 0.0f, 0.0f, 1.0f, 0.8025f }", "{ 0.0f, 0.0f, 1.0f, 0.79f }"), "points"),
    "outlines-inside-out": (("area += a->x * b->y - b->x * a->y;", "area -= a->x * b->y - b->x * a->y;"), "rays"),
    "edges-unlinked": (("edges[ring[k]].edge_indices[edges[ring[k]].surface_indices[1] == face]",
                        "edges[ring[k]].edge_indices[0]"), "faces"),
    "references-signed": (("references[face].plane_designator = plane_index;",
                           "references[face].plane_designator = plane_index | LONG_MIN;"), "spheres"),
}

MATH = ("set_real_point2d", "set_real_point3d", "set_real_vector3d", "vector_from_points2d", "cross_product2d",
        "vector_from_points3d", "dot_product3d", "cross_product3d", "magnitude_squared3d", "square_root", "magnitude3d",
        "scale_vector3d", "normalize3d", "plane3d_from_points", "plane3d_distance_to_point", "plane2d_distance_to_point",
        "projection_sign_from_vector3d", "project_point3d")
COLLISION_BSP = ("collision_surface_edge_ring_continues", "collision_bsp_valid_surface_index", "collision_surface_polygon",
                 "collision_bsp3d_enter_node", "collision_bsp2d_enter_node", "collision_bsp_valid_leaf", "add_feature",
                 "collision_surface_test_sphere", "bsp2d_test_sphere_recursive", "bsp3d_test_sphere_recursive",
                 "collision_bsp_test_sphere", "collision_leaf_test_vector", "collision_bsp_test_vector_recursive",
                 "collision_bsp_test_vector")
CELL = ("cell_allocate", "cell_block_set", "cell_faces", "cell_collision_bsp")


def union(source, name):
    """A named union's definition and its typedef."""
    match = re.search(r"^union\s+" + re.escape(name) + r"\s*\{.*?^\};\ntypedef union " + re.escape(name) + r" \w+;",
                      source, re.M | re.S)
    if not match:
        raise LookupError(f"union not found in the sources: {name}")
    return match.group(0)


def table(source, name):
    match = re.search(r"^static \w+ const " + re.escape(name) + r"\[.*?^\};\n", source, re.M | re.S)
    if not match:
        raise LookupError(f"{name} not found in coop_cryo_cell.c")
    return match.group(0)


def same(a, b):
    return re.sub(r"\s+", " ", a) == re.sub(r"\s+", " ", b)


def generated(fault=None):
    cseries = read("source/cseries/cseries.h")
    real_math = read("source/math/real_math.h")
    collision_bsp = read("source/physics/collision_bsp.c")
    definitions = read("source/physics/collision_bsp_definitions.h")
    cell = read("port/linux/game/coop_cryo_cell.c")
    if fault:
        cell = mutated(cell, *fault)

    epsilon = re.search(r"^#define _real_epsilon .*$", real_math, re.M)
    assert epsilon, "_real_epsilon not found in real_math.h"
    types = epsilon.group(0) + "\n" + enum_with(cseries, "_x = 0") + "\n"
    for name in ("real_point2d", "real_point3d", "real_vector2d", "real_vector3d"):
        types += union(real_math, name) + "\n"
    types += structure(real_math, "real_plane2d") + "\ntypedef struct real_plane2d real_plane2d;\n"
    types += structure(real_math, "real_plane3d") + "\ntypedef struct real_plane3d real_plane3d;\n"
    types += structure(read("source/tag_files/tag_groups.h"), "tag_block") + "\n"
    bsp2d = read("source/physics/bsp2d.h")
    bsp3d = read("source/physics/bsp3d.h")
    types += enum_with(bsp2d, "MAXIMUM_BSP2D_TRAVERSAL_DEPTH") + "\n" + enum_with(bsp3d, "MAXIMUM_BSP3D_TRAVERSAL_DEPTH") + "\n"
    types += structure(bsp2d, "bsp2d_node") + "\n" + structure(bsp2d, "bsp2d") + "\n"
    types += structure(bsp3d, "bsp3d") + "\n" + structure(bsp3d, "bsp3d_node") + "\n"
    types += enum_with(definitions, "MAXIMUM_EDGES_PER_COLLISION_SURFACE") + "\n"
    for name in ("collision_surface", "collision_edge", "collision_vertex", "collision_bsp"):
        types += structure(definitions, name) + "\n"
    types += enum_with(collision_bsp, "MAXIMUM_BSP3D_DEPTH") + "\n"
    for member in ("_collision_surface_two_sided_bit", "_collision_leaf_contains_two_sided_bit", "_contents_unknown"):
        types += enum_with(collision_bsp, member) + "\n"
    types += enum_with(read("source/physics/collisions.h"), "_collision_test_front_facing_surfaces_bit") + "\n"
    for name in ("collision_leaf", "bsp2d_reference"):
        engine = structure(collision_bsp, name)
        assert same(engine, structure(cell, name)), f"coop_cryo_cell.c's {name} is not collision_bsp.c's"
        types += engine + "\n"
    collision_bsp_h = read("source/physics/collision_bsp.h")
    types += structure(collision_bsp_h, "collision_bsp_test_vector_result") + "\n"
    types += structure(collision_bsp_h, "collision_bsp_test_sphere_result") + "\n"
    types += structure(collision_bsp, "collision_bsp_test_vector_context") + "\n"
    types += structure(collision_bsp, "test_sphere_data") + "\n"

    code = [inline(real_math, name) for name in MATH]
    real_math_c = read("source/math/real_math.c")
    mappings = re.search(r"^short const global_projection3d_mappings\[3\]\[2\]\[2\] =\n\{.*?^\};\n", real_math_c,
                         re.M | re.S)
    assert mappings, "global_projection3d_mappings not found in real_math.c"
    text = mappings.group(0)
    code.append(function(real_math_c, "fast_vector_intersects_sphere"))
    code.append(function(read("source/physics/bsp2d.c"), "bsp2d_test_point"))
    code.append(function(read("source/physics/bsp3d.c"), "bsp3d_test_point"))
    code += [function(collision_bsp, name) for name in COLLISION_BSP]

    text += enum_with(cell, "CELL_PIECES") + "\n"
    for name in ("cell_collision_corners", "cell_collision_triangles", "cell_collision_pieces", "cell_separators"):
        text += table(cell, name)
    text += structure(cell, "cell_face") + "\n"
    end = cell.index("} cell_globals = { NONE };\n")
    text += cell[cell.rindex("static struct\n{", 0, end):end] + "} cell_globals = { NONE };\n"
    code += [function(cell, name) for name in CELL]

    text += "".join(body[:body.index("{")].rstrip() + ";\n" for body in code)
    text += "\n".join(code) + "\n"
    return (("types.inc", types), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("coop_cryo_cell", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("coop_cryo_cell", generated(fault)), case)
    assert status == CHECK_FAILED, f"coop_cryo_cell.c with a fault ({control}) passed '{case}': the test cannot see it"
