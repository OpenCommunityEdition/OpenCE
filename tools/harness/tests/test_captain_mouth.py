"""The real lower lip levelling of captain_mouth.c: Captain Keyes's model has the right half of his lower lip 2 mm lower
and 0.8 mm further back than the left, so his lips never meet on his right and he talks with a lopsided grin. As a
map's tags load, the right half is moved to the left half's level, each vertex as far as its weight on the right
half's node; a lip already level, or not one to level, is left as it is."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = ["captain", "level", "not a lip", "weights"]

FUNCTIONS = ["model_vertex_node_weight", "lower_lip_half_add", "lower_lip_offset", "lower_lip_right_half_move"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # never levelled (before fix-captain-lower-lip)
    "not-levelled": (("return length >= LOWER_LIP_MINIMUM_OFFSET &&", "return FALSE &&"), "captain"),
    # the whole offset, whatever the weight: the middle of the lip moved as far as the right half
    "whole-offset": (("vertex->position.z += offset->k * weight;", "vertex->position.z += offset->k;"), "captain"),
    # moved across too, toward the left half
    "across": (("offset->j = 0.f;", "offset->j = left->sum.y / (real)left->count - right->sum.y / (real)right->count;"),
               "captain"),
    # levelled again, however little it is off
    "no-minimum": (("return length >= LOWER_LIP_MINIMUM_OFFSET &&", "return length >= 0.f &&"), "level"),
    # halves of different counts measured all the same
    "uneven": (("if (left->count <= 0 || left->count != right->count)", "if (left->count <= 0 || right->count <= 0)"),
               "not a lip"),
    # the second node's weight taken as the first's
    "second-weight": (("weight += 1.f - first_weight;", "weight += first_weight;"), "weights"),
}


def generated(fault=None):
    source = read("port/linux/game/captain_mouth.c").replace("\r\n", "\n")
    types = read("source/rasterizer/rasterizer_model_types.h").replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define (?:LOWER_LIP_\w+|MODEL_VERTEX_WHOLE_WEIGHT) .*$", source, re.M))
    code = "\n\n".join([structure(types, "model_vertex_compressed"), structure(source, "lower_lip_half"), defines] +
                       [function(source, name) for name in FUNCTIONS]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_maps_level_it():
    """An Xbox map's tags, as they load, have the captain's lip levelled before the mods' models replace any."""
    cache = read("source/cache/cache_files.c").replace("\r\n", "\n")
    load = function(cache, "scenario_tags_load")
    assert "captain_mouth_tags_loaded();" in load
    assert load.index("captain_mouth_tags_loaded();") < load.index("mods_tags_loaded(stripped_scenario_name);",
                                                                     load.index("captain_mouth_tags_loaded();"))
    source = read("port/linux/game/captain_mouth.c")
    assert '#define CAPTAIN_MODEL_NAME "characters\\\\captain\\\\captain"' in source


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("captain_mouth", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("captain_mouth", generated(fault)), case)
    assert status == CHECK_FAILED, f"the lip levelling with a fault ({control}) passed '{case}'"
