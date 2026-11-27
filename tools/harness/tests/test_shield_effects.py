"""The shield effects of a network game: the host's choice for every machine, or "players", each machine's own;
the host's message a client takes (port/linux/game/network_shield_effects.c)."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure


def generated(fault=None):
    header = read('port/linux/game/network_shield_effects.h')
    enums = '\n'.join([enum_with(header, '_shield_effects_stock'),
                       enum_with(read('source/game/game.h'), '_game_connection_local = 0'),
                       enum_with(read('port/linux/game/network_distributed.h'),
                                 '_distributed_message_shield_effects')]) + '\n'
    source = read('port/linux/game/network_shield_effects.c')
    names = re.search(r'static char const \*const shield_effects_names\[NUMBER_OF_SHIELD_EFFECTS\] =.*?\};', source,
                      re.S).group(0)
    state = re.search(r'static short client_effects = .*?;\nstatic short sent_effects = .*?;', source, re.S).group(0)
    code = '\n'.join([names, state, structure(source, 'shield_effects_message'), function(source, 'host_effects'),
                      function(source, 'network_shield_effects_reset'), function(source, 'network_shield_effects'),
                      function(source, 'network_shield_effects_handle_message')])
    if fault:
        code = mutated(code, *fault)
    return (('enums.inc', enums), ('under_test.inc', code))


@pytest.mark.parametrize('case', ['offline', 'host', 'client', 'client_untrusted', 'coop'])
def test_case(case):
    status, output = run(build('shield_effects', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    (('return effects == _shield_effects_players ? NONE : effects;', 'return effects;'), 'host'),
    (('return effects == _shield_effects_players ? NONE : effects;', 'return effects;'), 'client'),
    (('message.effects >= NUMBER_OF_SHIELD_EFFECTS', 'message.effects > NUMBER_OF_SHIELD_EFFECTS'),
     'client_untrusted'),
    (('"light_show_overshield", "players"', '"light_show_overshield", "own"'), 'host'),
    (('if (network_coop_active())', 'if (0)'), 'coop'),
])
def test_negative_control(fault, case):
    status, output = run(build('shield_effects', generated(fault)), case)
    assert status == CHECK_FAILED, output
