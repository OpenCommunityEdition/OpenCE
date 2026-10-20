"""The host's bodies drawn as wireframes (physics_draw.c, debug.physics_draw): each posed ragdoll's parts and each posed
piece of debris in the game's debug lines, at the host's poses, as the host's shapes are (a capsule along its frame's x
axis, a box's twelve edges), coloured by whether it is awake or torn; nothing with the setting off or without the host's
physics, and nothing for a ragdoll the host has not posed yet."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["off", "off without the host", "setting changed", "capsule", "sphere", "box", "colours", "unposed",
         "every debris id"]

FUNCTIONS = ["physics_draw_enabled", "physics_draw_point", "physics_draw_shape", "physics_draw_render"]

# a fault in the drawing and the case that must catch it
NEGATIVE_CONTROLS = {
    "always-drawn": (("\tif (!physics_draw_enabled())\n\t\treturn;\n", ""), "off"),
    "without-the-host": (("return physics_draw_on && physics_jolt_enabled();", "return physics_draw_on;"),
                         "off without the host"),
    "read-once": (("if (config_changes() != physics_draw_changes)", "if (physics_draw_changes == (unsigned long)-1)"),
                  "setting changed"),
    "capsule-from-its-middle": (("base.y = 2.0f * middle.y - end.y;", "base.y = middle.y;"), "capsule"),
    "capsule-along-y": (("end.x += size[1];", "end.y += size[1];"), "capsule"),
    "unturned": (("result->y = pose->position.y + point->y + q->w * t.j + (q->v.k * t.i - q->v.i * t.k);",
                  "result->y = pose->position.y + point->y;"), "sphere"),
    "off-centre": (("physics_draw_point(pose, center, &middle);", "middle = pose->position;"), "sphere"),
    "box-diagonals": (("if (!(index & bit))", "if (index == 0)"), "box"),
    "box-half-size": (("corner.y = center->y + ((index & 2) ? size[1] : -size[1]);",
                       "corner.y = center->y + ((index & 2) ? size[1] : 0.0f);"), "box"),
    "asleep-torn": (("!ragdoll->awake ? &physics_draw_asleep :\n\t\t\t\t", ""), "colours"),
    "nothing-torn": (("ragdoll_part_piece(ragdoll, part_index) != 0 ? &physics_draw_torn : ", ""), "colours"),
    "debris-always-awake": (("awake ? &physics_draw_debris_awake : &physics_draw_asleep", "&physics_draw_debris_awake"),
                            "colours"),
    "unposed-drawn": (("if (!ragdoll || !ragdoll->posed)", "if (!ragdoll)"), "unposed"),
    "last-debris-skipped": (("id < PHYSICS_DEBRIS_FIRST_ID + PHYSICS_DEBRIS_MAX;", "id < PHYSICS_DEBRIS_FIRST_ID + PHYSICS_DEBRIS_MAX - 1;"),
                            "every debris id"),
}


def generated(fault=None):
    draw = read("port/linux/game/physics_draw.c")
    bodies = read("port/linux/game/ragdoll_bodies.h")
    ragdolls = read("port/linux/game/ragdolls.h")
    debris = read("port/linux/game/physics_debris.h")
    state = draw[draw.index("static unsigned long physics_draw_changes"):draw.index("boolean physics_draw_enabled(void)")]
    config = "\n".join([
        enum_with(bodies, "RAGDOLL_BODY_PARTS_MAX"),
        enum_with(bodies, "_ragdoll_shape_capsule"),
        enum_with(ragdolls, "RAGDOLLS_MAX"),
        "#define PHYSICS_DEBRIS_FIRST_ID " + re.search(r"#define PHYSICS_DEBRIS_FIRST_ID (\S+)", debris)[1],
        "#define PHYSICS_DEBRIS_MAX " + re.search(r"#define PHYSICS_DEBRIS_MAX (\S+)", debris)[1],
        "\n".join(re.findall(r"#define HOST_PHYSICS_SHAPE_\w+ \d+", draw)[:3]),
        structure(bodies, "ragdoll_part"),
        structure(bodies, "ragdoll_body"),
        structure(ragdolls, "ragdoll_pose"),
        structure(ragdolls, "ragdoll"),
        structure(debris, "physics_debris_body"),
        state]) + "\n"
    code = "\n".join(function(draw, name) for name in FUNCTIONS)
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("physics_draw", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("name", sorted(NEGATIVE_CONTROLS))
def test_negative_control(name):
    fault, case = NEGATIVE_CONTROLS[name]
    status, output = run(build("physics_draw", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_the_shapes_are_the_hosts():
    """the host's shape numbers, and the ragdolls' the same"""
    host = read("port/macos/include/host_physics.h")
    draw = read("port/linux/game/physics_draw.c")
    for name, number in re.findall(r"#define (HOST_PHYSICS_SHAPE_\w+) (\d+)", draw):
        assert "#define %s %s" % (name, number) in host
    shapes = enum_with(read("port/linux/game/ragdoll_bodies.h"), "_ragdoll_shape_capsule")
    assert re.findall(r"_ragdoll_shape_(\w+)", shapes) == ["sphere", "capsule", "box"]


def test_render_debug_calls_in():
    """render_debug() draws them after the game's own debug drawing, marked `port:`"""
    render = function(read("source/render/render_debug.c"), "render_debug")
    assert re.search(r"players_debug_render\(\);\n\t/\* port:[^*]*\*/\n\tphysics_draw_render\(\);", render)
    assert "void physics_draw_render(void);" in read("port/linux/include/halo_linux_source_fixups.h")
