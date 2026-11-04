/*
OBJECT_DEATH.H

Deaths (object_death.c): what a unit of a group does as it dies, from a
mutator's [death] (docs/mutators.md): an effect at it, and its body vanishing.
*/

#ifndef OBJECT_DEATH_H
#define OBJECT_DEATH_H

/* ---------- macros */

/* (unit_died) a unit's death as its mutators' make it: one test of a count
while no mutator has a [death] */
#define object_death(unit_index) \
	do { if (object_death_count > 0) object_death_found(unit_index); } while (0)

/* ---------- prototypes/OBJECT_DEATH.C */

extern short object_death_count;

/* (mutator_tags.c) none: a new game's deaths are made again, a new map's
tags are others */
void object_death_clear(void);
/* (mutator_tags.c) each unit of the group (object_pose_group_tag's names)
plays the effect (NONE for none) as it dies, and its body vanishes if
`vanish`. FALSE (said into reason) if the map has no unit of it */
boolean object_death_add(long group_tag, long effect_index, boolean vanish, char *reason, unsigned long size);
void object_death_found(long unit_index);

#endif
