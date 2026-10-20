/*
RAGDOLLS.C

Dead bipeds as ragdolls (ragdolls.h). The game calls in when it damages an
object and when a unit dies; nothing here changes the game's state. Each
hit on a biped is remembered for the tick, so that a death can carry the
blow that caused it (the death comes later in the same damage, units.c's
unit_damage_aftermath), and a hit on a ragdoll's biped pushes the ragdoll;
each death of a biped with a body
(ragdoll_bodies.c) is queued with the biped's node matrices and velocity
for the physics' next tick to start its ragdoll from. The ragdolls the host
has are kept here too: when each started, whether its motors still follow
the animation, and the poses the host last gave; and the living bipeds and
vehicles near them, which the host has as kinematic stand-ins.

With game.dismemberment an explosion (a grenade's, a high explosive's or a
mounted weapon's) that kills a biped, or that reaches its ragdoll, tears the
joints whose parts it hurts past their species' threshold (ragdoll_bodies.c's
`break` overrides), the parts nearest it the most: the host lets each limb go
as a ragdoll of its own. Bullets, melee and vehicles tear nothing. The
biped's own blood (its collision model's localized damage effect, which
damage.c makes where a hit lands) comes out of the joint as it tears, and a
torn body is drawn a piece at a time, each draw of the model collapsing the
other pieces' bones to a point at the stump.
*/

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "objects/damage.h"
#include "objects/damage_effect_definitions.h"
#include "objects/object_definitions.h"
#include "objects/object_types.h"
#include "objects/objects.h"
#include "effects/effects.h"
#include "physics/collision_model_definitions.h"
#include "tag_files/tag_groups.h"
#include "physics_debris.h"
#include "physics_jolt.h"
#include "ragdolls.h"

#include <string.h>

/* (damage.c's _damage_effect_dont_scale_damage_by_distance_bit) */
#define RAGDOLL_DAMAGE_EFFECT_UNSCALED_BIT 0
/* (damage.c's _damage_category_grenade, _highexplosive and _mountedweapon) */
#define RAGDOLL_DAMAGE_CATEGORY_GRENADE 3
#define RAGDOLL_DAMAGE_CATEGORY_HIGH_EXPLOSIVE 4
#define RAGDOLL_DAMAGE_CATEGORY_MOUNTED_WEAPON 8

/* an effect's markers (as damage.c's and projectiles.c's) */
enum
{
	_effect_vector_normal = 0,
	_effect_vector_incident,
	_effect_vector_negative_incident,
	_effect_vector_reflected,
	_effect_vector_gravity,
	NUMBER_OF_EFFECT_MARKERS,
};

static struct
{
	long death_count;
	struct ragdoll_death deaths[RAGDOLL_DEATHS_MAX];
	long hit_count;
	struct ragdoll_hit hits[RAGDOLL_HITS_MAX];
	struct ragdoll active[RAGDOLLS_MAX];
	long impulse_count;
	struct ragdoll_impulse impulses[RAGDOLL_IMPULSES_MAX];
	long break_count;
	struct ragdoll_break breaks[RAGDOLL_BREAKS_MAX];
	struct
	{
		boolean used;
		boolean seen;
		long object_index;
	} stand_ins[RAGDOLL_STAND_INS_MAX];
	/* each ragdoll's node matrices as drawn (ragdolls_node_matrices) */
	real_matrix4x3 drawn[RAGDOLLS_MAX][RAGDOLL_BODY_NODES_MAX];
	/* one piece's, as drawn (ragdolls_piece_node_matrices); the model's draw
	takes its own copy */
	real_matrix4x3 piece[RAGDOLL_BODY_NODES_MAX];
} ragdolls;

boolean ragdolls_enabled(void)
{
	return physics_jolt_ragdolls_enabled();
}

boolean ragdolls_dismemberment_enabled(void)
{
	return physics_jolt_dismemberment_enabled();
}

static boolean ragdolls_object_is_biped(
	long object_index)
{
	struct object_datum const *object = object_get(object_index);

	return object->object.type == _object_type_biped;
}

/* a point in a part's frame, in the world's */
static void ragdoll_pose_transform(
	struct ragdoll_pose const *pose,
	real_point3d const *point,
	real_point3d *result)
{
	real_quaternion const *q = &pose->rotation;
	real_vector3d t;

	/* (p + 2w(v x p) + v x 2(v x p)) */
	t.i = 2.0f * (q->v.j * point->z - q->v.k * point->y);
	t.j = 2.0f * (q->v.k * point->x - q->v.i * point->z);
	t.k = 2.0f * (q->v.i * point->y - q->v.j * point->x);
	result->x = pose->position.x + point->x + q->w * t.i + (q->v.j * t.k - q->v.k * t.j);
	result->y = pose->position.y + point->y + q->w * t.j + (q->v.k * t.i - q->v.i * t.k);
	result->z = pose->position.z + point->z + q->w * t.k + (q->v.i * t.j - q->v.j * t.i);
}

short ragdoll_part_nearest(
	struct ragdoll_body const *body,
	struct ragdoll_pose const *poses,
	real_point3d const *point)
{
	short nearest = 0;
	real nearest_distance = 0.0f;
	short index;

	for (index = 0; index < body->part_count; index++)
	{
		real_point3d center;
		real distance;

		ragdoll_pose_transform(&poses[index], &body->parts[index].center, &center);
		distance = (center.x - point->x) * (center.x - point->x) + (center.y - point->y) * (center.y - point->y) +
			(center.z - point->z) * (center.z - point->z);
		if (index == 0 || distance < nearest_distance)
		{
			nearest = index;
			nearest_distance = distance;
		}
	}
	return nearest;
}

/* A hit's push on a ragdoll, a share of it: lifted as damage.c lifts it, the
ragdoll's mass at the hit's speed, on the part nearest where it struck. */
static void ragdolls_push(
	long id,
	struct ragdoll_hit const *hit,
	real share)
{
	struct ragdoll const *ragdoll = &ragdolls.active[id - 1];
	struct ragdoll_impulse *impulse;
	real_vector3d direction;
	real length;

	if (hit->speed <= 0.0f || ragdolls.impulse_count >= RAGDOLL_IMPULSES_MAX)
		return;
	direction = hit->direction;
	direction.k += RAGDOLL_PUSH_LIFT;
	length = magnitude3d(&direction);
	if (length <= _real_epsilon)
		return;
	impulse = &ragdolls.impulses[ragdolls.impulse_count++];
	impulse->id = id;
	impulse->part = ragdoll_part_nearest(ragdoll->body, ragdoll->poses, &hit->point);
	impulse->pad = 0;
	scale_vector3d(&direction, share * hit->speed * ragdoll->body->mass / length, &impulse->impulse);
	impulse->point = hit->point;
}

/* an id's pushes and tears not yet sent dropped: its ragdoll is gone */
static void ragdolls_impulses_drop(
	long id)
{
	long kept = 0;
	long index;

	for (index = 0; index < ragdolls.impulse_count; index++)
	{
		if (ragdolls.impulses[index].id != id)
			ragdolls.impulses[kept++] = ragdolls.impulses[index];
	}
	ragdolls.impulse_count = kept;
	kept = 0;
	for (index = 0; index < ragdolls.break_count; index++)
	{
		if (ragdolls.breaks[index].id != id)
			ragdolls.breaks[kept++] = ragdolls.breaks[index];
	}
	ragdolls.break_count = kept;
}

real ragdoll_hit_damage_at(
	struct ragdoll_hit const *hit,
	real_point3d const *point)
{
	real_vector3d offset;
	real distance;
	real scale = 1.0f;

	if (!hit->explosion)
		return 0.0f;
	offset.i = point->x - hit->epicenter.x;
	offset.j = point->y - hit->epicenter.y;
	offset.k = point->z - hit->epicenter.z;
	distance = square_root(offset.i * offset.i + offset.j * offset.j + offset.k * offset.k);
	if (hit->cutoff_radius > hit->falloff_radius)
	{
		scale = PIN(1.0f - (distance - hit->falloff_radius) / (hit->cutoff_radius - hit->falloff_radius),
			0.0f, 1.0f);
	}
	/* (damage.c does no damage where the distance's scale is nothing) */
	if (scale <= 0.0f)
		return 0.0f;
	if (!hit->scaled)
		scale = 1.0f;
	return (1.0f - scale) * hit->damage_minimum + scale * hit->damage_maximum;
}

real ragdoll_hit_tearing_at(
	struct ragdoll_hit const *hit,
	real_point3d const *point)
{
	real_vector3d offset;
	real closeness = 1.0f;

	offset.i = point->x - hit->epicenter.x;
	offset.j = point->y - hit->epicenter.y;
	offset.k = point->z - hit->epicenter.z;
	if (hit->cutoff_radius > _real_epsilon)
	{
		closeness = PIN(1.0f - square_root(offset.i * offset.i + offset.j * offset.j + offset.k * offset.k) /
			hit->cutoff_radius, 0.0f, 1.0f);
	}
	return ragdoll_hit_damage_at(hit, point) * closeness;
}

/* a body's own share of its species' threshold for a part's joint: a
quarter either way, the same for the body each time */
static real ragdoll_break_chance(
	long object_index,
	short part)
{
	unsigned long hash = ((unsigned long)object_index * 31UL + (unsigned long)part) * 2654435761UL;

	return 0.75f + 0.5f * (real)((hash >> 16) & 0xFFFF) / 65535.0f;
}

/* A hit's tearing of a ragdoll's joints: each part whose joint can break
and that the explosion hurts past its threshold, at most
RAGDOLL_BREAKS_PER_HIT of them, the worst hurt first; none torn already, or
on a limb torn off already. */
/* the biped's blood where a part's joint tore, as damage.c's
damage_effect_new_at_location makes a hit's: its collision model's localized
damage effect, at the joint and facing away from the blast */
static void ragdolls_bleed(
	struct ragdoll const *ragdoll,
	short part,
	struct ragdoll_hit const *hit)
{
	struct object_definition const *definition;
	struct collision_model const *collision_model;
	char const *marker_names[NUMBER_OF_EFFECT_MARKERS];
	real_point3d marker_points[NUMBER_OF_EFFECT_MARKERS];
	real_vector3d marker_forwards[NUMBER_OF_EFFECT_MARKERS];
	real_vector3d away;
	real length;
	short marker;

	definition = object_definition_get(object_get(ragdoll->object_index)->definition_index);
	if (definition->object.collision_model.index == NONE)
		return;
	collision_model = collision_model_definition_get(definition->object.collision_model.index);
	if (collision_model->resistance.localized_damage_effect.index == NONE)
		return;
	away.i = ragdoll->poses[part].position.x - hit->epicenter.x;
	away.j = ragdoll->poses[part].position.y - hit->epicenter.y;
	away.k = ragdoll->poses[part].position.z - hit->epicenter.z;
	length = magnitude3d(&away);
	if (length > _real_epsilon)
	{
		scale_vector3d(&away, 1.0f / length, &away);
	}
	else
	{
		away.i = away.j = 0.0f;
		away.k = 1.0f;
	}
	marker_names[_effect_vector_normal] = "normal";
	marker_names[_effect_vector_incident] = "incident";
	marker_names[_effect_vector_negative_incident] = "negative incident";
	marker_names[_effect_vector_reflected] = "reflection";
	marker_names[_effect_vector_gravity] = "gravity";
	marker_forwards[_effect_vector_normal] = away;
	scale_vector3d(&away, -1.0f, &marker_forwards[_effect_vector_incident]);
	marker_forwards[_effect_vector_negative_incident] = away;
	scale_vector3d(&away, -1.0f, &marker_forwards[_effect_vector_reflected]);
	marker_forwards[_effect_vector_gravity].i = 0.0f;
	marker_forwards[_effect_vector_gravity].j = 0.0f;
	marker_forwards[_effect_vector_gravity].k = -1.0f;
	for (marker = 0; marker < NUMBER_OF_EFFECT_MARKERS; marker++)
		marker_points[marker] = ragdoll->poses[part].position;
	/* (unattached: the game's node matrices are not where the ragdoll lies) */
	effect_new_unattached_from_markers(collision_model->resistance.localized_damage_effect.index,
		ragdoll->object_index, global_zero_vector3d, NUMBER_OF_EFFECT_MARKERS, marker_names, marker_points,
		marker_forwards, 1.0f, 0.0f, NULL, NULL, FALSE);
}

static void ragdolls_tear(
	long id,
	struct ragdoll_hit const *hit)
{
	struct ragdoll const *ragdoll = &ragdolls.active[id - 1];
	struct ragdoll_body const *body = ragdoll->body;
	unsigned long torn = ragdoll->broken_joints;
	long index;
	short count;

	/* (a hit that is no explosion hurts no part: ragdoll_hit_damage_at) */
	if (!ragdolls_dismemberment_enabled())
		return;
	for (index = 0; index < ragdolls.break_count; index++)
	{
		if (ragdolls.breaks[index].id == id)
			torn |= 1UL << ragdolls.breaks[index].part;
	}
	for (count = 0; count < RAGDOLL_BREAKS_PER_HIT && ragdolls.break_count < RAGDOLL_BREAKS_MAX; count++)
	{
		struct ragdoll_break *tear;
		short worst = NONE;
		real worst_share = 1.0f;
		short part;

		for (part = 1; part < body->part_count; part++)
		{
			real_point3d center;
			real share;
			short above = part;

			if (body->parts[part].break_damage <= 0.0f)
				continue;
			while (above != NONE && !(torn & (1UL << above)))
				above = body->parts[above].parent_part_index;
			if (above != NONE)
				continue;
			ragdoll_pose_transform(&ragdoll->poses[part], &body->parts[part].center, &center);
			share = ragdoll_hit_tearing_at(hit, &center) /
				(body->parts[part].break_damage * ragdoll_break_chance(ragdoll->object_index, part));
			if (share >= worst_share)
			{
				worst = part;
				worst_share = share;
			}
		}
		if (worst == NONE)
			break;
		torn |= 1UL << worst;
		tear = &ragdolls.breaks[ragdolls.break_count++];
		tear->id = id;
		tear->part = worst;
		tear->pad = 0;
		ragdolls_bleed(ragdoll, worst, hit);
	}
}

void ragdolls_damage(
	long object_index,
	struct damage_data const *damage)
{
	struct damage_effect_definition const *damage_effect;
	struct object_definition const *object_definition;
	struct ragdoll_hit *hit;
	struct ragdoll const *ragdoll;
	real length;

	if (!ragdolls_enabled() || object_index == NONE || !ragdolls_object_is_biped(object_index) ||
		damage->definition_index == NONE)
	{
		return;
	}
	damage_effect = damage_effect_definition_get(damage->definition_index);
	object_definition = object_definition_get(object_get(object_index)->definition_index);
	hit = &ragdolls.hits[ragdolls.hit_count % RAGDOLL_HITS_MAX];
	ragdolls.hit_count++;
	hit->object_index = object_index;
	hit->time = game_time_get();
	hit->point = damage->origin;
	hit->direction = damage->direction;
	length = magnitude3d(&hit->direction);
	/* (as damage.c's object_damage_aftermath accelerates the object: all of
	it, however far from an explosion) */
	hit->speed = damage_effect->damage.instantaneous_acceleration * object_definition->object.acceleration_scale;
	hit->explosion = TEST_FLAG(damage->flags, _damage_area_of_effect_bit) &&
		(damage_effect->damage.category == RAGDOLL_DAMAGE_CATEGORY_GRENADE ||
		damage_effect->damage.category == RAGDOLL_DAMAGE_CATEGORY_HIGH_EXPLOSIVE ||
		damage_effect->damage.category == RAGDOLL_DAMAGE_CATEGORY_MOUNTED_WEAPON);
	hit->scaled = !TEST_FLAG(damage_effect->flags, RAGDOLL_DAMAGE_EFFECT_UNSCALED_BIT);
	hit->epicenter = damage->epicenter;
	hit->damage_minimum = damage_effect->damage.damage_minimum * damage->multiplier;
	hit->damage_maximum = 0.5f * (damage_effect->damage.damage_lower_bound + damage_effect->damage.damage_upper_bound) *
		damage->multiplier;
	hit->falloff_radius = damage_effect->falloff_radius;
	hit->cutoff_radius = damage_effect->cutoff_radius;
	if (length > _real_epsilon && hit->speed > _real_epsilon)
	{
		scale_vector3d(&hit->direction, 1.0f / length, &hit->direction);
	}
	else
	{
		hit->direction.i = hit->direction.j = hit->direction.k = 0.0f;
		hit->speed = 0.0f;
	}
	/* a ragdoll's biped hit again: the whole push, as a corpse takes it */
	ragdoll = ragdolls_of_object(object_index);
	if (ragdoll)
	{
		ragdolls_push((long)(ragdoll - ragdolls.active) + 1, hit, 1.0f);
		ragdolls_tear((long)(ragdoll - ragdolls.active) + 1, hit);
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

short ragdoll_part_piece(
	struct ragdoll const *ragdoll,
	short part)
{
	if (part < 0 || part >= ragdoll->body->part_count)
		return NONE;
	while (part > 0 && !TEST_FLAG(ragdoll->broken_joints, part))
		part = ragdoll->body->parts[part].parent_part_index;
	return part;
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

real_matrix4x3 const *ragdolls_node_matrices(
	long object_index,
	real_matrix4x3 const *nodes)
{
	struct ragdoll const *ragdoll = ragdolls_of_object(object_index);
	struct ragdoll_body const *body;
	real_matrix4x3 *drawn;
	real_matrix4x3 frames[RAGDOLL_BODY_PARTS_MAX];
	real_matrix4x3 inverses[RAGDOLL_BODY_PARTS_MAX];
	short part;
	short node;

	/* (a piece of garbage that is a debris body, physics_debris.c) */
	if (!ragdoll)
		return physics_debris_node_matrices(object_index, nodes);
	/* (a frame between ticks is drawn from snapshots this has given already,
	render_interpolation.c) */
	if (!nodes || !ragdoll->posed || render_interpolation_object_node_matrices(object_index))
		return nodes;
	body = ragdoll->body;
	drawn = ragdolls.drawn[ragdoll - ragdolls.active];
	/* each part's node where the host has it, at the scale the game has it */
	for (part = 0; part < body->part_count; part++)
	{
		struct ragdoll_pose const *pose = &ragdoll->poses[part];
		real_quaternion game_rotation;
		short part_node = body->parts[part].node_index;

		game_rotation.v.i = -pose->rotation.v.i;
		game_rotation.v.j = -pose->rotation.v.j;
		game_rotation.v.k = -pose->rotation.v.k;
		game_rotation.w = pose->rotation.w;
		matrix4x3_rotation_from_quaternion(&frames[part], &game_rotation);
		frames[part].scale = nodes[part_node].scale;
		frames[part].position = pose->position;
		matrix4x3_inverse(&nodes[part_node], &inverses[part]);
	}
	/* every node carried with its part's, as the game has it from that node */
	for (node = 0; node < body->node_count; node++)
	{
		real_matrix4x3 from_part;

		part = body->node_parts[node];
		if (part < 0 || part >= body->part_count)
		{
			drawn[node] = nodes[node];
		}
		else if (body->parts[part].node_index == node)
		{
			drawn[node] = frames[part];
		}
		else
		{
			matrix4x3_multiply(&inverses[part], &nodes[node], &from_part);
			matrix4x3_multiply(&frames[part], &from_part, &drawn[node]);
		}
	}
	return drawn;
}

long ragdolls_piece_count(
	long object_index)
{
	struct ragdoll const *ragdoll = ragdolls_of_object(object_index);
	long count = 1;
	short part;

	if (!ragdoll || !ragdoll->posed)
		return 1;
	for (part = 1; part < ragdoll->body->part_count; part++)
	{
		if (TEST_FLAG(ragdoll->broken_joints, part))
			count++;
	}
	return count;
}

real_matrix4x3 const *ragdolls_piece_node_matrices(
	long object_index,
	real_matrix4x3 const *nodes,
	long piece)
{
	struct ragdoll const *ragdoll = ragdolls_of_object(object_index);
	struct ragdoll_body const *body;
	real_matrix4x3 const *game;
	short root = 0;
	short part;
	short node;

	if (piece < 0)
		return NULL;
	if (!nodes || !ragdoll || !ragdoll->posed || !ragdoll->broken_joints)
		return piece == 0 ? nodes : NULL;
	body = ragdoll->body;
	/* the piece's top part: the root's, else the piece'th torn one */
	for (part = 1; part < body->part_count && piece > 0; part++)
	{
		if (TEST_FLAG(ragdoll->broken_joints, part) && --piece == 0)
			root = part;
	}
	if (piece > 0)
		return NULL;
	/* (as of the last tick: object_get_node_matrices gives the drawn pose
	while a frame is drawn, render_interpolation.c, the ragdoll's own, and so
	the torn limb where it lies) */
	game = (real_matrix4x3 const *)object_header_block_get(object_index,
		&object_get(object_index)->object.node_matrices);
	for (node = 0; node < body->node_count; node++)
	{
		real_matrix4x3 *drawn = &ragdolls.piece[node];
		short top = body->node_parts[node];

		top = top >= 0 && top < body->part_count ? ragdoll_part_piece(ragdoll, top) : 0;
		*drawn = nodes[node];
		if (top == root)
			continue;
		/* another piece's node: its vertices drawn at one point (the rotation
		kept, for their normals), at this piece's own joint, or where a piece
		beneath this one tore from it, the joint as the game has it from its
		parent's node */
		drawn->scale = 0.0f;
		drawn->position = nodes[body->parts[root].node_index].position;
		while (top > 0)
		{
			short parent = body->parts[top].parent_part_index;
			short above = ragdoll_part_piece(ragdoll, parent);

			if (above == root)
			{
				short parent_node = body->parts[parent].node_index;
				real_matrix4x3 inverse;
				real_point3d joint;

				matrix4x3_inverse(&game[parent_node], &inverse);
				matrix4x3_transform_point(&inverse, &game[body->parts[top].node_index].position, &joint);
				matrix4x3_transform_point(&nodes[parent_node], &joint, &drawn->position);
				break;
			}
			top = above;
		}
	}
	return ragdolls.piece;
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
	ragdolls_impulses_drop(chosen + 1);
	ragdoll = &ragdolls.active[chosen];
	memset(ragdoll, 0, sizeof(*ragdoll));
	ragdoll->used = TRUE;
	ragdoll->object_index = death->object_index;
	ragdoll->body = death->body;
	ragdoll->start_time = game_time_get();
	for (index = 0; index < death->body->part_count; index++)
		ragdoll_pose_from_matrix(&death->nodes[death->body->parts[index].node_index], &ragdoll->poses[index]);
	/* (the living biped took half the killing push, as biped_accelerate
	halves it, which its velocity carries; a corpse takes all of it) */
	if (death->hit)
	{
		ragdolls_push(chosen + 1, &death->killing_hit, 0.5f);
		ragdolls_tear(chosen + 1, &death->killing_hit);
	}
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
	ragdolls_impulses_drop(id);
}

long ragdolls_impulse_count(void)
{
	return ragdolls.impulse_count;
}

struct ragdoll_impulse const *ragdolls_impulse_get(
	long index)
{
	return index >= 0 && index < ragdolls.impulse_count ? &ragdolls.impulses[index] : NULL;
}

void ragdolls_impulses_clear(void)
{
	ragdolls.impulse_count = 0;
}

long ragdolls_break_count(void)
{
	return ragdolls.break_count;
}

struct ragdoll_break const *ragdolls_break_get(
	long index)
{
	return index >= 0 && index < ragdolls.break_count ? &ragdolls.breaks[index] : NULL;
}

void ragdolls_breaks_clear(void)
{
	ragdolls.break_count = 0;
}

boolean ragdolls_near(
	real_point3d const *center,
	real radius)
{
	real reach = radius + RAGDOLL_STAND_IN_DISTANCE;
	long index;

	for (index = 0; index < RAGDOLLS_MAX; index++)
	{
		real_point3d const *at = &ragdolls.active[index].poses[0].position;
		real_vector3d offset;

		if (!ragdolls.active[index].used)
			continue;
		offset.i = at->x - center->x;
		offset.j = at->y - center->y;
		offset.k = at->z - center->z;
		if (offset.i * offset.i + offset.j * offset.j + offset.k * offset.k <= reach * reach)
			return TRUE;
	}
	return FALSE;
}

void ragdolls_stand_ins_begin(void)
{
	long index;

	for (index = 0; index < RAGDOLL_STAND_INS_MAX; index++)
		ragdolls.stand_ins[index].seen = FALSE;
}

long ragdolls_stand_in(
	long object_index,
	boolean *added)
{
	long free_index = NONE;
	long index;

	*added = FALSE;
	for (index = 0; index < RAGDOLL_STAND_INS_MAX; index++)
	{
		if (ragdolls.stand_ins[index].used && ragdolls.stand_ins[index].object_index == object_index)
		{
			ragdolls.stand_ins[index].seen = TRUE;
			return index + 1;
		}
		if (!ragdolls.stand_ins[index].used && free_index == NONE)
			free_index = index;
	}
	if (free_index == NONE)
		return NONE;
	ragdolls.stand_ins[free_index].used = TRUE;
	ragdolls.stand_ins[free_index].seen = TRUE;
	ragdolls.stand_ins[free_index].object_index = object_index;
	*added = TRUE;
	return free_index + 1;
}

boolean ragdolls_stand_in_unseen(
	long id)
{
	if (id < 1 || id > RAGDOLL_STAND_INS_MAX || !ragdolls.stand_ins[id - 1].used || ragdolls.stand_ins[id - 1].seen)
		return FALSE;
	ragdolls.stand_ins[id - 1].used = FALSE;
	return TRUE;
}

void ragdolls_dispose_from_old_map(void)
{
	ragdolls.death_count = 0;
	ragdolls.hit_count = 0;
	ragdolls.impulse_count = 0;
	ragdolls.break_count = 0;
	memset(ragdolls.active, 0, sizeof(ragdolls.active));
	memset(ragdolls.stand_ins, 0, sizeof(ragdolls.stand_ins));
}
