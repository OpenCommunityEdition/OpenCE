"""The real hud_hires_sprites_drawable (hud_hires.c): a bitmap's high-res sprites are not drawn while the GL functions
are not loaded (debug.null_renderer, or a context that failed), as the texture's glGenTextures is then NULL."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = ["drawn-with-gl", "not-drawn-without-gl", "reaches-pixels-check-with-gl"]

# a fault in hud_hires_sprites_drawable and the case that must catch it
NEGATIVE_CONTROLS = {
    "no-gl-guard": (("\tif (!glGenTextures)\n\t\treturn 0;\n", ""), "not-drawn-without-gl"),
    "guard-inverted": (("if (!glGenTextures)", "if (glGenTextures)"), "drawn-with-gl"),
}


def generated(fault=None):
    code = function(read("port/linux/src/hud_hires.c"), "hud_hires_sprites_drawable")
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", structure(read("port/linux/src/hud_hires.h"), "hud_hires_embedded") + "\n"),
            ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("hud_hires_sprites", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("hud_hires_sprites", generated(fault)), case)
    assert status == CHECK_FAILED, f"hud_hires.c with a fault ({control}) passed '{case}': the test cannot see it"
