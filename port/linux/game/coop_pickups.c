/*
COOP_PICKUPS.C

Server Setup's EXTRA PICKUPS (network.coop_pickups): weapons, grenades,
health kits, overshields and camouflage placed by the campaign, grown by PER PLAYER
(network.coop_enemies) or STATIC MULTIPLIER (network.coop_enemies_multiplier),
to at most eight times their number. Fractions
are distributed over placements of the same definition, rather than
rounding every loose grenade up. Enemies' drops and inventory are not
scenario placements and never enter here.

Only the host creates copies. A source's remaining count is kept in its
item flags, alongside a placement identity and a copy marker, in the existing
checkpoint image: no allocation or datum size changes. A revert restores
them together; scripted recreation counts surviving copies against the budget.
Copies are made once the source is connected to the current BSP, with
ground, a clear bounding sphere and no wall between them and the source.
No safe position means no copy; the level's originals are never moved.
*/

#include "cseries.h"
#include "game/game.h"
#include "items/equipment.h"
#include "items/equipment_definitions.h"
#include "items/items.h"
#include "items/weapons.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "objects/objects.h"
#include "physics/collision_models.h"
#include "physics/collisions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "coop_pickups.h"
#include "coop_scaling.h"
#include "network_coop.h"

#include <math.h>

char const *config_string(char const *name);
long config_integer(char const *name);
/* collisions.c's: TRUE outside the BSP or when the sphere touches it */
boolean collision_test_sphere(real_point3d const *center, real radius, long ignore_object_index);

enum
{
	/* items.h uses bits 0..6. These three bits store 0..7 extras and are
	cleared before creating them; ordinary copies keep zero here. */
	PICKUPS_COUNT_SHIFT = 7,
	PICKUPS_COUNT_MASK = 7 << PICKUPS_COUNT_SHIFT,
	/* Saved provenance for scripted recreation: bit 10 marks copies,
	bits 11..25 identify the scenario placement (1..SHORT_MAX). */
	PICKUPS_COPY_FLAG = 1 << 10,
	PICKUPS_ID_SHIFT = 11,
	PICKUPS_ID_MASK = 0x7fff << PICKUPS_ID_SHIFT,
	PICKUPS_SOURCES_PER_TICK = 8,
	PICKUPS_RINGS = 5,
	PICKUPS_PLACES_PER_RING = 12,
};

#define PICKUPS_COLLISION_FLAGS (FLAG(_collision_test_structure_bit) | FLAG(_collision_test_objects_bit) | \
	_collision_test_objects_sight_blocking_flags | FLAG(_collision_test_front_facing_surfaces_bit) | \
	FLAG(_collision_test_back_facing_surfaces_bit))

static boolean pickups_enabled;
static short pickups_percent;
static short pickups_multiplier;

static boolean pickups_host(void)
{
	return pickups_enabled && game_connection() == _game_connection_network_server && network_coop_active();
}

/* Incremental rounding gives a group of N placements exactly
round(N * growth / 100) extras, up to 7*N. */
static short pickups_extra_count(long ordinal, short players, short percent, short multiplier)
{
	long growth;

	if (ordinal < 1 || ordinal > SHORT_MAX)
		return 0;
	growth = multiplier > 0 ? (MIN(multiplier, COOP_SCALING_MAXIMUM_GROWTH) - 1) * 100 :
		coop_scaling_growth(players, percent);
	return (short)((ordinal * growth + 50) / 100 - ((ordinal - 1) * growth + 50) / 100);
}

static short pickups_player_count(void)
{
	struct network_game *game = network_game_get_game();
	short index, count = 0;

	for (index = 0; game && index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
		if (network_player_is_valid(&game->players[index]))
			count++;
	return count;
}

static boolean pickups_definition_allowed(long definition_index)
{
	struct object_definition *definition = object_definition_get(definition_index);

	if (definition->object.type == _object_type_weapon)
		return TRUE;
	if (definition->object.type == _object_type_equipment)
	{
		short type = equipment_definition_get(definition_index)->equipment.powerup_type;

		return type == _equipment_powerup_grenade || type == _equipment_powerup_overshield ||
			type == _equipment_powerup_active_camouflage || type == _equipment_powerup_health;
	}
	return FALSE;
}

/* Placement identity is the address in the scenario block, not a position
that two items might share. Only earlier placements of this same tag
contribute to fractional rounding. */
static long pickups_ordinal(struct item_datum const *item, struct scenario_object_datum const *source,
	struct tag_block *palette, long *identity)
{
	long element_size, index, ordinal = 0;
	struct tag_block *placements = scenario_get_object_type_scenario_datums(global_scenario_get(),
		item->object.type, &element_size);

	for (index = 0; index < MIN(placements->count, SHORT_MAX); index++)
	{
		struct scenario_object_datum *entry = tag_block_get_element_with_size(placements, index, element_size);

		if (VALID_INDEX(entry->palette_entry_index, palette->count) &&
			TAG_BLOCK_GET_ELEMENT(palette, entry->palette_entry_index, struct scenario_object_palette_entry)->reference.index ==
			item->definition_index)
			ordinal++;
		if (entry == source)
		{
			*identity = index + 1;
			return ordinal;
		}
	}
	return 0;
}

/* A script may recreate a named supply. Surviving copies (even in an
inventory) count against that placement's budget, rather than producing
another complete set. Identity and provenance travel in saved item flags. */
static short pickups_existing_copies(long definition_index, unsigned long identity)
{
	struct object_iterator iterator;
	struct item_datum *item;
	short count = 0;

	object_iterator_new(&iterator, _object_mask_weapon | _object_mask_equipment, 0);
	while ((item = object_iterator_next(&iterator)) != NULL)
		if (item->definition_index == definition_index && (item->item.flags & PICKUPS_COPY_FLAG) &&
			(item->item.flags & PICKUPS_ID_MASK) == identity && ++count >= COOP_SCALING_MAXIMUM_GROWTH - 1)
			break;
	return count;
}

/* Bounding spheres keep pickups apart. For large scenery bounds, test
the collision mesh as well so a room-sized crate group does not exclude
all otherwise free floor. */
static boolean pickups_occupied(real_point3d const *center, real radius, long source_index)
{
	struct location location;
	long indices[64];
	short count, index;

	scenario_location_from_point(&location, center);
	if (location.leaf_index == NONE || location.cluster_index == NONE)
		return TRUE;
	count = objects_in_sphere(0, _object_mask_all & ~_object_mask_projectile, &location, center,
		radius + 0.04f, indices, NUMBEROF(indices));
	if (count == NUMBEROF(indices))
		return TRUE;
	for (index = 0; index < count; index++)
	{
		struct object_datum *object = object_get(indices[index]);
		real dx = object->object.bounding_sphere_center.x - center->x;
		real dy = object->object.bounding_sphere_center.y - center->y;
		real dz = object->object.bounding_sphere_center.z - center->z;
		real clearance = radius + object->object.bounding_sphere_radius + 0.04f;

		if (object->object.parent_object_index != NONE || dx*dx + dy*dy + dz*dz >= clearance*clearance)
			continue;
		if (indices[index] != source_index && TEST_FLAG(_object_mask_sightblocking | _object_mask_device,
			object->object.type))
		{
			struct collision_model_instance instance;

			if (collision_model_instance_new(&instance, indices[index]) &&
				!collision_model_test_sphere(&instance, center, radius + 0.04f))
				continue;
		}
		return TRUE;
	}
	return FALSE;
}

static boolean pickups_position(long source_index, real_point3d const *origin, real radius,
	long attempt, real_point3d *center)
{
	short ring = (short)(attempt / PICKUPS_PLACES_PER_RING);
	short place = (short)(attempt % PICKUPS_PLACES_PER_RING);
	real angle = ((real)place + 0.5f * (ring % 2)) * (_pi * 2.0f / PICKUPS_PLACES_PER_RING);
	real distance = (2.0f * radius + 0.15f) * (ring + 1);
	real_point3d from = *origin;
	real_point3d above;
	real_vector3d down = {0.0f, 0.0f, -2.0f};
	real_vector3d way;
	struct collision_result collision;

	from.z += 0.75f;
	above = from;
	above.x += cosf(angle) * distance;
	above.y += sinf(angle) * distance;
	vector_from_points3d(&from, &above, &way);
	if (collision_test_vector(PICKUPS_COLLISION_FLAGS, &from, &way, source_index, &collision) ||
		!collision_test_vector(PICKUPS_COLLISION_FLAGS, &above, &down, source_index, &collision) ||
		collision.plane.n.k < 0.7f)
		return FALSE;
	*center = collision.point;
	center->z += radius + 0.04f;
	if (fabsf(center->z - origin->z) > 0.75f || collision_test_sphere(center, radius + 0.02f, source_index) ||
		pickups_occupied(center, radius, source_index))
		return FALSE;
	vector_from_points3d(origin, center, &way);
	return !collision_test_vector(PICKUPS_COLLISION_FLAGS, origin, &way, source_index, &collision);
}

static void pickups_place(long source_index)
{
	struct item_datum *source = item_get(source_index);
	struct object_placement_data placement;
	struct weapon_magazine magazines[2];
	real_point3d origin = source->object.bounding_sphere_center;
	real_vector3d offset;
	real radius = source->object.bounding_sphere_radius;
	short remaining = (short)((source->item.flags & PICKUPS_COUNT_MASK) >> PICKUPS_COUNT_SHIFT);
	short type = source->object.type;
	unsigned long identity = source->item.flags & PICKUPS_ID_MASK;
	long item_flags = source->item.flags & (FLAG(_item_does_not_accelerate_bit) | FLAG(_equipment_orient_to_ground_bit));
	long object_flags = source->object.flags & (FLAG(_object_cannot_be_garbage_bit) | FLAG(_object_shadowless_bit));
	long attempt;

	/* Clear first, including invalid/no-space cases. This saved source can
	never amplify itself again; copies have no pending count. */
	source->item.flags &= ~PICKUPS_COUNT_MASK;
	remaining = (short)MAX(0, remaining - pickups_existing_copies(source->definition_index, identity));
	if (!remaining || !(radius > 0.0f && radius < 2.0f) || source->object.parent_object_index != NONE)
		return;
	vector_from_points3d(&source->object.position, &origin, &offset);
	object_placement_data_new(&placement, source->definition_index, NONE);
	placement.forward = source->object.forward;
	placement.up = source->object.up;
	placement.variant_number = source->object.forced_shader_permutation_index;
	csmemcpy(placement.change_colors, source->object.base_change_colors, sizeof(placement.change_colors));
	if (type == _object_type_weapon)
		csmemcpy(magazines, weapon_get(source_index)->weapon.magazines, sizeof(magazines));
	for (attempt = 0; remaining > 0 && attempt < PICKUPS_RINGS * PICKUPS_PLACES_PER_RING; attempt++)
	{
		real_point3d center;
		long copy_index;
		struct item_datum *copy;

		/* Leave room for the campaign's actors and scripted objects. */
		if (object_header_data->actual_count >= MAXIMUM_OBJECTS_PER_MAP - 512)
			break;
		if (!pickups_position(source_index, &origin, radius, attempt, &center))
			continue;
		placement.position.x = center.x - offset.i;
		placement.position.y = center.y - offset.j;
		placement.position.z = center.z - offset.k;
		copy_index = object_new(&placement);
		if (copy_index == NONE)
			break;
		copy = item_get(copy_index);
		copy->item.flags |= item_flags | identity | PICKUPS_COPY_FLAG;
		copy->object.flags |= object_flags;
		if (type == _object_type_weapon)
			csmemcpy(weapon_get(copy_index)->weapon.magazines, magazines, sizeof(magazines));
		remaining--;
	}
}

void coop_pickups_new_game(void)
{
	char const *mode = config_string("network.coop_pickups");

	pickups_enabled = !strcmp(mode, "per_player") || !strcmp(mode, "multiplier");
	pickups_percent = (short)PIN(config_integer("network.coop_enemies"), 25, 200);
	pickups_multiplier = !strcmp(mode, "multiplier") ?
		(short)PIN(config_integer("network.coop_enemies_multiplier"), 2, 32) : 0;
}

void coop_pickups_register(long object_index, struct scenario_object_datum const *source, struct tag_block *palette)
{
	struct item_datum *item;
	short extra;
	long identity = 0;

	if (!pickups_host())
		return;
	item = item_try_and_get(object_index);
	if (!item || !pickups_definition_allowed(item->definition_index))
		return;
	extra = pickups_extra_count(pickups_ordinal(item, source, palette, &identity), pickups_player_count(),
		pickups_percent, pickups_multiplier);
	item->item.flags = (item->item.flags & ~(PICKUPS_COUNT_MASK | PICKUPS_ID_MASK | PICKUPS_COPY_FLAG)) |
		((unsigned long)extra << PICKUPS_COUNT_SHIFT) | ((unsigned long)identity << PICKUPS_ID_SHIFT);
}

/* After the host's tick, outside object iteration/physics: object_new may
compact the object pool. Gather indices before allocating any copies. */
void coop_pickups_update(void)
{
	struct object_iterator iterator;
	struct item_datum *item;
	long indices[PICKUPS_SOURCES_PER_TICK];
	short count = 0, index;

	if (!pickups_host())
		return;
	object_iterator_new(&iterator, _object_mask_weapon | _object_mask_equipment, 0);
	while ((item = object_iterator_next(&iterator)) != NULL && count < NUMBEROF(indices))
		if ((item->item.flags & PICKUPS_COUNT_MASK) && TEST_FLAG(item->object.flags, _object_connected_to_map_bit) &&
			!TEST_FLAG(item->object.flags, _object_outside_of_map_bit))
			indices[count++] = iterator.index;
	for (index = 0; index < count; index++)
		pickups_place(indices[index]);
}
