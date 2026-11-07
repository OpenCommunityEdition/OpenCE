/*
SWARM_SPAWNED_ACTORS.C (test)

The real actors_spawn_from_unit (actors.c), as a unit that spawns actors as it
dies (unit_test_spawning: a Flood carrier's infection forms) dies: the actors
it makes join the encounter and squad of the actor that drove it, the swarm's
for a unit of a swarm, whose own actor_index is NONE, or the unit's fake
encounter and squad with no actor. January read the unit's actor_index even
for a unit of a swarm, actor_get(NONE), which datum_get answers with NULL in a
release build, and the game crashed (a Custom Edition map's unit of a swarm
that spawns actors: no Xbox map has one). The fake world's actor_get refuses
an actor that is not there. test_swarm_spawned_actors.py takes the enums and
the code from the sources (config.inc, under_test.inc).
*/

#include "harness.h"

#define _pi 3.14159265f
#define _error_silent 0

#include "config.inc"

/* the units, in their slots: the one that dies and the ones it spawns */
#define MAXIMUM_UNITS 16
struct unit_datum
{
	struct { short type; } object;
	struct
	{
		long actor_index;
		long swarm_actor_index;
		short fake_encounter_index;
		short fake_squad_index;
	} unit;
};
static struct unit_datum units[MAXIMUM_UNITS];
static long unit_count;

static struct unit_datum *unit_get(long unit_index)
{
	CHECK(unit_index >= 0 && unit_index < unit_count, "unit_get(%ld): no such unit", unit_index);
	return &units[unit_index];
}

/* the actors: an actor_get of one not there is the game's crash */
#define MAXIMUM_ACTORS 4
struct actor_datum { struct { short encounter_index; short squad_index; } meta; };
static struct actor_datum actors[MAXIMUM_ACTORS];

static struct actor_datum *actor_get(long actor_index)
{
	CHECK(actor_index >= 0 && actor_index < MAXIMUM_ACTORS,
		"actor_get(%ld): no such actor (the game's datum_get gives NULL, and the caller crashes)", actor_index);
	return &actors[actor_index];
}

/* the spawned actor's variant and definition (not a swarm's) */
struct tag_reference { long index; };
struct actor_variant_definition { struct tag_reference actor_reference, unit_reference; };
struct actor_definition { unsigned long flags; };
static struct actor_variant_definition variant = { { 0 }, { 1 } };
static struct actor_definition definition = { 0 };
static struct actor_variant_definition *actor_variant_definition_get(long index) { return &variant; }
static struct actor_definition *actor_definition_get(long index) { return &definition; }

/* placing a spawned unit */
struct object_placement_data { long definition_index; real_point3d position; real_vector3d forward; };
static boolean network_game_distributed_client(void) { return FALSE; }
static real real_random_range(real lower, real upper) { return lower; }
static void object_placement_data_new(struct object_placement_data *data, long definition_index, long owner)
{
	memset(data, 0, sizeof(*data));
	data->definition_index = definition_index;
}
static void vector3d_from_angle(real_vector3d *vector, real angle) { vector->i = 1.f; vector->j = vector->k = 0.f; }
static void object_get_origin(long unit_index, real_point3d *point) { point->x = point->y = point->z = 0.f; }
static void point_from_line3d(real_point3d const *point, real_vector3d const *vector, real scale, real_point3d *out)
{
	out->x = point->x + vector->i * scale;
	out->y = point->y + vector->j * scale;
	out->z = point->z + vector->k * scale;
}
static void scale_vector3d(real_vector3d const *vector, real scale, real_vector3d *out)
{
	out->i = vector->i * scale;
	out->j = vector->j * scale;
	out->k = vector->k * scale;
}
static long object_new(struct object_placement_data const *data)
{
	CHECK(unit_count < MAXIMUM_UNITS, "too many units");
	memset(&units[unit_count], 0, sizeof(units[unit_count]));
	units[unit_count].object.type = _object_type_biped;
	units[unit_count].unit.actor_index = NONE;
	units[unit_count].unit.swarm_actor_index = NONE;
	return unit_count++;
}
#define biped_fix_position(unit_index, a, b, c, d, e, f, g) ((void)0)
#define biped_accelerate(unit_index, acceleration) ((void)0)
#define actor_customize_unit(variant_index, unit_index) ((void)0)
#define actor_verify_activation(actor_index) ((void)0)
#define object_delete(unit_index) ((void)0)
#define error(priority, ...) ((void)0)

/* the actors made: the encounter and squad each was given */
static int created;
static short created_encounters[MAXIMUM_UNITS], created_squads[MAXIMUM_UNITS];

static long actor_create_for_unit(boolean swarm, long unit_index, long variant_index, short encounter_index,
	short squad_index, boolean a, long b, boolean c, short initial_state, short return_state, long d, long e)
{
	created_encounters[created] = encounter_index;
	created_squads[created] = squad_index;
	return created++;
}

#include "under_test.inc"

/* a world with one unit, the one that dies: its swarm actor, its own actor
and its fake encounter and squad (NONE for none) */
static void reset(long swarm_actor_index, long actor_index, short fake_encounter_index, short fake_squad_index)
{
	memset(units, 0, sizeof(units));
	memset(actors, 0, sizeof(actors));
	created = 0;
	actors[1].meta.encounter_index = 3;
	actors[1].meta.squad_index = 2;
	actors[2].meta.encounter_index = 5;
	actors[2].meta.squad_index = 1;
	unit_count = 1;
	units[0].object.type = _object_type_biped;
	units[0].unit.swarm_actor_index = swarm_actor_index;
	units[0].unit.actor_index = actor_index;
	units[0].unit.fake_encounter_index = fake_encounter_index;
	units[0].unit.fake_squad_index = fake_squad_index;
}

static void check_spawned(short count, short encounter_index, short squad_index)
{
	int index;

	CHECK(created == count, "%d actors made, not %d", created, count);
	for (index = 0; index < created; index++)
	{
		CHECK(created_encounters[index] == encounter_index && created_squads[index] == squad_index,
			"actor %d made in encounter %d squad %d, not %d %d", index, created_encounters[index],
			created_squads[index], encounter_index, squad_index);
	}
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a unit of a swarm (actor 1's), with no actor of its own: its spawn join
	the swarm's encounter and squad */
	CASE("swarm-unit")
	{
		reset(1, NONE, NONE, NONE);
		CHECK(actors_spawn_from_unit(0, 7, 2, 0.f) == 2, "the unit of a swarm spawned no actors");
		check_spawned(2, 3, 2);
		return 0;
	}
	/* a unit driven by its own actor (2), as a carrier form is: as before */
	CASE("own-actor")
	{
		reset(NONE, 2, NONE, NONE);
		CHECK(actors_spawn_from_unit(0, 7, 3, 1.f) == 3, "the unit spawned no actors");
		check_spawned(3, 5, 1);
		return 0;
	}
	/* a unit with no actor: its fake encounter and squad, or none */
	CASE("no-actor")
	{
		reset(NONE, NONE, 4, 6);
		CHECK(actors_spawn_from_unit(0, 7, 1, 0.f) == 1, "the unit spawned no actors");
		check_spawned(1, 4, 6);
		reset(NONE, NONE, NONE, NONE);
		CHECK(actors_spawn_from_unit(0, 7, 1, 0.f) == 0, "a unit of no encounter spawned actors");
		check_spawned(0, NONE, NONE);
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
