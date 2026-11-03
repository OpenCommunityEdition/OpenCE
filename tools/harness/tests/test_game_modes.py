"""OpenCE's gametypes: options' defaults and range, Hardcore's restore of a killer, the modes a gametype allows,
Infection's round and state, Gun Game's weapons, One in the Chamber's kills and rounds, Frag Nation's grenades
(port/linux/game/game_modes.c, infection.c, gun_game.c, one_in_the_chamber.c, frag_nation.c)."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure


def generated(fault=None):
    engine = read('source/game/game_engine.h')
    network = read('port/linux/game/network_infection.h')
    frag_nation_header = read('port/linux/game/frag_nation.h')
    enums = '\n'.join([
        enum_with(engine, 'game_engine_none = 0'), enum_with(engine, '_loadout_weapon_none = 0'),
        enum_with(engine, '_variant_vehicle_warthog = 0'), enum_with(engine, '_game_mode_none = 0'),
        enum_with(engine, '_gun_game_order_strongest_first = 0'),
        enum_with(network, '_distributed_infection_waiting = 0'),
        enum_with(network, '_distributed_infection_role_none = 0'),
        enum_with(frag_nation_header, '_frag_nation_kill_other = 0')]) + '\n'
    structures = '\n'.join([
        structure(engine, 'universal_variant'), 'union game_engine_variant { long data[6]; };',
        structure(engine, 'game_variant').replace('wchar_t', 'unsigned short'),
        structure(engine, 'game_variant_options'),
        structure(network, 'distributed_infection_player'), structure(network, 'distributed_infection_state')]) + '\n'
    modes = read('port/linux/game/game_modes.c')
    infection = read('port/linux/game/infection.c')
    gun_game = read('port/linux/game/gun_game.c')
    chamber = read('port/linux/game/one_in_the_chamber.c')
    frag_nation = read('port/linux/game/frag_nation.c')
    defaults = re.search(r'/\* Infection\'s options when a gametype has none.*?\};', modes, re.S).group(0)
    order = re.search(r'static byte const gun_game_strongest_first\[\] =.*?\};', gun_game, re.S).group(0)
    code = '\n'.join([
        defaults, function(modes, 'game_modes_option'), function(modes, 'game_modes_options_default'),
        function(modes, 'game_modes_options_sanitize'), function(modes, 'game_modes_player_killed'),
        function(modes, 'game_modes_variant_mode'), '#define GAME_MODES_KIND_HARDCORE 0x80',
        function(modes, 'game_modes_kind'), function(modes, 'game_modes_kind_name'),
        function(infection, 'infection_round_result'),
        function(infection, 'infection_state_valid'), order,
        '#define GUN_GAME_MAXIMUM_LEVELS NUMBEROF(gun_game_strongest_first)',
        function(gun_game, 'gun_game_levels_build'), function(modes, 'game_modes_hit_kills'),
        function(modes, 'game_modes_map_grenades_off'), function(modes, 'game_modes_bottomless'), function(chamber, 'one_in_the_chamber_round_for_kill'),
        function(frag_nation, 'frag_nation_grenades_for_kill'), function(frag_nation, 'frag_nation_grenades_added')])
    if fault:
        code = mutated(code, *fault)
    return (('enums.inc', enums), ('structures.inc', structures), ('under_test.inc', code))


CASES = ['options_default', 'hardcore_restore', 'hardcore_no_restore', 'sanitize_defaults', 'sanitize_range',
         'sanitize_keeps', 'variant_mode', 'round_result', 'state_valid', 'gun_game_order', 'gun_game_missing',
         'gun_game_random', 'kind', 'kind_untrusted', 'chamber_round', 'chamber_round_resets', 'chamber_hit_kills',
         'bottomless', 'map_grenades', 'grenades_for_kill', 'grenades_added']


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('game_modes', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    (('== TRUE;\n\tcsmemset(options->reserved, 0,', '== TRUE;\n\tcsmemset(options->reserved, 1,'), 'sanitize_range'),
    (('options->chamber_kill_resets = options->chamber_kill_resets == TRUE;',
      'options->chamber_kill_resets = options->chamber_kill_resets != FALSE;'), 'sanitize_range'),
    (('options->bottomless = options->bottomless == TRUE;', 'options->bottomless = options->bottomless != FALSE;'),
     'sanitize_range'),
    (('options->bottomless = FALSE;\n\tcsmemset', 'options->bottomless = TRUE;\n\tcsmemset'), 'options_default'),
    (('return !network_game_distributed_client() && game_engine_running() && game_variant_options_get()->bottomless;',
      'return game_engine_running() && game_variant_options_get()->bottomless;'), 'bottomless'),
    (('return !network_game_distributed_client() && game_engine_running() && game_variant_options_get()->bottomless;',
      'return !network_game_distributed_client() && game_variant_options_get()->bottomless;'), 'bottomless'),
    (('return !network_game_distributed_client() && game_engine_running() && game_variant_options_get()->bottomless;',
      'return !network_game_distributed_client() && game_engine_running();'), 'bottomless'),
    (('unit->object.shield_vitality < 1.0f', 'TRUE'), 'hardcore_restore'),
    (('friendly_fire ||\n', ''), 'hardcore_no_restore'),
    (('return variant->universal_variant.teams ? _game_mode_infection', 'return 1 ? _game_mode_infection'),
     'variant_mode'),
    (('if (!humans)\n\t\treturn 1;', 'if (!humans)\n\t\treturn NONE;'), 'round_result'),
    (('state->revision < revision ||', ''), 'state_valid'),
    (('player->reserved[0] || player->reserved[1] || player->reserved[2]', 'player->reserved[0]'), 'state_valid'),
    (('if (game_engine_loadout_weapon_definition(weapon) != NONE)', 'if (TRUE)'), 'gun_game_missing'),
    (('&& !network_game_distributed_client()', ''), 'gun_game_random'),
    (('game_engine_index == game_engine_slayer && mode > _game_mode_none', 'mode > _game_mode_none'),
     'kind_untrusted'),
    (('if (!hardcore || game_engine_index', 'if (game_engine_index'), 'kind_untrusted'),
    (('return !variant->universal_variant.teams ? _game_mode_one_in_the_chamber', 'return 1 ? _game_mode_one_in_the_chamber'),
     'variant_mode'),
    (('else if (*loaded < loaded_maximum)', 'else if (FALSE)'), 'chamber_round'),
    (('else if (*carried < carried_maximum)', 'else'), 'chamber_round'),
    (('*loaded = 1;\n\t\t*carried = 0;', '(*loaded)++;'), 'chamber_round_resets'),
    (('unit->unit.player_index == owner_player_index)', 'FALSE)'), 'chamber_hit_kills'),
    (('||\n\t\tnetwork_game_distributed_client())', ')'), 'chamber_hit_kills'),
    (('return game_variant_options_get()->frag_nation_no_map_grenades;', 'return FALSE;'), 'map_grenades'),
    (('*plasmas = 2;', '*plasmas = 1;'), 'grenades_for_kill'),
    (('*frags = plasmas_only ? 0 : 1;', '*frags = 1;'), 'grenades_for_kill'),
    (('if (added <= 0 || count >= maximum)', 'if (added <= 0)'), 'grenades_added'),
])
def test_negative_control(fault, case):
    status, output = run(build('game_modes', generated(fault)), case)
    assert status == CHECK_FAILED, output
