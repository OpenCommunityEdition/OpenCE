"""Compile the production lobby gates with a small engine boundary.

Exercise both web and native builds without linking the game's renderer.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / "source/networking/network_server_manager.c").read_text()


def function(name):
    match = re.search(r"(?:static )?boolean " + name + r"\([^;]*?\)\s*\{", source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


boundary = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define MAXIMUM_NETWORK_MACHINE_COUNT 16
#define MAXIMUM_NETWORK_PLAYER_COUNT 16
#define NUMBER_OF_MULTIPLAYER_TEAMS 2
#define MAXIMUM_MACHINE_NAME_LENGTH 32
#define NETWORK_GAME_NAME_LENGTH 32
#define match_assert(file, line, check) assert(check)
#define network_event(...) ((void)0)
#define error(...) ((void)0)
#define ustrncpy wcsncpy
struct game_variant { struct { int teams; } universal_variant; };
struct network_player { int valid, machine_index, team_index; };
struct network_game_server_client_machine { int machine_index; };
struct network_game_server {
    struct { struct game_variant variant; struct { char name[256]; int version; } map;
        wchar_t name[32]; int minimum_players, maximum_players, maximum_teams, player_count;
        struct network_player players[16]; } game;
    struct network_game_server_client_machine client_machines[16];
};
static int distributed, splitscreen, opened;
boolean network_game_distributed(void) { return distributed; }
boolean network_game_is_splitscreen_local(void) { return splitscreen; }
boolean network_player_is_valid(struct network_player *player) { return player->valid; }
boolean game_engine_get_current_stage(struct game_variant *variant, char *name)
{ (void)variant; (void)name; return TRUE; }
void network_game_generate_local_machine_name(wchar_t *name) { wcscpy(name, L"Host"); }
void network_game_server_open_game(struct network_game_server *server) { (void)server; opened++; }
'''

gates = "\n".join(function(name) for name in (
    "server_needs_more_teams", "server_has_a_player_on_each_machine",
    "server_has_enough_machines", "server_ok_to_countdown",
    "network_game_server_setup_game_from_playlist",
))

cases = r'''
int main(void) {
    for (distributed = 0; distributed <= 1; distributed++) {
        struct network_game_server server = {0};
        for (int i = 0; i < 16; i++) server.client_machines[i].machine_index = -1;
        assert(network_game_server_setup_game_from_playlist(&server));
        int solo = 0;
#ifdef HALO_WEB
        solo = distributed;
#endif
        assert(server.game.minimum_players == (solo ? 1 : 2));
        assert(!server_ok_to_countdown(&server));
        server.client_machines[0].machine_index = 0;
        assert(!server_ok_to_countdown(&server));
        server.game.players[0] = (struct network_player){1, 0, 0};
        server.game.player_count = 1;
        assert(server_ok_to_countdown(&server) == solo);
        server.game.variant.universal_variant.teams = 1;
        assert(!server_ok_to_countdown(&server));
        server.game.variant.universal_variant.teams = 0;
        server.client_machines[1].machine_index = 1;
        assert(!server_ok_to_countdown(&server)); /* new machine has no player yet */
        server.game.players[1] = (struct network_player){1, 1, 1};
        server.game.player_count = 2;
        assert(server_ok_to_countdown(&server));
        server.game.variant.universal_variant.teams = 1;
        assert(server_ok_to_countdown(&server));
    }
    assert(opened == 2);
    puts("Solo lobby, empty lobby, joining machine, teams, and lockstep gates passed.");
}
'''

with tempfile.TemporaryDirectory(prefix="halo-lobby-test-") as directory:
    path = Path(directory)
    (path / "test.c").write_text(boundary + gates + cases)
    for defines in ([], ["-DHALO_WEB=1"]):
        subprocess.run([os.environ.get("CC", "clang"), "-std=c11", "-Wall", "-Wextra",
                        "-Werror", "-fsanitize=address,undefined", *defines,
                        str(path / "test.c"), "-o", str(path / "test")], check=True)
        subprocess.run([str(path / "test")], check=True)
