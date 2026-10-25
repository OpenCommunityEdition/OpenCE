"""A biped's ragdoll from its model (ragdoll_bodies.c): a part for each node whose collision piece is a big enough share
of the whole, the others moving with their nearest ancestor's part; capsules for long pieces and boxes for the rest;
masses by volume; joint limits by the bones' names; the overrides of the model's section, and only its, with bad
lines changing nothing; no body for `none`, too few parts, nodes out of order or pieces that are not numbers."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = [
    "parts",
    "shapes",
    "masses",
    "limits",
    "overrides",
    "none",
    "size",
    "no data",
    "order",
    "bounds",
    "rootless",
    "capacity",
]

FUNCTIONS = [
    "ragdoll_override_line",
    "ragdoll_override_word",
    "ragdoll_override_number",
    "ragdoll_override_node",
    "ragdoll_override_section",
    "ragdoll_node_bounds",
    "ragdoll_collision_node",
    "ragdoll_extent",
    "ragdoll_part_shape_from_bounds",
    "ragdoll_part_shape_between_children",
    "ragdoll_part_volume",
    "ragdoll_joint_class_get",
    "ragdoll_part_limits",
    "ragdoll_override_angle",
    "ragdoll_override_part",
    "ragdoll_body_build",
]

# a fault in the body's making and the case that must catch it
NEGATIVE_CONTROLS = {
    "every-node-a-part": (
        (
            "boolean is_part = node_index == 0 || (forced[node_index] ? forced[node_index] > 0 : big);",
            "boolean is_part = TRUE;",
        ),
        "parts",
    ),
    "no-capsules": (("if (x > RAGDOLL_CAPSULE_LENGTH * width)", "if (FALSE)"), "shapes"),
    "folded-volume-lost": (
        ("mass_volumes[part_index] += volumes[node_index];\n\t\t\ttotal_mass_volume", "total_mass_volume"),
        "masses",
    ),
    "case-sensitive": (("if (!csstrcasecmp(collision_name, name))", "if (!strcmp(collision_name, name))"), "parts"),
    "no-classes": (("if (strstr(name, ragdoll_joint_classes[class_index].name))", "if (FALSE)"), "limits"),
    "no-limbs": (("(joint_class->limb || volumes[node_index]", "(volumes[node_index]"), "parts"),
    "any-section": (("if (!csstrcasecmp(rest, model_name))", "if (TRUE)"), "overrides"),
    "no-none": (("if (!strcmp(line, \"none\"))\n\t\t\treturn FALSE;", ""), "none"),
    "past-the-size": (
        (
            "char const *limit = overrides + (overrides_size > 0 ? overrides_size : 0);",
            "char const *limit = overrides + strlen(overrides);",
        ),
        "size",
    ),
    "no-order-check": (
        (
            "node->parent_node_index < 0 || node->parent_node_index >= node_index)\n\t\t\treturn FALSE;",
            "FALSE)\n\t\t\treturn FALSE;",
        ),
        "order",
    ),
    "no-nan-check": (("if (point->x != point->x || point->y != point->y || point->z != point->z ||", "if ("), "bounds"),
    "no-capacity": (("if (!is_part || body->part_count >= RAGDOLL_BODY_PARTS_MAX)", "if (!is_part)"), "capacity"),
    "root-breaks": (
        ("if (part->parent_part_index == NONE || !ragdoll_override_number(", "if (!ragdoll_override_number("),
        "overrides",
    ),
    "tiny-breaks": (("changed.break_damage < 1.0f ||", ""), "overrides"),
    "unchecked-joint": (
        ("changed.twist_min > changed.twist_max)\n\t\t\treturn;", "FALSE)\n\t\t\treturn;"),
        "overrides",
    ),
}


def defines(source):
    return "".join(line + "\n" for line in source.splitlines() if re.match(r"#define RAGDOLL_\w+ ", line))


def generated(fault=None):
    bodies = read("port/linux/game/ragdoll_bodies.c")
    header = read("port/linux/game/ragdoll_bodies.h")
    config = (
        "\n".join(
            [
                enum_with(header, "RAGDOLL_BODY_PARTS_MAX"),
                enum_with(header, "_ragdoll_shape_sphere"),
                structure(header, "ragdoll_part"),
                structure(header, "ragdoll_body"),
                defines(bodies),
                structure(bodies, "ragdoll_joint_class"),
                bodies[
                    bodies.index("static struct ragdoll_joint_class const ragdoll_joint_classes[]") : bodies.index(
                        "};", bodies.index("ragdoll_joint_classes[]")
                    )
                    + 2
                ],
            ]
        )
        + "\n"
    )
    code = "\n".join(function(bodies, name) for name in FUNCTIONS)
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("ragdoll_bodies", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("name", sorted(NEGATIVE_CONTROLS))
def test_negative_control(name):
    fault, case = NEGATIVE_CONTROLS[name]
    status, output = run(build("ragdoll_bodies", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_overrides_are_embedded():
    embed = read("tools/embed_assets.py")
    assert "port/assets/ragdolls/ragdolls.txt" in embed and "ragdoll_overrides_embedded" in embed
