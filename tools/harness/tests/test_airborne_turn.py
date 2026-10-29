"""A player turning in the air: the body turns with the aim (game.enhanced_animations), so a shot goes where the
crosshair is rather than within the aiming screen about where the jump began."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run

CASES = ['turns', 'off', 'actor', 'melee', 'look']
CONTROLS = [
    ('throwing_turn ||\n\t\t\tairborne_turn)', 'throwing_turn)', 'turns'),
    ('biped->unit.player_index!=NONE &&\n\t\t\tbiped->unit.animation.state==_unit_state_airborne',
     'biped->unit.animation.state==_unit_state_airborne', 'actor'),
    ('biped->unit.animation.state==_unit_state_airborne &&\n\t\t\tunit_animation_enhanced(biped_index)',
     'biped->unit.animation.state==_unit_state_airborne', 'off'),
]


def generated(fault=None):
    bipeds = read('source/units/bipeds.c')
    units = read('source/units/units.h')
    config = '\n'.join([
        enum_with(read('source/objects/objects.h'), '_object_dead_bit'),
        enum_with(units, '_unit_state_airborne'),
        enum_with(units, '_unit_base_seat_asleep'),
        enum_with(units, '_unit_control_look_dont_turn_bit'),
        enum_with(units, '_unit_aim_without_turning_bit'),
        enum_with(read('source/units/unit_definitions.h'), '_unit_is_special_bit'),
        enum_with(read('source/units/biped_definitions.h'), '_biped_turns_without_animating_bit'),
        enum_with(bipeds, 'biped_state_idle = 0'),
    ]) + '\n'
    config += 'enum { TICKS_PER_SECOND = %d };\n' % constant(read('source/cseries/cseries.h'), 'TICKS_PER_SECOND')
    config += '#define _pi ((real)3.14159265358979323846f)\n'
    code = function(bipeds, 'biped_update_turning')
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('airborne_turn', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('airborne_turn', generated((before, after))), case)
    assert status == CHECK_FAILED, output
