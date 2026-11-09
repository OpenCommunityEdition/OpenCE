"""The real shadow_vector_drawn (render_interpolation.c): an object's lighting turns its shadow vector by a step each
time it is refreshed (every tick, or every 4 or 11) and holds in between, so that a dropship's shadow, cast from high up,
hopped while the dropship glided; the direction drawn glides from step to step as steadily as they come, and a new
object's, one long unseen and a turn no step makes are drawn as the lighting has them."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = ["steady turn", "every tick", "drawn at once"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # drawn as the lighting has it (before fix-shadow-direction-steps)
    "no-glide": (("\tshadow->drawn_at = now;\n\tvector_nlerp(&shadow->from, &shadow->to, "
                  "PIN((now - shadow->changed_at) / shadow->interval, 0.0f, 1.0f), drawn);",
                  "\tshadow->drawn_at = now;\n\t*drawn = *lighting;"), "steady turn"),
    # each step glided over a tick, whenever they come
    "a-tick-each": (("shadow->interval = PIN(now - shadow->changed_at, 1.0f, SHADOW_STEP_MAXIMUM_INTERVAL);",
                     "shadow->interval = 1.0f;"), "steady turn"),
    # a turn no step makes glided too
    "no-jump": (("\t\t\tfrom = *lighting;\n", ""), "drawn at once"),
    # one long unseen glided too
    "unseen-glides": (("now - shadow->drawn_at <= SHADOW_STEP_MAXIMUM_INTERVAL", "TRUE"), "drawn at once"),
}


def generated(fault=None):
    source = read("port/linux/game/render_interpolation.c").replace("\r\n", "\n")
    defines = "".join(re.search(r"^#define " + name + r" .*$", source, re.M).group(0) + "\n"
                      for name in ("SHADOW_STEP_MAXIMUM_INTERVAL", "SHADOW_JUMP_COSINE"))
    code = defines + structure(source, "interpolated_shadow") + "\n"
    code += "".join(function(source, name) + "\n" for name in ("lerp", "vector_length", "vector_nlerp", "shadow_vector_drawn"))
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_shadows_drawn_with_it():
    """Every object's shadow is cast along the direction drawn, not the lighting's own."""
    objects = read("source/render/render_objects.c").replace("\r\n", "\n")
    assert "render_interpolation_shadow_vector(data->object_index, &data->lighting->shadow_vector, &shadow_vector);\n" \
           "\tperpendicular3d(&shadow_vector, &shadow_right);" in objects
    assert "\t\t&shadow_right,\n\t\t&shadow_vector);" in objects


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("shadow_vector_glide", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("shadow_vector_glide", generated(fault)), case)
    assert status == CHECK_FAILED, f"shadow_vector_drawn with a fault ({control}) passed '{case}': the test cannot see it"
