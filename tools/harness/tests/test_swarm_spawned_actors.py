"""The real actors_spawn_from_unit (actors.c) as a unit that spawns actors dies: they join the encounter and squad of
the swarm's actor for a unit of a swarm, of the unit's own actor, or its fake ones; January read a unit of a swarm's
own actor, NONE, and crashed."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["swarm-unit", "own-actor", "no-actor"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # January's: the unit's own actor read, NONE for a unit of a swarm
    "unit-actor": (("actor_get(source_actor_index)", "actor_get(source_unit->unit.actor_index)"), "swarm-unit"),
    # the swarm's actor not chosen: a unit of a swarm spawns nothing
    "swarm-ignored": (("source_unit->unit.swarm_actor_index != NONE ?", "FALSE ?"), "swarm-unit"),
}


def generated(fault=None):
    source = read("source/ai/actors.c")
    config = "\n".join([
        enum_with(read("source/ai/actor_definitions.h"), "_actor_definition_swarm_actor_bit"),
        enum_with(read("source/objects/object_types.h"), "_object_type_biped = 0"),
        enum_with(source, "actor_default_state_alert"),
    ]) + "\n"
    code = function(source, "actors_spawn_from_unit") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("swarm_spawned_actors", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("swarm_spawned_actors", generated(fault)), case)
    assert status == CHECK_FAILED, f"actors_spawn_from_unit with a fault ({control}) passed '{case}': the test cannot see it"
