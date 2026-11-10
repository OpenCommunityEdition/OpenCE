"""Step 1's pick of a public game for a matchmaking search (p2p_lobby.c)."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure


def generated(fault=None):
    header = read('port/linux/src/p2p.h')
    version = constant(read('port/linux/include/halo_port_limits.h'), 'HALO_PORT_NETWORK_VERSION')
    enums = '\n'.join([f'#define HALO_PORT_NETWORK_VERSION {version}',
                       enum_with(header, 'P2P_LISTING_NAME_SIZE = 32'),
                       enum_with(header, 'P2P_LOBBY_PICK_PING = 200'),
                       structure(header, 'p2p_listing'),
                       structure(header, 'p2p_lobby_wanted')]) + '\n'
    source = read('port/linux/src/p2p_lobby.c')
    code = '\n'.join([function(source, 'pick_fits'), function(source, 'p2p_lobby_pick')])
    if fault:
        code = mutated(code, *fault)
    return (('enums.inc', enums), ('under_test.inc', code))


CASES = ['e2_most_players', 'e3_passed_over', 'e51_e56_dedicated']


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('matchmaking_pick', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    (('game->player_count > games[best].player_count', 'game->player_count < games[best].player_count'),
     'e2_most_players'),
    (('game->ping >= P2P_LOBBY_PICK_PING', 'game->ping > P2P_LOBBY_PICK_PING'), 'e3_passed_over'),
    (('if (game->dedicated)', 'if (0)'), 'e51_e56_dedicated'),
])
def test_negative_control(fault, case):
    status, output = run(build('matchmaking_pick', generated(fault)), case)
    assert status == CHECK_FAILED, output
