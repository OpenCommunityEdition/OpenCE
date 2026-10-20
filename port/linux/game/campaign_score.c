/*
CAMPAIGN_SCORE.C

The campaign's score (display.campaign_score, off by default: the stock
game has none). Each enemy a player kills is worth points by its actor
variant: a base by the kind of actor (a grunt, a jackal, an elite, a hunter,
the Flood's forms, a sentinel) times a rank by the variant's name (minor 1,
major 1.5, ultra 2, zealot 3, and so on) times the difficulty (easy 0.5,
normal 1, heroic 2, legendary 5), rounded up to a multiple of 5. Marines, crew and the like are
worth nothing, so friendly fire gains no points.

Only the machine that runs the AI knows the kills: the host of a co-op
game, or a single player. Each player has a score of his own, and the host's
screen shows the first local player's. A client's own score is not sent to
him (that would change the wire format).

The score is a log of kills, each with the game time it happened at. Going
back to a checkpoint puts the game time back, and the kills after it are
forgotten with it; the totals are summed from the log.

When a level is won (main_won_map) its totals and its best variants are
written to the log, and shown over the next level's start for a few
seconds.
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "ai/actors.h"
#include "ai/actor_definitions.h"
#include "ai/actor_types.h"
#include "cutscene/cinematics.h"
#include "input/input.h"
#include "interface/interface.h"
#include "interface/hud_messaging.h"
#include "main/console.h"
#include "objects/objects.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "units/units.h"

#include "campaign_score.h"

#include <stdio.h>
#include <string.h>

/* ---------- constants */

enum
{
	/* the kills one level logs (a level places a few hundred enemies) */
	CAMPAIGN_SCORE_MAXIMUM_KILLS = 8192,
	/* the players a score is kept for */
	CAMPAIGN_SCORE_MAXIMUM_PLAYERS = 16,
	/* the kinds of variant the summary tells apart */
	CAMPAIGN_SCORE_MAXIMUM_VARIANTS = 64,
	/* the lines of the summary, and their length */
	CAMPAIGN_SCORE_SUMMARY_LINES = 14,
	CAMPAIGN_SCORE_SUMMARY_LENGTH = 56,
	/* draw_string.c's right-justified and centred text (its enum is its own) */
	_campaign_score_text_right = 1,
	_campaign_score_text_center = 2,
	/* the corner's margins, in the HUD's 480 lines (the match clock's) */
	CAMPAIGN_SCORE_RIGHT_MARGIN = 5,
	CAMPAIGN_SCORE_BOTTOM_MARGIN = 14,
	/* and the score's gap above the clock */
	CAMPAIGN_SCORE_CLOCK_GAP = 8,
};

/* the score's colour: a player's name in the chat (network_chat.c's
chat_color) */
#define CAMPAIGN_SCORE_BLUE_RED 0.34f
#define CAMPAIGN_SCORE_BLUE_GREEN 0.51f
#define CAMPAIGN_SCORE_BLUE_BLUE 0.86f

/* ---------- structures */

struct campaign_score_kill
{
	long game_time;
	long variant_index;
	short player_index;
	short points;
};

struct campaign_score_variant
{
	long variant_index;
	long kills;
	long points;
};

/* ---------- prototypes */

/* port_config.c's, cinematics.c's / cseries_windows.c's, tag_files.c's */
int config_boolean(const char *name);
unsigned long config_changes(void);
/* input_abstraction.c: how many ticks the keyboard's jump key (A) has been held */
byte input_abstraction_port_accept(short controller_index);
char *tag_get_name(long tag_index);
/* sdl_platform.c's: the log (debug.txt) only, not the console on screen */
void platform_log(char const *format, ...);
/* network_coop.c's: co-op is a network game on a campaign level; the host's
kills and its win are sent to the clients (campaign_score_network_kill, ...) */
boolean network_coop_active(void);
void network_coop_note_score_kill(short player_index, long variant_index, short points);
void network_coop_note_score_won(void);

/* ---------- globals */

static struct campaign_score_kill campaign_score_kills[CAMPAIGN_SCORE_MAXIMUM_KILLS];
static long campaign_score_kill_count = 0;
static boolean campaign_score_full_logged = FALSE;
/* (the last game time seen: a smaller one is a checkpoint reverted to) */
static long campaign_score_last_time = 0;
/* a level just won, for the next one, and the one shown now */
static char campaign_score_pending[CAMPAIGN_SCORE_SUMMARY_LINES][CAMPAIGN_SCORE_SUMMARY_LENGTH];
static short campaign_score_pending_lines = 0;
static char campaign_score_summary[CAMPAIGN_SCORE_SUMMARY_LINES][CAMPAIGN_SCORE_SUMMARY_LENGTH];
static short campaign_score_summary_lines = 0;
/* (a button held when the summary came up does not dismiss it: only a new press) */
static boolean campaign_score_button_primed = FALSE;
static boolean campaign_score_button_held = FALSE;
/* the summary is shown over the level just won, which waits for it */
static boolean campaign_score_postgame = FALSE;

/* the difficulty's share of a point, in tenths: easy, normal, heroic, legendary */
static short const campaign_score_difficulty_tenths[NUMBER_OF_GAME_DIFFICULTY_LEVELS] = { 5, 10, 20, 50 };
static char const *const campaign_score_difficulty_names[NUMBER_OF_GAME_DIFFICULTY_LEVELS] =
	{ "EASY", "NORMAL", "HEROIC", "LEGENDARY" };

/* the rank of a variant by its name's words, tenths of a point per point
(the first that is in the name) */
static struct
{
	char const *word;
	short tenths;
} const campaign_score_ranks[] =
{
	{ "honor", 35 },
	{ "zealot", 30 },
	{ "commander", 30 },
	{ "captain", 30 },
	{ "gold", 30 },
	{ "white", 30 },
	{ "ultra", 20 },
	{ "spec", 20 },
	{ "stealth", 20 },
	{ "red", 20 },
	{ "major", 15 },
	{ "ranger", 15 },
	{ "heavy", 15 },
	{ "leader", 15 },
	{ "minor", 10 },
	{ NULL, 0 },
};

/* ---------- private code */

static boolean campaign_score_level(
	void)
{
	struct scenario *scenario;

	if (!game_in_progress() || game_engine_running())
		return FALSE;
	scenario = global_scenario_try_and_get();
	return scenario != NULL && scenario->type == _scenario_type_solo;
}

/* the difficulty, as an index of the tables above */
static short campaign_score_difficulty(
	void)
{
	short difficulty = game_difficulty_level_get();

	return PIN(difficulty, 0, NUMBER_OF_GAME_DIFFICULTY_LEVELS - 1);
}

/* the points a variant is worth (0 for one that is no enemy) */
static short campaign_score_variant_points(
	long variant_definition_index)
{
	struct actor_variant_definition *variant;
	struct actor_definition *actor;
	char const *name;
	char lower[128];
	long base;
	long tenths = 10;
	long points;
	long index;

	if (variant_definition_index == NONE)
		return 0;
	variant = actor_variant_definition_get(variant_definition_index);
	if (variant->actor_reference.index == NONE)
		return 0;
	actor = actor_definition_get(variant->actor_reference.index);
	switch (actor->type)
	{
	case _actor_elite: base = 30; break;
	case _actor_jackal: base = 15; break;
	case _actor_grunt: base = 10; break;
	case _actor_hunter: base = 100; break;
	case _actor_engineer: base = 5; break;
	case _actor_assassin: base = 30; break;
	case _actor_combat_form: base = 10; break;
	case _actor_infection_form: base = 5; break;
	case _actor_carrier_form: base = 10; break;
	case _actor_sentinel: base = 15; break;
	default: base = 0; break;
	}
	if (base == 0)
		return 0;
	name = tag_get_name(variant_definition_index);
	if (name != NULL)
	{
		for (index = 0; index < (long)sizeof(lower) - 1 && name[index]; index++)
			lower[index] = (char)((name[index] >= 'A' && name[index] <= 'Z') ? name[index] + 32 : name[index]);
		lower[index] = '\0';
		for (index = 0; campaign_score_ranks[index].word; index++)
		{
			if (strstr(lower, campaign_score_ranks[index].word))
			{
				tenths = campaign_score_ranks[index].tenths;
				break;
			}
		}
	}
	points = base * tenths * campaign_score_difficulty_tenths[campaign_score_difficulty()];
	/* (up to a multiple of 5: an enemy worth any is worth at least 5) */
	points = (points + 499) / 500 * 5;
	return (short)points;
}

/* a vehicle's worth (the ghost, banshee and wraith), scaled by difficulty
like an enemy; 0 for any other vehicle */
static short campaign_score_vehicle_points(
	long definition_index)
{
	char const *name = tag_get_name(definition_index);
	char lower[128];
	long base = 0;
	long index;
	long points;

	if (name == NULL)
		return 0;
	for (index = 0; index < (long)sizeof(lower) - 1 && name[index]; index++)
		lower[index] = (char)((name[index] >= 'A' && name[index] <= 'Z') ? name[index] + 32 : name[index]);
	lower[index] = '\0';
	if (strstr(lower, "ghost"))
		base = 50;
	else if (strstr(lower, "banshee"))
		base = 75;
	else if (strstr(lower, "wraith"))
		base = 100;
	if (base == 0)
		return 0;
	points = base * 10 * campaign_score_difficulty_tenths[campaign_score_difficulty()];
	points = (points + 499) / 500 * 5;
	return (short)points;
}

/* whether A or B is down on a controller (or the keyboard's jump key, A) */
static boolean campaign_score_button_down(
	void)
{
	short controller_index;

	for (controller_index = 0; controller_index < MAXIMUM_GAMEPADS; controller_index++)
	{
		struct gamepad_state const *gamepad = input_get_gamepad_state(controller_index);

		if (gamepad &&
			(gamepad->buttons[FIRST_GAMEPAD_ANALOG_BUTTON + _gamepad_analog_button_a] > 0 ||
				gamepad->buttons[FIRST_GAMEPAD_ANALOG_BUTTON + _gamepad_analog_button_b] > 0))
			return TRUE;
		if (input_abstraction_port_accept(controller_index) > 0)
			return TRUE;
	}
	return FALSE;
}

/* the kills after the game time are gone: a checkpoint was reverted to */
static void campaign_score_update(
	void)
{
	long time = game_time_get();

	if (time < campaign_score_last_time)
	{
		while (campaign_score_kill_count > 0 &&
			campaign_score_kills[campaign_score_kill_count - 1].game_time > time)
			campaign_score_kill_count--;
	}
	campaign_score_last_time = time;

	return;
}

static long campaign_score_total(
	short player_index)
{
	long total = 0;
	long index;

	for (index = 0; index < campaign_score_kill_count; index++)
	{
		if (campaign_score_kills[index].player_index == player_index)
			total += campaign_score_kills[index].points;
	}
	return total;
}

/* a variant's name as the summary has it: the last of its path, in capitals,
with spaces for underscores */
static void campaign_score_variant_name(
	long variant_index,
	char *name,
	long size)
{
	char const *path = tag_get_name(variant_index);
	char const *last;
	long index;

	if (path == NULL)
		path = "enemy";
	last = path;
	for (index = 0; path[index]; index++)
	{
		if (path[index] == '\\' || path[index] == '/')
			last = path + index + 1;
	}
	for (index = 0; index < size - 1 && last[index]; index++)
	{
		char c = last[index];

		if (c == '_')
			c = ' ';
		else if (c >= 'a' && c <= 'z')
			c = (char)(c - 32);
		name[index] = c;
	}
	name[index] = '\0';

	return;
}

/* the level's totals and best variants, as lines */
static short campaign_score_compose(
	char lines[CAMPAIGN_SCORE_SUMMARY_LINES][CAMPAIGN_SCORE_SUMMARY_LENGTH])
{
	struct campaign_score_variant variants[CAMPAIGN_SCORE_MAXIMUM_VARIANTS];
	long variant_count = 0;
	short count = 0;
	short player_index;
	long index;
	long kills = 0;
	long points = 0;
	long players = 0;

	for (index = 0; index < campaign_score_kill_count; index++)
	{
		long found;

		kills++;
		points += campaign_score_kills[index].points;
		for (found = 0; found < variant_count; found++)
		{
			if (variants[found].variant_index == campaign_score_kills[index].variant_index)
				break;
		}
		if (found == variant_count && variant_count < CAMPAIGN_SCORE_MAXIMUM_VARIANTS)
		{
			variants[found].variant_index = campaign_score_kills[index].variant_index;
			variants[found].kills = 0;
			variants[found].points = 0;
			variant_count++;
		}
		if (found < variant_count)
		{
			variants[found].kills++;
			variants[found].points += campaign_score_kills[index].points;
		}
	}
	if (kills == 0)
		return 0;

	snprintf(lines[count++], CAMPAIGN_SCORE_SUMMARY_LENGTH, "LEVEL COMPLETE  %s x%.1f",
		campaign_score_difficulty_names[campaign_score_difficulty()],
		campaign_score_difficulty_tenths[campaign_score_difficulty()] / 10.0);
	for (player_index = 0; player_index < CAMPAIGN_SCORE_MAXIMUM_PLAYERS; player_index++)
	{
		if (campaign_score_total(player_index) > 0)
			players++;
	}
	if (players > 1)
	{
		for (player_index = 0; player_index < CAMPAIGN_SCORE_MAXIMUM_PLAYERS &&
			count < CAMPAIGN_SCORE_SUMMARY_LINES - 6; player_index++)
		{
			if (campaign_score_total(player_index) > 0)
				snprintf(lines[count++], CAMPAIGN_SCORE_SUMMARY_LENGTH, "PLAYER %d  %ld",
					(int)player_index + 1, campaign_score_total(player_index));
		}
	}
	snprintf(lines[count++], CAMPAIGN_SCORE_SUMMARY_LENGTH, "SCORE %ld  KILLS %ld", points, kills);
	/* (the best variants, the most points first) */
	while (count < CAMPAIGN_SCORE_SUMMARY_LINES && variant_count > 0)
	{
		long best = 0;
		char name[40];

		for (index = 1; index < variant_count; index++)
		{
			if (variants[index].points > variants[best].points)
				best = index;
		}
		campaign_score_variant_name(variants[best].variant_index, name, sizeof(name));
		snprintf(lines[count++], CAMPAIGN_SCORE_SUMMARY_LENGTH, "%s x%ld  %ld",
			name, variants[best].kills, variants[best].points);
		variants[best] = variants[variant_count - 1];
		variant_count--;
	}

	return count;
}

/* ---------- public code */

boolean campaign_score_enabled(
	void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enabled = FALSE;

	if (changes != config_changes())
	{
		changes = config_changes();
		enabled = config_boolean("display.campaign_score") != 0;
	}
	return enabled && campaign_score_level();
}

void campaign_score_new_map(
	void)
{
	short line;

	campaign_score_kill_count = 0;
	campaign_score_postgame = FALSE;
	campaign_score_full_logged = FALSE;
	campaign_score_last_time = 0;
	/* (a level just won, shown now; else nothing) */
	campaign_score_summary_lines = campaign_score_pending_lines;
	for (line = 0; line < campaign_score_pending_lines; line++)
		memcpy(campaign_score_summary[line], campaign_score_pending[line], CAMPAIGN_SCORE_SUMMARY_LENGTH);
	campaign_score_pending_lines = 0;
	campaign_score_button_primed = FALSE;

	return;
}

/* a kill is logged (the host's own, or one it sent): FALSE if the log is full */
static boolean campaign_score_append(
	long variant_index,
	short player_index,
	short points)
{
	campaign_score_update();
	if (campaign_score_kill_count >= CAMPAIGN_SCORE_MAXIMUM_KILLS)
	{
		if (!campaign_score_full_logged)
		{
			campaign_score_full_logged = TRUE;
			console_warning("campaign score: the level's %d kills are logged; no more are scored",
				(int)CAMPAIGN_SCORE_MAXIMUM_KILLS);
		}
		return FALSE;
	}
	campaign_score_kills[campaign_score_kill_count].game_time = game_time_get();
	campaign_score_kills[campaign_score_kill_count].variant_index = variant_index;
	campaign_score_kills[campaign_score_kill_count].player_index = player_index;
	campaign_score_kills[campaign_score_kill_count].points = points;
	campaign_score_kill_count++;
	if (campaign_score_enabled())
		platform_log("campaign score: player %d killed %s for %d",
			(int)player_index, tag_get_name(variant_index), (int)points);

	return TRUE;
}

void campaign_score_record_kill(
	long unit_index,
	long killing_player_index)
{
	struct unit_datum *unit;
	long actor_index;
	long variant_index;
	short points;

	/* (a client's kills are the host's, which sends them: the AI is the host's) */
	if (killing_player_index == NONE || !campaign_score_level() || game_connection() == _game_connection_network_client)
		return;
	if (killing_player_index >= CAMPAIGN_SCORE_MAXIMUM_PLAYERS || !player_try_and_get(killing_player_index))
		return;
	unit = unit_get(unit_index);
	/* (a destroyed ghost, banshee or wraith is worth its own points) */
	variant_index = NONE;
	points = 0;
	if (unit->object.type == _object_type_vehicle)
	{
		variant_index = unit->definition_index;
		points = campaign_score_vehicle_points(variant_index);
	}
	if (points <= 0)
	{
		actor_index = unit->unit.swarm_actor_index != NONE ? unit->unit.swarm_actor_index : unit->unit.actor_index;
		if (actor_index == NONE)
			return;
		variant_index = actor_get(actor_index)->meta.variant_definition_index;
		points = campaign_score_variant_points(variant_index);
	}
	if (points <= 0)
		return;
	if (campaign_score_append(variant_index, (short)killing_player_index, points) && network_coop_active())
		network_coop_note_score_kill((short)killing_player_index, variant_index, points);

	return;
}

/* (network_coop.c) a kill the host scored, on a client */
void campaign_score_network_kill(
	short player_index,
	long variant_index,
	short points)
{
	if (!campaign_score_level() || player_index < 0 || player_index >= CAMPAIGN_SCORE_MAXIMUM_PLAYERS || points <= 0)
		return;
	campaign_score_append(variant_index, player_index, points);

	return;
}

static void campaign_score_win_compose(
	void)
{
	short line;

	if (!campaign_score_enabled())
		return;
	campaign_score_update();
	campaign_score_pending_lines = campaign_score_compose(campaign_score_pending);
	for (line = 0; line < campaign_score_pending_lines; line++)
		platform_log("campaign score: %s", campaign_score_pending[line]);

	return;
}

/* the pending summary is shown over the level (in the level itself, which is
still running): FALSE if there is none to show */
static boolean campaign_score_postgame_begin(
	void)
{
	short line;

	if (campaign_score_pending_lines <= 0 || !campaign_score_level() || cinematic_in_progress())
		return FALSE;
	campaign_score_summary_lines = campaign_score_pending_lines;
	for (line = 0; line < campaign_score_pending_lines; line++)
		memcpy(campaign_score_summary[line], campaign_score_pending[line], CAMPAIGN_SCORE_SUMMARY_LENGTH);
	campaign_score_pending_lines = 0;
	campaign_score_button_primed = FALSE;

	return TRUE;
}

boolean campaign_score_postgame_wait(
	void)
{
	if (!campaign_score_postgame)
	{
		if (!campaign_score_postgame_begin())
			return FALSE;
		campaign_score_postgame = TRUE;
		return TRUE;
	}
	/* (until A or B is pressed: campaign_score_draw) */
	if (campaign_score_summary_lines > 0 && campaign_score_level())
		return TRUE;
	campaign_score_postgame = FALSE;
	campaign_score_summary_lines = 0;

	return FALSE;
}

void campaign_score_level_won(
	void)
{
	if (!campaign_score_level())
		return;
	/* (the clients write their own summaries from the kills they were sent) */
	if (game_connection() == _game_connection_network_server && network_coop_active())
		network_coop_note_score_won();
	campaign_score_win_compose();

	return;
}

/* (network_coop.c) the host won the level, on a client */
void campaign_score_network_won(
	void)
{
	if (!campaign_score_level())
		return;
	campaign_score_win_compose();
	/* (shown at once: the host holds the level for its own, then the lobby
	takes everyone) */
	campaign_score_postgame_begin();

	return;
}

void campaign_score_draw(
	void)
{
	long font_index;
	struct font_header *font;
	short line_height;
	real_argb_color color;
	rectangle2d bounds;
	rectangle2d window;
	short line;

	if (cinematic_in_progress() || !game_in_progress())
		return;
	/* (the match clock's font: the HUD's) */
	font_index = hud_get_font_index();
	if (font_index == NONE)
		return;
	font = font_definition_get(font_index);
	line_height = (short)(font->ascending_height + font->descending_height + font->leading_height);
	if (line_height <= 0)
		return;
	window = render.camera.window_bounds;
	offset_rectangle2d(&window, (short)-render.camera.viewport_bounds.x0, (short)-render.camera.viewport_bounds.y0);

	/* the last level's summary, over this one's start */
	if (campaign_score_summary_lines > 0 && campaign_score_level())
	{
		boolean down = campaign_score_button_down();
		short top = (short)(window.y0 + (window.y1 - window.y0) / 4);

		/* (until A or B is pressed anew) */
		if (!campaign_score_button_primed)
			campaign_score_button_primed = TRUE;
		else if (down && !campaign_score_button_held)
			campaign_score_summary_lines = 0;
		campaign_score_button_held = down;

		color.alpha = 1.0f;
		for (line = 0; line < campaign_score_summary_lines; line++)
		{
			bounds.x0 = window.x0;
			bounds.x1 = window.x1;
			bounds.y0 = (short)(top + line * line_height);
			bounds.y1 = (short)(bounds.y0 + line_height);
			/* (the blue of a player's name in the chat, the header paler) */
			color.red = line == 0 ? 0.60f : CAMPAIGN_SCORE_BLUE_RED;
			color.green = line == 0 ? 0.74f : CAMPAIGN_SCORE_BLUE_GREEN;
			color.blue = line == 0 ? 1.0f : CAMPAIGN_SCORE_BLUE_BLUE;
			draw_string_set_draw_mode(font_index, NONE, _campaign_score_text_center, 0, &color);
			rasterizer_draw_string(&bounds, NULL, NULL, 0, campaign_score_summary[line]);
		}
		if (campaign_score_summary_lines > 0)
		{
			bounds.x0 = window.x0;
			bounds.x1 = window.x1;
			bounds.y0 = (short)(top + (campaign_score_summary_lines + 1) * line_height);
			bounds.y1 = (short)(bounds.y0 + line_height);
			color.red = color.green = color.blue = 0.7f;
			draw_string_set_draw_mode(font_index, NONE, _campaign_score_text_center, 0, &color);
			rasterizer_draw_string(&bounds, NULL, NULL, 0, "PRESS A OR B TO CONTINUE");
		}
	}

	return;
}

/* the score, above the match clock at the bottom right of one player's view
(hud.c's hud_draw_screen, beside the clock) */
void campaign_score_draw_view(
	long player_index)
{
	long font_index;
	struct font_header *font;
	short clock_height;
	real_argb_color color;
	rectangle2d bounds;
	rectangle2d window;
	char text[CAMPAIGN_SCORE_SUMMARY_LENGTH];

	if (!campaign_score_enabled() || player_index == NONE)
		return;
	font_index = hud_get_font_index();
	if (font_index == NONE)
		return;
	font = font_definition_get(font_index);
	clock_height = (short)(font->ascending_height + font->descending_height);
	window = render.camera.window_bounds;
	offset_rectangle2d(&window, (short)-render.camera.viewport_bounds.x0, (short)-render.camera.viewport_bounds.y0);
	campaign_score_update();
	snprintf(text, sizeof(text), "SCORE %ld", campaign_score_total((short)player_index));
	/* (above the match clock) */
	bounds.x1 = (short)(window.x1 - CAMPAIGN_SCORE_RIGHT_MARGIN);
	bounds.x0 = (short)(bounds.x1 - 320);
	bounds.y1 = (short)(window.y1 - CAMPAIGN_SCORE_BOTTOM_MARGIN - clock_height - CAMPAIGN_SCORE_CLOCK_GAP);
	bounds.y0 = (short)(bounds.y1 - clock_height);
	/* (the blue of a player's name in the chat, network_chat.c's) */
	color.red = CAMPAIGN_SCORE_BLUE_RED;
	color.green = CAMPAIGN_SCORE_BLUE_GREEN;
	color.blue = CAMPAIGN_SCORE_BLUE_BLUE;
	color.alpha = 1.0f;
	draw_string_set_draw_mode(font_index, NONE, _campaign_score_text_right, 0, &color);
	rasterizer_draw_string(&bounds, NULL, NULL, 0, text);

	return;
}
