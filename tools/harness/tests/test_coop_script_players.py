"""Script player0/followers must agree with the engine's actual players list."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, read, run, function, inline, constant, structure, mutated, CHECK_FAILED


def generated(fault=None):
    port = read('port/linux/game/coop_scripts.c')
    followers = function(port, 'players_following')
    if fault == 'forward':
        followers = mutated(followers, 'players[player_count - 1] != unit_index', 'players[0] != unit_index')
    if fault == 'named':
        followers = mutated(followers, 'player_count - NAMED_PLAYERS - 1', 'player_count - 2')
    return (
        ('reference.inc', structure(read('source/objects/reference_lists.h'), 'data_reference') + '\n' +
         inline(read('source/objects/reference_lists.h'), 'reference_list_add')),
        ('players.inc', function(read('source/hs/hs_library_external.c'), 'hs_players')),
        ('followers.inc', '#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS ' +
         str(constant(read('port/linux/include/halo_port_limits.h'), 'HALO_PORT_MAXIMUM_NETWORK_PLAYERS')) +
         '\n#define NAMED_PLAYERS ' + str(constant(port, 'NAMED_PLAYERS')) + '\n' + followers),
        ('actions.inc', '\n'.join(function(port, name) for name in [
            'coop_scripts_teleport_followers', 'coop_scripts_suspend_followers',
            'coop_scripts_exit_followers', 'follower_seat_name',
            'coop_scripts_board_followers', 'coop_scripts_player_add_equipment'])),
    )


@pytest.mark.parametrize('case', ['counts', 'gaps', 'respawn', 'bounded', 'nonhost', 'teleport', 'suspend', 'exit', 'board', 'equipment'])
def test_script_players(case):
    status, output = run(build('coop_script_players', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault', ['forward', 'named'])
def test_negative(fault):
    status, output = run(build('coop_script_players', generated(fault)), 'counts')
    assert status == CHECK_FAILED, output
