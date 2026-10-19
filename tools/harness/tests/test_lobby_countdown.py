"""The real pregame countdown of network_server_manager.c: the lobby's START NOW starts it for a host alone, a host with
split screen players and machines together, and a machine joining without a player does not stop it."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = [
    "split-screen-start-now", "alone-start-now", "empty-lobby-waits", "two-machines-start-now",
    "two-machines-team-game-needs-both-teams", "playerless-machine-keeps-countdown",
    "split-screen-team-game-one-team",
]
FUNCTIONS = (
    "countdown_timer_update", "countdown_timer_get_time_remaining", "countdown_timer_increment",
    "countdown_timer_decrement", "countdown_timer_set_time_remaining",
    "network_game_server_get_client_machine_count", "server_needs_more_teams",
    "server_has_enough_machines", "server_players_on_one_machine", "server_ok_to_countdown",
    "network_game_server_update_countdown",
)
CONSTANTS = (
    "MAXIMUM_NETWORK_MACHINE_COUNT", "MAXIMUM_NETWORK_PLAYER_COUNT", "NUMBER_OF_MULTIPLAYER_TEAMS",
    "NETWORK_GAME_COUNTDOWN_TIME", "NETWORK_GAME_SPLITSCREEN_COUNTDOWN_TIME",
    "NETWORK_GAME_COUNTDOWN_ADJUSTMENT", "NETWORK_GAME_MINIMUM_COUNTDOWN_TIME",
)

# a fault in network_server_manager.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "one-player-only": (
        ("else if ((short)player->machine_index != first_machine_index)", "else"),
        "server_players_on_one_machine", "split-screen-start-now",
    ),
    "machines-ignored": (
        ("else if ((short)player->machine_index != first_machine_index)", "else if (0)"),
        "server_players_on_one_machine", "two-machines-team-game-needs-both-teams",
    ),
    "no-player-counts": (
        ("return first_machine_index != NONE;", "return TRUE;"),
        "server_players_on_one_machine", "empty-lobby-waits",
    ),
    "joined-machines-counted": (
        (
            "return first_machine_index != NONE;",
            "return first_machine_index != NONE && network_game_server_get_client_machine_count(server) == 1;",
        ),
        "server_players_on_one_machine", "playerless-machine-keeps-countdown",
    ),
}


def generated(fault=None, faulty_function=None):
    source = read("source/networking/network_server_manager.c")
    # (the machine and player counts are halo_port_limits.h's, which constant() does not follow into)
    with_limits = source + read("port/linux/include/halo_port_limits.h")
    config = ""
    for name in CONSTANTS:
        config += f"#define {name} {constant(with_limits, name)}\n"
    for member in ("_network_game_server_state_pregame", "_network_game_server_countdown_event_player_left"):
        config += enum_with(source, member) + "\n"
    for name in ("countdown_timer", "network_game_server_countdown_state"):
        config += structure(source, name) + "\n"
    text = ""
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("lobby_countdown", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("lobby_countdown", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"network_server_manager.c with a fault ({control}) passed '{case}': the test cannot see it"
