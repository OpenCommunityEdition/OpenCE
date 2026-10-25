import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, mutated, read, run, structure

CASES = ['play', 'volume', 'cut', 'end', 'missing', 'long']


def generated(fault=None):
    lines = enum_with(read('port/linux/src/talking_timer_audio.h'), 'NUMBER_OF_TALKING_TIMER_LINES')
    clip = structure(read('port/linux/src/talking_timer_clips.h'), 'talking_timer_clip')
    source = read('port/linux/src/talking_timer_audio.c')
    code = source[source.index('/* ---------- constants */'):]
    if fault:
        code = mutated(code, *fault)
    return (('lines.inc', lines + '\n' + clip + '\n'), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('talking_timer_audio', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    ((' * talking_timer_gain', ''), 'volume'),
    (('talking_timer_read = 0;', ''), 'cut'),
    (('int idle = !talking_timer_playing;', 'int idle = talking_timer_playing;'), 'cut'),
    (('if (!talking_timer_logged)', 'if (1)'), 'missing'),
    (('(int)(TALKING_TIMER_MAXIMUM_SAMPLES - total)', 'TALKING_TIMER_MAXIMUM_SAMPLES'), 'long'),
])
def test_negative_control(fault, case):
    status, output = run(build('talking_timer_audio', generated(fault)), case)
    assert status == CHECK_FAILED, output
