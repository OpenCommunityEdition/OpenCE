"""The players of a machine that has left the game (network_server_manager.c): those held out of a game for a map
download go with their machine, none is put back for a machine not in the game, and a kick takes such a player out."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ['gone-while-held', 'index-taken-again', 'held-not-joined', 'held-put-back', 'other-machine-gone',
         'kick-in-game', 'kick-in-lobby', 'ban-refused', 'kick-joined', 'kick-nobody', 'kick-host', 'split-screen']
FUNCTIONS = ('network_game_server_client_machine_is_joined_to_game', 'network_game_server_lobby_is_open',
             'network_game_server_machine_has_players', 'network_game_server_hold_map_fetcher_players',
             'network_game_server_restore_held_players', 'network_game_server_discard_held_players',
             'network_game_server_remove_client_machine_from_game',
             'network_game_server_remove_players_of_gone_machine', 'network_game_server_kick_machine_of_player')

# a fault in network_server_manager.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    'held-kept': (('\t\t\t\tnetwork_game_server_discard_held_players(client->machine_index);\n', ''),
                  'network_game_server_remove_client_machine_from_game', 'index-taken-again'),
    'pending-kept': (('network_game_server_map_pending[client->machine_index] = FALSE;', ';'),
                     'network_game_server_remove_client_machine_from_game', 'gone-while-held'),
    'restore-unchecked': (('||\n\t\t\t!network_game_server_client_machine_is_joined_to_game(server, '
                           '&server->client_machines[player.machine_index])', ''),
                          'network_game_server_restore_held_players', 'held-not-joined'),
    'gone-not-kicked': (('if (!ban && network_game_server_remove_players_of_gone_machine(server, machine_index))',
                         'if (0)'),
                        'network_game_server_kick_machine_of_player', 'kick-in-game'),
    'ban-takes-out': (('if (!ban && network_game_server_remove_players_of_gone_machine(server, machine_index))',
                       'if (network_game_server_remove_players_of_gone_machine(server, machine_index))'),
                      'network_game_server_kick_machine_of_player', 'ban-refused'),
    'quit-not-sent': (('if (server->state == _network_game_server_state_ingame)', 'if (0)'),
                      'network_game_server_remove_players_of_gone_machine', 'kick-in-game'),
    'lobby-not-sent': (('if (network_game_server_lobby_is_open(server) && ', 'if (0 && '),
                       'network_game_server_remove_players_of_gone_machine', 'kick-in-lobby'),
    'joined-taken-out': (('network_game_server_client_machine_is_joined_to_game(server, '
                          '&server->client_machines[machine_index]) ||\n\t\tnetwork_game_is_splitscreen_local()',
                          'network_game_is_splitscreen_local()'),
                         'network_game_server_remove_players_of_gone_machine', 'kick-host'),
    'split-screen-taken-out': ((' ||\n\t\tnetwork_game_is_splitscreen_local())', ')'),
                               'network_game_server_remove_players_of_gone_machine', 'split-screen'),
}


def generated(fault=None, faulty_function=None):
    server = read('source/networking/network_server_manager.c')
    limits = read('port/linux/include/halo_port_limits.h')
    config = ''
    for name in ('HALO_PORT_MAXIMUM_NETWORK_MACHINES', 'HALO_PORT_MAXIMUM_NETWORK_PLAYERS',
                 'HALO_PORT_MAXIMUM_NETWORK_MESSAGE_SIZE'):
        config += f'#define {name} {constant(limits, name)}\n'
    config += f'#define MILLISECONDS_PER_SECOND {constant(read("source/cseries/cseries.h"), "MILLISECONDS_PER_SECOND")}\n'
    config += f'#define MAXIMUM_LOCAL_PLAYERS {constant(read("source/game/players.h"), "MAXIMUM_LOCAL_PLAYERS")}\n'
    for member in ('_network_client_machine_validated_bit', '_network_game_server_state_pregame'):
        config += enum_with(server, member) + '\n'
    config += enum_with(read('source/networking/network_messages.h'), '_message_server_remove_player_ingame') + '\n'
    messages = structure(server, 'message_server_remove_player_ingame') + '\n'
    code = ''
    for name in FUNCTIONS:
        text = function(server, name)
        if fault and name == faulty_function:
            text = mutated(text, *fault)
        if not text.startswith('static '):
            text = 'static ' + text
        code += text + '\n'
    return (('config.inc', config), ('messages.inc', messages), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_gone_machine_players(case):
    status, output = run(build('server_gone_machine_players', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('control', NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build('server_gone_machine_players', generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f'network_server_manager.c with a fault ({control}) passed {case!r}: the test cannot see it'
