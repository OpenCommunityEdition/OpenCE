"""Real menu and server teardown, including native/legacy platform guards."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run  # noqa: E402

PLATFORMS = {
    'windows': ('#define HALO_WINDOWS 1', 0),
    'linux': ('#define __linux__ 1', 0),
    'android': ('#define HALO_ANDROID 1', 0),
    'xbox': ('', 1000),
    'other-port': ('#define HALO_UNEXAMINED_PORT 1', 1000),
}
CASES = [(platform, case) for platform in PLATFORMS for case in ('back', 'reopen')]
CASES += [('windows', case) for case in ('postgame', 'ingame', 'message-failure',
                                       'send-failure', 'machine-failure', 'no-server')]
CASES += [('xbox', case) for case in ('postgame', 'ingame')]
GUARD = '#if !defined(HALO_WINDOWS) && !defined(HALO_ANDROID) && !defined(__linux__)'
CONTROLS = {
    'native-sleeps': (GUARD, '#if 1', 'windows', 'back'),
    'xbox-loses-grace': ('SleepEx(MILLISECONDS_PER_SECOND, FALSE);', ';', 'xbox', 'back'),
    'transport-not-terminated': ('transport_server_terminate();', ';', 'windows', 'back'),
    'connections-not-closed': ('network_connection_delete(server->connection);', ';', 'windows', 'reopen'),
    'server-state-not-cleared': ('csmemset(server, 0, sizeof(*server));', ';', 'windows', 'back'),
}


def generated(platform, control=None):
    server = read('source/networking/network_server_manager.c')
    code = function(server, 'network_game_server_dispose')
    if control:
        before, after, _, _ = CONTROLS[control]
        code = mutated(code, before, after)
    globals_source = read('source/networking/network_game_globals.c')
    for name in ('dispose_global_network_game_server', 'dispose_global_network_game_client'):
        code += '\n' + function(globals_source, name)
    widgets = read('source/interface/ui_widget_event_handler_functions.c')
    for name in ('clear_multiplayer_player_joins', 'ui_widget_port_host'):
        code += '\n' + function(widgets, name)
    defines, sleep = PLATFORMS[platform]
    # The harness runs on Linux; each compiled variant selects its own target.
    config = '#undef __linux__\n#define xbox 1\n' + defines + f'\n#define EXPECTED_SLEEP {sleep}\n'
    config += f'#define MILLISECONDS_PER_SECOND {constant(read("source/cseries/cseries.h"), "MILLISECONDS_PER_SECOND")}\n'
    for member in ('_network_game_server_state_pregame', '_network_game_server_game_open_bit'):
        config += enum_with(server, member) + '\n'
    config += enum_with(read('source/networking/network_messages.h'), '_message_server_graceful_game_exit_pregame') + '\n'
    for name in ('message_server_graceful_game_exit_pregame', 'message_server_graceful_game_exit_postgame'):
        config += re.search(r'struct ' + name + r'\s*\{[^}]+\};', server).group() + '\n'
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('platform,case', CASES)
def test_case(platform, case):
    status, output = run(build('server_teardown', generated(platform)), case)
    assert status == 0, output


@pytest.mark.parametrize('control', CONTROLS)
def test_negative_control(control):
    _, _, platform, case = CONTROLS[control]
    status, output = run(build('server_teardown', generated(platform, control)), case)
    assert status == CHECK_FAILED, f'{control} was not caught: {output}'
