"""The real pelican_lights_tags_loaded, _tags_unloaded and _zbias (pelican_lights.c): the Pelican's flashing light
shader has no decal flag, so its lights were tested against the hull with no depth bias, and the decals' bias of 8
left both light shaders cut through where the hull curves over them as a Pelican flies by (a30's and b30's
extractions). lights flashing is made a decal as the tags load, and a model decal drawn with either shader is biased
32; every other shader keeps its bias."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = ["flagged", "biased", "other maps"]

SOURCE = "port/linux/game/pelican_lights.c"

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the flag not set
    "unflagged": (("\t\t\tSET_FLAG(chicago->flags, PELICAN_LIGHTS_DECAL_BIT, TRUE);\n", ""), "flagged"),
    # the decals' bias kept
    "unbiased": (("\t\t\treturn PELICAN_LIGHTS_ZBIAS;", "\t\t\treturn zbias;"), "biased"),
    # every shader biased
    "any-shader": (("\t\tif (shader == pelican_lights_shaders[index])", "\t\tif (TRUE)"), "biased"),
    # noted after the tags unload
    "kept": (("\t\tpelican_lights_shaders[index] = NULL;\n", ""), "other maps"),
}


def generated(fault=None):
    source = read(SOURCE).replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define PELICAN_LIGHTS_\w+ .*$", source, re.M))
    names = re.search(r"^static char const \*const pelican_lights_shader_names\[\] =\n\{.*?^\};", source,
                      re.M | re.S).group(0)
    shaders = re.search(r"^static struct shader const \*pelican_lights_shaders\[.*?\];", source, re.M).group(0)
    config = defines + "\n" + structure(source, "pelican_lights_chicago") + ";\n"
    code = "\n\n".join([names, shaders] + [function(source, name) for name in
                                           ("pelican_lights_tags_loaded", "pelican_lights_tags_unloaded",
                                            "pelican_lights_zbias")]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


def test_wired():
    """An Xbox map's tags loaded and any map's unloaded call it, and a model's transparent decal takes its bias."""
    cache = read("source/cache/cache_files.c").replace("\r\n", "\n")
    assert cache.index("pelican_lights_tags_loaded();") > cache.index("longsword_exhaust_tags_loaded();")
    assert "pelican_lights_tags_unloaded();" in function(cache, "scenario_tags_unload")
    transparent = read("source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c").replace("\r\n", "\n")
    assert ("shader_is_decal(group->shader) ?\n"
            "\t\t\t\t\t\t\tpelican_lights_zbias(group->shader, rasterizer_debug_options.zbias) :") in transparent


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("pelican_lights", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("pelican_lights", generated(fault)), case)
    assert status == CHECK_FAILED, f"pelican_lights with a fault ({control}) passed '{case}'"
