"""The real crewman_hands_turn, _node_turn, _deepest, _arm_lift and _postprocess_node_matrices (crewman_hands.c):
a seated crewman (a10's bridge crew at their desks, its pilots) had his forearms and hands in the desk he works, as
his poses were made; each tick his elbow is bent, or for a forearm his arm raised at the shoulder, until his
forearm's and hand's vertices are out of what the line from his shoulder to them crosses, by a clearance, no more
than 30 degrees a joint, and not for a vertex past something thin."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = ["lifted", "untouched", "shoulder", "capped", "who"]

SOURCE = "port/linux/game/crewman_hands.c"

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the hand left where it was
    "hand-left": (("\t\tcrewman_hands_node_turn(&node_matrices[arm->hand_node_index], &pivot, &axis, "
                   "sine_value, cosine_value);\n", ""), "lifted"),
    # bent the wrong way
    "wrong-way": (("\t\tangle = MIN(", "\t\tangle = -MIN("), "lifted"),
    # no clearance
    "touching": (("(depth + CREWMAN_HANDS_CLEARANCE) / reach", "depth / reach"), "lifted"),
    # a vertex past something thin taken as in it
    "any-depth": ((" && depth < CREWMAN_HANDS_MAXIMUM_DEPTH", ""), "untouched"),
    # no most a joint bends
    "uncapped": (("real bendable = CREWMAN_HANDS_MAXIMUM_ANGLE - ", "real bendable = 1000.f - "), "capped"),
    # the forearm lifted by the elbow too
    "elbow-only": (("boolean at_shoulder = node_index == arm->forearm_node_index;",
                    "boolean at_shoulder = FALSE;"), "shoulder"),
    # every crewman measured, standing too
    "anyone": (("		object->object.parent_object_index == NONE)", "		FALSE)"), "who"),
}


def generated(fault=None):
    source = read(SOURCE).replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define CREWMAN_HANDS_\w+(?:.*\\\n)*.*$", source, re.M))
    arms = re.search(r"^enum\n\{\n\t_crewman_hands_right_arm.*?^\};", source, re.M | re.S).group(0)
    globals_ = "\n".join(re.findall(r"^static (?:long|short|struct crewman_hands_\w+) crewman_hands_\w+(?:\[\w+\])*"
                                    r"(?: = NONE)?;$", source, re.M))
    config = "\n".join([defines, arms, structure(source, "crewman_hands_arm") + ";",
                        structure(source, "crewman_hands_vertex") + ";", globals_]) + "\n"
    code = "\n\n".join(function(source, name) for name in
                       ("crewman_hands_turn", "crewman_hands_node_turn", "crewman_hands_deepest",
                        "crewman_hands_arm_lift", "crewman_hands_postprocess_node_matrices")) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


def test_wired():
    """Every unit's node matrices pass through it once the unit's own are made, and the tags load and unload it."""
    objects = read("source/objects/objects.c").replace("\r\n", "\n")
    postprocess = function(objects, "object_postprocess_node_matrices")
    assert postprocess.index("crewman_hands_postprocess_node_matrices(object_index, matrices);") > \
        postprocess.index("object_type_postprocess_node_matrices(object_index, matrices);")
    cache = read("source/cache/cache_files.c").replace("\r\n", "\n")
    assert cache.index("crewman_hands_tags_loaded();") > cache.index("powerup_render_bounds_tags_loaded();")
    assert "crewman_hands_tags_unloaded();" in function(cache, "scenario_tags_unload")


def test_generated_globals():
    """The model, arms, vertices and counts the code reads are all taken from the sources."""
    config = generated()[0][1]
    for name in ("crewman_hands_model_index", "crewman_hands_arms", "crewman_hands_vertices",
                 "crewman_hands_vertex_counts", "CREWMAN_HANDS_COLLISION_FLAGS", "NUMBER_OF_CREWMAN_HANDS_ARMS"):
        assert name in config, name


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("crewman_hands", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("crewman_hands", generated(fault)), case)
    assert status == CHECK_FAILED, f"crewman_hands with a fault ({control}) passed '{case}'"
