/*
RAGDOLL_PHYSICS.H

The ragdolls' physics engine, Jolt Physics (port/third_party/JoltPhysics,
github.com/jrouwe/JoltPhysics, MIT licence), behind a C interface for
ragdoll.c (port/linux/physics/ragdoll_physics.cpp).

One physics world holds every ragdoll (a slot each, as ragdoll.c numbers
them) and, for each, the level around it: static triangle meshes made from
the game's own collision (the structure's, cached, and the objects', each
tick).

Everything here is in the game's units: world units (1 = 10 feet = 3.048 m)
and ticks (1/30 s); the engine works in metres and seconds inside.
*/

#ifndef RAGDOLL_PHYSICS_H
#define RAGDOLL_PHYSICS_H

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	RAGDOLL_PHYSICS_MAXIMUM_RAGDOLLS = 32,
	RAGDOLL_PHYSICS_MAXIMUM_BONES = 64,
	RAGDOLL_PHYSICS_ENVIRONMENT_LAYERS = 2, /* the structure's, the objects' */
};

enum ragdoll_physics_joint
{
	_ragdoll_physics_joint_none = 0, /* the root */
	_ragdoll_physics_joint_cone, /* swing and twist, from the pose it is made in */
	_ragdoll_physics_joint_hinge, /* a knee or elbow */
};

struct ragdoll_physics_bone
{
	int parent; /* a bone before it, -1 for the root */
	float position[3]; /* the body's origin (its node), world units */
	float rotation[4]; /* the node's frame (forward, left, up) as a quaternion: i, j, k, w */
	/* its shape, in the node's frame: a capsule from start to end (the same:
	a sphere) */
	float capsule_start[3];
	float capsule_end[3];
	float radius;
	/* a second capsule (a foot), when radius2 > 0 */
	float capsule2_start[3];
	float capsule2_end[3];
	float radius2;
	/* its joint to its parent, at its origin */
	int joint;
	float joint_axis[3]; /* (world) a cone's twist axis (along the bone), a hinge's axis */
	float joint_normal[3]; /* (world) square to the axis */
	float swing_limit; /* a cone: radians either way from the pose it is made in */
	float twist_limit;
	float hinge_minimum; /* a hinge: radians from the pose it is made in */
	float hinge_maximum;
	float friction_scale; /* times the ragdoll's joint friction, at this joint */
	float mass_share; /* its share of the ragdoll's mass; 0 (every bone's): by its volume */
	float velocity[3]; /* at the start, world units a tick (added to the settings') */
	float angular_velocity[3]; /* radians a tick */
};

struct ragdoll_physics_settings
{
	float mass; /* the whole body's, kilograms */
	float friction;
	float restitution;
	float linear_damping; /* a second */
	float angular_damping;
	float velocity[3]; /* every bone's at the start, world units a tick */
	int self_collision; /* its limbs collide with each other */
	/* how hard a joint resists turning, as a share of the torque the limbs
	beyond it weigh on it (held out level): 1 holds them out */
	float joint_friction;
	float gravity; /* world units a tick a tick: what the limbs weigh */
};

/* 1 once the engine runs (it starts at the first call) */
int ragdoll_physics_initialize(void);
/* every ragdoll and every environment gone (a new map) */
void ragdoll_physics_reset(void);

int ragdoll_physics_create(int slot, struct ragdoll_physics_bone const *bones, int bone_count,
	struct ragdoll_physics_settings const *settings);
void ragdoll_physics_destroy(int slot);
int ragdoll_physics_exists(int slot);

/* the level around a ragdoll, one layer of it: triangles of 3 points of 3
coordinates (world units; counter-clockwise seen from outside), or none */
void ragdoll_physics_set_environment(int slot, int layer, float const *triangles, int triangle_count);

/* a tick for everything: collision_steps steps of 1/30 / collision_steps
seconds, gravity in world units a tick a tick */
void ragdoll_physics_step(int collision_steps, float gravity);

/* a bone as it is: its origin and its frame */
void ragdoll_physics_get_bone(int slot, int bone, float position[3], float rotation[4]);
void ragdoll_physics_get_bone_velocity(int slot, int bone, float velocity[3]);
/* moving or not (asleep) */
int ragdoll_physics_active(int slot);
/* a hit: the momentum of the whole body at velocity (world units a tick),
share of it given to the bone at the point, the rest to the whole body */
void ragdoll_physics_hit(int slot, int bone, float const point[3], float const velocity[3], float share);
/* every bone faster by velocity */
void ragdoll_physics_add_velocity(int slot, float const velocity[3]);
/* the whole body spun about its centre of mass (a blow that knocks it head
over heels): spin, radians a tick about its axis */
void ragdoll_physics_add_spin(int slot, float const spin[3]);
/* the whole ragdoll moved by offset, as it is */
void ragdoll_physics_translate(int slot, float const offset[3]);
/* its world bounds: centre and radius */
void ragdoll_physics_bounds(int slot, float center[3], float *radius);

/* a body left sitting in a seat: the bone (its pelvis) held where the seat
has it, moving there over the next tick; the rest hangs from it, limp */
void ragdoll_physics_pin(int slot, int bone, float const position[3], float const rotation[4]);
/* a bone eased toward where a pose has it (strength of the way there each
tick, 0 to 1), moving at base_velocity (world units a tick) besides: a body
sitting limp holds its pose, swaying with what jolts it */
void ragdoll_physics_drive(int slot, int bone, float const position[3], float const rotation[4],
	float const base_velocity[3], float strength);
/* ... let go: the bone falls with the rest */
void ragdoll_physics_unpin(int slot, int bone);
/* the vehicle (its key) the ragdoll passes through: the one it sits in, and
for a moment once thrown from it; -1: none */
void ragdoll_physics_ignore_vehicle(int slot, long key);

/* the vehicles near the ragdolls, for a tick: given between _begin and _end,
each as the convex hull of its collision's points (in its own frame, world
units), where it is and how it moves (world units and radians a tick). Jolt
moves them as kinematic bodies, which nothing stops: a ragdoll they meet is
thrown, or rolls over them. Those not given in a tick are gone */
void ragdoll_physics_vehicles_begin(void);
void ragdoll_physics_vehicle(long key, float const *points, int point_count, float const position[3],
	float const rotation[4], float const velocity[3], float const angular_velocity[3]);
void ragdoll_physics_vehicles_end(void);

/* a vehicle's surfaces (triangles in its frame, as its points are), for the
dead sitting in it, who meet them rather than its hull: made once; 1 when
it has them, or is not given this tick */
int ragdoll_physics_vehicle_has_mesh(long key);
void ragdoll_physics_vehicle_mesh(long key, float const *triangles, int triangle_count);
/* a seat for a ragdoll sitting dead in a vehicle, its own alone: a cushion
(half sizes, about the seat's origin) and a back (its centre and half sizes,
in the seat's frame), where the seat is now and moving at velocity (world
units a tick); made at the first call, moved at each */
void ragdoll_physics_seat(int slot, float const position[3], float const rotation[4],
	float const velocity[3], float const cushion_half[3], float const back_center[3], float const back_half[3]);
void ragdoll_physics_seat_remove(int slot);

/* the vehicle (its key) whose surfaces the ragdoll meets: the one it sits
in; -1: none */
void ragdoll_physics_seat_vehicle(int slot, long key);

/* a vehicle meeting a ragdoll in the last step (one a pair, the hardest) */
struct ragdoll_physics_contact
{
	int slot; /* the ragdoll's */
	long vehicle; /* its key */
	float point[3]; /* on the vehicle, world units */
	float normal[3]; /* out of the vehicle, toward the body */
	float speed; /* the two's speed against each other there, world units a tick */
};

/* the contacts of the last step, up to maximum; their count */
int ragdoll_physics_vehicle_contacts(struct ragdoll_physics_contact *contacts, int maximum);

#ifdef __cplusplus
}
#endif

#endif
