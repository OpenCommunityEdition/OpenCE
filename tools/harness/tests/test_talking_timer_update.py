import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run

CASES = ['game', 'turned_on', 'postgame', 'not_multiplayer', 'skip', 'once', 'on_the_second', 'countdown',
         'volume', 'default', 'silent_at_zero']


def generated(fault=None, timer_default=None):
    lines = enum_with(read('port/linux/src/talking_timer_audio.h'), 'TALKING_TIMER_MINUTE_1')
    source = read('port/linux/game/talking_timer.c')
    gain = re.search(r'#define TALKING_TIMER_MAXIMUM_GAIN .*', source).group(0)
    default = timer_default or re.search(r'"audio\.timer_volume", _config_real, "([^"]+)"',
                                         read('port/linux/src/port_config.c')).group(1)
    code = function(source, 'talking_timer_line_at') + '\n' + function(source, 'talking_timer_update')
    if fault:
        code = mutated(code, *fault)
    return (('lines.inc', '%s\n%s\n#define TIMER_VOLUME_DEFAULT %sf\n' % (lines, gain, default)),
            ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('talking_timer_update', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    (('if (watching == NONE)', 'if (0)'), 'turned_on'),
    (('watching = NONE;\n\t\treturn;', 'return;'), 'turned_on'),
    (('!game_engine_in_play()', '0'), 'postgame'),
    (('!game_engine_running()', '0'), 'not_multiplayer'),
    (('if (second != watching)', 'if (1)'), 'once'),
    (('game_time_get() / TICKS_PER_SECOND', '(game_time_get() - 20) / TICKS_PER_SECOND'), 'on_the_second'),
    (('game_time_get() / TICKS_PER_SECOND', '(game_time_get() - 20) / TICKS_PER_SECOND'), 'countdown'),
    ((' * PIN(config_real("audio.timer_volume"), 0.0, 1.0)', ''), 'volume'),
    (('\n\t\t\t* TALKING_TIMER_MAXIMUM_GAIN', ''), 'volume'),
    (('"audio.timer_volume"', '"audio.effects_volume"'), 'volume'),
    (('0.0, 1.0)', '0.0, 2.0)'), 'volume'),
    ((' && volume > 0.0f', ''), 'silent_at_zero'),
])
def test_negative_control(fault, case):
    status, output = run(build('talking_timer_update', generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_negative_control_default():
    status, output = run(build('talking_timer_update', generated(timer_default='1.0')), 'default')
    assert status == CHECK_FAILED, output
