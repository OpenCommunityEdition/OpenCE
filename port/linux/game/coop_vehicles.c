/*
COOP_VEHICLES.C

EXTRA VEHICLES: one replacement at the cleared starting place whenever a
player clears the previous vehicle from the home, with a 15 percent buffer. Each authored placement
has its own floor-rounded PER PLAYER budget or STATIC MULTIPLIER total;
copies never register as new
sources. Pelican cargo is anchored only after detaching and coming to rest.

The host alone makes ordinary networked objects. Keep full salted handles,
never datum pointers across object_new (which can compact the object pool).
The small home table follows in-memory campaign checkpoints through the
save/load callbacks, without changing the game's saved allocations or datums.
A disk/core load without a matching in-memory checkpoint forgets the table;
it must not manufacture homes or budgets from coop_vehicle_homes in an unknown state.
*/
#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "physics/collision_models.h"
#include "physics/physics_definitions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "units/vehicles.h"
#include "units/vehicle_datum.h"
#include "units/vehicle_definitions.h"
#include "network_coop.h"
#include "coop_vehicles.h"

char const *config_string(char const *name);
long config_integer(char const *name);
boolean collision_test_sphere(real_point3d const *center, real radius, long ignore_object_index);

/* vehicles.c's tag layout; the public header deliberately leaves it opaque. */
struct coop_vehicle_definition
{
	byte unused[0x2f4];
	short type;
};
typedef char coop_vehicle_type_offset_assert[
	offsetof(struct coop_vehicle_definition, type) == 0x2f4 ? 1 : -1];

enum { VEHICLE_HOMES = 256, VEHICLE_SETTLE_TICKS = 15 };
#define VEHICLE_CLEARANCE 1.15f
#define VEHICLE_SCALING_NUMERATOR 2
#define VEHICLE_SCALING_DENOMINATOR 3
#define VEHICLE_COLLISION_FLAGS (FLAG(_collision_test_structure_bit) | FLAG(_collision_test_objects_bit) | \
	_collision_test_objects_sight_blocking_flags | FLAG(_collision_test_front_facing_surfaces_bit) | \
	FLAG(_collision_test_back_facing_surfaces_bit))

struct coop_vehicle_home
{
	struct scenario_object_datum const *source;
	long current;
	long definition;
	short copies;
	short settled;
	short bsp;
	boolean anchored;
	boolean entered;
	struct object_placement_data placement;
	real_point3d center;
	real radius;
};

static struct
{
	short count;
	struct coop_vehicle_home homes[VEHICLE_HOMES];
} coop_vehicle_homes, saved_vehicles;
static boolean vehicles_enabled, saved_valid;
static short vehicles_percent, vehicles_multiplier;
static long saved_tick;

static boolean vehicles_host(void)
{
	return vehicles_enabled && game_connection() == _game_connection_network_server && network_coop_active();
}

/* Apply two-thirds extra growth in either mode, then round down. */
static short vehicles_extra_count(short players, short percent, short multiplier)
{
	return multiplier > 0 ? (short)((PIN(multiplier, 2, 32) - 1) * VEHICLE_SCALING_NUMERATOR / VEHICLE_SCALING_DENOMINATOR) :
		(short)(MAX(0, MIN(players, HALO_PORT_MAXIMUM_NETWORK_PLAYERS) - 1) *
		(long)PIN(percent, 25, 200) * VEHICLE_SCALING_NUMERATOR / (100 * VEHICLE_SCALING_DENOMINATOR));
}

static short vehicles_player_count(void)
{
	struct network_game *game = network_game_get_game();
	short index, count = 0;

	for (index = 0; game && index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
		if (network_player_is_valid(&game->players[index]))
			count++;
	return count;
}

static boolean vehicles_player_driving(long index, struct vehicle_datum const *vehicle)
{
	struct unit_datum *driver = unit_try_and_get(vehicle->unit.driver_object_index);
	struct player_datum *player;

	if (!driver || driver->object.parent_object_index != index || driver->unit.player_index == NONE)
		return FALSE;
	player = player_try_and_get(driver->unit.player_index);
	return player && player->unit_index == vehicle->unit.driver_object_index;
}

/* Capture an unloaded, upright vehicle's resting pose, rather than a
dropship's airborne authored placement. A ray below it must find ground. */
static boolean vehicles_anchor(struct coop_vehicle_home *home, struct vehicle_datum const *vehicle)
{
	real_point3d above = vehicle->object.position;
	real_vector3d down = {0.0f, 0.0f, -1.5f};
	real_vector3d const *speed = &vehicle->object.translational_velocity;
	struct collision_result hit;

	if (vehicle->vehicle.airborne_ticks || vehicle->object.up.k < 0.5f || speed->i*speed->i + speed->j*speed->j + speed->k*speed->k > 0.0004f ||
		TEST_FLAG(vehicle->unit.flags, _unit_possessed_by_recording_bit))
		return FALSE;
	above.z += 0.2f;
	if (!collision_test_vector(VEHICLE_COLLISION_FLAGS, &above, &down, home->current, &hit) || hit.plane.n.k < 0.65f)
		return FALSE;
	object_placement_data_new(&home->placement, vehicle->definition_index, NONE);
	home->placement.position = vehicle->object.position;
	home->placement.forward = vehicle->object.forward;
	home->placement.up = vehicle->object.up;
	home->placement.variant_number = vehicle->object.variant_number;
	csmemcpy(home->placement.change_colors, vehicle->object.base_change_colors, sizeof(home->placement.change_colors));
	home->center = vehicle->object.bounding_sphere_center;
	home->radius = vehicle->object.bounding_sphere_radius;
	home->bsp = global_structure_bsp_index;
	return TRUE;
}

/* Use the physics mass points, not the vehicle's large bounding sphere
(which normally intersects its own floor). Check their upper halves with
a small lift above contact, and check nearby objects against their collision meshes.
An obstructed pad is retried later, never replaced by a guessed nearby spot. */
static boolean vehicles_home_clear(struct coop_vehicle_home const *home)
{
	struct unit_definition *definition = vehicle_definition_get(home->definition);
	struct physics_definition *physics;
	struct location location;
	real_vector3d left;
	long indices[64];
	short index, count;

	if (home->bsp != global_structure_bsp_index || definition->object.physics.index == NONE)
		return FALSE;
	scenario_location_from_point(&location, &home->center);
	if (location.leaf_index == NONE || location.cluster_index == NONE)
		return FALSE;
	count = objects_in_sphere(0, _object_mask_all & ~_object_mask_projectile, &location,
		&home->center, home->radius + 0.1f, indices, NUMBEROF(indices));
	if (count == NUMBEROF(indices))
		return FALSE;
	physics = physics_definition_get(definition->object.physics.index);
	if (physics->mass_points.count < 1 || physics->mass_points.count > MAXIMUM_MASS_POINTS_PER_PHYSICS)
		return FALSE;
	cross_product3d(&home->placement.up, &home->placement.forward, &left);
	for (index = 0; index < physics->mass_points.count; index++)
	{
		struct mass_point_definition const *point = TAG_BLOCK_GET_ELEMENT(&physics->mass_points, index, struct mass_point_definition);
		real_point3d center = home->placement.position;
		real radius = MAX(0.02f, point->radius * 0.5f);

		center.x += home->placement.forward.i * point->position.x + left.i * point->position.y + home->placement.up.i * point->position.z;
		center.y += home->placement.forward.j * point->position.x + left.j * point->position.y + home->placement.up.j * point->position.z;
		center.z += home->placement.forward.k * point->position.x + left.k * point->position.y + home->placement.up.k * point->position.z + 0.1f;
		if (collision_test_sphere(&center, radius, home->current))
			return FALSE;
		{
			short other;

			for (other = 0; other < count; other++)
			{
				struct object_datum *object = object_get(indices[other]);
				struct collision_model_instance instance;
				real dx, dy, dz, clearance;

				if (object->object.parent_object_index != NONE)
					continue;
				if (collision_model_instance_new(&instance, indices[other]))
				{
					if (!collision_model_test_sphere(&instance, &center, radius))
						continue;
				}
				else
				{
					dx = object->object.bounding_sphere_center.x - center.x;
					dy = object->object.bounding_sphere_center.y - center.y;
					dz = object->object.bounding_sphere_center.z - center.z;
					clearance = radius + object->object.bounding_sphere_radius;
					if (dx*dx + dy*dy + dz*dz >= clearance*clearance)
						continue;
				}
				return FALSE;
			}
		}
	}
	return TRUE;
}

void coop_vehicles_new_game(void)
{
	char const *mode = config_string("network.coop_vehicles");

	csmemset(&coop_vehicle_homes, 0, sizeof(coop_vehicle_homes));
	saved_valid = FALSE;
	vehicles_enabled = !strcmp(mode, "per_player") || !strcmp(mode, "multiplier");
	vehicles_percent = (short)PIN(config_integer("network.coop_enemies"), 25, 200);
	vehicles_multiplier = !strcmp(mode, "multiplier") ?
		(short)PIN(config_integer("network.coop_enemies_multiplier"), 2, 32) : 0;
}

void coop_vehicles_register(long index, struct scenario_object_datum const *source)
{
	struct vehicle_datum *vehicle;
	struct coop_vehicle_definition const *definition;
	struct coop_vehicle_home *home;
	short number;

	if (!vehicles_host())
		return;
	vehicle = vehicle_datum_try_and_get(index);
	if (!vehicle)
		return;
	definition = (struct coop_vehicle_definition const *)vehicle_definition_get(vehicle->definition_index);
	/* Tanks, Warthogs, Ghosts and Banshees; no dropships or fixed turrets. */
	if (definition->type != 0 && definition->type != 1 && definition->type != 4 && definition->type != 5)
		return;
	for (number = 0; number < coop_vehicle_homes.count; number++)
		if (coop_vehicle_homes.homes[number].source == source)
			break;
	if (number == coop_vehicle_homes.count)
	{
		if (coop_vehicle_homes.count == VEHICLE_HOMES)
			return;
		coop_vehicle_homes.count++;
	}
	home = &coop_vehicle_homes.homes[number];
	/* Scripted recreation cannot reset the used budget or displace a copy. */
	if (home->source && vehicle_try_and_get(home->current))
		return;
	home->source = source;
	home->definition = vehicle->definition_index;
	home->current = index;
	home->settled = 0;
	home->anchored = home->entered = FALSE;
}

void coop_vehicles_update(void)
{
	short number, budget;

	if (!vehicles_host())
		return;
	budget = vehicles_extra_count(vehicles_player_count(), vehicles_percent, vehicles_multiplier);
	for (number = 0; number < coop_vehicle_homes.count; number++)
	{
		struct coop_vehicle_home *home = &coop_vehicle_homes.homes[number];
		struct vehicle_datum *vehicle = vehicle_datum_try_and_get(home->current);
		real dx, dy, dz, clearance;
		long copy;
		struct object_placement_data placement;

		if (!vehicle || vehicle->definition_index != home->definition ||
			TEST_FLAG(vehicle->object.damage_flags, _object_dead_bit))
			continue;
		if (vehicle->object.parent_object_index != NONE)
		{
			home->anchored = home->entered = FALSE;
			home->settled = 0;
			continue;
		}
		if (!TEST_FLAG(vehicle->object.flags, _object_connected_to_map_bit) ||
			TEST_FLAG(vehicle->object.flags, _object_outside_of_map_bit))
			continue;
		if (!home->anchored)
		{
			if (!vehicles_anchor(home, vehicle))
				home->settled = 0;
			else if (++home->settled >= VEHICLE_SETTLE_TICKS)
				home->anchored = TRUE;
			continue;
		}
		if (vehicles_player_driving(home->current, vehicle))
			home->entered = TRUE;
		if (!home->entered || home->copies >= budget || game_time_get() % 6)
			continue;
		/* Separate both bounding spheres, including a buffer, before
		checking the pad against the collision meshes. */
		clearance = (home->radius + vehicle->object.bounding_sphere_radius) * VEHICLE_CLEARANCE;
		dx = vehicle->object.bounding_sphere_center.x - home->center.x;
		dy = vehicle->object.bounding_sphere_center.y - home->center.y;
		dz = vehicle->object.bounding_sphere_center.z - home->center.z;
		if (dx*dx + dy*dy + dz*dz < clearance*clearance ||
			object_header_data->actual_count >= MAXIMUM_OBJECTS_PER_MAP - 512 || !vehicles_home_clear(home))
			continue;
		placement = home->placement;
		/* vehicle_new lifts physics objects by half their tag radius. */
		placement.position.z -= vehicle_definition_get(home->definition)->object.bounding_radius * 0.5f;
		copy = object_new(&placement);
		if (copy == NONE)
			continue;
		home->current = copy;
		home->copies++;
		home->entered = FALSE;
		/* Only one allocation per tick, outside object/physics iteration. */
		break;
	}
}

void coop_vehicles_before_save(void)
{
	if (vehicles_host())
	{
		saved_vehicles = coop_vehicle_homes;
		saved_tick = game_time_get();
		saved_valid = TRUE;
	}
}

void coop_vehicles_after_load(void)
{
	if (!vehicles_host())
		return;
	if (saved_valid && saved_tick == game_time_get())
		coop_vehicle_homes = saved_vehicles;
	else
		csmemset(&coop_vehicle_homes, 0, sizeof(coop_vehicle_homes));
}
