"""The real animation_blend_space.c: a blend from a pose in one space (the object's, or the world's for a
world-relative animation) into an animation of the other is not drawn. In c10's flashback (x50) marine_2 and Bisenti
blended from their standing pose into their world-relative custom animations, and Bisenti back, and flew some 16 to
19 units across the room in 6 ticks. The space is noted for each pose computed and kept as a blend starts from it;
objects.c asks it for every blend, and the game state forgets it at a map's start and before a load."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["into the world's", "back to its own", "within one space", "the pose last computed", "nothing known",
         "forgotten"]

FUNCTIONS = ["animation_blend_space_forget", "animation_blend_space_computed", "animation_blend_space_started",
             "animation_blend_space_crossed"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # never crossed: the original's blend
    "never-crossed": (("animation_blend_spaces[absolute_index].blend_world_relative != (world_relative != FALSE);",
                       "FALSE;"), "into the world's"),
    # the blend's space taken from the new animation, set before the blend starts
    "space-of-the-new": (("animation_blend_spaces[absolute_index].blend_world_relative =\n"
                          "\t\t\tanimation_blend_spaces[absolute_index].computed_world_relative;",
                          "animation_blend_spaces[absolute_index].blend_world_relative = FALSE;"),
                         "back to its own"),
    # crossed whatever the blend's space
    "always-crossed": (("animation_blend_spaces[absolute_index].blend_world_relative != (world_relative != FALSE);",
                        "TRUE;"), "within one space"),
    # an index's entry taken for another object there
    "no-handle": (("\t\tanimation_blend_spaces[absolute_index].blend_object_index == object_index &&\n", ""),
                  "nothing known"),
    # nothing forgotten
    "kept": (("\t\tanimation_blend_spaces[index].blend_object_index = NONE;\n\t}", "\t}"), "forgotten"),
}

SOURCE = "port/linux/game/animation_blend_space.c"


def generated(fault=None):
    source = read(SOURCE).replace("\r\n", "\n")
    start = source.index("static struct\n{\n\tlong computed_object_index;")
    globals_ = source[start:source.index(";", source.index("} animation_blend_spaces")) + 1]
    config = "\n".join([
        "#define MAXIMUM_OBJECTS_PER_MAP 2048",
    ]) + "\n"
    code = globals_ + "\n\n" + "\n\n".join(function(source, name) for name in FUNCTIONS) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


def test_wired():
    """object_start_interpolation notes the blend after keeping the pose; object_compute_node_matrices blends only
    when not crossed, and notes the pose's space after; the game state forgets at a map's start and before a load."""
    objects = read("source/objects/objects.c").replace("\r\n", "\n")
    start = function(objects, "object_start_interpolation")
    kept = start.index("&object->object.original_node_orientations")
    assert start.index("animation_blend_space_started(object_index);", kept) > kept
    compute = objects[objects.index("\t\tif (object->object.animation.interpolation_frame_count>0 &&\n"
                                    "\t\t\t!animation_blend_space_crossed(object_index, world_relative))"):]
    blended = compute.index("interpolate_node_orientations(")
    assert compute.index("animation_blend_space_computed(object_index, world_relative);", blended) > blended
    state = read("source/saved games/game_state.c").replace("\r\n", "\n")
    procs = state[state.index("static game_state_before_load_proc before_load_procs[] ="):]
    assert "\tanimation_blend_space_forget,\n" in procs[:procs.index("};")]
    initialize = function(state, "game_state_initialize_for_new_map")
    assert "animation_blend_space_forget();" in initialize


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("animation_blend_space", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("animation_blend_space", generated(fault)), case)
    assert status == CHECK_FAILED, f"animation_blend_space with a fault ({control}) passed '{case}'"
