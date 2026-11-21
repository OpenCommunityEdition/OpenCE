"""The real upscale_pixels, upscale_blur_pass and upscale_sharpen_alpha (hud_hires.c): the message icons' sheets (the
weapons the pickup prompt shows, the buttons and sticks), which have no redraw, were drawn from the map's texels by
interpolation alone, 4 times their size, so their outlines a texel or two wide were soft. They are drawn 8 times their
size and their coverage sharpened by an unsharp mask: an edge steeper, a flat part as it was. hud_hires_upscale_texture
does so for those sheets (kind 1) alone."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["size", "steeper", "flat kept"]

FUNCTIONS = ["upscale_weights", "upscale_pixels", "upscale_blur_pass", "upscale_sharpen_alpha"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # not sharpened
    "unsharpened": (("float value = alpha[index] + UPSCALE_SHARPEN_AMOUNT * (alpha[index] - blurred[index]);",
                     "float value = alpha[index];"), "steeper"),
    # sharpened about one half, as a contrast curve: the flat fill fades
    "contrast": (("float value = alpha[index] + UPSCALE_SHARPEN_AMOUNT * (alpha[index] - blurred[index]);",
                  "float value = (alpha[index] - 127.5f) * 2.5f + 127.5f;"), "flat kept"),
    # the blur's weights not made to sum to one
    "unnormalised": (("kernel[tap] /= total;", "kernel[tap] /= 0.5f * total;"), "flat kept"),
    # the old step, 4 times the size, whatever the factor
    "fixed-step": (("float step = 1.0f / (float)factor;", "float step = 0.25f;"), "size"),
}

SOURCE = "port/linux/src/hud_hires.c"


def generated(fault=None):
    source = read(SOURCE).replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define UPSCALE_\w+ .*$", source, re.M))
    code = "\n\n".join(function(source, name) for name in FUNCTIONS) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", defines + "\n"), ("under_test.inc", code))


def test_wired():
    """The message icons' sheets (kind 1) are drawn 8 times their size and sharpened; the others as before."""
    source = read(SOURCE).replace("\r\n", "\n")
    compute = function(source, "upscale_compute")
    assert "*factor = kind == 1 ? UPSCALE_LINE_ART_FACTOR : UPSCALE_FACTOR;" in compute
    assert "if (kind == 1)\n\t\tupscale_sharpen_alpha(pixels, width * *factor, height * *factor);" in compute
    assert "(GLsizei)(width * factor), (GLsizei)(height * factor)" in function(source, "upscale_upload")
    texture = function(source, "hud_hires_upscale_texture")
    assert "upscale_compute(bgra, width, height, kind, &factor)" in texture
    assert "upscale_upload(pixels, width, height, factor, levels)" in texture
    # (only the line-art sheets, the slow ones, are upscaled on a thread of their own)
    assert "if (kind != 1 || !bgra || !xgpu_gl_ready())" in function(source, "hud_hires_upscale_begin")
    tags = read("port/linux/game/hud_hires_tags.c").replace("\r\n", "\n")
    assert '{ "ui\\\\hud\\\\bitmaps\\\\combined\\\\hud_msg_icons", 0, 3 },' in tags


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("hud_line_art", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("hud_line_art", generated(fault)), case)
    assert status == CHECK_FAILED, f"hud_line_art with a fault ({control}) passed '{case}'"
