"""The real lens_flare_reflection_coverage (rasterizer_lights.c): a lens flare's reflection smaller than a pixel of the
game's 480 lines is dimmed by as much of one as it covers, as the Xbox drew it now and then on the pixels whose centres
it fell on, not drawn as a steady dot at the display's resolution; one of a pixel or more is drawn as it was."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["small", "large", "split", "degenerate"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # never dimmed
    "never": (("? 1.0f : diameter * diameter", "? 1.0f : 1.0f"), "small"),
    # dimmed by the diameter, not the area it covers
    "linear": (("diameter >= 1.0f || !(diameter >= 0.0f) ? 1.0f : diameter * diameter",
                "diameter >= 1.0f || !(diameter >= 0.0f) ? 1.0f : diameter"), "small"),
    # the window's lines not counted (as though always 480)
    "lines": (("\tdiameter = 2.0f * radius / depth * half_height / tangent;",
               "\tdiameter = 2.0f * radius / depth * 240.0f / tangent;"), "split"),
    # a radius not a number dimmed to nothing
    "nan": (("diameter >= 1.0f || !(diameter >= 0.0f) ? 1.0f : diameter * diameter",
             "diameter >= 1.0f ? 1.0f : diameter * diameter"), "degenerate"),
}


def generated(fault=None):
    source = read("source/rasterizer/rasterizer_lights.c").replace("\r\n", "\n")
    code = function(source, "lens_flare_reflection_coverage") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_drawn_dimmed():
    """each reflection's brightness is dimmed by its coverage, with the radius as it is drawn, before its color."""
    source = read("source/rasterizer/rasterizer_lights.c").replace("\r\n", "\n")
    draw = function(source, "rasterizer_lens_flares_draw")
    dimmed = draw.index("brightness *= lens_flare_reflection_coverage(")
    assert draw.index("real radius = radius_lower_bound+") < dimmed < draw.index("if (reflection->tint_color.alpha == 0.0f &&")
    for flag in ("_lens_flare_reflection_radius_scaled_by_occlusion_bit",
                 "_lens_flare_reflection_radius_not_scaled_by_distance_bit"):
        assert flag in draw[dimmed:draw.index("if (reflection->tint_color.alpha == 0.0f &&")]


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("lens_flare_coverage", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("lens_flare_coverage", generated(fault)), case)
    assert status == CHECK_FAILED, f"lens_flare_reflection_coverage with a fault ({control}) passed '{case}'"
