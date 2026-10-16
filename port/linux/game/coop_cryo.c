/*
COOP_CRYO.C

Network co-op on The Pillar of Autumn with a cryotube for every player
(network.coop_cryo_bay). The scripts play their co-op intro, sixteen more
tubes stand in the cryo bay, and the host seats the waiting players in them
and films the rows opening before Bungie's shot.
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "camera/camera_scripting.h"
#include "cutscene/cinematics.h"
#include "effects/player_effects.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "hs/hs.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "objects/objects.h"
#include "objects/object_types.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"
#include "units/biped_definitions.h"
#include "units/unit_definitions.h"
#include "units/units.h"
#include "units/vehicle_definitions.h"

#include "coop_cryo.h"
#include "network_coop.h"

/* camera_scripting.c's */
void scripted_camera_set_camera_point_absolute(real_point3d const *position, real_vector3d const *forward,
	real_vector3d const *up, real field_of_view, word transition_time);
short scripted_camera_next_camera_point(void);

/* ---------- constants */

#define CRYO_LEVEL "levels\\a10\\a10"
#define CRYO_TUBE "levels\\a10\\devices\\cryotube\\cryotube"
#define CRYO_SEAT "CT-driver"
#define CRYO_STEAM_JET "scenery\\emitters\\steam jets\\steam jets"

enum
{
	CRYO_ROWS = 2,
	CRYO_ROW_TUBES = 8,
	CRYO_TUBES = CRYO_ROWS * CRYO_ROW_TUBES,
	/* cryotube_1, cryotube_2, then the rows */
	CRYO_BUNGIE_SEATS = 2,
	CRYO_SEATS = CRYO_BUNGIE_SEATS + CRYO_TUBES,
	CRYO_JETS = 2,

	CRYO_WHITE_IN_TICKS = 15,
	CRYO_WHITE_OUT_TICKS = 8,
	CRYO_RIPPLE_TICKS = 3,
	/* hoods lift 30 to 120 ticks after unit_open; a pan starts at 80% */
	CRYO_HOOD_LIFT_TICKS = 30,
	CRYO_PAN_WAIT_TICKS = CRYO_HOOD_LIFT_TICKS + 72,
	CRYO_PAN_TICKS = 6 * TICKS_PER_SECOND,
	CRYO_CUT_TO_OPEN_TICKS = 6,
	CRYO_CRANE_TICKS = 7 * TICKS_PER_SECOND,
	CRYO_NORTH_OPEN = CRYO_WHITE_IN_TICKS,
	/* for a client that got the seats before the stand-ins */
	CRYO_SEATS_AGAIN = CRYO_NORTH_OPEN - 3,
	CRYO_NORTH_PAN = CRYO_NORTH_OPEN + CRYO_RIPPLE_TICKS * (CRYO_ROW_TUBES - 1) + CRYO_PAN_WAIT_TICKS,
	CRYO_SOUTH_CUT = CRYO_NORTH_PAN + CRYO_PAN_TICKS,
	CRYO_SOUTH_OPEN = CRYO_SOUTH_CUT + CRYO_CUT_TO_OPEN_TICKS,
	CRYO_SOUTH_PAN = CRYO_SOUTH_OPEN + CRYO_RIPPLE_TICKS * (CRYO_ROW_TUBES - 1) + CRYO_PAN_WAIT_TICKS,
	CRYO_CRANE = CRYO_SOUTH_PAN + CRYO_PAN_TICKS,
	CRYO_WHITE_OUT = CRYO_CRANE + CRYO_CRANE_TICKS,
	CRYO_HERO = CRYO_WHITE_OUT + CRYO_WHITE_OUT_TICKS,
	CRYO_HERO_TICKS = 250,

	CRYO_STEAM_TICKS = 45,
	CRYO_EXIT_JITTER_TICKS = 24,
	CRYO_CLOSE_DELAY_TICKS = 20,
	CRYO_CLOSE_TICKS = 4,
};

#define CRYO_ROW_Y 2.65f
#define CRYO_TUBE_Z -0.035f
/* where "CT-driver exit" leaves the Chief */
#define CRYO_EXIT_DISTANCE 0.849f
#define CRYO_FIELD_OF_VIEW (70.0f * _pi / 180.0f)
#define CRYO_BAY_X0 -62.4f
#define CRYO_BAY_X1 -51.5f
#define CRYO_BAY_Y 5.0f

/* doors to oxygen tanks, two to a cell */
static real const cryo_tube_x[CRYO_ROW_TUBES] =
{
	-53.698f, -54.502f, -55.548f, -56.352f, -58.218f, -59.022f, -60.068f, -60.872f
};

/* Bungie's jets on cryotube_1, in the tube's frame */
static struct cryo_jet
{
	real_point3d offset;
	real_euler_angles3d angles;
} const cryo_jets[CRYO_JETS] =
{
	{ { -0.746f, 0.250f, 0.820f }, { -5.358971f, -7.077125f, -2.056297f } },
	{ { -0.744f, -0.263f, 0.816f }, { 4.906380f, -6.033730f, 2.908450f } },
};

enum
{
	_cryo_camera_north_start,
	_cryo_camera_north_end,
	_cryo_camera_south_start,
	_cryo_camera_south_end,
	_cryo_camera_crane_low,
	_cryo_camera_crane_high,
	NUMBER_OF_CRYO_CAMERAS
};

static struct cryo_camera
{
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
} const cryo_cameras[NUMBER_OF_CRYO_CAMERAS] =
{
	{ { -52.198f, 1.3f, 1.0f }, { -0.561953f, 0.776031f, -0.286329f }, { -0.167935f, 0.23191f, 0.958131f } },
	{ { -59.822f, 1.3f, 1.0f }, { -0.561953f, 0.776031f, -0.286329f }, { -0.167935f, 0.23191f, 0.958131f } },
	{ { -52.198f, -1.3f, 1.0f }, { -0.561953f, -0.776031f, -0.286329f }, { -0.167935f, -0.23191f, 0.958131f } },
	{ { -59.822f, -1.3f, 1.0f }, { -0.561953f, -0.776031f, -0.286329f }, { -0.167935f, -0.23191f, 0.958131f } },
	{ { -52.45f, 0.0f, 0.45f }, { -0.998927f, 0.0f, 0.046308f }, { 0.046308f, 0.0f, 0.998927f } },
	{ { -52.35f, 0.0f, 2.45f }, { -0.929533f, 0.0f, -0.36874f }, { -0.36874f, 0.0f, 0.929533f } },
};

static char const *const bungie_jet_names[] =
{
	"cryotube_1_steam_1", "cryotube_1_steam_2", "cryotube_2_steam_1", "cryotube_2_steam_2"
};

/* ---------- definitions */

struct cryo_seat
{
	long vehicle_index;
	long unit_index;
	long player_index;
	boolean stand_in;
	long exit_time;
};

/* ---------- globals */

static struct
{
	/* host */
	boolean armed;
	long intro_time;
	long intro_tick;
	boolean released;
	long first_exit_time;
	long close_time;
	boolean stand_ins_gone;
	struct cryo_seat seats[CRYO_SEATS];
	word bungie_jets_held;

	/* every machine */
	long tube_indices[CRYO_TUBES];
	long open_times[CRYO_TUBES];
	boolean seen_shut[CRYO_TUBES];
	long jet_indices[CRYO_TUBES][CRYO_JETS];
} coop_cryo;

/* ---------- private code */

static boolean coop_cryo_active(
	void)
{
	struct network_game *game;

	return network_coop_active() && (game = network_game_get_game()) != NULL &&
		TEST_FLAG(game->cooperative_flags, _network_game_cooperative_cryo_bay_bit) &&
		global_scenario_index != NONE && !csstrcasecmp(tag_get_name(global_scenario_index), CRYO_LEVEL);
}

static boolean coop_cryo_host(
	void)
{
	return game_connection() == _game_connection_network_server;
}

static void cryo_tube_frame(
	short tube,
	real_point3d *position,
	real_vector3d *forward)
{
	boolean north = tube < CRYO_ROW_TUBES;

	position->x = cryo_tube_x[tube % CRYO_ROW_TUBES];
	position->y = north ? CRYO_ROW_Y : -CRYO_ROW_Y;
	position->z = CRYO_TUBE_Z;
	forward->i = 0.0f;
	forward->j = north ? -1.0f : 1.0f;
	forward->k = 0.0f;
}

static short cryo_tube_of(
	long object_index)
{
	short tube;

	for (tube = 0; object_index != NONE && tube < CRYO_TUBES; tube++)
	{
		if (coop_cryo.tube_indices[tube] == object_index)
			return tube;
	}
	return NONE;
}

static short cryo_object_name_index(
	char const *name)
{
	struct scenario *scenario = global_scenario_get();
	short index;

	for (index = 0; index < scenario->object_names.count; index++)
	{
		if (!csstrcasecmp(TAG_BLOCK_GET_ELEMENT(&scenario->object_names, index, struct scenario_object_name)->name,
			name))
		{
			return index;
		}
	}
	return NONE;
}

static long cryo_named_object(
	char const *name)
{
	short name_index = cryo_object_name_index(name);

	return name_index != NONE ? object_index_from_name_index(name_index) : NONE;
}

static short cryo_camera_point_index(
	char const *name)
{
	struct scenario *scenario = global_scenario_get();
	short index;

	for (index = 0; index < scenario->cutscene_camera_points.count; index++)
	{
		if (!csstrcasecmp(TAG_BLOCK_GET_ELEMENT(&scenario->cutscene_camera_points, index,
			struct scenario_cutscene_camera_point)->name, name))
		{
			return index;
		}
	}
	return NONE;
}

static long cryo_occupant(
	long vehicle_index)
{
	struct object_iterator iterator;
	struct object_datum *object;

	if (vehicle_index == NONE)
		return NONE;
	object_iterator_new(&iterator, _object_mask_unit, 0);
	while ((object = object_iterator_next(&iterator)) != NULL)
	{
		if (object->object.parent_object_index == vehicle_index)
			return iterator.index;
	}
	return NONE;
}

static void cryo_camera_set(
	short camera,
	word ticks)
{
	struct cryo_camera const *point = &cryo_cameras[camera];

	scripted_camera_set_camera_point_absolute(&point->position, &point->forward, &point->up, CRYO_FIELD_OF_VIEW,
		ticks);
}

static void cryo_bungie_jets(
	boolean create)
{
	short index;

	for (index = 0; index < NUMBEROF(bungie_jet_names); index++)
	{
		short name_index = cryo_object_name_index(bungie_jet_names[index]);
		long object_index = name_index != NONE ? object_index_from_name_index(name_index) : NONE;

		if (create && object_index == NONE && TEST_FLAG(coop_cryo.bungie_jets_held, index))
		{
			object_new_by_name(name_index);
		}
		else if (!create && object_index != NONE)
		{
			object_delete(object_index);
			SET_FLAG(coop_cryo.bungie_jets_held, index, TRUE);
		}
	}
}

static void cryo_unit_face(
	long unit_index,
	real_vector3d const *forward)
{
	struct unit_datum *unit = unit_get(unit_index);
	long player_index = player_index_from_unit_index(unit_index);

	unit->unit.desired_facing_vector = *forward;
	unit->unit.desired_aiming_vector = *forward;
	unit->unit.desired_looking_vector = *forward;
	if (player_index != NONE && player_get(player_index)->local_player_index != NONE)
		player_control_set_facing(player_get(player_index)->local_player_index, forward);
}

/* the netcode seats players' units itself */
static void cryo_seat_note(
	struct cryo_seat const *seat)
{
	struct unit_datum *unit = object_try_and_get_and_verify_type(seat->unit_index, _object_mask_unit);

	if (seat->stand_in && unit && unit->object.parent_object_index == seat->vehicle_index)
		network_coop_note_unit_seat(seat->unit_index, seat->vehicle_index, unit->unit.parent_seat_index);
}

static void cryo_seat_unit(
	struct cryo_seat *seat,
	long unit_index)
{
	short tube = cryo_tube_of(seat->vehicle_index);

	if (tube != NONE)
	{
		real_point3d position;
		real_vector3d forward;

		cryo_tube_frame(tube, &position, &forward);
		cryo_unit_face(unit_index, &forward);
	}
	unit_scripting_enter_vehicle(unit_index, seat->vehicle_index, CRYO_SEAT);
	seat->unit_index = unit_index;
	cryo_seat_note(seat);
}

static void cryo_stand_in(
	struct cryo_seat *seat)
{
	struct game_globals_player_information *player_information = TAG_BLOCK_GET_ELEMENT(
		&scenario_get_game_globals()->player_information, 0, struct game_globals_player_information);
	struct object_placement_data data;
	long unit_index;

	if (seat->vehicle_index == NONE || player_information->player_unit.index == NONE)
		return;
	object_placement_data_new(&data, player_information->player_unit.index, NONE);
	data.position = object_get(seat->vehicle_index)->object.position;
	data.forward = object_get(seat->vehicle_index)->object.forward;
	data.up = *global_up3d;
	unit_index = object_new(&data);
	if (unit_index == NONE)
		return;
	unit_delete_all_weapons(unit_index);
	seat->stand_in = TRUE;
	cryo_seat_unit(seat, unit_index);
}

static void cryo_arm(
	void)
{
	long tube_definition_index = tag_loaded(VEHICLE_DEFINITION_TAG, CRYO_TUBE);
	long waiting[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	struct data_iterator iterator;
	struct player_datum *player;
	short waiting_count = 0;
	short player_count = 0;
	short stand_in_count = 0;
	short index;

	coop_cryo.armed = TRUE;
	coop_cryo.seats[0].vehicle_index = cryo_named_object("cryotube_1");
	coop_cryo.seats[1].vehicle_index = cryo_named_object("cryotube_2");
	for (index = 0; index < CRYO_TUBES; index++)
	{
		struct object_placement_data data;

		coop_cryo.tube_indices[index] = NONE;
		if (tube_definition_index != NONE)
		{
			object_placement_data_new(&data, tube_definition_index, NONE);
			cryo_tube_frame(index, &data.position, &data.forward);
			data.up = *global_up3d;
			coop_cryo.tube_indices[index] = object_new(&data);
		}
		coop_cryo.seats[CRYO_BUNGIE_SEATS + index].vehicle_index = coop_cryo.tube_indices[index];
	}

	data_iterator_new(&iterator, player_data);
	while ((player = data_iterator_next(&iterator)) != NULL && waiting_count < NUMBEROF(waiting))
	{
		if (player->unit_index == NONE && !player->quit_out_of_game && player->statistics.deaths == 0)
			waiting[waiting_count++] = iterator.datum_index;
	}
	for (index = 0; index < CRYO_SEATS; index++)
	{
		struct cryo_seat *seat = &coop_cryo.seats[index];

		/* (filled by the scripts) */
		if (seat->vehicle_index == NONE || (seat->unit_index = cryo_occupant(seat->vehicle_index)) != NONE)
			continue;
		if (waiting_count > 0)
		{
			short pick = random_range(0, waiting_count);

			seat->player_index = waiting[pick];
			waiting[pick] = waiting[--waiting_count];
			player_count++;
		}
		else
		{
			cryo_stand_in(seat);
			stand_in_count++;
		}
	}
	error(_error_silent, "co-op: the cryo bay seats %d waiting players and %d stand-ins", player_count, stand_in_count);
}

static void cryo_intro_tick(
	long tick)
{
	short tube;
	short index;

	for (tube = 0; tube < CRYO_TUBES; tube++)
	{
		long open = (tube < CRYO_ROW_TUBES ? CRYO_NORTH_OPEN : CRYO_SOUTH_OPEN) +
			CRYO_RIPPLE_TICKS * (tube % CRYO_ROW_TUBES);

		if (tick == open && coop_cryo.tube_indices[tube] != NONE)
			unit_open(coop_cryo.tube_indices[tube]);
	}
	switch (tick)
	{
	case CRYO_SEATS_AGAIN:
		for (index = 0; index < CRYO_SEATS; index++)
			cryo_seat_note(&coop_cryo.seats[index]);
		break;
	case 0:
		cryo_bungie_jets(FALSE);
		cryo_camera_set(_cryo_camera_north_start, 0);
		player_effect_screen_fade_in(1.0f, 1.0f, 1.0f, CRYO_WHITE_IN_TICKS);
		break;
	case CRYO_NORTH_PAN:
		cryo_camera_set(_cryo_camera_north_end, CRYO_PAN_TICKS);
		break;
	case CRYO_SOUTH_CUT:
		cryo_camera_set(_cryo_camera_south_start, 0);
		break;
	case CRYO_SOUTH_PAN:
		cryo_camera_set(_cryo_camera_south_end, CRYO_PAN_TICKS);
		break;
	case CRYO_CRANE:
		cryo_camera_set(_cryo_camera_crane_low, 0);
		cryo_camera_set(_cryo_camera_crane_high, CRYO_CRANE_TICKS);
		break;
	case CRYO_WHITE_OUT:
		player_effect_screen_fade_out(1.0f, 1.0f, 1.0f, CRYO_WHITE_OUT_TICKS);
		break;
	case CRYO_HERO:
		cryo_bungie_jets(TRUE);
		scripted_camera_set_absolute(cryo_camera_point_index("tutorial_action_2"), 0);
		scripted_camera_set_absolute(cryo_camera_point_index("tutorial_action_1"), CRYO_HERO_TICKS);
		player_effect_screen_fade_in(1.0f, 1.0f, 1.0f, CRYO_WHITE_IN_TICKS);
		coop_cryo.released = TRUE;
		break;
	}
}

/* every script thread sleeps through it */
static void cryo_intro_update(
	void)
{
	long tick = game_time_get() - coop_cryo.intro_time;

	while (coop_cryo.intro_tick < tick && !coop_cryo.released)
	{
		cryo_intro_tick(++coop_cryo.intro_tick);
		if (!coop_cryo.released)
			hs_runtime_port_shift_sleep_times(1);
	}
}

/* the scripts' player0 is the newest player with a unit, by then not
necessarily the one in cryotube_1 */
static void cryo_exits_update(
	void)
{
	long now = game_time_get();
	short index;

	if (coop_cryo.first_exit_time == NONE)
	{
		for (index = 0; index < CRYO_SEATS && coop_cryo.first_exit_time == NONE; index++)
		{
			struct unit_datum *unit = object_try_and_get_and_verify_type(coop_cryo.seats[index].unit_index,
				_object_mask_unit);

			if (unit && unit->object.parent_object_index == coop_cryo.seats[index].vehicle_index &&
				unit->unit.animation.state == _unit_state_exiting_seat)
			{
				coop_cryo.first_exit_time = now;
			}
		}
		if (coop_cryo.first_exit_time == NONE)
			return;
		for (index = 0; index < CRYO_SEATS; index++)
			coop_cryo.seats[index].exit_time = now + random_range(0, CRYO_EXIT_JITTER_TICKS + 1);
	}
	for (index = 0; index < CRYO_SEATS; index++)
	{
		struct cryo_seat *seat = &coop_cryo.seats[index];

		if (seat->exit_time != NONE && seat->exit_time <= now)
		{
			seat->exit_time = NONE;
			if (seat->unit_index != NONE && object_try_and_get_and_verify_type(seat->unit_index, _object_mask_unit) &&
				object_get(seat->unit_index)->object.parent_object_index == seat->vehicle_index)
			{
				unit_scripting_exit_vehicle(seat->unit_index);
				if (seat->stand_in)
					network_coop_note_unit_seat(seat->unit_index, NONE, NONE);
			}
		}
	}
}

static void cryo_after_intro_update(
	void)
{
	long now = game_time_get();
	short index;

	if (coop_cryo.released && !coop_cryo.stand_ins_gone && !cinematic_in_progress())
	{
		coop_cryo.stand_ins_gone = TRUE;
		for (index = 0; index < CRYO_SEATS; index++)
		{
			struct cryo_seat *seat = &coop_cryo.seats[index];

			if (seat->stand_in && object_try_and_get_and_verify_type(seat->unit_index, _object_mask_unit))
				object_delete(seat->unit_index);
			seat->unit_index = NONE;
		}
	}
	if (coop_cryo.close_time == NONE)
	{
		struct unit_datum *tube = object_try_and_get_and_verify_type(cryo_named_object("cryotube_1"),
			_object_mask_vehicle);

		if (coop_cryo.released && tube && tube->unit.animation.state == _unit_state_closing)
			coop_cryo.close_time = now;
		return;
	}
	for (index = 0; index < CRYO_TUBES; index++)
	{
		if (coop_cryo.close_time + CRYO_CLOSE_DELAY_TICKS + CRYO_CLOSE_TICKS * index == now &&
			object_try_and_get_and_verify_type(coop_cryo.tube_indices[index], _object_mask_vehicle))
		{
			unit_close(coop_cryo.tube_indices[index]);
		}
	}
}

static void cryo_host_update(
	void)
{
	if (!coop_cryo.armed)
	{
		long occupant_index = cryo_occupant(cryo_named_object("cryotube_1"));
		short hero_index = cryo_camera_point_index("tutorial_action_1");

		/* (fast_setup seats it and cuts to Bungie's shot in one tick) */
		if (occupant_index == NONE || player_index_from_unit_index(occupant_index) == NONE ||
			!cinematic_in_progress() || hero_index == NONE || scripted_camera_next_camera_point() != hero_index)
		{
			return;
		}
		cryo_arm();
		coop_cryo.intro_time = game_time_get();
		coop_cryo.intro_tick = -1;
	}
	cryo_intro_update();
	cryo_exits_update();
	cryo_after_intro_update();
}

static void cryo_tubes_find(
	void)
{
	long definition_index = tag_loaded(VEHICLE_DEFINITION_TAG, CRYO_TUBE);
	struct object_iterator iterator;
	struct object_datum *object;

	object_iterator_new(&iterator, _object_mask_vehicle, 0);
	while ((object = object_iterator_next(&iterator)) != NULL)
	{
		short tube;

		if (object->definition_index != definition_index || cryo_tube_of(iterator.index) != NONE)
			continue;
		for (tube = 0; tube < CRYO_TUBES; tube++)
		{
			real_point3d position;
			real_vector3d forward;

			cryo_tube_frame(tube, &position, &forward);
			if (coop_cryo.tube_indices[tube] == NONE && distance_squared3d(&position, &object->object.position) < 0.01f)
				coop_cryo.tube_indices[tube] = iterator.index;
		}
	}
}

static long cryo_jet_new(
	long definition_index,
	short tube,
	short jet)
{
	struct cryo_jet const *source = &cryo_jets[jet];
	struct object_placement_data data;
	real_point3d origin;
	real_vector3d forward;
	real_vector3d left;
	real_matrix4x3 matrix;
	real yaw;

	cryo_tube_frame(tube, &origin, &forward);
	cross_product3d(global_up3d, &forward, &left);
	yaw = forward.j < 0.0f ? -90.0f : 90.0f;
	matrix4x3_rotation_from_angles(&matrix, (source->angles.yaw + yaw) * _pi / 180.0f,
		source->angles.pitch * _pi / 180.0f, source->angles.roll * _pi / 180.0f);
	object_placement_data_new(&data, definition_index, NONE);
	data.position.x = origin.x + forward.i * source->offset.x + left.i * source->offset.y;
	data.position.y = origin.y + forward.j * source->offset.x + left.j * source->offset.y;
	data.position.z = origin.z + source->offset.z;
	data.forward = matrix.forward;
	data.up = matrix.up;
	return object_new(&data);
}

/* a late joiner's open tubes don't vent */
static void cryo_steam_update(
	void)
{
	long now = game_time_get();
	long jet_definition_index = NONE;
	short tube;

	if (!coop_cryo_host())
		cryo_tubes_find();
	for (tube = 0; tube < CRYO_TUBES; tube++)
	{
		struct unit_datum *unit = object_try_and_get_and_verify_type(coop_cryo.tube_indices[tube],
			_object_mask_vehicle);
		short jet;

		if (!unit)
		{
			coop_cryo.tube_indices[tube] = NONE;
			continue;
		}
		if (unit->unit.animation.state != _unit_state_opening)
			coop_cryo.seen_shut[tube] = TRUE;
		else if (coop_cryo.seen_shut[tube] && coop_cryo.open_times[tube] == NONE)
			coop_cryo.open_times[tube] = now;
		if (coop_cryo.open_times[tube] == NONE)
			continue;
		if (now == coop_cryo.open_times[tube] + CRYO_HOOD_LIFT_TICKS)
		{
			if (jet_definition_index == NONE)
				jet_definition_index = tag_loaded('scen', CRYO_STEAM_JET);
			for (jet = 0; jet_definition_index != NONE && jet < CRYO_JETS; jet++)
				coop_cryo.jet_indices[tube][jet] = cryo_jet_new(jet_definition_index, tube, jet);
		}
		else if (now >= coop_cryo.open_times[tube] + CRYO_HOOD_LIFT_TICKS + CRYO_STEAM_TICKS)
		{
			for (jet = 0; jet < CRYO_JETS; jet++)
			{
				if (coop_cryo.jet_indices[tube][jet] != NONE &&
					object_try_and_get_and_verify_type(coop_cryo.jet_indices[tube][jet], _object_mask_scenery))
				{
					object_delete(coop_cryo.jet_indices[tube][jet]);
				}
				coop_cryo.jet_indices[tube][jet] = NONE;
			}
		}
	}
}

/* ---------- public code */

void coop_cryo_new_game(
	void)
{
	short index;

	csmemset(&coop_cryo, 0, sizeof(coop_cryo));
	coop_cryo.intro_time = NONE;
	coop_cryo.first_exit_time = NONE;
	coop_cryo.close_time = NONE;
	for (index = 0; index < CRYO_SEATS; index++)
	{
		coop_cryo.seats[index].vehicle_index = NONE;
		coop_cryo.seats[index].unit_index = NONE;
		coop_cryo.seats[index].player_index = NONE;
		coop_cryo.seats[index].exit_time = NONE;
	}
	csmemset(coop_cryo.tube_indices, NONE, sizeof(coop_cryo.tube_indices));
	csmemset(coop_cryo.open_times, NONE, sizeof(coop_cryo.open_times));
	csmemset(coop_cryo.jet_indices, NONE, sizeof(coop_cryo.jet_indices));
}

void coop_cryo_update(
	void)
{
	if (!coop_cryo_active())
		return;
	if (coop_cryo_host())
		cryo_host_update();
	cryo_steam_update();
}

void coop_cryo_place_cells(
	void)
{
	struct object_iterator iterator;
	struct object_datum *object;
	long definition_index;
	short cell;

	if (!coop_cryo_active() || global_structure_bsp_index_get() != 0 ||
		(definition_index = coop_cryo_cell_definition()) == NONE)
	{
		return;
	}
	object_iterator_new(&iterator, _object_mask_scenery, 0);
	while ((object = object_iterator_next(&iterator)) != NULL)
	{
		if (object->definition_index == definition_index)
			return;
	}
	for (cell = 0; cell < CRYO_TUBES / 2; cell++)
	{
		struct object_placement_data data;
		real_point3d position;
		real_vector3d forward;

		object_placement_data_new(&data, definition_index, NONE);
		cryo_tube_frame(2 * cell, &position, &forward);
		data.position = position;
		data.position.x = (cryo_tube_x[(2 * cell) % CRYO_ROW_TUBES] + cryo_tube_x[(2 * cell + 1) % CRYO_ROW_TUBES]) / 2.0f;
		data.position.z = 0.0f;
		data.forward = forward;
		data.up = *global_up3d;
		object_new(&data);
	}
}

void coop_cryo_tags_unloaded(
	void)
{
	coop_cryo_cell_release();
}

boolean coop_cryo_scripts_cooperative(
	void)
{
	return coop_cryo_active();
}

boolean coop_cryo_placement_skipped(
	short object_type,
	struct scenario_object_datum const *placement,
	struct tag_block *palette)
{
	struct scenario_object_palette_entry *entry;

	if (object_type != _object_type_machine || !VALID_INDEX(placement->palette_entry_index, palette->count) ||
		!coop_cryo_active())
	{
		return FALSE;
	}
	entry = TAG_BLOCK_GET_ELEMENT(palette, placement->palette_entry_index, struct scenario_object_palette_entry);
	return entry->reference.index != NONE && !csstrcasecmp(tag_get_name(entry->reference.index), CRYO_TUBE) &&
		placement->position.x > CRYO_BAY_X0 && placement->position.x < CRYO_BAY_X1 &&
		placement->position.y > -CRYO_BAY_Y && placement->position.y < CRYO_BAY_Y;
}

boolean coop_cryo_seat_waiting(
	long player_index)
{
	short index;

	if (!coop_cryo.armed || coop_cryo.released || !coop_cryo_host() || !coop_cryo_active())
		return FALSE;
	for (index = 0; index < CRYO_SEATS; index++)
	{
		if (coop_cryo.seats[index].player_index == player_index)
			return TRUE;
	}
	return FALSE;
}

void coop_cryo_seat_player(
	long player_index)
{
	struct player_datum *player = player_get(player_index);
	short index;

	for (index = 0; index < CRYO_SEATS; index++)
	{
		struct cryo_seat *seat = &coop_cryo.seats[index];

		if (seat->player_index != player_index)
			continue;
		seat->player_index = NONE;
		if (player->unit_index != NONE && object_try_and_get_and_verify_type(seat->vehicle_index, _object_mask_vehicle))
			cryo_seat_unit(seat, player->unit_index);
	}
}

void coop_cryo_exit_seat_end(
	long unit_index,
	long vehicle_index)
{
	short tube;
	real_point3d position;
	real_vector3d forward;

	if (!coop_cryo_active() || (tube = cryo_tube_of(vehicle_index)) == NONE)
		return;
	cryo_tube_frame(tube, &position, &forward);
	position.x += forward.i * CRYO_EXIT_DISTANCE;
	position.y += forward.j * CRYO_EXIT_DISTANCE;
	position.z = object_get(unit_index)->object.position.z;
	object_set_position(unit_index, &position, &forward, global_up3d);
	cryo_unit_face(unit_index, &forward);
}

