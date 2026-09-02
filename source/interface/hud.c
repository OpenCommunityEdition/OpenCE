/*
HUD.C

symbols in this file:
000BF660 0030:
	_code_000bf660 (0000)
000BF690 0050:
	_hud_initialize (0000)
000BF6E0 0020:
	_hud_dispose (0000)
000BF700 0080:
	_hud_initialize_for_new_map (0000)
000BF780 0020:
	_hud_dispose_from_old_map (0000)
000BF7A0 0020:
	_scripted_show_hud (0000)
000BF7C0 0020:
	_scripted_show_hud_help_text (0000)
000BF7E0 0060:
	_hud_update (0000)
000BF840 0030:
	_code_000bf840 (0000)
000BF870 06d0:
	_code_000bf870 (0000)
000BFF40 0050:
	_hud_get_item_string (0000)
000BFF90 0020:
	_hud_picked_up_grenade (0000)
000BFFB0 0030:
	_hud_picked_up_ammunition (0000)
000BFFE0 0020:
	_hud_picked_up_weapon (0000)
000C0000 0020:
	_hud_picked_up_powerup (0000)
000C0020 00d0:
	_code_000c0020 (0000)
000C00F0 0060:
	_hud_load (0000)
000C0150 00e0:
	_hud_autosave (0000)
000C0230 0160:
	_hud_draw_friendly_indicator (0000)
000C0390 00a0:
	_hud_draw_players (0000)
000C0430 0370:
	_code_000c0430 (0000)
000C07A0 0140:
	_hud_draw_screen (0000)
0026FC1C 0015:
	??_C@_0BF@NENKDAEK@hud_scripted_globals?$AA@ (0000)
0026FC34 001f:
	??_C@_0BP@ILJLOMEO@c?3?2halo?2SOURCE?2interface?2hud?4c?$AA@ (0000)
0026FC54 0015:
	??_C@_0BF@MENHAMGO@hud?5scripted?5globals?$AA@ (0000)
0026FC6C 0036:
	??_C@_0DG@OKALAJAB@interface_get_tag_index?$CI_interfa@ (0000)
0026FCA4 0013:
	??_C@_0BD@EAHBLILM@ACTIVE?9CAMOUFLAGE?5?$AA@ (0000)
0026FCB8 0016:
	??_C@_0BG@GFCGCBJE@FULL?9SPECTRUM?5VISION?5?$AA@ (0000)
0026FCD0 0005:
	??_C@_04OPODHAPJ@?$CFs?$HMn?$AA@ (0000)
0026FCD8 0037:
	??_C@_0DH@GKJDMEOF@?$CFs?$HMntotal?5?$CFd?1?$CFd?$HMnloaded?5?$CFd?1?$CFd?$HMnh@ (0000)
0026FD10 0008:
	??_C@_07IFCAFJHP@?$CFd?5?$CFs?$HMn?$AA@ (0000)
0026FD18 001a:
	??_C@_0BK@NJIPBDGF@?$CFs?5?$CIpress?5WHITE?5to?5use?$CJ?$HMn?$AA@ (0000)
002E4C34 0004:
	_default_string (0000)
00453AAC 0008:
	_hud_globals (0000)
	_hud_scripted_globals (0004)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "bitmaps/bitmap_group.h"
#include "cache/texture_cache.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "interface/hud.h"
#include "interface/hud_messaging.h"
#include "interface/hud_unit.h"
#include "interface/hud_weapon.h"
#include "interface/interface.h"
#include "interface/motion_sensor.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "render/render.h"
#include "render/render_camera_projection.h"
#include "saved games/game_state.h"
#include "sound/game_sound.h"
#include "text/text_group.h"
#include "units/units.h"

/* ---------- constants */

/* ---------- macros */

#define hud_globals_definition_get(index) \
	((struct hud_globals_definition *)tag_get('hudg', (index)))

/* ---------- structures */

struct hud_scripted_globals
{
	boolean show_hud;
	boolean show_hud_help_text;
	byte reserved2[2];
};

struct hud_globals_definition
{
	byte reserved000[0x94];
	struct tag_reference hud_item_messages;
	byte reserved0A4[0x334];
	short loading_begin_index;
	short loading_end_index;
	short checkpoint_begin_index;
	short checkpoint_end_index;
	struct tag_reference checkpoint_sound;
};

typedef char hud_scripted_globals_size_assert[
	sizeof(struct hud_scripted_globals) == 0x4 ? 1 : -1];
typedef char hud_scripted_globals_show_hud_help_text_offset_assert[
	offsetof(struct hud_scripted_globals, show_hud_help_text) == 0x1 ? 1 : -1];
typedef char hud_globals_hud_item_messages_index_offset_assert[
	offsetof(struct hud_globals_definition, hud_item_messages.index) == 0xA0 ? 1 : -1];
typedef char hud_globals_loading_begin_index_offset_assert[
	offsetof(struct hud_globals_definition, loading_begin_index) == 0x3D8 ? 1 : -1];
typedef char hud_globals_checkpoint_sound_index_offset_assert[
	offsetof(struct hud_globals_definition, checkpoint_sound.index) == 0x3EC ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

extern struct hud_globals_definition *hud_globals;
extern struct hud_scripted_globals *hud_scripted_globals;

wchar_t const *default_string = L"";

/* ---------- public code */

void hud_initialize(
	void)
{
	hud_scripted_globals = game_state_malloc(
		"hud scripted globals",
		NULL,
		sizeof(*hud_scripted_globals));
	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud.c",
		87,
		hud_scripted_globals);
	hud_messaging_initialize();
	hud_initialize_unit_interface();
	hud_initialize_weapon_interface();
	hud_initialize_nav_points();
	motion_sensor_initialize();
	return;
}

void hud_dispose(
	void)
{
	motion_sensor_dispose();
	hud_dispose_nav_points();
	hud_dispose_weapon_interface();
	hud_dispose_unit_interface();
	hud_messaging_dispose();
	return;
}

void hud_initialize_for_new_map(
	void)
{
	csmemset(hud_scripted_globals, 0, sizeof(*hud_scripted_globals));
	hud_scripted_globals->show_hud = TRUE;
	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud.c",
		113,
		interface_get_tag_index(_interface_hud_globals)!=NONE);
	hud_globals = hud_globals_definition_get(
		interface_get_tag_index(_interface_hud_globals));
	hud_messaging_initialize_for_new_map();
	hud_initialize_unit_interface_for_new_map();
	hud_initialize_weapon_interface_for_new_map();
	hud_initialize_nav_points_for_new_map();
	motion_sensor_initialize_for_new_map();
	return;
}

void hud_dispose_from_old_map(
	void)
{
	motion_sensor_dispose_from_old_map();
	hud_dispose_nav_points_from_old_map();
	hud_dispose_weapon_interface_from_old_map();
	hud_dispose_unit_interface_from_old_map();
	hud_messaging_dispose_from_old_map();
	return;
}

boolean scripted_show_hud(
	boolean show)
{
	hud_scripted_globals->show_hud = show;
	return hud_scripted_globals->show_hud;
}

boolean scripted_show_hud_help_text(
	boolean show)
{
	hud_scripted_globals->show_hud_help_text = show;
	return hud_scripted_globals->show_hud_help_text;
}

void hud_update(
	void)
{
	short local_player_index;
	long player_index;

	hud_update_weapon();
	hud_update_unit();
	hud_update_nav_points();
	hud_messaging_globals_update();
	if (game_engine_force_single_screen())
	{
		for (local_player_index = 0;
			local_player_index < MAXIMUM_LOCAL_PLAYERS;
			local_player_index++)
		{
			player_index = local_player_get_player_index(local_player_index);
			if (player_index != NONE)
			{
				hud_play_unit_sounds(
					datum_get(player_data, player_index),
					FALSE);
			}
		}
	}
	return;
}

wchar_t const *hud_get_item_string(
	long string_index)
{
	long string_list_index = hud_globals->hud_item_messages.index;
	wchar_t const *string = default_string;

	if (string_list_index != NONE)
	{
		struct string_list const *string_list =
			unicode_string_list_definition_get(string_list_index);
		if (string_list && VALID_INDEX(string_index, string_list->strings.count))
		{
			string = unicode_string_list_get_string(
				string_list_index,
				(short)string_index);
		}
	}

	return string;
}

void hud_picked_up_grenade(
	short local_player_index,
	long grenade_definition_index)
{
	if (local_player_index != NONE)
	{
		hud_add_item_message(
			local_player_index,
			grenade_definition_index,
			1,
			-1);
	}

	return;
}

void hud_picked_up_ammunition(
	short local_player_index,
	long weapon_definition_index,
	short ammunition_count)
{
	if (local_player_index != NONE)
	{
		hud_add_item_message(
			local_player_index,
			weapon_definition_index,
			ammunition_count,
			1);
	}

	return;
}

void hud_picked_up_weapon(
	short local_player_index,
	long weapon_definition_index)
{
	if (local_player_index != NONE)
	{
		hud_add_item_message(
			local_player_index,
			weapon_definition_index,
			0,
			0);
	}

	return;
}

void hud_picked_up_powerup(
	short local_player_index,
	long powerup_definition_index)
{
	if (local_player_index != NONE)
	{
		hud_add_item_message(
			local_player_index,
			powerup_definition_index,
			0,
			0);
	}

	return;
}

void hud_load(
	boolean load)
{
	short string_index = load
		? hud_globals->loading_begin_index
		: hud_globals->loading_end_index;

	scripted_hud_messages_clear();
	if (string_index != NONE)
	{
		hud_print_message(
			local_player_get_next(NONE),
			hud_get_item_string(string_index));
	}

	return;
}

void hud_autosave(
	boolean active)
{
	short string_index = active
		? hud_globals->checkpoint_begin_index
		: hud_globals->checkpoint_end_index;
	short local_player_index;

	scripted_hud_messages_clear();
	if (active && hud_globals->checkpoint_sound.index != NONE)
	{
		unspatialized_impulse_sound_new(
			hud_globals->checkpoint_sound.index,
			1.0f);
	}

	if (string_index != NONE)
	{
		for (local_player_index = 0;
			local_player_index < MAXIMUM_LOCAL_PLAYERS;
			local_player_index++)
		{
			if (local_player_get_player_index(local_player_index) != NONE)
			{
				hud_print_message(
					local_player_index,
					hud_get_item_string(string_index));
			}
		}
	}

	return;
}

static void hud_draw_friendly_indicator(
	long player_index)
{
	struct player_datum const *player = player_get(player_index);
	real_point3d head_position;
	real_point3d view_position;
	real_point2d screen_position;
	point2d position;
	real_rectangle2d clip;
	struct bitmap_data *bitmap;
	real depth_factor;
	real fade;

	unit_get_head_position(
		player->unit_index,
		&head_position);
	head_position.z += 0.30000001f;
	matrix4x3_transform_point(
		&render.frustum.world_to_view,
		&head_position,
		&view_position);
	if (render_camera_view_to_screen(
		&render.camera,
		&render.frustum,
		&view_position,
		&screen_position))
	{
		bitmap = bitmap_group_get_bitmap_from_sequence(
			interface_get_tag_index(_interface_bitmap_multiplayer_hud),
			0,
			0);
		if (_texture_cache_bitmap_get_hardware_format(bitmap, FALSE, TRUE))
		{
			position.x =
				(short)fast_ftol(screen_position.x) - render.camera.viewport_bounds.x0;
			position.y =
				(short)fast_ftol(screen_position.y) - render.camera.viewport_bounds.y0;
			depth_factor = 1.0f -
				((-view_position.z - render.frustum.z_near) * 100.0f) /
				(render.frustum.z_far - render.frustum.z_near);
			fade = PIN(depth_factor, 0.075000003f, 1.0f);
			clip.x0 = 0.0f;
			clip.x1 = 1.0f;
			clip.y0 = (1.0f - fade) * 0.5f;
			clip.y1 = 1.0f;
			interface_draw_bitmap(
				bitmap,
				&position,
				&clip,
				1.0f,
				0.0f,
				1.0f);
		}
	}

	return;
}

void hud_draw_players(
	void)
{
	long player_index = local_player_get_player_index(render.local_player_index);
	long team_index = player_get(player_index)->team_index;

	if (player_index != NONE)
	{
		long teammate_indices[16];
		long teammate_count = 0;
		struct data_iterator iterator;
		struct player_datum *player;
		long teammate_index;

		data_iterator_new(&iterator, player_data);
		while ((player = data_iterator_next(&iterator)) != NULL)
		{
			if (player_index != iterator.datum_index &&
				team_index == player->team_index &&
				player->unit_index != NONE)
			{
				teammate_indices[teammate_count++] = iterator.datum_index;
			}
		}

		for (teammate_index = 0;
			teammate_index < teammate_count;
			teammate_index++)
		{
			hud_draw_friendly_indicator(teammate_indices[teammate_index]);
		}
	}

	return;
}

/* ---------- private code */
