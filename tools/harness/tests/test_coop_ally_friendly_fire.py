"""Network co-op's ALLY FRIENDLY FIRE: a player's hits of AI allies, and the rest left alone."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run


def generated(fault=None):
    header = read('source/game/game_engine.h')
    enums = enum_with(header, '_friendly_fire_on = 0') + '\n' + enum_with(header, '_friendly_damage_all = 0') + '\n'
    code = function(read('source/game/game_engine.c'), 'game_engine_friendly_damage')
    if fault:
        code = mutated(code, *fault)
    return (('enums.inc', enums), ('under_test.inc', code))


@pytest.mark.parametrize('case', ['ally_on', 'ally_off', 'ally_off_ai_hit', 'ally_off_players', 'ally_off_not_unit',
                                  'not_coop'])
def test_case(case):
    status, output = run(build('coop_ally_friendly_fire', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    (('attacker_player_index != NONE && !network_coop_ally_friendly_fire()', 'attacker_player_index != NONE'),
     'ally_on'),
    (('attacker_player_index != NONE && !network_coop_ally_friendly_fire()', '!network_coop_ally_friendly_fire()'),
     'ally_off_ai_hit'),
    (('_friendly_damage_none : _friendly_damage_all;', '_friendly_damage_all : _friendly_damage_all;'), 'ally_off'),
    (('if (unit->unit.player_index == NONE)', 'if (unit->unit.player_index == NONE || !network_coop_ally_friendly_fire())'),
     'ally_off_players'),
])
def test_negative_control(fault, case):
    status, output = run(build('coop_ally_friendly_fire', generated(fault)), case)
    assert status == CHECK_FAILED, output
