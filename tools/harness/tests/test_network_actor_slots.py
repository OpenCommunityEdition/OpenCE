"""The AI units' tables of network_actors.c, found by a hint per unit: over random lookups, removals and new games,
the same entries a search of the table finds, in the same order, and each unit's hint at its entry."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run

CASES = ['host', 'client']
CONTROLS = [
    # a hint taken without checking it holds the unit
    ('index < count && ACTOR_TABLE_UNIT(index) == unit_index', 'index < count', 'host'),
    # a hint past the table's end (a removed entry's old place)
    ('index >= 0 && index < count && ', 'index >= 0 && ', 'host'),
    # a moved entry's hint left where it was
    ('host_actors[index] = host_actors[--host_actor_count];\n\tactor_slot_set(host_actor_slots, host_actors[index].unit_index, index);',
     'host_actors[index] = host_actors[--host_actor_count];', 'host'),
    ('client_actors[index] = client_actors[--client_actor_count];\n\tactor_slot_set(client_actor_slots, client_actors[index].unit_index, index);',
     'client_actors[index] = client_actors[--client_actor_count];', 'client'),
    # a new entry's hint not set
    ('actor_slot_set(client_actor_slots, unit_index, client_actor_count);\n', '', 'client'),
]


def generated(fault=None):
    source = read('port/linux/game/network_actors.c').replace('\r\n', '\n')
    code = '\n'.join(function(source, name) for name in
                     ['actor_slot_set', 'actor_table_find', 'host_actor_for', 'client_actor_for', 'host_actor_remove',
                      'client_actor_remove'])
    if fault:
        code = mutated(code, *fault)
    return (('under_test.inc', code),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('network_actor_slots', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('network_actor_slots', generated((before, after))), case)
    assert status == CHECK_FAILED, output
