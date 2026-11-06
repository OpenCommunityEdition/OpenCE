/*
EXPLOSION_DECALS.H

Explosion decals (explosion_decals.c): a decal an explosion leaves, as big as
its damage reaches, from a mutator's [explosion] (docs/mutators.md).
*/

#ifndef EXPLOSION_DECALS_H
#define EXPLOSION_DECALS_H

/* ---------- macros */

/* (an effect's damage part, effects.c) the decal of an explosion of the
damage at the point, facing out of what it hit: one test of a count while no
mutator has an [explosion] */
#define explosion_decal(damage_definition_index, point, forward) \
	do { if (explosion_decal_count > 0) explosion_decal_found((damage_definition_index), (point), (forward)); } \
	while (0)

/* ---------- prototypes/EXPLOSION_DECALS.C */

extern short explosion_decal_count;

/* (mutator_tags.c) none: a new game's are made again */
void explosion_decal_clear(void);
/* (mutator_tags.c) every explosion leaves the decal, whose largest radius is
decal_radius, at the radius its damage reaches */
void explosion_decal_add(long decal_index, real decal_radius);
void explosion_decal_found(long damage_definition_index, real_point3d const *point, real_vector3d const *forward);

#endif
