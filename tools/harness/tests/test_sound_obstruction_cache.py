"""A sound's wall test once a tick: a moving sound's frames reuse it; the next tick, another listener or slot do not."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure
from harness.audio import manager_types

CASES = ['moving', 'next-tick', 'listener', 'slot', 'impulse', 'out-of-range']
NEGATIVE_CONTROLS = {
    'keyed-on-position': ('entry->local_player_index != local_player_index)',
                          'entry->local_player_index != local_player_index ||\n\t\tsource->location.position.x != 2.f)',
                          'moving'),
    'kept-across-ticks': ('entry->game_time != now ||', '(entry->game_time != now && FALSE) ||', 'next-tick'),
    'kept-across-listeners': ('entry->local_player_index != local_player_index)', 'FALSE)', 'listener'),
    'kept-for-another-sound': ('entry->datum_index != datum_index ||', '', 'slot'),
    'tested-out-of-range': ('nearest_distance_squared <= maximum_distance * maximum_distance)', 'TRUE)',
                            'out-of-range'),
}


def generated(fault=None):
    manager = read('source/sound/sound_manager.c')
    types = manager_types()
    types += enum_with(manager, 'MAXIMUM_SOUND_DATUMS =') + '\n'
    types += enum_with(manager, '_sound_obstruction_cache_none') + '\n'
    types += structure(manager, 'sound_obstruction_cache_entry') + '\n'
    tables = re.findall(r'^static struct sound_obstruction_cache_entry \w+\[\w+\];$', manager, re.M)
    assert len(tables) == 2, 'the obstruction tables are not found in sound_manager.c'
    code = '\n'.join(tables) + '\n'
    code += '\n'.join(function(manager, name) for name in [
        'sound_obstruction_cache_entry', 'sound_obstruction_cache_recall', 'sound_obstruction_cache_store',
        'source_audible'])
    if fault:
        code = mutated(code, fault[0], fault[1])
    return (('types.inc', types), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('sound_obstruction_cache', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('control', NEGATIVE_CONTROLS)
def test_negative_control(control):
    before, after, case = NEGATIVE_CONTROLS[control]
    status, output = run(build('sound_obstruction_cache', generated((before, after))), case)
    assert status == CHECK_FAILED, f'{control} escaped {case}: {output}'
