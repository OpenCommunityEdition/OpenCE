/*
PHYSICS_DEBRIS.H

Debris in the host's Jolt world (game.physics_debris): a particle that uses
point physics becomes a small rigid body when it first strikes the level, and
a piece of garbage (what a destroyed object leaves) becomes one as it
appears. Each tumbles there and comes to rest; the particle takes its
position from the body, and the garbage is drawn as the body lies. For show
only: the particles' and the objects' lives are the game's, and a body goes
when its particle or object does. Past the cap (game.physics_debris_max) the
oldest piece is handed back to the game for good.
*/

#ifndef __PHYSICS_DEBRIS_H
#define __PHYSICS_DEBRIS_H

#include "ragdolls.h"

/* the host's body ids for debris: those after the ragdolls' stand-ins, up
to the host's last */
#define PHYSICS_DEBRIS_FIRST_ID 17
#define PHYSICS_DEBRIS_MAX 2032

/* a body as it is sent to the host: a shape (HOST_PHYSICS_SHAPE_*) of a size
in world units, a mass in kilograms, its material, its pose and its
velocities (units a second, radians a second) */
struct physics_debris_body
{
	unsigned long shape;
	real size[3];
	real mass;
	real friction;
	real restitution;
	struct ragdoll_pose pose;
	real_vector3d velocity;
	real_vector3d angular_velocity;
};

struct point_physics_definition;

/* a particle's body from its radius, its point physics and its velocity
after the bounce, at its position: a capsule along the way it flies, as heavy
as its density makes it; FALSE for a particle lighter than air (smoke, a
flash) */
boolean physics_debris_particle_body(real radius, struct point_physics_definition const *physics,
	real_point3d const *position, real_vector3d const *velocity, struct physics_debris_body *body);

/* (particles.c) a particle struck the level; with debris on it becomes a
body, unless it is one or was handed back */
void physics_debris_particle_collided(long particle_index);
/* (particles.c) a particle that is a body takes its position from the
host's pose, and is settled once the body sleeps; FALSE for any other */
boolean physics_debris_particle_update(long particle_index, real dt, boolean *settled);
/* whether a particle is a body now (debug.network_test_debris) */
boolean physics_debris_particle_is_body(long particle_index);
/* (physics_jolt.c, each tick) new pieces of garbage made bodies */
void physics_debris_garbage_update(void);
/* the node matrices a piece of garbage is drawn with: its body's, else
`nodes` (ragdolls_node_matrices) */
real_matrix4x3 const *physics_debris_node_matrices(long object_index, real_matrix4x3 const *nodes);

/* (physics_jolt.c, each tick, for each debris id in order) whether the
host's body goes now: its piece gone, at rest, or another taking its id */
boolean physics_debris_remove(long id);
/* then the body to add for an id, made live; NULL if none is new */
struct physics_debris_body const *physics_debris_add(long id);
/* then each body's pose from the results */
void physics_debris_result(long id, boolean awake, struct ragdoll_pose const *pose);
/* every piece forgotten (the map goes, a saved game loads) */
void physics_debris_dispose_from_old_map(void);

/* (debug.physics_draw) the body of an id as the host last posed it, and
whether it is awake; FALSE for an id with no live, posed body */
boolean physics_debris_posed_body(long id, struct physics_debris_body *body, boolean *awake);

#endif
