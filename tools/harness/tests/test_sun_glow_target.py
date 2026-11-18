"""The real surface_is_sun_glow (port/linux/src/d3d8_gl.c): the sun glow's two 64x64 targets are drawn at twice the
screen's scale, and the water's ripples' 64x64 second level stays in its place in their texture, where the water
reads it as it lies farther off (drawn apart, that level kept nothing and the water far off sparkled)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["sun glow", "water", "others"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # any 64x64 A8R8G8B8 surface taken, as before the fix
    "any-level": (("&&\n\t\t(!surface->Parent || surface->Data == surface->Parent->Data)", ""), "water"),
    # a level taken whatever its place in its texture
    "place-ignored": (("surface->Data == surface->Parent->Data", "surface->Parent != NULL"), "water"),
    # a target of no texture refused
    "parentless-refused": (("(!surface->Parent || surface->Data", "(surface->Parent && surface->Data"), "sun glow"),
}


def generated(fault=None):
    source = read("port/linux/src/d3d8_gl.c").replace("\r\n", "\n")
    size = re.search(r"^#define SUN_GLOW_TARGET_SIZE \d+$", source, re.M).group(0)
    code = size + "\n" + function(source, "surface_is_sun_glow") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_drawn_at_its_scale():
    """render_target_get draws a target surface_is_sun_glow takes at twice the screen's scale."""
    source = read("port/linux/src/d3d8_gl.c").replace("\r\n", "\n")
    get = function(source, "render_target_get")
    taken = get.index("surface_is_sun_glow(surface)")
    assert get.index("scale[0] = 2.0f *", taken) > taken


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("sun_glow_target", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sun_glow_target", generated(fault)), case)
    assert status == CHECK_FAILED, f"surface_is_sun_glow with a fault ({control}) passed '{case}'"
