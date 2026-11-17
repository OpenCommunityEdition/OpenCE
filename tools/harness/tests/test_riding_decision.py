"""Whether a game rides: the one rule below the map's own choice, and the gametype's flag for it."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure

CONTROLS = {'coop-reads-rule': ('? entry_rule != FALSE : host_option;', '? entry_rule != FALSE : gametype_rule;', 'decide-grid'),
            'override-without-dedicated': ('dedicated && override_gametype', 'override_gametype', 'decide-grid'),
            'multiplayer-reads-option': (': gametype_rule;', ': host_option;', 'decide-grid'),
            'entry-rule-ignored': ('entry_rule != NONE ? entry_rule != FALSE : host_option', 'host_option', 'decide-grid'),
            'entry-rule-in-multiplayer': ('\tif (!multiplayer)\n', '\tif (entry_rule != NONE)\n\t\treturn entry_rule != FALSE;\n\tif (!multiplayer)\n',
                                          'decide-grid'),
            'start-or-only': ('SET_FLAG(game->cooperative_flags, _network_game_cooperative_vehicle_riding_bit, riding);',
                              'if (riding)\n\t\tgame->cooperative_flags |= FLAG(_network_game_cooperative_vehicle_riding_bit);',
                              'start-overwrites-stale-bit'),
            'start-always-multiplayer': ('game->variant.game_engine_index != game_engine_none', 'TRUE', 'start-coop-uses-host-option'),
            'start-always-dedicated': ('halo_server_mode() != 0', 'TRUE', 'start-listen-host-ignores-override'),
            'start-clobbers-flags': ('SET_FLAG(game->cooperative_flags, _network_game_cooperative_vehicle_riding_bit, riding);',
                                     'game->cooperative_flags = (byte)(riding ? FLAG(_network_game_cooperative_vehicle_riding_bit) : 0);',
                                     'start-keeps-other-flags'),
            'start-wrong-bit': ('_game_variant_vehicle_riding_bit),', '_game_variant_generic_starting_equipment_bit),',
                                'start-stock-flags-silent'),
            'start-entry-rule-unread': ('\t\triding_entry_rule,\n', '\t\tNONE,\n', 'start-coop-entry-rule'),
            'entry-setter-ignores': ('\triding_entry_rule = rule == NONE || rule == FALSE ? rule : TRUE;\n', '', 'start-coop-entry-rule'),
            'bit-inserted-before': ('\t_game_variant_vehicle_riding_bit', '\t_game_variant_extra_bit,\n\t_game_variant_vehicle_riding_bit',
                                    'bit-six'),
            'entry-rule-wrong-key': ('strcmp(entry->rules[index].key, "vehicle_riding")', 'strcmp(entry->rules[index].key, "radar")',
                                     'entry-rule-read')}


def generated(fault=None):
    coop = read('port/linux/game/network_coop.c')
    entry_rule = re.search(r'^static short riding_entry_rule = NONE;$', coop, re.M).group(0)
    code = chr(10).join((function(read('port/linux/game/vehicle_riding.c'), 'vehicle_riding_decide'),
                      function(read('port/linux/game/server_director.c'), 'server_entry_riding_rule'), entry_rule,
                      function(coop, 'network_game_port_vehicle_riding_entry'),
                      function(coop, 'network_game_port_vehicle_riding_start')))
    engine = read('source/game/game_engine.h')
    rotation = read('port/linux/src/server_rotation.h')
    enums = chr(10).join((enum_with(engine, '_game_variant_draw_object_in_motion_sensor_bit = 0'), enum_with(engine, 'game_engine_none = 0'),
                       enum_with(read('source/networking/network_game_manager.h'),
                                 '_network_game_cooperative_no_player_collisions_bit = 0'),
                       enum_with(rotation, 'SERVER_RULES_MAXIMUM = 48'), structure(rotation, 'server_rule'),
                       structure(rotation, 'server_entry')))
    if fault:
        before, after, _ = CONTROLS[fault]
        if fault == 'bit-inserted-before':
            enums = mutated(enums, before, after)
        else:
            code = mutated(code, before, after)
    return (('under_test.inc', code), ('enums.inc', enums))


CASES = ['decide-grid', 'bit-six', 'start-coop-uses-host-option', 'start-mp-uses-gametype', 'start-overwrites-stale-bit',
         'start-dedicated-override', 'start-listen-host-ignores-override', 'start-keeps-other-flags', 'start-stock-flags-silent',
         'start-coop-entry-rule', 'start-entry-rule-multiplayer-ignored', 'entry-rule-read', 'entry-rule-not-inherited']


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('riding_decision', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault', sorted(CONTROLS))
def test_negative_control(fault):
    status, output = run(build('riding_decision', generated(fault)), CONTROLS[fault][2])
    assert status == CHECK_FAILED and 'check failed:' in output, output
