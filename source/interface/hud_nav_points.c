/*
HUD_NAV_POINTS.C

symbols in this file:
000C5240 0080:
	_find_nav_point (0000)
000C52C0 0070:
	_get_nav_point_datum (0000)
000C5330 0040:
	_hud_initialize_nav_points (0000)
000C5370 0020:
	_hud_initialize_nav_points_for_new_map (0000)
000C5390 0010:
	_hud_dispose_nav_points_from_old_map (0000)
000C53A0 0010:
	_hud_dispose_nav_points (0000)
000C53B0 00f0:
	_code_000c53b0 (0000)
000C54A0 0020:
	_hud_activate_nav_point_with_game_engine_flag (0000)
000C54C0 0020:
	_hud_activate_nav_point_with_flag (0000)
000C54E0 0020:
	_hud_activate_nav_point_with_object (0000)
000C5500 0070:
	_code_000c5500 (0000)
000C5570 0030:
	_hud_activate_team_nav_point_with_game_engine_flag (0000)
000C55A0 0030:
	_hud_activate_team_nav_point_with_flag (0000)
000C55D0 0030:
	_hud_activate_team_nav_point_with_object (0000)
000C5600 0070:
	_code_000c5600 (0000)
000C5670 0030:
	_hud_activate_global_nav_point_with_game_engine_flag (0000)
000C56A0 0070:
	_code_000c56a0 (0000)
000C5710 0020:
	_hud_deactivate_nav_point_with_game_engine_flag (0000)
000C5730 0020:
	_hud_deactivate_nav_point_with_flag (0000)
000C5750 0020:
	_hud_deactivate_nav_point_with_object (0000)
000C5770 0060:
	_code_000c5770 (0000)
000C57D0 0020:
	_hud_deactivate_team_nav_point_with_flag (0000)
000C57F0 0020:
	_hud_deactivate_team_nav_point_with_object (0000)
000C5810 0030:
	_hud_unit_activate_nav_point_with_flag (0000)
000C5840 0030:
	_hud_unit_activate_nav_point_with_object (0000)
000C5870 0030:
	_hud_unit_deactivate_nav_point_with_flag (0000)
000C58A0 0030:
	_hud_unit_deactivate_nav_point_with_object (0000)
000C58D0 0110:
	_hud_get_nav_point_render_type (0000)
000C59E0 0660:
	_custom_render_nav_point (0000)
000C6040 0190:
	_hud_render_nav_points (0000)
000C61D0 0230:
	_code_000c61d0 (0000)
000C6400 0030:
	_hud_update_nav_points (0000)
00270134 0019:
	??_C@_0BJ@MJDIOPPI@could?5not?5find?5nav?5point?$AA@ (0000)
00270150 000f:
	??_C@_0P@MOLLEBG@nav_point_data?$AA@ (0000)
00270160 004a:
	??_C@_0EK@HLGJFFJP@local_player_index?$DO?$DN0?$CG?$CGlocal_pla@ (0000)
002701AC 002a:
	??_C@_0CK@OOCBMBAL@c?3?2halo?2SOURCE?2interface?2hud_nav@ (0000)
002701D8 000f:
	??_C@_0P@FGJPNLAM@hud?5nav?5points?$AA@ (0000)
002701E8 0020:
	??_C@_0CA@LFILMGD@Could?5not?5add?5another?5nav?5point?$AA@ (0000)
00270208 0008:
	__real@4010000000000000 (0000)
00270210 0008:
	__real@4024000000000000 (0000)
00270218 0004:
	__real@3f28f5c3 (0000)
0027021C 0004:
	__real@3ea8f5c3 (0000)
00270220 0004:
	__real@4043126f (0000)
00270224 0014:
	??_C@_0BE@OJIBABCH@waypoint_type?$CB?$DNNONE?$AA@ (0000)
00270238 0008:
	__real@3fe6666666666666 (0000)
00453ABC 0004:
	_nav_point_data (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "game_state.h"
#include "game/players.h"
#include "networking/network_connection.h"
#include "memory/data.h"
#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "math/integer_math.h"
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
#include "interface/hud.h"
#include "interface/hud_definitions.h"
#include "objects/object_types.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"

/* ---------- constants */

enum
{
	MAXIMUM_NUMBER_OF_NAV_POINTS = 4
};

enum
{
	_hud_nav_point_type_flag,
	_hud_nav_point_type_object,
	_hud_nav_point_type_game_engine_flag
};

/* ---------- structures */

struct hud_absolute_placement_definition
{
	short corner;
	short pad;
	long unused[8];
};

struct hud_placement_definition
{
	point2d offset;
	real_vector2d scale;
	short multiplayer_scaling_flags;
	short pad;
	long unused0[5];
};

struct hud_color_definition
{
	unsigned long color;
	unsigned long flash_color;
	real flash_period;
	real flash_delay;
	short number_of_flashes;
	unsigned short flash_flags;
	real flash_length;
	unsigned long disabled_color;
	unsigned long custom;
};

struct hud_messaging_parameters_definition
{
	struct hud_absolute_placement_definition absolute_placement;
	struct hud_placement_definition placement;
	struct tag_reference single_player_font;
	struct tag_reference multi_player_font;
	real up_time;
	real fade_time;
	real_argb_color state_color;
	real_argb_color text_color;
	real spacing;
	struct tag_reference hud_item_messages;
	struct tag_reference messaging_icons;
	struct tag_reference alternate_icon_text;
	struct tag_block button_icons;
	struct hud_color_definition color;
	struct tag_reference hud_messages;
	struct hud_color_definition objective_color;
};

struct hud_waypoint_arrow
{
	char name[0x20];
	long unused0[2];
	unsigned long color;
	real opacity;
	real fade;
	short sequence_indices[3];
	short pad;
	long unused1[4];
	unsigned long flags;
	long unused2[6];
};

struct hud_waypoint_definition
{
	real top_offset;
	real bottom_offset;
	real left_offset;
	real right_offset;
	long unused0[8];
	struct tag_reference arrow_bitmap;
	struct tag_block arrows;
	long unused1[0x14];
};

struct hud_globals_definition
{
	struct hud_messaging_parameters_definition messaging;
	struct hud_waypoint_definition waypoint;
	byte unknown1BC[0x294];
};

struct hud_nav_point_datum
{
	short nav_index;
	short type : 4;
	short screen_type : 4;
	float z_offset;
	long reference_index;
};

struct hud_nav_point_player_datum
{
	struct hud_nav_point_datum nav_points[MAXIMUM_NUMBER_OF_NAV_POINTS];
};

struct scenario_cutscene_flag
{
	long runtime_unused;
	char name[TAG_STRING_LENGTH];
	real_point3d position;
	real_euler_angles2d facing;
	byte unused[0x24];
};

struct hud_nav_object_datum
{
	byte unused[0xB6];
	word damage_flags;
};

#define hud_nav_object_try_and_get(object_index) \
	((struct hud_nav_object_datum *)object_try_and_get_and_verify_type((object_index), _object_mask_all))

/* ---------- prototypes */

void hud_update_nav_point_local_player(
	short local_player_index);

void *object_try_and_get_and_verify_type(
	long object_index,
	unsigned long valid_type_flags);

void object_get_bounding_sphere(
	long object_index,
	real_point3d *center,
	real *radius);

real_point3d *game_engine_get_goal_position(
	real_point3d *position,
	short goal_index);

void custom_render_nav_point(
	short local_player_index,
	real_point3d const *position,
	short nav_index,
	short render_type);

void game_engine_render_nav_points(
	short local_player_index);

long get_return_eip(
	void);

void unit_get_head_position(
	long unit_index,
	real_point3d *head_position);

/* ---------- globals */

extern struct hud_globals_definition *hud_globals;

static struct hud_nav_point_player_datum *nav_point_data;

/* ---------- private code */

short find_nav_point(
	char const *name)
{
	short nav_index = NONE;

	if (hud_globals)
	{
		short index;

		for (index = 0; index<hud_globals->waypoint.arrows.count; index++)
		{
			struct hud_waypoint_arrow *arrow = TAG_BLOCK_GET_ELEMENT(
				&hud_globals->waypoint.arrows, index, struct hud_waypoint_arrow);

			if (!_stricmp(name, arrow->name))
			{
				nav_index = index;
				break;
			}
		}
	}

	if (nav_index==NONE)
	{
		error(2, "could not find nav point");
	}

	return nav_index;
}

struct hud_nav_point_player_datum *get_nav_point_datum(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\interface\\hud_nav_points.c", 95, local_player_index>=0&&local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
	match_assert("c:\\halo\\SOURCE\\interface\\hud_nav_points.c", 96, nav_point_data);

	return &nav_point_data[local_player_index];
}

static void hud_activate_nav_point(
	short nav_index,
	long player_index,
	short type,
	long reference_index,
	float vertical_offset)
{
	if (player_index!=NONE)
	{
		short local_player_index = player_get(player_index)->local_player_index;

		if (local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS &&
			reference_index!=NONE && nav_index!=NONE)
		{
			struct hud_nav_point_player_datum *datum = get_nav_point_datum(local_player_index);
			short empty_index = NONE;
			short index;

			for (index = 0; index<MAXIMUM_NUMBER_OF_NAV_POINTS; index++)
			{
				struct hud_nav_point_datum *nav_point = &datum->nav_points[index];

				if (nav_point->type==type && nav_point->reference_index==reference_index)
				{
					nav_point->nav_index = nav_index;
					nav_point->z_offset = vertical_offset;

					return;
				}

				if (nav_point->type==NONE)
					empty_index = index;
			}

			if (empty_index!=NONE)
			{
				struct hud_nav_point_datum *nav_point = &datum->nav_points[empty_index];

				nav_point->type = type;
				nav_point->reference_index = reference_index;
				nav_point->nav_index = nav_index;
				nav_point->z_offset = vertical_offset;
			}
			else
			{
				error(2, "Could not add another nav point");
			}
		}
	}

	return;
}

static void hud_activate_team_nav_point(
	short nav_index,
	short team_index,
	short type,
	long reference_index,
	float vertical_offset)
{
	struct data_iterator player_iterator;
	struct player_datum *player;

	data_iterator_new(&player_iterator, player_data);

	while ((player = data_iterator_next(&player_iterator))!=NULL)
	{
		if (player->local_player_index!=NONE && team_index==player->team_index)
		{
			hud_activate_nav_point(nav_index, player_iterator.datum_index, type,
				reference_index, vertical_offset);
		}
	}

	return;
}

static void hud_activate_global_nav_point(
	short nav_index,
	short type,
	short reference_index,
	float vertical_offset)
{
	struct data_iterator player_iterator;
	struct player_datum *player;

	data_iterator_new(&player_iterator, player_data);

	while ((player = data_iterator_next(&player_iterator))!=NULL)
	{
		if (player->local_player_index!=NONE)
		{
			hud_activate_nav_point(nav_index, player_iterator.datum_index, type,
				reference_index, vertical_offset);
		}
	}

	return;
}

static void hud_deactivate_nav_point(
	long player_index,
	short type,
	long reference_index)
{
	if (player_index!=NONE)
	{
		short local_player_index = player_get(player_index)->local_player_index;

		if (local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS &&
			reference_index!=NONE)
		{
			struct hud_nav_point_player_datum *datum = get_nav_point_datum(local_player_index);
			short index;

			for (index = 0; index<MAXIMUM_NUMBER_OF_NAV_POINTS; index++)
			{
				struct hud_nav_point_datum *nav_point = &datum->nav_points[index];

				if (nav_point->type==type && nav_point->reference_index==reference_index)
				{
					nav_point->type = NONE;
					nav_point->reference_index = NONE;
					nav_point->nav_index = NONE;

					break;
				}
			}
		}
	}

	return;
}

static void hud_deactivate_team_nav_point(
	short team_index,
	short type,
	long reference_index)
{
	struct data_iterator player_iterator;
	struct player_datum *player;

	data_iterator_new(&player_iterator, player_data);

	while ((player = data_iterator_next(&player_iterator))!=NULL)
	{
		if (player->local_player_index!=NONE && team_index==player->team_index)
		{
			hud_deactivate_nav_point(player_iterator.datum_index, type, reference_index);
		}
	}

	return;
}

/* ---------- public code */

void hud_initialize_nav_points(
	void)
{
	nav_point_data = game_state_malloc("hud nav points", NULL, 0xC0);
	match_assert("c:\\halo\\SOURCE\\interface\\hud_nav_points.c", 106, nav_point_data);

	return;
}

void hud_initialize_nav_points_for_new_map(
	void)
{
	csmemset(nav_point_data, NONE, 0xC0);

	return;
}

void hud_dispose_nav_points_from_old_map(
	void)
{
	return;
}

void hud_dispose_nav_points(
	void)
{
	return;
}

void hud_activate_nav_point_with_game_engine_flag(
	short nav_index,
	long player_index,
	short flag_index,
	float vertical_offset)
{
	hud_activate_nav_point(nav_index, player_index, _hud_nav_point_type_game_engine_flag, flag_index, vertical_offset);

	return;
}

void hud_activate_nav_point_with_flag(
	short nav_index,
	long player_index,
	short flag_index,
	float vertical_offset)
{
	hud_activate_nav_point(nav_index, player_index, _hud_nav_point_type_flag, flag_index, vertical_offset);

	return;
}

void hud_activate_nav_point_with_object(
	short nav_index,
	long player_index,
	long object_index,
	float vertical_offset)
{
	hud_activate_nav_point(nav_index, player_index, _hud_nav_point_type_object, object_index, vertical_offset);

	return;
}

void hud_activate_team_nav_point_with_game_engine_flag(
	short nav_index,
	short team_index,
	short flag_index,
	float vertical_offset)
{
	hud_activate_team_nav_point(nav_index, team_index, _hud_nav_point_type_game_engine_flag, flag_index, vertical_offset);

	return;
}

void hud_activate_team_nav_point_with_flag(
	short nav_index,
	short team_index,
	short flag_index,
	float vertical_offset)
{
	hud_activate_team_nav_point(nav_index, team_index, _hud_nav_point_type_flag, flag_index, vertical_offset);

	return;
}

void hud_activate_team_nav_point_with_object(
	short nav_index,
	short team_index,
	long object_index,
	float vertical_offset)
{
	hud_activate_team_nav_point(nav_index, team_index, _hud_nav_point_type_object, object_index, vertical_offset);

	return;
}

void hud_activate_global_nav_point_with_game_engine_flag(
	short nav_index,
	short flag_index,
	float vertical_offset)
{
	hud_activate_global_nav_point(nav_index, _hud_nav_point_type_game_engine_flag, flag_index, vertical_offset);

	return;
}

void hud_deactivate_nav_point_with_game_engine_flag(
	long player_unit_index,
	short flag_index)
{
	hud_deactivate_nav_point(player_unit_index, _hud_nav_point_type_game_engine_flag, flag_index);

	return;
}

void hud_deactivate_nav_point_with_flag(
	long player_unit_index,
	short flag_index)
{
	hud_deactivate_nav_point(player_unit_index, _hud_nav_point_type_flag, flag_index);

	return;
}

void hud_deactivate_nav_point_with_object(
	long player_unit_index,
	long object_index)
{
	hud_deactivate_nav_point(player_unit_index, _hud_nav_point_type_object, object_index);

	return;
}

void hud_deactivate_team_nav_point_with_flag(
	short team_index,
	short flag_index)
{
	hud_deactivate_team_nav_point(team_index, _hud_nav_point_type_flag, flag_index);

	return;
}

void hud_deactivate_team_nav_point_with_object(
	short team_index,
	long object_index)
{
	hud_deactivate_team_nav_point(team_index, _hud_nav_point_type_object, object_index);

	return;
}

void hud_unit_activate_nav_point_with_flag(
	short nav_index,
	long unit_index,
	short flag_index,
	float vertical_offset)
{
	long player_index = player_index_from_unit_index(unit_index);

	if (player_index!=NONE)
	{
		hud_activate_nav_point(nav_index, player_index, _hud_nav_point_type_flag, flag_index, vertical_offset);
	}

	return;
}

void hud_unit_activate_nav_point_with_object(
	short nav_index,
	long unit_index,
	long object_index,
	float vertical_offset)
{
	long player_index = player_index_from_unit_index(unit_index);

	if (player_index!=NONE)
	{
		hud_activate_nav_point(nav_index, player_index, _hud_nav_point_type_object, object_index, vertical_offset);
	}

	return;
}

void hud_unit_deactivate_nav_point_with_flag(
	long unit_index,
	short flag_index)
{
	long player_index = player_index_from_unit_index(unit_index);

	if (player_index!=NONE)
	{
		hud_deactivate_nav_point(player_index, _hud_nav_point_type_flag, flag_index);
	}

	return;
}

void hud_unit_deactivate_nav_point_with_object(
	long unit_index,
	long object_index)
{
	long player_index = player_index_from_unit_index(unit_index);

	if (player_index!=NONE)
	{
		hud_deactivate_nav_point(player_index, _hud_nav_point_type_object, object_index);
	}

	return;
}

short hud_get_nav_point_render_type(
	short local_player_index,
	real_point3d const *head,
	real_point3d const *position,
	long reference_object_index)
{
	struct collision_result result;
	real_vector3d vector;
	long player_unit_index;
	short render_type;

	match_assert("c:\\halo\\SOURCE\\interface\\hud_nav_points.c", 510, global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = 20;

	player_unit_index = local_player_get_player_index(local_player_index)==NONE ?
		NONE :
		player_get(local_player_get_player_index(local_player_index))->unit_index;

	vector.i = position->x-head->x;
	vector.j = position->y-head->y;
	vector.k = position->z-head->z;

	if (collision_test_vector(_collision_test_for_line_of_sight_flags, head, &vector,
			player_unit_index, &result) &&
		(result.type!=3 || result.object_index!=reference_object_index))
	{
		render_type = 2;
	}
	else
	{
		render_type = 0;
	}

	match_assert("c:\\halo\\SOURCE\\interface\\hud_nav_points.c", 528, global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return render_type;
}

void hud_render_nav_points(
	short local_player_index)
{
	if (local_player_index!=NONE &&
		local_player_get_player_index(local_player_index)!=NONE &&
		player_get(local_player_get_player_index(local_player_index))->unit_index!=NONE &&
		hud_globals->waypoint.arrow_bitmap.index!=NONE)
	{
		struct hud_nav_point_player_datum *datum = get_nav_point_datum(local_player_index);
		struct hud_nav_point_datum *nav_point = datum->nav_points;
		long nav_point_count = MAXIMUM_NUMBER_OF_NAV_POINTS;

		do
		{
			real_point3d position;

			if (nav_point->nav_index==NONE ||
				nav_point->reference_index==NONE ||
				nav_point->type==NONE)
			{
				nav_point->type = NONE;
				continue;
			}

			switch (nav_point->type)
			{
			case _hud_nav_point_type_flag:
				position = TAG_BLOCK_GET_ELEMENT(
					&global_scenario_get()->cutscene_flags,
					nav_point->reference_index,
					struct scenario_cutscene_flag)->position;
				break;

			case _hud_nav_point_type_object:
				if (!hud_nav_object_try_and_get(nav_point->reference_index))
					continue;

				{
					real radius;
					object_get_bounding_sphere(
						nav_point->reference_index,
						&position,
						&radius);
				}
				break;

			case _hud_nav_point_type_game_engine_flag:
				{
					real_point3d goal_position;
					position = *game_engine_get_goal_position(
						&goal_position,
						(short)nav_point->reference_index);
				}
				break;

			default:
				match_assert("c:\\halo\\SOURCE\\interface\\hud_nav_points.c", 725, !"unreachable");
				break;
			}

			position.z += nav_point->z_offset;
			custom_render_nav_point(
				local_player_index,
				&position,
				nav_point->nav_index,
				nav_point->screen_type);

		}
		while (nav_point++, --nav_point_count);
	}

	game_engine_render_nav_points(local_player_index);

	return;
}

void hud_update_nav_point_local_player(
	short local_player_index)
{
	long return_eip = get_return_eip();
	long stack_buffer[0x80];
	struct hud_nav_point_player_datum *datum;
	long unit_index;
	short nav_point_index;

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	datum = get_nav_point_datum(local_player_index);
	unit_index = local_player_get_player_index(local_player_index)==NONE ?
		NONE :
		player_get(local_player_get_player_index(local_player_index))->unit_index;

	for (nav_point_index = 0; nav_point_index<MAXIMUM_NUMBER_OF_NAV_POINTS; nav_point_index++)
	{
		struct hud_nav_point_datum *nav_point = &datum->nav_points[nav_point_index];

		if (nav_point->nav_index==NONE ||
			nav_point->reference_index==NONE ||
			nav_point->type==NONE)
		{
			nav_point->type = NONE;
			continue;
		}

		if (unit_index!=NONE)
		{
			long reference_object_index = NONE;
			real_point3d head_position;
			real_point3d position;

			unit_get_head_position(unit_index, &head_position);

			switch (nav_point->type)
			{
			case _hud_nav_point_type_flag:
				position = TAG_BLOCK_GET_ELEMENT(
					&global_scenario_get()->cutscene_flags,
					nav_point->reference_index,
					struct scenario_cutscene_flag)->position;
				break;

			case _hud_nav_point_type_object:
				{
					struct hud_nav_object_datum *object =
						hud_nav_object_try_and_get(nav_point->reference_index);

					reference_object_index = nav_point->reference_index;
					if (!object || TEST_FLAG(object->damage_flags, _object_dead_bit))
					{
						nav_point->type = NONE;
						nav_point->reference_index = NONE;
						nav_point->nav_index = NONE;
						continue;
					}

					{
						real radius;
						object_get_bounding_sphere(
							reference_object_index,
							&position,
							&radius);
					}
				}
				break;

			case _hud_nav_point_type_game_engine_flag:
				{
					real_point3d goal_position;
					position = *game_engine_get_goal_position(
						&goal_position,
						(short)nav_point->reference_index);
				}
				break;

			default:
				break;
			}

			position.z += nav_point->z_offset;
			nav_point->screen_type = hud_get_nav_point_render_type(
				local_player_index,
				&head_position,
				&position,
				reference_object_index);
		}
	}

	{
		short corrupt_index;
		short buffer_index;

		for (buffer_index = 0x7F; buffer_index>=0; buffer_index--)
		{
			if (stack_buffer[buffer_index]!=0x62626262)
			{
				goto corrupt_stack_found;
			}
		}

		corrupt_index = NONE;
		goto stack_buffer_checked;

corrupt_stack_found:
		corrupt_index = buffer_index;

stack_buffer_checked:

		match_vassert(
			"c:\\halo\\SOURCE\\interface\\hud_nav_points.c",
			496,
			return_eip==get_return_eip(),
			"corrupt return address!");
		match_vassert(
			"c:\\halo\\SOURCE\\interface\\hud_nav_points.c",
			496,
			corrupt_index==NONE,
			csprintf(temporary, "corrupt stack at %d!", corrupt_index));
	}

	return;
}

void hud_update_nav_points(
	void)
{
	short local_player_index = local_player_get_next(NONE);

	while (local_player_index!=NONE)
	{
		hud_update_nav_point_local_player(local_player_index);
		local_player_index = local_player_get_next(local_player_index);
	}

	return;
}
