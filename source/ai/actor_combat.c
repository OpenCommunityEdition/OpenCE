/*
ACTOR_COMBAT.C

symbols in this file:
0000F590 0090:
	_code_0000f590 (0000)
0000F620 0030:
	_actor_combat_fire_wildly (0000)
0000F650 0040:
	_actor_combat_disable_bursts (0000)
0000F690 0030:
	_actor_firing_blindly (0000)
0000F6C0 0040:
	_actor_combat_currently_firing_burst (0000)
0000F700 0040:
	_actor_get_weapon_definition (0000)
0000F740 00c0:
	_actor_get_weapon_vector (0000)
0000F800 0080:
	_actor_combat_get_firing_variant_definition (0000)
0000F880 00a0:
	_code_0000f880 (0000)
0000F920 0040:
	_code_0000f920 (0000)
0000F960 0020:
	_fast_ftol (0000)
0000F980 0010:
	_sine (0000)
0000F990 0010:
	_cosine (0000)
0000F9A0 0010:
	_tangent (0000)
0000F9B0 0020:
	_cross_product2d (0000)
0000F9D0 0030:
	_add_vectors3d (0000)
0000FA00 0020:
	_random_boolean (0000)
0000FA20 0020:
	_valid_real (0000)
0000FA40 0160:
	_actor_combat_find_nearby_target (0000)
0000FBA0 00b0:
	_code_0000fba0 (0000)
0000FC50 00d0:
	_code_0000fc50 (0000)
0000FD20 01c0:
	_actor_combat_reaim_grenade (0000)
0000FEE0 0110:
	_actor_combat_build_grenade_trajectory (0000)
0000FFF0 0100:
	_actor_combat_find_grenade_target (0000)
000100F0 0370:
	_actor_combat_check_collateral_damage (0000)
00010460 0120:
	_actor_combat_plan_grenade_trajectory (0000)
00010580 0040:
	_valid_realcmp (0000)
000105C0 0060:
	_valid_real_normal3d (0000)
00010620 00b0:
	_code_00010620 (0000)
000106D0 02d0:
	_actor_aim_projectile (0000)
000109A0 07b0:
	_code_000109a0 (0000)
00011150 0060:
	_actor_combat_retarget_grenade (0000)
000111B0 0220:
	_actor_aim_grenade (0000)
000113D0 1240:
	_actor_combat_update (0000)
002444A0 000e:
	??_C@_0O@HEGILGAC@weapon_vector?$AA@ (0000)
002444B0 0021:
	??_C@_0CB@NKOAMEKE@c?3?2halo?2SOURCE?2ai?2actor_combat?4c@ (0000)
002444D4 0035:
	??_C@_0DF@COLBENAC@burst_geometry_reference?5?$CG?$CG?5firi@ (0000)
0024450C 0004:
	__real@3dcccccd (0000)
00244510 0004:
	__real@3fd9999a (0000)
00244514 0016:
	??_C@_0BG@ODFAEBCF@projectile_definition?$AA@ (0000)
00244530 0042:
	??_C@_0EC@CDIDOCJH@grenade_origin?5?$CG?$CG?5desired_impact@ (0000)
00244574 0004:
	__real@3e4ccccd (0000)
00244578 0008:
	__real@3f50624de0000000 (0000)
00244580 0043:
	??_C@_0ED@MKICAFGB@actor?9?$DOcontrol?4current_fire_targ@ (0000)
002445C4 002b:
	??_C@_0CL@GJIEKICC@?$CFs?3?5assert_valid_real_normal3d?$CI?$CF@ (0000)
002445F0 0007:
	??_C@_06NBFPPGEG@vector?$AA@ (0000)
002445F8 0004:
	__real@3f490fdb (0000)
002445FC 003b:
	??_C@_0DL@NNPMKEAI@?$CFs?3?5proj?5?$CF?41f?5rof?5?$CF?41f?5dmg?1s?5?$CF?41@ (0000)
00244638 0020:
	??_C@_0CA@FHOKFHD@?$CFs?3?5manual?5damage?5modifier?5?$CF?42f?$AA@ (0000)
00244658 0010:
	??_C@_0BA@FHPPJCFM@?$CGnew_aim_vector?$AA@ (0000)
00244668 0021:
	??_C@_0CB@CPANGHMG@?$CGactor?9?$DOcontrol?4burst_aim_vector@ (0000)
0024468C 000f:
	??_C@_0P@DBEDAMI@aiming_success?$AA@ (0000)
0024469C 0012:
	??_C@_0BC@CHLPMLFC@?$CBhold_burst_start?$AA@ (0000)
002446B0 0025:
	??_C@_0CF@EHKIDLPJ@new_state?$CB?$DNactor?9?$DOcontrol?4fire_s@ (0000)
002446D8 0018:
	??_C@_0BI@MMOBPCCM@firing_decision?5?$DN?$DN?5NONE?$AA@ (0000)
002446F0 0004:
	__real@3f3504f3 (0000)
002446F4 0004:
	__real@3e3377ed (0000)
002446F8 004b:
	??_C@_0EL@FHBLJBEG@actor?9?$DOcontrol?4current_fire_targ@ (0000)
00244744 0015:
	??_C@_0BF@GLLAMKHP@weapon_index?5?$CB?$DN?5NONE?$AA@ (0000)
0024475C 0036:
	??_C@_0DG@EHGBJHNC@actor?9?$DOcontrol?4current_fire_targ@ (0000)
*/

/* ---------- headers */

#define cosine cosine_inline
#define sine sine_inline
#define tangent tangent_inline
#define cross_product2d cross_product2d_inline
#define normalize2d normalize2d_inline
#define normalize3d normalize3d_inline
#define add_vectors3d add_vectors3d_inline
#define point_from_line3d actor_combat_point_from_line3d_inline
#define vector_from_points3d actor_combat_vector_from_points3d_inline
#define magnitude_squared3d actor_combat_magnitude_squared3d_inline
#define distance_squared3d actor_combat_distance_squared3d_inline
#define real_random_range actor_combat_real_random_range_inline
#define collision_test_line actor_combat_collision_test_line_inline
#include "cseries.h"

#include "ai.h"
#include "ai_debug.h"
#include "actor_definitions.h"
#include "actors.h"
#include "encounters.h"
#include "props.h"
#include "items/projectile_definitions.h"
#include "items/projectiles.h"
#include "items/weapon_definitions.h"
#include "items/weapons.h"
#include "game/game_globals.h"
#include "game/game.h"
#include "physics/collisions.h"
#include "scenario/scenario.h"
#include "units/vehicle_definitions.h"
#include "units/vehicles.h"
#undef collision_test_line
#undef real_random_range
#undef distance_squared3d
#undef magnitude_squared3d
#undef vector_from_points3d
#undef point_from_line3d
#undef add_vectors3d
#undef normalize3d
#undef normalize2d
#undef cross_product2d
#undef cosine
#undef sine
#undef tangent

/* ---------- constants */

enum
{
	_actor_fire_target_none = 0,
	_actor_fire_target_prop,

	_vehicle_ai_weapon_cannot_rotate_bit = 8,

	MAXIMUM_COLLATERAL_DAMAGE_ACTORS = 32,
};

/* ---------- macros */

#define GRENADE_AIMING_ANGLE_COSINE 0.8660254f
#define GRENADE_AIMING_ANGLE_SINE 0.5f
#define actor_combat_vehicle_definition_get(index) \
	((struct actor_combat_vehicle_definition_view *)vehicle_specific_definition_get(index))

/* ---------- structures */

struct encounter_actor_iterator
{
	long encounter_index;
	long index;
	long next_index;
};

struct actor_combat_vehicle_definition_view
{
	byte __unknown0[0x2F0];
	unsigned long flags;
};

typedef char actor_combat_vehicle_definition_flags_offset_assert[
	offsetof(struct actor_combat_vehicle_definition_view, flags) == 0x2F0 ? 1 : -1];

/* ---------- prototypes */

static void actor_combat_find_nearby_target(
	real_point3d *target_point,
	real miss_distance);
static void actor_get_weapon_vector(
	long actor_index,
	real_vector3d *weapon_vector);
/* The owner declarations in actions.h are macro-renamed while importing the
 * January inline set; restore the external names after that schedule ends. */
real normalize2d(
	real_vector2d *vector);
real normalize3d(
	real_vector3d *vector);
static boolean actor_combat_build_grenade_trajectory(
	short grenade_type_index,
	real_point3d const *grenade_origin,
	real velocity_max,
	real_point3d const *desired_impact_point,
	real *target_ballistic_fraction_min,
	boolean lob,
	real_vector3d *aim_vector,
	real *aim_speed,
	real *aim_ticks,
	real_vector3d *aim_velocity,
	real *aim_gravity);
static boolean actor_combat_reaim_grenade(
	long actor_index,
	real_point3d const *grenade_origin);
static boolean actor_combat_retarget_grenade(
	long actor_index,
	real_point3d const *desired_grenade_target);

/* ---------- globals */

/* ---------- public code */

void actor_combat_fire_wildly(
	long actor_index,
	short fire_ticks)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor->control.fire_state = _actor_fire_state_wild;
	actor->control.fire_state_timer = fire_ticks;

	return;
}

void actor_combat_disable_bursts(
	long actor_index,
	long disable_timer)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor->control.burst_disable_timer =
		MAX(actor->control.burst_disable_timer, disable_timer);

	return;
}

boolean actor_firing_blindly(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	return actor->control.fire_state == _actor_fire_state_wild;
}

boolean actor_combat_currently_firing_burst(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = FALSE;

	if (actor->control.current_fire_target_type > _actor_fire_target_none)
	{
		result = actor->control.fire_state == _actor_fire_state_bursting;
	}

	return result;
}

struct weapon_definition *actor_get_weapon_definition(
	long actor_index)
{
	struct weapon_definition *result = NULL;
	long weapon_index = actor_get_weapon(actor_index);

	if (weapon_index != NONE)
	{
		struct weapon_datum *weapon = weapon_get(weapon_index);
		result = weapon_definition_get(weapon->definition_index);
	}

	return result;
}

static void actor_get_weapon_vector(
	long actor_index,
	real_vector3d *weapon_vector)
{
	struct actor_datum *actor = actor_get(actor_index);
	long unit_index = actor->meta.unit_index;
	boolean have_vector = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\ai\\actor_combat.c",
		1166,
		weapon_vector);

	if (actor->input.vehicle_gunner)
	{
		struct unit_datum *vehicle = vehicle_get(actor->input.vehicle_index);
		struct actor_combat_vehicle_definition_view *vehicle_definition =
			actor_combat_vehicle_definition_get(vehicle->definition_index);

		unit_index = actor->input.vehicle_index;
		if (TEST_FLAG(
			vehicle_definition->flags,
			_vehicle_ai_weapon_cannot_rotate_bit))
		{
			*weapon_vector = vehicle->object.forward;
			have_vector = TRUE;
		}
	}

	if (!have_vector)
	{
		unit_get(unit_index);
		unit_get_aiming_vector(unit_index, weapon_vector);
		unit_clip_to_aiming_bounds(unit_index, weapon_vector, TRUE);
	}

	return;
}

struct actor_variant_definition *actor_combat_get_firing_variant_definition(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_variant_definition *firing_variant_definition =
		actor_variant_definition_get(actor->meta.variant_definition_index);
	long weapon_index = actor_get_weapon(actor_index);

	if (weapon_index != NONE)
	{
		struct weapon_datum *weapon = weapon_get(weapon_index);
		struct weapon_definition *weapon_definition =
			weapon_definition_get(weapon->definition_index);

		if (weapon_definition != NULL &&
			weapon_definition->weapon.ai_firing_parameters.index != NONE)
		{
			firing_variant_definition = actor_variant_definition_get(
				weapon_definition->weapon.ai_firing_parameters.index);
		}
	}

	return firing_variant_definition;
}

real sine(
	real angle)
{
	return sin(angle);
}

real cosine(
	real angle)
{
	return cos(angle);
}

real tangent(
	real angle)
{
	return tan(angle);
}

real cross_product2d(
	real_vector2d const *a,
	real_vector2d const *b)
{
	return a->i*b->j - a->j*b->i;
}

real_vector3d *add_vectors3d(
	real_vector3d const *a,
	real_vector3d const *b,
	real_vector3d *result)
{
	result->i = a->i+b->i;
	result->j = a->j+b->j;
	result->k = a->k+b->k;

	return result;
}

static void actor_combat_find_nearby_target(
	real_point3d *target_point,
	real miss_distance)
{
	real_point3d above;
	real_point3d target;
	real_vector3d direction;
	struct collision_result collision;

	/* Keep January's inlined real-math schedule without materializing the
	 * header helpers as actor_combat-owned COMDATs. */
	above.x = global_up3d->i*1.5f + target_point->x;
	above.y = global_up3d->j*1.5f + target_point->y;
	above.z = global_up3d->k*1.5f + target_point->z;
	vector3d_from_angle(
		&direction,
		real_seed_random_range(get_global_random_seed_address(), -_pi, _pi));
	target.x = direction.i*miss_distance + above.x;
	target.y = direction.j*miss_distance + above.y;
	target.z = direction.k*miss_distance + above.z;

	{
		real_vector3d collision_vector;

		collision_vector.i = above.x - target_point->x;
		collision_vector.j = above.y - target_point->y;
		collision_vector.k = above.z - target_point->z;
		if (collision_test_vector(
			FLAG(_collision_test_front_facing_surfaces_bit) |
				FLAG(_collision_test_back_facing_surfaces_bit) |
				FLAG(_collision_test_structure_bit),
			target_point,
			&collision_vector,
			NONE,
			&collision))
		{
			above = *target_point;
		}
	}

	{
		real_vector3d collision_vector;

		collision_vector.i = target.x - above.x;
		collision_vector.j = target.y - above.y;
		collision_vector.k = target.z - above.z;
		if (collision_test_vector(
			FLAG(_collision_test_front_facing_surfaces_bit) |
				FLAG(_collision_test_back_facing_surfaces_bit) |
				FLAG(_collision_test_structure_bit),
			&above,
			&collision_vector,
			NONE,
			&collision))
		{
			real distance = MAX(0.0f, collision.t*miss_distance - 0.1f);

			target.x = direction.i*distance + above.x;
			target.y = direction.j*distance + above.y;
			target.z = direction.k*distance + above.z;
		}
	}

	*target_point = target;

	return;
}

static boolean actor_combat_reaim_grenade(
	long actor_index,
	real_point3d const *grenade_origin)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_variant_definition *variant_definition =
		actor_variant_definition_get(actor->meta.variant_definition_index);
	short grenade_type_index = variant_definition->grenade_combat.grenade_type;
	struct game_globals_grenade *grenade = TAG_BLOCK_GET_ELEMENT(
		&scenario_get_game_globals()->grenades,
		grenade_type_index,
		struct game_globals_grenade);
	struct projectile_definition *projectile_definition = NULL;
	real aim_gravity;
	real_vector2d aim_vector2d;
	real_vector3d aim_vector;
	real aim_speed;
	real aim_ticks;
	boolean linear;
	boolean result = FALSE;

	if (grenade && grenade->projectile.index != NONE)
	{
		projectile_definition =
			projectile_definition_get(grenade->projectile.index);
	}

	match_assert(
		"c:\\halo\\SOURCE\\ai\\actor_combat.c",
		1731,
		projectile_definition);

	if (projectile_aim(
		projectile_definition,
		grenade_origin,
		&actor->control.grenade_current_target,
		NULL,
		NULL,
		NULL,
		&actor->control.grenade_current_aim_speed,
		actor->control.grenade_current_lob,
		&aim_vector,
		&aim_speed,
		&aim_ticks,
		NULL,
		&linear))
	{
		aim_vector2d.i = aim_vector.i;
		aim_vector2d.j = aim_vector.j;

		if (normalize2d(&aim_vector2d) > 0.0f &&
			aim_vector2d.i*actor->input.facing_vector.i +
				aim_vector2d.j*actor->input.facing_vector.j >
					GRENADE_AIMING_ANGLE_COSINE)
		{
			real_vector3d aim_velocity;

			aim_velocity.i = aim_vector.i*aim_speed;
			aim_velocity.j = aim_vector.j*aim_speed;
			aim_velocity.k = aim_vector.k*aim_speed;

			aim_gravity = linear ?
				0.0f :
				projectile_get_ballistic_acceleration(projectile_definition);

			if (ai_test_ballistic_line_of_fire(
				actor_index,
				grenade_origin,
				aim_ticks,
				&aim_velocity,
				aim_gravity,
				actor->control.grenade_current_ignore_object_index,
				actor->input.vehicle_index != NONE))
			{
				actor->control.grenade_current_aim_vector = aim_vector;
				actor->control.grenade_current_aim_speed = aim_speed;
				result = TRUE;
			}
		}
	}

	return result;
}

static boolean actor_combat_build_grenade_trajectory(
	short grenade_type_index,
	real_point3d const *grenade_origin,
	real velocity_max,
	real_point3d const *desired_impact_point,
	real *target_ballistic_fraction_min,
	boolean lob,
	real_vector3d *aim_vector,
	real *aim_speed,
	real *aim_ticks,
	real_vector3d *aim_velocity,
	real *aim_gravity)
{
	boolean result = FALSE;
	struct game_globals_grenade *grenade;

	match_assert(
		"c:\\halo\\SOURCE\\ai\\actor_combat.c",
		1780,
		grenade_origin && desired_impact_point && aim_vector && aim_speed);

	grenade = TAG_BLOCK_GET_ELEMENT(
		&scenario_get_game_globals()->grenades,
		grenade_type_index,
		struct game_globals_grenade);

	if (grenade && grenade->projectile.index != NONE)
	{
		struct projectile_definition *projectile_definition =
			projectile_definition_get(grenade->projectile.index);

		if (projectile_definition)
		{
			boolean linear = FALSE;

			if (projectile_aim(
				projectile_definition,
				grenade_origin,
				desired_impact_point,
				&velocity_max,
				NULL,
				target_ballistic_fraction_min,
				NULL,
				FALSE,
				aim_vector,
				aim_speed,
				aim_ticks,
				NULL,
				&linear))
			{
				result = TRUE;

				if (aim_velocity)
				{
					real speed = *aim_speed;

					aim_velocity->i = speed*aim_vector->i;
					aim_velocity->j = speed*aim_vector->j;
					aim_velocity->k = speed*aim_vector->k;
				}

				if (aim_gravity)
				{
					*aim_gravity = linear ?
						0.0f :
						projectile_get_ballistic_acceleration(projectile_definition);
				}
			}
		}
	}

	return result;
}

boolean actor_combat_find_grenade_target(
	long actor_index,
	real_point3d *grenade_target,
	long *grenade_target_prop_index,
	long *grenade_ignore_object_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_variant_definition *variant_definition =
		actor_variant_definition_get(actor->meta.variant_definition_index);
	boolean result = FALSE;

	if (actor->target.target_prop_index != NONE)
	{
		struct prop_datum *prop = prop_get(actor->target.target_prop_index);

		if (prop->enemy &&
			!prop->dead &&
			((prop->state >= _prop_state_becoming_unacknowledged &&
				prop->state <= _prop_state_acknowledged) ||
				prop->state == _prop_state_uninspected_orphan) &&
			prop->distance > variant_definition->grenade_combat.grenade_range_lower_bound &&
			prop->distance < variant_definition->grenade_combat.grenade_range_upper_bound)
		{
			*grenade_target = prop->body_position;
			grenade_target->z += 0.2f;
			*grenade_target_prop_index = actor->target.target_prop_index;
			*grenade_ignore_object_index = prop->vehicle_index;
			result = TRUE;

			if (actor->external_orders.playfighting)
			{
				actor_combat_find_nearby_target(grenade_target, 1.5f);
			}
		}
	}

	return result;
}

boolean actor_combat_check_collateral_damage(
	long actor_index,
	real enemy_radius,
	real collateral_damage_radius,
	real_point3d const *test_point,
	short *threat_count)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result;
	short threats;
	short counted_actor_count;
	long counted_actor_indices[MAXIMUM_COLLATERAL_DAMAGE_ACTORS];

	/* Preserve the original tag validation before iterating combat state. */
	actor_variant_definition_get(actor->meta.variant_definition_index);
	result = TRUE;
	threats = 0;
	counted_actor_count = 0;

	{
		struct prop_iterator prop_iterator;
		struct prop_datum *prop;

		prop_iterator_new(&prop_iterator, actor_index);
		for (prop = prop_iterator_next(&prop_iterator);
			prop;
			prop = prop_iterator_next(&prop_iterator))
		{
			if (prop->state >= _prop_state_becoming_unacknowledged &&
				prop->state <= _prop_state_acknowledged &&
				!prop->dead)
			{
				if (prop->enemy)
				{
					real_vector3d displacement;

					displacement.i = test_point->x - prop->body_position.x;
					displacement.j = test_point->y - prop->body_position.y;
					displacement.k = test_point->z - prop->body_position.z;
					if (displacement.i*displacement.i +
						displacement.j*displacement.j +
						displacement.k*displacement.k <
						enemy_radius*enemy_radius)
					{
						if (prop->player)
						{
							threats += 10;
						}
						else if (prop->vehicle_index != NONE)
						{
							threats += 5;
						}
						else if (prop->actor_index != NONE)
						{
							if (counted_actor_count < NUMBEROF(counted_actor_indices))
							{
								counted_actor_indices[counted_actor_count++] = prop->actor_index;
							}

							if (prop->swarm)
							{
								threats += actor_get(prop->actor_index)->meta.swarm_unit_count;
							}
							else
							{
								threats++;
							}
						}
					}
				}
				else if (collateral_damage_radius > 0.0f)
				{
					real_vector3d displacement;

					displacement.i = test_point->x - prop->body_position.x;
					displacement.j = test_point->y - prop->body_position.y;
					displacement.k = test_point->z - prop->body_position.z;
					if (displacement.i*displacement.i +
						displacement.j*displacement.j +
						displacement.k*displacement.k <
							collateral_damage_radius*collateral_damage_radius)
					{
						result = FALSE;
						break;
					}
				}
			}
		}
	}

	if (enemy_radius > 0.0f && actor->target.target_prop_index != NONE)
	{
		struct prop_datum *target_prop = prop_get(actor->target.target_prop_index);

		if (target_prop->actor_index != NONE)
		{
			struct actor_datum *target_actor = actor_get(target_prop->actor_index);

			if (target_actor->meta.encounter_index != NONE)
			{
				struct encounter_actor_iterator actor_iterator;
				struct actor_datum *enemy_actor;

				encounter_actor_iterator_new(
					&actor_iterator,
					target_actor->meta.encounter_index);
				for (enemy_actor = encounter_actor_iterator_next(&actor_iterator);
					enemy_actor;
					enemy_actor = encounter_actor_iterator_next(&actor_iterator))
				{
					short index;
					boolean already_counted = FALSE;

					for (index = 0; index < counted_actor_count; index++)
					{
						if (counted_actor_indices[index] == actor_iterator.index)
						{
							already_counted = TRUE;
							break;
						}
					}

					if (!already_counted)
					{
						real_vector3d displacement;

						displacement.i = test_point->x -
							enemy_actor->input.position.body_position.x;
						displacement.j = test_point->y -
							enemy_actor->input.position.body_position.y;
						displacement.k = test_point->z -
							enemy_actor->input.position.body_position.z;
						if (displacement.i*displacement.i +
							displacement.j*displacement.j +
							displacement.k*displacement.k <
								enemy_radius*enemy_radius)
						{
							if (enemy_actor->meta.swarm)
							{
								threats += enemy_actor->meta.swarm_unit_count;
							}
							else
							{
								threats++;
							}
						}
					}
				}
			}
		}
	}

	if (result && actor->meta.encounter_index != NONE && collateral_damage_radius > 0.0f)
	{
		struct encounter_actor_iterator actor_iterator;
		struct actor_datum *friend_actor;

		encounter_actor_iterator_new(
			&actor_iterator,
			actor->meta.encounter_index);
		for (friend_actor = encounter_actor_iterator_next(&actor_iterator);
			friend_actor;
			friend_actor = encounter_actor_iterator_next(&actor_iterator))
		{
			real_vector3d displacement;

			displacement.i = test_point->x -
				friend_actor->input.position.body_position.x;
			displacement.j = test_point->y -
				friend_actor->input.position.body_position.y;
			displacement.k = test_point->z -
				friend_actor->input.position.body_position.z;
			if (displacement.i*displacement.i +
				displacement.j*displacement.j +
				displacement.k*displacement.k <
					collateral_damage_radius*collateral_damage_radius)
			{
				result = FALSE;
				break;
			}
		}
	}

	if (threat_count)
	{
		*threat_count = threats;
	}

	return result;
}

boolean actor_combat_plan_grenade_trajectory(
	long actor_index,
	short trajectory_type,
	real_point3d const *grenade_target,
	long grenade_target_prop_index,
	long grenade_ignore_object_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_variant_definition *variant_definition =
		actor_variant_definition_get(actor->meta.variant_definition_index);
	real_point3d grenade_origin = actor->input.position.head_position;
	real_vector3d aim_vector;
	real_vector3d aim_velocity;
	real aim_speed;
	real aim_ticks;
	real aim_gravity;
	boolean result = FALSE;

	if (actor_combat_build_grenade_trajectory(
			variant_definition->grenade_combat.grenade_type,
			&grenade_origin,
			variant_definition->grenade_combat.grenade_maximum_velocity,
			grenade_target,
			NULL,
			FALSE,
			&aim_vector,
			&aim_speed,
			&aim_ticks,
			&aim_velocity,
			&aim_gravity) &&
		ai_test_ballistic_line_of_fire(
			actor_index,
			&grenade_origin,
			aim_ticks,
			&aim_velocity,
			aim_gravity,
			grenade_ignore_object_index,
			actor->input.vehicle_index != NONE))
	{
		actor->control.grenade_current_target = *grenade_target;
		actor->control.grenade_current_prop_index = grenade_target_prop_index;
		actor->control.grenade_current_ignore_object_index = grenade_ignore_object_index;
		actor->control.grenade_current_aim_vector = aim_vector;
		actor->control.grenade_current_aim_speed = aim_speed;
		actor->control.grenade_current_lob = FALSE;

		result = TRUE;
	}

	return result;
}

long actor_aim_projectile(
	long actor_index,
	real_point3d const *origin,
	real_vector3d *vector,
	real *error_reference)
{
	struct actor_datum *actor = actor_get(actor_index);
	long target_unit_index = NONE;

	if (actor->control.fire_state == _actor_fire_state_bursting)
	{
		struct actor_debug_info *actor_debug_info =
			&actor_debug_array[DATUM_INDEX_TO_ABSOLUTE_INDEX(actor_index)];
		real_vector3d weapon_vector;
		real alignment;

		actor_debug_info->last_projectile_aiming_time = game_time_get();

		if (actor->control.current_fire_target_type == _actor_fire_target_prop &&
			actor->control.current_fire_target_prop_index != NONE)
		{
			struct prop_datum *prop =
				prop_get(actor->control.current_fire_target_prop_index);

			if (prop->state >= _prop_state_becoming_unacknowledged &&
				prop->state <= _prop_state_acknowledged)
			{
				target_unit_index = prop->unit_index;
			}
		}

		match_assert(
			"c:\\halo\\SOURCE\\ai\\actor_combat.c",
			1071,
			vector);

		actor_debug_info->field_60 = actor->control.burst_aim_by_vector;
		actor_debug_info->field_64 = *origin;

		if (actor->control.burst_aim_by_vector)
		{
			*vector = actor->control.burst_aim_vector;
		}
		else
		{
			real_point3d const *burst_target = &actor->control.burst_target;

			actor_debug_info->field_7C = *burst_target;
			vector->i = burst_target->x - origin->x;
			vector->j = burst_target->y - origin->y;
			vector->k = burst_target->z - origin->z;
			normalize3d(vector);
		}

		match_assert_valid_real_normal3d(
			"c:\\halo\\SOURCE\\ai\\actor_combat.c",
			1090,
			vector);
		actor_debug_info->field_70 = *vector;

		actor_get_weapon_vector(actor_index, &weapon_vector);
		alignment = dot_product3d(&weapon_vector, vector);
		if (alignment < GRENADE_AIMING_ANGLE_COSINE)
		{
			real_vector3d rotation_axis;
			boolean aiming_success = TRUE;

			cross_product3d(&weapon_vector, vector, &rotation_axis);
			if (normalize3d(&rotation_axis) == 0.0f)
			{
				perpendicular3d(&weapon_vector, &rotation_axis);
				if (normalize3d(&rotation_axis) == 0.0f)
				{
					aiming_success = FALSE;
				}
			}

			*vector = weapon_vector;
			if (aiming_success)
			{
				rotate_vector_about_axis(
					vector,
					&rotation_axis,
					GRENADE_AIMING_ANGLE_SINE,
					GRENADE_AIMING_ANGLE_COSINE);
			}

			actor_debug_info->field_98 = weapon_vector;
			actor_debug_info->field_88 = TRUE;
			actor_debug_info->field_8C = *vector;
		}
		else
		{
			actor_debug_info->field_88 = FALSE;
		}

		*error_reference = actor->control.burst_error;
	}

	return target_unit_index;
}

static boolean actor_combat_retarget_grenade(
	long actor_index,
	real_point3d const *desired_grenade_target)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_variant_definition *variant_definition =
		actor_variant_definition_get(actor->meta.variant_definition_index);
	boolean result = FALSE;

	if (actor_combat_check_collateral_damage(
		actor_index,
		variant_definition->grenade_combat.enemy_radius,
		variant_definition->grenade_combat.collateral_damage_radius,
		desired_grenade_target,
		NULL))
	{
		actor->control.grenade_current_target = *desired_grenade_target;
		result = TRUE;
	}

	return result;
}

long actor_aim_grenade(
	long actor_index,
	real_point3d const *origin,
	real_vector3d *vector)
{
	struct actor_datum *actor = actor_get(actor_index);
	real_vector3d aim_vector;
	long target_unit_index = NONE;

	if (actor->control.grenade_current_prop_index != NONE)
	{
		struct prop_datum *prop = prop_get(actor->control.grenade_current_prop_index);

		if (prop->state >= _prop_state_becoming_unacknowledged &&
			prop->state <= _prop_state_acknowledged)
		{
			target_unit_index = prop->unit_index;
		}

		if (prop->state < _prop_state_unacknowledged ||
			prop->state > _prop_state_becoming_acknowledged)
		{
			aim_vector.n[0] = prop->body_position.x;
			aim_vector.n[1] = prop->body_position.y;
			aim_vector.n[2] = prop->body_position.z + 0.2f;
			actor_combat_retarget_grenade(
				actor_index,
				(real_point3d const *)&aim_vector);
		}
	}

	actor_combat_reaim_grenade(actor_index, origin);

	if (actor->input.vehicle_index == NONE)
	{
		real_vector2d aim_vector2d;

		aim_vector = actor->control.grenade_current_aim_vector;
		aim_vector2d.i = aim_vector.i;
		aim_vector2d.j = aim_vector.j;

		if (normalize2d(&aim_vector2d) > 0.0f &&
			aim_vector2d.i*actor->input.facing_vector.i +
				aim_vector2d.j*actor->input.facing_vector.j <
					GRENADE_AIMING_ANGLE_COSINE)
		{
			boolean counterclockwise =
				aim_vector2d.j*actor->input.facing_vector.i -
					aim_vector2d.i*actor->input.facing_vector.j > 0.0f;
			real_vector3d new_aim_vector = actor->input.facing_vector;
			real magnitude;

			rotate_vector_about_axis(
				&new_aim_vector,
				global_up3d,
				(counterclockwise ? 1 : -1)*GRENADE_AIMING_ANGLE_SINE,
				GRENADE_AIMING_ANGLE_COSINE);

			magnitude = square_root(
				aim_vector.i*aim_vector.i + aim_vector.j*aim_vector.j);
			new_aim_vector.i *= magnitude;
			new_aim_vector.j *= magnitude;
			new_aim_vector.k = aim_vector.k;

			match_assert_valid_real_normal3d(
				"c:\\halo\\SOURCE\\ai\\actor_combat.c",
				1865,
				&new_aim_vector);

			aim_vector = new_aim_vector;
		}
	}

	vector->i = aim_vector.i*actor->control.grenade_current_aim_speed;
	vector->j = aim_vector.j*actor->control.grenade_current_aim_speed;
	vector->k = aim_vector.k*actor->control.grenade_current_aim_speed;

	return target_unit_index;
}

/* ---------- private code */
