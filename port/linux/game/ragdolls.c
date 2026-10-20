/*
RAGDOLLS.C

Dead bipeds as ragdolls (ragdolls.h). The game calls in when it damages an
object and when a unit dies; nothing here changes the game's state. Each
hit on a biped is remembered for the tick, so that a death can carry the
blow that caused it (the death comes later in the same damage, units.c's
unit_damage_aftermath); each death of a biped with a body
(ragdoll_bodies.c) is queued with the biped's node matrices and velocity
for the physics' next tick to start its ragdoll from. The ragdolls the host
has are kept here too: when each started, whether its motors still follow
the animation, and the poses the host last gave.
*/

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "objects/damage.h"
#include "objects/damage_effect_definitions.h"
#include "objects/object_types.h"
#include "objects/objects.h"
#include "tag_files/tag_groups.h"
#include "physics_jolt.h"
#include "ragdolls.h"

#include <string.h>

static struct
{
	long death_count;
	struct ragdoll_death deaths[RAGDOLL_DEATHS_MAX];
	long hit_count;
	struct ragdoll_hit hits[RAGDOLL_HITS_MAX];
	struct ragdoll active[RAGDOLLS_MAX];
} ragdolls;

boolean ragdolls_enabled(void)
{
	return physics_jolt_ragdolls_enabled();
}

static boolean ragdolls_object_is_biped(
	long object_index)
{
	struct object_datum const *object = object_get(object_index);

	return object->object.type == _object_type_biped;
}

void ragdolls_damage(
	long object_index,
	struct damage_data const *damage)
{
	struct damage_effect_definition const *damage_effect;
	struct ragdoll_hit *hit;
	real zero_scale;
	real length;

	if (!ragdolls_enabled() || object_index == NONE || !ragdolls_object_is_biped(object_index) ||
		damage->definition_index == NONE)
	{
		return;
	}
	damage_effect = damage_effect_definition_get(damage->definition_index);
	hit = &ragdolls.hits[ragdolls.hit_count % RAGDOLL_HITS_MAX];
	ragdolls.hit_count++;
	hit->object_index = object_index;
	hit->time = game_time_get();
	hit->point = damage->origin;
	hit->direction = damage->direction;
	length = magnitude3d(&hit->direction);
	/* (the damage's scale, falling off with an explosion's distance, takes
	the acceleration from its zero-scale factor to all of it) */
	zero_scale = damage_effect->damage.instantaneous_acceleration_zero_scale_factor;
	hit->speed = damage_effect->damage.instantaneous_acceleration *
		(zero_scale + (1.0f - zero_scale) * PIN(damage->scale, 0.0f, 1.0f));
	if (length > _real_epsilon && hit->speed == hit->speed)
	{
		scale_vector3d(&hit->direction, 1.0f / length, &hit->direction);
	}
	else
	{
		hit->direction.i = hit->direction.j = hit->direction.k = 0.0f;
		hit->speed = 0.0f;
	}
}

struct ragdoll_hit const *ragdolls_hit_get(
	long object_index)
{
	long time = game_time_get();
	long count = ragdolls.hit_count < RAGDOLL_HITS_MAX ? ragdolls.hit_count : RAGDOLL_HITS_MAX;
	long index;

	for (index = 1; index <= count; index++)
	{
		struct ragdoll_hit const *hit = &ragdolls.hits[(ragdolls.hit_count - index) % RAGDOLL_HITS_MAX];

		if (hit->object_index == object_index && hit->time == time)
			return hit;
	}
	return NULL;
}

void ragdolls_unit_died(
	long unit_index)
{
	struct object_datum const *object;
	struct ragdoll_body const *body;
	struct ragdoll_death *death;
	struct ragdoll_hit const *hit;
	real_vector3d angular_velocity;
	long index;

	if (!ragdolls_enabled() || unit_index == NONE || !ragdolls_object_is_biped(unit_index))
		return;
	for (index = 0; index < ragdolls.death_count; index++)
	{
		if (ragdolls.deaths[index].object_index == unit_index)
			return;
	}
	object = object_get(unit_index);
	body = ragdoll_body_get(object->definition_index);
	if (!body || ragdolls.death_count >= RAGDOLL_DEATHS_MAX)
		return;
	death = &ragdolls.deaths[ragdolls.death_count++];
	death->object_index = unit_index;
	death->definition_index = object->definition_index;
	death->body = body;
	memcpy(death->nodes, object_get_node_matrices(unit_index), body->node_count * sizeof(real_matrix4x3));
	object_get_velocities(unit_index, &death->velocity, &angular_velocity);
	hit = ragdolls_hit_get(unit_index);
	death->hit = hit != NULL;
	if (hit)
		death->killing_hit = *hit;
	else
		memset(&death->killing_hit, 0, sizeof(death->killing_hit));
}

long ragdolls_death_count(void)
{
	return ragdolls.death_count;
}

struct ragdoll_death const *ragdolls_death_get(
	long index)
{
	return index >= 0 && index < ragdolls.death_count ? &ragdolls.deaths[index] : NULL;
}

void ragdolls_deaths_clear(void)
{
	ragdolls.death_count = 0;
}

void ragdoll_pose_from_matrix(
	real_matrix4x3 const *matrix,
	struct ragdoll_pose *pose)
{
	real length;

	pose->position = matrix->position;
	matrix4x3_rotation_to_quaternion(matrix, &pose->rotation);
	pose->rotation.v.i = -pose->rotation.v.i;
	pose->rotation.v.j = -pose->rotation.v.j;
	pose->rotation.v.k = -pose->rotation.v.k;
	length = square_root(pose->rotation.v.i * pose->rotation.v.i + pose->rotation.v.j * pose->rotation.v.j +
		pose->rotation.v.k * pose->rotation.v.k + pose->rotation.w * pose->rotation.w);
	if (length > _real_epsilon)
	{
		pose->rotation.v.i /= length;
		pose->rotation.v.j /= length;
		pose->rotation.v.k /= length;
		pose->rotation.w /= length;
	}
	else
	{
		pose->rotation.v.i = pose->rotation.v.j = pose->rotation.v.k = 0.0f;
		pose->rotation.w = 1.0f;
	}
}

struct ragdoll *ragdolls_get(
	long id)
{
	return id >= 1 && id <= RAGDOLLS_MAX && ragdolls.active[id - 1].used ? &ragdolls.active[id - 1] : NULL;
}

struct ragdoll const *ragdolls_of_object(
	long object_index)
{
	long index;

	if (object_index == NONE)
		return NULL;
	for (index = 0; index < RAGDOLLS_MAX; index++)
	{
		if (ragdolls.active[index].used && ragdolls.active[index].object_index == object_index)
			return &ragdolls.active[index];
	}
	return NULL;
}

long ragdolls_start(
	struct ragdoll_death const *death,
	long *replaced)
{
	struct ragdoll *ragdoll;
	long chosen = NONE;
	long index;

	*replaced = NONE;
	if (ragdolls_of_object(death->object_index))
		return NONE;
	for (index = 0; index < RAGDOLLS_MAX && chosen == NONE; index++)
	{
		if (!ragdolls.active[index].used)
			chosen = index;
	}
	if (chosen == NONE)
	{
		chosen = 0;
		for (index = 1; index < RAGDOLLS_MAX; index++)
		{
			if (ragdolls.active[index].start_time < ragdolls.active[chosen].start_time)
				chosen = index;
		}
		*replaced = chosen + 1;
	}
	ragdoll = &ragdolls.active[chosen];
	memset(ragdoll, 0, sizeof(*ragdoll));
	ragdoll->used = TRUE;
	ragdoll->object_index = death->object_index;
	ragdoll->body = death->body;
	ragdoll->start_time = game_time_get();
	return chosen + 1;
}

short ragdolls_action(
	long id)
{
	struct ragdoll *ragdoll = ragdolls_get(id);

	if (!ragdoll)
		return _ragdoll_action_none;
	if (!object_try_and_get_and_verify_type(ragdoll->object_index, _object_mask_biped))
		return _ragdoll_action_remove;
	if (ragdoll->limp)
		return _ragdoll_action_none;
	if (game_time_get() - ragdoll->start_time < RAGDOLL_DRIVE_TICKS)
		return _ragdoll_action_drive;
	ragdoll->limp = TRUE;
	return _ragdoll_action_limp;
}

void ragdolls_stop(
	long id)
{
	struct ragdoll *ragdoll = ragdolls_get(id);

	if (ragdoll)
		ragdoll->used = FALSE;
}

void ragdolls_dispose_from_old_map(void)
{
	ragdolls.death_count = 0;
	ragdolls.hit_count = 0;
	memset(ragdolls.active, 0, sizeof(ragdolls.active));
}
