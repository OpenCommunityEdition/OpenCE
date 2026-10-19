/* The PC version's menus on the Mac, over the loaded game map (HALO_MAC_MENU=1).
   menu_tags.c builds the XML's widgets, strings and bitmaps into tags; here
   they join the native tag registry (the cache_files.c table menu_tags.c
   grows on the other ports), and their pictures are drawn by
   engine_menu_art.c. */
#include "cseries.h"
#include "engine_tags.h"
#include "engine_menu.h"
#include "engine_menu_art.h"
#include "interface/ui_widget.h"
#include "interface/player_ui.h"
#include "game/players.h"
#include "game/local_players.h"
#include "input/input.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "saved games/player_profile.h"
#include "halo_menus.h"
#include "interface/hud.h"
#include "camera/director.h"
#include "camera/camera_scripting.h"
#include "interface/virtual_keyboard.h"
#define MAC_INPUT_NO_CARBON
#include "mac_input.h"
#include "engine_input.h"
#include "../linux/include/halo_keyboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* port/linux/game/menu_tags.c */
extern boolean menu_tags_standalone;
void menu_tags_loaded(char const *map_name);
void menu_tags_unloaded(void);
char const *pc_menus_root_name(void);
boolean pc_menu_art_size(void const *hardware_format, long *width, long *height);
void main_menu_active(boolean active);
char *main_get_map_name(void);
void event_manager_update(void);
/* the main.c view's (tools/mac_main_game_render_view.py) */
int mac_engine_main_wants_menu(int clear);
/* the maps main_set_map_name was given so far (the same view) */
long mac_engine_main_map_serial(void);
int mac_engine_main_map_change_deferred(void);

/* cache_files.c's tag table entry, as menu_tags.c lays it out */
struct menu_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};
typedef char verify_menu_tag_instance_size[sizeof(struct menu_tag_instance) == 0x20 ? 1 : -1];

#define MENU_DEFINITION_BYTES 0x3EC
#define MENU_OTHER_BYTES 16
/* a virtual keyboard definition (menu_tags.c makes one: the PC version's is ui.map's) */
#define MENU_KEYBOARD_BYTES 0x3C
#define MENU_PLACEHOLDERS 4096

static struct menu_tag_instance *menu_instances;
static struct menu_tag_instance *menu_original;
static long menu_instance_count;
static long menu_cache_count;
static boolean menu_started;
static boolean menu_view_scripted;
static boolean menu_ready;
static boolean menu_was_typing, menu_enter_armed;
static boolean menu_pause_pending, menu_escape_down, menu_escape_back_armed, menu_escape_was_down;
static void *menu_placeholders[MENU_PLACEHOLDERS];
static long menu_placeholder_count;

/* port: local_player_count, 0 before the players' globals are made (source/game/players.c) */
short players_port_local_player_count(void);

int mac_engine_menu_requested(void)
{
	const char *value = getenv("HALO_MAC_MENU");

	return value && value[0] == '1' && !value[1];
}

int config_boolean(const char *name);
long config_integer(const char *name);

/* the diagnostic HALO_MAC_MENU=1, or the setting game.start_in_menu */
int mac_engine_menu_enabled(void)
{
	return mac_engine_menu_requested() || config_boolean("game.start_in_menu");
}

/* the Mac's virtual key codes of the keys that stand for pad buttons */
#define KEY_RETURN 36
#define KEY_KEYPAD_ENTER 76
#define KEY_BACKSPACE 51
#define KEY_ESCAPE 53
#define KEY_LEFT 123
#define KEY_RIGHT 124
#define KEY_DOWN 125
#define KEY_UP 126
#define KEY_SPACE 49
#define KEY_F1 122
/* the pad's analog and binary buttons (input.h) */
#define PAD_A 0x01u
#define PAD_B 0x02u
#define PAD_UP 0x01u
#define PAD_DOWN 0x02u
#define PAD_LEFT 0x04u
#define PAD_RIGHT 0x08u
#define PAD_START 0x10u
#define PAD_BACK 0x20u

void mac_engine_menu_input(const struct mac_input_frame *frame)
{
	unsigned analog = 0, binary = 0;

	if (frame && mac_engine_menu_enabled() && mac_engine_input_text_typing())
	{
		/* port: the on-screen keyboard or a text field is up: the keys type into it (as the shared
		   backend's typing_gamepad), so Backspace erases and Space types a space instead of pressing B and
		   A (which cancelled the name). Only the arrows move about the keyboard, Return is Done (once it
		   has been let go of since the keyboard came up) and Escape cancels. */
		if (!menu_was_typing)
			menu_enter_armed = FALSE;
		menu_was_typing = TRUE;
		if (frame->keys[KEY_UP])
			binary |= PAD_UP;
		if (frame->keys[KEY_DOWN])
			binary |= PAD_DOWN;
		if (frame->keys[KEY_LEFT])
			binary |= PAD_LEFT;
		if (frame->keys[KEY_RIGHT])
			binary |= PAD_RIGHT;
		if (!frame->keys[KEY_RETURN] && !frame->keys[KEY_KEYPAD_ENTER])
			menu_enter_armed = TRUE;
		else if (menu_enter_armed)
			binary |= PAD_START;
		if (frame->keys[KEY_ESCAPE])
			analog |= PAD_B;
		menu_escape_was_down = frame->keys[KEY_ESCAPE] != 0;
	}
	else if (frame && mac_engine_menu_enabled())
	{
		menu_was_typing = FALSE;
		if (frame->keys[KEY_F1])
			binary |= PAD_BACK;
		if (frame->keys[KEY_UP])
			binary |= PAD_UP;
		if (frame->keys[KEY_DOWN])
			binary |= PAD_DOWN;
		if (frame->keys[KEY_LEFT])
			binary |= PAD_LEFT;
		if (frame->keys[KEY_RIGHT])
			binary |= PAD_RIGHT;
		if (frame->keys[KEY_RETURN] || frame->keys[KEY_KEYPAD_ENTER] || frame->keys[KEY_SPACE])
			analog |= PAD_A;
		if (frame->keys[KEY_BACKSPACE])
			analog |= PAD_B;
		/* port: Escape is Back in a menu; the pause menu opens by the pause
		   control (controls.pause, Escape unless Controls Setup changed it) */
		{
			int pause_held = mac_engine_input_bound_held(HALO_KEYBOARD_PAUSE, frame);

			/* port: Escape is Back only if it went down while a menu was already up: the press (or hold)
			   that opened the pause menu must not close it on the next frame as Back, so it is armed by
			   a fresh press and disarmed by the release (a second press closes the menu, as on the PC) */
			if (!frame->keys[KEY_ESCAPE] || !ui_widgets_active())
				menu_escape_back_armed = FALSE;
			else if (!menu_escape_was_down)
				menu_escape_back_armed = TRUE;
			menu_escape_was_down = frame->keys[KEY_ESCAPE] != 0;
			if (menu_escape_back_armed)
				analog |= PAD_B;
			else if (frame->keys[KEY_ESCAPE] && ui_widgets_active())
				;
			else if (pause_held && !menu_escape_down && !ui_widgets_active())
			{
				/* port: with a split screen the pause control is player 1's Start:
				the game opens their own pause menu, on their part of the screen */
				if (players_port_local_player_count() > 1)
					binary |= PAD_START;
				else
					menu_pause_pending = TRUE;
			}
			menu_escape_down = pause_held;
		}
	}
	mac_engine_input_keyboard_pad(analog, binary);
}

/* ---------- local players (split screen)

   As main.c's create_local_players makes a campaign's (player_spawn_count of
   them, on the controllers the menus chose: co-op's), and a network game's
   lobby has them join: a multiplayer game here plays the players the menus
   joined (a controller's START, or ADD PLAYER: menu_functions.c's lobby),
   player 1 if there are none. */
extern short player_spawn_count;

/* port: the players of this game, each on its controller (their local
   player index): the first's player index, NONE if none */
long mac_engine_menu_create_local_players(void)
{
	short controllers[MAXIMUM_GAMEPADS];
	short count = 0, index, controller;
	long first = NONE;
	boolean multiplayer = game_engine != NULL;

	/* port: debug.mac_local_players (HALO_MAC_LOCAL_PLAYERS), for a hidden or
	   scripted run: the players the menus would have chosen. Their pads are
	   the virtual pads (engine_input.c). A campaign has 2 at most, as the
	   co-op menu makes. */
	{
		long wanted = config_integer("debug.mac_local_players");

		if (wanted > MAXIMUM_GAMEPADS)
			wanted = MAXIMUM_GAMEPADS;
		if (wanted > 1)
		{
			if (multiplayer)
			{
				for (controller = 0; controller < wanted; controller++)
					player_ui_local_player_joined_multiplayer_game(controller);
			}
			else if (player_spawn_count < wanted)
				player_spawn_count = (short)(wanted > 2 ? 2 : wanted);
		}
	}
	memset(controllers, NONE, sizeof(controllers));
	if (multiplayer)
	{
		for (controller = 0; controller < MAXIMUM_GAMEPADS; controller++)
			if (player_ui_local_player_wants_to_play_multiplayer(controller))
				controllers[count++] = controller;
	}
	else
	{
		boolean used[MAXIMUM_GAMEPADS] = { FALSE, FALSE, FALSE, FALSE };

		for (index = 0; index < player_spawn_count && index < MAXIMUM_GAMEPADS; index++)
		{
			short wanted = player_ui_get_single_player_local_player_controller(index);

			if (wanted == NONE || wanted < 0 || wanted >= MAXIMUM_GAMEPADS)
				wanted = index;
			/* (a controller two players chose: the first that is free) */
			for (controller = 0; used[wanted] && controller < MAXIMUM_GAMEPADS; controller++)
				if (!used[controller])
					wanted = controller;
			used[wanted] = TRUE;
		}
		for (controller = 0; controller < MAXIMUM_GAMEPADS; controller++)
			if (used[controller])
				controllers[count++] = controller;
	}
	if (!count)
		controllers[count++] = 0;
	for (index = 0; index < count; index++)
	{
		long player;

		if (multiplayer && count > 1)
		{
			struct network_player named;
			struct player_profile profile;

			memset(&named, 0, sizeof(named));
			player_ui_get_active_player_profile(controllers[index], &profile);
			ustrncpy(named.name, profile.player_name, 11);
			player = player_new(0, NONE, controllers[index], &named);
		}
		else
			player = player_new(0, NONE, controllers[index], NULL);
		if (player == NONE)
			return first;
		local_player_set_player_index(controllers[index], player);
		if (first == NONE)
			first = player;
	}
	return first;
}

static char menu_pending_map[256];
/* port: a network game asked for a session of its map (engine_network_test.c) */
static int menu_game_requested;

void mac_engine_menu_request_game(const char *map_name)
{
	strncpy(menu_pending_map, map_name, sizeof(menu_pending_map) - 1);
	menu_pending_map[sizeof(menu_pending_map) - 1] = 0;
	menu_game_requested = 1;
}

const char *mac_engine_menu_pending_map(void)
{
	return menu_pending_map;
}

/* a game chosen (main_set_map_name, which the menus call), or the main menu
asked for (main_goto_main_menu, the pause menu's leave) */
int mac_engine_menu_poll(void)
{
	static long seen;
	long serial = mac_engine_main_map_serial();
	char *name = main_get_map_name();

	if (menu_game_requested)
	{
		menu_game_requested = 0;
		mac_engine_main_wants_menu(1);
		fprintf(stderr, "menus: the network game %s is started\n", menu_pending_map);
		return MAC_MENU_REQUEST_START;
	}
	/* (the name stays: the pause menu's Quit saves the campaign's level it names) */
	/* port: the map is named at the level list and the game starts once the menus have chosen it (the
	difficulty menu's OK: main_menu_switch_to_single_player), as the Xbox's loop changes the map only when it
	is deferred: a map named and no more (main_defer_map_map_change) is not a game yet, and the difficulty is asked */
	if (serial != seen && name && name[0] && mac_engine_main_map_change_deferred())
	{
		seen = serial;
		strncpy(menu_pending_map, name, sizeof(menu_pending_map) - 1);
		menu_pending_map[sizeof(menu_pending_map) - 1] = 0;
		mac_engine_main_wants_menu(1);
		fprintf(stderr, "menus: the game %s is started\n", menu_pending_map);
		return MAC_MENU_REQUEST_START;
	}
	if (!(serial != seen && name && name[0]))
		seen = serial;
	if (mac_engine_main_wants_menu(1))
		return menu_started ? MAC_MENU_REQUEST_NONE : MAC_MENU_REQUEST_MENU;
	return MAC_MENU_REQUEST_NONE;
}

/* port: an Xbox map's own table and its setter (cache_files.c, renamed by the native view) */
void *cache_files_tag_instances_retail_legacy(long *count);
void cache_files_set_tag_instances_retail_legacy(void *instances, long count);
extern int mac_cache_files_xbox_active(void);

/* the table of the registry's tags, for menu_tags.c to grow */
void *cache_files_tag_instances(long *count)
{
	if (mac_cache_files_xbox_active())
		return cache_files_tag_instances_retail_legacy(count);
	struct mac_native_tags *tags = mac_engine_tags_active();
	long total = tags ? (long)mac_native_tags_count(tags) : 0, index;
	struct menu_tag_instance *table;

	if (!tags || !total)
		return NULL;
	table = (struct menu_tag_instance *)calloc((size_t)total, sizeof(*table));
	if (!table)
		return NULL;
	for (index = 0; index < total; index++)
	{
		uint32_t handle = UINT32_MAX, group = UINT32_MAX;
		const char *name = "";
		void *root = mac_native_tags_root(tags, (uint32_t)index, &handle, &group, &name);

		table[index].group_tag = handle == 0 ? NONE : (long)group;
		table[index].parent_group_tags[0] = NONE;
		table[index].parent_group_tags[1] = NONE;
		table[index].tag_index = handle == 0 ? NONE : (long)handle;
		table[index].name = (char *)name;
		table[index].base_address = root;
	}
	free(menu_original);
	menu_original = table;
	menu_instances = table;
	menu_instance_count = total;
	menu_cache_count = total;
	if (count)
		*count = total;
	return table;
}

/* the table after menu_tags.c grew it (its new entries from the old count),
or cut back to the old one */
void cache_files_set_tag_instances(void *instances, long count)
{
	if (mac_cache_files_xbox_active())
	{
		cache_files_set_tag_instances_retail_legacy(instances, count);
		return;
	}
	struct mac_native_tags *tags = mac_engine_tags_active();
	long old_count = tags ? (long)mac_native_tags_count(tags) : 0, index;
	struct menu_tag_instance *table = (struct menu_tag_instance *)instances;

	if (!tags || !table || count < 0)
		return;
	if (count < old_count)
	{
		mac_native_tags_truncate(tags, (uint32_t)count);
		menu_instances = menu_original;
		menu_instance_count = count;
		return;
	}
	if (count == old_count)
		return;
	if (!mac_native_tags_extend(tags, (uint32_t)count))
	{
		fprintf(stderr, "menus: %ld tags are too many for the registry\n", count);
		return;
	}
	for (index = old_count; index < count; index++)
	{
		struct menu_tag_instance *instance = &table[index];
		void *root = instance->base_address;
		size_t bytes = MENU_OTHER_BYTES;

		if (instance->tag_index == NONE || instance->group_tag == NONE)
			continue;
		if (instance->group_tag == 'vcky')
		{
			/* (built after the table is given, as the widgets') */
			bytes = MENU_KEYBOARD_BYTES;
			if (!root)
			{
				root = calloc(1, MENU_KEYBOARD_BYTES);
				if (root && menu_placeholder_count < MENU_PLACEHOLDERS)
					menu_placeholders[menu_placeholder_count++] = root;
			}
		}
		if (instance->group_tag == 'DeLa')
		{
			bytes = MENU_DEFINITION_BYTES;
			if (!root)
			{
				/* (built after the table is given: the root is put here when it is) */
				root = calloc(1, MENU_DEFINITION_BYTES);
				if (root && menu_placeholder_count < MENU_PLACEHOLDERS)
					menu_placeholders[menu_placeholder_count++] = root;
			}
		}
		if (!root || !mac_native_tags_define(tags, (uint32_t)index, (uint32_t)instance->tag_index,
			(uint32_t)instance->group_tag, instance->name ? instance->name : "", root, bytes))
			fprintf(stderr, "menus: tag %ld (%s) is not in the registry\n", index, instance->name ? instance->name : "");
	}
	menu_instances = table;
	menu_instance_count = count;
}

/* the widget definitions menu_tags.c built after it gave its table */
static void menu_roots_sync(void)
{
	struct mac_native_tags *tags = mac_engine_tags_active();
	long index;

	if (!tags || !menu_instances)
		return;
	for (index = menu_cache_count; index < menu_instance_count; index++)
	{
		struct menu_tag_instance *instance = &menu_instances[index];

		if (instance->tag_index != NONE && (instance->group_tag == 'DeLa' || instance->group_tag == 'vcky') && instance->base_address)
			mac_native_tags_set_root(tags, (uint32_t)instance->tag_index, instance->base_address);
	}
}

/* a menu texture's pictures, by the memory its texels are in (the texture
the binder is given is not the pointer the bitmap keeps) */
#define MENU_MAXIMUM_ART 1024
static struct
{
	const void *texture;
	unsigned long data;
} menu_art[MENU_MAXIMUM_ART];
static long menu_art_count;

void halo_menus_art_register(void const *texture, char const *png)
{
	const D3DBaseTexture *base = (const D3DBaseTexture *)texture;

	if (getenv("HALO_MAC_MENU_DEBUG"))
		fprintf(stderr, "menus: art %p data %08lx %s\n", texture, base ? (unsigned long)base->Data : 0UL, png);
	if (base && base->Data && menu_art_count < MENU_MAXIMUM_ART)
	{
		menu_art[menu_art_count].texture = texture;
		menu_art[menu_art_count].data = (unsigned long)base->Data;
		menu_art_count++;
	}
	mac_menu_art_register(texture, png);
}

void halo_menus_art_forget(void)
{
	menu_art_count = 0;
	mac_menu_art_forget();
}

int mac_engine_menu_art_resolve(const void *resource, unsigned char sampler,
	unsigned int *name, float scale[4])
{
	long width = 0, height = 0;
	int result;

	const D3DBaseTexture *base = (const D3DBaseTexture *)resource;
	const void *texture = NULL;
	long index;

	if (!menu_ready || !base || !base->Data)
		return 0;
	for (index = 0; index < menu_art_count; index++)
	{
		if (menu_art[index].data == (unsigned long)base->Data)
		{
			texture = menu_art[index].texture;
			break;
		}
	}
	if (!texture || !pc_menu_art_size(texture, &width, &height))
		return 0;
	if (sampler != 1)
		return -1;
	result = mac_menu_art_texture(texture, width, height, name, scale);
	{
		static long logged;

		if (getenv("HALO_MAC_MENU_DEBUG") && logged < 30)
		{
			fprintf(stderr, "menus: art resolve %ldx%ld result %d name %u scale %g %g\n", width, height, result,
				result > 0 ? *name : 0U, result > 0 ? scale[0] : 0.0, result > 0 ? scale[1] : 0.0);
			logged++;
		}
	}
	return result;
}

/* The PC version asks for a name (its keyboard) when there is no player profile. */
static void menu_profile_ensure(void)
{
	word count = 1;
	long index = NONE;
	struct player_profile profile;

	player_profiles_enumerate_available_to_local_player_index(NONE, &count, &index, FALSE);
	if ((short)count > 0)
		return;
	/* port: as on the PC (and port/linux), with no profile the main menu's first item asks for a name
	(the on-screen keyboard, or the keys typed: menu_functions.c's profile_edit_begin and the items'
	"new game if no plyr profiles"), and the profile is made with it. A diagnostic run (scripted
	frames, HALO_MAC_RENDER_FRAMES) still gets Player at once, unless HALO_MAC_MENU_ASK_PROFILE is set. */
	if (!getenv("HALO_MAC_RENDER_FRAMES") || getenv("HALO_MAC_MENU_ASK_PROFILE"))
	{
		fprintf(stderr, "menus: no player profile: the menus ask for a name\n");
		return;
	}
	index = player_profile_new(0, L"Player");
	if (index != NONE && player_profile_get(index, &profile))
	{
		player_ui_set_single_player_local_player_controller(0, 0);
		player_ui_set_active_player_profile(0, index, &profile);
		fprintf(stderr, "menus: made the player profile Player\n");
	}
	else
		fprintf(stderr, "menus: could not make a player profile\n");
}

int mac_engine_menu_start(const char *map_name, int main_menu)
{
	struct mac_native_tags *tags = mac_engine_tags_active();
	const char *root;

	if (menu_started || menu_ready)
		return 1;
	/* port: an Xbox map's own tags are the engine's (cache_files.c): its loader built the
	menus' tags when it read the map (ui.map's main menu, or a game map's pause menu),
	and ui.map has the on-screen keyboard */
	if (!tags && !mac_cache_files_xbox_active())
	{
		fprintf(stderr, "menus: no tags are bound\n");
		return 0;
	}
	if (!mac_cache_files_xbox_active())
	{
		menu_tags_standalone = TRUE;
		menu_tags_loaded(map_name);
		menu_roots_sync();
	}
	root = pc_menus_root_name();
	/* (an Xbox game map has its pause screen only: the main menu is ui.map's) */
	if (tag_loaded('DeLa', mac_cache_files_xbox_active() && !main_menu ? "pc\\pause\\pause_screen" : root) == NONE)
	{
		fprintf(stderr, "menus: the main menu is not in the tags\n");
		if (!mac_cache_files_xbox_active())
			menu_tags_unloaded();
		return 0;
	}
	menu_ready = TRUE;
	/* (the on-screen keyboard menu_tags.c made, for the names the menus ask) */
	if (!virtual_keyboard_initialize())
		fprintf(stderr, "menus: no on-screen keyboard\n");
	menu_profile_ensure();
	if (!main_menu)
	{
		/* (in a game: the pause menu opens on Escape) */
		fprintf(stderr, "menus: the pause menu is ready\n");
		return 1;
	}
	fprintf(stderr, "menus: root widget %s\n", root);
	menu_started = TRUE;
	main_menu_active(TRUE);
	if (!ui_widget_load_by_name_or_tag(root, NONE, NULL, NONE, NONE, NONE, NONE))
	{
		fprintf(stderr, "menus: the main menu did not open\n");
		main_menu_active(FALSE);
		menu_started = FALSE;
		menu_ready = FALSE;
		if (!mac_cache_files_xbox_active())
			menu_tags_unloaded();
		return 0;
	}
	fprintf(stderr, "menus: the main menu is open\n");
	/* port: the title music (main_screen_shell_load starts it) */
	ui_start_main_menu_music();
	fprintf(stderr, "menus: title music %s\n", ui_main_menu_music_active() ? "started" :
		"not started (the map has no sound\\music\\title1\\title1)");
	return 1;
}

int mac_engine_menu_active(void)
{
	return menu_started;
}

#define PAUSE_SCREEN "pc\\pause\\pause_screen"

void mac_engine_menu_tick(void)
{
	if (menu_ready)
	{
		static long ticks;
		static int debug = -1;

		/* (Escape in a game: the pause menu, if no widget is up) */
		if (menu_pause_pending)
		{
			menu_pause_pending = FALSE;
			if (!menu_started && !ui_widgets_active() && game_in_progress())
			{
				if (!ui_widget_load_by_name_or_tag(PAUSE_SCREEN, NONE, NULL, NONE, NONE, NONE, NONE))
					fprintf(stderr, "menus: the pause menu did not open\n");
			}
		}
		/* (the main menu is drawn over the map's world with no HUD) */
		if (menu_started)
		{
			scripted_show_hud(FALSE);
			scripted_show_hud_help_text(FALSE);
			/* port: nothing of the player draws behind the main menu: the
			   scripts' camera is the view (as the title map's), not the
			   player's first-person weapon of a game's map used instead */
			if (!menu_view_scripted && game_in_progress() && global_scenario_try_and_get() &&
				global_scenario_try_and_get()->cutscene_camera_points.count > 0)
			{
				director_script_camera(TRUE);
				scripted_camera_set(0, 0, NONE);
				menu_view_scripted = TRUE;
			}
		}
		event_manager_update();
		process_ui_widgets();
		if (debug < 0)
		{
			const char *value = getenv("HALO_MAC_MENU_DEBUG");

			debug = value && value[0] == '1';
		}
		if (debug && ticks < 8)
			fprintf(stderr, "menus: tick %ld, widgets active %d\n", ticks, (int)ui_widgets_active());
		ticks++;
	}
}

void mac_engine_menu_stop(void)
{
	long index;

	if (!menu_ready)
		return;
	ui_widgets_close_all();
	ui_stop_main_menu_music();
	virtual_keyboard_dispose();
	main_menu_active(FALSE);
	/* (an Xbox map's menu tags go with the map: scenario_tags_unload) */
	if (!mac_cache_files_xbox_active())
		menu_tags_unloaded();
	menu_started = FALSE;
	menu_view_scripted = FALSE;
	menu_ready = FALSE;
	for (index = 0; index < menu_placeholder_count; index++)
		free(menu_placeholders[index]);
	menu_placeholder_count = 0;
	free(menu_original);
	menu_original = NULL;
	menu_instances = NULL;
	menu_instance_count = 0;
}
