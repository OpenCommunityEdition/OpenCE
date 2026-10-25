"""Debris as rigid bodies (physics_debris.c): a particle with point physics that strikes the level becomes a capsule as
wide as it is, as heavy as its density makes it, of its material within the host's limits, flying on as it bounced;
smoke and other particles lighter than air stay the game's, as does everything with game.physics_debris off. The
particle is held where it struck until the host poses its body, then moved between the host's last two poses, and is
settled once the body sleeps or has crept slower than the game's particles at rest (0.25 units a second) for half a
second. The body goes with its particle; past game.physics_debris_max the oldest piece is handed back to point physics
for good, as is one the host refused. The point physics are the maps' own: bloodgulch's casings (effects\\point
physics\\stones: 2.4, 0.1, 0.35), the Warthog's (old tags\\stones: 2.4, 0.2, 0.5) and the smoke's (0.0004 to 0.00115)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = [
    "capsule",
    "real profiles",
    "clamps",
    "collided",
    "off",
    "not debris",
    "not twice",
    "held until posed",
    "moved by the host",
    "creeping",
    "out of the level",
    "lifetime",
    "gone before added",
    "cap",
    "cap before added",
    "refused",
    "ids",
    "disposed",
    "garbage",
    "garbage without a hull",
    "garbage not debris",
    "garbage drawn",
]

FUNCTIONS = [
    "physics_debris_maximum",
    "physics_debris_table",
    "physics_debris_handed_back",
    "physics_debris_of",
    "physics_debris_was_handed_back",
    "physics_debris_free",
    "physics_debris_take",
    "physics_debris_particle_body",
    "physics_debris_particle_collided",
    "physics_debris_particle_update",
    "physics_debris_garbage_update",
    "physics_debris_node_matrices",
    "physics_debris_of_id",
    "physics_debris_piece_gone",
    "physics_debris_remove",
    "physics_debris_add",
    "physics_debris_result",
    "physics_debris_dispose_from_old_map",
]

MATRIX_FUNCTIONS = [
    "matrix4x3_rotation_to_quaternion",
    "matrix4x3_rotation_from_quaternion",
    "matrix4x3_inverse",
    "matrix4x3_multiply",
    "matrix4x3_transform_point",
]

# a fault in the debris and the case that must catch it
NEGATIVE_CONTROLS = {
    "always-on": (
        (
            "if (!physics_jolt_debris_enabled() ||\n\t\tphysics_debris_of(_physics_debris_particle",
            "if (physics_debris_of(_physics_debris_particle",
        ),
        "off",
    ),
    "smoke-tumbles": (("!(physics->density >= PHYSICS_DEBRIS_DENSITY_MIN) || ", ""), "real profiles"),
    "massless": (("physics->density * 1000.0f *", "1000.0f *"), "capsule"),
    "half-as-wide": (("capsule_radius = PIN(0.35f * radius", "capsule_radius = PIN(0.175f * radius"), "capsule"),
    "unclamped-friction": (("PIN(physics->contact_friction, 0.0f, 1.0f)", "physics->contact_friction"), "clamps"),
    "unclamped-speed": (("if (speed > PHYSICS_DEBRIS_SPEED_MAX)", "if (FALSE)"), "clamps"),
    "sideways": (("frame.forward.i = velocity->i / speed;", "frame.forward.i = 1.0f;"), "capsule"),
    "no-tumble": (("spin = PIN(speed / (4.0f * radius), 0.0f, 50.0f);", "spin = 0.0f;"), "capsule"),
    "twice": (("\t\tphysics_debris_of(_physics_debris_particle, particle_index) ||\n", "\t\t"), "not twice"),
    "handed-back-again": (
        ("||\n\t\tphysics_debris_was_handed_back(_physics_debris_particle, particle_index))", ")"),
        "cap",
    ),
    "not-held": (("\tif (!debris->posed)\n\t\treturn TRUE;\n", ""), "held until posed"),
    "uninterpolated": (("t = PIN(debris->elapsed * TICKS_PER_SECOND, 0.0f, 1.0f);", "t = 1.0f;"), "moved by the host"),
    "never-settled": (
        ("*settled = !debris->awake || debris->ticks_slow >= PHYSICS_DEBRIS_REST_TICKS;", "*settled = FALSE;"),
        "moved by the host",
    ),
    "never-at-rest-awake": (
        ("*settled = !debris->awake || debris->ticks_slow >= PHYSICS_DEBRIS_REST_TICKS;", "*settled = !debris->awake;"),
        "creeping",
    ),
    "at-rest-at-once": (
        ("\t\t\tdebris->ticks_slow++;", "\t\t\tdebris->ticks_slow = PHYSICS_DEBRIS_REST_TICKS;"),
        "creeping",
    ),
    "slow-forever": (("\t\t\tdebris->ticks_slow = 0;\n", "\t\t\t;\n"), "creeping"),
    "left-the-level": (("if (location.cluster_index != NONE)", "if (TRUE)"), "out of the level"),
    "kept-at-rest": ((" || TEST_FLAG(particle->flags, _particle_datum_at_rest_bit)", ""), "lifetime"),
    "same-index-same-piece": ((" || debris->datum_index != datum_index)", ")"), "lifetime"),
    "kept-before-added": (
        ("physics_debris_free(debris, FALSE);\n\t\treturn FALSE;", "return FALSE;"),
        "gone before added",
    ),
    "no-room-made": (
        ("\t\tif (debris->state == _physics_debris_live)\n\t\t\tdebris->remove_first = TRUE;\n", ""),
        "cap",
    ),
    "newest-evicted": (
        ("candidate->serial - oldest->serial > 0x80000000u", "candidate->serial - oldest->serial < 0x80000000u"),
        "cap",
    ),
    "evicted-not-handed-back": (
        (
            "\t\tphysics_debris_free(debris, TRUE);\n\t}\n\tdebris->datum_index",
            "\t\tphysics_debris_free(debris, FALSE);\n\t}\n\tdebris->datum_index",
        ),
        "cap",
    ),
    "added-under-the-old": (
        ("debris->state != _physics_debris_new || debris->remove_first)", "debris->state != _physics_debris_new)"),
        "cap",
    ),
    "never-refused": (
        ("if (!debris->posed && ++debris->ticks_unposed", "if (FALSE && ++debris->ticks_unposed"),
        "refused",
    ),
    "refused-forgotten": (
        (
            "\t\t/* (the host refused it: the game has it again) */\n\t\tphysics_debris_free(debris, TRUE);",
            "\t\tphysics_debris_free(debris, FALSE);",
        ),
        "refused",
    ),
    "added-twice": (
        ("if (!debris || debris->state != _physics_debris_new", "if (!debris || debris->state == _physics_debris_free"),
        "collided",
    ),
    "attached-garbage": (("object->object.parent_object_index != NONE ||\n\t\t\t", ""), "garbage not debris"),
    "garbage-twice": (("physics_debris_of(_physics_debris_garbage, iterator.index) ||\n\t\t\t", ""), "garbage"),
    "garbage-unscaled": (
        ("half_extents.i = PIN(half_extents.i * nodes[0].scale,", "half_extents.i = PIN(half_extents.i,"),
        "garbage without a hull",
    ),
    "garbage-at-its-origin": (("matrix4x3_transform_point(&nodes[0], &center, &frame.position);\n", ""), "garbage"),
    "garbage-per-tick": (
        (
            "debris->body.velocity.i = object->object.translational_velocity.i * TICKS_PER_SECOND;",
            "debris->body.velocity.i = object->object.translational_velocity.i;",
        ),
        "garbage",
    ),
    "garbage-drawn-unposed": (
        ("if (!debris || !debris->posed || debris->drawn_index", "if (!debris || debris->drawn_index"),
        "garbage drawn",
    ),
    "garbage-uninterpolated": (
        ("||\n\t\trender_interpolation_object_node_matrices(object_index))", ")"),
        "garbage drawn",
    ),
    "garbage-at-its-centre": (
        ("body.position.z = debris->current.position.z - offset.k;", "body.position.z = debris->current.position.z;"),
        "garbage drawn",
    ),
    "garbage-not-carried": (
        ("matrix4x3_multiply(&carry, &nodes[node], &drawn[node]);", "drawn[node] = body;"),
        "garbage drawn",
    ),
}


def generated(fault=None):
    debris = read("port/linux/game/physics_debris.c")
    header = read("port/linux/game/physics_debris.h")
    ragdolls = read("port/linux/game/ragdolls.c")
    state = debris[
        debris.index("/* the pieces of garbage drawn") : debris.index("} physics_debris;") + len("} physics_debris;")
    ]
    config = (
        "\n".join(
            [
                structure(read("port/linux/game/ragdolls.h"), "ragdoll_pose"),
                "#define PHYSICS_DEBRIS_FIRST_ID " + re.search(r"#define PHYSICS_DEBRIS_FIRST_ID (\S+)", header)[1],
                "#define PHYSICS_DEBRIS_MAX " + re.search(r"#define PHYSICS_DEBRIS_MAX (\S+)", header)[1],
                structure(header, "physics_debris_body"),
                "void ragdoll_pose_from_matrix(real_matrix4x3 const *matrix, struct ragdoll_pose *pose);",
                state,
            ]
        )
        + "\n"
    )
    matrix_math = read("source/math/matrix_math.c")
    code = "\n".join(
        [function(matrix_math, name) for name in MATRIX_FUNCTIONS]
        + [function(ragdolls, "ragdoll_pose_from_matrix")]
        + [function(debris, name) for name in FUNCTIONS]
    )
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("physics_debris", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("name", sorted(NEGATIVE_CONTROLS))
def test_negative_control(name):
    fault, case = NEGATIVE_CONTROLS[name]
    status, output = run(build("physics_debris", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_the_particles_call_in():
    """the point-physics update's hooks, each marked `port:`, declared for the game's sources"""
    update = function(read("source/effects/particles.c"), "particle_update_physics")
    assert re.search(
        r"/\* port:[^*]*\*/\n\t\tif \(physics_debris_particle_update\(particle_index, dt, &settled\)\)\n"
        r"\t\t\{\n\t\t\}\n\t\telse if \(particle->object_index == NONE\)",
        update,
    )
    assert re.search(r"/\* port:[^*]*\*/\n\t+physics_debris_particle_collided\(particle_index\);", update)
    fixups = read("port/linux/include/halo_linux_source_fixups.h")
    assert "void physics_debris_particle_collided(long particle_index);" in fixups
    assert (
        "unsigned char physics_debris_particle_update(long particle_index, float dt, unsigned char *settled);" in fixups
    )


def test_the_garbage_calls_in():
    """each tick's garbage made bodies before the batch's, and a piece drawn from its body where a ragdoll would be"""
    jolt = read("port/linux/game/physics_jolt.c")
    update = function(jolt, "physics_jolt_debris_update")
    assert update.index("physics_debris_garbage_update();") < update.index("physics_debris_add(id)")
    drawn = function(read("port/linux/game/ragdolls.c"), "ragdolls_node_matrices")
    assert re.search(r"if \(!ragdoll\)\n\t\treturn physics_debris_node_matrices\(object_index, nodes\);", drawn)


def test_the_bodies_go_with_the_map():
    jolt = read("port/linux/game/physics_jolt.c")
    assert "physics_debris_dispose_from_old_map();" in function(jolt, "physics_jolt_dispose_from_old_map")


def test_the_cap_is_the_hosts():
    """game.physics_debris_max reaches no further than the host's last body"""
    header = read("port/linux/game/physics_debris.h")
    first = int(re.search(r"#define PHYSICS_DEBRIS_FIRST_ID (\d+)", header)[1])
    most = int(re.search(r"#define PHYSICS_DEBRIS_MAX (\d+)", header)[1])
    bodies = int(re.search(r"#define HOST_PHYSICS_BODIES_MAX (\d+)", read("port/macos/include/host_physics.h"))[1])
    assert first + most - 1 <= bodies
    assert "(1 to %d)" % most in read("port/linux/src/port_config.c")
