"""The real campaign_carry.c (game.carry_weapons): a won campaign level's weapons, their rounds and battery, the one
in hand and the grenades, given at the next level's first spawn in place of its loadout, looked up by name in the
new map; a weapon that map lacks replaced by one of the level's, none given when it has none of them; again on a
restart, never on another map, at a second spawn, in a network game, with the setting off or for a player dead at
the win."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure

CASES = ['carried', 'missing', 'none in map', 'once', 'restart', 'menu between', 'network', 'off', 'dead',
         'mcc grenades']
CONTROLS = [
    # (the weapon in hand not given first)
    ('if (slot == in_hand)', 'if (slot == in_hand && FALSE)', 'carried'),
    # (the carried weapon by the old map's tag index, not its name in the new)
    ('definition_index = tag_loaded(WEAPON_DEFINITION_TAG, carried->weapons[weapon].name);',
     'definition_index = carried->weapons[weapon].definition_index;', 'carried'),
    # (the rounds as carried, beyond the magazine's maxima)
    ('(short)PIN(carried->rounds_total[magazine], 0, magazine_definition->rounds_total_maximum)',
     'carried->rounds_total[magazine]', 'carried'),
    # (no weapon of the level's in a missing one's place)
    ('count < carried->weapon_count', 'count < 0', 'missing'),
    # (the level's loadout taken away though none carried is in the map)
    ('if (count > 0)', 'if (TRUE)', 'none in map'),
    # (given at every spawn)
    ('if (!carried->taken || carried->given)', 'if (!carried->taken)', 'once'),
    # (kept on any map begun after the level)
    ('!strcmp(map_name, campaign_carry_level)', 'TRUE', 'restart'),
    # (still pending on the map after the one after the win)
    ('if (campaign_carry_state == _campaign_carry_pending ||', 'if (campaign_carry_state != _campaign_carry_none ||',
     'menu between'),
    ('game_connection() != _game_connection_local ||', '', 'network'),
    ('if (TEST_FLAG(unit->object.damage_flags, _object_dead_bit))\n\t\treturn;', '', 'dead'),
]
CARRY = 'port/linux/game/campaign_carry.c'
FUNCTIONS = ['campaign_carry_weapon_keep', 'campaign_carry_take_player', 'campaign_carry_take', 'campaign_carry_new_map',
             'campaign_carry_holds', 'campaign_carry_add', 'campaign_carry_give_player', 'campaign_carry_give']


def generated(fault=None):
    carry = read(CARRY).replace('\r\n', '\n')
    units = read('source/units/units.h')
    config = '\n'.join(
        [enum_with(units, 'MAXIMUM_WEAPONS_PER_UNIT'), enum_with(units, '_unit_add_weapon_normal'),
         enum_with(units, 'NUMBER_OF_UNIT_GRENADE_TYPES'),
         enum_with(read('source/networking/network_connection.h'), 'MAXIMUM_NUMBER_OF_LOCAL_PLAYERS'),
         enum_with(read('source/objects/objects.h'), '_object_dead_bit'),
         enum_with(read('source/objects/object_types.h'), '_object_type_weapon'),
         enum_with(read('source/game/game.h'), '_game_connection_local'),
         enum_with(carry, 'CAMPAIGN_CARRY_GRENADE_TYPES'), enum_with(carry, '_campaign_carry_none'),
         structure(carry, 'campaign_carry_weapon'), structure(carry, 'campaign_carry_player')] +
        re.findall(r'^static [^;(]*\bcampaign_carry_(?:state|level|players)\b[^;(]*;$', carry, re.M)) + '\n'
    code = '\n'.join(function(carry, name) for name in FUNCTIONS)
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


def test_statics_taken():
    """(the module's three statics found, or the cases would not compile)"""
    assert len(re.findall(r'^static ', generated()[0][1], re.M)) == 3


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('campaign_carry', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('campaign_carry', generated((before, after))), case)
    assert status == CHECK_FAILED, output
