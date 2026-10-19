"""The big-endian reading of an aiming animation's frames (model_animations.c): the table of where each byte of a frame is read from
gives the bytes the original's per-byte walk of the nodes gave, over random animations."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, mutated, read, run

CASES = ['same_bytes', 'new_animation_at_an_old_address', 'beyond_the_data']
CONTROLS = [
    ('map[position] = (unsigned short)(cursor + unit_start + (unit - 1 - (relative - unit_start)));',
     'map[position] = (unsigned short)(cursor + unit_start + (relative - unit_start));', 'same_bytes'),
    ('entry->node_count == animation->node_count && !memcmp(entry->flags, flags, sizeof(flags))',
     'entry->node_count == animation->node_count', 'new_animation_at_an_old_address'),
    ('frame_start + map[position] < animation->data.size', '1', 'beyond_the_data'),
]


def generated(fault=None):
    source = read('source/models/model_animations.c')
    start = source.index('#define AIMING_MAP_ENTRIES')
    code = source[start:source.index('static void aiming_decompress_8byte(', start)]
    if fault:
        code = mutated(code, *fault)
    return (('under_test.inc', code),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('aiming_map', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('aiming_map', generated((before, after))), case)
    assert status != 0, f'{case} passes with the fault'
