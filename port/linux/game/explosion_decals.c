/*
EXPLOSION_DECALS.C

A decal every explosion leaves, from a mutator's [explosion]
(docs/mutators.md): placed on what the explosion hit (back along its effect's
facing, which is out of the surface), and on the ground below one that does
not face up (off a wall, or in the air), its radius the explosion's damage's
(the damage effect's cutoff radius), whatever weapon made it. An explosion is
an effect's damage part (effects.c), which every machine plays, so each
machine leaves the decal itself, as it leaves the effect's own.
*/

#include "cseries.h"
#include "effects/decals.h"
#include "game/game_globals.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"
#include "objects/damage_effect_definitions.h"

#include "explosion_decals.h"

/* ---------- constants */

/* how much farther than the explosion's radius the surface it hit is looked
for */
#define EXPLOSION_DECAL_REACH 0.5f
/* the least upward an explosion faces (out of the ground) to leave no decal
below it as well */
#define EXPLOSION_DECAL_UP 0.7f

/* ---------- globals */

short explosion_decal_count;

static struct
{
	long decal_index;
	real decal_radius;
} explosion_decal_globals;

/* ---------- public code */

void explosion_decal_clear(
	void)
{
	explosion_decal_count = 0;
}

void explosion_decal_add(
	long decal_index,
	real decal_radius)
{
	explosion_decal_globals.decal_index = decal_index;
	explosion_decal_globals.decal_radius = decal_radius;
	explosion_decal_count = 1;
}

void explosion_decal_found(
	long damage_definition_index,
	real_point3d const *point,
	real_vector3d const *forward)
{
	struct damage_effect_definition const *damage = damage_effect_definition_get(damage_definition_index);
	real_vector3d toward;
	real reach;

	/* (an explosion's blast: damage done around it, not a shock wave's push
	alone, which reaches farther) */
	if (damage->cutoff_radius <= 0.f || damage->damage.damage_upper_bound <= 0.f ||
		explosion_decal_globals.decal_radius <= 0.f)
	{
		return;
	}
	reach = damage->cutoff_radius + EXPLOSION_DECAL_REACH;
	toward.i = -forward->i * reach;
	toward.j = -forward->j * reach;
	toward.k = -forward->k * reach;
	decal_new(explosion_decal_globals.decal_index, point, &toward,
		damage->cutoff_radius / explosion_decal_globals.decal_radius, FALSE, NONE, NULL);
	/* (and on the ground below one not facing up, off a wall or in the air) */
	if (forward->k < EXPLOSION_DECAL_UP)
	{
		toward.i = toward.j = 0.f;
		toward.k = -reach;
		decal_new(explosion_decal_globals.decal_index, point, &toward,
			damage->cutoff_radius / explosion_decal_globals.decal_radius, FALSE, NONE, NULL);
	}
}
