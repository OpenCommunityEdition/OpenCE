"""Actual scene clock and round-reset code with live network roles in the shell."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run  # noqa: E402

CASES = ['shell-local', 'shell-browser', 'shell-host', 'shell-joined',
         'shell-round-reset', 'return-to-shell', 'game-round-reset',
         'game-client-wait', 'game-client-ready', 'game-local', 'game-host', 'game-paused']
CONTROLS = {
    'shell-uses-network-queues': ('game_time_update', 'if (main_menu)\n', 'if (FALSE)\n', 'shell-host'),
    'shell-stops-on-reset': ('network_game_reset_for_next_round', 'if (!main_menu_is_active())', 'if (TRUE)', 'shell-round-reset'),
    'shell-sends-game-updates': ('game_time_update', 'if (!main_menu) /* port: no gameplay replication from a shell tick */\n\t\t\t\t\t\t\tnetwork_distributed_tick();',
                               'network_distributed_tick();', 'shell-browser'),
    'shell-client-authority': ('network_game_distributed_client', ' && !main_menu_is_active()', '', 'shell-browser'),
    'shell-clock-held': ('game_time_held', 'main_menu_is_active() || ', '', 'shell-browser'),
    'game-no-longer-waits': ('game_time_held', 'if (main_menu_is_active() ||', 'if (TRUE ||', 'game-client-wait'),
}


def generated(control=None):
    clock = read('source/game/game_time.c')
    functions = {name: function(clock, name) for name in
                 ['game_time_end', 'game_time_get_speed', 'game_time_held', 'game_time_update']}
    functions['network_game_reset_for_next_round'] = function(
        read('source/networking/network_game_manager.c'), 'network_game_reset_for_next_round')
    functions['network_game_distributed_client'] = function(
        read('source/networking/network_game_globals.c'), 'network_game_distributed_client')
    if control:
        name, before, after, _ = CONTROLS[control]
        functions[name] = mutated(functions[name], before, after)
    code = functions.pop('network_game_distributed_client') + '\n' + '\n'.join(functions.values())
    config = enum_with(read('source/game/game.h'), '_game_connection_local')
    config += f"\nenum {{ TICKS_PER_SECOND = {constant(read('source/cseries/cseries.h'), 'TICKS_PER_SECOND')}, SOME_LARGE_NUMBER_OF_TICKS = {constant(clock, 'SOME_LARGE_NUMBER_OF_TICKS')} }};\n"
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('menu_network_clock', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('control', CONTROLS)
def test_negative_control(control):
    status, output = run(build('menu_network_clock', generated(control)), CONTROLS[control][3])
    assert status == CHECK_FAILED, f'{control} was not caught: {output}'
