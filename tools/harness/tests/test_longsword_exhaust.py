"""The real rasterizer_lens_flare_face_light_direction (rasterizer_lights.c), longsword_exhaust.c and the draw's own
fade of a reflection by its light's direction to the camera (rasterizer_lens_flares_draw): the Longswords' exhaust
flare, which its tag fades by no angle, was drawn across the black ships as a pale smear wherever its occlusion test
cleared the top of the hull. It is made to fade by its light's direction to the camera, its direction the ship's way
back: whole from behind, none from ahead or abreast; a flare with a fade of its own is left as it is."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, inline, mutated, read, run, structure  # noqa: E402

CASES = ["faced", "as it is", "the ship's way back", "other maps", "behind only"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the marker's own direction kept, the ship's way forward
    "forward-not-back": (("\tdirection->j = -object->object.forward.j;\n", "\tdirection->j = object->object.forward.j;\n"),
                         "behind only"),
    # the angles set and the cosines the draw reads left
    "cosines-left": (("\tdefinition->runtime_cosine_cutoff_angle = (real)cos(cutoff_angle);\n", ""), "faced"),
    # a reflection scaled by something changed too
    "scaled-changed": (("\t\t\tbrightness_scale_function != _lens_flare_reflection_scale_function_none)\n",
                        "\t\t\tbrightness_scale_function != _lens_flare_reflection_scale_function_none && FALSE)\n"),
                       "as it is"),
    # a flare with a fade of its own changed
    "fade-changed": (("if (definition->falloff_angle != 0.0f || definition->cutoff_angle != 0.0f || ",
                      "if ("), "as it is"),
    # the flare kept after the map's tags unload
    "kept-after-unload": (("\tlongsword_exhaust_flare_index = NONE;\n\n\treturn;\n}", "\treturn;\n}"),
                          "the ship's way back"),
}


def generated(fault=None):
    lights = read("source/rasterizer/rasterizer_lights.c").replace("\r\n", "\n")
    exhaust = read("port/linux/game/longsword_exhaust.c").replace("\r\n", "\n")
    maths = read("source/math/real_math.h").replace("\r\n", "\n")
    draw = function(lights, "rasterizer_lens_flares_draw")
    start = draw.index("cosine_scale = 1.0f/")
    end = draw.index(";", draw.index("scale_functions[_lens_flare_reflection_scale_function_camera_direction] = PIN(")) + 1
    defines = "\n".join(line for line in re.findall(r"^#define LONGSWORD_EXHAUST_\w+ .*$", exhaust, re.M)
                        if "FLARE_TAG" not in line)
    config = "\n".join([
        "typedef unsigned short word;",
        "typedef struct { real i, j; } real_vector2d;",
        "typedef struct { real alpha, red, green, blue; } real_argb_color;",
        "struct tag_block { long count; void *address; long definition; };",
        "struct tag_reference { long group_tag; char *name; long name_length; long index; };",
        "#define TAG_BLOCK_GET_ELEMENT(block, element, type) (&((type *)(block)->address)[(long)(element)])",
        "#define CEILING(a, b) ((a) > (b) ? (b) : (a))",
        "#define PIN(n, f, c) ((n) < (f) ? (f) : CEILING((n), (c)))",
        "#define _pi ((real)3.14159265358979323846f)",
        "#define LONGSWORD_EXHAUST_FLARE_TAG 0",
        defines,
        enum_with(lights, "_lens_flare_reflection_scale_function_none"),
        structure(lights, "lens_flare_definition"),
        structure(lights, "lens_flare_reflection"),
        "#define lens_flare_definition_get(index) (&test_definitions[(index)])",
        "static struct { struct { real_vector3d forward; } camera; } global_window_parameters;",
        inline(maths, "dot_product3d"),
    ]) + "\n"
    fade = ("static real light_to_camera_fade(struct lens_flare_definition *definition, real_vector3d direction,\n"
            "\treal_vector3d eye_to_corona_vector)\n{\n\treal cosine_scale;\n\treal cosine_offset;\n"
            "\treal scale_functions[4];\n\n\t" + draw[start:end] +
            "\n\n\treturn scale_functions[_lens_flare_reflection_scale_function_light_to_camera];\n}\n")
    code = "\n\n".join([function(lights, "rasterizer_lens_flare_face_light_direction"),
                        re.search(r"^static long longsword_exhaust_flare_index = NONE;$", exhaust, re.M).group(0),
                        function(exhaust, "longsword_exhaust_tags_loaded"),
                        function(exhaust, "longsword_exhaust_tags_unloaded"),
                        function(exhaust, "longsword_exhaust_flare_direction"),
                        fade]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


def test_wired():
    """The map's tags loaded (an Xbox map's) and unloaded call it, and a light's flare on a marker is turned by it
    before its direction is stored."""
    cache = read("source/cache/cache_files.c").replace("\r\n", "\n")
    loaded = cache.index("powerup_render_bounds_tags_loaded();")
    assert cache.index("longsword_exhaust_tags_loaded();", loaded) > loaded
    assert "longsword_exhaust_tags_unloaded();" in function(cache, "scenario_tags_unload")
    lights = function(read("source/objects/object_lights.c").replace("\r\n", "\n"), "lights_preprocess_scene")
    turned = lights.index("longsword_exhaust_flare_direction(definition->lens_flare.index, light->object_index, "
                          "&direction);")
    assert lights.index("compress_real_vector3d_to_int32_clamp(&direction);", turned) > turned
    assert lights.index("rasterizer_lens_flare_submit(&lens_flare_parameters);", turned) > turned
    assert '#define LONGSWORD_EXHAUST_FLARE_NAME "vehicles\\\\fighterbomber\\\\fighterbomber exhaust"' in \
        read("port/linux/game/longsword_exhaust.c")


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("longsword_exhaust", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("longsword_exhaust", generated(fault)), case)
    assert status == CHECK_FAILED, f"longsword_exhaust with a fault ({control}) passed '{case}'"
