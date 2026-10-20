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
	/* the ragdolls in the host's world at once; past it the oldest goes back
	to the stock game's corpse */
	RAGDOLLS_MAX = 32,
	/* how long the joints' motors follow the death animation, then let go */
	RAGDOLL_DRIVE_TICKS = 12,
	/* the pushes queued in a tick; past it a hit pushes nothing */
	RAGDOLL_IMPULSES_MAX = 32,
	/* the living bipeds and vehicles near ragdolls that the host's world has
	as kinematic bodies, for the ragdolls to meet; named to the host by their
	index plus one */
	RAGDOLL_STAND_INS_MAX = 16,
	/* the joints torn in a tick; past it a blast tears no more */
	RAGDOLL_BREAKS_MAX = 32,
	/* the joints one blast tears in a body, the worst hurt first */
	RAGDOLL_BREAKS_PER_HIT = 2,
};

/* how near a ragdoll (world units, past the object's bounding sphere) a
biped or vehicle is given a stand-in */
#define RAGDOLL_STAND_IN_DISTANCE 3.0f
/* the lift damage.c gives an acceleration's direction before it pushes an
object */
#define RAGDOLL_PUSH_LIFT 0.45f

/* the joints' motors' spring (Hz) while they follow the animation */
#define RAGDOLL_DRIVE_FREQUENCY 8.0f

/* what a tick does with a ragdoll */
enum
{
	_ragdoll_action_none,
	/* the motors toward the biped's node matrices now */
	_ragdoll_action_drive,
	/* the motors off */
	_ragdoll_action_limp,
	/* out of the world: its biped is gone */
	_ragdoll_action_remove,
};

/* a part's frame, as the host's world has them: a point, and the rotation
from the part's frame to the world's as a unit quaternion of the usual sense
(the game's quaternions turn the other way) */
struct ragdoll_pose
{
	real_point3d position;
	real_quaternion rotation;
};

/* a hit on a biped: the damage's direction (unit length), the speed its
instantaneous acceleration gives (world units a second, scaled as the
damage is), and where it struck; and an explosion's, what it does at a
point (ragdoll_hit_damage_at): a grenade's, a high explosive's or a mounted
weapon's damage over an area (not melee's, nor a vehicle's collision, which
damage.c also gives an area) */
struct ragdoll_hit
{
	long object_index;
	long time;
	real_vector3d direction;
	real speed;
	real_point3d point;
	boolean explosion;
	/* whether the damage falls off with the distance */
	boolean scaled;
	real_point3d epicenter;
	/* the damage at the cutoff and at the centre (the mean of its bounds),
	times the damage's multiplier */
	real damage_minimum;
	real damage_maximum;
	real falloff_radius;
	real cutoff_radius;
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

/* a push for the host: an impulse (kilograms times world units a second) on
a ragdoll's part, at a point in the world */
struct ragdoll_impulse
{
	long id;
	short part;
	short pad;
	real_vector3d impulse;
	real_point3d point;
};

/* a joint for the host to tear: the one between a ragdoll's part and its
parent part */
struct ragdoll_break
{
	long id;
	short part;
	short pad;
};

/* a dead biped's ragdoll, named to the host by its index plus one */
struct ragdoll
{
	boolean used;
	long object_index;
	struct ragdoll_body const *body;
	long start_time;
	boolean limp;
	/* the host's poses for its parts, once it has given them */
	boolean posed;
	boolean awake;
	unsigned long broken_joints;
	/* its parts' frames: as the biped died, until the host gives them */
	struct ragdoll_pose poses[RAGDOLL_BODY_PARTS_MAX];
};

/* whether a biped dies as a ragdoll (game.ragdolls, and the host's physics) */
boolean ragdolls_enabled(void);
/* whether explosions tear ragdolls' joints (game.dismemberment, with
game.ragdolls) */
boolean ragdolls_dismemberment_enabled(void);

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

/* the rotation of a node matrix */
void ragdoll_pose_from_matrix(real_matrix4x3 const *matrix, struct ragdoll_pose *pose);

/* A ragdoll for a death: its id, or NONE if its biped has one already. With
every ragdoll in use the oldest is taken, and its id given in `replaced`
(else NONE) for the host to remove first. */
long ragdolls_start(struct ragdoll_death const *death, long *replaced);
/* the ragdoll of an id, or NULL */
struct ragdoll *ragdolls_get(long id);
/* what this tick does with a ragdoll (_ragdoll_action_*), a limp or remove
taken as done */
short ragdolls_action(long id);
void ragdolls_stop(long id);
/* the ragdoll of an object, or NULL */
struct ragdoll const *ragdolls_of_object(long object_index);
/* The part at the top of the piece a part is in, its joints torn as the
host last reported (broken_joints): the nearest of it and its ancestors
whose joint to its parent is torn, or the root. NONE for no part. */
short ragdoll_part_piece(struct ragdoll const *ragdoll, short part);

/* The node matrices an object is drawn with: its ragdoll's, once the host
has posed it, each node carried with its part as the game's matrices have
it from that part's node; else the game's, `nodes`. The game's own stay as
they are, the ragdolls being for show. */
real_matrix4x3 const *ragdolls_node_matrices(long object_index, real_matrix4x3 const *nodes);
/* The draws of an object's model: one for each piece of its ragdoll as torn
(the root's first, then each torn part's in order), else one. */
long ragdolls_piece_count(long object_index);
/* A piece's draw from `nodes` (the matrices drawn, or interpolation's): the
piece's own nodes as they are, every other node's vertices at one point, the
stump where that piece tore from this one or else this piece's own joint;
`nodes` itself for an object drawn whole; NULL past its pieces. */
real_matrix4x3 const *ragdolls_piece_node_matrices(long object_index, real_matrix4x3 const *nodes, long piece);

/* the part whose shape's centre is nearest a point */
short ragdoll_part_nearest(struct ragdoll_body const *body, struct ragdoll_pose const *poses,
	real_point3d const *point);

/* the pushes queued since the last clear: each ragdoll's killing hit as it
starts, and each later hit on its biped */
long ragdolls_impulse_count(void);
struct ragdoll_impulse const *ragdolls_impulse_get(long index);
void ragdolls_impulses_clear(void);

/* The damage an explosion's hit does at a point, as damage.c's
area-of-effect damage works it out, with the mean of the damage's bounds
for its chance; 0 past the cutoff and for other hits. */
real ragdoll_hit_damage_at(struct ragdoll_hit const *hit, real_point3d const *point);

/* What of that damage tears a joint at a point: less the farther the point
is from the explosion, all of it at the epicenter and none at the cutoff, so
that the limbs nearest a blast go first, though its damage is the same out
to the falloff. */
real ragdoll_hit_tearing_at(struct ragdoll_hit const *hit, real_point3d const *point);

/* the joints torn since the last clear: a dying biped's by the explosion
that killed it, and a ragdoll's by each later one on its biped */
long ragdolls_break_count(void);
struct ragdoll_break const *ragdolls_break_get(long index);
void ragdolls_breaks_clear(void);

/* Whether a sphere is near enough a ragdoll to need a stand-in. */
boolean ragdolls_near(real_point3d const *center, real radius);
/* A tick's stand-ins: begun, then each object near a ragdoll given its id
(and whether it is new, for the host to add) or NONE with every one in use;
then each id not given this tick taken back, for the host to remove. */
void ragdolls_stand_ins_begin(void);
long ragdolls_stand_in(long object_index, boolean *added);
boolean ragdolls_stand_in_unseen(long id);

void ragdolls_dispose_from_old_map(void);

#endif
