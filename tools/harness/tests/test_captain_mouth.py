"""The real lower lip levelling of captain_mouth.c: Captain Keyes's model holds the right half of his lower lip 2 mm
lower and 0.8 mm further back than the left. Each frame of his emotions moves the half's node back to the left's
level, so the lip sags only in a pose with no emotion: there the node is moved as the emotions move it. A pose with an
emotion, one that moves the node itself, another model's and another map's are left as they are, and the model's
vertices are never changed (moved, they stood 2 mm too high under every emotion: in the cutscenes)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = ["measured", "level", "not a lip", "as the emotions", "no emotion", "emotion", "posed", "another model",
         "another map"]

FUNCTIONS = ["lower_lip_half_add", "lower_lip_offset", "lower_lip_node_level", "captain_mouth_node_orientations"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # never measured as a lip to level (before fix-captain-lower-lip)
    "not-levelled": (("return length >= LOWER_LIP_MINIMUM_OFFSET &&", "return FALSE &&"), "measured"),
    # across too, toward the left half
    "across": (("offset->j = 0.f;", "offset->j = left->sum.y / (real)left->count - right->sum.y / (real)right->count;"),
               "measured"),
    # levelled again, however little it is off
    "no-minimum": (("return length >= LOWER_LIP_MINIMUM_OFFSET &&", "return length >= 0.f &&"), "level"),
    # halves of different counts measured all the same
    "uneven": (("if (left->count <= 0 || left->count != right->count)", "if (left->count <= 0 || right->count <= 0)"),
               "not a lip"),
    # the node moved under an emotion too, whose frame has levelled the lip (as moving the vertices did)
    "under-emotions": (("if (captain_mouth_model_index != NONE && !emotion_applied)",
                        "if (captain_mouth_model_index != NONE)"), "emotion"),
    # never moved
    "not-moved": (("orientation->translation.y += node_offset->j;", ""), "no emotion"),
    # a pose that moves the node itself moved as well
    "posed-too": (("> LOWER_LIP_HELD_DISTANCE * LOWER_LIP_HELD_DISTANCE)", "< 0.f)"), "posed"),
    # every unit's node 33
    "any-model": (("if (definition->object.model.index == captain_mouth_model_index &&\n", "if (\n"), "another model"),
    # the index of the last map this looked at, whatever tag has it now
    "stale-index": ((" &&\n\t\t\t!csstrcmp(tag_get_name(captain_mouth_model_index), CAPTAIN_MODEL_NAME))", ")"),
                    "another map"),
}


def generated(fault=None):
    source = read("port/linux/game/captain_mouth.c").replace("\r\n", "\n")
    types = read("source/rasterizer/rasterizer_model_types.h").replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define (?:LOWER_LIP_\w+|MODEL_VERTEX_WHOLE_WEIGHT|CAPTAIN_MODEL_NAME) .*$", source,
                                   re.M))
    state = "\n".join(re.findall(r"^static [^(\n]*;$", source, re.M))
    transform = function(read("source/math/matrix_math.c").replace("\r\n", "\n"), "matrix4x3_transform_vector")
    code = "\n\n".join([structure(types, "model_vertex_compressed"), structure(source, "lower_lip_half"), defines,
                        state, transform] + [function(source, name) for name in FUNCTIONS]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_maps_measure_it_and_poses_are_levelled():
    """An Xbox map's tags, as they load, have the captain's lip measured before the mods' models replace any; a
    unit's pose is levelled once its emotion's frame has or has not been laid over it, before its mouth opens; and
    the model's vertices are only read."""
    cache = read("source/cache/cache_files.c").replace("\r\n", "\n")
    load = function(cache, "scenario_tags_load")
    assert "captain_mouth_tags_loaded();" in load
    assert load.index("captain_mouth_tags_loaded();") < load.index("mods_tags_loaded(stripped_scenario_name);",
                                                                     load.index("captain_mouth_tags_loaded();"))
    source = read("port/linux/game/captain_mouth.c").replace("\r\n", "\n")
    assert '#define CAPTAIN_MODEL_NAME "characters\\\\captain\\\\captain"' in source
    lock = function(source, "model_part_vertices_lock")
    assert "D3DLOCK_READONLY" in lock and "position" not in function(source, "model_lower_lip_measure")
    units = function(read("source/units/units.c").replace("\r\n", "\n"), "unit_preprocess_node_orientations")
    emotion = units.index("unit->unit.animation.emotion_index,\n\t\t\t\t\t\tnode_orientations);\n"
                          "\t\t\t\t\temotion_applied = TRUE;")
    levelled = units.index("captain_mouth_node_orientations(unit_index, emotion_applied, node_orientations);", emotion)
    mouth = units.index("unit->unit.mouth_aperture,", levelled)
    assert units.index("boolean emotion_applied = FALSE;") < emotion < levelled < mouth
    assert units.count("emotion_applied = TRUE;") == 1


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("captain_mouth", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("captain_mouth", generated(fault)), case)
    assert status == CHECK_FAILED, f"the lip levelling with a fault ({control}) passed '{case}'"
