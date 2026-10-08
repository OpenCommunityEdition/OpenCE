"""Real distributed packet admission and host notices with a running UI clock.

Only the dispatcher's entry guard is compiled here; packet decoding/replication
is covered by its own tests. Host kick/ban and ban-record functions are complete.
"""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run  # noqa: E402

CASES = ['menu-packets', 'game-packets', 'paused-game-packets', 'loading-packets',
         'short-packets', 'packet-transitions', 'menu-kick', 'game-kick',
         'menu-ban', 'game-ban', 'loading-ban']
CONTROLS = {
    'menu-packets-admitted': ('network_distributed_handle_message', '!distributed_game_in_progress()', '!game_in_progress()', 'menu-packets'),
    'menu-kick-broadcast': ('network_distributed_kick', 'distributed_game_in_progress()', 'game_in_progress()', 'menu-kick'),
    'menu-ban-broadcast': ('network_distributed_ban', 'if (distributed_game_in_progress())', 'if (game_in_progress())', 'menu-ban'),
    'menu-ban-old-identity': ('network_distributed_ban', 'if (distributed_game_in_progress() &&', 'if (game_in_progress() &&', 'menu-ban'),
    'menu-record-old-identity': ('distributed_write_player_record', '&& distributed_game_in_progress())', '&& game_in_progress())', 'menu-ban'),
}


def generated(control=None):
    source = read('port/linux/game/network_distributed.c')
    functions = {name: function(source, name) for name in
                 ['network_distributed_handle_message', 'distributed_printable',
                  'distributed_write_player_record', 'network_distributed_ban', 'network_distributed_kick']}
    # Stop immediately after the real admission guard, recording entry into
    # decoding. No simulated implementation of the guard or packet handlers.
    functions['network_distributed_handle_message'] = functions['network_distributed_handle_message'].split(
        '\tcsmemcpy(&header, message, sizeof(header));')[0] + '\tadmitted++;\n}\n'
    try:
        predicate = function(source, 'distributed_game_in_progress')
    except LookupError:
        # Allows the new regression to reproduce the pre-audit failure.
        predicate = 'static boolean distributed_game_in_progress(void) { return game_in_progress(); }'
    if control:
        name, before, after, _ = CONTROLS[control]
        functions[name] = mutated(functions[name], before, after)
    code = function(read('source/game/game_time.c'), 'game_in_progress') + '\n' + predicate
    code += '\n' + '\n'.join(functions.values())
    header = re.search(r'struct distributed_message_header\s*\{[^}]+\};',
                       read('port/linux/game/network_distributed.h')).group()
    identity = re.search(r'struct distributed_client_identity\s*\{[^}]+\};', source).group()
    defines = '\n'.join(f'#define {name} {constant(source, name)}' for name in
                        ['DISCORD_ID_SIZE', 'DISCORD_NAME_SIZE', 'MAXIMUM_NOTICE_LENGTH'])
    defines += f'\n#define HALO_PORT_MAXIMUM_NETWORK_MACHINES {constant(read("port/linux/include/halo_port_limits.h"), "HALO_PORT_MAXIMUM_NETWORK_MACHINES")}\n'
    return (('config.inc', defines + '\n' + header + '\n' + identity), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('menu_network_packets', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('control', CONTROLS)
def test_negative_control(control):
    status, output = run(build('menu_network_packets', generated(control)), CONTROLS[control][3])
    assert status == CHECK_FAILED, f'{control} was not caught: {output}'
