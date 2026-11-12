"""The real clearing of the armored marines' reflection mask under their fingertips (marine_fingers.c): their arms'
multipurpose map has its red, the reflection mask, set under the ends of the hand's fingers, which only the hands'
triangles sample, so the fingertips shone as their armour does. The texels only the hands sample are found from the
model's triangles as a map's tags load, and cleared in the mask's DXT1 blocks each time its pixels load: a block no
other triangle samples has its colors' red cleared (every texel keeping its green, blue and opacity), one another does
has only the hands' texels moved to its least red opaque color."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["whole red", "swapped", "three colors", "shared", "shared three colors", "covered", "marked", "blocks",
         "still"]
FUNCTIONS = ["texels_test", "texels_mark", "triangle_covers_texel", "triangle_mark", "texture_animation_still",
             "level_blocks_add", "dxt1_color_channel", "dxt1_palette_channel", "dxt1_block_clear"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # never cleared (before fix-marine-fingers)
    "not-cleared": (("unsigned short new_first = (unsigned short)(first & 0x07FF);",
                     "unsigned short new_first = first;"), "whole red"),
    # the colors swapped, their indices not
    "indices-kept": (("\t\t\tindices ^= 0x55555555UL;\n", ""), "swapped"),
    # a three-color block's transparent index taken for an opaque one
    "transparent-chosen": (("short opaque = four ? 4 : 3;", "short opaque = 4;"), "shared three colors"),
    # every texel of a shared block moved, the other triangles' too
    "shared-all": (("if (!(texels & (1 << texel)) || (long)current >= opaque)", "if ((long)current >= opaque)"),
                   "shared"),
    # a texel only touching a triangle's edge left out
    "edge-left-out": (("(inside > 0.f && most < -margin)", "(inside > 0.f && most <= 0.f)"), "covered"),
    # no neighbours marked (filtering reads them)
    "no-neighbours": (("for (dy = -1; dy <= 1; dy++)", "for (dy = 0; dy <= 0; dy++)"), "marked"),
    # another triangle's texels taken for the hands'
    "others-ignored": (("if (texels_test(marine_fingers_other_texels, width, x, y))\n\t\t\t\t\tshared = TRUE;\n"
                        "\t\t\t\telse if", "if (0)\n\t\t\t\t\tshared = TRUE;\n\t\t\t\telse if"), "blocks"),
    # any turn taken as still
    "any-turn": (("offsets[2] = animation->r_scale / 360.f;", "offsets[2] = 0.f;"), "still"),
}


def generated(fault=None):
    source = read("port/linux/game/marine_fingers.c").replace("\r\n", "\n")
    cseries = read("source/cseries/cseries.h").replace("\r\n", "\n")
    animation = read("source/shaders/shader_texture_animation.h").replace("\r\n", "\n")
    parts = [
        "enum { LONG_BITS = %d };" % int(re.search(r"\bLONG_BITS = (\d+)", cseries).group(1)),
        "\n".join(re.findall(r"^#define (?:MARINE_FINGERS_\w+|TEXEL_COVER_MARGIN) .*$", source, re.M)),
        enum_with(source, "_marine_fingers_periodic_function_one"),
        structure(animation, "shader_texture_animation"),
        structure(source, "marine_fingers_block"),
        re.search(r"^static struct\n\{.*?^\} marine_fingers_globals = \{ NONE \};", source, re.M | re.S).group(0),
        "\n".join(re.findall(r"^static unsigned long marine_fingers_\w+_texels\[.*\];$", source, re.M)),
    ]
    code = "\n\n".join(parts + [function(source, name) for name in FUNCTIONS]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_the_game_measures_and_clears_it():
    """An Xbox map's tags, as they load, have the mask measured (before the mods' models replace any), and a bitmap's
    pixels are given to it once each time they load, before they are first drawn."""
    cache = function(read("source/cache/cache_files.c").replace("\r\n", "\n"), "scenario_tags_load")
    measured = cache.index("marine_fingers_tags_loaded();")
    assert measured < cache.index("mods_tags_loaded(stripped_scenario_name);", measured)
    texture = function(read("source/cache/xbox_texture_cache.c").replace("\r\n", "\n"),
                       "_texture_cache_bitmap_get_hardware_format")
    first_use = texture.index("if (!texture->used)")
    cleared = texture.index("marine_fingers_bitmap_loaded(bitmap);", first_use)
    assert cleared < texture.index("texture->used = TRUE;", first_use)
    source = read("port/linux/game/marine_fingers.c")
    assert '#define MARINE_MODEL_NAME "characters\\\\marine_armored\\\\marine_armored"' in source
    clear = function(source.replace("\r\n", "\n"), "marine_fingers_bitmap_loaded")
    assert "mod_bitmaps_contain(bitmap)" in clear and "model->geometries.address != marine_fingers_globals.geometries" in clear


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("marine_fingers", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("marine_fingers", generated(fault)), case)
    assert status == CHECK_FAILED, f"the marines' fingers with a fault ({control}) passed '{case}'"
