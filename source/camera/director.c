/*
DIRECTOR.C

symbols in this file:
000747D0 0040:
	_director_get (0000)
00074810 0020:
	_director_initialize (0000)
00074830 0010:
	_director_dispose (0000)
00074840 0050:
	_director_inhibit_facing (0000)
00074890 0050:
	_director_inhibit_input (0000)
000748E0 0050:
	_director_inhibited_facing (0000)
00074930 0050:
	_director_inhibited_input (0000)
00074980 0050:
	_director_set_mode (0000)
000749D0 00b0:
	_director_save_camera (0000)
00074A80 00a0:
	_director_get_perspective (0000)
00074B20 00f0:
	_director_desired_perspective (0000)
00074C10 0060:
	_director_set_camera (0000)
00074C70 0070:
	_director_initialize_variables (0000)
00074CE0 0220:
	_director_process_variables (0000)
00074F00 0070:
	_director_dispose_from_old_map (0000)
00074F70 0150:
	_director_load_camera (0000)
000750C0 0130:
	_director_rotate_cameras (0000)
000751F0 0060:
	_director_camera_deterministic (0000)
00075250 00d0:
	_director_choose_game_perspective (0000)
00075320 0130:
	_director_script_camera (0000)
00075450 01c0:
	_director_choose_camera_game (0000)
00075610 00b0:
	_director_choose_camera_editor (0000)
000756C0 00c0:
	_director_choose_camera_script_camera_record (0000)
00075780 03c0:
	_director_update_controls (0000)
00075B40 0050:
	_director_choose_camera (0000)
00075B90 00d0:
	_director_initialize_for_new_map (0000)
00075C60 01f0:
	_director_update (0000)
00075E50 0020:
	_director_initialize_for_saved_game (0000)
00256B18 0008:
	_director_game_camera_modes (0000)
00256B20 0008:
	_director_script_camera_record_camera_modes (0000)
00256B28 0004:
	__real@3cf5c28f (0000)
00256B2C 0004:
	_friction (0000)
00256B30 0004:
	__real@41c80000 (0000)
00256B34 0004:
	__real@3fa66666 (0000)
00256B38 000d:
	??_C@_0N@PCFDPHGK@first?5person?$AA@ (0000)
00256B48 0007:
	??_C@_06GIFLLBB@editor?$AA@ (0000)
00256B50 0007:
	??_C@_06CPNKLHAC@flying?$AA@ (0000)
00256B58 0009:
	??_C@_08GFACNFFL@orbiting?$AA@ (0000)
00256B64 000a:
	??_C@_09BEIPBMDN@following?$AA@ (0000)
00256B70 004c:
	??_C@_0EM@JCKEKCNI@local_player_index?$DO?$DN0?5?$CG?$CG?5local_p@ (0000)
00256BBC 0021:
	??_C@_0CB@MLDELAOE@c?3?2halo?2SOURCE?2camera?2director?4c@ (0000)
00256BE0 0013:
	??_C@_0BD@CNMFAHOJ@director?5scripting?$AA@ (0000)
00256BF4 002e:
	??_C@_0CO@JGFKNOPP@mode?$DO?$DN0?5?$CG?$CG?5mode?$DMNUMBER_OF_DIRECT@ (0000)
00256C24 0004:
	??_C@_03PPOCCAPH@?$CFf?6?$AA@ (0000)
00256C28 000a:
	??_C@_09MALOBAAJ@?$CFf?5?$CFf?5?$CFf?6?$AA@ (0000)
00256C34 000e:
	??_C@_0O@EOBEHDJM@d?3?2camera?4txt?$AA@ (0000)
00256C44 000a:
	??_C@_09LKGGBKMJ@?$CFs?5camera?$AA@ (0000)
00256C50 0004:
	__real@bb4de32e (0000)
00256C54 0004:
	__real@b851b717 (0000)
00256C58 0004:
	__real@3851b717 (0000)
00256C5C 0004:
	__real@37a4b5be (0000)
00256C60 0004:
	__real@b824b5be (0000)
002DCBA0 0018:
	_director_camera_mode_names (0000)
002DCBB8 0070:
	_variables (0000)
0031D048 03ea:
	_director_globals (0000)
	_director_camera_switch_fast (03e8)
*/

/* ---------- headers */

#include "director.h"

#include "first_person_camera.h"
#include "following_camera.h"
#include "observer.h"

#include "editor/editor_stubs.h"
#include "saved games/game_state.h"

#include "units/unit_definitions.h"
#include "units/units.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void following_camera_update(
	void *camera,
	void *command,
	void *result);
void scripted_camera_update(
	void *camera,
	void *command,
	void *result);
void director_initialize_variables(
	short local_player_index);
void director_choose_camera(
	short local_player_index,
	boolean initialize,
	boolean key);

/* ---------- globals */

short const director_game_camera_modes[3] =
{
	_camera_first_person,
	_camera_flying,
	_camera_following
};

short const director_script_camera_record_camera_modes[4] =
{
	_camera_first_person,
	_camera_flying,
	_camera_following,
	_camera_orbiting
};

real const friction = 5.f;

char const *director_camera_mode_names[NUMBER_OF_DIRECTOR_CAMERA_MODES] =
{
	"following",
	"orbiting",
	"flying",
	"editor",
	"first person"
};

struct director_variable_definition variables[NUMBER_OF_DIRECTOR_VARIABLES] =
{
	{ 5, 4, NONE, { 0, 0 }, 0.15f, 0.f, -REAL_MAX, REAL_MAX, TRUE, { 0, 0, 0 } },
	{ 6, 7, NONE, { 0, 0 }, 0.075f, 0.f, -REAL_MAX, REAL_MAX, FALSE, { 0, 0, 0 } },
	{ 1, 0, NONE, { 0, 0 }, 0.075f, 0.f, -REAL_MAX, REAL_MAX, TRUE, { 0, 0, 0 } },
	{ 3, 2, NONE, { 0, 0 }, 0.075f, 0.f, -REAL_MAX, REAL_MAX, TRUE, { 0, 0, 0 } }
};

struct director_globals director_globals;
boolean director_camera_switch_fast;

/* ---------- public code */

static struct director *director_get(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\camera\\director.c", 179, local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
	return &director_globals.local_players[local_player_index];
}

void director_initialize(
	void)
{
	director_camera_scripted = game_state_malloc("director scripting", NULL, sizeof(*director_camera_scripted));
	director_camera_scripted->camera_scripted = FALSE;
	return;
}

void director_dispose(
	void)
{
	return;
}

void director_dispose_from_old_map(
	void)
{
	short local_player_index;

	for (local_player_index = 0; local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; local_player_index++)
	{
		struct director *director = director_get(local_player_index);

		director->camera_proc = NULL;
		director->debug_input_scale = 1.f;
		director->debug_controls = FALSE;
	}

	director_camera_scripted->camera_scripted = FALSE;
	return;
}

void director_inhibit_facing(
	short local_player_index)
{
	director_get(local_player_index)->inhibited_facing = TRUE;
	return;
}

void director_inhibit_input(
	short local_player_index)
{
	director_get(local_player_index)->inhibited_input = TRUE;
	return;
}

boolean director_inhibited_facing(
	short local_player_index)
{
	return director_get(local_player_index)->inhibited_facing;
}

boolean director_inhibited_input(
	short local_player_index)
{
	return director_get(local_player_index)->inhibited_input;
}

director_perspective director_get_perspective(
	short local_player_index)
{
	struct director *camera;

	camera = director_get(local_player_index);

	if (camera->camera_proc == first_person_camera_update)
	{
		if (camera->camera_change_pause == 0.f)
			camera->perspective = 0;
	}
	else if (camera->camera_proc == following_camera_update)
	{
		camera->perspective = 1;
	}
	else if (camera->camera_proc == scripted_camera_update)
	{
		camera->perspective = 2;
	}
	else
	{
		camera->perspective = 3;
	}

	return camera->perspective;
}

short director_desired_perspective(
	long unit_index,
	director_perspective *perspective)
{
	short following = FALSE;

	*perspective = 0;
	if (unit_index != NONE)
	{
		struct unit_datum *unit = unit_get(unit_index);
		if (unit->object.parent_object_index != NONE)
		{
			struct object_datum *parent_object = object_get(unit->object.parent_object_index);
			if (TEST_FLAG(_object_mask_unit, parent_object->object.type))
			{
				boolean third_person_on_enter;
				struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(
					&unit_definition_get(parent_object->definition_index)->unit.seats,
					unit->unit.parent_seat_index,
					struct unit_seat);

				third_person_on_enter = TEST_FLAG(seat->flags, _unit_seat_third_person_on_enter_bit);
				if (TEST_FLAG(seat->flags, _unit_seat_third_person_camera_bit))
					following = TRUE;
				if (third_person_on_enter)
				{
					if (unit->unit.animation.state == _unit_state_entering_seat)
						*perspective = 1;
					else if (unit->unit.animation.state == _unit_state_exiting_seat)
						*perspective = 3;
					else
						*perspective = 2;
				}
				else
				{
					*perspective = 2;
				}
			}
			else
			{
				*perspective = 2;
			}
		}

		if (*perspective == 1 || *perspective == 3)
			following = TRUE;
	}

	return following;
}

void director_set_mode(
	short mode)
{
	match_assert("c:\\halo\\SOURCE\\camera\\director.c", 384, mode>=0 && mode<NUMBER_OF_DIRECTOR_GAME_MODES);
	if (director_globals.game_mode != mode)
	{
		director_globals.game_mode = mode;
		director_globals.initialize_camera = TRUE;
	}

	return;
}

void director_save_camera(
	void)
{
	FILE *file;

	file = fopen("d:\\camera.txt", "w");
	if (file)
	{
		struct observer_result const *camera;

		camera = observer_get_camera(0);
		fprintf(file, "%f %f %f\n", camera->position.x, camera->position.y, camera->position.z);
		fprintf(file, "%f %f %f\n", camera->forward.i, camera->forward.j, camera->forward.k);
		fprintf(file, "%f %f %f\n", camera->up.i, camera->up.j, camera->up.k);
		fprintf(file, "%f\n", camera->field_of_view);
		fclose(file);
	}

	return;
}

void director_initialize_for_new_map(
	void)
{
	short local_player_index;

	director_globals.game_mode = game_in_editor() ? _director_mode_editor : _director_mode_game;
	director_globals.initialize_camera = FALSE;
	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		struct director *camera = director_get(local_player_index);

		camera->camera_change_pause = 0.f;
		camera->bored_time = 0;
		camera->bored = FALSE;
		director_choose_camera(local_player_index, TRUE, FALSE);
		director_initialize_variables(local_player_index);
	}

	return;
}

void director_initialize_for_saved_game(
	void)
{
	director_initialize_for_new_map();
	director_script_camera(director_camera_scripted->camera_scripted);

	return;
}

short director_camera_deterministic(
	long unit_index,
	real_point3d *position,
	real_vector3d *forward)
{
	director_perspective perspective;
	short following;

	following = director_desired_perspective(unit_index, &perspective);
	if (!following)
		first_person_camera_deterministic(unit_index, position, forward);
	else
		following_camera_deterministic(unit_index, position, forward);

	return following;
}

/* ---------- private code */
