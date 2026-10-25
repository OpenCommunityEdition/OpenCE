"""The level's triangles for the host's Jolt world (physics_jolt.c): every collision surface of the bsp cut into a fan
facing as its plane does (the designator's sign bit turning it), two-sided surfaces both ways, broken rings left out,
and no more triangles than there is room for."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["counted", "floor", "wall", "pane", "capacity", "indices"]

# a fault in the triangulation and the case that must catch it
NEGATIVE_CONTROLS = {
    "no-turning": (("flipped = (facing < 0.0f)", "flipped = (facing < 0.0f && FALSE)"), "floor"),
    "no-sign-bit": (("!= ((surface->plane_designator & LONG_MIN) != 0);", "!= FALSE;"), "wall"),
    "one-side": (("short sides = two_sided ? 2 : 1;", "short sides = 1;"), "counted"),
    "no-vertex-check": (
        ("if (vertex_index < 0 || vertex_index >= bsp->vertices.count)\n\t\t\treturn 0;", ""),
        "counted",
    ),
    "no-capacity": (("if (triangle_count >= triangle_capacity)\n\t\t\t\t\t\treturn triangle_count;", ""), "capacity"),
}


def generated(fault=None):
    jolt = read("port/linux/game/physics_jolt.c")
    config = enum_with(jolt, "_physics_jolt_surface_two_sided_bit") + "\n"
    config += "#define PHYSICS_JOLT_RING_MAX 8\n"
    code = "\n".join([function(jolt, "physics_jolt_surface_ring"), function(jolt, "physics_jolt_level_triangles")])
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("physics_level", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("name", sorted(NEGATIVE_CONTROLS))
def test_negative_control(name):
    fault, case = NEGATIVE_CONTROLS[name]
    status, output = run(build("physics_level", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_ring_limit_is_the_sources():
    jolt = read("port/linux/game/physics_jolt.c")
    assert "#define PHYSICS_JOLT_RING_MAX 8" in jolt
