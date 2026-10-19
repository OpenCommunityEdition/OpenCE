"""The game pad layout presets (input.gamepad_layout) over the profile's buttons: the real function and table."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, enum_with, function, mutated, read, run

CASES = ['profile', 'infinite', 'start_back', 'halo2']
CONTROLS = [
    ('if (!strcmp(name, gamepad_layouts[layout_index].name))', 'if (strcmp(name, gamepad_layouts[layout_index].name))', 'infinite'),
    ('for (layout_index = _gamepad_layout_halo2;', 'for (layout_index = _gamepad_layout_profile;', 'profile'),
]


def generated(fault=None):
    source = read('source/input/input_abstraction.c')
    code = function(source, 'input_abstraction_port_apply_layout')
    if fault:
        code = mutated(code, *fault)
    config = enum_with(source, '_game_control_jump') + '\n'
    return (('config.inc', config), ('gamepad_layouts.h', read('port/linux/game/gamepad_layouts.h')),
            ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('gamepad_layout_apply', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('gamepad_layout_apply', generated((before, after))), case)
    assert status != 0, f'{case} passes with the fault'
