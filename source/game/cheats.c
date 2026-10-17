/*
CHEATS.C

symbols in this file:
00094C50 0020:
	_cheats_initialize (0000)
00094C70 0010:
	_cheats_dispose (0000)
00094C80 0010:
	_cheats_dispose_from_old_map (0000)
00094C90 00b0:
	_cheats_update (0000)
00094D40 0090:
	_cheats_load (0000)
00094DD0 0060:
	_cheat_active_camouflage_local_player (0000)
00094E30 0060:
	_cheat_player_index (0000)
00094E90 0010:
	_cheats_initialize_for_new_map (0000)
00094EA0 00b0:
	_cheat_teleport_to_camera (0000)
00094F50 0050:
	_cheat_active_camouflage (0000)
00094FA0 0150:
	_cheat_objects (0000)
000950F0 00d0:
	_cheat_all_weapons (0000)
000951C0 0050:
	_cheat_all_powerups (0000)
00095210 0050:
	_cheat_all_vehicles (0000)
0025AC64 003b:
	??_C@_0DL@LCJIJOPI@Cannot?5execute?5cheats?5attached?5t@ (0000)
0025ACA0 0005:
	??_C@_04JNJDPIIA@?$AN?6?7?$DL?$AA@ (0000)
0025ACA8 000e:
	??_C@_0O@KAFNGCBO@d?3?2cheats?4txt?$AA@ (0000)
0025ACB8 003a:
	??_C@_0DK@OACLODJP@Camera?5is?5outside?5BSP?4?4?4?5cannot?5@ (0000)
0025ACF4 001d:
	??_C@_0BN@DPPGFJCA@c?3?2halo?2SOURCE?2game?2cheats?4c?$AA@ (0000)
0025AD14 0004:
	__real@3ec90fdb (0000)
0043D808 0c81:
	_cheat_strings (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "cheats.h"
#include "cache/cache_files.h"
#include "camera/director.h"
#include "camera/observer.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "hs/hs.h"
#include "input/input.h"
#include "interface/terminal.h"
#include "items/weapon_definitions.h"
#include "main/console.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "objects/object_types.h"
#include "physics/bsp3d.h"
#include "physics/collisions.h"
#include "rasterizer/rasterizer_console_vars.h"
#include "scenario/scenario.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	MAXIMUM_CHEATS = NUMBER_OF_GAMEPAD_BUTTONS,
	MAXIMUM_CHEAT_LENGTH = 200
};

#define CHEATS_FILENAME "d:\\cheats.txt"

/* ---------- macros */

#define TAG_BLOCK_TRY_AND_GET_ELEMENT(block_address, index, type) \
	((block_address)->count ? TAG_BLOCK_GET_ELEMENT((block_address), (index), type) : NULL)

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

static char cheat_strings[MAXIMUM_CHEATS][MAXIMUM_CHEAT_LENGTH] = {0};
/* January emits this otherwise unreferenced byte after cheat_strings.
 * Its original name and purpose are unknown; the name is descriptive only. */
static boolean cheats_unused_flag = FALSE;

/* ---------- code */

void cheats_initialize(
	void)
{
	csmemset(cheat_strings, 0, sizeof(cheat_strings));

	return;
}

void cheats_dispose(
	void)
{
	return;
}

void cheats_dispose_from_old_map(
	void)
{
	return;
}

/* network_game_globals.c's */
boolean network_game_distributed_client(void);
/* bipeds.c's, player_control.c's */
extern boolean rider_ejection;
extern boolean player_autoaim_flag;
extern boolean player_magnetism_flag;
/* game_time.c's */
boolean game_time_reset_speed(void);

/* port: what the machine draws of the world put back as it draws it for
everyone: nothing seen through walls (wireframe, a drawing mode, the
environment or its parts left out), past fog, or under grass and water;
whether any was other */
static boolean cheats_network_client_rasterizer_enforce(
	void)
{
	boolean changed = FALSE;
	boolean *environment_part;

	if (rasterizer_debug_options.wireframe_enabled || rasterizer_debug_options.drawing_mode != 0 ||
		!rasterizer_debug_options.draw_water || !rasterizer_debug_options.draw_detail_objects ||
		!rasterizer_debug_options.draw_lens_flares || !rasterizer_debug_options.fog_atmospheric_enabled ||
		!rasterizer_debug_options.fog_planar_enabled)
	{
		changed = TRUE;
	}
	rasterizer_debug_options.wireframe_enabled = FALSE;
	rasterizer_debug_options.drawing_mode = 0;
	rasterizer_debug_options.draw_water = TRUE;
	rasterizer_debug_options.draw_detail_objects = TRUE;
	rasterizer_debug_options.draw_lens_flares = TRUE;
	rasterizer_debug_options.fog_atmospheric_enabled = TRUE;
	rasterizer_debug_options.fog_planar_enabled = TRUE;
	/* (the environment and its parts, lightmaps to screen fog, all drawn,
	as rasterizer_frame_begin leaves them from its switch: 2, given) */
	for (environment_part = &rasterizer_debug_options.draw_environment_lightmaps;
		environment_part <= &rasterizer_debug_options.draw_environment_fog_screen;
		environment_part++)
	{
		if (!*environment_part)
			changed = TRUE;
		*environment_part = TRUE;
	}
	if (rasterizer_debug_options.draw_environment != 2)
		changed = TRUE;
	rasterizer_debug_options.draw_environment = 2;

	return changed;
}

/* port: a client in another's game plays by the host's rules: none of its
own cheats, its own game speed or its own changes to how players play, nor
what its drawing shows it that others' does not, set before it joined too
(after, hs_compile_and_evaluate refuses them); each frame and each tick */
void cheats_network_client_enforce(
	void)
{
	static struct cheat_globals const none = { 0 };
	boolean changed;

	if (!network_game_distributed_client())
		return;
	changed = csmemcmp(&cheat, &none, sizeof(cheat)) != 0 || !rider_ejection || !player_autoaim_flag ||
		!player_magnetism_flag;
	csmemset(&cheat, 0, sizeof(cheat));
	rider_ejection = TRUE;
	player_autoaim_flag = TRUE;
	player_magnetism_flag = TRUE;
	changed |= game_time_reset_speed();
	changed |= cheats_network_client_rasterizer_enforce();
	if (changed)
	{
		console_warning("playing in another's game: its host's rules (cheats, game speed and drawing put back)");
		error(_error_log, "playing in another's game: cheats, game speed and drawing put back to the host's");
	}

	return;
}

void cheats_update(
	void)
{
	cheats_network_client_enforce();
	if (cheat.controller_enabled)
	{
		short local_player_index;

		for (local_player_index = local_player_get_next(NONE);
			local_player_index!=NONE;
			local_player_index = local_player_get_next(local_player_index))
		{
			struct gamepad_state const *gamepad = input_get_gamepad_state(local_player_index);

			if (gamepad && gamepad->buttons[_gamepad_binary_button_back]>0)
			{
				short button_index;

				for (button_index = 0; button_index<MAXIMUM_CHEATS; button_index++)
				{
					if (cheat_strings[button_index][0] && gamepad->buttons[button_index]>0)
					{
						director_inhibit_input(local_player_index);

						if (gamepad->buttons[button_index]==1)
						{
							console_printf(FALSE, cheat_strings[button_index]);
							if (!hs_compile_and_evaluate(cheat_strings[button_index]))
								cheat_strings[button_index][0] = 0;
						}
					}
				}
			}
		}
	}

	return;
}

void cheats_load(
	void)
{
	FILE *file = fopen(CHEATS_FILENAME, "r");

	if (file)
	{
		short cheat_index;

		for (cheat_index = 0; cheat_index<MAXIMUM_CHEATS; cheat_index++)
		{
			if (!fgets(cheat_strings[cheat_index], MAXIMUM_CHEAT_LENGTH-1, file))
				break;

			csstrtok(cheat_strings[cheat_index], "\r\n\t;");

			if ((cheat_index==_gamepad_binary_button_back || cheat_index==_gamepad_binary_button_start)
				&& cheat_strings[cheat_index][0])
			{
				cheat_strings[cheat_index][0] = 0;
				error(2, "Cannot execute cheats attached to the back or start button");
			}
		}

		fclose(file);
	}

	return;
}

void cheat_active_camouflage_local_player(
	short local_player_index)
{
	if (local_player_index>=0 && local_player_index<MAXIMUM_LOCAL_PLAYERS)
	{
		long player_index = local_player_get_player_index(local_player_index);

		if (player_index!=NONE)
		{
			struct player_datum *player = player_get(player_index);
			struct unit_datum *unit = unit_get(player->unit_index);

			unit->unit.active_camouflage = 1.0f;
			if (TEST_FLAG(unit->unit.flags, _unit_active_camouflaged_bit))
				unit->unit.flags |= FLAG(_unit_super_camouflaged_bit);
			unit->unit.flags |= FLAG(_unit_active_camouflaged_bit);
		}
	}

	return;
}

static long cheat_player_index(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	long player_index = NONE;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator))!=NULL)
	{
		if (player->unit_index!=NONE)
		{
			player_index = iterator.datum_index;
			break;
		}
	}

	return player_index;
}

void cheats_initialize_for_new_map(
	void)
{
	cheats_load();

	return;
}

void cheat_teleport_to_camera(
	void)
{
	long player_index = cheat_player_index();

	if (player_index!=NONE)
	{
		struct player_datum *player = player_get(player_index);

		if (player->local_player_index!=NONE)
		{
			struct observer_result const *result = observer_get_camera(player->local_player_index);

			match_assert("c:\\halo\\SOURCE\\game\\cheats.c", 256, result);

			if (result->location.cluster_index!=NONE)
			{
				long object_index = unit_get(player->unit_index)->object.parent_object_index;

				if (object_index==NONE)
					object_index = player->unit_index;

				object_set_position(object_index, &result->position, NULL, NULL);
			}
			else
			{
				terminal_printf(global_real_argb_orange, "Camera is outside BSP... cannot initiate teleportation...");
			}
		}
	}

	return;
}

void cheat_active_camouflage(
	void)
{
	long player_index = cheat_player_index();

	if (player_index!=NONE)
	{
		struct player_datum *player = player_get(player_index);
		struct unit_datum *unit = unit_get(player->unit_index);

		unit->unit.active_camouflage = 1.0f;
		if (TEST_FLAG(unit->unit.flags, _unit_active_camouflaged_bit))
			unit->unit.flags |= FLAG(_unit_super_camouflaged_bit);
		unit->unit.flags |= FLAG(_unit_active_camouflaged_bit);
	}

	return;
}

static void cheat_objects(
	struct tag_reference *references,
	short reference_count)
{
	long player_index = cheat_player_index();

	if (player_index!=NONE)
	{
		struct player_datum *player = player_get(player_index);
		struct unit_datum *unit = unit_get(player->unit_index);
		real_point3d origin;
		real_vector3d forward, up;
		short reference_index;

		object_get_origin(player->unit_index, &origin);
		object_get_orientation(player->unit_index, &forward, &up);

		for (reference_index = 0; reference_index<reference_count; reference_index++)
		{
			if (references[reference_index].index!=NONE)
			{
				struct object_placement_data data;
				real angle_separation = CEILING(2*_pi/reference_count, _pi/8);
				real angle = arctangent(forward.i, forward.j)+
					(reference_index-reference_count/2)*angle_separation;

				object_placement_data_new(&data, references[reference_index].index, NONE);

				data.position.x = cosine(angle)*1.5f+origin.x;
				data.position.y = sine(angle)*1.5f+origin.y;
				data.position.z = origin.z+0.8f;
				data.forward = forward;
				data.up = up;

				object_new(&data);
			}
		}
	}

	return;
}

/* port: a block's references (the map's count and address): none from a
block with no count or no address, and no more than a short counts (the
count was cast to one, and wrapped) */
static void cheat_objects_from_block(
	struct tag_block const *block)
{
	if (block->count > 0 && block->address)
	{
		cheat_objects(
			(struct tag_reference *)block->address,
			(short)MIN(block->count, SHORT_MAX));
	}

	return;
}

void cheat_all_weapons(
	void)
{
	struct game_globals *globals = scenario_get_game_globals();

	if (TAG_BLOCK_TRY_AND_GET_ELEMENT(&globals->weapon_list, 0, struct tag_reference))
	{
		cheat_objects_from_block(&globals->weapon_list);
	}
	else
	{
		struct tag_reference references[MAXIMUM_CHEATS];
		struct tag_iterator iterator;
		long tag_index;
		short reference_count = 0;

		tag_iterator_new(&iterator, WEAPON_DEFINITION_TAG);
		for (tag_index = tag_iterator_next(&iterator);
			tag_index!=NONE && reference_count<NUMBEROF(references);
			tag_index = tag_iterator_next(&iterator))
		{
			references[reference_count].index = tag_index;
			reference_count++;
		}

		cheat_objects(references, reference_count);
	}

	return;
}

void cheat_all_powerups(
	void)
{
	struct game_globals *globals = scenario_get_game_globals();

	cheat_objects_from_block(&globals->cheat_powerups);

	return;
}

void cheat_all_vehicles(
	void)
{
	struct game_globals *globals = scenario_get_game_globals();

	/* port: a block with an address, too */
	if (globals->multiplayer_information.count > 0 && globals->multiplayer_information.address)
	{
		cheat_objects_from_block(
			&TAG_BLOCK_GET_ELEMENT(&globals->multiplayer_information, 0,
				struct game_globals_multiplayer_information)->vehicles);
	}

	return;
}

/* port: finds an object definition tag matching the given tag identifier (name or index) */
static long cheat_find_object_tag(
	char const *tag_identifier)
{
	struct tag_iterator iterator;
	long tag_index;
	long exact_match_tag = NONE;
	long stripped_match_tag = NONE;
	long full_substring_match_tag = NONE;
	char search_lower[128];
	char *endptr;
	unsigned long parsed_id;

	if (!tag_identifier || !tag_identifier[0])
		return NONE;

	/* 1. Check if the identifier is a numerical or hexadecimal tag ID */
	parsed_id = strtoul(tag_identifier, &endptr, 0);
	if (endptr != tag_identifier && *endptr == 0)
	{
		if (tag_index_is_group(parsed_id, OBJECT_DEFINITION_TAG) ||
			object_definition_index_to_object_type(parsed_id) != NONE)
		{
			return (long)parsed_id;
		}
	}

	/* Prepare lowercase search string for case-insensitive matching */
	csstrncpy(search_lower, tag_identifier, sizeof(search_lower) - 1);
	search_lower[sizeof(search_lower) - 1] = 0;
	strlwr(search_lower);

	/* Iterate all tags in the loaded cache */
	tag_iterator_new(&iterator, NONE);
	for (tag_index = tag_iterator_next(&iterator);
		tag_index != NONE;
		tag_index = tag_iterator_next(&iterator))
	{
		short obj_type = object_definition_index_to_object_type(tag_index);
		char const *full_name;
		char const *stripped_name;
		char full_lower[256];
		char stripped_lower[128];

		if (obj_type == NONE)
			continue;

		full_name = tag_get_name(tag_index);
		if (!full_name || !full_name[0])
			continue;

		stripped_name = tag_name_strip_path(full_name);

		csstrncpy(full_lower, full_name, sizeof(full_lower) - 1);
		full_lower[sizeof(full_lower) - 1] = 0;
		strlwr(full_lower);

		csstrncpy(stripped_lower, stripped_name, sizeof(stripped_lower) - 1);
		stripped_lower[sizeof(stripped_lower) - 1] = 0;
		strlwr(stripped_lower);

		/* Exact match on stripped name (e.g. "warthog" or "mp_warthog") */
		if (!csstrcmp(stripped_lower, search_lower))
		{
			exact_match_tag = tag_index;
			break;
		}

		/* Exact match on full tag path */
		if (!csstrcmp(full_lower, search_lower))
		{
			exact_match_tag = tag_index;
			break;
		}

		/* Substring match on stripped name */
		if (strstr(stripped_lower, search_lower))
		{
			if (stripped_match_tag == NONE || obj_type == _object_type_vehicle)
				stripped_match_tag = tag_index;
		}
		/* Substring match on full name */
		else if (strstr(full_lower, search_lower))
		{
			if (full_substring_match_tag == NONE || obj_type == _object_type_vehicle)
				full_substring_match_tag = tag_index;
		}
	}

	if (exact_match_tag != NONE)
		return exact_match_tag;
	if (stripped_match_tag != NONE)
		return stripped_match_tag;
	return full_substring_match_tag;
}

/* port: spawns an object near the host if in a valid position or reports FAIL if no space */
boolean cheat_spawn_object(
	char const *tag_identifier)
{
	long definition_index;
	short object_type;
	struct object_definition *object_definition;
	real bounding_radius;
	long player_index;
	struct player_datum *player;
	long host_unit_index;
	real_point3d host_origin;
	real_vector3d host_forward, host_up;
	real_vector3d horizontal_forward;
	real spawn_distance;
	real_point3d spawn_pos;
	struct collision_result ground_collision;
	struct collision_result line_collision;
	struct location spawn_loc;
	real_vector3d down_vector;
	real_vector3d host_to_spawn;
	long nearby_objects[32];
	short nearby_count;
	short i;
	struct object_placement_data placement_data;
	long new_object_index;

	if (!tag_identifier || !tag_identifier[0])
	{
		console_warning("spawn: give a tag identifier or name");
		return FALSE;
	}

	definition_index = cheat_find_object_tag(tag_identifier);
	if (definition_index == NONE)
	{
		console_warning("spawn: FAIL - object tag \"%s\" not found", tag_identifier);
		return FALSE;
	}

	object_type = object_definition_index_to_object_type(definition_index);
	if (object_type == NONE)
	{
		console_warning("spawn: FAIL - tag \"%s\" is not a valid object", tag_identifier);
		return FALSE;
	}

	object_definition = object_definition_get(definition_index);
	bounding_radius = object_definition->object.bounding_radius;
	if (bounding_radius <= 0.1f)
		bounding_radius = 1.0f;

	player_index = cheat_player_index();
	if (player_index == NONE)
	{
		console_warning("spawn: FAIL - no player in game");
		return FALSE;
	}

	player = player_get(player_index);
	if (player->unit_index == NONE)
	{
		console_warning("spawn: FAIL - host player is not alive");
		return FALSE;
	}

	host_unit_index = player->unit_index;
	object_get_origin(host_unit_index, &host_origin);
	object_get_orientation(host_unit_index, &host_forward, &host_up);

	horizontal_forward = host_forward;
	horizontal_forward.k = 0.0f;
	if (magnitude_squared3d(&horizontal_forward) > 0.0001f)
		normalize3d(&horizontal_forward);
	else
		horizontal_forward = host_forward;

	spawn_distance = bounding_radius + 2.0f;
	if (spawn_distance < 2.5f)
		spawn_distance = 2.5f;

	spawn_pos.x = host_origin.x + horizontal_forward.i * spawn_distance;
	spawn_pos.y = host_origin.y + horizontal_forward.j * spawn_distance;
	spawn_pos.z = host_origin.z + 0.5f;

	/* Ground detection via downward collision trace */
	down_vector.i = 0.0f;
	down_vector.j = 0.0f;
	down_vector.k = -4.0f;
	if (collision_test_vector(
		FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_structure_bit),
		&spawn_pos,
		&down_vector,
		host_unit_index,
		&ground_collision))
	{
		// add bounding radius to solve, plus a padding for collision errors
		spawn_pos.z = ground_collision.point.z + bounding_radius + 0.1f;
	}

	// fix for crashes prob
	/* 1. Location and cluster validation */
	scenario_location_from_point(&spawn_loc, &spawn_pos);
	if (spawn_loc.cluster_index == NONE)
	{
		console_warning("spawn: FAIL - no space (position outside cluster/BSP)");
		return FALSE;
	}

	if (global_bsp3d_get() && bsp3d_test_point(global_bsp3d_get(), 0, &spawn_pos) == NONE)
	{
		console_warning("spawn: FAIL - no space (position outside BSP leaf)");
		return FALSE;
	}

	/* 2. Line of sight / obstacle check between host and spawn position */
	vector_from_points3d(&host_origin, &spawn_pos, &host_to_spawn);
	if (collision_test_vector(
		FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_structure_bit),
		&host_origin,
		&host_to_spawn,
		host_unit_index,
		&line_collision))
	{
		console_warning("spawn: FAIL - no space (line of sight blocked by structure)");
		return FALSE;
	}

	// MAIN CHECK
	/* 3. Collision sphere check against structure BSP */
	if (collision_test_sphere(&spawn_pos, bounding_radius * 0.75f, host_unit_index))
	{
		console_warning("spawn: FAIL - no space (intersects structure)");
		return FALSE;
	}

	/* 4. Overlap check with nearby objects */
	/*
	  TODO: fix nearby objects

	nearby_count = objects_in_sphere(
		0,
		_object_mask_vehicle | _object_mask_biped | _object_mask_scenery | _object_mask_machine,
		&spawn_loc,
		&spawn_pos,
		bounding_radius,
		nearby_objects,
		NUMBEROF(nearby_objects));


	for (i = 0; i < nearby_count; i++)
	{
		if (nearby_objects[i] != host_unit_index)
		{
			short nearby_type = object_get_type(nearby_objects[i]);
			console_warning("spawn: FAIL - no space (blocked by nearby %s)",
				object_type_get_name(nearby_type));
			return FALSE;
		}
	}
*/
	/* Instantiate object */
	object_placement_data_new(&placement_data, definition_index, NONE);
	placement_data.position = spawn_pos;
	placement_data.forward = horizontal_forward;
	placement_data.up = *global_up3d;

	new_object_index = object_new(&placement_data);
	if (new_object_index == NONE)
	{
		console_warning("spawn: FAIL - object creation failed");
		return FALSE;
	}

	console_printf("spawn: created %s", tag_get_name(definition_index));
	return TRUE;
}
