import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run

CASES = ['seconds', 'countdown', 'minutes', 'silent']


def generated(fault=None):
    lines = enum_with(read('port/linux/src/talking_timer_audio.h'), 'TALKING_TIMER_MINUTE_1')
    code = function(read('port/linux/game/talking_timer.c'), 'talking_timer_line_at')
    if fault:
        code = mutated(code, *fault)
    return (('lines.inc', lines + '\n'), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('talking_timer', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    (('within == 30', 'within == 31'), 'seconds'),
    (('within == 40', 'within == 20'), 'seconds'),
    (('within >= 50', 'within >= 51'), 'countdown'),
    (('second / 60 <= 30', 'second / 60 <= 31'), 'minutes'),
    (('second / 60 <= 30', 'second / 60 <= 29'), 'minutes'),
    (('second / 60 <= 30', 'second / 60 > 0'), 'minutes'),
    (('if (second / 60 <= 30)\n\t\t\tline = TALKING_TIMER_MINUTE_1 + second / 60 - 1;',
      'line = TALKING_TIMER_MINUTE_1 + (second / 60 - 1) % 30;'), 'minutes'),
    (('second / 60 - 1', 'second / 60'), 'minutes'),
    (('if (second <= 0)', 'if (second < 0)'), 'silent'),
])
def test_negative_control(fault, case):
    status, output = run(build('talking_timer', generated(fault)), case)
    assert status == CHECK_FAILED, output
