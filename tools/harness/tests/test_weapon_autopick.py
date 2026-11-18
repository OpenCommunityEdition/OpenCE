"""The real unit_should_autopick_weapon (source/game/players.c): walking over a weapon picks up a first weapon, one
that does not count toward the two (the flag, the ball), capture the flag's flag, and in campaign a second; in
multiplayer a second weapon is the action button's ("Hold X to pick up"), as on the Xbox."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["multiplayer", "campaign", "not counted", "forced"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # a second weapon picked up in multiplayer too
    "multiplayer-second": (("(!game_engine_running() &&\n", "(\n"), "multiplayer"),
    # a second weapon never picked up, campaign's too
    "no-second": (("weapon_count < 2) ||", "weapon_count < 1) ||"), "campaign"),
    # a weapon that does not count toward the two, counted (another flag read)
    "counted": (("_weapon_doesnt_count_toward_maximum_bit)) ||", "_weapon_must_be_picked_up_bit)) ||"),
                "not counted"),
    # the unit's approval not asked
    "unapproved": (("(!game_engine_running() &&\n\t\t\tunit_approve_weapon_pickup(unit_index, weapon_index) &&",
                    "(!game_engine_running() &&"), "campaign"),
}


def generated(fault=None):
    players = read("source/game/players.c").replace("\r\n", "\n")
    code = function(players, "unit_should_autopick_weapon") + "\n"
    if fault:
        code = mutated(code, *fault)
    enums = enum_with(read("source/items/weapon_definitions.h").replace("\r\n", "\n"),
                      "_weapon_doesnt_count_toward_maximum_bit") + "\n"
    return (("under_test_enums.inc", enums), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("weapon_autopick", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("weapon_autopick", generated(fault)), case)
    assert status == CHECK_FAILED, f"unit_should_autopick_weapon with a fault ({control}) passed '{case}'"
