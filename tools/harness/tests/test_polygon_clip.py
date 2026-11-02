"""The real convex_hull2d_intersect and convex_polygon2d_clip_to_plane (source/math/geometry.c): a corner of a portal's
outline lying on a clip line, with the corner before it a rounding error behind, makes no NaN corner (0/0 at the
crossing), which spread through the portal walk and left whole clusters undrawn for a frame (the level flashing)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, inline, mutated, read, run  # noqa: E402

CASES = ["shared-corners-a", "shared-corners-b", "ordinary-clip"]

# a fault and the case that must catch it
NEGATIVE_CONTROLS = {
    "nan-crossing": (("if (scale != scale) scale = 0.f;\n", ""), "shared-corners-a"),
    "nan-crossing-b": (("if (scale != scale) scale = 0.f;\n", ""), "shared-corners-b"),
    "no-clip": (("result_count = convex_polygon2d_clip_to_plane(", "result_count = 0 * convex_polygon2d_clip_to_plane("),
                "shared-corners-a"),
}

HELPERS = ["square_root", "magnitude_squared2d", "magnitude2d", "scale_vector2d", "normalize2d", "dot_product2d",
           "vector_from_points2d", "point_from_line2d", "plane2d_from_points", "plane2d_distance_to_point"]


def generated(fault=None):
    real_math = read("source/math/real_math.h")
    geometry = read("source/math/geometry.c")
    code = "#define CLIP_BUFFER_SIZE %d\n" % constant(geometry, "CLIP_BUFFER_SIZE")
    code += "\n".join(inline(real_math, name) for name in HELPERS) + "\n"
    code += "static " + function(geometry, "vector_intersect_plane2d") + "\n"
    clip = "static " + function(geometry, "convex_polygon2d_clip_to_plane") + "\n"
    hull = "static " + function(geometry, "convex_hull2d_intersect") + "\n"
    if fault:
        before, after = fault
        if before in clip:
            clip = mutated(clip, before, after)
        else:
            hull = mutated(hull, before, after)
    return (("under_test.inc", code + clip + hull),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("polygon_clip", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("polygon_clip", generated(fault)), case)
    assert status == CHECK_FAILED, f"the clip with a fault ({control}) passed '{case}': the test cannot see it"
