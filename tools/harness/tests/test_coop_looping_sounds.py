"""A co-op client's script looping sounds: one that plays once (a cutscene's) is not restarted by the host's resends."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure


def generated(fault=None):
    coop = read('port/linux/game/network_coop.c')
    sound = read('source/sound/game_sound.c')
    types = structure(coop, 'distributed_coop_event') + '\n'
    types += enum_with(read('port/linux/game/network_distributed.h'), '_coop_sound_impulse') + '\n'
    types += enum_with(read('source/sound/sound_definitions.h'), '_looping_sound_fake_impulse_sound_bit') + '\n'
    code = ''
    for name in ('unattached_looping_sound_stop', 'scripted_looping_sound_stop_internal', 'scripted_looping_sound_stop',
                 'unattached_looping_sound_start', 'scripted_looping_sound_start'):
        code += function(sound, name) + '\n'
    code += 'static struct {long definition_indices[MAXIMUM_LOOPING_SOUNDS];short count;} client_looping_sounds;\n'
    for name in ('client_looping_sound_find', 'client_looping_sound_started', 'client_looping_sound_forget'):
        code += function(coop, name) + '\n'
    apply = function(coop, 'client_apply_sound')
    if fault == 'resent':
        apply = mutated(apply, ' &&\n\t\t\tclient_looping_sound_find(event->tag_index) == NONE)', ')')
    if fault == 'kept':
        apply = mutated(apply, '\t\t\tclient_looping_sound_forget(event->tag_index);\n', '')
    if fault == 'any-time':
        apply = mutated(apply, ' ? event->target : NONE', ' ? event->target : event->target')
    if fault == 'kept-time':
        apply = mutated(apply, '\t\t\tcoop_sound_sync_end(event->tag_index);\n', '')
    return (('types.inc', types), ('under_test.inc', code + apply))


@pytest.mark.parametrize('case', ['played-out', 'playing', 'started-again', 'late-joiner', 'music', 'no-room', 'stopped',
                                  'joined-during', 'music-time', 'just-started'])
def test_looping_sounds(case):
    status, output = run(build('coop_looping_sounds', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [('resent', 'played-out'), ('resent', 'late-joiner'), ('kept', 'started-again'),
                                        ('any-time', 'music-time'), ('kept-time', 'joined-during')])
def test_negative(fault, case):
    status, output = run(build('coop_looping_sounds', generated(fault)), case)
    assert status == CHECK_FAILED, output
