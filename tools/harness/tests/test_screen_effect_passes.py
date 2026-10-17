"""The real pass count and radii of a convolution screen effect (rasterizer_xbox_screen_effect.c): a scope's zoom draws
few full-screen passes at any resolution, its copies of the screen blend rather than ghost, and its blur stays as wide
as the Xbox's."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["stock-size-unchanged", "pass-radii", "few-passes", "no-ghosts", "blur-as-wide"]
FUNCTIONS = ("rasterizer_screen_effect_refinement_passes", "rasterizer_screen_effect_pass_radius")

# a fault in rasterizer_xbox_screen_effect.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "halves-not-thirds": (("radius /= 3.0f", "radius /= 2.0f"), "rasterizer_screen_effect_pass_radius", "no-ghosts"),
    "first-refinement-full-radius": (("refinement <= pass", "refinement < pass"),
                                     "rasterizer_screen_effect_pass_radius", "pass-radii"),
    "never-refines": (("passes += 2", "passes += 0"),
                      "rasterizer_screen_effect_refinement_passes", "no-ghosts"),
    "refines-at-stock-size": (("screen_scale * spacing > 1.0f", "screen_scale * spacing >= 1.0f"),
                              "rasterizer_screen_effect_refinement_passes", "stock-size-unchanged"),
    "unbounded": (("passes < 4", "passes < 12"), "rasterizer_screen_effect_refinement_passes", "few-passes"),
    "odd-count": (("passes += 2", "passes += 1"), "rasterizer_screen_effect_refinement_passes", "few-passes"),
}


def generated(fault=None, faulty_function=None):
    source = read("source/rasterizer/xbox/rasterizer_xbox_screen_effect.c")
    text = ""
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("under_test.inc", text),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("screen_effect_passes", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("screen_effect_passes", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the passes with a fault ({control}) passed '{case}': the test cannot see it"
