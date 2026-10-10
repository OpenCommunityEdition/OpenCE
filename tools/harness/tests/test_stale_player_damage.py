"""Player-slot retirement and late damage, using the production lifetime paths."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure

CASES = ['retire', 'deleted', 'reused', 'live', 'dead', 'quit', 'unspawned',
         'unowned', 'nonlethal', 'friendly', 'driver', 'gunner', 'stale-item', 'ai-item', 'payload', 'authority']
CONTROLS = [
    ('object->object.owner_player_index = NONE;', ';', 'retire'),
    ('unit->unit.player_index = NONE;', ';', 'retire'),
    ('unit->unit.attackers[attacker_index].player_index = NONE;', ';', 'retire'),
    ('if (!attacker_player)', 'if (FALSE)', 'nonlethal'),
    ('damage->owner_player_index = NONE;', ';', 'payload'),
]


def generated(fault=None):
    units = read('source/units/units.c')
    damage = read('source/objects/damage.c')
    network = read('port/linux/game/network_damage.c')
    config = enum_with(read('source/objects/object_types.h'), '_object_type_biped')
    config += '\n#define MAXIMUM_ATTACKERS_PER_UNIT %d\n' % constant(read('source/units/units.h'), 'MAXIMUM_ATTACKERS_PER_UNIT')
    config += structure(read('source/units/units.h'), 'unit_attacker') + '\n'
    config += structure(read('source/objects/objects.h'), 'location') + '\n'
    config += structure(read('source/objects/damage.h'), 'damage_data') + '\n'
    code = '\n'.join([
        function(read('source/game/players.c'), 'player_delete'),
        function(units, 'unit_record_damage'),
        function(damage, 'damage_data_validate_owner'),
        function(network, 'network_damage_deals'),
    ])
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('stale_player_damage', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('stale_player_damage', generated((before, after))), case)
    assert status == CHECK_FAILED, output


def test_admission_before_normalization():
    # A rejected client's hit must retain its original identity for admission;
    # a nonlethal host replay bypasses object_cause_damage and needs its own guard.
    source = read('source/objects/damage.c')
    cause = function(source, 'object_cause_damage')
    assert cause.index('network_damage_deals(') < cause.index('return;') < cause.index('damage_data_validate_owner(damage);')
    aftermath = function(source, 'object_damage_aftermath')
    assert aftermath.index('damage_data_validate_owner(damage);') < aftermath.index('game_statistics_record_damage(')
    assert 'object_damage_aftermath(' in function(source, 'damage_replay_aftermath')
