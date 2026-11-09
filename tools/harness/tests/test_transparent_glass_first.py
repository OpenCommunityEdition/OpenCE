"""The real group_sorted_indices_cmpfn and transparent_object_glass_first (rasterizer_transparent_geometry.c): a model's
transparent parts tie at its object's centroid, and qsort drew a powerup's glass box before or after the glow inside it
as the view fell (after it, the over shield's bright red glow dimmed to purple); of one object, its glass is now drawn
first, whatever order the groups come in, and depth still comes first."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["glass first", "depth first", "no glass"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # tied as before (before fix-powerup-glass-order)
    "tied": (("\t\t\t\t\tcomparison = transparent_object_glass_first(group1, group2);\n", ""), "glass first"),
    # the glass last
    "glass-last": (("return glass1 ? -1 : 1;", "return glass1 ? 1 : -1;"), "glass first"),
    # glass first whatever the depth
    "glass-over-depth": (("\t\t\tif (group1->z_sort>group2->z_sort)\n\t\t\t{\n\t\t\t\tcomparison = 1;\n\t\t\t\tgoto cortana_tiebreak;\n\t\t\t}\n",
                          "\t\t\tif ((comparison = transparent_object_glass_first(group1, group2)) != 0)\n\t\t\t\tgoto cortana_tiebreak;\n"
                          "\t\t\tif (group1->z_sort>group2->z_sort)\n\t\t\t{\n\t\t\t\tcomparison = 1;\n\t\t\t\tgoto cortana_tiebreak;\n\t\t\t}\n"),
                         "depth first"),
}


def generated(fault=None):
    source = read("source/rasterizer/rasterizer_transparent_geometry.c").replace("\r\n", "\n")
    code = "enum {\n" + "".join(re.search(r"^\t" + name + r" = \d+,.*$", source, re.M).group(0) + "\n" for name in (
        "_shader_type_transparent_water", "_shader_type_transparent_glass", "_rasterizer_geometry_first_person_bit")) + "};\n"
    code += function(source, "transparent_object_glass_first") + "\n"
    code += function(source, "group_sorted_indices_cmpfn") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_glass_is_the_games():
    """The glass type is the game's (shaders' eighth: rasterizer_xbox_transparent_geometry.c's list)."""
    xbox = read("source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c").replace("\r\n", "\n")
    types = re.search(r"enum\n\{\n\t_shader_type_screen = 0,\n(.*?)NUMBER_OF_SHADER_TYPES", xbox, re.S).group(1)
    names = ["_shader_type_screen"] + re.findall(r"\t(_shader_type_\w+),", types)
    assert names.index("_shader_type_transparent_glass") == 8


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("transparent_glass_first", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("transparent_glass_first", generated(fault)), case)
    assert status == CHECK_FAILED, f"the transparent sort with a fault ({control}) passed '{case}': the test cannot see it"
