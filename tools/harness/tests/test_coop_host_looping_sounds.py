"""A co-op host's script looping sounds: where nothing draws (a dedicated server), one that plays once is ended in time."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure

# a fault in network_coop.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    'never-ended': (('if (length > 0 && played >= length + LOOPING_SOUND_END_MARGIN_TICKS &&', 'if (0 &&'),
                    'host_send_looping_sounds', 'ended'),
    'resent-to-the-end': ((' && !(length > 0 && played + LOOPING_SOUND_END_MARGIN_TICKS >= length)', ''),
                          'host_send_looping_sounds', 'not-resent-near-end'),
    'ended-where-drawn': (('debug_no_drawing ? host_looping_sounds[index].length : 0', 'host_looping_sounds[index].length'),
                          'host_send_looping_sounds', 'drawing-host'),
    'alternate-loop-ignored': (('MAX(looping_sound_part_milliseconds(track->loop_sound.index),\n'
                                '\t\t\t\tlooping_sound_part_milliseconds(track->alternate_loop_sound.index))',
                                'looping_sound_part_milliseconds(track->loop_sound.index)'),
                               'looping_sound_play_once_ticks', 'length'),
    'huge-length-kept': (('return PIN(length, 0, MAXIMUM_LOOPING_SOUND_PART_MILLISECONDS);', 'return length;'),
                         'looping_sound_part_milliseconds', 'length'),
    'start-time-kept': (('\t\t\thost_looping_sounds[index].start_time = game_time_get();\n', ''),
                        'network_coop_note_sound', 'started-again'),
    'time-not-sent': (('\t\t\t\tevent->target = played * 1000 / TICKS_PER_SECOND;\n', '\t\t\t\t;\n'),
                      'host_send_looping_sounds', 'resent-time'),
    'music-time-sent': (('if (host_looping_sounds[index].length > 0 && played > 0)', 'if (played > 0)'),
                        'host_send_looping_sounds', 'resent-time'),
}


def generated(fault=None, faulty_function=None):
    coop = read('port/linux/game/network_coop.c')
    sound = read('source/sound/game_sound.c')
    definitions = read('source/sound/sound_definitions.h')
    tags = read('source/tag_files/tag_groups.h')
    types = structure(coop, 'distributed_coop_event') + '\n'
    types += enum_with(coop, '_coop_event_sound') + '\n'
    types += enum_with(read('port/linux/game/network_distributed.h'), '_coop_sound_impulse') + '\n'
    types += enum_with(definitions, '_looping_sound_fake_impulse_sound_bit') + '\n'
    types += enum_with(definitions, 'MAXIMUM_TRACKS_PER_LOOPING_SOUND') + '\n'
    for name in ('tag_block', 'tag_reference'):
        types += structure(tags, name) + '\n'
    for name in ('looping_sound_scale_modifiers', 'looping_sound_track', 'looping_sound_definition'):
        types += structure(definitions, name) + '\n'
    # the host's list of looping sounds, as network_coop.c declares it
    host = re.search(r'^enum\n\{\n\tMAXIMUM_LOOPING_SOUNDS.*?^static short host_looping_sound_count;\n', coop, re.M | re.S)
    assert host, 'host_looping_sounds not found in network_coop.c'
    types += host.group(0)
    code = ''
    for name in ('host_looping_sound_find', 'looping_sound_part_milliseconds', 'looping_sound_play_once_ticks',
                 'host_send_looping_sounds', 'network_coop_note_sound'):
        text = function(coop, name)
        if fault and name == faulty_function:
            text = mutated(text, *fault)
        code += text + '\n'
    for name in ('unattached_looping_sound_stop', 'scripted_looping_sound_stop_internal', 'scripted_looping_sound_stop',
                 'unattached_looping_sound_start', 'scripted_looping_sound_start'):
        code += function(sound, name) + '\n'
    return (('types.inc', types), ('under_test.inc', code))


@pytest.mark.parametrize('case', ['length', 'ended', 'not-resent-near-end', 'drawing-host', 'music', 'no-length',
                                  'started-again', 'queue-full', 'resent-time'])
def test_host_looping_sounds(case):
    status, output = run(build('coop_host_looping_sounds', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('control', NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build('coop_host_looping_sounds', generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f'network_coop.c with a fault ({control}) passed {case!r}: the test cannot see it'
