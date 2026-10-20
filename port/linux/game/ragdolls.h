/*
RAGDOLLS.H

Dead bipeds as ragdolls in the host's Jolt world (physics_jolt.c), for show
only (game.ragdolls). The game's damage and deaths are recorded here as
they happen: each hit on a biped, and each biped that dies with its pose
and the hit that killed it, queued for the physics' next tick.
*/

#ifndef __RAGDOLLS_H
#define __RAGDOLLS_H

#include "ragdoll_bodies.h"

struct damage_data;

enum
{
	/* the deaths queued in a tick; past it a biped dies as the stock game's */
	RAGDOLL_DEATHS_MAX = 16,
	/* the hits remembered, the newest overwriting the oldest */
	RAGDOLL_HITS_MAX = 64,
};

/* a hit on a biped: the damage's direction (unit length), the speed its
instantaneous acceleration gives (world units a second, scaled as the
damage is), and where it struck */
struct ragdoll_hit
{
	long object_index;
	long time;
	real_vector3d direction;
	real speed;
	real_point3d point;
};

struct ragdoll_death
{
	long object_index;
	long definition_index;
	struct ragdoll_body const *body;
	/* the biped's node matrices and velocity as it died */
	real_matrix4x3 nodes[RAGDOLL_BODY_NODES_MAX];
	real_vector3d velocity;
	/* the hit that killed it, if it was hit that tick */
	boolean hit;
	struct ragdoll_hit killing_hit;
};

/* whether a biped dies as a ragdoll (game.ragdolls, and the host's physics) */
boolean ragdolls_enabled(void);

/* the game's hooks (halo_linux_source_fixups.h): damage done to an object
(damage.c, object_damage_aftermath), and a unit dying, not feigning
(units.c, unit_died) */
void ragdolls_damage(long object_index, struct damage_data const *damage);
void ragdolls_unit_died(long unit_index);

/* the deaths queued since the last clear, oldest first */
long ragdolls_death_count(void);
struct ragdoll_death const *ragdolls_death_get(long index);
void ragdolls_deaths_clear(void);

/* the hit on an object this tick, newest first, or NULL */
struct ragdoll_hit const *ragdolls_hit_get(long object_index);

void ragdolls_dispose_from_old_map(void);

#endif
