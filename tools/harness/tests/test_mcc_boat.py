import pytest
from tools.harness import build, function, read, run, structure


def boat_code():
    return (function(read("port/linux/game/mcc_objects.c"), "mcc_boat_controls_initialize") +
            function(read("source/units/vehicles.c"), "update_human_boat_physics"))


def executable(code):
    state = structure(read("source/physics/powered_mass_point_datum.h"), "powered_mass_point_datum")
    return build("mcc_boat", (("state.inc", state), ("under_test.inc", code)))


@pytest.mark.parametrize("case", ["mcc", "legacy", "other_count"])
def test_boat_controls(case):
    status, output = run(executable(boat_code()), case)
    assert status == 0, output


def test_uninitialized_hydrofoils_regression():
    code = boat_code().replace("mcc_boat_controls_initialize(state);", "")
    status, output = run(executable(code), "mcc")
    assert status == 1, output
