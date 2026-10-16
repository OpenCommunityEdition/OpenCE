/*
COOP_CRYO.C (test)

The world port/linux/game/coop_cryo.c's intro, rows, seats and exits run in
(test_coop_cryo.py).
*/

#include "harness.h"

#include <math.h>

typedef unsigned short word;

#define TICKS_PER_SECOND 30
#define NUMBEROF(array) ((long)(sizeof(array) / sizeof((array)[0])))
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))
#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS 128
#define VEHICLE_DEFINITION_TAG 0x76656869
#define CRYO_TUBE "levels\\a10\\devices\\cryotube\\cryotube"
#define _error_silent 0

static void error(int level, char const *format, ...)
{
}

enum
{
	_object_mask_unit = 1,
	_object_mask_vehicle = 2,
	_object_mask_scenery = 4,
};

enum
{
	_unit_state_idle = 0,
	_unit_state_exiting_seat = 27,
};

#include "config.inc"

/* ---------- the world */

struct unit_datum
{
	struct
	{
		long parent_object_index;
		real_point3d position;
		real_vector3d forward;
	} object;
	struct
	{
		struct
		{
			short state;
		} animation;
		short parent_seat_index;
	} unit;
};

struct object_placement_data
{
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
};

struct player_datum
{
	long unit_index;
	boolean quit_out_of_game;
	struct
	{
		short deaths;
	} statistics;
};

struct data_iterator
{
	long datum_index;
	long next;
};

enum
{
	MAXIMUM_OBJECTS = 512,
	CRYOTUBE_1 = 1,
	CRYOTUBE_2 = 2,
};

static struct unit_datum objects[MAXIMUM_OBJECTS];
static unsigned long object_types[MAXIMUM_OBJECTS];
static long object_count;
static long now;
static struct player_datum players[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static long player_count;
static void *player_data;
static real_vector3d const up3d = { 0.0f, 0.0f, 1.0f };
static real_vector3d const *global_up3d = &up3d;
static unsigned long seed = 1;

static short cryo_tube_of(long object_index);

static long object_add(
	unsigned long type,
	long parent_index)
{
	long index = object_count++;

	memset(&objects[index], 0, sizeof(objects[index]));
	objects[index].object.parent_object_index = parent_index;
	object_types[index] = type;
	return index;
}

static void *object_try_and_get_and_verify_type(long index, unsigned long mask)
{
	return index >= 0 && index < MAXIMUM_OBJECTS && (object_types[index] & mask) ? &objects[index] : NULL;
}

static struct unit_datum *object_get(long index)
{
	return &objects[index];
}

static long game_time_get(void)
{
	return now;
}

/* [lower, upper), as random_range */
static short random_range(short lower, short upper)
{
	seed = seed * 1664525UL + 1013904223UL;
	return (short)(lower + (long)((seed >> 16) % (unsigned long)(upper - lower)));
}

static void data_iterator_new(struct data_iterator *iterator, void *data)
{
	iterator->next = 0;
	iterator->datum_index = NONE;
}

static struct player_datum *data_iterator_next(struct data_iterator *iterator)
{
	if (iterator->next >= player_count)
		return NULL;
	iterator->datum_index = 0xE0000 | iterator->next;
	return &players[iterator->next++];
}

static long tag_loaded(long group_tag, char const *name)
{
	return 7;
}

static void object_placement_data_new(struct object_placement_data *data, long definition_index, long owner_index)
{
	memset(data, 0, sizeof(*data));
}

static long object_new(struct object_placement_data *data)
{
	long index = object_add(_object_mask_vehicle, NONE);

	objects[index].object.position = data->position;
	objects[index].object.forward = data->forward;
	return index;
}

static long cryo_named_object(char const *name)
{
	return !strcmp(name, "cryotube_1") ? CRYOTUBE_1 : !strcmp(name, "cryotube_2") ? CRYOTUBE_2 : NONE;
}

static long cryo_occupant(long vehicle_index)
{
	long index;

	for (index = 0; vehicle_index != NONE && index < object_count; index++)
	{
		if ((object_types[index] & _object_mask_unit) && objects[index].object.parent_object_index == vehicle_index)
			return index;
	}
	return NONE;
}

static void cryo_stand_in(struct cryo_seat *seat)
{
	seat->stand_in = TRUE;
	seat->unit_index = object_add(_object_mask_unit, seat->vehicle_index);
}

enum
{
	EVENT_OPEN,
	EVENT_CAMERA,
	EVENT_FADE_IN,
	EVENT_FADE_OUT,
	EVENT_HERO_CAMERA,
	EVENT_JETS,
};

static struct
{
	long tick;
	int kind;
	long a;
	long b;
} events[1024];
static int event_count;

static void note(int kind, long a, long b)
{
	if (event_count < NUMBEROF(events))
	{
		events[event_count].tick = now;
		events[event_count].kind = kind;
		events[event_count].a = a;
		events[event_count].b = b;
		event_count++;
	}
}

static void unit_open(long index) { note(EVENT_OPEN, cryo_tube_of(index), 0); }
static void cryo_seat_note(struct cryo_seat const *seat) {}
static void cryo_bungie_jets(boolean create) { note(EVENT_JETS, create, 0); }
static void cryo_camera_set(short camera, word ticks) { note(EVENT_CAMERA, camera, ticks); }
static void player_effect_screen_fade_in(real red, real green, real blue, short ticks) { note(EVENT_FADE_IN, ticks, 0); }
static void player_effect_screen_fade_out(real red, real green, real blue, short ticks) { note(EVENT_FADE_OUT, ticks, 0); }
static void scripted_camera_set_absolute(short point, word ticks) { note(EVENT_HERO_CAMERA, point, ticks); }

static short cryo_camera_point_index(char const *name)
{
	return !strcmp(name, "tutorial_action_2") ? 0 : !strcmp(name, "tutorial_action_1") ? 1 : NONE;
}

static long exit_times[MAXIMUM_OBJECTS];
static int exits[MAXIMUM_OBJECTS];
static boolean sent_out[MAXIMUM_OBJECTS];
static real_vector3d faced[MAXIMUM_OBJECTS];

static void unit_scripting_exit_vehicle(long unit_index)
{
	exit_times[unit_index] = now;
	exits[unit_index]++;
	objects[unit_index].unit.animation.state = _unit_state_exiting_seat;
}

static void network_coop_note_unit_seat(long unit_index, long vehicle_index, short seat_index)
{
	if (vehicle_index == NONE)
		sent_out[unit_index] = TRUE;
}

static boolean coop_cryo_active(void)
{
	return TRUE;
}

static void object_set_position(long index, real_point3d const *position, real_vector3d const *forward,
	real_vector3d const *up)
{
	objects[index].object.position = *position;
	objects[index].object.forward = *forward;
}

static void cryo_unit_face(long unit_index, real_vector3d const *forward)
{
	faced[unit_index] = *forward;
}

#include "under_test.inc"

/* ---------- the cases */

/* the design's numbers */
#define RIPPLE 3
#define LAST_HOOD_TO_PAN (30 + 72)
#define EXIT_DISTANCE 0.849f
#define EXIT_JITTER 24

static void world_new(void)
{
	memset(objects, 0, sizeof(objects));
	memset(object_types, 0, sizeof(object_types));
	memset(players, 0, sizeof(players));
	memset(exits, 0, sizeof(exits));
	memset(sent_out, 0, sizeof(sent_out));
	memset(&coop_cryo, 0, sizeof(coop_cryo));
	object_count = 0;
	player_count = 0;
	event_count = 0;
	now = 0;
	object_add(0, NONE);
	object_add(_object_mask_vehicle, NONE);
	object_add(_object_mask_vehicle, NONE);
	coop_cryo.first_exit_time = NONE;
}

static void intro_play(void)
{
	short tube;

	world_new();
	for (tube = 0; tube < CRYO_TUBES; tube++)
		coop_cryo.tube_indices[tube] = object_add(_object_mask_vehicle, NONE);
	for (now = 0; now <= CRYO_HERO && !coop_cryo.released; now++)
		cryo_intro_tick(now);
}

static long event_tick(int kind, long a, long b)
{
	int index;

	for (index = 0; index < event_count; index++)
	{
		if (events[index].kind == kind && events[index].a == a && (b == NONE || events[index].b == b))
			return events[index].tick;
	}
	return NONE;
}

static void add_player(long unit_index, boolean quit, short deaths)
{
	players[player_count].unit_index = unit_index;
	players[player_count].quit_out_of_game = quit;
	players[player_count].statistics.deaths = deaths;
	player_count++;
}

static int seated_once(void)
{
	long player_index;
	int seated = 0;

	for (player_index = 0; player_index < player_count; player_index++)
	{
		int count = 0;
		short index;

		for (index = 0; index < CRYO_SEATS; index++)
			count += coop_cryo.seats[index].player_index == (0xE0000 | player_index);
		CHECK(count <= 1, "player %ld has %d seats", player_index, count);
		CHECK(!count || players[player_index].unit_index == NONE, "player %ld has a unit and a seat", player_index);
		seated += count;
	}
	return seated;
}

static void seats_new(void)
{
	short index;

	for (index = 0; index < CRYO_SEATS; index++)
	{
		coop_cryo.seats[index].vehicle_index = NONE;
		coop_cryo.seats[index].unit_index = NONE;
		coop_cryo.seats[index].player_index = NONE;
		coop_cryo.seats[index].exit_time = NONE;
	}
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	CASE("hoods")
	{
		long white = NONE;
		long opens[CRYO_TUBES];
		short tube;
		int index;

		intro_play();
		for (index = 0; index < event_count; index++)
		{
			if (events[index].kind == EVENT_FADE_IN && events[index].tick == 0)
				white = events[index].a;
		}
		CHECK(white > 0, "no fade in from white at the start");
		for (tube = 0; tube < CRYO_TUBES; tube++)
		{
			int count = 0;

			opens[tube] = NONE;
			for (index = 0; index < event_count; index++)
			{
				if (events[index].kind == EVENT_OPEN && events[index].a == tube)
				{
					opens[tube] = events[index].tick;
					count++;
				}
			}
			CHECK(count == 1, "tube %d opens %d times", tube, count);
			CHECK(opens[tube] >= white, "tube %d opens at %ld, in the white (to %ld)", tube, opens[tube], white);
		}
		for (tube = 0; tube < CRYO_TUBES; tube++)
		{
			short row = tube / CRYO_ROW_TUBES;
			long cut = event_tick(EVENT_CAMERA, row ? _cryo_camera_south_start : _cryo_camera_north_start, 0);
			long pan = event_tick(EVENT_CAMERA, row ? _cryo_camera_south_end : _cryo_camera_north_end, NONE);

			CHECK(cut != NONE && pan != NONE, "row %d has no shot", row);
			CHECK(opens[tube] >= cut && opens[tube] < pan, "tube %d opens at %ld, outside its row's shot %ld..%ld",
				tube, opens[tube], cut, pan);
			if (tube % CRYO_ROW_TUBES)
			{
				CHECK(opens[tube] - opens[tube - 1] == RIPPLE, "tube %d opens %ld ticks after the one before", tube,
					opens[tube] - opens[tube - 1]);
				CHECK(cryo_tube_x[tube % CRYO_ROW_TUBES] < cryo_tube_x[tube % CRYO_ROW_TUBES - 1],
					"tube %d is not further from the doors than the one before", tube);
			}
			else
			{
				CHECK(pan - opens[tube + CRYO_ROW_TUBES - 1] == LAST_HOOD_TO_PAN,
					"row %d pans %ld ticks after its last hood opens, not %d", row,
					pan - opens[tube + CRYO_ROW_TUBES - 1], LAST_HOOD_TO_PAN);
			}
		}
		return 0;
	}
	CASE("shots")
	{
		long north, north_pan, south, south_pan, crane, white, hero;

		intro_play();
		north = event_tick(EVENT_CAMERA, _cryo_camera_north_start, 0);
		north_pan = event_tick(EVENT_CAMERA, _cryo_camera_north_end, NONE);
		south = event_tick(EVENT_CAMERA, _cryo_camera_south_start, 0);
		south_pan = event_tick(EVENT_CAMERA, _cryo_camera_south_end, NONE);
		crane = event_tick(EVENT_CAMERA, _cryo_camera_crane_high, NONE);
		white = event_tick(EVENT_FADE_OUT, CRYO_WHITE_OUT_TICKS, NONE);
		hero = event_tick(EVENT_HERO_CAMERA, 1, NONE);
		CHECK(north == 0, "the north row's shot starts at %ld", north);
		CHECK(south == north_pan + CRYO_PAN_TICKS, "the south row's cut at %ld, not when the north pan ends", south);
		CHECK(crane == south_pan + CRYO_PAN_TICKS, "the crane at %ld, not when the south pan ends", crane);
		CHECK(event_tick(EVENT_CAMERA, _cryo_camera_crane_low, 0) == crane, "the crane does not start low");
		CHECK(white == crane + CRYO_CRANE_TICKS, "the white at %ld, not as the crane ends", white);
		CHECK(hero == white + CRYO_WHITE_OUT_TICKS, "Bungie's shot at %ld, not once white", hero);
		CHECK(event_tick(EVENT_HERO_CAMERA, 0, 0) == hero, "Bungie's shot does not start at tutorial_action_2");
		CHECK(event_tick(EVENT_JETS, TRUE, NONE) == hero, "Bungie's jets are not back for his shot");
		CHECK(event_tick(EVENT_JETS, FALSE, NONE) == 0, "Bungie's jets are not held back from the start");
		CHECK(coop_cryo.released && now == hero + 1, "the scripts run again at %ld, not with Bungie's shot", now - 1);
		return 0;
	}
	CASE("rows")
	{
		short tube;

		for (tube = 0; tube < CRYO_TUBES; tube++)
		{
			real_point3d position, other_position;
			real_vector3d forward, other_forward;
			short other = (tube + CRYO_ROW_TUBES) % CRYO_TUBES;

			cryo_tube_frame(tube, &position, &forward);
			cryo_tube_frame(other, &other_position, &other_forward);
			CHECK(fabsf(position.y) > 2.0f && fabsf(position.y) < 3.8f, "tube %d at y %f", tube, position.y);
			CHECK(position.x > -62.4f && position.x < -51.5f, "tube %d at x %f, outside the bay", tube, position.x);
			CHECK(fabsf(position.y + forward.j * EXIT_DISTANCE) < fabsf(position.y),
				"tube %d faces away from the aisle", tube);
			CHECK(position.x == other_position.x && position.y == -other_position.y &&
				forward.j == -other_forward.j, "tube %d is not across the aisle from tube %d", tube, other);
			if (tube % 2)
			{
				real_point3d previous;
				real_vector3d previous_forward;

				cryo_tube_frame(tube - 1, &previous, &previous_forward);
				CHECK(fabsf(previous.x - position.x - 0.804f) < 0.001f, "tubes %d and %d are %f apart, not a cell's",
					tube - 1, tube, previous.x - position.x);
			}
		}
		return 0;
	}
	CASE("exit-spot")
	{
		short tube;

		world_new();
		for (tube = 0; tube < CRYO_TUBES; tube++)
			coop_cryo.tube_indices[tube] = object_add(_object_mask_vehicle, NONE);
		for (tube = 0; tube < CRYO_TUBES; tube++)
		{
			long unit_index = object_add(_object_mask_unit, coop_cryo.tube_indices[tube]);
			real_point3d position;
			real_vector3d forward;

			objects[unit_index].object.position.z = 0.02f;
			coop_cryo_exit_seat_end(unit_index, coop_cryo.tube_indices[tube]);
			cryo_tube_frame(tube, &position, &forward);
			CHECK(fabsf(objects[unit_index].object.position.x - (position.x + forward.i * EXIT_DISTANCE)) < 0.0001f &&
				fabsf(objects[unit_index].object.position.y - (position.y + forward.j * EXIT_DISTANCE)) < 0.0001f,
				"tube %d's Chief stands at %f %f", tube, objects[unit_index].object.position.x,
				objects[unit_index].object.position.y);
			CHECK(objects[unit_index].object.position.z == 0.02f, "tube %d's Chief moved up or down", tube);
			CHECK(objects[unit_index].object.forward.j == forward.j && faced[unit_index].j == forward.j,
				"tube %d's Chief does not face out", tube);
		}
		{
			long unit_index = object_add(_object_mask_unit, CRYOTUBE_1);

			objects[unit_index].object.position.x = 1.0f;
			coop_cryo_exit_seat_end(unit_index, CRYOTUBE_1);
			CHECK(objects[unit_index].object.position.x == 1.0f, "Bungie's tube's Chief was moved");
		}
		return 0;
	}
	CASE("exits")
	{
		short index;
		long first;

		world_new();
		seats_new();
		for (index = 0; index < CRYO_SEATS; index++)
		{
			struct cryo_seat *seat = &coop_cryo.seats[index];

			seat->vehicle_index = index < 2 ? index + 1 : object_add(_object_mask_vehicle, NONE);
			seat->unit_index = object_add(_object_mask_unit, seat->vehicle_index);
			seat->stand_in = index >= 5;
		}
		for (now = 100; now < 200; now++)
			cryo_exits_update();
		for (index = 0; index < CRYO_SEATS; index++)
			CHECK(!exits[coop_cryo.seats[index].unit_index], "seat %d climbed out before anyone", index);
		first = coop_cryo.seats[3].unit_index;
		objects[first].unit.animation.state = _unit_state_exiting_seat;
		for (now = 200; now < 400; now++)
			cryo_exits_update();
		for (index = 0; index < CRYO_SEATS; index++)
		{
			long unit_index = coop_cryo.seats[index].unit_index;

			if (unit_index == first)
				continue;
			CHECK(exits[unit_index] == 1, "seat %d climbed out %d times", index, exits[unit_index]);
			CHECK(exit_times[unit_index] >= 200 && exit_times[unit_index] <= 200 + EXIT_JITTER,
				"seat %d climbed out %ld ticks after the first", index, exit_times[unit_index] - 200);
			CHECK(sent_out[unit_index] == coop_cryo.seats[index].stand_in, "seat %d's climb out %s to the clients",
				index, sent_out[unit_index] ? "went" : "did not go");
		}
		return 0;
	}
	CASE("seating")
	{
		long scripts_unit_index;
		short index;
		int stand_ins = 0;

		world_new();
		seats_new();
		scripts_unit_index = object_add(_object_mask_unit, CRYOTUBE_1);
		add_player(scripts_unit_index, FALSE, 0);
		add_player(NONE, FALSE, 0);
		add_player(NONE, TRUE, 0);
		add_player(NONE, FALSE, 0);
		add_player(NONE, FALSE, 1);
		add_player(NONE, FALSE, 0);
		cryo_arm();
		CHECK(coop_cryo.seats[0].unit_index == scripts_unit_index && coop_cryo.seats[0].player_index == NONE,
			"the scripts' player lost cryotube_1");
		CHECK(seated_once() == 3, "%d players seated, not the 3 waiting", seated_once());
		CHECK(coop_cryo.seats[1].player_index != NONE, "cryotube_2 does not wait for a player");
		CHECK(coop_cryo.seats[2].player_index != NONE && coop_cryo.seats[3].player_index != NONE,
			"the first row seats do not wait for players");
		for (index = 0; index < CRYO_SEATS; index++)
			stand_ins += coop_cryo.seats[index].stand_in;
		CHECK(stand_ins == CRYO_SEATS - 4, "%d stand-ins, not %d", stand_ins, CRYO_SEATS - 4);
		for (index = 0; index < CRYO_TUBES; index++)
		{
			real_point3d position;
			real_vector3d forward;
			long tube_index = coop_cryo.tube_indices[index];

			cryo_tube_frame(index, &position, &forward);
			CHECK(tube_index != NONE && coop_cryo.seats[2 + index].vehicle_index == tube_index,
				"row tube %d is not seat %d", index, 2 + index);
			CHECK(objects[tube_index].object.position.x == position.x &&
				objects[tube_index].object.position.y == position.y, "row tube %d is not where the table has it",
				index);
		}
		return 0;
	}
	CASE("seating-crowded")
	{
		short index;

		world_new();
		seats_new();
		add_player(object_add(_object_mask_unit, CRYOTUBE_1), FALSE, 0);
		for (index = 0; index < 25; index++)
			add_player(NONE, FALSE, 0);
		cryo_arm();
		CHECK(seated_once() == CRYO_SEATS - 1, "%d players seated, not %d", seated_once(), CRYO_SEATS - 1);
		for (index = 0; index < CRYO_SEATS; index++)
			CHECK(!coop_cryo.seats[index].stand_in, "seat %d has a stand-in", index);
		return 0;
	}
	CASE("seating-filled")
	{
		long second_unit_index;

		world_new();
		seats_new();
		add_player(object_add(_object_mask_unit, CRYOTUBE_1), FALSE, 0);
		second_unit_index = object_add(_object_mask_unit, CRYOTUBE_2);
		add_player(second_unit_index, FALSE, 0);
		add_player(NONE, FALSE, 0);
		cryo_arm();
		CHECK(coop_cryo.seats[1].unit_index == second_unit_index && coop_cryo.seats[1].player_index == NONE,
			"the scripts' player lost cryotube_2");
		CHECK(coop_cryo.seats[2].player_index != NONE, "the waiting player is not in the first row seat");
		CHECK(seated_once() == 1, "%d players seated, not 1", seated_once());
		return 0;
	}

	fprintf(stderr, "no case %s\n", case_name);
	return 2;
}
