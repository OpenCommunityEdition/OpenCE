"""A contrail point added since the points last aged (on a frame between ticks,
or by a projectile's hit) has no state yet: it is drawn in the contrail's first
state, not state -1, which halted a debug build in a network co-op game
("#-1 is not a valid index in [#0,#2)", from render_contrails_normal)."""
import pytest
from tools.harness import build, function, mutated, read, run


def code():
    return function(read("source/render/render_contrails.c"), "contrail_point_state_get")


def execute(source, case):
    return run(build("contrail_point_state", (("under_test.inc", source),)), case)


@pytest.mark.parametrize("case", ["new", "aged"])
def test_contrail_point_state(case):
    status, output = execute(code(), case)
    assert status == 0, output


def test_negative_control():
    source = mutated(code(), "point->state_index == NONE ? 0 : point->state_index", "point->state_index")
    status, output = execute(source, "new")
    assert status == 1 and "check failed" in output, output
