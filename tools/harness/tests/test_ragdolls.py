"""Deaths for the ragdolls (ragdolls.c): a hit on a biped remembered for its tick (its direction, the speed of the
damage's instantaneous acceleration as the damage is scaled, its point); a biped with a body that dies queues its node
matrices, its velocity and the hit that killed it, once and no more than there is room for; nothing with
game.ragdolls off, for other objects or for bipeds without a body. The game calls in from object_damage_aftermath and
from unit_died, for a death that is not feigned. With game.dismemberment an explosion that kills a biped or reaches its
ragdoll tears the joints it hurts most past their species' thresholds (each body's own share of them), at most two a
hit, none twice and none on a limb gone already; bullets and melee tear nothing."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = [
    "death",
    "killing hit",
    "newest hit",
    "stale hit",
    "bad numbers",
    "off",
    "not biped",
    "no body",
    "once",
    "capacity",
    "pose",
    "unused at first",
    "start",
    "drive then limp",
    "removed",
    "nearest part",
    "corpse push",
    "killing push",
    "pushes dropped",
    "replaced drops",
    "near",
    "stand-ins",
    "drawn",
    "drawn unposed",
    "blast's damage",
    "falloff",
    "pieces",
    "chance",
    "torn by a blast",
    "past their thresholds",
    "by chance",
    "bullets tear none",
    "tear off",
    "corpse torn",
    "torn once",
    "breaks dropped",
    "bleeds",
    "bloodless",
    "piece draws",
]

FUNCTIONS = [
    "ragdolls_enabled",
    "ragdolls_object_is_biped",
    "ragdoll_pose_transform",
    "ragdoll_part_nearest",
    "ragdolls_push",
    "ragdolls_dismemberment_enabled",
    "ragdoll_hit_damage_at",
    "ragdoll_hit_tearing_at",
    "ragdoll_break_chance",
    "ragdolls_bleed",
    "ragdolls_tear",
    "ragdolls_impulses_drop",
    "ragdolls_damage",
    "ragdolls_hit_get",
    "ragdolls_unit_died",
    "ragdolls_death_count",
    "ragdolls_death_get",
    "ragdolls_deaths_clear",
    "ragdoll_pose_from_matrix",
    "ragdolls_get",
    "ragdolls_of_object",
    "ragdolls_node_matrices",
    "ragdoll_part_piece",
    "ragdolls_piece_count",
    "ragdolls_piece_node_matrices",
    "ragdolls_start",
    "ragdolls_action",
    "ragdolls_stop",
    "ragdolls_impulse_count",
    "ragdolls_impulse_get",
    "ragdolls_impulses_clear",
    "ragdolls_break_count",
    "ragdolls_break_get",
    "ragdolls_breaks_clear",
    "ragdolls_near",
    "ragdolls_stand_ins_begin",
    "ragdolls_stand_in",
    "ragdolls_stand_in_unseen",
    "ragdolls_dispose_from_old_map",
]

MATRIX_FUNCTIONS = [
    "matrix4x3_rotation_to_quaternion",
    "matrix4x3_rotation_from_quaternion",
    "matrix4x3_inverse",
    "matrix4x3_multiply",
    "matrix4x3_transform_point",
]

# a fault in the deaths and the case that must catch it
NEGATIVE_CONTROLS = {
    "always-on": (("if (!ragdolls_enabled() || unit_index == NONE", "if (unit_index == NONE"), "off"),
    "any-object": (
        ("unit_index == NONE || !ragdolls_object_is_biped(unit_index))", "unit_index == NONE)"),
        "not biped",
    ),
    "hits-never-stale": (
        ("hit->object_index == object_index && hit->time == time", "hit->object_index == object_index"),
        "stale hit",
    ),
    "twice": (("if (ragdolls.deaths[index].object_index == unit_index)\n\t\t\treturn;", ""), "once"),
    "no-capacity": (("if (!body || ragdolls.death_count >= RAGDOLL_DEATHS_MAX)", "if (!body)"), "capacity"),
    "no-acceleration-scale": (
        ("object_definition->object.acceleration_scale;", "(object_definition ? 1.0f : 1.0f);"),
        "newest hit",
    ),
    "unnormalised": (("scale_vector3d(&hit->direction, 1.0f / length, &hit->direction);", ""), "killing hit"),
    "no-zero-check": (
        ("if (length > _real_epsilon && hit->speed > _real_epsilon)", "if (length > _real_epsilon)"),
        "bad numbers",
    ),
    "unrotated-shape": (
        (
            "result->x = pose->position.x + point->x + q->w * t.i + (q->v.j * t.k - q->v.k * t.j);",
            "result->x = pose->position.x + point->x;",
        ),
        "nearest part",
    ),
    "farthest-part": (("distance < nearest_distance", "distance > nearest_distance"), "corpse push"),
    "no-lift": (("direction.k += RAGDOLL_PUSH_LIFT;", ""), "corpse push"),
    "no-mass": ((" * ragdoll->body->mass / length", " / length"), "corpse push"),
    "corpse-not-pushed": (
        ("if (ragdoll)\n\t{\n\t\tragdolls_push(", "if (FALSE)\n\t{\n\t\tragdolls_push("),
        "corpse push",
    ),
    "whole-killing-push": (("&death->killing_hit, 0.5f", "&death->killing_hit, 1.0f"), "killing push"),
    "unposed-start": (
        ("ragdoll_pose_from_matrix(&death->nodes[death->body->parts[index].node_index], &ragdoll->poses[index]);", ";"),
        "killing push",
    ),
    "pushes-kept": (("\tragdolls_impulses_drop(id);", ""), "pushes dropped"),
    "no-push-capacity": ((" || ragdolls.impulse_count >= RAGDOLL_IMPULSES_MAX", ""), "pushes dropped"),
    "replaced-pushes-kept": (("ragdolls_impulses_drop(chosen + 1);", ""), "replaced drops"),
    "always-near": (("<= reach * reach", "<= 1e30f"), "near"),
    "stand-in-twice": (
        ("ragdolls.stand_ins[index].used && ragdolls.stand_ins[index].object_index == object_index", "FALSE"),
        "stand-ins",
    ),
    "never-unseen": ((" || ragdolls.stand_ins[id - 1].seen)", ")"), "stand-ins"),
    "never-forgotten": (
        ("ragdolls.stand_ins[index].seen = FALSE;", "ragdolls.stand_ins[index].seen = TRUE;"),
        "stand-ins",
    ),
    "the-game's-turn": (
        (
            "pose->rotation.v.i = -pose->rotation.v.i;\n\tpose->rotation.v.j = -pose->rotation.v.j;\n"
            "\tpose->rotation.v.k = -pose->rotation.v.k;",
            "",
        ),
        "pose",
    ),
    "no-identity": (("\t\tpose->rotation.w = 1.0f;", "\t\tpose->rotation.w = 0.0f;"), "pose"),
    "zero-is-used": (("ragdolls.active[id - 1].used ?", "TRUE ?"), "unused at first"),
    "two-a-biped": (("if (ragdolls_of_object(death->object_index))\n\t\treturn NONE;", ""), "start"),
    "newest-replaced": (
        (
            "ragdolls.active[index].start_time < ragdolls.active[chosen].start_time",
            "ragdolls.active[index].start_time > ragdolls.active[chosen].start_time",
        ),
        "start",
    ),
    "never-limp": (
        ("if (game_time_get() - ragdoll->start_time < RAGDOLL_DRIVE_TICKS)", "if (TRUE)"),
        "drive then limp",
    ),
    "limp-every-tick": (("if (ragdoll->limp)\n\t\treturn _ragdoll_action_none;", ""), "drive then limp"),
    "kept-when-gone": (
        (
            "if (!object_try_and_get_and_verify_type(ragdoll->object_index, _object_mask_biped))\n"
            "\t\treturn _ragdoll_action_remove;",
            "",
        ),
        "removed",
    ),
    "drawn-the-game's-turn": (
        ("game_rotation.v.k = -pose->rotation.v.k;", "game_rotation.v.k = pose->rotation.v.k;"),
        "drawn",
    ),
    "drawn-unscaled": (("frames[part].scale = nodes[part_node].scale;", ""), "drawn"),
    "nodes-left-behind": (
        ("matrix4x3_multiply(&frames[part], &from_part, &drawn[node]);", "drawn[node] = nodes[node];"),
        "drawn",
    ),
    "drawn-before-posed": (
        ("!nodes || !ragdoll->posed || render_interpolation", "!nodes || render_interpolation"),
        "drawn unposed",
    ),
    "not-interpolated": ((" || render_interpolation_object_node_matrices(object_index))", ")"), "drawn"),
    "always-dismember": (("\tif (!ragdolls_dismemberment_enabled())\n\t\treturn;\n", ""), "tear off"),
    "bullets-tear": (("\tif (!hit->explosion)\n\t\treturn 0.0f;\n", ""), "bullets tear none"),
    "every-hit-a-blast": (
        ("hit->explosion = TEST_FLAG(damage->flags, _damage_area_of_effect_bit) &&", "hit->explosion = TRUE ||"),
        "bullets tear none",
    ),
    "melee-explodes": (
        (
            "TEST_FLAG(damage->flags, _damage_area_of_effect_bit) &&\n\t\t(",
            "TEST_FLAG(damage->flags, _damage_area_of_effect_bit) ||\n\t\t(",
        ),
        "blast's damage",
    ),
    "no-closeness": (
        (
            "\treturn ragdoll_hit_damage_at(hit, point) * closeness;",
            "\treturn ragdoll_hit_damage_at(hit, point) + 0.0f * closeness;",
        ),
        "falloff",
    ),
    "unmultiplied": (
        ("damage_effect->damage.damage_minimum * damage->multiplier;", "damage_effect->damage.damage_minimum;"),
        "blast's damage",
    ),
    "no-falloff": (("if (hit->cutoff_radius > hit->falloff_radius)", "if (FALSE)"), "falloff"),
    "always-scaled": (("\tif (!hit->scaled)\n\t\tscale = 1.0f;\n", ""), "falloff"),
    "past-the-cutoff": (("\tif (scale <= 0.0f)\n\t\treturn 0.0f;\n", ""), "falloff"),
    "no-tear-cap": (("count < RAGDOLL_BREAKS_PER_HIT && ", "count >= 0 && "), "torn by a blast"),
    "pieces-whole": (("part > 0 && !TEST_FLAG(ragdoll->broken_joints, part)", "part > 0"), "pieces"),
    "piece-one-up": (("\twhile (part > 0 && !TEST_FLAG", "\tif (part > 0 && !TEST_FLAG"), "pieces"),
    "piece-past-the-parts": (("part < 0 || part >= ragdoll->body->part_count", "part < 0"), "pieces"),
    "no-chance": (("return 0.75f + 0.5f * ", "return 1.0f + 0.0f * "), "chance"),
    "chance-unused": ((" * ragdoll_break_chance(ragdoll->object_index, part))", ")"), "by chance"),
    "least-hurt": (("\t\t\t\tworst_share = share;\n", ""), "torn by a blast"),
    "killing-blast-tears-none": (("\t\tragdolls_tear(chosen + 1, &death->killing_hit);\n", ""), "torn by a blast"),
    "no-threshold": (("real worst_share = 1.0f;", "real worst_share = 0.0f;"), "past their thresholds"),
    "unbreakable-breaks": (
        ("\t\t\tif (body->parts[part].break_damage <= 0.0f)\n\t\t\t\tcontinue;\n", ""),
        "past their thresholds",
    ),
    "corpse-not-torn": (("\t\tragdolls_tear((long)(ragdoll - ragdolls.active) + 1, hit);\n", ""), "corpse torn"),
    "queued-forgotten": (("torn |= 1UL << ragdolls.breaks[index].part;", ";"), "torn once"),
    "limbs-forgotten": (("above = body->parts[above].parent_part_index;", "above = NONE;"), "torn once"),
    "breaks-kept": (("\tragdolls.break_count = kept;\n", ""), "breaks dropped"),
    "bloodless-tears": (
        ("\t\tragdolls_bleed(ragdoll, worst, hit);", "\t\tif (FALSE)\n\t\t\tragdolls_bleed(ragdoll, worst, hit);"),
        "bleeds",
    ),
    "bleeds-at-the-blast": (
        ("marker_points[marker] = ragdoll->poses[part].position;", "marker_points[marker] = hit->epicenter;"),
        "bleeds",
    ),
    "bleeds-toward-the-blast": (
        (
            "marker_forwards[_effect_vector_normal] = away;",
            "scale_vector3d(&away, -1.0f, &marker_forwards[_effect_vector_normal]);",
        ),
        "bleeds",
    ),
    "blood-falls-up": (
        ("marker_forwards[_effect_vector_gravity].k = -1.0f;", "marker_forwards[_effect_vector_gravity].k = 1.0f;"),
        "bleeds",
    ),
    "blood-from-nothing": (
        ("\tif (collision_model->resistance.localized_damage_effect.index == NONE)\n\t\treturn;\n", ""),
        "bloodless",
    ),
    "one-piece": (("broken_joints, part))\n\t\t\tcount++;", "broken_joints, part))\n\t\t\t;"), "piece draws"),
    "pieces-not-collapsed": (("drawn->scale = 0.0f;", "drawn->scale = nodes[node].scale;"), "piece draws"),
    "no-stump": (("if (above == root)", "if (FALSE)"), "piece draws"),
    "stump-from-the-drawn": (
        ("matrix4x3_inverse(&game[parent_node], &inverse);", "matrix4x3_inverse(&nodes[parent_node], &inverse);"),
        "piece draws",
    ),
    "stump-from-interpolation": (
        (
            "game = (real_matrix4x3 const *)object_header_block_get(object_index,\n\t\t&object_get(object_index)->object.node_matrices);",
            "game = object_get_node_matrices(object_index);",
        ),
        "piece draws",
    ),
    "stump-one-up": (("\t\t\ttop = above;\n", "\t\t\ttop = 0;\n"), "piece draws"),
    "torn-drawn-whole": (
        (
            "|| !ragdoll->broken_joints)\n\t\treturn piece == 0 ? nodes : NULL;",
            "|| TRUE)\n\t\treturn piece == 0 ? nodes : NULL;",
        ),
        "piece draws",
    ),
    "pieces-without-end": (("\tif (piece > 0)\n\t\treturn NULL;\n", ""), "piece draws"),
    "oldest-first": (
        ("(ragdolls.hit_count - index) % RAGDOLL_HITS_MAX", "(index - 1) % RAGDOLL_HITS_MAX"),
        "newest hit",
    ),
}


def generated(fault=None):
    ragdolls = read("port/linux/game/ragdolls.c")
    header = read("port/linux/game/ragdolls.h")
    bodies = read("port/linux/game/ragdoll_bodies.h")
    state = ragdolls[ragdolls.index("static struct\n{") : ragdolls.index("} ragdolls;") + len("} ragdolls;")]
    config = (
        "\n".join(
            [
                enum_with(bodies, "RAGDOLL_BODY_PARTS_MAX"),
                structure(bodies, "ragdoll_part"),
                structure(bodies, "ragdoll_body"),
                enum_with(header, "RAGDOLL_DEATHS_MAX"),
                enum_with(header, "_ragdoll_action_none"),
                structure(header, "ragdoll_hit"),
                structure(header, "ragdoll_death"),
                structure(header, "ragdoll_pose"),
                structure(header, "ragdoll_impulse"),
                structure(header, "ragdoll_break"),
                structure(header, "ragdoll"),
                "#define RAGDOLL_DRIVE_FREQUENCY " + re.search(r"#define RAGDOLL_DRIVE_FREQUENCY (\S+)", header)[1],
                "#define RAGDOLL_STAND_IN_DISTANCE " + re.search(r"#define RAGDOLL_STAND_IN_DISTANCE (\S+)", header)[1],
                "#define RAGDOLL_PUSH_LIFT " + re.search(r"#define RAGDOLL_PUSH_LIFT (\S+)", header)[1],
                "struct ragdoll const *ragdolls_of_object(long object_index);",
                "static boolean physics_jolt_ragdolls_enabled(void);",
                "static boolean physics_jolt_dismemberment_enabled(void);",
                "#define RAGDOLL_DAMAGE_EFFECT_UNSCALED_BIT "
                + re.search(r"#define RAGDOLL_DAMAGE_EFFECT_UNSCALED_BIT (\S+)", ragdolls)[1],
                *re.findall(r"#define RAGDOLL_DAMAGE_CATEGORY_\w+ \d+", ragdolls),
                enum_with(ragdolls, "_effect_vector_normal"),
                "static struct ragdoll_body const *ragdoll_body_get(long definition_index);",
                state,
            ]
        )
        + "\n"
    )
    matrix_math = read("source/math/matrix_math.c")
    code = "\n".join(
        [function(matrix_math, name) for name in MATRIX_FUNCTIONS] + [function(ragdolls, name) for name in FUNCTIONS]
    )
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("ragdolls", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("name", sorted(NEGATIVE_CONTROLS))
def test_negative_control(name):
    fault, case = NEGATIVE_CONTROLS[name]
    status, output = run(build("ragdolls", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_the_game_calls_in():
    """the hooks, each marked `port:`, declared for the game's sources"""
    died = function(read("source/units/units.c"), "unit_died")
    not_feigned = died[died.index("if (!feigned)\n\t{\n\t\tobject_set_garbage") :]
    not_feigned = not_feigned[: not_feigned.index("\n\t}\n")]
    assert re.search(r"/\* port:[^*]*\*/\n\t\tragdolls_unit_died\(unit_index\);", not_feigned)
    aftermath = function(read("source/objects/damage.c"), "object_damage_aftermath")
    assert re.search(r"/\* port:[^*]*\*/\n\tragdolls_damage\(object_index, damage\);", aftermath)
    fixups = read("port/linux/include/halo_linux_source_fixups.h")
    assert "void ragdolls_damage(long object_index, struct damage_data const *damage);" in fixups
    assert "void ragdolls_unit_died(long unit_index);" in fixups


def test_deaths_go_with_the_map():
    jolt = function(read("port/linux/game/physics_jolt.c"), "physics_jolt_dispose_from_old_map")
    assert "ragdolls_dispose_from_old_map();" in jolt


def test_ragdolls_go_with_a_saved_game():
    """a load (a revert, a checkpoint) removes every ragdoll, and the host's world is made again with the level"""
    loaded = function(read("port/linux/game/physics_jolt.c"), "physics_jolt_game_state_loaded")
    assert "ragdolls_dispose_from_old_map();" in loaded
    assert loaded.index("HOST_PHYSICS_OP_CLEAR") < loaded.index("physics_jolt_structure_bsp_reconnect();")
    procs = read("source/saved games/game_state.c")
    procs = procs[procs.index("static game_state_after_load_proc after_load_procs[] =") :]
    procs = procs[: procs.index("};")]
    assert re.search(r"/\* port:[^*]*\*/\n\tphysics_jolt_game_state_loaded\n", procs)
    assert "void physics_jolt_game_state_loaded(void);" in read("port/linux/include/halo_linux_source_fixups.h")


def test_drawn_as_the_ragdoll_lies():
    """render_objects.c swaps in the ragdoll's node matrices (but for the first person's legs), and interpolation's
    snapshots are taken of them, so a frame between ticks blends the ragdoll's"""
    listed = function(read("source/render/render_objects.c"), "render_object_list")
    read_at = listed.index("object_get_node_matrices(object_index);")
    assert re.search(
        r"/\* port:[^*]*\*/\n\t\tif \(!legs\)\n\t\t\tnode_matrices = "
        r"ragdolls_node_matrices\(object_index, node_matrices\);",
        listed[read_at:],
    )
    tick = function(read("port/linux/game/render_interpolation.c"), "render_interpolation_tick")
    assert "ragdolls_node_matrices(iterator.index, object_get_node_matrices(iterator.index))" in tick
    assert (
        "struct real_matrix4x3 const *ragdolls_node_matrices(long object_index, struct real_matrix4x3 const *nodes);"
        in read("port/linux/include/halo_linux_source_fixups.h")
    )


def test_drawn_a_piece_at_a_time():
    """render_objects.c draws a torn ragdoll's model once for each piece, and its shadow too, a whole one (and the
    first person's legs) once with the matrices as they are"""
    listed = function(read("source/render/render_objects.c"), "render_object_list")
    assert re.search(
        r"/\* port:[^*]*\*/\n\t+long piece_count = legs \? 1 : ragdolls_piece_count\(object_index\);", listed
    )
    loops = re.findall(
        r"/\* port:[^*]*\*/\n\t+for \(piece = 0; piece < piece_count; piece\+\+\)\n\t+render_model\(\n"
        r"[^;]*piece_count > 1 \? ragdolls_piece_node_matrices\(object_index, node_matrices, piece\) : "
        r"node_matrices,",
        listed,
    )
    assert len(loops) == 2 and listed.count("render_model(") == 2
    fixups = read("port/linux/include/halo_linux_source_fixups.h")
    assert "long ragdolls_piece_count(long object_index);" in fixups
    assert (
        "ragdolls_piece_node_matrices(long object_index, struct real_matrix4x3 const *nodes,\n\tlong piece);" in fixups
    )


def test_the_tick_steps_the_ragdolls():
    """after the objects' update, so a death this tick is a ragdoll this tick, marked `port:`"""
    tick = function(read("source/game/game.c"), "game_tick")
    objects = tick.index("objects_update();)")
    assert re.search(r"/\* port:[^*]*\*/\n\tphysics_jolt_update\(seconds_per_tick\);", tick[objects:])
    assert "void physics_jolt_update(float seconds);" in read("port/linux/include/halo_linux_source_fixups.h")
