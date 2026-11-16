"""The real obey_obstacles_clear_way (obey_obstacles.c): an actor that follows a command list takes a go-to whose path
cannot be made as reached, and when none can be the list ends and it stands. In c10's insertion two grunts of a squad,
running its way slower than an optional biped, shut the gap between two rocks as the last one's path was made again,
and it stood as the others ran on. For an actor that obeys, the bipeds ahead that move its way are taken from the
obstacles and the path is made once more; anything else is left as it was.

The real obey_obstacles_follow: in a cinematic, before any path of an actor that obeys is made, the bipeds ahead that
move along its way (within 45 degrees) are taken from its obstacles. In d40's ending flood_2 and flood_3 walked behind
flood_1, were given a point to its side at each path and turned back for their goal once near it: their facing swung
twice a second. One crossing its way, one behind, outside a cinematic, or for an actor that does not obey, is kept."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, inline, mutated, read, run, structure  # noqa: E402

CASES = ["squad ahead", "standing", "behind or coming", "obeys", "does not obey", "no actor",
         "column in a cinematic", "not in a cinematic", "follow does not obey"]

FUNCTIONS = ["obey_obstacles_clears_way", "obey_obstacles_moves_along", "obey_obstacles_remove",
             "obey_obstacles_obeys", "obey_obstacles_take", "obey_obstacles_clear_way", "obey_obstacles_follow"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # any biped moving along, however slowly
    "no-least-speed": (("dot_product3d(velocity, movement_direction) > OBEY_OBSTACLES_MINIMUM_SPEED",
                        "dot_product3d(velocity, movement_direction) >= 0.f"), "standing"),
    # behind it too
    "not-ahead": (("\treturn ahead > 0.f && ", "\treturn ahead > -10.f && "), "behind or coming"),
    # for any actor
    "any-action": (("actor_get(actor_index)->state.action == _actor_action_obey",
                    "actor_get(actor_index)->state.action >= 0"), "does not obey"),
    # any unit, a vehicle too
    "any-unit": (("object_try_and_get_and_verify_type(disc->object_index, _object_mask_biped)",
                  "object_try_and_get_and_verify_type(disc->object_index, _object_mask_unit)"), "obeys"),
    # the optional ones not counted again
    "optional-count": (("\tobstacles->disc_optional_count = optional_count;\n",
                        "\tobstacles->disc_optional_count += 0 * optional_count;\n"), "obeys"),
    # followed outside a cinematic too
    "follow-outside": (("if (!cinematic_in_progress() || ", "if ("), "not in a cinematic"),
    # a biped crossing the way followed too
    "follow-across": (("return along * along > OBEY_OBSTACLES_FOLLOW_ANGLE_COSINE_SQUARED * magnitude_squared3d(velocity);",
                       "return along * along >= 0.f;"), "column in a cinematic"),
    # every biped that clears the way followed, crossing ones too
    "follow-not-along": (("(!along_only || ", "(TRUE || "), "column in a cinematic"),
    # followed for any actor
    "follow-any-action": (("|| !obey_obstacles_obeys(source_object_index))", "|| FALSE)"), "follow does not obey"),
}


def generated(fault=None):
    source = read("port/linux/game/obey_obstacles.c").replace("\r\n", "\n")
    path = read("source/ai/path.h").replace("\r\n", "\n")
    maths = read("source/math/real_math.h").replace("\r\n", "\n")
    config = "\n".join([
        "typedef struct { real x, y; } real_point2d;",
        "#define TICKS_PER_SECOND 30",
        "enum { _object_mask_unit = 1, _object_mask_biped = 2 };",
        "enum { _actor_action_guard = 6, _actor_action_obey = 11 };",
        enum_with(path, "MAXIMUM_DISC_COUNT"),
        enum_with(path, "_disc_optional_bit"),
        structure(path, "obstacle_disc"),
        structure(path, "obstacles"),
        inline(maths, "dot_product3d"),
        inline(maths, "magnitude_squared3d"),
        "#define OBEY_OBSTACLES_MINIMUM_SPEED" +
        source.split("#define OBEY_OBSTACLES_MINIMUM_SPEED", 1)[1].split("\n", 1)[0],
        "#define OBEY_OBSTACLES_FOLLOW_ANGLE_COSINE_SQUARED" +
        source.split("#define OBEY_OBSTACLES_FOLLOW_ANGLE_COSINE_SQUARED", 1)[1].split("\n", 1)[0],
    ]) + "\n"
    code = "\n\n".join(function(source, name) for name in FUNCTIONS) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


def test_path_avoid_obstacles_uses_it():
    """When no way round is found, after the original's search without the optional discs, path_avoid_obstacles asks
    obey_obstacles_clear_way, and with any taken groups the obstacles again and searches once more."""
    avoid = function(read("source/ai/path_obstacle_avoidance.c").replace("\r\n", "\n"), "path_avoid_obstacles")
    optional = avoid.index("if (!path_found && obstacles->disc_optional_count>0)")
    cleared = avoid.index("obey_obstacles_clear_way(obstacles, state->input.ignore_source_object_index, "
                          "start_point, &movement_direction))", optional)
    recomputed = avoid.index("obstacles_recompute(obstacles, radius);", cleared)
    searched = avoid.index("path_found= path_find(obstacle_path,", recomputed)
    assert searched < avoid.index("if (path_found)", searched)
    fixups = read("port/linux/include/halo_linux_source_fixups.h")
    assert "int obey_obstacles_clear_way(struct obstacles *obstacles, long source_object_index," in fixups


def test_path_avoid_obstacles_follows():
    """With the obstacles gathered (and the attractor added), before they are grouped and any path is searched,
    path_avoid_obstacles asks obey_obstacles_follow."""
    avoid = function(read("source/ai/path_obstacle_avoidance.c").replace("\r\n", "\n"), "path_avoid_obstacles")
    gathered = avoid.index("obstacles_get_discs_in_sphere(obstacles, start_point, 4.f, &movement_direction,")
    attractor = avoid.index("&state->input.attractor_point, state->input.attractor_radius);", gathered)
    follow = avoid.index("obey_obstacles_follow(obstacles, state->input.ignore_source_object_index, start_point,",
                         attractor)
    recomputed = avoid.index("obstacles_recompute(obstacles, radius);", gathered)
    assert follow < recomputed < avoid.index("path_found= path_find(obstacle_path,", recomputed)
    fixups = read("port/linux/include/halo_linux_source_fixups.h")
    assert "int obey_obstacles_follow(struct obstacles *obstacles, long source_object_index," in fixups


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("obey_obstacles", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("obey_obstacles", generated(fault)), case)
    assert status == CHECK_FAILED, f"obey_obstacles with a fault ({control}) passed '{case}'"
