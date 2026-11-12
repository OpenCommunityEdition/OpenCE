#include "harness.h"
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))
#define VALID_INDEX(index, count) ((index) >= 0 && (index) < (count))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define PLAYERS 16
#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS PLAYERS
#include "config.inc"
enum { _server_log_info };
struct network_player
{
	short player_list_index;
	char team_index;
};
struct player_datum
{
	short identifier;
	boolean quit_out_of_game;
	short team_index;
	long unit_index;
	struct network_player network_player_data;
};
struct data_iterator
{
	long index;
	long datum_index;
};
struct scenario
{
	short type;
};
struct game_variant_options
{
	boolean auto_team_balance;
};
struct network_game
{
	struct network_player players[PLAYERS];
};
struct network_game_server
{
	int unused;
};
static struct player_datum people[PLAYERS];
static struct data_array *player_data;
static struct scenario scenario = { _scenario_type_multiplayer }, *global_scenario = &scenario;
static struct game_variant_options options = { TRUE };
static struct network_game fake_game;
static struct network_game_server fake_server;
static boolean running = TRUE, can_score = TRUE, teams = TRUE, infection = FALSE, switch_on = TRUE;
static long cooldown_seconds = 30, killed_unit = NONE;
static int kills;
static unsigned long now_ms = 100000;
/* (functions, so that a fault that reads one no more still compiles) */
static boolean game_engine_running(void) { return running; }
static boolean game_engine_can_score(void) { return can_score; }
static boolean game_engine_has_teams(void) { return teams; }
static boolean infection_enabled(void) { return infection; }
static struct game_variant_options *game_variant_options_get(void) { return &options; }
static int config_boolean_value(void) { return switch_on; }
#define config_boolean(name) config_boolean_value()
#define config_integer(name) cooldown_seconds
#define system_milliseconds() now_ms
#define global_network_game_server_get() (&fake_server)
#define network_game_server_get_game(server) (&fake_game)
#define server_log(level, category, ...) ((void)0)
#define csmemset memset
static long player_index_of(int slot)
{
	return ((long)(unsigned short)people[slot].identifier << 16) | slot;
}
static struct player_datum *player_try_and_get(long index)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	if (slot >= PLAYERS || !people[slot].identifier || people[slot].identifier != (short)(index >> 16))
		return NULL;
	return &people[slot];
}
static void data_iterator_new(struct data_iterator *iterator, struct data_array *data)
{
	iterator->index = NONE;
}
static struct player_datum *data_iterator_next(struct data_iterator *iterator)
{
	while (++iterator->index < PLAYERS)
	{
		if (people[iterator->index].identifier)
		{
			iterator->datum_index = player_index_of((int)iterator->index);
			return &people[iterator->index];
		}
	}
	return NULL;
}
/* (the death it asks for is dealt by the next objects update: dies) */
static void unit_kill_no_statistics(long unit_index)
{
	kills++;
	killed_unit = unit_index;
}
#include "under_test.inc"

static int added;
/* a player in a slot of their own: their team, their unit (NONE: dead) */
static int add(short team, long unit)
{
	int slot = added++;
	people[slot].identifier = (short)(0xE000 + slot * 7);
	people[slot].team_index = team;
	people[slot].unit_index = unit;
	people[slot].network_player_data.player_list_index = (short)slot;
	people[slot].network_player_data.team_index = (char)team;
	fake_game.players[slot].team_index = (char)team;
	return slot;
}
/* red and blue players, the first a living red one (unit 77) */
static int teams_of(int red, int blue)
{
	int first = add(0, 77), index;
	for (index = 1; index < red; index++)
		add(0, 100 + index);
	for (index = 0; index < blue; index++)
		add(1, 200 + index);
	return first;
}
static char notice[64];
static boolean ask(int slot)
{
	notice[0] = 0;
	return server_team_switch(player_index_of(slot), "Chief", notice, (long)sizeof(notice));
}
/* a player's team everywhere it is kept: theirs, their network data's and
the game's record's (NONE: they disagree) */
static short team_of(int slot)
{
	short team = people[slot].team_index;
	if (people[slot].network_player_data.team_index != team || fake_game.players[slot].team_index != team)
		return NONE;
	return team;
}
static long killer, killing_object;
static boolean friendly;
/* the next objects update deals a living switching player's unit the death
the switch asked for: object_damage_aftermath's no-statistics death (around
which it tells server_team_switch_damage), game_engine_player_killed's
hooks; then they respawn */
static void dies(int slot)
{
	long player_index = player_index_of(slot), unit = people[slot].unit_index;
	killer = player_index;
	killing_object = unit;
	friendly = TRUE;
	server_team_switch_damage(player_index, unit, TRUE);
	server_team_switch_player_killed(&killer, &killing_object, player_index, &friendly);
	server_team_switch_player_dead(player_index);
	server_team_switch_damage(player_index, unit, FALSE);
	people[slot].unit_index = 900 + slot;
}
/* another player (in slot by) kills a player first: an ordinary kill, which
no no-statistics damage told of */
static void killed_by(int slot, int by)
{
	long player_index = player_index_of(slot);
	killer = player_index_of(by);
	killing_object = people[by].unit_index;
	friendly = FALSE;
	server_team_switch_player_killed(&killer, &killing_object, player_index, &friendly);
	server_team_switch_player_dead(player_index);
	people[slot].unit_index = 900 + slot;
}
static void unmoved(int slot, char const *why)
{
	CHECK(people[slot].team_index == 0 && people[slot].network_player_data.team_index == 0 &&
		fake_game.players[slot].team_index == 0, "a refused switch (%s) changed the team", why);
	CHECK(kills == 0, "a refused switch (%s) killed the unit", why);
	CHECK(strstr(notice, why) != NULL, "the refusal says \"%s\": \"%s\"", why, notice);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	int slot;

	CASE("moves")
	{
		slot = teams_of(3, 2);
		CHECK(ask(slot), "3 red against 2 blue: a red player may switch (%s)", notice);
		CHECK(!strcmp(notice, "Chief switched to the Blue team"), "everyone is told: \"%s\"", notice);
		CHECK(kills == 1 && killed_unit == 77, "their living unit is killed with no statistics");
		/* (still red until the death: a flag carrier moved now would hold
		their new team's flag) */
		CHECK(team_of(slot) == 0, "red until their unit dies (%d)", team_of(slot));
		dies(slot);
		CHECK(team_of(slot) == 1, "blue once it has died (%d)", team_of(slot));
		CHECK(killer == NONE && killing_object == NONE && !friendly, "the death is no one's: no suicide");
		return 0;
	}
	CASE("dead")
	{
		slot = teams_of(3, 2);
		people[slot].unit_index = NONE;
		CHECK(ask(slot) && team_of(slot) == 1, "a dead player switches at once (%s)", notice);
		CHECK(kills == 0, "a dead player has no unit to kill");
		return 0;
	}
	CASE("other-kill")
	{
		slot = teams_of(3, 2);
		CHECK(ask(slot), "the switch (%s)", notice);
		killed_by(slot, 4);
		CHECK(killer == player_index_of(4) && killing_object == 201 && !friendly,
			"another's kill, come first, stays theirs");
		CHECK(team_of(slot) == 1, "and the player is moved all the same (%d)", team_of(slot));
		return 0;
	}
	CASE("stale")
	{
		slot = teams_of(3, 2);
		CHECK(ask(slot), "the switch (%s)", notice);
		/* (their unit went without dying: deleted, and a new one theirs) */
		people[slot].unit_index = 500;
		dies(slot);
		CHECK(team_of(slot) == 0, "a later unit's death does not move them (%d)", team_of(slot));
		CHECK(killer == player_index_of(slot) && killing_object == 500 && friendly, "nor is it made no one's");
		return 0;
	}
	CASE("waiting-counted")
	{
		slot = teams_of(3, 2);
		CHECK(ask(slot), "the first red player's switch (%s)", notice);
		CHECK(!ask(1), "3 against 2 with one moving is 2 against 3: a second may not");
		CHECK(team_of(1) == 0 && strstr(notice, "Blue has 3 players, Red 2") != NULL, "\"%s\"", notice);
		return 0;
	}
	CASE("waiting-again")
	{
		cooldown_seconds = 0;
		options.auto_team_balance = FALSE;
		slot = teams_of(2, 2);
		CHECK(ask(slot), "the switch (%s)", notice);
		CHECK(!ask(slot), "another before the death");
		CHECK(kills == 1 && strstr(notice, "switching teams already") != NULL, "\"%s\"", notice);
		dies(slot);
		CHECK(team_of(slot) == 1, "the first is made (%d)", team_of(slot));
		return 0;
	}
	CASE("new-map")
	{
		options.auto_team_balance = FALSE;
		slot = teams_of(2, 2);
		people[slot].unit_index = NONE;
		CHECK(ask(slot) && team_of(slot) == 1, "a switch on the last map (%s)", notice);
		/* (and another player's move, waiting) */
		CHECK(ask(1), "a living player's switch (%s)", notice);
		server_team_switch_initialize_for_new_map();
		/* (the new map's players have the same indices) */
		people[slot].team_index = people[slot].network_player_data.team_index = 0;
		fake_game.players[slot].team_index = 0;
		CHECK(ask(slot) && team_of(slot) == 1, "no cooldown kept from the last map (%s)", notice);
		dies(1);
		CHECK(team_of(1) == 0, "no move kept from the last map (%d)", team_of(1));
		return 0;
	}
	CASE("balance-even")
	{
		slot = teams_of(2, 2);
		CHECK(!ask(slot), "2 against 2 would become 1 against 3");
		unmoved(slot, "unbalance");
		CHECK(!strcmp(notice, "Blue has 2 players, Red 2: that would unbalance the teams"), "\"%s\"", notice);
		return 0;
	}
	CASE("balance-bigger")
	{
		slot = teams_of(2, 3);
		CHECK(!ask(slot), "to the bigger team");
		unmoved(slot, "unbalance");
		return 0;
	}
	CASE("balance-off")
	{
		options.auto_team_balance = FALSE;
		slot = teams_of(1, 4);
		CHECK(ask(slot), "without the balance, any switch (%s)", notice);
		dies(slot);
		CHECK(team_of(slot) == 1, "1 red against 4 blue: moved to blue (%d)", team_of(slot));
		return 0;
	}
	CASE("quit-not-counted")
	{
		slot = teams_of(2, 1);
		people[add(1, 300)].quit_out_of_game = TRUE;
		people[add(1, 301)].quit_out_of_game = TRUE;
		CHECK(ask(slot), "2 red against 1 blue (and 2 blue who quit): allowed (%s)", notice);
		return 0;
	}
	CASE("no-teams")
	{
		teams = FALSE;
		slot = teams_of(3, 1);
		CHECK(!ask(slot), "a game without teams");
		unmoved(slot, "only in team games");
		return 0;
	}
	CASE("infection")
	{
		infection = TRUE;
		slot = teams_of(3, 1);
		CHECK(!ask(slot), "Infection decides the teams");
		unmoved(slot, "Infection");
		return 0;
	}
	CASE("postgame")
	{
		can_score = FALSE;
		slot = teams_of(3, 1);
		CHECK(!ask(slot), "the postgame");
		unmoved(slot, "only while a game is played");
		return 0;
	}
	CASE("no-game")
	{
		running = FALSE;
		slot = teams_of(3, 1);
		CHECK(!ask(slot), "no game");
		unmoved(slot, "only while a game is played");
		return 0;
	}
	CASE("coop")
	{
		scenario.type = _scenario_type_solo;
		slot = teams_of(3, 1);
		CHECK(!ask(slot), "co-op");
		unmoved(slot, "Co-op");
		return 0;
	}
	CASE("off")
	{
		switch_on = FALSE;
		slot = teams_of(3, 1);
		CHECK(!ask(slot), "server.team_switch off");
		unmoved(slot, "off");
		return 0;
	}
	CASE("no-team")
	{
		slot = teams_of(3, 1);
		people[slot].team_index = NONE;
		/* (a switch of theirs a second ago is held, so that code which does not
		look at the team stops at the cooldown, with its notice, and reads no
		counts or names by a team of NONE: what those hold is no one's) */
		team_switch_records[slot].player_index = player_index_of(slot);
		team_switch_records[slot].milliseconds = now_ms - 1000;
		CHECK(!ask(slot), "a player without a team");
		CHECK(people[slot].team_index == NONE && kills == 0, "left as they were");
		CHECK(!strcmp(notice, "You can't switch teams now"), "\"%s\"", notice);
		return 0;
	}
	CASE("cooldown")
	{
		options.auto_team_balance = FALSE;
		slot = teams_of(2, 2);
		people[slot].unit_index = NONE;
		CHECK(ask(slot), "the first switch");
		now_ms += 29001;
		CHECK(!ask(slot), "again 29 seconds later");
		CHECK(people[slot].team_index == 1, "held: still blue");
		CHECK(!strcmp(notice, "You can switch teams again in 1 second"), "\"%s\"", notice);
		now_ms += 999;
		CHECK(ask(slot) && people[slot].team_index == 0, "again 30 seconds later (%s)", notice);
		/* (a later player in the slot is not held) */
		now_ms += 1000;
		people[slot].identifier++;
		CHECK(ask(slot), "a new player in the slot (%s)", notice);
		/* (none with no cooldown) */
		cooldown_seconds = 0;
		CHECK(ask(slot), "no cooldown (%s)", notice);
		return 0;
	}
	CASE("cooldown-wrap")
	{
		options.auto_team_balance = FALSE;
		slot = teams_of(2, 2);
		people[slot].unit_index = NONE;
		now_ms = 0xFFFFF000UL;
		CHECK(ask(slot), "the first switch");
		now_ms = 0x00001000UL;
		CHECK(!ask(slot), "8 seconds later, across the clock's wrap");
		CHECK(strstr(notice, "again in 22 seconds") != NULL, "\"%s\"", notice);
		return 0;
	}
	CASE("cooldown-bounded")
	{
		options.auto_team_balance = FALSE;
		cooldown_seconds = 100000;
		slot = teams_of(2, 2);
		people[slot].unit_index = NONE;
		CHECK(ask(slot), "the first switch");
		now_ms += 3600 * 1000UL;
		CHECK(ask(slot), "an hour later, the longest cooldown (%s)", notice);
		return 0;
	}
	CASE("command")
	{
		CHECK(chat_team_switch_command("!switch"), "!switch");
		CHECK(chat_team_switch_command("!SWITCH"), "either case");
		CHECK(chat_team_switch_command("!team"), "!team");
		CHECK(!chat_team_switch_command("!switch now"), "more words are said");
		CHECK(!chat_team_switch_command("!team push"), "more words are said");
		CHECK(!chat_team_switch_command("!switcher"), "a word of its own");
		CHECK(!chat_team_switch_command("!teams"), "a word of its own");
		CHECK(!chat_team_switch_command("switch"), "the !");
		CHECK(!chat_team_switch_command("gg !switch"), "at the start");
		return 0;
	}
	fprintf(stderr, "no case %s\n", case_name);
	return 2;
}
