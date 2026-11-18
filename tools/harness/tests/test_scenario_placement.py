"""The real scenario_placement_withheld_by_name (scenario_placement.c): a10's Johnson, whom only its opening cinematic
uses (and creates), is not placed as a local game of a10 loads, so that a player who skips the cinematic doesn't find
him standing in a hallway; in a network game, where the objects placed as a map loads must be alike on every machine,
he is, as is every other object."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["a10 local", "a10 network", "anything else"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # placed (before fix-a10-skipped-intro-johnson)
    "placed": (("return connection == _game_connection_local &&", "return FALSE &&"), "a10 local"),
    # held back in network games too
    "network-too": (("return connection == _game_connection_local &&", "return connection >= _game_connection_local &&"),
                    "a10 network"),
    # any object of a10
    "any-name": (('object_name && !csstrcmp(object_name, "johnson") &&', "object_name &&"), "anything else"),
}


def generated(fault=None):
    source = read("port/linux/game/scenario_placement.c").replace("\r\n", "\n")
    code = function(source, "scenario_placement_withheld_by_name") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_placement_asks_it():
    """A map's objects placed as it loads are asked of scenario_placement_withheld; ones created later are not."""
    objects = read("source/objects/objects.c").replace("\r\n", "\n")
    placement = function(objects, "object_new_from_scenario")
    assert "!object_globals->initial_placement ||\n" \
           "\t\t\t(!TEST_FLAG(scenario_object->placement_flags, _scenario_object_placement_not_automatic_bit) &&" in placement
    assert "!scenario_placement_withheld(scenario_object, palette))" in placement


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("scenario_placement", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("scenario_placement", generated(fault)), case)
    assert status == CHECK_FAILED, f"scenario_placement_withheld_by_name with a fault ({control}) passed '{case}'"
