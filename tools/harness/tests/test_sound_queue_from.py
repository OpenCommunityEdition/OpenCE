"""A sound queued from a byte into its samples: idle channels only, at a block's start, never the next sound's."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure
from harness.audio import stream_types

CASES = ['from', 'uncompressed', 'next-sound', 'no-channel', 'playing', 'outside']
NEGATIVE_CONTROLS = {
    'kept-for-the-next': ('\tdsound_queue_sample_offset= 0;\n\n\treturn;', '\n\treturn;', 'next-sound'),
    'kept-without-a-channel': ('\tdsound_queue_sample_offset= 0;\n\n\treturn;', '\n\treturn;', 'no-channel'),
    'mid-block': ('dsound_queue_sample_offset-dsound_queue_sample_offset%block_size', 'dsound_queue_sample_offset',
                  'from'),
    'mono-blocks': ('SOUND_COMPRESSED_BLOCK_SIZE*(TEST_FLAG(channel->type_flags, _sound_channel_stereo_bit) ? 2 : 1)',
                    'SOUND_COMPRESSED_BLOCK_SIZE', 'from'),
    'past-the-sound': ('dsound_queue_sample_offset<sound->samples.size', 'TRUE', 'outside'),
}


def generated(fault=None):
    game = read('source/sound/sound_dsound_xbox.c')
    types = stream_types()
    types += structure(read('source/tag_files/tag_groups.h'), 'tag_data') + '\n'
    types += structure(read('source/sound/sound_definitions.h'), 'sound_permutation') + '\n'
    types += enum_with(game, '_sound_channel_3d_bit') + '\n'
    types += f"#define SOUND_COMPRESSED_BLOCK_SIZE {constant(game, 'SOUND_COMPRESSED_BLOCK_SIZE')}\n"
    offset = re.search(r'^static long dsound_queue_sample_offset=.*;$', game, re.M)
    assert offset, 'dsound_queue_sample_offset not found in sound_dsound_xbox.c'
    code = offset.group(0) + '\n'
    code += '\n'.join(function(game, name) for name in ['dsound_channel_queue_sound', 'dsound_virtual_queue_sound',
                                                         'dsound_port_queue_sound_from'])
    if fault:
        code = mutated(code, fault[0], fault[1])
    return (('types.inc', types), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('sound_queue_from', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('control', NEGATIVE_CONTROLS)
def test_negative_control(control):
    before, after, case = NEGATIVE_CONTROLS[control]
    status, output = run(build('sound_queue_from', generated((before, after))), case)
    assert status == CHECK_FAILED, f'{control} escaped {case}: {output}'
