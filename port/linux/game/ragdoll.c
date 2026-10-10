/*
RAGDOLL.C

Dynamic ragdolls for dead bipeds, simulated by Jolt Physics
(github.com/jrouwe/JoltPhysics, MIT licence; ragdoll_physics.h). The Xbox
game plays a death animation and then, for a few bipeds, relaxes its last
frame onto the ground (biped_limp_noodle.c). Here a biped that dies becomes
a ragdoll at once: its skeleton, in the pose it died in, falls, tumbles and
slides under gravity, thrown by the hit that killed it and pushed by every
later bullet and explosion, against the level, the objects in it and the
other bodies, until it comes to rest.

This file is the game's side: which bipeds become ragdolls, their skeletons
made into Jolt ragdolls, the level around each given to Jolt, hits passed on,
and the bodies Jolt moves written back into the bipeds' node matrices.

- Each bone of the model's skeleton longer than ragdoll_min_bone is a rigid
  body: a capsule along the bone (or a ball, a head or a hand), as thick as
  its name says (3ds Max biped names, as every Halo biped has) for the
  skeleton's size. A shorter one (a finger, a mandible) rides on its parent.
- Each joint turns from the pose the biped died in no further than its
  limit, by its name: the spine little, the neck more, the hips and
  shoulders far (Jolt's swing-twist constraints). A knee or elbow ("calf",
  "forearm", "horselink") is a hinge: about one axis, one way, from straight
  to its limit. The joints have friction (muscle and sinew).
- The level: the game's own collision surfaces near each body (its
  structure, cached while the body stays in them, and its scenery, machines
  and vehicles, each tick), as triangle meshes.
- The node matrices are written straight from the bodies, and the game no
  longer computes them from animation for the body
  (_object_do_not_recompute_node_matrices_bit, as limp bodies do).

A body Jolt puts to sleep (or simulated for ragdoll_max_seconds) freezes: it
frees its slot and keeps its pose, and a later hit wakes it. Every machine
simulates its own bodies: nothing goes over the network, and a body's
position from the host moves the whole ragdoll there. A saved game holds the
frozen pose (the node matrices and the biped's ragdoll bit); a ragdoll whose
body changed under it (a revert, a teleport) starts again from the body.

Settings: config.toml's game.death_physics, "ragdoll" or "classic" (Video
Settings' Death Physics), and the console's `ragdoll` command (`ragdoll`
alone lists the parameters; `ragdoll off`, `ragdoll on`, `ragdoll <name>
<value>`).

Units: world units (1 = 10 feet = 3.048 m) and ticks (1/30 s).
*/

#include "cseries.h"
#include "math/real_math.h"
#include "models/model_definitions.h"
#include "objects/objects.h"
#include "objects/damage.h"
#include "physics/collision_features.h"
#include "physics/collision_usage.h"
#include "physics/collisions.h"
#include "physics/physics.h"
#include "units/biped_definitions.h"
#include "units/bipeds.h"
#include "units/unit_definitions.h"
#include "units/units.h"
#include "render/render_debug.h"
#include "game/game.h"
#include "main/console.h"
#include "models/model_animation_definitions.h"
#include "sound/game_sound.h"
#include "tag_files/tag_files.h"

#include "ragdoll.h"
/* (dynamic blood, port/linux/game/blood.c, when the build has it: a body
struck by a vehicle bleeds onto it) */
#if __has_include("blood.h")
#include "blood.h"
#endif

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#if RAGDOLLS

#include "ragdoll_physics.h"

/* the port's settings (port/linux/src/port_config.c) */
char const *config_string(char const *name);
int config_write(char const *name, char const *value);
unsigned long config_changes(void);
/* object_bounds_cache.c: the copy of the bounds collision reads */
void object_bounds_cache_update(long object_index, real_point3d const *center, real radius);

/* ---------- constants */

enum
{
	RAGDOLL_MAXIMUM_ACTIVE = RAGDOLL_PHYSICS_MAXIMUM_RAGDOLLS,
	RAGDOLL_MAXIMUM_NODES = MAXIMUM_NODES_PER_MODEL,
	RAGDOLL_MAXIMUM_PENDING_HITS = 32,
	RAGDOLL_MAXIMUM_TRIANGLES = MAXIMUM_COLLISION_FEATURES_PER_TEST * (MAXIMUM_POINTS_PER_COLLISION_PRISM - 2),
};

enum
{
	_ragdoll_environment_structure = 0,
	_ragdoll_environment_objects,
};

/* the fastest a hit makes a body move (world units a tick: 15 a second,
faster than anything throws a body) */
#define RAGDOLL_MAXIMUM_SPEED 0.5f
/* the most a bone keeps of its death animation's motion: world units a tick
(1.5 m/s) and radians a tick (3 a second) */
#define RAGDOLL_ANIMATION_SPEED 0.016f
#define RAGDOLL_ANIMATION_SPIN 0.1f
/* a human's skeleton's longest chain of bones (the pelvis to a hand),
world units: the size the bones' thicknesses and the mass are given for */
#define RAGDOLL_HUMAN_SIZE 0.394f
#define RAGDOLL_METRES 3.048f

/* ---------- structures */

struct ragdoll_parameters
{
	real gravity_scale;
	real collision_steps;
	real friction;
	real bounce;
	real air_drag;
	real spin_drag;
	real joint_friction;
	real mass;
	real thickness;
	real self_collision;
	real bullet_impulse;
	real bullet_impulse_maximum;
	real melee_impulse;
	real sniper_impulse;
	real headshot_share;
	real headshot_spin;
	real grunt_tank;
	real death_animation_whole;
	real explosion_scale;
	real death_impulse_scale;
	real hit_share;
	real death_animation;
	real explosion_flight;
	real down_height;
	real animation_momentum;
	real knee_buckle;
	real sleep_distance;
	real sleep_seconds;
	real maximum_seconds;
	real maximum_active;
	real minimum_bone;
	real halo2_skeleton;
	real hip_limit;
	real shoulder_limit;
	real neck_limit;
	real neck_twist;
	real twist_limit;
	real knee_limit;
	real elbow_limit;
	real hyperextend;
	real spine_limit;
	real joint_limit;
	real seated_share;
	real seated_crash;
	real seated_surfaces;
	real seated_hold;
	real debug;
};

struct ragdoll_parameter_definition
{
	char const *name;
	real *value;
	real default_value;
	real minimum;
	real maximum;
	char const *description;
};

struct ragdoll_quaternion
{
	real i;
	real j;
	real k;
	real w;
};

struct ragdoll_node
{
	short parent; /* the model's */
	short body; /* its body, NONE when it rides on its parent */
	short simulated_parent; /* the nearest ancestor with a body */
	short pad;
	real scale;
	real_point3d position; /* the result: where the node is */
	real_vector3d frame[3]; /* and its forward, left, up */
	real_vector3d attach_offset; /* riding: its place in its parent's frame */
	real_vector3d attach_frame[3]; /* riding: its axes in its parent's frame */
};

struct ragdoll_body
{
	short node;
	short parent_body; /* NONE for the root */
	short joint;
	short pad;
	real radius;
	real_vector3d capsule_start; /* its shape, in the node's frame */
	real_vector3d capsule_end;
	real foot_radius; /* a second capsule, a foot's (0: none) */
	real_vector3d foot_start;
	real_vector3d foot_end;
};

struct ragdoll
{
	long object_index; /* NONE: a free slot */
	long start_time;
	long last_update_time;
	long ticks;
	long still_ticks;
	short node_count;
	short body_count;
	short head_body; /* its head's body, NONE: none */
	short pad_head;
	/* its thud on the ground: the death animation's own (not yet played), or
	NONE; when it last thudded; the torso's speed last tick */
	long thud_sound;
	long last_thud_time;
	boolean last_velocity_valid;
	byte pad_thud[3];
	real_vector3d last_velocity;
	short order[RAGDOLL_MAXIMUM_NODES]; /* parents before children */
	real_point3d written_object_position;
	real_point3d written_root_position;
	real_vector3d front_local; /* the body's front, in the root's frame */
	real_point3d still_anchor[RAGDOLL_MAXIMUM_NODES]; /* where it was when it last moved */
	/* the structure Jolt has around it: where it was gathered, how far */
	boolean structure_valid;
	boolean objects_present;
	byte pad[2];
	real_point3d structure_center;
	real structure_radius;
	struct ragdoll_node nodes[RAGDOLL_MAXIMUM_NODES];
	struct ragdoll_body bodies[RAGDOLL_MAXIMUM_NODES]; /* parents before children */
};

struct ragdoll_pending_hit
{
	long object_index; /* NONE: free */
	long time;
	boolean has_point;
	byte pad;
	short category; /* the damage's (RAGDOLL_DAMAGE_CATEGORY_...) */
	real_point3d point;
	real_vector3d point_velocity; /* the whole body's, given at the point, world units a tick */
	real_vector3d body_velocity; /* to every bone (an explosion's, as the game gives it) */
	/* the body's own velocity before the game pushed it (biped_accelerate,
	right after this hit is noted) */
	real_vector3d velocity_before;
};

/* a biped playing its death animation until it is down, then limp: its
skeleton as the animation last left it, and how it stood when it died */
struct ragdoll_dying
{
	long object_index; /* NONE: free */
	long death_time;
	long last_time;
	short node_count;
	short head_node; /* the highest node as it died (its head) */
	short still_ticks; /* ticks the animation has not moved the skeleton */
	boolean flying; /* thrown by an explosion, in the game's own flight */
	boolean was_airborne;
	boolean against_wall; /* gone limp falling against a wall */
	boolean whole; /* its animation plays to its end (a share of deaths) */
	byte pad2[2];
	real floor_z; /* the ground under it as it died (the object's origin) */
	real head_height; /* its head above that */
	real_point3d anchor; /* its pelvis as it died: inside the body, clear of walls */
	real_vector3d last_velocity; /* the body's, last tick */
	real_point3d positions[RAGDOLL_MAXIMUM_NODES];
	real_vector3d frames[RAGDOLL_MAXIMUM_NODES][3];
};

enum
{
	RAGDOLL_MAXIMUM_DYING = 32,
};

/* ---------- prototypes */

static struct ragdoll *ragdoll_find(long object_index);
static int ragdoll_slot(struct ragdoll const *ragdoll);
static struct ragdoll *ragdoll_allocate(long object_index);
static void ragdoll_free(struct ragdoll *ragdoll);
static boolean ragdoll_eligible(long biped_index);
struct ragdoll_dying;
static boolean ragdoll_seed(struct ragdoll *ragdoll, long biped_index, struct ragdoll_dying const *motion);
static void ragdoll_update(struct ragdoll *ragdoll, long biped_index);
static void ragdoll_freeze(struct ragdoll *ragdoll, long biped_index);
static short ragdoll_active_count(void);
static void ragdoll_write(struct ragdoll *ragdoll, long biped_index);
static void ragdoll_apply_pending_hits(struct ragdoll *ragdoll);
static void ragdoll_thud_prepare(struct ragdoll *ragdoll, long biped_index, boolean dying);
static void ragdoll_apply_hit(struct ragdoll *ragdoll, struct ragdoll_pending_hit const *hit);
static void ragdolls_step(void);
static boolean ragdolls_enabled(void);

/* ---------- globals */

static struct ragdoll_parameters ragdoll_parameters;

static struct ragdoll_parameter_definition const ragdoll_parameter_definitions[] =
{
	{ "gravity_scale", &ragdoll_parameters.gravity_scale, 1.5f, 0.f, 4.f, "times the game's gravity (more: heavier, faster falls)" },
	{ "collision_steps", &ragdoll_parameters.collision_steps, 4.f, 1.f, 8.f, "Jolt's steps a 30 Hz tick (more: stiffer, exact at speed, slower)" },
	{ "friction", &ragdoll_parameters.friction, 0.8f, 0.f, 2.f, "how bodies grip the ground and each other" },
	{ "bounce", &ragdoll_parameters.bounce, 0.f, 0.f, 1.f, "restitution: 0 bodies land dead, 1 they bounce" },
	{ "air_drag", &ragdoll_parameters.air_drag, 0.05f, 0.f, 10.f, "speed lost to the air, a second (0.05: 5%)" },
	{ "spin_drag", &ragdoll_parameters.spin_drag, 0.8f, 0.f, 20.f, "spin lost to the air, a second" },
	{ "joint_friction", &ragdoll_parameters.joint_friction, 0.2f, 0.f, 2.f, "how hard a joint resists turning, as a share of the weight of the limb beyond it (more: stiffer, the body moves as one; less: floppier)" },
	{ "mass", &ragdoll_parameters.mass, 80.f, 5.f, 1000.f, "kilograms, of a human-sized body (bigger and smaller bodies by their size)" },
	{ "thickness", &ragdoll_parameters.thickness, 1.f, 0.25f, 3.f, "times the bones' thickness against the level and each other" },
	{ "self_collision", &ragdoll_parameters.self_collision, 1.f, 0.f, 1.f, "1: a body's limbs collide with each other" },
	{ "bullet_impulse", &ragdoll_parameters.bullet_impulse, 0.0008f, 0.f, 0.05f, "world units a tick the whole body would gain from a point of damage (given at the hit)" },
	{ "bullet_impulse_max", &ragdoll_parameters.bullet_impulse_maximum, 0.03f, 0.f, 0.5f, "most one hit gives the whole body, world units a tick" },
	{ "melee_impulse", &ragdoll_parameters.melee_impulse, 0.07f, 0.f, 0.5f, "world units a tick a melee blow gives the whole body (a Spartan's: bodies roll and tumble), the way it was struck" },
	{ "sniper_impulse", &ragdoll_parameters.sniper_impulse, 0.06f, 0.f, 0.5f, "world units a tick a sniper round gives the whole body" },
	{ "headshot_share", &ragdoll_parameters.headshot_share, 0.35f, 0.f, 1.f, "how much of a sniper round in the head goes into the head (it snaps back)" },
	{ "headshot_spin", &ragdoll_parameters.headshot_spin, 2.5f, 0.f, 30.f, "radians a second a sniper headshot turns the body over backward (2.5: over onto its back, legs up; 3 and more: a full back flip)" },
	{ "grunt_tank", &ragdoll_parameters.grunt_tank, 1.f, 0.f, 2.f, "a Grunt's methane tank's size on its back (0: none)" },
	{ "death_animation_whole", &ragdoll_parameters.death_animation_whole, 0.5f, 0.f, 1.f, "the share of deaths whose animation plays to its end (only a melee blow, an explosion, or the body meeting a wall or object ends it sooner)" },
	{ "explosion_scale", &ragdoll_parameters.explosion_scale, 1.1f, 0.f, 5.f, "times a damage effect's own acceleration" },
	{ "death_impulse_scale", &ragdoll_parameters.death_impulse_scale, 1.5f, 0.f, 5.f, "times the push of the killing blow" },
	{ "hit_share", &ragdoll_parameters.hit_share, 0.35f, 0.f, 1.f, "how much of a hit goes into the bone it hit (the rest moves the whole body)" },
	{ "death_animation", &ragdoll_parameters.death_animation, 3.f, 0.f, 10.f, "the most seconds the death animation plays before the body goes limp (it goes limp sooner once it is down; at once when thrown, in the air, falling or shot again; 0: limp at once always)" },
	{ "explosion_flight", &ragdoll_parameters.explosion_flight, 0.5f, 0.f, 1.f, "the share of explosion deaths that fly in the game's own thrown animation until anything touches them (a surface, a bullet, another blast), then go limp; the rest go limp at once" },
	{ "down_height", &ragdoll_parameters.down_height, 0.2f, 0.f, 1.f, "the body is down (the ragdoll takes it) once its head is below this share of the height it stood at" },
	{ "sleep_distance", &ragdoll_parameters.sleep_distance, 0.01f, 0.f, 0.2f, "world units no joint may move for the body to count as at rest" },
	{ "sleep_seconds", &ragdoll_parameters.sleep_seconds, 1.f, 0.1f, 10.f, "seconds a body barely moving takes to freeze (Jolt also puts bodies to sleep itself)" },
	{ "animation_momentum", &ragdoll_parameters.animation_momentum, 0.6f, 0.f, 1.f, "how much of the death animation's motion the body keeps when it goes limp" },
	{ "knee_buckle", &ragdoll_parameters.knee_buckle, 2.5f, 0.f, 20.f, "radians a second a still body's knees start bending at (so it crumples, not stands)" },
	{ "max_seconds", &ragdoll_parameters.maximum_seconds, 15.f, 1.f, 120.f, "seconds a body is simulated at most" },
	{ "max_active", &ragdoll_parameters.maximum_active, 16.f, 1.f, (real)RAGDOLL_MAXIMUM_ACTIVE, "bodies simulated at once (the oldest freezes)" },
	{ "min_bone", &ragdoll_parameters.minimum_bone, 0.02f, 0.f, 0.2f, "nodes nearer their parent ride on it, world units" },
	{ "halo2_skeleton", &ragdoll_parameters.halo2_skeleton, 1.f, 0.f, 1.f, "1: Halo 2's ten bodies (torso, head, upper arms, forearms, thighs, calves; the rest rides on them); 0: every bone" },
	{ "hip_limit", &ragdoll_parameters.hip_limit, 40.f, 0.f, 180.f, "degrees a thigh swings from the death pose (Halo 2: 30 to 45)" },
	{ "shoulder_limit", &ragdoll_parameters.shoulder_limit, 60.f, 0.f, 180.f, "degrees an upper arm swings (Halo 2: 60)" },
	{ "neck_limit", &ragdoll_parameters.neck_limit, 45.f, 0.f, 180.f, "degrees the head swings (Halo 2: 35 to 50)" },
	{ "neck_twist", &ragdoll_parameters.neck_twist, 40.f, 0.f, 180.f, "degrees the head twists (Halo 2: 45)" },
	{ "twist_limit", &ragdoll_parameters.twist_limit, 5.f, 0.f, 180.f, "degrees an arm or leg twists about itself (Halo 2: 5 to 15)" },
	{ "knee_limit", &ragdoll_parameters.knee_limit, 80.f, 0.f, 180.f, "degrees a knee bends from straight (Halo 2: 70 to 80)" },
	{ "elbow_limit", &ragdoll_parameters.elbow_limit, 85.f, 0.f, 180.f, "degrees an elbow bends from straight (Halo 2: 70 to 85)" },
	{ "hyperextend", &ragdoll_parameters.hyperextend, 7.5f, 0.f, 90.f, "degrees a knee or elbow bends past straight (Halo 2: 7.5)" },
	{ "spine_limit", &ragdoll_parameters.spine_limit, 20.f, 0.f, 180.f, "every bone (halo2_skeleton 0): degrees a spine or clavicle joint turns" },
	{ "joint_limit", &ragdoll_parameters.joint_limit, 35.f, 0.f, 180.f, "every bone (halo2_skeleton 0): degrees any other joint turns" },
	{ "seated_share", &ragdoll_parameters.seated_share, 0.5f, 0.f, 1.f, "the share of bodies killed in a vehicle's seat that stay there, limp (the seat stays free: anyone getting in throws the body out)" },
	{ "seated_crash", &ragdoll_parameters.seated_crash, 0.06f, 0.f, 1.f, "world units a tick a vehicle's speed changes by in one tick (a crash) that throws its seated dead out" },
	{ "seated_surfaces", &ragdoll_parameters.seated_surfaces, 0.f, 0.f, 1.f, "1: the seated dead meet their vehicle's own surfaces too (some vehicles' are solid where the seats are, and throw them out); 0: their seat alone" },
	{ "debug", &ragdoll_parameters.debug, 0.f, 0.f, 1.f, "1: draw the ragdolls' bones (with debug_objects 1)" },
};

static boolean ragdoll_parameters_initialized = FALSE;
static struct ragdoll ragdolls[RAGDOLL_MAXIMUM_ACTIVE];
static struct ragdoll_pending_hit ragdoll_pending_hits[RAGDOLL_MAXIMUM_PENDING_HITS];
static struct ragdoll_dying ragdoll_dying[RAGDOLL_MAXIMUM_DYING];
static struct collision_feature_list ragdoll_features;
static real ragdoll_triangles[RAGDOLL_MAXIMUM_TRIANGLES * 9];
static long ragdoll_step_time = NONE;
static boolean ragdoll_freezing_at_rest = FALSE;

/* bodies frozen while still moving (in the air: their slot given to a newer
body, or the frame rate fell so far the game skipped them): they fall on as
soon as a slot is free, from where they stopped, at the speed they had */
enum { RAGDOLL_MAXIMUM_UNSETTLED = 64, RAGDOLL_MAXIMUM_UNSETTLED_WAKES = 4 };
static struct
{
	long object_index; /* NONE: free */
	short wakes;
	boolean pending; /* frozen, to fall on */
	byte pad;
	real_vector3d velocity;
} ragdoll_unsettled[RAGDOLL_MAXIMUM_UNSETTLED];
static unsigned long ragdoll_config_changes = (unsigned long)-1;
static boolean ragdoll_config_enabled = TRUE;
static boolean ragdoll_physics_failed = FALSE;

/* ---------- vectors */

static real ragdoll_dot(real_vector3d const *a, real_vector3d const *b)
{
	return a->i * b->i + a->j * b->j + a->k * b->k;
}

static void ragdoll_cross(real_vector3d const *a, real_vector3d const *b, real_vector3d *result)
{
	real_vector3d cross;

	cross.i = a->j * b->k - a->k * b->j;
	cross.j = a->k * b->i - a->i * b->k;
	cross.k = a->i * b->j - a->j * b->i;
	*result = cross;
}

static real ragdoll_length(real_vector3d const *v)
{
	return (real)sqrt(ragdoll_dot(v, v));
}

static real ragdoll_normalize(real_vector3d *v)
{
	real length = ragdoll_length(v);

	if (length > 1e-7f)
	{
		v->i /= length;
		v->j /= length;
		v->k /= length;
	}
	else
	{
		length = 0.f;
	}
	return length;
}

static void ragdoll_sub(real_point3d const *a, real_point3d const *b, real_vector3d *result)
{
	result->i = a->x - b->x;
	result->j = a->y - b->y;
	result->k = a->z - b->z;
}

static void ragdoll_add_scaled(real_point3d *point, real_vector3d const *v, real scale)
{
	point->x += v->i * scale;
	point->y += v->j * scale;
	point->z += v->k * scale;
}

static void ragdoll_vector_add_scaled(real_vector3d *vector, real_vector3d const *v, real scale)
{
	vector->i += v->i * scale;
	vector->j += v->j * scale;
	vector->k += v->k * scale;
}

static boolean ragdoll_valid_point(real_point3d const *p)
{
	return valid_real(p->x) && valid_real(p->y) && valid_real(p->z) &&
		fabs(p->x) < 5000.f && fabs(p->y) < 5000.f && fabs(p->z) < 5000.f;
}

static void ragdoll_clamp_length(real_vector3d *v, real maximum)
{
	real length = ragdoll_length(v);

	if (length > maximum)
		scale_vector3d(v, maximum / length, v);
}

/* forward, left, up kept unit length, square and right handed (left = up x forward) */
static void ragdoll_orthonormalize(real_vector3d frame[3])
{
	if (ragdoll_normalize(&frame[0]) == 0.f)
		frame[0] = *global_forward3d;
	ragdoll_cross(&frame[2], &frame[0], &frame[1]);
	if (ragdoll_normalize(&frame[1]) == 0.f)
	{
		real_vector3d other = fabs(frame[0].k) < 0.9f ? *global_up3d : *global_left3d;

		ragdoll_cross(&other, &frame[0], &frame[1]);
		ragdoll_normalize(&frame[1]);
	}
	ragdoll_cross(&frame[0], &frame[1], &frame[2]);
	ragdoll_normalize(&frame[2]);
}

/* a world vector's coordinates in a frame, and back */
static void ragdoll_to_local(real_vector3d const frame[3], real_vector3d const *world, real_vector3d *local)
{
	real_vector3d result;

	result.i = ragdoll_dot(world, &frame[0]);
	result.j = ragdoll_dot(world, &frame[1]);
	result.k = ragdoll_dot(world, &frame[2]);
	*local = result;
}

static void ragdoll_to_world(real_vector3d const frame[3], real_vector3d const *local, real_vector3d *world)
{
	real_vector3d result;

	result.i = frame[0].i * local->i + frame[1].i * local->j + frame[2].i * local->k;
	result.j = frame[0].j * local->i + frame[1].j * local->j + frame[2].j * local->k;
	result.k = frame[0].k * local->i + frame[1].k * local->j + frame[2].k * local->k;
	*world = result;
}

/* a unit vector square to v */
static void ragdoll_perpendicular(real_vector3d const *v, real_vector3d *result)
{
	real_vector3d other = fabs(v->k) < 0.9f ? *global_up3d : *global_forward3d;

	ragdoll_cross(v, &other, result);
	if (ragdoll_normalize(result) == 0.f)
		*result = *global_left3d;
}

/* ---------- quaternions (a frame: local x, y, z to forward, left, up) */

static void ragdoll_quaternion_normalize(struct ragdoll_quaternion *q)
{
	real length = (real)sqrt(q->i * q->i + q->j * q->j + q->k * q->k + q->w * q->w);

	if (length > 1e-7f && valid_real(length))
	{
		q->i /= length;
		q->j /= length;
		q->k /= length;
		q->w /= length;
	}
	else
	{
		q->i = q->j = q->k = 0.f;
		q->w = 1.f;
	}
}

static void ragdoll_rotate(struct ragdoll_quaternion const *q, real_vector3d const *v, real_vector3d *result)
{
	/* v + 2w (u x v) + 2 u x (u x v) */
	real_vector3d u;
	real_vector3d t;
	real_vector3d r;
	real_vector3d out;

	u.i = q->i;
	u.j = q->j;
	u.k = q->k;
	ragdoll_cross(&u, v, &t);
	scale_vector3d(&t, 2.f, &t);
	ragdoll_cross(&u, &t, &r);
	out.i = v->i + q->w * t.i + r.i;
	out.j = v->j + q->w * t.j + r.j;
	out.k = v->k + q->w * t.k + r.k;
	*result = out;
}

static void ragdoll_unrotate(struct ragdoll_quaternion const *q, real_vector3d const *v, real_vector3d *result)
{
	struct ragdoll_quaternion conjugate;

	conjugate.i = -q->i;
	conjugate.j = -q->j;
	conjugate.k = -q->k;
	conjugate.w = q->w;
	ragdoll_rotate(&conjugate, v, result);
}

static void ragdoll_quaternion_from_frame(real_vector3d const frame[3], struct ragdoll_quaternion *q)
{
	/* the rotation's matrix has the frame's axes as its columns */
	real m00 = frame[0].i, m01 = frame[1].i, m02 = frame[2].i;
	real m10 = frame[0].j, m11 = frame[1].j, m12 = frame[2].j;
	real m20 = frame[0].k, m21 = frame[1].k, m22 = frame[2].k;
	real trace = m00 + m11 + m22;

	if (trace > 0.f)
	{
		real s = (real)sqrt(trace + 1.f) * 2.f;

		q->w = 0.25f * s;
		q->i = (m21 - m12) / s;
		q->j = (m02 - m20) / s;
		q->k = (m10 - m01) / s;
	}
	else if (m00 > m11 && m00 > m22)
	{
		real s = (real)sqrt(1.f + m00 - m11 - m22) * 2.f;

		q->w = (m21 - m12) / s;
		q->i = 0.25f * s;
		q->j = (m01 + m10) / s;
		q->k = (m02 + m20) / s;
	}
	else if (m11 > m22)
	{
		real s = (real)sqrt(1.f + m11 - m00 - m22) * 2.f;

		q->w = (m02 - m20) / s;
		q->i = (m01 + m10) / s;
		q->j = 0.25f * s;
		q->k = (m12 + m21) / s;
	}
	else
	{
		real s = (real)sqrt(1.f + m22 - m00 - m11) * 2.f;

		q->w = (m10 - m01) / s;
		q->i = (m02 + m20) / s;
		q->j = (m12 + m21) / s;
		q->k = 0.25f * s;
	}
	ragdoll_quaternion_normalize(q);
}

static void ragdoll_frame_from_quaternion(struct ragdoll_quaternion const *q, real_vector3d frame[3])
{
	ragdoll_rotate(q, global_forward3d, &frame[0]);
	ragdoll_rotate(q, global_left3d, &frame[1]);
	ragdoll_rotate(q, global_up3d, &frame[2]);
	ragdoll_orthonormalize(frame);
}

/* the turn from one frame to the next (a tick apart): its axis, its length
the angle */
static void ragdoll_quaternion_spin(
	struct ragdoll_quaternion const *from,
	struct ragdoll_quaternion const *to,
	real_vector3d *spin)
{
	/* to * conjugate(from) */
	struct ragdoll_quaternion d;
	real sine;
	real angle;

	d.w = to->w * from->w + to->i * from->i + to->j * from->j + to->k * from->k;
	d.i = -to->w * from->i + to->i * from->w - to->j * from->k + to->k * from->j;
	d.j = -to->w * from->j + to->i * from->k + to->j * from->w - to->k * from->i;
	d.k = -to->w * from->k - to->i * from->j + to->j * from->i + to->k * from->w;
	if (d.w < 0.f)
	{
		d.i = -d.i;
		d.j = -d.j;
		d.k = -d.k;
		d.w = -d.w;
	}
	spin->i = d.i;
	spin->j = d.j;
	spin->k = d.k;
	sine = ragdoll_normalize(spin);
	angle = 2.f * (real)atan2(sine, d.w);
	scale_vector3d(spin, sine > 0.f ? angle : 0.f, spin);
}

/* ---------- parameters and the setting */

static void ragdoll_parameters_initialize(
	void)
{
	short index;

	if (ragdoll_parameters_initialized)
		return;
	for (index = 0; index < (short)NUMBEROF(ragdoll_parameter_definitions); index++)
		*ragdoll_parameter_definitions[index].value = ragdoll_parameter_definitions[index].default_value;
	ragdoll_parameters_initialized = TRUE;
}

/* game.death_physics (Video Settings' Death Physics): "ragdoll", or
"classic" (the default), the game's own deaths; read again whenever a
setting changes */
static boolean ragdolls_enabled(
	void)
{
	unsigned long changes = config_changes();

	if (changes != ragdoll_config_changes)
	{
		char const *value = config_string("game.death_physics");

		ragdoll_config_changes = changes;
		ragdoll_config_enabled = !value || strcmp(value, "classic") != 0;
	}
	return ragdoll_config_enabled && !ragdoll_physics_failed;
}

/* ---------- the death animation's first moments */

static long ragdoll_death_animation_ticks(
	void)
{
	return (long)(PIN(ragdoll_parameters.death_animation, 0.f, 10.f) * TICKS_PER_SECOND + 0.5f);
}

static struct ragdoll_dying *ragdoll_dying_find(
	long object_index)
{
	short index;

	for (index = 0; index < RAGDOLL_MAXIMUM_DYING; index++)
	{
		if (ragdoll_dying[index].object_index == object_index)
			return &ragdoll_dying[index];
	}
	return NULL;
}

/* the skeleton as it is now, for the motion of the next tick */
static void ragdoll_dying_record(
	struct ragdoll_dying *dying,
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	real_matrix4x3 const *matrices = (real_matrix4x3 const *)object_header_block_get(
		biped_index,
		&biped->object.node_matrices);
	long count = MIN(biped->object.node_matrices.size / (long)sizeof(real_matrix4x3), RAGDOLL_MAXIMUM_NODES);
	long index;

	dying->node_count = (short)count;
	for (index = 0; index < count; index++)
	{
		dying->positions[index] = matrices[index].position;
		dying->frames[index][0] = matrices[index].forward;
		dying->frames[index][1] = matrices[index].left;
		dying->frames[index][2] = matrices[index].up;
		ragdoll_orthonormalize(dying->frames[index]);
	}
	dying->last_velocity = valid_real_vector3d(&biped->object.translational_velocity) ?
		biped->object.translational_velocity : *global_zero_vector3d;
	dying->was_airborne = TEST_FLAG(biped->biped.flags, _biped_airborne_bit) != 0;
	dying->last_time = game_time_get();
}

/* the skeleton meeting the level or an object (a wall it falls back
against, the ground it lands on): a ray to each node from where it was
(from, NULL: from its pelvis as it died) finds a surface between; steep
surfaces only (walls), unless any_surface */
static boolean ragdoll_dying_touches(
	long biped_index,
	struct ragdoll_dying const *dying,
	boolean from_last,
	boolean any_surface)
{
	struct biped_datum *biped = biped_get(biped_index);
	real_matrix4x3 const *matrices = (real_matrix4x3 const *)object_header_block_get(
		biped_index,
		&biped->object.node_matrices);
	long count = MIN(biped->object.node_matrices.size / (long)sizeof(real_matrix4x3), dying->node_count);
	boolean touches = FALSE;
	long index;

	if (!matrices)
		return FALSE;
	match_assert("ragdoll.c", __LINE__, global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_limp_body_physics;
	for (index = 0; index < count && !touches; index++)
	{
		real_point3d const *from = from_last ? &dying->positions[index] : &dying->anchor;
		real_point3d const *to = &matrices[index].position;
		real_vector3d ray;
		struct collision_result collision;

		ragdoll_sub(to, from, &ray);
		if (!valid_real_vector3d(&ray) || ragdoll_length(&ray) < 1e-4f)
			continue;
		if (collision_test_vector(
			FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_ignore_invisible_surfaces_bit) |
				FLAG(_collision_test_structure_bit) | FLAG(_collision_test_objects_bit) |
				FLAG(_collision_test_objects_scenery_bit) | FLAG(_collision_test_objects_machines_bit) |
				FLAG(_collision_test_objects_vehicles_bit),
			from, &ray, biped_index, &collision) &&
			(any_surface || fabs(collision.plane.n.k) < 0.6f || collision.type != _collision_result_structure))
		{
			touches = TRUE;
		}
	}
	match_assert("ragdoll.c", __LINE__, global_current_collision_user_depth > 1);
	global_current_collision_user_depth--;
	return touches;
}

/* a hit waiting for the body that throws it (an explosion): it goes limp at
once, as Halo 2's thrown bodies do */
static boolean ragdoll_vehicle_hit(long biped_index, real_vector3d *velocity);

static boolean ragdoll_thrown(
	long object_index)
{
	short index;

	/* (run over: thrown by the vehicle, at once) */
	if (ragdoll_vehicle_hit(object_index, NULL))
		return TRUE;

	for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
	{
		struct ragdoll_pending_hit const *hit = &ragdoll_pending_hits[index];

		if (hit->object_index == object_index && ragdoll_length(&hit->body_velocity) > 0.03f)
			return TRUE;
		/* (killed by a sniper round or a melee blow: it knocks the body
		over, no death animation) */
		if (hit->object_index == object_index && game_time_get() - hit->time <= 2 &&
			(hit->category == RAGDOLL_DAMAGE_CATEGORY_SNIPER || hit->category == RAGDOLL_DAMAGE_CATEGORY_MELEE))
		{
			return TRUE;
		}
	}
	return FALSE;
}

/* killed by an explosion just now (not a vehicle): whether it flies in the
game's own thrown animation (a share of them, explosion_flight, by a coin
of its own: the same body and moment, the same way) */
static boolean ragdoll_explosion_flies(
	long biped_index)
{
	long now = game_time_get();
	unsigned long coin;
	short index;
	boolean blasted = FALSE;

	if (ragdoll_parameters.explosion_flight <= 0.f || ragdoll_vehicle_hit(biped_index, NULL))
		return FALSE;
	for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
	{
		struct ragdoll_pending_hit const *hit = &ragdoll_pending_hits[index];

		if (hit->object_index == biped_index && now - hit->time <= 2 && now >= hit->time &&
			ragdoll_length(&hit->body_velocity) > 0.03f)
		{
			blasted = TRUE;
		}
	}
	if (!blasted)
		return FALSE;
	coin = ((unsigned long)biped_index * 2654435761ul) ^ ((unsigned long)now * 40503ul);
	coin = (coin ^ (coin >> 15)) * 2246822519ul;
	return (real)((coin >> 8) & 0xffff) / 65536.f < ragdoll_parameters.explosion_flight;
}

/* a body flying as the explosion threw it: done when anything touches it
(another hit, a vehicle, a surface: landing, a wall, a ceiling), or after
long enough */
static boolean ragdoll_flight_over(
	long biped_index,
	struct ragdoll_dying const *dying)
{
	struct biped_datum *biped = biped_get(biped_index);
	long now = game_time_get();
	long elapsed = now - dying->death_time;
	real_vector3d expected = dying->last_velocity;
	real_vector3d change;
	short index;

	if (elapsed > 5 * TICKS_PER_SECOND || ragdoll_vehicle_hit(biped_index, NULL))
		return TRUE;
	/* shot, or blasted again */
	for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
	{
		struct ragdoll_pending_hit const *hit = &ragdoll_pending_hits[index];

		if (hit->object_index == biped_index && hit->time > dying->death_time)
			return TRUE;
	}
	/* landed (or never left the ground) */
	if (!TEST_FLAG(biped->biped.flags, _biped_airborne_bit) && (dying->was_airborne || elapsed >= 2))
		return TRUE;
	/* its flight turned: it struck something (a wall, a ceiling, an edge) */
	expected.k -= global_gravity;
	ragdoll_sub((real_point3d const *)&biped->object.translational_velocity, (real_point3d const *)&expected, &change);
	if (valid_real_vector3d(&change) && ragdoll_length(&change) > MAX(0.015f, 0.25f * ragdoll_length(&dying->last_velocity)))
		return TRUE;
	/* a limb through a surface */
	if (ragdoll_dying_touches(biped_index, dying, TRUE, TRUE))
		return TRUE;
	return FALSE;
}

/* how the body is dying: in its death animation still, or limp */
static boolean ragdoll_dying_dynamic(
	long biped_index,
	struct ragdoll_dying const *dying)
{
	struct biped_datum *biped = biped_get(biped_index);
	short index;

	/* thrown: by an explosion or a vehicle */
	if (ragdoll_thrown(biped_index))
		return TRUE;
	/* playing its death animation to the end: only a melee blow, an
	explosion or a sniper round ends it sooner (or meeting a wall: below) */
	if (dying && dying->whole)
	{
		for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
		{
			struct ragdoll_pending_hit const *hit = &ragdoll_pending_hits[index];

			if (hit->object_index == biped_index && hit->time > dying->death_time &&
				(hit->category == RAGDOLL_DAMAGE_CATEGORY_MELEE || hit->category == RAGDOLL_DAMAGE_CATEGORY_SNIPER ||
					ragdoll_length(&hit->body_velocity) > 0.f))
			{
				return TRUE;
			}
		}
		return biped->object.position.z < dying->floor_z - 0.5f;
	}
	/* off the ground: jumping, knocked into the air, off a ledge */
	if (TEST_FLAG(biped->biped.flags, _biped_airborne_bit))
		return TRUE;
	/* moving fast (a flung body, a fall) */
	if (ragdoll_length(&biped->object.translational_velocity) > 0.1f)
		return TRUE;
	if (dying)
	{
		/* fallen below where it stood (off an edge as it died) */
		if (biped->object.position.z < dying->floor_z - 0.15f)
			return TRUE;
		/* hit again while dying */
		for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
		{
			struct ragdoll_pending_hit const *hit = &ragdoll_pending_hits[index];

			if (hit->object_index == biped_index && hit->time > dying->death_time)
				return TRUE;
		}
	}
	return FALSE;
}

/* the body's death animation plays until it is down (its head near the
ground, or the animation over: the skeleton still), then the ragdoll takes
it, moving as the animation left it; at once when it dies (or, dying, is
caught) in motion: thrown, in the air, falling, shot again. TRUE while the
animation has it (the game updates it this tick); else the motion it goes
limp with, NULL for none */
static boolean ragdoll_dying_update(
	long biped_index,
	struct ragdoll_dying const **motion)
{
	long now = game_time_get();
	long ticks = ragdoll_death_animation_ticks();
	struct biped_datum *biped = biped_get(biped_index);
	struct ragdoll_dying *dying = ragdoll_dying_find(biped_index);
	real_matrix4x3 const *matrices = (real_matrix4x3 const *)object_header_block_get(
		biped_index,
		&biped->object.node_matrices);

	*motion = NULL;
	/* (a record not kept up, a revert or an old death: forgotten) */
	if (dying && (now - dying->last_time > 2 || now < dying->last_time))
	{
		dying->object_index = NONE;
		dying = NULL;
	}
	/* flying as the explosion threw it, until anything touches it: then
	limp, with the momentum it had (not what the surface left it) */
	if (dying && dying->flying)
	{
		if (!ragdoll_flight_over(biped_index, dying))
		{
			ragdoll_dying_record(dying, biped_index);
			return TRUE;
		}
		if (valid_real_vector3d(&dying->last_velocity) &&
			ragdoll_length(&biped->object.translational_velocity) < ragdoll_length(&dying->last_velocity))
		{
			biped->object.translational_velocity = dying->last_velocity;
		}
		if (now - dying->last_time == 1)
			*motion = dying;
		return FALSE;
	}

	if (ticks <= 0 || (!dying && !ragdoll_explosion_flies(biped_index) && ragdoll_dying_dynamic(biped_index, dying)) ||
		(dying && ragdoll_dying_dynamic(biped_index, dying)))
	{
		if (dying && now - dying->last_time == 1)
			*motion = dying;
		return FALSE;
	}

	if (!dying)
	{
		short index;
		short oldest = 0;

		for (index = 0; index < RAGDOLL_MAXIMUM_DYING; index++)
		{
			if (ragdoll_dying[index].object_index == NONE)
			{
				oldest = index;
				break;
			}
			if (ragdoll_dying[index].last_time < ragdoll_dying[oldest].last_time)
				oldest = index;
		}
		dying = &ragdoll_dying[oldest];
		dying->object_index = biped_index;
		dying->death_time = now;
		dying->still_ticks = 0;
		dying->floor_z = biped->object.position.z;
		ragdoll_dying_record(dying, biped_index);
		/* its head: the highest node as it dies */
		dying->head_node = 0;
		for (index = 1; index < dying->node_count; index++)
		{
			if (dying->positions[index].z > dying->positions[dying->head_node].z)
				dying->head_node = index;
		}
		dying->head_height = dying->positions[dying->head_node].z - dying->floor_z;
		dying->anchor = dying->positions[0];
		{
			unsigned long coin = ((unsigned long)biped_index * 2246822519ul) ^ ((unsigned long)now * 3266489917ul);

			coin = (coin ^ (coin >> 13)) * 668265263ul;
			dying->whole = (real)((coin >> 8) & 0xffff) / 65536.f < ragdoll_parameters.death_animation_whole;
		}
		dying->against_wall = FALSE;
		dying->flying = ragdoll_explosion_flies(biped_index);
		return TRUE;
	}

	{
		long elapsed = now - dying->death_time;
		real head_height = matrices[dying->head_node].position.z - dying->floor_z;
		real moved = 0.f;
		short index;
		boolean down;

		/* the animation still moving the skeleton? */
		for (index = 0; index < dying->node_count; index++)
			moved = MAX(moved, distance3d(&matrices[index].position, &dying->positions[index]));
		dying->still_ticks = moved < 0.0015f ? dying->still_ticks + 1 : 0;

		/* down: its head near the ground (a share of the height it stood at),
		or the animation over (the skeleton still), or the longest it plays */
		if (dying->whole)
		{
			/* (to the end of the animation: the skeleton still) */
			down = (elapsed >= 9 && dying->still_ticks >= 3) || elapsed >= MAX(ticks, 8 * TICKS_PER_SECOND);
		}
		else
		{
			down =
				(dying->head_height > 0.05f && head_height < ragdoll_parameters.down_height * dying->head_height) ||
				(elapsed >= 9 && dying->still_ticks >= 3) ||
				elapsed >= ticks;
		}
		/* falling back against a wall: limp now, to slump down it, not
		finish the animation through it */
		if (!down && ragdoll_dying_touches(biped_index, dying, FALSE, FALSE))
		{
			dying->against_wall = TRUE;
			down = TRUE;
		}
		if (!down)
		{
			ragdoll_dying_record(dying, biped_index);
			return TRUE;
		}
	}
	*motion = dying;
	return FALSE;
}

/* ---------- public code */

/* ---------- the dead left sitting in their seats */

/* port: a share of those killed in a vehicle's seat stay sitting there,
limp: the pelvis held in the seat, the rest hanging from it. Only how the
body is drawn: the game has taken it out of the seat as ever (the seat is
free, nothing else changes), and the body is no target for shots while it
sits. Anyone getting into that seat throws it out where the game would
have; the vehicle on its side or roof, or a crash, throws it loose. */

/* the seat: its cushion's top this far under the pelvis's origin (world
units), its sizes */
#define RAGDOLL_SEAT_DROP 0.08f
#define RAGDOLL_SEAT_HALF_DEPTH 0.09f
#define RAGDOLL_SEAT_HALF_WIDTH 0.09f
#define RAGDOLL_SEAT_HALF_THICK 0.02f

enum
{
	RAGDOLL_MAXIMUM_SEATED = 16,
	/* ticks a body thrown from a seat passes through the vehicle's hull (it
	starts inside it) */
	RAGDOLL_SEATED_THROWN_TICKS = 90,
};

struct ragdoll_seated
{
	long biped_index; /* NONE: free */
	long vehicle_index;
	short seat_index;
	short node_count;
	boolean started; /* its body on the seat (after the game took it out) */
	boolean released; /* thrown: passing through the hull until released_time */
	boolean velocity_valid;
	byte pad;
	long released_time;
	real_point3d exit_local; /* where the game put it, getting out, in the vehicle's frame */
	real_vector3d last_vehicle_velocity;
	real_matrix4x3 pin_vehicle; /* the vehicle as the pelvis was last held */
	real_matrix4x3 root_local; /* the pelvis as it sat, in the vehicle's frame */
	real_matrix4x3 seat_local; /* its seat (under the pelvis, facing as it faced), the same */
	real_matrix4x3 local[RAGDOLL_MAXIMUM_NODES]; /* the body, in the vehicle's frame */
	real_matrix4x3 target_local[RAGDOLL_MAXIMUM_NODES]; /* the pose it slumps in, the same */
};

static struct ragdoll_seated ragdoll_seated_bodies[RAGDOLL_MAXIMUM_SEATED];

/* (to halo.log: what became of the dead left sitting) */
#ifndef RAGDOLL_TEST
void platform_log(const char *format, ...);
#endif
static void ragdoll_seated_log(char const *format, char const *what)
{
#ifndef RAGDOLL_TEST
	char text[160];

	snprintf(text, sizeof(text), format, what);
	platform_log("ragdolls: a body sitting dead in a vehicle %s", text);
#else
	(void)format;
	(void)what;
#endif
}

void ragdolls_initialize_for_new_map(
	void)
{
	short index;

	ragdoll_parameters_initialize();
	for (index = 0; index < RAGDOLL_MAXIMUM_ACTIVE; index++)
		ragdolls[index].object_index = NONE;
	for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
		ragdoll_pending_hits[index].object_index = NONE;
	for (index = 0; index < RAGDOLL_MAXIMUM_DYING; index++)
		ragdoll_dying[index].object_index = NONE;
	for (index = 0; index < RAGDOLL_MAXIMUM_SEATED; index++)
		ragdoll_seated_bodies[index].biped_index = NONE;
	for (index = 0; index < RAGDOLL_MAXIMUM_UNSETTLED; index++)
		ragdoll_unsettled[index].object_index = NONE;
	ragdoll_physics_reset();
	ragdoll_step_time = NONE;
}

void ragdolls_dispose_from_old_map(
	void)
{
	ragdolls_initialize_for_new_map();
}



static struct ragdoll_seated *ragdoll_seated_find(
	long biped_index)
{
	short index;

	for (index = 0; index < RAGDOLL_MAXIMUM_SEATED; index++)
	{
		if (ragdoll_seated_bodies[index].biped_index == biped_index)
			return &ragdoll_seated_bodies[index];
	}
	return NULL;
}

/* a dead body sits in the vehicle, as a ragdoll */
static boolean ragdoll_seated_in(
	long vehicle_index)
{
	short index;

	for (index = 0; index < RAGDOLL_MAXIMUM_SEATED; index++)
	{
		struct ragdoll_seated const *seated = &ragdoll_seated_bodies[index];

		if (seated->biped_index != NONE && seated->started && !seated->released &&
			seated->vehicle_index == vehicle_index && ragdoll_find(seated->biped_index))
		{
			return TRUE;
		}
	}
	return FALSE;
}

static real_matrix4x3 const *ragdoll_seated_vehicle_matrix(
	long vehicle_index)
{
	struct object_datum *vehicle = object_try_and_get(vehicle_index);

	if (!vehicle || vehicle->object.node_matrices.size < (long)sizeof(real_matrix4x3))
		return NULL;
	return (real_matrix4x3 const *)object_header_block_get(vehicle_index, &vehicle->object.node_matrices);
}

/* the body's skeleton put where the vehicle has it (local), the object
about it */
static void ragdoll_seated_place(
	struct ragdoll_seated *seated,
	real_matrix4x3 const *vehicle_matrix)
{
	struct biped_datum *biped = biped_get(seated->biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);
	real_matrix4x3 *matrices = (real_matrix4x3 *)object_header_block_get(seated->biped_index, &biped->object.node_matrices);
	long matrix_count = biped->object.node_matrices.size / (long)sizeof(real_matrix4x3);
	real_point3d minimum, maximum, center;
	real_vector3d half;
	short index;

	if (matrix_count < seated->node_count)
		return;
	for (index = 0; index < seated->node_count; index++)
		matrix4x3_multiply(vehicle_matrix, &seated->local[index], &matrices[index]);
	minimum = maximum = matrices[0].position;
	for (index = 0; index < seated->node_count; index++)
	{
		real_point3d const *p = &matrices[index].position;

		minimum.x = MIN(minimum.x, p->x); minimum.y = MIN(minimum.y, p->y); minimum.z = MIN(minimum.z, p->z);
		maximum.x = MAX(maximum.x, p->x); maximum.y = MAX(maximum.y, p->y); maximum.z = MAX(maximum.z, p->z);
	}
	center.x = (minimum.x + maximum.x) * 0.5f;
	center.y = (minimum.y + maximum.y) * 0.5f;
	center.z = (minimum.z + maximum.z) * 0.5f;
	ragdoll_sub(&maximum, &center, &half);
	object_disconnect_from_map(seated->biped_index);
	biped->object.position.x = matrices[0].position.x;
	biped->object.position.y = matrices[0].position.y;
	biped->object.position.z = minimum.z;
	biped->object.bounding_sphere_center = center;
	biped->object.bounding_sphere_radius = MAX(definition->object.bounding_radius, ragdoll_length(&half) + 0.25f);
	object_bounds_cache_update(seated->biped_index, &biped->object.bounding_sphere_center, biped->object.bounding_sphere_radius);
	object_reconnect_to_map(seated->biped_index, NULL);
	{
		struct object_datum *vehicle = object_try_and_get(seated->vehicle_index);

		biped->object.translational_velocity = vehicle ? vehicle->object.translational_velocity : *global_zero_vector3d;
	}
	biped->object.angular_velocity = *global_zero_vector3d;
	{
		struct ragdoll *ragdoll = ragdoll_find(seated->biped_index);

		/* (the ragdoll's own record of where it wrote the body: else it takes
		this for the body moved under it) */
		if (ragdoll)
		{
			ragdoll->written_object_position = biped->object.position;
			ragdoll->written_root_position = matrices[0].position;
		}
	}
}

/* the body's skeleton as it is now, in the vehicle's frame */
static void ragdoll_seated_take(
	struct ragdoll_seated *seated,
	real_matrix4x3 const *vehicle_matrix)
{
	struct biped_datum *biped = biped_get(seated->biped_index);
	real_matrix4x3 const *matrices = (real_matrix4x3 const *)object_header_block_get(seated->biped_index, &biped->object.node_matrices);
	real_matrix4x3 inverse;
	short index;

	matrix4x3_inverse(vehicle_matrix, &inverse);
	for (index = 0; index < seated->node_count; index++)
		matrix4x3_multiply(&inverse, &matrices[index], &seated->local[index]);
}

/* a ragdoll for the body, sitting as it is, moving with the vehicle */
static struct ragdoll *ragdoll_seated_wake(
	long biped_index)
{
	struct ragdoll *ragdoll = ragdoll_allocate(biped_index);

	if (ragdoll && !ragdoll_seed(ragdoll, biped_index, NULL))
	{
		ragdoll_free(ragdoll);
		ragdoll = NULL;
	}
	if (ragdoll)
		ragdoll_thud_prepare(ragdoll, biped_index, FALSE);
	return ragdoll;
}

static boolean ragdoll_seated_seat_taken(
	struct ragdoll_seated const *seated)
{
	struct object_datum *vehicle = object_try_and_get(seated->vehicle_index);
	long child = vehicle ? vehicle->object.first_child_object_index : NONE;
	short guard = 0;

	while (child != NONE && guard++ < 64)
	{
		struct object_datum *object = object_try_and_get(child);

		if (!object)
			break;
		if (child != seated->biped_index &&
			(object->object.type == _object_type_biped || object->object.type == _object_type_vehicle) &&
			unit_get(child)->unit.parent_seat_index == seated->seat_index)
		{
			return TRUE;
		}
		child = object->object.next_object_index;
	}
	return FALSE;
}

/* thrown from the seat: by someone getting in (out where the game puts a
body leaving it), or loose (over with the vehicle, or a crash) */
static void ragdoll_seated_release(
	struct ragdoll_seated *seated,
	boolean getting_in,
	real_matrix4x3 const *vehicle_matrix,
	char const *why)
{
	ragdoll_seated_log("let go: %s", why);
	struct biped_datum *biped = biped_get(seated->biped_index);
	struct ragdoll *ragdoll;
	real_vector3d offset = *global_zero_vector3d;

	SET_FLAG(biped->object.flags, _object_no_collisions_bit, FALSE);
	if (getting_in && vehicle_matrix)
	{
		/* (the whole body moved there, as it sits; then it falls) */
		real_matrix4x3 *matrices = (real_matrix4x3 *)object_header_block_get(seated->biped_index, &biped->object.node_matrices);
		real_point3d exit;
		short index;

		matrix4x3_transform_point(vehicle_matrix, &seated->exit_local, &exit);
		exit.z += 0.25f;
		ragdoll_sub(&exit, &matrices[0].position, &offset);
		for (index = 0; index < seated->node_count; index++)
			ragdoll_add_scaled(&matrices[index].position, &offset, 1.f);
		ragdoll_seated_take(seated, vehicle_matrix);
		ragdoll_seated_place(seated, vehicle_matrix);
	}
	ragdoll = ragdoll_find(seated->biped_index);
	if (ragdoll && ragdoll_length(&offset) > 0.f)
	{
		float moved[3];

		moved[0] = offset.i;
		moved[1] = offset.j;
		moved[2] = offset.k;
		ragdoll_physics_translate(ragdoll_slot(ragdoll), moved);
		ragdoll->structure_valid = FALSE;
	}
	if (!ragdoll && ragdolls_enabled())
		ragdoll = ragdoll_seated_wake(seated->biped_index);
	if (ragdoll)
	{
		struct ragdoll_node const *root = &ragdoll->nodes[0];
		int slot = ragdoll_slot(ragdoll);

		if (root->body != NONE)
			ragdoll_physics_unpin(slot, root->body);
		ragdoll_physics_seat_vehicle(slot, -1);
		ragdoll_physics_seat_remove(slot);
		ragdoll_physics_ignore_vehicle(slot, getting_in ? -1 : seated->vehicle_index);
	}
	seated->released = TRUE;
	seated->released_time = game_time_get();
	if (!ragdoll)
		seated->biped_index = NONE;
}

/* the body's tick while it sits: TRUE, it sits (and biped_update does
nothing else); FALSE, it is thrown, a ragdoll as any other */
static boolean ragdoll_seated_update(
	struct ragdoll_seated *seated,
	boolean enabled)
{
	long biped_index = seated->biped_index;
	struct biped_datum *biped = biped_get(biped_index);
	struct object_datum *vehicle = object_try_and_get(seated->vehicle_index);
	real_matrix4x3 const *vehicle_matrix = ragdoll_seated_vehicle_matrix(seated->vehicle_index);
	struct ragdoll *ragdoll;
	real_vector3d change;
	boolean moving;
	boolean just_started = FALSE;

	if (seated->released)
	{
		/* (clear of the hull by now: it meets it as any other body) */
		if (game_time_get() - seated->released_time >= RAGDOLL_SEATED_THROWN_TICKS ||
			game_time_get() < seated->released_time)
		{
			ragdoll = ragdoll_find(biped_index);
			if (ragdoll)
				ragdoll_physics_ignore_vehicle(ragdoll_slot(ragdoll), -1);
			seated->biped_index = NONE;
		}
		return FALSE;
	}
	if (!enabled || !vehicle || !vehicle_matrix || ragdoll_parameters.seated_share <= 0.f)
	{
		ragdoll_seated_release(seated, FALSE, vehicle_matrix, "its vehicle gone, ragdolls off, or seated_share 0");
		return FALSE;
	}
	if (ragdoll_seated_seat_taken(seated))
	{
		ragdoll_seated_release(seated, TRUE, vehicle_matrix, "someone took the seat");
		return FALSE;
	}
	ragdoll_sub((real_point3d const *)&vehicle->object.translational_velocity,
		(real_point3d const *)&seated->last_vehicle_velocity, &change);
	moving = ragdoll_length(&vehicle->object.translational_velocity) > 0.004f ||
		ragdoll_length(&vehicle->object.angular_velocity) > 0.004f;
	if (vehicle->object.up.k < 0.3f ||
		(seated->velocity_valid && ragdoll_length(&change) > ragdoll_parameters.seated_crash))
	{
		ragdoll_seated_release(seated, FALSE, vehicle_matrix,
			vehicle->object.up.k < 0.3f ? "the vehicle over on its side or roof" : "a crash (the vehicle's speed changed hard)");
		return FALSE;
	}
	seated->last_vehicle_velocity = vehicle->object.translational_velocity;
	seated->velocity_valid = TRUE;

	if (!seated->started)
	{
		just_started = TRUE;
		seated->started = TRUE;
		SET_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_BIT, TRUE);
		SET_FLAG(biped->object.flags, _object_do_not_recompute_node_matrices_bit, TRUE);
		SET_FLAG(biped->unit.animation.flags, _unit_animation_ignore_translation_bit, TRUE);
		SET_FLAG(biped->object.flags, _object_no_collisions_bit, TRUE);
		biped_stop_limp_body_physics(biped_index);
		ragdoll_seated_place(seated, vehicle_matrix);
	}

	/* (limp from the first, its pelvis held in the seat and its limbs eased
	toward the pose it slumped in, so they sway and swing with the vehicle
	but stay in it; frozen, still, it rides as it lies, and the vehicle
	moving wakes it) */
	ragdoll = ragdoll_find(biped_index);
	if (!ragdoll && (moving || just_started))
	{
		ragdoll_seated_place(seated, vehicle_matrix);
		ragdoll = ragdoll_seated_wake(biped_index);
		if (ragdoll)
		{
			ragdoll_physics_ignore_vehicle(ragdoll_slot(ragdoll), seated->vehicle_index);
			if (ragdoll_parameters.seated_surfaces >= 0.5f)
				ragdoll_physics_seat_vehicle(ragdoll_slot(ragdoll), seated->vehicle_index);
			seated->pin_vehicle = *vehicle_matrix;
		}
	}
	if (ragdoll)
	{
		ragdoll_update(ragdoll, biped_index);
		/* (the pose it took, in the frame the vehicle had in the step) */
		if (ragdoll_find(biped_index) == ragdoll)
		{
			real_matrix4x3 seat;
			real_matrix4x3 const *matrices = (real_matrix4x3 const *)object_header_block_get(
				biped_index, &biped->object.node_matrices);

			/* (fallen or flung out of it: a ragdoll as any other) */
			matrix4x3_multiply(&seated->pin_vehicle, &seated->root_local, &seat);
			if (distance3d(&matrices[0].position, &seat.position) > 0.5f)
			{
				ragdoll_seated_release(seated, FALSE, NULL, "fallen or flung more than half a world unit from the seat");
				return FALSE;
			}
			ragdoll_seated_take(seated, &seated->pin_vehicle);
		}
	}
	ragdoll_seated_place(seated, vehicle_matrix);
	SET_FLAG(biped->object.flags, _object_at_rest_bit, FALSE);
	return TRUE;
}

/* from ragdolls_step, before the world moves: each seated body's pelvis held
in its seat */
static void ragdolls_seated_pin(
	void)
{
	short index;

	for (index = 0; index < RAGDOLL_MAXIMUM_SEATED; index++)
	{
		struct ragdoll_seated *seated = &ragdoll_seated_bodies[index];
		struct ragdoll *ragdoll;
		real_matrix4x3 const *vehicle_matrix;
		real_matrix4x3 root;
		struct ragdoll_quaternion q;
		float position[3];
		float rotation[4];

		if (seated->biped_index == NONE || !seated->started || seated->released)
			continue;
		ragdoll = ragdoll_find(seated->biped_index);
		vehicle_matrix = ragdoll_seated_vehicle_matrix(seated->vehicle_index);
		if (!ragdoll || !vehicle_matrix)
			continue;
		matrix4x3_multiply(vehicle_matrix, &seated->root_local, &root);
		{
			real_vector3d frame[3];

			frame[0] = root.forward;
			frame[1] = root.left;
			frame[2] = root.up;
			ragdoll_orthonormalize(frame);
			ragdoll_quaternion_from_frame(frame, &q);
		}
		position[0] = root.position.x;
		position[1] = root.position.y;
		position[2] = root.position.z;
		rotation[0] = q.i;
		rotation[1] = q.j;
		rotation[2] = q.k;
		rotation[3] = q.w;
		/* (not held: it sits on the vehicle's seat and floor, Jolt's; the
		vehicle as it is now, the frame its pose is read in) */
		(void)position;
		(void)rotation;
		seated->pin_vehicle = *vehicle_matrix;
		/* (its seat, under it and behind it, moving with the vehicle) */
		{
			struct object_datum *vehicle = object_try_and_get(seated->vehicle_index);
			real_matrix4x3 seat;
			real_vector3d frame[3];
			struct ragdoll_quaternion sq;
			float seat_position[3], seat_rotation[4], velocity[3] = { 0.f, 0.f, 0.f };
			float cushion[3] = { RAGDOLL_SEAT_HALF_DEPTH, RAGDOLL_SEAT_HALF_WIDTH, RAGDOLL_SEAT_HALF_THICK };
			float back_center[3] = { -RAGDOLL_SEAT_HALF_DEPTH - 0.02f, 0.f, 0.15f };
			float back_half[3] = { 0.02f, RAGDOLL_SEAT_HALF_WIDTH, 0.15f };

			matrix4x3_multiply(vehicle_matrix, &seated->seat_local, &seat);
			/* (its centre half its thickness under its top) */
			ragdoll_add_scaled(&seat.position, &seat.up, -RAGDOLL_SEAT_HALF_THICK);
			frame[0] = seat.forward;
			frame[1] = seat.left;
			frame[2] = seat.up;
			ragdoll_orthonormalize(frame);
			ragdoll_quaternion_from_frame(frame, &sq);
			seat_position[0] = seat.position.x;
			seat_position[1] = seat.position.y;
			seat_position[2] = seat.position.z;
			seat_rotation[0] = sq.i;
			seat_rotation[1] = sq.j;
			seat_rotation[2] = sq.k;
			seat_rotation[3] = sq.w;
			if (vehicle)
			{
				velocity[0] = vehicle->object.translational_velocity.i;
				velocity[1] = vehicle->object.translational_velocity.j;
				velocity[2] = vehicle->object.translational_velocity.k;
			}
			ragdoll_physics_seat(ragdoll_slot(ragdoll), seat_position, seat_rotation, velocity, cushion,
				back_center, back_half);
		}
	}
}

void ragdolls_seated_after_objects(
	void)
{
	short index;

	/* (every object has moved this tick: the seated where their vehicles
	ended it, as it is drawn) */
	for (index = 0; index < RAGDOLL_MAXIMUM_SEATED; index++)
	{
		struct ragdoll_seated *seated = &ragdoll_seated_bodies[index];
		real_matrix4x3 const *vehicle_matrix;

		if (seated->biped_index == NONE || !seated->started || seated->released)
			continue;
		if (!object_try_and_get(seated->biped_index))
		{
			seated->biped_index = NONE;
			continue;
		}
		vehicle_matrix = ragdoll_seated_vehicle_matrix(seated->vehicle_index);
		if (vehicle_matrix)
			ragdoll_seated_place(seated, vehicle_matrix);
	}
}

void ragdoll_unit_left_seat(
	long unit_index)
{
	struct ragdoll_seated *seated = ragdoll_seated_find(unit_index);
	real_matrix4x3 const *vehicle_matrix;
	real_matrix4x3 inverse;

	if (!seated || seated->started)
		return;
	vehicle_matrix = ragdoll_seated_vehicle_matrix(seated->vehicle_index);
	if (!vehicle_matrix)
	{
		seated->biped_index = NONE;
		return;
	}
	matrix4x3_inverse(vehicle_matrix, &inverse);
	matrix4x3_transform_point(&inverse, &object_get(unit_index)->object.position, &seated->exit_local);
	/* (the pose it sat in kept, not the game's falling out of it) */
	SET_FLAG(biped_get(unit_index)->object.flags, _object_do_not_recompute_node_matrices_bit, TRUE);
	ragdoll_seated_place(seated, vehicle_matrix);
}


/* (v turned by angle radians about the unit axis) */
static void ragdoll_seated_turn(
	real_vector3d *v,
	real_vector3d const *axis,
	real angle)
{
	real c = (real)cos(angle);
	real sn = (real)sin(angle);
	real d = ragdoll_dot(axis, v);
	real_vector3d cross;
	real_vector3d result;

	ragdoll_cross(axis, v, &cross);
	result.i = v->i * c + cross.i * sn + axis->i * d * (1.f - c);
	result.j = v->j * c + cross.j * sn + axis->j * d * (1.f - c);
	result.k = v->k * c + cross.k * sn + axis->k * d * (1.f - c);
	*v = result;
}

/* the node and every node below it turned about the node (in the vehicle's
frame) */
static void ragdoll_seated_turn_subtree(
	struct ragdoll_seated *seated,
	struct model const *model,
	short root,
	real_vector3d const *axis,
	real angle)
{
	real_point3d pivot = seated->local[root].position;
	short index;

	for (index = 0; index < seated->node_count; index++)
	{
		short walk = index;
		short guard = 0;
		real_matrix4x3 *m = &seated->local[index];
		real_vector3d offset;

		while (walk != NONE && walk != root && guard++ < RAGDOLL_MAXIMUM_NODES)
			walk = TAG_BLOCK_GET_ELEMENT(&model->nodes, walk, struct model_node)->parent_node_index;
		if (walk != root)
			continue;
		ragdoll_sub(&m->position, &pivot, &offset);
		ragdoll_seated_turn(&offset, axis, angle);
		m->position = pivot;
		ragdoll_add_scaled(&m->position, &offset, 1.f);
		ragdoll_seated_turn(&m->forward, axis, angle);
		ragdoll_seated_turn(&m->left, axis, angle);
		ragdoll_seated_turn(&m->up, axis, angle);
	}
}

/* limp where it sat: slumped forward a little, its head fallen forward and
to a side, its arms dropped (the pose it died in otherwise) */
static void ragdoll_seated_slump(
	struct ragdoll_seated *seated,
	real_vector3d const *facing_local,
	real_vector3d const *up_local)
{
	struct biped_datum *biped = biped_get(seated->biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);
	struct model const *model = definition->object.model.index != NONE ?
		model_definition_get(definition->object.model.index) : NULL;
	real_vector3d up = *up_local;
	real_vector3d facing = *facing_local;
	real_vector3d left;
	short spine = NONE, head = NONE, index;
	short depth_spine = SHRT_MAX;

	if (!model || model->nodes.count < seated->node_count)
		return;
	ragdoll_vector_add_scaled(&facing, &up, -ragdoll_dot(&facing, &up));
	if (ragdoll_normalize(&facing) <= 0.f)
		return;
	ragdoll_cross(&up, &facing, &left);
	if (ragdoll_normalize(&left) <= 0.f)
		return;
	for (index = 0; index < seated->node_count; index++)
	{
		char const *name = TAG_BLOCK_GET_ELEMENT(&model->nodes, index, struct model_node)->name;

		if (strstr(name, "spine"))
		{
			short depth = 0, walk = index;

			while (walk != NONE && depth < RAGDOLL_MAXIMUM_NODES)
			{
				walk = TAG_BLOCK_GET_ELEMENT(&model->nodes, walk, struct model_node)->parent_node_index;
				depth++;
			}
			if (depth < depth_spine)
			{
				depth_spine = depth;
				spine = index;
			}
		}
		if (head == NONE && strstr(name, "head"))
			head = index;
	}
	/* (the chest forward) */
	if (spine != NONE)
		ragdoll_seated_turn_subtree(seated, model, spine, &left, real_local_random_range(0.15f, 0.3f));
	/* (the arms down from the wheel or the gun) */
	for (index = 0; index < seated->node_count; index++)
	{
		char const *name = TAG_BLOCK_GET_ELEMENT(&model->nodes, index, struct model_node)->name;

		if (strstr(name, "upperarm"))
			ragdoll_seated_turn_subtree(seated, model, index, &left, real_local_random_range(0.3f, 0.6f));
	}
	/* (the head fallen forward, and over to a side) */
	if (head != NONE)
	{
		ragdoll_seated_turn_subtree(seated, model, head, &left, real_local_random_range(0.45f, 0.75f));
		ragdoll_seated_turn_subtree(seated, model, head, &facing,
			real_local_random_range(0.15f, 0.4f) * (local_random_range(0, 2) ? 1.f : -1.f));
	}
}

static void ragdoll_seated_begin(
	long unit_index)
{
	struct biped_datum *biped = biped_get(unit_index);
	long vehicle_index = biped->object.parent_object_index;
	real_matrix4x3 const *vehicle_matrix = ragdoll_seated_vehicle_matrix(vehicle_index);
	long matrix_count = biped->object.node_matrices.size / (long)sizeof(real_matrix4x3);
	struct ragdoll_seated *seated = NULL;
	unsigned long coin;
	short index;

	ragdoll_parameters_initialize();
	if (!ragdolls_enabled() || !vehicle_matrix || biped->unit.parent_seat_index == NONE ||
		matrix_count <= 0 || matrix_count > RAGDOLL_MAXIMUM_NODES || !ragdoll_eligible(unit_index))
	{
		return;
	}
	coin = ((unsigned long)unit_index * 2246822519ul) ^ ((unsigned long)game_time_get() * 3266489917ul);
	coin = (coin ^ (coin >> 13)) * 2654435761ul;
	if ((real)((coin >> 8) & 0xffff) >= ragdoll_parameters.seated_share * 65536.f)
		return;
	for (index = 0; index < RAGDOLL_MAXIMUM_SEATED && !seated; index++)
	{
		struct ragdoll_seated *each = &ragdoll_seated_bodies[index];

		if (each->biped_index == NONE || !object_try_and_get(each->biped_index))
			seated = each;
	}
	if (!seated)
		return; /* (too many sitting dead: this one falls out) */
	csmemset(seated, 0, sizeof(*seated));
	seated->biped_index = unit_index;
	seated->vehicle_index = vehicle_index;
	seated->seat_index = biped->unit.parent_seat_index;
	seated->node_count = (short)matrix_count;
	ragdoll_seated_take(seated, vehicle_matrix);
	seated->root_local = seated->local[0];
	seated->pin_vehicle = *vehicle_matrix;
	{
		real_matrix4x3 inverse;
		real_vector3d facing;
		real_vector3d up_local;

		matrix4x3_inverse(vehicle_matrix, &inverse);
		matrix4x3_transform_vector(&inverse, &biped->object.forward, &facing);
		matrix4x3_transform_vector(&inverse, &object_get(vehicle_index)->object.up, &up_local);
		if (ragdoll_normalize(&up_local) <= 0.f)
			up_local.i = 0.f, up_local.j = 0.f, up_local.k = 1.f;
		ragdoll_seated_slump(seated, &facing, &up_local);
	}
	csmemcpy(seated->target_local, seated->local, sizeof(seated->target_local));
	seated->root_local = seated->target_local[0];
	ragdoll_seated_log("%s", "stays in its seat");
	/* (its seat: under its pelvis, level in the vehicle, facing as it faced) */
	{
		real_matrix4x3 inverse;
		real_vector3d facing;

		real_vector3d up_local;

		/* (the vehicle's own frame, its root node's, need not stand upright:
		up and the facing as the world has them, in it) */
		matrix4x3_inverse(vehicle_matrix, &inverse);
		matrix4x3_transform_vector(&inverse, &biped->object.forward, &facing);
		matrix4x3_transform_vector(&inverse, &object_get(vehicle_index)->object.up, &up_local);
		if (ragdoll_normalize(&up_local) <= 0.f)
			up_local.i = 0.f, up_local.j = 0.f, up_local.k = 1.f;
		ragdoll_vector_add_scaled(&facing, &up_local, -ragdoll_dot(&facing, &up_local));
		if (ragdoll_normalize(&facing) <= 0.f)
		{
			real_vector3d any = { { 1.f, 0.f, 0.f } };

			facing = any;
			ragdoll_vector_add_scaled(&facing, &up_local, -ragdoll_dot(&facing, &up_local));
			ragdoll_normalize(&facing);
		}
		csmemset(&seated->seat_local, 0, sizeof(seated->seat_local));
		seated->seat_local.scale = 1.f;
		seated->seat_local.forward = facing;
		seated->seat_local.up = up_local;
		ragdoll_cross(&seated->seat_local.up, &seated->seat_local.forward, &seated->seat_local.left);
		seated->seat_local.position = seated->root_local.position;
		ragdoll_add_scaled(&seated->seat_local.position, &up_local, -RAGDOLL_SEAT_DROP);
	}
}

void ragdoll_unit_killed_in_seat(
	long unit_index)
{
	struct object_datum *object = object_try_and_get(unit_index);

	/* (a body killed in a seat goes limp once thrown out, as any other; from a
	Banshee, half fall as the game has them fall) */
	if (object && object->object.type == _object_type_biped && object->object.parent_object_index != NONE)
	{
		struct object_datum *vehicle = object_try_and_get(object->object.parent_object_index);
		char const *name = vehicle ? tag_get_name(vehicle->definition_index) : NULL;

		if (name && strstr(name, "banshee"))
		{
			unsigned long coin = ((unsigned long)unit_index * 2654435761ul) ^ ((unsigned long)game_time_get() * 40503ul);

			coin = (coin ^ (coin >> 15)) * 2246822519ul;
			if (((coin >> 8) & 0xffff) < 0x8000)
				SET_FLAG(biped_get(unit_index)->biped.flags, BIPED_PORT_RAGDOLL_CLASSIC_BIT, TRUE);
		}
		/* (else, a share stay sitting, limp) */
		if (!TEST_FLAG(biped_get(unit_index)->biped.flags, BIPED_PORT_RAGDOLL_CLASSIC_BIT))
			ragdoll_seated_begin(unit_index);
	}
}

void ragdoll_biped_deleted(
	long biped_index)
{
	struct ragdoll *ragdoll = ragdoll_find(biped_index);
	struct ragdoll_seated *seated = ragdoll_seated_find(biped_index);

	if (seated)
		seated->biped_index = NONE;
	if (ragdoll)
		ragdoll_free(ragdoll);
}

boolean ragdoll_biped_update(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	boolean dead = TEST_FLAG(biped->object.damage_flags, _object_dead_bit);
	boolean ragdolled = TEST_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_BIT);
	boolean enabled;
	struct ragdoll *ragdoll;
	short index;

	ragdoll_parameters_initialize();
	enabled = ragdolls_enabled();

	/* the world moves once a tick, before the first body's update reads it */
	if (ragdoll_step_time != game_time_get())
	{
		ragdoll_step_time = game_time_get();
		ragdolls_step();
	}

	ragdoll = ragdoll_find(biped_index);

	/* back to life (a Flood combat form that feigned death): the animation
	takes the body back, from where the ragdoll left it */
	if (!dead)
	{
		{
			struct ragdoll_seated *seated = ragdoll_seated_find(biped_index);

			if (seated)
			{
				seated->biped_index = NONE;
				SET_FLAG(biped->object.flags, _object_no_collisions_bit, FALSE);
			}
		}
		if (ragdoll)
			ragdoll_freeze(ragdoll, biped_index);
		if (ragdolled)
		{
			SET_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_BIT, FALSE);
			SET_FLAG(biped->object.flags, _object_do_not_recompute_node_matrices_bit, FALSE);
		}
		SET_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_FAILED_BIT, FALSE);
		SET_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_CLASSIC_BIT, FALSE);
		return FALSE;
	}

	/* left sitting in its seat, dead */
	{
		struct ragdoll_seated *seated = ragdoll_seated_find(biped_index);

		if (seated && biped->object.parent_object_index == NONE)
		{
			if (ragdoll_seated_update(seated, enabled))
			{
				for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
				{
					if (ragdoll_pending_hits[index].object_index == biped_index)
						ragdoll_pending_hits[index].object_index = NONE;
				}
				return TRUE;
			}
			ragdolled = TEST_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_BIT);
			ragdoll = ragdoll_find(biped_index);
		}
	}

	/* in a seat, the seat has the body until it falls out; killed there (a
pilot blown out of a Banshee), it falls as the game has it fall */
	if (biped->object.parent_object_index != NONE)
	{
		return ragdolled;
	}

	if (!ragdolled)
	{
		boolean woken = FALSE;

		/* (a body that died the game's way, lying still, hit again: a ragdoll
		from then on, as any other body) */
		if (TEST_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_CLASSIC_BIT) && enabled &&
			!TEST_FLAG(biped->biped.flags, _biped_airborne_bit) &&
			ragdoll_length(&biped->object.translational_velocity) < 0.005f)
		{
			for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
			{
				struct ragdoll_pending_hit const *hit = &ragdoll_pending_hits[index];

				if (hit->object_index == biped_index && game_time_get() - hit->time <= 2 &&
					game_time_get() >= hit->time && hit->category != RAGDOLL_DAMAGE_CATEGORY_FALLING)
				{
					woken = TRUE;
				}
			}
			if (woken)
				SET_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_CLASSIC_BIT, FALSE);
		}
		if (!enabled ||
			TEST_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_FAILED_BIT) ||
			TEST_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_CLASSIC_BIT) ||
			!ragdoll_eligible(biped_index))
		{
			return FALSE;
		}
		/* falling to its death (a second and more in the air as it died, or
		killed by the fall): the game's own fall, scream and landing */
		if (!woken && !ragdoll_dying_find(biped_index) &&
			(biped->biped.airborne_ticks > 30 ||
				biped->unit.last_damage_category == RAGDOLL_DAMAGE_CATEGORY_FALLING))
		{
			SET_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_CLASSIC_BIT, TRUE);
			return FALSE;
		}

		/* the first moments of its death animation, then limp, moving as it
		left it */
		{
			struct ragdoll_dying const *motion;

			if (ragdoll_dying_update(biped_index, &motion))
				return FALSE;

			/* (run over: the vehicle's speed) */
			{
				real_vector3d thrown;

				if (ragdoll_vehicle_hit(biped_index, &thrown))
					biped->object.translational_velocity = thrown;
			}

			/* (the game has pushed the body by the explosion that killed it,
			biped_accelerate, and the ragdoll pushes it by the same blow:
			the body takes its velocity from before the push, else it would
			fly twice as fast, four times as far) */
			for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
			{
				struct ragdoll_pending_hit const *hit = &ragdoll_pending_hits[index];

				if (hit->object_index == biped_index && ragdoll_length(&hit->body_velocity) > 0.f &&
					game_time_get() - hit->time <= 2 && game_time_get() >= hit->time &&
					valid_real_vector3d(&hit->velocity_before))
				{
					biped->object.translational_velocity = hit->velocity_before;
					break;
				}
			}

			/* (a skeleton it cannot use: the stock death, for good) */
			ragdoll = ragdoll_allocate(biped_index);
			if (ragdoll && !ragdoll_seed(ragdoll, biped_index, motion))
			{
				ragdoll_free(ragdoll);
				ragdoll = NULL;
			}
			if (ragdoll)
				ragdoll_thud_prepare(ragdoll, biped_index, TRUE);
			if (motion)
				((struct ragdoll_dying *)motion)->object_index = NONE;
		}
		if (!ragdoll)
		{
			if (!ragdoll_physics_failed)
				SET_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_FAILED_BIT, TRUE);
			return FALSE;
		}

		/* the death animation stops where the biped was when it died */
		biped_stop_limp_body_physics(biped_index);
		SET_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_BIT, TRUE);
		SET_FLAG(biped->object.flags, _object_do_not_recompute_node_matrices_bit, TRUE);
		SET_FLAG(biped->unit.animation.flags, _unit_animation_ignore_translation_bit, TRUE);
		ragdoll_apply_pending_hits(ragdoll);
	}
	else if (!ragdoll)
	{
		/* frozen: a hit wakes it */
		boolean hit = FALSE;

		for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
		{
			if (ragdoll_pending_hits[index].object_index == biped_index)
				hit = TRUE;
		}
		/* (frozen moving, in the air: on again once there is room) */
		if (!hit && enabled &&
			ragdoll_active_count() < (short)PIN((long)ragdoll_parameters.maximum_active, 1, RAGDOLL_MAXIMUM_ACTIVE))
		{
			for (index = 0; index < RAGDOLL_MAXIMUM_UNSETTLED; index++)
			{
				if (ragdoll_unsettled[index].object_index == biped_index && ragdoll_unsettled[index].pending)
				{
					/* (not forever: a body that never settles stops at last) */
					ragdoll_unsettled[index].pending = FALSE;
					if (ragdoll_unsettled[index].wakes++ < RAGDOLL_MAXIMUM_UNSETTLED_WAKES)
					{
						biped->object.translational_velocity = ragdoll_unsettled[index].velocity;
						hit = TRUE;
					}
				}
			}
		}
		if (hit && enabled)
		{
			ragdoll = ragdoll_allocate(biped_index);
			if (ragdoll && !ragdoll_seed(ragdoll, biped_index, NULL))
			{
				ragdoll_free(ragdoll);
				ragdoll = NULL;
			}
			if (ragdoll)
			{
				ragdoll_thud_prepare(ragdoll, biped_index, FALSE);
				SET_FLAG(biped->object.flags, _object_at_rest_bit, FALSE);
				ragdoll_apply_pending_hits(ragdoll);
			}
		}
	}

	if (ragdoll)
	{
		if (!enabled)
			ragdoll_freeze(ragdoll, biped_index);
		else
			ragdoll_update(ragdoll, biped_index);
	}
	else
	{
		/* frozen bodies count as idle, as the limp ones do */
		SET_FLAG(biped->object.flags, _object_at_rest_bit, TRUE);
		biped->object.translational_velocity = *global_zero_vector3d;
		biped->object.angular_velocity = *global_zero_vector3d;
		if (biped->object.idle_ticks < SHRT_MAX)
			++biped->object.idle_ticks;
	}

	/* (hits for a body that did not wake are old news) */
	for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
	{
		if (ragdoll_pending_hits[index].object_index == biped_index)
			ragdoll_pending_hits[index].object_index = NONE;
	}

	return TRUE;
}

void ragdoll_note_damage(
	long object_index,
	struct damage_data const *damage,
	real damage_amount,
	real instantaneous_acceleration,
	real acceleration_scale,
	boolean lethal,
	short damage_category)
{
	struct ragdoll_pending_hit hit;
	struct object_datum *object = object_get(object_index);
	short index;

	ragdoll_parameters_initialize();
	if (!ragdolls_enabled() || object->object.type != _object_type_biped)
		return;

	/* a living biped's hit matters only as the killing blow (the body becomes
	a ragdoll at its next update, and takes it then) */
	if (!lethal && !TEST_FLAG(object->object.damage_flags, _object_dead_bit))
		return;

	csmemset(&hit, 0, sizeof(hit));
	hit.object_index = object_index;
	hit.time = game_time_get();
	hit.velocity_before = object->object.translational_velocity;

	/* the bullet: the shot's momentum, at the point it hit */
	{
		real_vector3d direction = damage->direction;

		if (ragdoll_normalize(&direction) > 0.f && valid_real(damage_amount) && damage_amount > 0.f)
		{
			real speed = MIN(
				ragdoll_parameters.bullet_impulse * damage_amount,
				ragdoll_parameters.bullet_impulse_maximum);

			/* a Spartan's blow: the body thrown the way it was struck, a
			little up, to roll and tumble */
			if (damage_category == RAGDOLL_DAMAGE_CATEGORY_MELEE)
			{
				speed = MAX(speed, ragdoll_parameters.melee_impulse);
				direction.k = MAX(direction.k, 0.f) + 0.3f;
				ragdoll_normalize(&direction);
			}
			/* a sniper round: momentum enough to knock a body over */
			else if (damage_category == RAGDOLL_DAMAGE_CATEGORY_SNIPER)
			{
				speed = MAX(speed, ragdoll_parameters.sniper_impulse);
			}
			hit.category = damage_category;
			hit.has_point = ragdoll_valid_point(&damage->epicenter);
			hit.point = damage->epicenter;
			scale_vector3d(&direction, speed, &hit.point_velocity);
		}
	}

	/* the explosion: as the game pushes a living biped
	(object_damage_aftermath), to the whole body */
	if (instantaneous_acceleration > _real_epsilon && acceleration_scale > _real_epsilon)
	{
		real_vector3d direction = damage->direction;

		direction.k += 0.45f;
		ragdoll_normalize(&direction);
		scale_vector3d(
			&direction,
			MIN(instantaneous_acceleration * acceleration_scale * (1.f / TICKS_PER_SECOND), RAGDOLL_MAXIMUM_SPEED),
			&hit.body_velocity);
	}

	/* an active ragdoll takes it now */
	{
		struct ragdoll *ragdoll = ragdoll_find(object_index);

		if (ragdoll && TEST_FLAG(object->object.damage_flags, _object_dead_bit))
		{
			ragdoll_apply_hit(ragdoll, &hit);
			return;
		}
	}

	/* else it waits for the body's update: in a free slot, else the oldest */
	{
		short oldest = 0;

		for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
		{
			if (ragdoll_pending_hits[index].object_index == NONE ||
				!object_try_and_get(ragdoll_pending_hits[index].object_index))
			{
				oldest = index;
				break;
			}
			if (ragdoll_pending_hits[index].time < ragdoll_pending_hits[oldest].time)
				oldest = index;
		}
		ragdoll_pending_hits[oldest] = hit;
	}
}

/* ---------- slots */

static int ragdoll_slot(
	struct ragdoll const *ragdoll)
{
	return (int)(ragdoll - ragdolls);
}

static struct ragdoll *ragdoll_find(
	long object_index)
{
	short index;

	for (index = 0; index < RAGDOLL_MAXIMUM_ACTIVE; index++)
	{
		if (ragdolls[index].object_index == object_index)
			return &ragdolls[index];
	}
	return NULL;
}

/* a slot whose body is no ragdoll's any more: deleted without
biped_delete's word, alive again, or a revert to before it died */
static boolean ragdoll_stale(
	struct ragdoll const *ragdoll)
{
	struct object_datum *object = object_try_and_get(ragdoll->object_index);

	return !object || object->object.type != _object_type_biped ||
		!TEST_FLAG(object->object.damage_flags, _object_dead_bit) ||
		!TEST_FLAG(biped_get(ragdoll->object_index)->biped.flags, BIPED_PORT_RAGDOLL_BIT);
}

static struct ragdoll *ragdoll_allocate(
	long object_index)
{
	short maximum_active = (short)PIN((long)ragdoll_parameters.maximum_active, 1, RAGDOLL_MAXIMUM_ACTIVE);
	short active_count = 0;
	short free_index = NONE;
	short oldest_index = NONE;
	real oldest_speed = 0.f;
	short index;

	for (index = 0; index < RAGDOLL_MAXIMUM_ACTIVE; index++)
	{
		struct ragdoll *ragdoll = &ragdolls[index];

		/* (the body being made one is no ragdoll's yet: not stale) */
		if (ragdoll->object_index != NONE && ragdoll->object_index != object_index && ragdoll_stale(ragdoll))
			ragdoll_free(ragdoll);
		if (ragdoll->object_index == NONE)
		{
			if (free_index == NONE && index < maximum_active)
				free_index = index;
		}
		else
		{
			/* (the one to give way: the stillest, not one in the air; the
			oldest of those as still) */
			float velocity[3] = { 0.f, 0.f, 0.f };
			real speed = 0.f;

			active_count++;
			if (ragdoll->nodes[0].body != NONE && ragdoll_physics_exists(index))
			{
				ragdoll_physics_get_bone_velocity(index, ragdoll->nodes[0].body, velocity);
				speed = (real)sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]);
				if (!valid_real(speed))
					speed = 0.f;
			}
			if (oldest_index == NONE || speed < oldest_speed - 0.005f ||
				(speed < oldest_speed + 0.005f && ragdoll->start_time < ragdolls[oldest_index].start_time))
			{
				oldest_index = index;
				oldest_speed = speed;
			}
		}
	}

	if (free_index == NONE || active_count >= maximum_active)
	{
		if (oldest_index == NONE)
			return NULL;
		ragdoll_freeze(&ragdolls[oldest_index], ragdolls[oldest_index].object_index);
		free_index = oldest_index;
	}

	csmemset(&ragdolls[free_index], 0, sizeof(ragdolls[free_index]));
	ragdolls[free_index].object_index = object_index;
	ragdolls[free_index].start_time = game_time_get();
	ragdolls[free_index].last_update_time = game_time_get();
	return &ragdolls[free_index];
}

static void ragdoll_free(
	struct ragdoll *ragdoll)
{
	ragdoll_physics_destroy(ragdoll_slot(ragdoll));
	ragdoll->object_index = NONE;
}

static boolean ragdoll_eligible(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);

	if (definition->object.model.index == NONE)
		return FALSE;
	/* (popped, not dropped: an infection form; simple creatures; the special) */
	if (TEST_FLAG(definition->unit.flags, _unit_is_destroyed_after_dying_bit) ||
		TEST_FLAG(definition->unit.flags, _unit_simple_creature_bit) ||
		TEST_FLAG(definition->unit.flags, _unit_is_special_bit))
	{
		return FALSE;
	}
	return TRUE;
}

/* ---------- bones by name */

static boolean ragdoll_name_has(
	char const *name,
	char const *word)
{
	char lower[TAG_STRING_LENGTH + 1];
	short index;

	for (index = 0; index < TAG_STRING_LENGTH && name[index]; index++)
		lower[index] = (char)tolower((unsigned char)name[index]);
	lower[index] = 0;
	return strstr(lower, word) != NULL;
}

/* the joint at a node (between its bone and its parent's): how far it turns
and twists, in degrees, and whether it is a knee or elbow (and which way of
the body's front it bends: 1 the knee forward, -1 the elbow back) */
static real ragdoll_joint_limit(
	char const *name,
	short *hinge_direction,
	real *twist)
{
	*hinge_direction = 0;
	*twist = ragdoll_parameters.twist_limit;
	if (ragdoll_name_has(name, "calf") || ragdoll_name_has(name, "knee"))
	{
		*hinge_direction = 1;
		return ragdoll_parameters.knee_limit;
	}
	if (ragdoll_name_has(name, "forearm") || ragdoll_name_has(name, "elbow"))
	{
		*hinge_direction = -1;
		return ragdoll_parameters.elbow_limit;
	}
	if (ragdoll_name_has(name, "horselink"))
	{
		*hinge_direction = 1;
		return ragdoll_parameters.knee_limit;
	}
	if (ragdoll_name_has(name, "thigh"))
		return ragdoll_parameters.hip_limit;
	if (ragdoll_name_has(name, "upperarm"))
		return ragdoll_parameters.shoulder_limit;
	if (ragdoll_name_has(name, "neck") || ragdoll_name_has(name, "head"))
	{
		*twist = ragdoll_parameters.neck_twist;
		return ragdoll_parameters.neck_limit;
	}
	if (ragdoll_name_has(name, "spine") || ragdoll_name_has(name, "pelvis") || ragdoll_name_has(name, "clavicle"))
		return ragdoll_parameters.spine_limit;
	return ragdoll_parameters.joint_limit;
}

/* Halo 2's ragdolls (physics_model tags: Marine, Elite, Grunt, Jackal,
Hunter, Master Chief all alike) have ten bodies: the torso (the pelvis,
spine and chest as one), the head, the upper arms, the forearms (with the
hands), the thighs, the calves (with the feet). Whether a node is one of
them, and its share of the body's mass (as Halo 2's: the torso a third or
more, the legs heavy, the arms light) */
static boolean ragdoll_halo2_body(
	char const *name)
{
	return ragdoll_name_has(name, "thigh") || ragdoll_name_has(name, "calf") ||
		ragdoll_name_has(name, "upperarm") || ragdoll_name_has(name, "forearm") ||
		ragdoll_name_has(name, "head");
}

static real ragdoll_halo2_mass_share(
	char const *name)
{
	if (ragdoll_name_has(name, "pelvis"))
		return 0.36f;
	if (ragdoll_name_has(name, "head") || ragdoll_name_has(name, "neck"))
		return 0.07f;
	if (ragdoll_name_has(name, "upperarm"))
		return 0.04f;
	if (ragdoll_name_has(name, "forearm"))
		return 0.05f;
	if (ragdoll_name_has(name, "thigh"))
		return 0.10f;
	if (ragdoll_name_has(name, "calf"))
		return 0.09f;
	return 0.03f;
}

/* a bone's thickness (its radius), metres for a human-sized body */
static real ragdoll_bone_radius(
	char const *name)
{
	if (ragdoll_name_has(name, "pelvis"))
		return 0.13f;
	if (ragdoll_name_has(name, "spine"))
		return 0.125f;
	if (ragdoll_name_has(name, "neck"))
		return 0.05f;
	if (ragdoll_name_has(name, "head"))
		return 0.1f;
	if (ragdoll_name_has(name, "clavicle"))
		return 0.05f;
	if (ragdoll_name_has(name, "upperarm"))
		return 0.05f;
	if (ragdoll_name_has(name, "forearm"))
		return 0.045f;
	if (ragdoll_name_has(name, "hand") || ragdoll_name_has(name, "finger"))
		return 0.04f;
	if (ragdoll_name_has(name, "thigh"))
		return 0.075f;
	if (ragdoll_name_has(name, "calf") || ragdoll_name_has(name, "horselink"))
		return 0.055f;
	return 0.045f; /* a foot, a toe, anything else */
}

/* ---------- bodies */

static boolean ragdoll_seed(
	struct ragdoll *ragdoll,
	long biped_index,
	struct ragdoll_dying const *motion)
{
	static struct ragdoll_physics_bone bones[RAGDOLL_MAXIMUM_NODES];
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);
	struct model *model = model_definition_get(definition->object.model.index);
	real_matrix4x3 const *matrices = (real_matrix4x3 const *)object_header_block_get(
		biped_index,
		&biped->object.node_matrices);
	long matrix_count = biped->object.node_matrices.size / (long)sizeof(real_matrix4x3);
	short node_count = (short)MIN(model->nodes.count, matrix_count);
	struct ragdoll_physics_settings settings;
	real chain[RAGDOLL_MAXIMUM_NODES];
	real size = 0.f;
	real scale;
	real body_mass;
	short order_count;
	short index;
	real_vector3d velocity;
	boolean halo2 = ragdoll_parameters.halo2_skeleton >= 0.5f;

	if (node_count <= 0 || node_count > RAGDOLL_MAXIMUM_NODES)
		return FALSE;
	ragdoll_physics_destroy(ragdoll_slot(ragdoll));

	/* Halo 2's ten bodies, where the skeleton has them (a pelvis, thighs,
	calves and a head at least); else a body for every bone */
	if (halo2)
	{
		short found = 0;

		for (index = 0; index < node_count; index++)
		{
			struct model_node const *model_node = TAG_BLOCK_GET_ELEMENT(&model->nodes, index, struct model_node);

			if (index > 0 && ragdoll_halo2_body(model_node->name))
				found++;
		}
		halo2 = found >= 5;
	}
	ragdoll->node_count = node_count;
	ragdoll->body_count = 0;
	ragdoll->head_body = NONE;
	ragdoll->ticks = 0;
	ragdoll->still_ticks = 0;
	ragdoll->structure_valid = FALSE;
	ragdoll->objects_present = FALSE;

	/* the hierarchy, parents first (a map's links checked) */
	{
		boolean visited[RAGDOLL_MAXIMUM_NODES];

		csmemset(visited, 0, sizeof(visited));
		ragdoll->order[0] = 0;
		visited[0] = TRUE;
		order_count = 1;
		for (index = 0; index < order_count; index++)
		{
			struct model_node const *node = TAG_BLOCK_GET_ELEMENT(
				&model->nodes, ragdoll->order[index], struct model_node);
			short child = node->first_child_node_index;
			short guard = 0;

			while (child != NONE && guard++ < RAGDOLL_MAXIMUM_NODES)
			{
				struct model_node const *child_node;

				if (!VALID_INDEX(child, node_count) || visited[child] || order_count >= RAGDOLL_MAXIMUM_NODES)
					return FALSE;
				child_node = TAG_BLOCK_GET_ELEMENT(&model->nodes, child, struct model_node);
				if (child_node->parent_node_index != ragdoll->order[index])
					return FALSE;
				visited[child] = TRUE;
				ragdoll->order[order_count++] = child;
				child = child_node->next_sibling_node_index;
			}
		}
		if (order_count != node_count)
			return FALSE;
	}

	velocity = biped->object.translational_velocity;
	if (!valid_real_vector3d(&velocity))
		velocity = *global_zero_vector3d;
	ragdoll_clamp_length(&velocity, RAGDOLL_MAXIMUM_SPEED);

	/* the nodes as the body stands, and which have bodies (parents first);
	the skeleton's size: its longest chain of bones from the root */
	for (index = 0; index < node_count; index++)
	{
		short node_index = ragdoll->order[index];
		struct ragdoll_node *node = &ragdoll->nodes[node_index];
		struct model_node const *model_node = TAG_BLOCK_GET_ELEMENT(&model->nodes, node_index, struct model_node);
		real_matrix4x3 const *matrix = &matrices[node_index];
		boolean has_body;

		csmemset(node, 0, sizeof(*node));
		node->parent = index == 0 ? NONE : model_node->parent_node_index;
		node->scale = matrix->scale;
		node->position = matrix->position;
		node->frame[0] = matrix->forward;
		node->frame[1] = matrix->left;
		node->frame[2] = matrix->up;
		ragdoll_orthonormalize(node->frame);
		if (!ragdoll_valid_point(&node->position))
			return FALSE;

		if (node->parent == NONE)
		{
			has_body = TRUE;
			node->simulated_parent = NONE;
			chain[node_index] = 0.f;
		}
		else
		{
			struct ragdoll_node const *parent = &ragdoll->nodes[node->parent];
			real_vector3d offset;
			real length;

			ragdoll_sub(&node->position, &parent->position, &offset);
			length = ragdoll_length(&offset);
			has_body = halo2 ?
				ragdoll_halo2_body(model_node->name) && length >= 0.25f * ragdoll_parameters.minimum_bone :
				length >= ragdoll_parameters.minimum_bone;
			node->simulated_parent = parent->body != NONE ? node->parent : parent->simulated_parent;
			ragdoll_to_local(parent->frame, &offset, &node->attach_offset);
			ragdoll_to_local(parent->frame, &node->frame[0], &node->attach_frame[0]);
			ragdoll_to_local(parent->frame, &node->frame[1], &node->attach_frame[1]);
			ragdoll_to_local(parent->frame, &node->frame[2], &node->attach_frame[2]);
			chain[node_index] = chain[node->parent] + length;
			size = MAX(size, chain[node_index]);
		}
		node->body = has_body && ragdoll->body_count < RAGDOLL_PHYSICS_MAXIMUM_BONES ? ragdoll->body_count++ : NONE;
		if (node->body != NONE)
		{
			struct ragdoll_body *body = &ragdoll->bodies[node->body];

			csmemset(body, 0, sizeof(*body));
			body->node = node_index;
			body->parent_body = node->simulated_parent != NONE ? ragdoll->nodes[node->simulated_parent].body : NONE;
		}
	}
	if (ragdoll->body_count < 4 || size < 0.05f)
		return FALSE;
	/* (thicknesses and the mass for a body of this size) */
	scale = size / RAGDOLL_HUMAN_SIZE;
	body_mass = PIN(ragdoll_parameters.mass * scale * scale * scale, 5.f, 2000.f);

	/* each body: its shape along its bone, its joint to its parent */
	for (index = 0; index < ragdoll->body_count; index++)
	{
		struct ragdoll_body *body = &ragdoll->bodies[index];
		struct ragdoll_node const *node = &ragdoll->nodes[body->node];
		struct model_node const *model_node = TAG_BLOCK_GET_ELEMENT(&model->nodes, body->node, struct model_node);
		struct ragdoll_physics_bone *bone = &bones[index];

		if (ragdoll->head_body == NONE && ragdoll_name_has(model_node->name, "head"))
			ragdoll->head_body = index;
		struct ragdoll_quaternion rotation;
		real_vector3d sum = *global_zero_vector3d;
		real_vector3d farthest_rider = *global_zero_vector3d;
		real farthest_rider_length = 0.f;
		short child_count = 0;
		short other;
		real_vector3d head_local = *global_zero_vector3d;
		real_vector3d foot_local = *global_zero_vector3d;
		boolean head_local_valid = FALSE;
		boolean foot_local_valid = FALSE;
		real_vector3d direction; /* the bone's, in the world */

		/* what it reaches: the joints of the bodies it carries, and the nodes
		riding on it (a foot's toe, a hand's fingers) */
		for (other = 0; other < node_count; other++)
		{
			struct ragdoll_node const *other_node = &ragdoll->nodes[other];
			real_vector3d offset;
			real_vector3d local;

			if (other == body->node || other_node->simulated_parent != body->node)
				continue;
			ragdoll_sub(&other_node->position, &node->position, &offset);
			ragdoll_to_local(node->frame, &offset, &local);
			if (other_node->body != NONE)
			{
				struct model_node const *other_model = TAG_BLOCK_GET_ELEMENT(&model->nodes, other, struct model_node);

				ragdoll_vector_add_scaled(&sum, &local, 1.f);
				child_count++;
				if (ragdoll_name_has(other_model->name, "head") || ragdoll_name_has(other_model->name, "neck"))
				{
					head_local = local;
					head_local_valid = TRUE;
				}
			}
			else if (ragdoll_name_has(TAG_BLOCK_GET_ELEMENT(&model->nodes, other, struct model_node)->name, "foot"))
			{
				foot_local = local;
				foot_local_valid = TRUE;
			}
			if (other_node->body == NONE && ragdoll_length(&local) > farthest_rider_length)
			{
				farthest_rider_length = ragdoll_length(&local);
				farthest_rider = local;
			}
		}

		body->radius = ragdoll_bone_radius(model_node->name) / RAGDOLL_METRES * scale *
			PIN(ragdoll_parameters.thickness, 0.25f, 3.f);
		body->capsule_start = *global_zero_vector3d;
		if (halo2 && body->parent_body == NONE && head_local_valid)
		{
			/* the torso: up to the neck */
			scale_vector3d(&head_local, 0.85f, &body->capsule_end);
		}
		else if (halo2 && foot_local_valid)
		{
			/* a calf: down to the ankle (the foot rides on it) */
			body->capsule_end = foot_local;
		}
		else if (child_count > 0)
		{
			/* to its child's joint, or the middle of its children's */
			scale_vector3d(&sum, 1.f / child_count, &body->capsule_end);
		}
		else if (farthest_rider_length > 0.f)
		{
			body->capsule_end = farthest_rider;
		}
		else
		{
			/* a ball beyond its joint, along its bone from its parent (a
			head, a hand) */
			real_vector3d offset = *global_zero_vector3d;
			real_vector3d local;

			if (node->simulated_parent != NONE)
			{
				ragdoll_sub(&node->position, &ragdoll->nodes[node->simulated_parent].position, &offset);
				ragdoll_normalize(&offset);
			}
			ragdoll_to_local(node->frame, &offset, &local);
			scale_vector3d(&local, 0.7f * body->radius, &body->capsule_start);
			body->capsule_end = body->capsule_start;
		}

		ragdoll_quaternion_from_frame(node->frame, &rotation);
		csmemset(bone, 0, sizeof(*bone));
		/* (moving as the death animation last moved it: each node's motion
		over its last tick, on top of the body's) */
		if (motion && motion->node_count == node_count)
		{
			struct ragdoll_quaternion last;
			real_vector3d moved;
			real_vector3d spin;

			ragdoll_sub(&node->position, &motion->positions[body->node], &moved);
			ragdoll_vector_add_scaled(&moved, &velocity, -1.f);
			/* (no more than a stagger: an animation can snap a bone a long way
			in a tick, which as speed would throw the body) */
			scale_vector3d(&moved, ragdoll_parameters.animation_momentum, &moved);
			ragdoll_clamp_length(&moved, RAGDOLL_ANIMATION_SPEED);
			ragdoll_quaternion_from_frame(motion->frames[body->node], &last);
			ragdoll_quaternion_spin(&last, &rotation, &spin);
			scale_vector3d(&spin, ragdoll_parameters.animation_momentum, &spin);
			ragdoll_clamp_length(&spin, RAGDOLL_ANIMATION_SPIN);
			if (valid_real_vector3d(&moved) && valid_real_vector3d(&spin))
			{
				bone->velocity[0] = moved.i;
				bone->velocity[1] = moved.j;
				bone->velocity[2] = moved.k;
				bone->angular_velocity[0] = spin.i;
				bone->angular_velocity[1] = spin.j;
				bone->angular_velocity[2] = spin.k;
			}
		}
		bone->parent = body->parent_body;
		bone->position[0] = node->position.x;
		bone->position[1] = node->position.y;
		bone->position[2] = node->position.z;
		bone->rotation[0] = rotation.i;
		bone->rotation[1] = rotation.j;
		bone->rotation[2] = rotation.k;
		bone->rotation[3] = rotation.w;
		bone->capsule_start[0] = body->capsule_start.i;
		bone->capsule_start[1] = body->capsule_start.j;
		bone->capsule_start[2] = body->capsule_start.k;
		bone->capsule_end[0] = body->capsule_end.i;
		bone->capsule_end[1] = body->capsule_end.j;
		bone->capsule_end[2] = body->capsule_end.k;
		bone->radius = body->radius;
		bone->joint = _ragdoll_physics_joint_none;
		bone->mass_share = halo2 ? ragdoll_halo2_mass_share(model_node->name) : 0.f;
		/* (a calf's foot: a second capsule, the ankle to the toes, as Halo 2's
		calf has) */
		if (halo2 && foot_local_valid && farthest_rider_length > ragdoll_length(&foot_local) + 0.01f)
		{
			bone->capsule2_start[0] = foot_local.i;
			bone->capsule2_start[1] = foot_local.j;
			bone->capsule2_start[2] = foot_local.k;
			bone->capsule2_end[0] = farthest_rider.i;
			bone->capsule2_end[1] = farthest_rider.j;
			bone->capsule2_end[2] = farthest_rider.k;
			bone->radius2 = ragdoll_bone_radius("foot") / RAGDOLL_METRES * scale * PIN(ragdoll_parameters.thickness, 0.25f, 3.f);
			body->foot_start = foot_local;
			body->foot_end = farthest_rider;
			body->foot_radius = bone->radius2;
		}
		/* (a Grunt's methane tank: on its torso's back, a second capsule along
		it, so a body face down rests on it, not through the floor) */
		else if (index == 0 && ragdoll_parameters.grunt_tank > 0.f && tag_get_name(biped->definition_index) &&
			strstr(tag_get_name(biped->definition_index), "grunt"))
		{
			real_vector3d back;
			real_vector3d back_local;
			real tank_radius = body->radius * 0.9f * ragdoll_parameters.grunt_tank;

			scale_vector3d(&biped->object.forward, -1.f, &back);
			ragdoll_to_local(node->frame, &back, &back_local);
			if (ragdoll_normalize(&back_local) > 0.f)
			{
				real offset = body->radius * 0.9f;

				body->foot_start = body->capsule_start;
				body->foot_end = body->capsule_end;
				ragdoll_vector_add_scaled(&body->foot_start, &back_local, offset);
				ragdoll_vector_add_scaled(&body->foot_end, &back_local, offset);
				body->foot_radius = tank_radius;
				bone->capsule2_start[0] = body->foot_start.i;
				bone->capsule2_start[1] = body->foot_start.j;
				bone->capsule2_start[2] = body->foot_start.k;
				bone->capsule2_end[0] = body->foot_end.i;
				bone->capsule2_end[1] = body->foot_end.j;
				bone->capsule2_end[2] = body->foot_end.k;
				bone->radius2 = tank_radius;
			}
		}

		/* the bone's direction: along its capsule, else from its parent */
		{
			real_vector3d along;

			along.i = body->capsule_end.i - body->capsule_start.i;
			along.j = body->capsule_end.j - body->capsule_start.j;
			along.k = body->capsule_end.k - body->capsule_start.k;
			ragdoll_to_world(node->frame, &along, &direction);
			if (ragdoll_normalize(&direction) == 0.f)
			{
				direction = *global_zero_vector3d;
				if (node->simulated_parent != NONE)
					ragdoll_sub(&node->position, &ragdoll->nodes[node->simulated_parent].position, &direction);
				if (ragdoll_normalize(&direction) == 0.f)
					direction = node->frame[0];
			}
		}

		/* its joint to its parent: swing and twist about its bone, or a hinge */
		if (body->parent_body != NONE)
		{
			struct ragdoll_node const *parent_node = &ragdoll->nodes[ragdoll->bodies[body->parent_body].node];
			short hinge_direction;
			real twist;
			real limit = ragdoll_joint_limit(model_node->name, &hinge_direction, &twist);
			real_vector3d normal;

			ragdoll_perpendicular(&direction, &normal);
			bone->joint = _ragdoll_physics_joint_cone;
			bone->joint_axis[0] = direction.i;
			bone->joint_axis[1] = direction.j;
			bone->joint_axis[2] = direction.k;
			bone->joint_normal[0] = normal.i;
			bone->joint_normal[1] = normal.j;
			bone->joint_normal[2] = normal.k;
			bone->swing_limit = DEGREES_TO_RADIANS(PIN(limit, 0.f, 180.f));
			bone->twist_limit = DEGREES_TO_RADIANS(PIN(twist, 0.f, 180.f));
			/* (the neck a little looser than the rest) */
			bone->friction_scale = ragdoll_name_has(model_node->name, "neck") || ragdoll_name_has(model_node->name, "head") ? 0.7f : 1.f;

			/* a knee or elbow: a hinge about the axis it bent about as the
			biped died (else the one bending it the body's way), from
			straight to the limit (never tighter than it died) */
			if (hinge_direction && (child_count > 0 || farthest_rider_length > 0.f))
			{
				real_vector3d upper;
				real_vector3d lower = direction;
				real_vector3d axis;
				real_vector3d cross;

				ragdoll_sub(&node->position, &parent_node->position, &upper);
				if (ragdoll_normalize(&upper) > 0.f)
				{
					ragdoll_cross(&upper, &lower, &axis);
					if (ragdoll_length(&axis) < 0.17f)
					{
						/* (nearly straight: the shin bends back of the body, the
						forearm forward) */
						real_vector3d bend = biped->object.forward;

						scale_vector3d(&bend, (real)-hinge_direction, &bend);
						ragdoll_cross(&upper, &bend, &axis);
					}
					if (ragdoll_normalize(&axis) > 0.f)
					{
						real bent;
						real minimum;
						real maximum;

						ragdoll_cross(&upper, &lower, &cross);
						bent = (real)atan2(ragdoll_dot(&axis, &cross), ragdoll_dot(&upper, &lower));
						minimum = MIN(-DEGREES_TO_RADIANS(PIN(ragdoll_parameters.hyperextend, 0.f, 90.f)), bent);
						maximum = MAX(DEGREES_TO_RADIANS(PIN(limit, 0.f, 180.f)), bent);
						ragdoll_perpendicular(&axis, &normal);
						bone->joint = _ragdoll_physics_joint_hinge;
						bone->joint_axis[0] = axis.i;
						bone->joint_axis[1] = axis.j;
						bone->joint_axis[2] = axis.k;
						bone->joint_normal[0] = normal.i;
						bone->joint_normal[1] = normal.j;
						bone->joint_normal[2] = normal.k;
						/* (from the pose it is made in) */
						bone->hinge_minimum = minimum - bent;
						bone->hinge_maximum = maximum - bent;
						/* a dead body's knees buckle: one dying straight-legged
						would else stand, balanced on stiff legs (or lean on a
						wall: one fallen against a wall slumps down it) */
						if (hinge_direction > 0 && (!motion || motion->against_wall))
						{
							real buckle = ragdoll_parameters.knee_buckle / TICKS_PER_SECOND;

							bone->angular_velocity[0] += axis.i * buckle;
							bone->angular_velocity[1] += axis.j * buckle;
							bone->angular_velocity[2] += axis.k * buckle;
						}
					}
				}
			}
		}
		body->joint = (short)bone->joint;
	}

	/* the body to Jolt */
	csmemset(&settings, 0, sizeof(settings));
	settings.mass = body_mass;
	settings.friction = ragdoll_parameters.friction;
	settings.restitution = ragdoll_parameters.bounce;
	settings.linear_damping = ragdoll_parameters.air_drag;
	settings.angular_damping = ragdoll_parameters.spin_drag;
	settings.velocity[0] = velocity.i;
	settings.velocity[1] = velocity.j;
	settings.velocity[2] = velocity.k;
	settings.self_collision = ragdoll_parameters.self_collision >= 0.5f;
	settings.joint_friction = ragdoll_parameters.joint_friction;
	settings.gravity = global_gravity * ragdoll_parameters.gravity_scale;
	if (!ragdoll_physics_initialize())
	{
		/* (no engine: the stock deaths, for every body, this run) */
		ragdoll_physics_failed = TRUE;
		return FALSE;
	}
	if (!ragdoll_physics_create(ragdoll_slot(ragdoll), bones, ragdoll->body_count, &settings))
		return FALSE;

	/* the body's front, for which way it lies (fallen on its front or back) */
	{
		struct ragdoll_quaternion root;

		ragdoll_quaternion_from_frame(ragdoll->nodes[0].frame, &root);
		ragdoll_unrotate(&root, &biped->object.forward, &ragdoll->front_local);
	}

	ragdoll->written_object_position = biped->object.position;
	ragdoll->written_root_position = matrices[0].position;
	return TRUE;
}

/* ---------- hits */

/* the nearest point to p on a body's bone (its capsule's line) */
static real ragdoll_body_nearest(
	struct ragdoll const *ragdoll,
	short body_index,
	real_point3d const *p,
	real_point3d *nearest)
{
	struct ragdoll_body const *body = &ragdoll->bodies[body_index];
	struct ragdoll_node const *node = &ragdoll->nodes[body->node];
	real_vector3d start;
	real_vector3d end;
	real_vector3d along;
	real_vector3d to_point;
	real_point3d a = node->position;
	real length_squared;
	real t = 0.f;

	ragdoll_to_world(node->frame, &body->capsule_start, &start);
	ragdoll_to_world(node->frame, &body->capsule_end, &end);
	ragdoll_add_scaled(&a, &start, 1.f);
	along.i = end.i - start.i;
	along.j = end.j - start.j;
	along.k = end.k - start.k;
	ragdoll_sub(p, &a, &to_point);
	length_squared = ragdoll_dot(&along, &along);
	if (length_squared > 1e-10f)
		t = PIN(ragdoll_dot(&to_point, &along) / length_squared, 0.f, 1.f);
	*nearest = a;
	ragdoll_add_scaled(nearest, &along, t);
	return distance3d(nearest, p) - body->radius;
}

static void ragdoll_apply_hit(
	struct ragdoll *ragdoll,
	struct ragdoll_pending_hit const *hit)
{
	int slot = ragdoll_slot(ragdoll);
	short index;

	if (!ragdoll_physics_exists(slot))
		return;

	if (ragdoll_length(&hit->body_velocity) > 0.f)
	{
		float velocity[3];
		real scale = ragdoll_parameters.explosion_scale;

		velocity[0] = hit->body_velocity.i * scale;
		velocity[1] = hit->body_velocity.j * scale;
		velocity[2] = hit->body_velocity.k * scale;
		ragdoll_physics_add_velocity(slot, velocity);
	}

	if (hit->has_point && ragdoll_length(&hit->point_velocity) > 0.f)
	{
		/* the shot's momentum (as much as would move the whole body at
		point_velocity), part of it given to the bone it hit, where it hit (a
		light bone, a hand or the head, swings hard), the rest to the whole
		body (as its joints would hand it on, without tearing at them) */
		short nearest = NONE;
		real nearest_distance = REAL_MAX;
		real_point3d nearest_point = hit->point;

		for (index = 0; index < ragdoll->body_count; index++)
		{
			real_point3d point;
			real distance = ragdoll_body_nearest(ragdoll, index, &hit->point, &point);

			if (distance < nearest_distance)
			{
				nearest_distance = distance;
				nearest = index;
				nearest_point = point;
			}
		}
		if (nearest != NONE)
		{
			float point[3];
			float velocity[3];

			point[0] = nearest_point.x;
			point[1] = nearest_point.y;
			point[2] = nearest_point.z;
			real share = ragdoll_parameters.hit_share;
			real_vector3d push = hit->point_velocity;

			/* a sniper round in the head: the head takes most of it, and the
			body goes over backward about it, up off its feet (Halo 2's and 3's
			headshots) */
			boolean head = nearest == ragdoll->head_body;

			/* (in the head: on it, though the neck's top be as near) */
			if (!head && ragdoll->head_body != NONE)
			{
				real_point3d on_head;

				head = ragdoll_body_nearest(ragdoll, ragdoll->head_body, &hit->point, &on_head) <=
					nearest_distance + 0.5f * ragdoll->bodies[ragdoll->head_body].radius;
			}
			if (hit->category == RAGDOLL_DAMAGE_CATEGORY_SNIPER && head)
			{
				real_vector3d along = hit->point_velocity;
				real_vector3d axis;

				share = ragdoll_parameters.headshot_share;
				push.k += 0.25f * ragdoll_length(&hit->point_velocity);
				/* (head over heels: about the level line across the shot) */
				along.k = 0.f;
				if (ragdoll_normalize(&along) > 0.f)
				{
					float spin[3];

					ragdoll_cross(global_up3d, &along, &axis);
					scale_vector3d(&axis, ragdoll_parameters.headshot_spin / TICKS_PER_SECOND, &axis);
					spin[0] = axis.i;
					spin[1] = axis.j;
					spin[2] = axis.k;
					ragdoll_physics_add_spin(slot, spin);
				}
			}
			/* a melee blow: the most to the whole body (it rolls), some to
			what it struck */
			else if (hit->category == RAGDOLL_DAMAGE_CATEGORY_MELEE)
			{
				share = 0.25f;
			}
			velocity[0] = push.i;
			velocity[1] = push.j;
			velocity[2] = push.k;
			ragdoll_physics_hit(slot, nearest, point, velocity, share);
		}
	}
}

static void ragdoll_apply_pending_hits(
	struct ragdoll *ragdoll)
{
	short index;

	for (index = 0; index < RAGDOLL_MAXIMUM_PENDING_HITS; index++)
	{
		struct ragdoll_pending_hit *hit = &ragdoll_pending_hits[index];

		if (hit->object_index == ragdoll->object_index)
		{
			/* (the killing blow: harder, as death throws a body) */
			struct ragdoll_pending_hit scaled = *hit;

			scale_vector3d(&scaled.point_velocity, ragdoll_parameters.death_impulse_scale, &scaled.point_velocity);
			/* (a killing blow struck in melee: no harder than one on a body
			already down, a little more) */
			if (hit->category == RAGDOLL_DAMAGE_CATEGORY_MELEE)
				ragdoll_clamp_length(&scaled.point_velocity, ragdoll_parameters.melee_impulse * 1.2f);
			/* (the blows of the last ticks: a body that played its death
			animation has shown the killing one already) */
			if (game_time_get() - hit->time <= 2)
				ragdoll_apply_hit(ragdoll, &scaled);
			hit->object_index = NONE;
		}
	}
}

/* ---------- the level, for Jolt */

/* the collision surfaces in a sphere, as triangles, to a ragdoll's layer of
the level; the number of surfaces found */
static short ragdoll_gather_environment(
	struct ragdoll *ragdoll,
	short layer,
	unsigned long flags,
	real_point3d const *center,
	real radius)
{
	short prism_index;
	long triangle_count = 0;

	collision_get_features_in_sphere(
		flags | FLAG(_collision_test_ignore_invisible_surfaces_bit),
		center,
		radius,
		0.f,
		0.f,
		ragdoll->object_index,
		&ragdoll_features);

	for (prism_index = 0; prism_index < ragdoll_features.count[_collision_feature_prism]; prism_index++)
	{
		struct collision_prism const *prism = &ragdoll_features.prisms[prism_index];
		real_point3d points[MAXIMUM_POINTS_PER_COLLISION_PRISM];
		long point_count = MIN(prism->point_count, MAXIMUM_POINTS_PER_COLLISION_PRISM);
		long point_index;

		if (point_count < 3 || !valid_real_plane3d(&prism->plane) ||
			prism->projection_axis < _x || prism->projection_axis > _z)
		{
			continue;
		}
		for (point_index = 0; point_index < point_count; point_index++)
		{
			project_point2d(&prism->points[point_index], &prism->plane, prism->projection_axis,
				prism->projection_sign, &points[point_index]);
		}

		/* a fan, each triangle turned to face where the surface does */
		for (point_index = 1; point_index + 1 < point_count && triangle_count < RAGDOLL_MAXIMUM_TRIANGLES; point_index++)
		{
			real_point3d const *a = &points[0];
			real_point3d const *b = &points[point_index];
			real_point3d const *c = &points[point_index + 1];
			real_vector3d ab;
			real_vector3d ac;
			real_vector3d normal;
			real *out = &ragdoll_triangles[triangle_count * 9];

			ragdoll_sub(b, a, &ab);
			ragdoll_sub(c, a, &ac);
			ragdoll_cross(&ab, &ac, &normal);
			if (ragdoll_dot(&normal, &prism->plane.n) < 0.f)
			{
				real_point3d const *swap = b;

				b = c;
				c = swap;
			}
			out[0] = a->x; out[1] = a->y; out[2] = a->z;
			out[3] = b->x; out[4] = b->y; out[5] = b->z;
			out[6] = c->x; out[7] = c->y; out[8] = c->z;
			triangle_count++;
		}
	}

	ragdoll_physics_set_environment(ragdoll_slot(ragdoll), layer, ragdoll_triangles, (int)triangle_count);
	return ragdoll_features.count[_collision_feature_prism];
}

static void ragdoll_update_environment(
	struct ragdoll *ragdoll)
{
	int slot = ragdoll_slot(ragdoll);
	real_point3d center;
	real extent;
	real speed = 0.f;
	short index;

	/* about the body as it is, as far as it reaches and moves */
	{
		float bounds_center[3] = { 0.f, 0.f, 0.f };
		float bounds_radius = 0.f;

		ragdoll_physics_bounds(slot, bounds_center, &bounds_radius);
		center.x = bounds_center[0];
		center.y = bounds_center[1];
		center.z = bounds_center[2];
		extent = bounds_radius;
		if (!ragdoll_valid_point(&center) || !valid_real(extent))
			return;
		for (index = 0; index < ragdoll->body_count; index++)
		{
			float velocity[3] = { 0.f, 0.f, 0.f };
			real_vector3d v;

			ragdoll_physics_get_bone_velocity(slot, index, velocity);
			v.i = velocity[0];
			v.j = velocity[1];
			v.k = velocity[2];
			if (valid_real_vector3d(&v))
				speed = MAX(speed, ragdoll_length(&v));
		}
		speed = MIN(speed, RAGDOLL_MAXIMUM_SPEED);
	}

	/* the structure: gathered again only when the body (and where it may get
	to this tick) leaves what was */
	if (!ragdoll->structure_valid ||
		distance3d(&center, &ragdoll->structure_center) + extent + 2.f * speed > ragdoll->structure_radius)
	{
		real radius = extent + 2.f * speed + 0.6f;

		/* (too many surfaces for one gathering: nearer) */
		if (ragdoll_gather_environment(ragdoll, _ragdoll_environment_structure,
			FLAG(_collision_test_structure_bit), &center, radius) >= MAXIMUM_COLLISION_FEATURES_PER_TEST)
		{
			radius = extent + 2.f * speed + 0.15f;
			ragdoll_gather_environment(ragdoll, _ragdoll_environment_structure,
				FLAG(_collision_test_structure_bit), &center, radius);
		}
		ragdoll->structure_valid = TRUE;
		ragdoll->structure_center = center;
		ragdoll->structure_radius = radius;
	}

	/* the objects (scenery, machines that move): each tick (vehicles are
	bodies of their own, ragdolls_vehicles) */
	ragdoll->objects_present = ragdoll_gather_environment(ragdoll, _ragdoll_environment_objects,
		FLAG(_collision_test_objects_bit) |
			FLAG(_collision_test_objects_scenery_bit) |
			FLAG(_collision_test_objects_machines_bit),
		&center, extent + 2.f * speed + 0.2f) > 0;
}

/* ---------- vehicles */

enum
{
	RAGDOLL_MAXIMUM_VEHICLES = 32,
	RAGDOLL_MAXIMUM_VEHICLE_POINTS = 2048,
};

/* (moving, world units a tick: a vehicle faster than this wakes the bodies it
meets) */
#define RAGDOLL_VEHICLE_MOVING 0.004f

struct ragdoll_vehicle
{
	long object_index;
	real_point3d center;
	real radius;
	real speed;
};

static real ragdoll_vehicle_points[RAGDOLL_MAXIMUM_VEHICLE_POINTS * 3];
/* (this tick's, for the bodies a vehicle kills: ragdoll_vehicle_hit) */
static struct ragdoll_vehicle ragdoll_vehicles_now[RAGDOLL_MAXIMUM_VEHICLES];
static short ragdoll_vehicle_count_now = 0;

/* the vehicles near the ragdolls, to Jolt as bodies that move as the game
moves them (nothing stops them: a body they meet is thrown or rolls over
them); and a body at rest that a moving vehicle meets wakes */
static void ragdolls_vehicles(
	void)
{
	struct ragdoll_vehicle vehicles[RAGDOLL_MAXIMUM_VEHICLES];
	short vehicle_count = 0;
	boolean any_moving = FALSE;
	struct object_iterator iterator;
	short index;

	/* (a campaign map has more vehicles than are kept, parked and wrecked
	ones among them: the moving first, then the rest, as many as fit) */
	{
	short pass;

	for (pass = 0; pass < 2; pass++)
	{
	object_iterator_new(&iterator, _object_mask_vehicle, 0);
	while (object_iterator_next(&iterator) && vehicle_count < RAGDOLL_MAXIMUM_VEHICLES)
	{
		struct object_datum *object = object_get(iterator.index);
		struct ragdoll_vehicle *vehicle = &vehicles[vehicle_count];
		boolean moving;

		if (!ragdoll_valid_point(&object->object.bounding_sphere_center) ||
			!valid_real_vector3d(&object->object.translational_velocity))
		{
			continue;
		}
		moving = ragdoll_length(&object->object.translational_velocity) > RAGDOLL_VEHICLE_MOVING ||
			(valid_real_vector3d(&object->object.angular_velocity) &&
				ragdoll_length(&object->object.angular_velocity) > RAGDOLL_VEHICLE_MOVING);
		if (moving != (pass == 0))
			continue;
		vehicle->object_index = iterator.index;
		vehicle->center = object->object.bounding_sphere_center;
		vehicle->radius = object->object.bounding_sphere_radius;
		vehicle->speed = MIN(ragdoll_length(&object->object.translational_velocity), 2.f);
		any_moving = any_moving || vehicle->speed > RAGDOLL_VEHICLE_MOVING;
		vehicle_count++;
	}
	}
	}

	csmemcpy(ragdoll_vehicles_now, vehicles, sizeof(vehicles[0]) * vehicle_count);
	ragdoll_vehicle_count_now = vehicle_count;

	/* bodies at rest, met by a moving vehicle: awake */
	if (any_moving && ragdolls_enabled())
	{
		object_iterator_new(&iterator, _object_mask_biped, 0);
		while (object_iterator_next(&iterator))
		{
			struct biped_datum *biped = biped_get(iterator.index);
			short vehicle_index;

			if (!TEST_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_BIT) ||
				!TEST_FLAG(biped->object.damage_flags, _object_dead_bit) ||
				biped->object.parent_object_index != NONE ||
				ragdoll_find(iterator.index))
			{
				continue;
			}
			for (vehicle_index = 0; vehicle_index < vehicle_count; vehicle_index++)
			{
				struct ragdoll_vehicle const *vehicle = &vehicles[vehicle_index];

				if (vehicle->speed > RAGDOLL_VEHICLE_MOVING &&
					distance3d(&vehicle->center, &biped->object.bounding_sphere_center) <
						vehicle->radius + biped->object.bounding_sphere_radius + 2.f * vehicle->speed)
				{
					struct ragdoll *ragdoll = ragdoll_allocate(iterator.index);

					if (ragdoll && !ragdoll_seed(ragdoll, iterator.index, NULL))
						ragdoll_free(ragdoll);
					else if (ragdoll)
						SET_FLAG(biped->object.flags, _object_at_rest_bit, FALSE);
					break;
					/* (the vehicle itself pushes it, a body of its own in Jolt) */
				}
			}
		}
	}

	/* the vehicles near a moving body, to Jolt */
	ragdoll_physics_vehicles_begin();
	for (index = 0; index < vehicle_count; index++)
	{
		struct ragdoll_vehicle const *vehicle = &vehicles[index];
		struct object_datum *object = object_get(vehicle->object_index);
		boolean near = FALSE;
		short slot;

		for (slot = 0; slot < RAGDOLL_MAXIMUM_ACTIVE && !near; slot++)
		{
			float center[3];
			float radius = 0.f;
			real_point3d point;

			if (ragdolls[slot].object_index == NONE || !ragdoll_physics_exists(slot))
				continue;
			ragdoll_physics_bounds(slot, center, &radius);
			point.x = center[0];
			point.y = center[1];
			point.z = center[2];
			near = distance3d(&point, &vehicle->center) < vehicle->radius + radius + 3.f * vehicle->speed + 0.3f;
		}
		if (!near)
			continue;

		/* its collision's points, in its own frame */
		{
			real_vector3d frame[3];
			struct ragdoll_quaternion rotation;
			long point_count = 0;
			short prism_index;

			frame[0] = object->object.forward;
			frame[2] = object->object.up;
			ragdoll_orthonormalize(frame);
			collision_get_features_in_sphere(
				FLAG(_collision_test_objects_bit) | FLAG(_collision_test_objects_vehicles_bit),
				&vehicle->center,
				vehicle->radius,
				0.f,
				0.f,
				NONE,
				&ragdoll_features);
			for (prism_index = 0; prism_index < ragdoll_features.count[_collision_feature_prism]; prism_index++)
			{
				struct collision_prism const *prism = &ragdoll_features.prisms[prism_index];
				long point_index;

				if (prism->object_index != vehicle->object_index || !valid_real_plane3d(&prism->plane) ||
					prism->projection_axis < _x || prism->projection_axis > _z)
				{
					continue;
				}
				for (point_index = 0; point_index < MIN(prism->point_count, MAXIMUM_POINTS_PER_COLLISION_PRISM) &&
					point_count < RAGDOLL_MAXIMUM_VEHICLE_POINTS; point_index++)
				{
					real_point3d world;
					real_vector3d offset;
					real_vector3d local;

					project_point2d(&prism->points[point_index], &prism->plane, prism->projection_axis,
						prism->projection_sign, &world);
					ragdoll_sub(&world, &object->object.position, &offset);
					ragdoll_to_local(frame, &offset, &local);
					ragdoll_vehicle_points[point_count * 3 + 0] = local.i;
					ragdoll_vehicle_points[point_count * 3 + 1] = local.j;
					ragdoll_vehicle_points[point_count * 3 + 2] = local.k;
					point_count++;
				}
			}
			if (point_count < 4)
				continue;

			ragdoll_quaternion_from_frame(frame, &rotation);
			{
				float position[3];
				float turn[4];
				float velocity[3];
				float spin[3];
				real_vector3d angular = object->object.angular_velocity;

				if (!valid_real_vector3d(&angular))
					angular = *global_zero_vector3d;
				position[0] = object->object.position.x;
				position[1] = object->object.position.y;
				position[2] = object->object.position.z;
				turn[0] = rotation.i;
				turn[1] = rotation.j;
				turn[2] = rotation.k;
				turn[3] = rotation.w;
				velocity[0] = object->object.translational_velocity.i;
				velocity[1] = object->object.translational_velocity.j;
				velocity[2] = object->object.translational_velocity.k;
				spin[0] = angular.i;
				spin[1] = angular.j;
				spin[2] = angular.k;
				ragdoll_physics_vehicle(vehicle->object_index, ragdoll_vehicle_points, (int)point_count,
					position, turn, velocity, spin);
			}
			/* (the dead sitting in it meet its surfaces, its seats and floor, not
			its hull: made once, in its frame) */
			if (ragdoll_parameters.seated_surfaces >= 0.5f && ragdoll_seated_in(vehicle->object_index) &&
				!ragdoll_physics_vehicle_has_mesh(vehicle->object_index))
			{
				long triangle_count = 0;

				for (prism_index = 0; prism_index < ragdoll_features.count[_collision_feature_prism]; prism_index++)
				{
					struct collision_prism const *prism = &ragdoll_features.prisms[prism_index];
					real_point3d points[MAXIMUM_POINTS_PER_COLLISION_PRISM];
					long count = MIN(prism->point_count, MAXIMUM_POINTS_PER_COLLISION_PRISM);
					long point_index;

					if (prism->object_index != vehicle->object_index || count < 3 || !valid_real_plane3d(&prism->plane) ||
						prism->projection_axis < _x || prism->projection_axis > _z)
					{
						continue;
					}
					for (point_index = 0; point_index < count; point_index++)
					{
						real_point3d world;
						real_vector3d offset;
						real_vector3d local;

						project_point2d(&prism->points[point_index], &prism->plane, prism->projection_axis,
							prism->projection_sign, &world);
						ragdoll_sub(&world, &object->object.position, &offset);
						ragdoll_to_local(frame, &offset, &local);
						points[point_index].x = local.i;
						points[point_index].y = local.j;
						points[point_index].z = local.k;
					}
					{
						real_vector3d normal_local;

						ragdoll_to_local(frame, &prism->plane.n, &normal_local);
						for (point_index = 1; point_index + 1 < count && triangle_count < RAGDOLL_MAXIMUM_TRIANGLES; point_index++)
						{
							real_point3d const *a = &points[0];
							real_point3d const *b = &points[point_index];
							real_point3d const *c = &points[point_index + 1];
							real_vector3d ab, ac, normal;
							real *out = &ragdoll_triangles[triangle_count * 9];

							ragdoll_sub(b, a, &ab);
							ragdoll_sub(c, a, &ac);
							ragdoll_cross(&ab, &ac, &normal);
							if (ragdoll_dot(&normal, &normal_local) < 0.f)
							{
								real_point3d const *swap = b;

								b = c;
								c = swap;
							}
							out[0] = a->x; out[1] = a->y; out[2] = a->z;
							out[3] = b->x; out[4] = b->y; out[5] = b->z;
							out[6] = c->x; out[7] = c->y; out[8] = c->z;
							triangle_count++;
						}
					}
				}
				ragdoll_physics_vehicle_mesh(vehicle->object_index, ragdoll_triangles, (int)triangle_count);
			}
		}
	}
	ragdoll_physics_vehicles_end();
}

/* a moving vehicle reaching the body (that killed it, as like as not): its
velocity, which the body takes on as it goes limp, and somewhat up, as a
body hit by a vehicle is thrown */
static boolean ragdoll_vehicle_hit(
	long biped_index,
	real_vector3d *velocity)
{
	struct biped_datum *biped = biped_get(biped_index);
	short index;

	for (index = 0; index < ragdoll_vehicle_count_now; index++)
	{
		struct ragdoll_vehicle const *vehicle = &ragdoll_vehicles_now[index];
		struct object_datum *object = object_try_and_get(vehicle->object_index);

		if (!object || vehicle->speed <= 4.f * RAGDOLL_VEHICLE_MOVING)
			continue;
		if (distance3d(&vehicle->center, &biped->object.bounding_sphere_center) <
			vehicle->radius + biped->object.bounding_sphere_radius * 0.5f + 2.f * vehicle->speed)
		{
			if (velocity)
			{
				scale_vector3d(&object->object.translational_velocity, 0.85f, velocity);
				velocity->k += 0.25f * vehicle->speed;
				ragdoll_clamp_length(velocity, RAGDOLL_MAXIMUM_SPEED);
			}
			return TRUE;
		}
	}
	return FALSE;
}

/* ---------- the world's tick */

static void ragdolls_step(
	void)
{
	long now = game_time_get();
	boolean any = FALSE;
	short index;

	match_assert("ragdoll.c", __LINE__, global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_limp_body_physics;

	/* (vehicles first: a body one meets wakes, and is stepped this tick) */
	if (ragdolls_enabled())
		ragdolls_vehicles();
	ragdolls_seated_pin();

	for (index = 0; index < RAGDOLL_MAXIMUM_ACTIVE; index++)
	{
		struct ragdoll *ragdoll = &ragdolls[index];

		if (ragdoll->object_index == NONE)
			continue;
		if (ragdoll_stale(ragdoll) || !ragdoll_physics_exists(index))
		{
			ragdoll_free(ragdoll);
			continue;
		}
		/* (a body the game no longer updates, out of the part of the map
		that runs: it stays as it lies) */
		if (now - ragdoll->last_update_time > 2 || now < ragdoll->last_update_time)
		{
			ragdoll_freeze(ragdoll, ragdoll->object_index);
			continue;
		}
		ragdoll_update_environment(ragdoll);
		any = TRUE;
	}

	match_assert("ragdoll.c", __LINE__, global_current_collision_user_depth > 1);
	global_current_collision_user_depth--;

	if (any)
	{
		ragdoll_physics_step(
			(int)PIN((long)ragdoll_parameters.collision_steps, 1, 8),
			global_gravity * ragdoll_parameters.gravity_scale);

		/* bodies a vehicle met this step bleed, as hard as it hit them */
		{
			struct ragdoll_physics_contact contacts[32];
			int count = ragdoll_physics_vehicle_contacts(contacts, 32);
			int contact_index;

			for (contact_index = 0; contact_index < count; contact_index++)
			{
				struct ragdoll_physics_contact const *contact = &contacts[contact_index];
				real_point3d point;
				real_vector3d normal;

				if (contact->slot < 0 || contact->slot >= RAGDOLL_MAXIMUM_ACTIVE ||
					ragdolls[contact->slot].object_index == NONE)
					continue;
				/* (a body that sat dead in this vehicle, coming out of it: no
				hit, no blood) */
				{
					struct ragdoll_seated const *seated = ragdoll_seated_find(ragdolls[contact->slot].object_index);

					if (seated && seated->vehicle_index == contact->vehicle)
						continue;
				}
				point.x = contact->point[0];
				point.y = contact->point[1];
				point.z = contact->point[2];
				normal.i = contact->normal[0];
				normal.j = contact->normal[1];
				normal.k = contact->normal[2];
#if defined(BLOOD_EFFECTS) && BLOOD_EFFECTS
				blood_vehicle_hit_body(
					ragdolls[contact->slot].object_index,
					contact->vehicle,
					&point,
					&normal,
					contact->speed);
#else
				(void)point;
				(void)normal;
#endif
			}
		}
	}
}

/* ---------- a body's tick */

static void ragdoll_update(
	struct ragdoll *ragdoll,
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	real_matrix4x3 const *matrices = (real_matrix4x3 const *)object_header_block_get(
		biped_index,
		&biped->object.node_matrices);
	int slot = ragdoll_slot(ragdoll);
	short index;

	ragdoll->last_update_time = game_time_get();

	/* moved under it: the object alone (object_translate) or with its
	skeleton, the whole ragdoll goes there; the skeleton changed otherwise (a
	revert, a saved game loaded), it starts again from the body as it now is */
	{
		real_vector3d moved;
		real_vector3d root_moved;
		real_vector3d difference;
		real moved_distance;
		real root_moved_distance;

		ragdoll_sub(&biped->object.position, &ragdoll->written_object_position, &moved);
		ragdoll_sub(&matrices[0].position, &ragdoll->written_root_position, &root_moved);
		moved_distance = ragdoll_length(&moved);
		root_moved_distance = ragdoll_length(&root_moved);
		difference = root_moved;
		ragdoll_vector_add_scaled(&difference, &moved, -1.f);
		if (root_moved_distance > 1e-3f && ragdoll_length(&difference) > 1e-3f)
		{
			if (!ragdoll_seed(ragdoll, biped_index, NULL))
			{
				ragdoll_freeze(ragdoll, biped_index);
				return;
			}
		}
		else if (moved_distance >= 1e-4f)
		{
			float offset[3];

			offset[0] = moved.i;
			offset[1] = moved.j;
			offset[2] = moved.k;
			ragdoll_physics_translate(slot, offset);
			ragdoll->structure_valid = FALSE;
		}
	}

	/* the nodes: of the bodies, and riding on their parents */
	for (index = 0; index < ragdoll->node_count; index++)
	{
		short node_index = ragdoll->order[index];
		struct ragdoll_node *node = &ragdoll->nodes[node_index];

		if (node->body != NONE)
		{
			float position[3];
			float rotation[4];
			struct ragdoll_quaternion q;
			real_point3d point;

			position[0] = node->position.x;
			position[1] = node->position.y;
			position[2] = node->position.z;
			rotation[0] = rotation[1] = rotation[2] = 0.f;
			rotation[3] = 1.f;
			ragdoll_physics_get_bone(slot, node->body, position, rotation);
			point.x = position[0];
			point.y = position[1];
			point.z = position[2];
			q.i = rotation[0];
			q.j = rotation[1];
			q.k = rotation[2];
			q.w = rotation[3];
			/* (anything gone wrong, not a number or out of the world: it stops
			where it last was) */
			if (!ragdoll_valid_point(&point) || !valid_real(q.i) || !valid_real(q.j) ||
				!valid_real(q.k) || !valid_real(q.w))
			{
				ragdoll_freeze(ragdoll, biped_index);
				return;
			}
			ragdoll_quaternion_normalize(&q);
			node->position = point;
			ragdoll_frame_from_quaternion(&q, node->frame);
		}
		else
		{
			struct ragdoll_node const *parent = &ragdoll->nodes[node->parent];
			real_vector3d offset;

			ragdoll_to_world(parent->frame, &node->attach_offset, &offset);
			node->position = parent->position;
			ragdoll_add_scaled(&node->position, &offset, 1.f);
			ragdoll_to_world(parent->frame, &node->attach_frame[0], &node->frame[0]);
			ragdoll_to_world(parent->frame, &node->attach_frame[1], &node->frame[1]);
			ragdoll_to_world(parent->frame, &node->attach_frame[2], &node->frame[2]);
			ragdoll_orthonormalize(node->frame);
		}
	}
	ragdoll_write(ragdoll, biped_index);

	/* barely moving: no node further than sleep_distance from where it was
	when it last moved (a body creeping, or rocking in place) */
	{
		boolean moved = ragdoll->still_ticks == 0;

		for (index = 0; index < ragdoll->node_count && !moved; index++)
		{
			if (distance3d(&ragdoll->nodes[index].position, &ragdoll->still_anchor[index]) > ragdoll_parameters.sleep_distance)
				moved = TRUE;
		}
		if (moved)
		{
			for (index = 0; index < ragdoll->node_count; index++)
				ragdoll->still_anchor[index] = ragdoll->nodes[index].position;
			ragdoll->still_ticks = 1;
		}
		else
		{
			ragdoll->still_ticks++;
		}
	}

	/* at rest (Jolt has put it to sleep, or it has barely moved), or
	simulated long enough: it freezes */
	ragdoll->ticks++;
	if ((ragdoll->ticks > 1 && !ragdoll_physics_active(slot)) ||
		ragdoll->still_ticks >= (long)(ragdoll_parameters.sleep_seconds * TICKS_PER_SECOND) ||
		ragdoll->ticks >= (long)(ragdoll_parameters.maximum_seconds * TICKS_PER_SECOND))
	{
		/* (at rest, or simulated long enough: it stays where it is) */
		ragdoll_freezing_at_rest = TRUE;
		ragdoll_freeze(ragdoll, biped_index);
		ragdoll_freezing_at_rest = FALSE;
	}
}

/* the skeleton into the body: its node matrices, where it is, its bounds */
/* ---------- the thud of a body hitting the ground */

/* an animation graph's sound reference (model_animations.c's) */
struct ragdoll_animation_sound_reference
{
	struct tag_reference sound;
	long unused;
};

#ifndef RAGDOLL_TEST
typedef char verify_ragdoll_animation_sound_reference_size[
	sizeof(struct ragdoll_animation_sound_reference) == 0x14 ? 1 : -1];
#endif

/* the body's thud: its death animation's sound (a body fall, played by the
game at a frame of the animation) when the animation never reached it, as
the ragdoll took it first; else none yet */
static void ragdoll_thud_prepare(
	struct ragdoll *ragdoll,
	long biped_index,
	boolean dying)
{
	struct biped_datum *biped = biped_get(biped_index);

	ragdoll->thud_sound = NONE;
	ragdoll->last_thud_time = game_time_get();
	ragdoll->last_velocity_valid = FALSE;
	if (!dying || biped->object.animation.animation_graph_index == NONE)
		return;
	{
		struct animation_graph const *graph = animation_graph_definition_get(biped->object.animation.animation_graph_index);
		struct animation const *animation;

		if (!graph || !VALID_INDEX(biped->object.animation.state.index, graph->animations.count))
			return;
		animation = TAG_BLOCK_GET_ELEMENT(&graph->animations, biped->object.animation.state.index, struct animation);
		if (VALID_INDEX(animation->sound_index, graph->sound_references.count) &&
			biped->object.animation.state.frame_index <= animation->private_sound_frame_index)
		{
			ragdoll->thud_sound = TAG_BLOCK_GET_ELEMENT(&graph->sound_references, animation->sound_index,
				struct ragdoll_animation_sound_reference)->sound.index;
			/* (none played yet: the first landing is free to thud) */
			ragdoll->last_thud_time = NONE;
		}
	}
}

/* a body fall of the body's own (its animation graph's "bodyfalls"), one at
random, or NONE */
static long ragdoll_body_fall_sound(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct animation_graph const *graph;
	long found[16];
	short count = 0;
	short index;

	if (biped->object.animation.animation_graph_index == NONE)
		return NONE;
	graph = animation_graph_definition_get(biped->object.animation.animation_graph_index);
	if (!graph || !graph->sound_references.address)
		return NONE;
	for (index = 0; index < graph->sound_references.count && count < 16; index++)
	{
		long sound = TAG_BLOCK_GET_ELEMENT(&graph->sound_references, index, struct ragdoll_animation_sound_reference)->sound.index;
		char const *name = sound != NONE ? tag_get_name(sound) : NULL;

		if (name && strstr(name, "bodyfall"))
			found[count++] = sound;
	}
	return count ? found[local_random_range(0, count)] : NONE;
}

/* the torso's motion turned hard upward (it met the ground, hard): a thud,
as loud as the blow */
static void ragdoll_thud_update(
	struct ragdoll *ragdoll,
	long biped_index,
	real_vector3d const *velocity)
{
	long now = game_time_get();

	if (ragdoll->last_velocity_valid && valid_real_vector3d(velocity))
	{
		real_vector3d change;
		real speed_before = ragdoll_length(&ragdoll->last_velocity);
		real blow;

		ragdoll_sub((real_point3d const *)velocity, (real_point3d const *)&ragdoll->last_velocity, &change);
		/* (less what falling alone does) */
		change.k += global_gravity * ragdoll_parameters.gravity_scale;
		blow = ragdoll_length(&change);
		if (speed_before > 0.012f && blow > 0.012f && change.k > 0.4f * blow &&
			(ragdoll->last_thud_time == NONE || now - ragdoll->last_thud_time >= 10 || now < ragdoll->last_thud_time))
		{
			long sound = ragdoll->thud_sound != NONE ? ragdoll->thud_sound : ragdoll_body_fall_sound(biped_index);

			if (sound != NONE)
			{
				object_impulse_sound_new(biped_index, sound, 0, global_origin3d, global_forward3d, PIN(blow / 0.04f, 0.35f, 1.f));
				ragdoll->thud_sound = NONE;
				ragdoll->last_thud_time = now;
			}
		}
	}
	if (valid_real_vector3d(velocity))
	{
		ragdoll->last_velocity = *velocity;
		ragdoll->last_velocity_valid = TRUE;
	}
}

static void ragdoll_write(
	struct ragdoll *ragdoll,
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);
	real_matrix4x3 *matrices = (real_matrix4x3 *)object_header_block_get(
		biped_index,
		&biped->object.node_matrices);
	real_point3d minimum;
	real_point3d maximum;
	real_point3d center;
	real_vector3d half;
	real lowest;
	short index;

	minimum = maximum = ragdoll->nodes[0].position;
	for (index = 0; index < ragdoll->node_count; index++)
	{
		struct ragdoll_node const *node = &ragdoll->nodes[index];
		real_matrix4x3 *matrix = &matrices[index];

		matrix->scale = node->scale;
		matrix->forward = node->frame[0];
		matrix->left = node->frame[1];
		matrix->up = node->frame[2];
		matrix->position = node->position;

		minimum.x = MIN(minimum.x, node->position.x);
		minimum.y = MIN(minimum.y, node->position.y);
		minimum.z = MIN(minimum.z, node->position.z);
		maximum.x = MAX(maximum.x, node->position.x);
		maximum.y = MAX(maximum.y, node->position.y);
		maximum.z = MAX(maximum.z, node->position.z);
	}
	lowest = minimum.z;
	for (index = 0; index < ragdoll->body_count; index++)
		lowest = MIN(lowest, ragdoll->nodes[ragdoll->bodies[index].node].position.z - ragdoll->bodies[index].radius);

	center.x = (minimum.x + maximum.x) * 0.5f;
	center.y = (minimum.y + maximum.y) * 0.5f;
	center.z = (minimum.z + maximum.z) * 0.5f;
	ragdoll_sub(&maximum, &center, &half);

	/* the object where the body is: its origin on the ground beneath the
	root, its bounds about the whole skeleton (so it is drawn, lit, shot and
	found where it lies) */
	object_disconnect_from_map(biped_index);
	biped->object.position.x = ragdoll->nodes[0].position.x;
	biped->object.position.y = ragdoll->nodes[0].position.y;
	biped->object.position.z = lowest;
	biped->object.bounding_sphere_center = center;
	biped->object.bounding_sphere_radius = MAX(
		definition->object.bounding_radius,
		ragdoll_length(&half) + 0.25f);
	object_bounds_cache_update(biped_index, &biped->object.bounding_sphere_center, biped->object.bounding_sphere_radius);
	object_reconnect_to_map(biped_index, NULL);

	{
		float velocity[3] = { 0.f, 0.f, 0.f };

		ragdoll_physics_get_bone_velocity(ragdoll_slot(ragdoll), 0, velocity);
		biped->object.translational_velocity.i = velocity[0];
		biped->object.translational_velocity.j = velocity[1];
		biped->object.translational_velocity.k = velocity[2];
		if (!valid_real_vector3d(&biped->object.translational_velocity))
			biped->object.translational_velocity = *global_zero_vector3d;
		ragdoll_thud_update(ragdoll, biped_index, &biped->object.translational_velocity);
	}
	biped->object.angular_velocity = *global_zero_vector3d;
	SET_FLAG(biped->object.flags, _object_at_rest_bit, FALSE);
	biped->object.idle_ticks = 0;

	ragdoll->written_object_position = biped->object.position;
	ragdoll->written_root_position = matrices[0].position;
}

static void ragdoll_unsettled_note(
	struct ragdoll *ragdoll,
	long biped_index)
{
	float velocity[3] = { 0.f, 0.f, 0.f };
	real_vector3d moving;
	short index;
	short slot = NONE;

	if (ragdoll->nodes[0].body == NONE || !ragdoll_physics_exists(ragdoll_slot(ragdoll)))
		return;
	ragdoll_physics_get_bone_velocity(ragdoll_slot(ragdoll), ragdoll->nodes[0].body, velocity);
	moving.i = velocity[0];
	moving.j = velocity[1];
	moving.k = velocity[2];
	for (index = 0; index < RAGDOLL_MAXIMUM_UNSETTLED; index++)
	{
		if (ragdoll_unsettled[index].object_index == biped_index)
			slot = index;
	}
	if (!valid_real_vector3d(&moving))
		moving = *global_zero_vector3d;
	/* (come to rest by itself: settled) */
	if (ragdoll_freezing_at_rest)
	{
		if (slot != NONE)
			ragdoll_unsettled[slot].object_index = NONE;
		return;
	}
	if (slot == NONE)
	{
		for (index = 0; index < RAGDOLL_MAXIMUM_UNSETTLED && slot == NONE; index++)
		{
			if (ragdoll_unsettled[index].object_index == NONE ||
				!object_try_and_get(ragdoll_unsettled[index].object_index))
			{
				slot = index;
			}
		}
		if (slot == NONE)
			return;
		ragdoll_unsettled[slot].wakes = 0;
	}
	ragdoll_unsettled[slot].object_index = biped_index;
	ragdoll_unsettled[slot].velocity = moving;
	ragdoll_unsettled[slot].pending = TRUE;
}

static short ragdoll_active_count(
	void)
{
	short count = 0;
	short index;

	for (index = 0; index < RAGDOLL_MAXIMUM_ACTIVE; index++)
	{
		if (ragdolls[index].object_index != NONE)
			count++;
	}
	return count;
}

static void ragdoll_freeze(
	struct ragdoll *ragdoll,
	long biped_index)
{
	struct biped_datum *biped = biped_try_and_get(biped_index);

	ragdoll_unsettled_note(ragdoll, biped_index);

	if (biped && TEST_FLAG(biped->object.damage_flags, _object_dead_bit))
	{
		real_vector3d front;
		struct ragdoll_quaternion root;

		/* which way it lies, for a Flood form that gets up again */
		ragdoll_quaternion_from_frame(ragdoll->nodes[0].frame, &root);
		ragdoll_rotate(&root, &ragdoll->front_local, &front);
		SET_FLAG(biped->unit.animation.flags, _unit_animation_fallen_on_front_bit, front.k < 0.f);
		front.k = 0.f;
		if (ragdoll_normalize(&front) > 0.f)
		{
			biped->object.forward = front;
			biped->object.up = *global_up3d;
		}
		biped->object.translational_velocity = *global_zero_vector3d;
		biped->object.angular_velocity = *global_zero_vector3d;
		SET_FLAG(biped->object.flags, _object_at_rest_bit, TRUE);
	}
	ragdoll_free(ragdoll);
}

/* ---------- moved from outside */

boolean ragdoll_object_set_position(
	long object_index,
	real_point3d const *position)
{
	struct object_datum *object = object_get(object_index);
	struct biped_datum *biped;
	struct ragdoll *ragdoll;
	real_matrix4x3 *matrices;
	long matrix_count;
	real_vector3d moved;
	long index;

	if (object->object.type != _object_type_biped)
		return FALSE;
	/* (sitting in its seat: it stays with the vehicle) */
	{
		struct ragdoll_seated *seated = ragdoll_seated_find(object_index);

		if (seated && seated->started && !seated->released)
			return TRUE;
	}
	biped = biped_get(object_index);
	if (!TEST_FLAG(biped->biped.flags, BIPED_PORT_RAGDOLL_BIT) ||
		!TEST_FLAG(biped->object.damage_flags, _object_dead_bit) ||
		biped->object.parent_object_index != NONE)
	{
		return FALSE;
	}
	/* (turned only, or nowhere a body can be: it stays as it lies) */
	if (!position || !ragdoll_valid_point(position))
		return TRUE;

	ragdoll_sub(position, &biped->object.position, &moved);
	matrices = (real_matrix4x3 *)object_header_block_get(object_index, &biped->object.node_matrices);
	matrix_count = biped->object.node_matrices.size / (long)sizeof(real_matrix4x3);
	for (index = 0; index < matrix_count; index++)
		ragdoll_add_scaled(&matrices[index].position, &moved, 1.f);

	object_disconnect_from_map(object_index);
	biped->object.position = *position;
	ragdoll_add_scaled(&biped->object.bounding_sphere_center, &moved, 1.f);
	object_bounds_cache_update(object_index, &biped->object.bounding_sphere_center, biped->object.bounding_sphere_radius);
	object_reconnect_to_map(object_index, NULL);

	ragdoll = ragdoll_find(object_index);
	if (ragdoll)
	{
		float offset[3];

		offset[0] = moved.i;
		offset[1] = moved.j;
		offset[2] = moved.k;
		ragdoll_physics_translate(ragdoll_slot(ragdoll), offset);
		for (index = 0; index < ragdoll->node_count; index++)
			ragdoll_add_scaled(&ragdoll->nodes[index].position, &moved, 1.f);
		ragdoll->structure_valid = FALSE;
		ragdoll->written_object_position = biped->object.position;
		ragdoll->written_root_position = matrices[0].position;
	}
	return TRUE;
}

/* ---------- debug */

void ragdoll_render_debug(
	long biped_index)
{
	struct ragdoll const *ragdoll;
	short index;

	if (ragdoll_parameters.debug < 0.5f)
		return;
	ragdoll = ragdoll_find(biped_index);
	if (!ragdoll)
		return;

	for (index = 0; index < ragdoll->body_count; index++)
	{
		struct ragdoll_body const *body = &ragdoll->bodies[index];
		struct ragdoll_node const *node = &ragdoll->nodes[body->node];
		real_vector3d start;
		real_vector3d end;
		real_point3d a = node->position;
		real_point3d b = node->position;

		if (body->parent_body != NONE)
		{
			render_debug_line(
				TRUE,
				&ragdoll->nodes[ragdoll->bodies[body->parent_body].node].position,
				&node->position,
				body->joint == _ragdoll_physics_joint_hinge ? global_real_argb_yellow : global_real_argb_green);
		}
		/* its shape: the capsule's ends */
		ragdoll_to_world(node->frame, &body->capsule_start, &start);
		ragdoll_to_world(node->frame, &body->capsule_end, &end);
		ragdoll_add_scaled(&a, &start, 1.f);
		ragdoll_add_scaled(&b, &end, 1.f);
		render_debug_sphere(TRUE, &a, body->radius, global_real_argb_white);
		if (body->foot_radius > 0.f)
		{
			real_vector3d foot_a;
			real_vector3d foot_b;
			real_point3d fa = node->position;
			real_point3d fb = node->position;

			ragdoll_to_world(node->frame, &body->foot_start, &foot_a);
			ragdoll_to_world(node->frame, &body->foot_end, &foot_b);
			ragdoll_add_scaled(&fa, &foot_a, 1.f);
			ragdoll_add_scaled(&fb, &foot_b, 1.f);
			render_debug_line(TRUE, &fa, &fb, global_real_argb_red);
		}
		if (distance3d(&a, &b) > 1e-4f)
		{
			render_debug_sphere(TRUE, &b, body->radius, global_real_argb_white);
			render_debug_line(TRUE, &a, &b, global_real_argb_red);
		}
	}
}

/* ---------- console */

static char const *ragdoll_skip_space(
	char const *text)
{
	while (*text == ' ' || *text == '\t' || *text == '(' || *text == ')' || *text == '"')
		text++;
	return text;
}

static short ragdoll_read_word(
	char const **text,
	char *word,
	short size)
{
	short length = 0;

	*text = ragdoll_skip_space(*text);
	while (**text && **text != ' ' && **text != '\t' && **text != ')' && **text != '"' && length < size - 1)
		word[length++] = (char)tolower((unsigned char)*(*text)++);
	word[length] = 0;
	return length;
}

static void ragdoll_print_parameter(
	struct ragdoll_parameter_definition const *definition)
{
	console_printf(FALSE, "ragdoll %s %g  (%s)", definition->name, (double)*definition->value, definition->description);
}

long ragdoll_console_command(
	char const *expression)
{
	char const *text = ragdoll_skip_space(expression);
	char word[64];
	char value[64];
	short index;

	if (strncmp(text, "ragdoll", 7) != 0 || (text[7] && text[7] != ' ' && text[7] != '\t' && text[7] != ')'))
		return NONE;
	text += 7;
	ragdoll_parameters_initialize();

	if (!ragdoll_read_word(&text, word, sizeof(word)))
	{
		short active = 0;

		for (index = 0; index < RAGDOLL_MAXIMUM_ACTIVE; index++)
		{
			if (ragdolls[index].object_index != NONE)
				active++;
		}
		console_printf(FALSE, "ragdolls %s (Jolt Physics), %d simulated now. ragdoll <name> <value> sets one; ragdoll reset puts them back:",
			ragdoll_physics_failed ? "failed to start" : ragdolls_enabled() ? "on" : "off", active);
		for (index = 0; index < (short)NUMBEROF(ragdoll_parameter_definitions); index++)
			ragdoll_print_parameter(&ragdoll_parameter_definitions[index]);
		return TRUE;
	}

	/* on and off: Video Settings' Death Physics (config.toml's
	game.death_physics), kept */
	if (!strcmp(word, "on") || !strcmp(word, "off") || !strcmp(word, "1") || !strcmp(word, "0") ||
		!strcmp(word, "true") || !strcmp(word, "false"))
	{
		boolean on = !strcmp(word, "on") || !strcmp(word, "1") || !strcmp(word, "true");

		config_write("game.death_physics", on ? "ragdoll" : "classic");
		console_printf(FALSE, "ragdolls %s (death physics: %s)", ragdolls_enabled() ? "on" : "off", on ? "ragdoll" : "classic");
		return TRUE;
	}
	if (!strcmp(word, "reset"))
	{
		ragdoll_parameters_initialized = FALSE;
		ragdoll_parameters_initialize();
		console_printf(FALSE, "ragdoll parameters reset");
		return TRUE;
	}
	if (!strcmp(word, "freeze"))
	{
		for (index = 0; index < RAGDOLL_MAXIMUM_ACTIVE; index++)
		{
			if (ragdolls[index].object_index != NONE)
				ragdoll_freeze(&ragdolls[index], ragdolls[index].object_index);
		}
		console_printf(FALSE, "every ragdoll frozen");
		return TRUE;
	}

	for (index = 0; index < (short)NUMBEROF(ragdoll_parameter_definitions); index++)
	{
		struct ragdoll_parameter_definition const *definition = &ragdoll_parameter_definitions[index];

		if (strcmp(word, definition->name))
			continue;
		if (ragdoll_read_word(&text, value, sizeof(value)))
		{
			char *end;
			double number = strtod(value, &end);

			if (end == value || *end)
			{
				console_printf(FALSE, "ragdoll %s: %s is no number", definition->name, value);
				return FALSE;
			}
			*definition->value = PIN((real)number, definition->minimum, definition->maximum);
		}
		ragdoll_print_parameter(definition);
		return TRUE;
	}

	console_printf(FALSE, "ragdoll: no parameter %s (ragdoll alone lists them)", word);
	return FALSE;
}

#endif // RAGDOLLS
