/* Network co-op: a script's switch_bsp back to the BSP before waits while a
player still stands in the volume a script switched it for (players.c,
hs.c's switch_bsp), so two of a level's volumes don't switch it back and
forth. The real hs_volume_found, switch_bsp evaluator and hold, in a world
of two players along a corridor: volume 0 (x 0..1) switches to BSP 3,
volume 1 (x -2..-1) back to 2, as Operation Purgepoint's x05 does. */
#include "harness.h"

typedef unsigned short word;
#define _error_silent 0
static int logged;
#define error(priority, ...) (logged++)
enum { _game_connection_local, _game_connection_network_client, _game_connection_network_server };
enum { _hs_script_startup, _hs_script_dormant, _hs_script_continuous, _hs_script_static };

static int coop, connection, input, console, script_type, switches;
static long now;
static short global_structure_bsp_index;
static float player_x[2];
struct player_datum { long unit_index; };
static struct player_datum people[2] = { { 0 }, { 1 } };
struct data_iterator { long index; };
static void *player_data;
static void data_iterator_new(struct data_iterator *iterator, void *data) { iterator->index = -1; }
static struct player_datum *data_iterator_next(struct data_iterator *iterator)
{
	return ++iterator->index < 2 ? &people[iterator->index] : NULL;
}
static boolean network_coop_active(void) { return coop; }
static int game_connection(void) { return connection; }
static boolean player_input_enabled(void) { return input; }
static long game_time_get(void) { return now; }
static boolean game_engine_running(void) { return FALSE; }
static short global_structure_bsp_index_get(void) { return global_structure_bsp_index; }
static boolean scenario_trigger_volume_test_object(short volume, long unit)
{
	return volume == 0 ? player_x[unit] >= 0.f && player_x[unit] <= 1.f : player_x[unit] >= -2.f && player_x[unit] <= -1.f;
}
static boolean scenario_switch_structure_bsp(short index)
{
	if (index == global_structure_bsp_index)
		return FALSE;
	global_structure_bsp_index = index;
	switches++;
	return TRUE;
}
static boolean hs_runtime_port_executing_console_command(void) { return console; }
static short hs_runtime_port_executing_script_type(void) { return (short)script_type; }
static short scripted_multiplayer_bsp = NONE;
static boolean scripted_multiplayer_bsp_reported;
struct hs_arguments_word { word value; };
static struct hs_arguments_word argument;
static void *hs_macro_function_evaluate(short function_index, long thread_index, boolean initialize) { return &argument; }
static void hs_return(long thread_index, long value) {}
static struct
{
	boolean script_switch_held;
	short script_switch_volume;
	short script_switch_from;
	boolean script_switch_logged;
} players_coop_state;
static long hs_volume_found_thread = NONE;
static short hs_volume_found_volume = NONE;
static long hs_volume_found_time = NONE;
boolean players_coop_script_bsp_switch_allowed(short structure_bsp_index, boolean asks_again);
void players_coop_note_script_bsp_switch(short from_structure_bsp_index, short trigger_volume_index);
#include "hold.inc"
#include "switch.inc"

/* a thread's (volume_test_objects <volume> (players)) and (switch_bsp <index>) */
static boolean volume_test(long thread, short volume)
{
	return hs_volume_found(thread, volume,
		scenario_trigger_volume_test_object(volume, 0) || scenario_trigger_volume_test_object(volume, 1));
}
static void switch_bsp(long thread, short index)
{
	argument.value = (word)index;
	scenario_switch_structure_bsp_evaluate(0, thread, FALSE);
}
/* times round x05's bsp_switch3leftdown: each half in a tick of its own (its sleeps) */
static void loops(int count)
{
	while (count-- > 0)
	{
		now++;
		if (volume_test(7, 0))
			switch_bsp(7, 3);
		now++;
		if (volume_test(7, 1))
			switch_bsp(7, 2);
	}
}
static void reset(float first, float second)
{
	coop = input = TRUE;
	connection = _game_connection_network_server;
	console = FALSE;
	script_type = _hs_script_continuous;
	switches = logged = 0;
	now = 100;
	global_structure_bsp_index = 2;
	player_x[0] = first;
	player_x[1] = second;
	memset(&players_coop_state, 0, sizeof(players_coop_state));
	hs_volume_found_thread = hs_volume_found_time = NONE;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	CASE("straddle")
	{
		/* one in each volume: switched once, held, then switched back once
		the first volume is left */
		reset(0.5f, -1.5f);
		loops(20);
		CHECK(switches == 1 && global_structure_bsp_index == 3, "switched back and forth: %d switches", switches);
		CHECK(logged == 1, "the wait logged %d times", logged);
		player_x[0] = 5.f;
		loops(1);
		CHECK(switches == 2 && global_structure_bsp_index == 2, "not switched back once the volume was left");
		return 0;
	}
	CASE("crossing")
	{
		/* one player down and back, as alone: both switches */
		reset(0.5f, 10.f);
		loops(2);
		player_x[0] = -1.5f;
		loops(2);
		CHECK(switches == 2 && global_structure_bsp_index == 2 && !logged, "a lone crossing was held");
		return 0;
	}
	CASE("once")
	{
		/* a script that runs once (startup, dormant) never asks again: not held */
		reset(0.5f, -1.5f);
		loops(1);
		script_type = _hs_script_startup;
		loops(1);
		CHECK(global_structure_bsp_index == 2, "a script that runs once was held");
		return 0;
	}
	CASE("unheld")
	{
		/* the console, a cutscene, a client, single player, another BSP */
		reset(0.5f, -1.5f);
		loops(1);
		console = TRUE;
		now++;
		switch_bsp(7, 2);
		CHECK(global_structure_bsp_index == 2, "the console's switch was held");
		reset(0.5f, -1.5f);
		loops(1);
		input = FALSE;
		loops(1);
		CHECK(global_structure_bsp_index == 2, "a cutscene's switch was held");
		reset(0.5f, -1.5f);
		loops(1);
		connection = _game_connection_network_client;
		loops(1);
		CHECK(global_structure_bsp_index == 2, "a client held");
		reset(0.5f, -1.5f);
		loops(1);
		coop = FALSE;
		loops(1);
		CHECK(global_structure_bsp_index == 2, "single player held");
		reset(0.5f, -1.5f);
		loops(1);
		now++;
		switch_bsp(7, 4);
		CHECK(global_structure_bsp_index == 4, "a switch to a third BSP was held");
		return 0;
	}
	CASE("earlier")
	{
		/* a volume found in an earlier tick than the switch (sleep_until,
		then more) holds nothing */
		reset(0.5f, -1.5f);
		now++;
		volume_test(7, 0);
		now += 5;
		switch_bsp(7, 3);
		now++;
		if (volume_test(7, 1))
			switch_bsp(7, 2);
		CHECK(switches == 2 && global_structure_bsp_index == 2, "held by a volume found ticks before the switch");
		return 0;
	}
	return 2;
}
