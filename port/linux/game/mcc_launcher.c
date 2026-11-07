/*
MCC_LAUNCHER.C

A temporary launcher for the Master Chief Collection's maps
(docs/mcc_caches.md), until the menus run on them. MCC's ui.map holds Halo
PC's menus, not the Xbox's: the game's own screens that are not in it do not
open (the network pregame screen), and Halo PC's list widgets stop the game.
So while the main menu is MCC's, its menus are closed and this lists the
campaign's levels and the multiplayer maps instead: up and down (the D-pad,
the left stick, the arrow keys) choose one, A or Enter plays it. A level
starts as the console's map_name starts it; a multiplayer map, a local slayer
game for controller 1, as Multiplayer > Split Screen starts one
(network_test.c does the same for its tests). The main menu then comes back
to the list.

Not shown when an automated test drives the game (debug.network_test).
*/

/* ---------- headers */

#include "cseries.h"
#include "main/main.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "input/input.h"
#include "interface/interface.h"
#include "interface/player_ui.h"
#include "interface/ui_widget.h"
#include "networking/network_game_globals.h"
#include "networking/network_client_manager.h"
#include "networking/network_server_manager.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "custom_edition_cache.h"

#include <stdio.h>
#include <string.h>

/* ---------- constants */

enum
{
	/* the list's rows shown at once, and the rows' gap */
	MCC_LAUNCHER_VISIBLE_ROWS = 18,
	MCC_LAUNCHER_ROW_GAP = 2,
	/* the left stick's travel that counts as up or down */
	MCC_LAUNCHER_STICK_THRESHOLD = 16000,
	/* draw_string.c's centred text (its enum is its own) */
	_mcc_launcher_text_centred = 2,
};

/* the seconds a local game takes to set up before it starts, as
network_test.c's: its map and gametype, then the player, then the start */
#define MCC_LAUNCHER_MAP_SECONDS 1.0f
#define MCC_LAUNCHER_PLAYER_SECONDS 2.0f
#define MCC_LAUNCHER_START_SECONDS 3.0f

/* ---------- structures */

struct mcc_launcher_entry
{
	char const *title;
	char const *path;
	boolean multiplayer;
};

/* ---------- prototypes */

/* port_config.c's, cseries_windows.c's, interface.c's, network_test.c's */
char const *config_string(char const *name);
void platform_log(char const *format, ...);
void draw_quad(rectangle2d *rectangle, pixel32 color);

/* ---------- globals */

static struct mcc_launcher_entry const mcc_launcher_entries[] =
{
	{ "The Pillar of Autumn", "levels\\a10\\a10", FALSE },
	{ "Halo", "levels\\a30\\a30", FALSE },
	{ "The Truth and Reconciliation", "levels\\a50\\a50", FALSE },
	{ "The Silent Cartographer", "levels\\b30\\b30", FALSE },
	{ "Assault on the Control Room", "levels\\b40\\b40", FALSE },
	{ "343 Guilty Spark", "levels\\c10\\c10", FALSE },
	{ "The Library", "levels\\c20\\c20", FALSE },
	{ "Two Betrayals", "levels\\c40\\c40", FALSE },
	{ "Keyes", "levels\\d20\\d20", FALSE },
	{ "The Maw", "levels\\d40\\d40", FALSE },
	{ "Battle Creek", "levels\\test\\beavercreek\\beavercreek", TRUE },
	{ "Blood Gulch", "levels\\test\\bloodgulch\\bloodgulch", TRUE },
	{ "Boarding Action", "levels\\test\\boardingaction\\boardingaction", TRUE },
	{ "Chill Out", "levels\\test\\chillout\\chillout", TRUE },
	{ "Chiron TL-34", "levels\\test\\putput\\putput", TRUE },
	{ "Damnation", "levels\\test\\damnation\\damnation", TRUE },
	{ "Derelict", "levels\\test\\carousel\\carousel", TRUE },
	{ "Hang 'Em High", "levels\\test\\hangemhigh\\hangemhigh", TRUE },
	{ "Longest", "levels\\test\\longest\\longest", TRUE },
	{ "Prisoner", "levels\\test\\prisoner\\prisoner", TRUE },
	{ "Rat Race", "levels\\test\\ratrace\\ratrace", TRUE },
	{ "Sidewinder", "levels\\test\\sidewinder\\sidewinder", TRUE },
	{ "Wizard", "levels\\test\\wizard\\wizard", TRUE },
};

static struct
{
	boolean checked;
	boolean disabled;
	boolean active;
	/* the entries whose maps are there, by index in mcc_launcher_entries */
	short shown[NUMBEROF(mcc_launcher_entries)];
	short shown_count;
	short selected;
	short first_row;
	/* the inputs held last frame, so that each press counts once */
	boolean held_up;
	boolean held_down;
	boolean held_accept;
	/* a local game being set up (NONE: none) */
	short starting;
	real setup_seconds;
	boolean map_set;
	boolean player_added;
} mcc_launcher;

/* ---------- private code */

static boolean mcc_launcher_up(
	struct gamepad_state const *gamepad)
{
	return input_key_is_down(_key_up_arrow) ||
		(gamepad && (gamepad->buttons[_gamepad_binary_button_dpad_up] ||
			gamepad->sticks[_gamepad_stick_left].y > MCC_LAUNCHER_STICK_THRESHOLD));
}

static boolean mcc_launcher_down(
	struct gamepad_state const *gamepad)
{
	return input_key_is_down(_key_down_arrow) ||
		(gamepad && (gamepad->buttons[_gamepad_binary_button_dpad_down] ||
			gamepad->sticks[_gamepad_stick_left].y < -MCC_LAUNCHER_STICK_THRESHOLD));
}

static boolean mcc_launcher_accept(
	struct gamepad_state const *gamepad)
{
	return input_key_is_down(_key_return) ||
		(gamepad && (gamepad->buttons[_gamepad_analog_button_a] || gamepad->buttons[_gamepad_binary_button_start]));
}

/* the entries whose maps are in the maps folder */
static void mcc_launcher_list(
	void)
{
	short index;

	mcc_launcher.shown_count = 0;
	for (index = 0; index < NUMBEROF(mcc_launcher_entries); index++)
	{
		if (mcc_level_name(mcc_launcher_entries[index].path))
			mcc_launcher.shown[mcc_launcher.shown_count++] = index;
	}
	mcc_launcher.selected = 0;
	mcc_launcher.first_row = 0;
}

/* a local slayer game on the multiplayer map `path`, set up as Multiplayer >
Split Screen sets one up (split_screen_game_initialize, as network_test.c
does): its map, gametype and player follow over the next seconds */
static void mcc_launcher_local_game_begin(
	char const *path)
{
	main_set_multiplayer_map_name(tag_name_strip_path(path));
	ui_widgets_close_all();
	dispose_global_network_game_server();
	dispose_global_network_game_client();
	network_game_accept_remote_connections(FALSE);
	game_engine_playlist_initialize();
	if (create_global_network_game_server())
	{
		game_engine_playlist_begin();
		game_connection_set(_game_connection_network_server);
	}
	if (!global_network_game_client_get())
		create_global_network_game_client();
	mcc_launcher.setup_seconds = 0.0f;
	mcc_launcher.map_set = FALSE;
	mcc_launcher.player_added = FALSE;
}

static void mcc_launcher_local_game_update(
	real seconds)
{
	char const *path = mcc_launcher_entries[mcc_launcher.starting].path;

	mcc_launcher.setup_seconds += seconds;
	if (!mcc_launcher.map_set && mcc_launcher.setup_seconds >= MCC_LAUNCHER_MAP_SECONDS && global_network_game_server_get())
	{
		struct game_variant variant;

		network_game_server_change_map_name(global_network_game_server_get(), path);
		variant = *game_engine_get_variant_by_name(&variant, "slayer");
		player_ui_set_game_variant(&variant);
		network_game_server_change_game_variant(global_network_game_server_get(), &variant);
		mcc_launcher.map_set = TRUE;
	}
	if (!mcc_launcher.player_added && mcc_launcher.setup_seconds >= MCC_LAUNCHER_PLAYER_SECONDS &&
		global_network_game_client_get())
	{
		mcc_launcher.player_added = network_game_client_add_player(global_network_game_client_get(), 0);
	}
	if (mcc_launcher.map_set && mcc_launcher.player_added && mcc_launcher.setup_seconds >= MCC_LAUNCHER_START_SECONDS)
	{
		network_game_client_request_immediate_start();
		platform_log("mcc launcher: a local game on %s", path);
		mcc_launcher.starting = NONE;
	}
}

static void mcc_launcher_choose(
	short entry_index)
{
	struct mcc_launcher_entry const *entry = &mcc_launcher_entries[entry_index];

	if (entry->multiplayer)
	{
		mcc_launcher.starting = entry_index;
		mcc_launcher_local_game_begin(entry->path);
	}
	else
	{
		platform_log("mcc launcher: the campaign's %s", entry->path);
		main_set_map_name(entry->path);
	}
}

/* ---------- public code */

boolean mcc_launcher_active(
	void)
{
	return mcc_launcher.active;
}

void mcc_launcher_update(
	boolean main_menu_loaded,
	real seconds)
{
	struct gamepad_state const *gamepad;
	boolean up;
	boolean down;
	boolean accept;

	if (!mcc_launcher.checked)
	{
		mcc_launcher.checked = TRUE;
		mcc_launcher.disabled = config_string("debug.network_test")[0] != 0;
		mcc_launcher.starting = NONE;
	}
	if (mcc_launcher.disabled)
		return;
	if (!main_menu_loaded || !mcc_cache_loaded())
	{
		mcc_launcher.active = FALSE;
		mcc_launcher.starting = NONE;
		return;
	}
	if (!mcc_launcher.active)
	{
		mcc_launcher.active = TRUE;
		mcc_launcher_list();
	}
	if (mcc_launcher.starting != NONE)
	{
		mcc_launcher_local_game_update(seconds);
		return;
	}
	/* (the main menu's own screens, which do not run on MCC's ui.map) */
	ui_widgets_close_all();

	/* (port 0 is the keyboard too, which drives it as the menus' controller
	while the launcher counts as a menu: ui_widget.c) */
	gamepad = input_get_gamepad_state(0);
	up = mcc_launcher_up(gamepad);
	down = mcc_launcher_down(gamepad);
	accept = mcc_launcher_accept(gamepad);
	if (mcc_launcher.shown_count > 0)
	{
		if (up && !mcc_launcher.held_up)
			mcc_launcher.selected = (short)((mcc_launcher.selected + mcc_launcher.shown_count - 1) % mcc_launcher.shown_count);
		if (down && !mcc_launcher.held_down)
			mcc_launcher.selected = (short)((mcc_launcher.selected + 1) % mcc_launcher.shown_count);
		if (mcc_launcher.selected < mcc_launcher.first_row)
			mcc_launcher.first_row = mcc_launcher.selected;
		if (mcc_launcher.selected >= mcc_launcher.first_row + MCC_LAUNCHER_VISIBLE_ROWS)
			mcc_launcher.first_row = (short)(mcc_launcher.selected - MCC_LAUNCHER_VISIBLE_ROWS + 1);
		if (accept && !mcc_launcher.held_accept)
			mcc_launcher_choose(mcc_launcher.shown[mcc_launcher.selected]);
	}
	mcc_launcher.held_up = up;
	mcc_launcher.held_down = down;
	mcc_launcher.held_accept = accept;
}

void mcc_launcher_draw(
	void)
{
	long font_index;
	struct font_header *font;
	short line_height;
	short row;
	short top;
	rectangle2d screen;
	rectangle2d bounds;
	real_argb_color color;
	char text[96];

	if (!mcc_launcher.active)
		return;
	font_index = interface_get_tag_index(_interface_font_terminal);
	if (font_index == NONE)
		return;
	font = font_definition_get(font_index);
	line_height = (short)(font->ascending_height + font->descending_height + font->leading_height + MCC_LAUNCHER_ROW_GAP);
	if (line_height <= MCC_LAUNCHER_ROW_GAP)
		return;
	screen = render.camera.window_bounds;
	offset_rectangle2d(&screen, (short)-render.camera.viewport_bounds.x0, (short)-render.camera.viewport_bounds.y0);
	/* a dark panel over the menu's scene */
	bounds = screen;
	draw_quad(&bounds, 0xB0000000);

	color.alpha = 1.0f;
	color.red = color.green = color.blue = 1.0f;
	top = (short)(screen.y0 + line_height * 2);
	bounds.y0 = top;
	bounds.y1 = (short)(top + line_height);
	draw_string_set_draw_mode(font_index, NONE, _mcc_launcher_text_centred, 0, &color);
	rasterizer_draw_string(&bounds, NULL, NULL, 0,
		mcc_launcher.starting != NONE ? "Starting the game..." :
		mcc_launcher.shown_count ? "MCC maps: up and down to choose, A or Enter to play" :
		"No MCC maps are in the maps folder");

	for (row = 0; row < MCC_LAUNCHER_VISIBLE_ROWS && mcc_launcher.first_row + row < mcc_launcher.shown_count; row++)
	{
		short shown_index = (short)(mcc_launcher.first_row + row);
		struct mcc_launcher_entry const *entry = &mcc_launcher_entries[mcc_launcher.shown[shown_index]];
		boolean selected = shown_index == mcc_launcher.selected;

		bounds.y0 = (short)(top + line_height * (row + 2));
		bounds.y1 = (short)(bounds.y0 + line_height);
		if (selected)
		{
			rectangle2d highlight = bounds;

			highlight.x0 = (short)(screen.x0 + (screen.x1 - screen.x0) / 4);
			highlight.x1 = (short)(screen.x1 - (screen.x1 - screen.x0) / 4);
			draw_quad(&highlight, 0xC0305080);
		}
		color.red = selected ? 1.0f : 0.75f;
		color.green = selected ? 1.0f : 0.8f;
		color.blue = selected ? 1.0f : 0.9f;
		draw_string_set_draw_mode(font_index, NONE, _mcc_launcher_text_centred, 0, &color);
		snprintf(text, sizeof(text), "%s%s", entry->title, entry->multiplayer ? "  (local slayer)" : "  (campaign)");
		rasterizer_draw_string(&bounds, NULL, NULL, 0, text);
	}
}
