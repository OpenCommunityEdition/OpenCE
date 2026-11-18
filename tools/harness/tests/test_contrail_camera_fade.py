"""A contrail point at the camera (a player's own projectile starts at their eye)
is drawn unfaded, not with the NaN alpha of 0/0 that halted a debug build on the
first shot of a weapon whose contrail fades by the view."""
import pytest
from tools.harness import build, constant, enum_with, function, inline, mutated, read, run


def code():
    return function(read("source/render/render_contrails.c"), "contrail_fade")


def execute(source, case):
    math = read("source/math/real_math.h")
    constants = "\n".join([
        f"#define _real_epsilon {read('source/math/real_math.h').split('#define _real_epsilon ')[1].split()[0]}",
        enum_with(read("source/effects/contrail_definitions.h"), "_contrail_fades_slowly_bit"),
    ])
    vectors = "\n".join(inline(math, name) for name in (
        "vector_from_points3d", "magnitude_squared3d", "magnitude3d", "dot_product3d"))
    return run(build("contrail_camera_fade", (("constants.inc", constants), ("vectors.inc", vectors),
                                              ("under_test.inc", source))), case)


@pytest.mark.parametrize("case", ["at-camera", "away"])
def test_contrail_fade(case):
    status, output = execute(code(), case)
    assert status == 0, output


def test_negative_control():
    source = mutated(code(), "distance > _real_epsilon ?", "1 ?")
    status, output = execute(source, "at-camera")
    assert status == 1 and "check failed" in output, output
