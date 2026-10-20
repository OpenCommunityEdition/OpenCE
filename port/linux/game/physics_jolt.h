/*
PHYSICS_JOLT.H

The game's side of the physics bridge (port/macos/include/host_physics.h):
the batch it writes each tick for the host's Jolt world, and the poses it
reads back. Ragdolls and debris are for show only; nothing here changes
the game's state.
*/

#ifndef __PHYSICS_JOLT_H
#define __PHYSICS_JOLT_H

struct collision_bsp;

/* whether ragdolls or debris are on (game.ragdolls, game.physics_debris) and
the host has a physics world (the macOS app) */
boolean physics_jolt_enabled(void);
/* whether ragdolls are (game.ragdolls, and the host's world) */
boolean physics_jolt_ragdolls_enabled(void);

/* the level's collision triangles: every vertex of the bsp (x, y, z each, in
world units) and three vertex indices a triangle, facing out of the level
(a two-sided surface twice, once each way); the triangle count, or with
NULL indices the count it would be. Surfaces whose rings are broken or name
vertices the bsp does not have are left out. */
long physics_jolt_level_triangles(struct collision_bsp const *bsp, real *vertices,
	unsigned long *indices, long triangle_capacity);

/* the current bsp's triangles sent to the host when it loads or switches
(scenario.c), and everything removed when the map goes */
void physics_jolt_structure_bsp_reconnect(void);
void physics_jolt_dispose_from_old_map(void);

#endif
