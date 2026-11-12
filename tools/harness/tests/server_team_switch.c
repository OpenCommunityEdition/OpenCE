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
static long cooldown_seconds = 30, killed_player = NONE, killed_unit = NONE;
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
/* (as mcc_ui_teams.c's: a dead player has no unit to kill) */
static void mcc_ui_team_change_kill(long player_index, long unit_index)
{
	if (unit_index == NONE)
		return;
	kills++;
	killed_player = player_index;
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
		CHECK(people[slot].team_index == 1, "the player is blue");
		CHECK(people[slot].network_player_data.team_index == 1, "the player's network data is blue");
		CHECK(fake_game.players[slot].team_index == 1, "the game's record is blue");
		CHECK(kills == 1 && killed_player == player_index_of(slot) && killed_unit == 77,
			"their living unit is killed as a team change's (no one's kill)");
		CHECK(!strcmp(notice, "Chief switched to the Blue team"), "everyone is told: \"%s\"", notice);
		return 0;
	}
	CASE("dead")
	{
		slot = teams_of(3, 2);
		people[slot].unit_index = NONE;
		CHECK(ask(slot) && people[slot].team_index == 1, "a dead player switches");
		CHECK(kills == 0, "a dead player has no unit to kill");
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
		CHECK(ask(slot) && people[slot].team_index == 1, "without the balance, any switch (%s)", notice);
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
		CHECK(!ask(slot), "a player without a team");
		CHECK(people[slot].team_index == NONE && kills == 0, "left as they were");
		return 0;
	}
	CASE("cooldown")
	{
		options.auto_team_balance = FALSE;
		slot = teams_of(2, 2);
		CHECK(ask(slot), "the first switch");
		now_ms += 29001;
		kills = 0;
		CHECK(!ask(slot), "again 29 seconds later");
		CHECK(people[slot].team_index == 1 && kills == 0, "held: still blue");
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
