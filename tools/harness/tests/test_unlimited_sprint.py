"""Unlimited sprint (gameplay.unlimited_sprint): the forward throttle of a player holding the sprint control, from the
real function and constants of players.c."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, constant, enum_with, function, mutated, read, run

CASES = ['sprints', 'rules_off', 'not_held', 'crouching', 'forward_only', 'on_foot']
CONTROLS = [
    ('action->throttle.i * UNLIMITED_SPRINT_SPEED_SCALE', 'action->throttle.i * 1.4f', 'sprints'),
    ('\t\tnetwork_game_unlimited_sprint())', '\t\t1)', 'rules_off'),
    ('TEST_FLAG(action->control_flags, UNIT_CONTROL_PORT_SPRINT_BIT) &&', '1 &&', 'not_held'),
    ('!TEST_FLAG(action->control_flags, _unit_control_crouch_modifier_bit) &&', '1 &&', 'crouching'),
    ('action->throttle.i > UNLIMITED_SPRINT_MINIMUM_THROTTLE', 'action->throttle.i != 0.f', 'forward_only'),
    ('unit->object.parent_object_index == NONE &&', '1 &&', 'on_foot'),
]


def generated(fault=None):
    players = read('source/game/players.c')
    code = function(players, 'player_forward_throttle')
    if fault:
        code = mutated(code, *fault)
    config = '\n'.join([
        enum_with(read('source/units/units.h'), '_unit_control_crouch_modifier_bit'),
        '#define UNIT_CONTROL_PORT_SPRINT_BIT %d' % constant(read('source/units/units.h'), 'UNIT_CONTROL_PORT_SPRINT_BIT'),
        '#define UNLIMITED_SPRINT_SPEED_SCALE %s' % constant_text(players, 'UNLIMITED_SPRINT_SPEED_SCALE'),
        '#define UNLIMITED_SPRINT_MINIMUM_THROTTLE %s' % constant_text(players, 'UNLIMITED_SPRINT_MINIMUM_THROTTLE'),
    ]) + '\n'
    return (('config.inc', config), ('under_test.inc', code))


def constant_text(source, name):
    import re
    return re.search(r'#define\s+' + name + r'\s+(\S+)', source).group(1)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('unlimited_sprint', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('unlimited_sprint', generated((before, after))), case)
    assert status != 0, f'{case} passes with the fault'
