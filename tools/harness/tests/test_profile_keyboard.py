"""Keyboard lifetime and actual Enter mapping, without maps or profile files."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ['done', 'cancel', 'external-close', 'dispose', 'initialize',
         'invalid-name', 'duplicate-name', 'field-owner', 'reopen', 'launch-failure', 'held-enter']


def generated(fault=False):
    vk = read('source/interface/virtual_keyboard.c')
    inputs = read('port/linux/src/xinput_sdl.c')
    config = '\n'.join(enum_with(vk, member) for member in
                       ['VIRTUAL_KEYBOARD_ROW_COUNT', '_vkey_done', '_ui_audio_feedback_none'])
    config += '\n' + '\n'.join(re.findall(r'^#define XINPUT_GAMEPAD_\w+ [^\n]+',
                                         read('port/include/xdk/xdk_xbox.h'), re.M))
    keyboard = function(inputs, 'keyboard_gamepad')
    names = sorted(set(re.findall(r'SDL_SCANCODE_\w+', keyboard + function(inputs, 'typing_gamepad'))))
    config += '\nenum {' + ','.join(names) + ', TEST_SCANCODE_COUNT};\n'
    start = vk.index('static char const virtual_keyboard_layout_table')
    config += vk[start:vk.index('\n};', start)+3] + '\n'
    names = ['text_typing_update', 'platform_text_typing', 'platform_text_field',
             'analog', 'typing_gamepad', 'keyboard_gamepad', 'keys_held_over_switch']
    code = '\n'.join(function(inputs, name) for name in names) + '\n'
    if fault == 'held-enter':
        code, count = re.subn(r' \| \(text_typing \? [24] : 0\)', '', code)
        assert count == 1
    setter = function(vk, 'virtual_keyboard_set_active')
    if fault is True:
        setter = mutated(setter, 'platform_text_typing(active);', 'if (active) platform_text_typing(active);')
    code += setter + '\n'
    code += '\n'.join(function(vk, name) for name in
                      ['virtual_keyboard_initialize', 'virtual_keyboard_dispose',
                       'virtual_keyboard_launch', 'virtual_keyboard_cancel',
                       'virtual_keyboard_close', 'virtual_keyboard_select', 'virtual_keyboard_process'])
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_lifetime(case):
    status, output = run(build('profile_keyboard', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('case', ['done', 'cancel', 'external-close', 'dispose'])
def test_original_sticky_typing_detected(case):
    status, output = run(build('profile_keyboard', generated(True)), case)
    assert status == CHECK_FAILED, output


def test_held_enter_guard_detected():
    status, output = run(build('profile_keyboard', generated('held-enter')), 'held-enter')
    assert status == CHECK_FAILED, output
