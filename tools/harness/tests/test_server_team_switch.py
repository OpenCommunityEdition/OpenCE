"""A dedicated server's !switch (server_team_switch.c): a player changes teams only in a team game being played (not
co-op, Infection or the postgame), with server.team_switch on, once each cooldown, and with the auto team balance
only to a smaller team (a waiting move counted in its new team); the move reaches the player, their network data and
the game's record. A dead player moves at once; a living one's unit is killed with no statistics and they move only
at its death (game_engine_player_killed's hooks), which is no one's kill, while another's kill that comes first stays
theirs. Each new map forgets the cooldowns and waiting moves. And network_chat.c's reading of the line: "!switch" or
"!team" alone, in either case."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run, structure  # noqa: E402

CASES = ["moves", "dead", "other-kill", "stale", "waiting-counted", "waiting-again", "new-map", "balance-even",
         "balance-bigger", "balance-off", "quit-not-counted", "no-teams", "infection", "postgame", "no-game", "coop",
         "off", "no-team", "cooldown", "cooldown-wrap", "cooldown-bounded", "command"]
FUNCTIONS = ("team_switch_balanced", "team_switch_seconds_left", "team_switch_move_get", "team_switch_counted_team",
             "team_switch_move_player", "server_team_switch", "server_team_switch_initialize_for_new_map",
             "server_team_switch_damage", "server_team_switch_player_killed", "server_team_switch_player_dead")
CHAT_FUNCTIONS = ("chat_lower", "chat_command_word", "chat_team_switch_command")

# a fault in server_team_switch.c (or network_chat.c), the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "balance-allows-even": (("return counts[1 - team] < counts[team];", "return counts[1 - team] <= counts[team];"),
                            "team_switch_balanced", "balance-even"),
    "balance-ignored": (("if (game_variant_options_get()->auto_team_balance && !team_switch_balanced(counts, team))",
                         "if (0)"), "server_team_switch", "balance-bigger"),
    "balance-always": (("if (game_variant_options_get()->auto_team_balance && !team_switch_balanced(counts, team))",
                        "if (!team_switch_balanced(counts, team))"), "server_team_switch", "balance-off"),
    "quit-counted": (("if (!other->quit_out_of_game && VALID_INDEX(other_team, 2))",
                      "if (VALID_INDEX(other_team, 2))"), "server_team_switch", "quit-not-counted"),
    "unit-not-killed": (("unit_kill_no_statistics(player->unit_index);", ""), "server_team_switch", "moves"),
    "moved-before-death": (("team_switch_moves[slot].active = TRUE;",
                            "team_switch_move_player(player, team); team_switch_moves[slot].active = TRUE;"),
                           "server_team_switch", "moves"),
    "record-not-moved": (("game->players[list_index].team_index = (char)team;", ";"), "team_switch_move_player",
                         "moves"),
    "network-data-not-moved": (("player->network_player_data.team_index = (char)team;", ""),
                               "team_switch_move_player", "moves"),
    "death-not-made": (("move->died = TRUE;", ""), "server_team_switch_player_killed", "moves"),
    "death-a-suicide": (("*killing_player_index = NONE;", ""), "server_team_switch_player_killed", "moves"),
    "damage-not-heard": (("move->no_statistics = no_statistics;", ";"), "server_team_switch_damage", "moves"),
    "other-kill-taken": (("if (move->no_statistics && *killing_player_index == dead_player_index &&", "if (1 ||"),
                         "server_team_switch_player_killed", "other-kill"),
    "stale-kept": (("|| player->unit_index != move->unit_index)", ")"), "server_team_switch_player_killed", "stale"),
    "waiting-not-counted": (("return move ? move->team : player->team_index;", "return player->team_index;"),
                            "team_switch_counted_team", "waiting-counted"),
    "waiting-again-allowed": (("if (team_switch_move_get(player_index))", "if (0)"), "server_team_switch",
                              "waiting-again"),
    "map-keeps-cooldowns": (("csmemset(team_switch_records, 0, sizeof(team_switch_records));", ""),
                            "server_team_switch_initialize_for_new_map", "new-map"),
    "map-keeps-moves": (("csmemset(team_switch_moves, 0, sizeof(team_switch_moves));", ""),
                        "server_team_switch_initialize_for_new_map", "new-map"),
    "no-teams-allowed": (("if (!game_engine_has_teams())", "if (0)"), "server_team_switch", "no-teams"),
    "infection-allowed": (("if (infection_enabled())", "if (0)"), "server_team_switch", "infection"),
    "postgame-allowed": (("if (!game_engine_running() || !game_engine_can_score())", "if (!game_engine_running())"),
                         "server_team_switch", "postgame"),
    "coop-allowed": (("if (!global_scenario || global_scenario->type != _scenario_type_multiplayer)",
                      "if (!global_scenario)"), "server_team_switch", "coop"),
    "setting-ignored": (('if (!config_boolean("server.team_switch"))', "if (0)"), "server_team_switch", "off"),
    "no-team-moved": (("if (!VALID_INDEX(team, 2))", "if (0)"), "server_team_switch", "no-team"),
    "cooldown-not-kept": (("team_switch_records[slot].milliseconds = now;", ""), "server_team_switch", "cooldown"),
    "cooldown-by-slot": (("if (cooldown_seconds <= 0 || record->player_index != player_index)",
                          "if (cooldown_seconds <= 0)"), "team_switch_seconds_left", "cooldown"),
    "cooldown-wrap-broken": (("unsigned long elapsed = now - record->milliseconds;",
                              "unsigned long elapsed = now >= record->milliseconds ? now - record->milliseconds : "
                              "UNSIGNED_LONG_MAX;"), "team_switch_seconds_left", "cooldown-wrap"),
    "cooldown-unbounded": (("MIN(cooldown_seconds, (long)TEAM_SWITCH_MAXIMUM_COOLDOWN_SECONDS)", "cooldown_seconds"),
                           "team_switch_seconds_left", "cooldown-bounded"),
    "command-with-words": (("return rest && !*rest;", "return rest != NULL;"), "chat_team_switch_command", "command"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/game/server_team_switch.c")
    chat = read("port/linux/game/network_chat.c")
    scenario = read("source/scenario/scenario_definitions.h")
    config = f"#define TEAM_SWITCH_MAXIMUM_COOLDOWN_SECONDS {constant(source, 'TEAM_SWITCH_MAXIMUM_COOLDOWN_SECONDS')}\n"
    types = re.search(r"(_scenario_type_solo = 0,\s*_scenario_type_multiplayer,)", scenario).group(1)
    config += "enum { " + types + " };\n"
    config += structure(source, "team_switch_record") + "\n"
    config += structure(source, "team_switch_move") + "\n"
    config += re.search(r"^static struct team_switch_record team_switch_records\[.*?\];$", source, re.M).group(0) + "\n"
    config += re.search(r"^static struct team_switch_move team_switch_moves\[.*?\];$", source, re.M).group(0) + "\n"
    config += re.search(r"^static char const \*const team_switch_team_names\[2\] = .*?;$", source, re.M).group(0)
    config += "\n"
    text = ""
    for path_source, names in ((chat, CHAT_FUNCTIONS), (source, FUNCTIONS)):
        for name in names:
            code = function(path_source, name)
            if fault and name == faulty_function:
                code = mutated(code, *fault)
            text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("server_team_switch", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("server_team_switch", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
