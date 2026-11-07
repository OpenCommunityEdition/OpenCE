"""The real rasterizer_screen_effect_set_texture_transforms (rasterizer_xbox_screen_effect.c): the video effect's mask
is laid with its middle 640 columns, the Xbox's screen, across the whole width of a wider screen, its rows one a line,
and the other effects' textures are laid as before."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["fills-a-wide-screen", "as-the-xbox-at-640", "other-effects-as-before"]

# a fault in rasterizer_screen_effect_set_texture_transforms and the case that must catch it
NEGATIVE_CONTROLS = {
    # laid a texel a column on a wide screen too, as before
    "a-texel-a-column": (("viewport_map.width > 640)", "FALSE)"), "fills-a-wide-screen"),
}


def generated(fault=None):
    source = read("source/rasterizer/xbox/rasterizer_xbox_screen_effect.c")
    config = "\n".join([
        structure(read("source/bitmaps/bitmap_group.h"), "bitmap_data"),
        structure(read("source/rasterizer/rasterizer_cinematics.h"), "rasterizer_cinematic_screen_effect_parameters"),
        enum_with(source, "_rasterizer_screen_effect_convolution_type_none"),
        enum_with(source, "_bitmap_linear_bit"),
    ]) + "\n"
    code = function(source, "vector2d_reciprocal") + "\n"
    texture_transforms = function(source, "rasterizer_screen_effect_set_texture_transforms")
    if fault:
        texture_transforms = mutated(texture_transforms, *fault)
    code += texture_transforms + "\n"
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("screen_effect_video_mask", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("screen_effect_video_mask", generated(fault)), case)
    assert status == CHECK_FAILED, f"the mask with a fault ({control}) passed '{case}': the test cannot see it"
