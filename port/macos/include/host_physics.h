/* The physics bridge (docs/macos_host.md, "Physics"): the guest's ragdolls
and debris, simulated for show by Jolt in the host
(port/macos/host/physics). Once a tick the guest writes a batch in its
memory and gives it to the host with host_physics_step, with a buffer for
the results; the host checks the batch (port/macos/host/physics/
physics_bridge.c), carries it out, steps its world and writes back every
ragdoll's and body's pose, at one exit. The guest and the host both
include this header.

Everything is little-endian 32-bit words. Lengths and positions are in the
game's world units (HOST_PHYSICS_METRES_PER_UNIT metres each), z up;
masses in kilograms; impulses in kilogram world units a second;
rotations unit quaternions (x, y, z, w). "f" marks a float, "i" a signed
word, and the rest are unsigned.

A batch is a header and its commands:

	word 0  HOST_PHYSICS_BATCH_MAGIC
	word 1  HOST_PHYSICS_VERSION
	word 2  the batch's size in bytes, its header included
	word 3  the number of commands
	word 4  f the seconds to step the world after the commands (0: none)

A command's first word is its op and its size in words, itself included
(HOST_PHYSICS_COMMAND); its fields follow in the order listed below. A
command the host refuses is skipped and logged, and the next is read; a
size that runs past the batch ends it.

Ragdolls and bodies are named by the guest: an id from 1 to the kind's
limit, added once and removed before it is added again. The arrays a
command names by guest address (a ragdoll's parts and poses, the level's
triangles) are copied when the command is read.

The results the host writes:

	word 0  HOST_PHYSICS_RESULTS_MAGIC
	word 1  HOST_PHYSICS_VERSION
	word 2  the results' size in bytes, their header included
	word 3  the number of ragdolls
	word 4  the number of bodies
	word 5  flags (HOST_PHYSICS_RESULTS_*)
	word 6  the bodies awake

then each ragdoll, in the order of their ids: its id, its part count, 1 if
any part is awake, a mask of its broken joints (bit n: part n's), and each
part's pose; then each body: its id, 1 if awake, and its pose. */

#ifndef HOST_PHYSICS_H
#define HOST_PHYSICS_H

#define HOST_PHYSICS_BATCH_MAGIC 0x4250434fu /* "OCPB" */
#define HOST_PHYSICS_RESULTS_MAGIC 0x5250434fu /* "OCPR" */
#define HOST_PHYSICS_VERSION 1
#define HOST_PHYSICS_BATCH_HEADER_WORDS 5
#define HOST_PHYSICS_RESULTS_HEADER_WORDS 7

#define HOST_PHYSICS_COMMAND(op, words) ((unsigned int)(op) | ((unsigned int)(words) << 16))
#define HOST_PHYSICS_COMMAND_OP(word) ((word) & 0xffffu)
#define HOST_PHYSICS_COMMAND_WORDS(word) ((word) >> 16)

/* 10 feet */
#define HOST_PHYSICS_METRES_PER_UNIT 3.048f

#define HOST_PHYSICS_MAX_BATCH_LENGTH (1u << 20)
#define HOST_PHYSICS_RAGDOLLS_MAX 256
#define HOST_PHYSICS_BODIES_MAX 2048
#define HOST_PHYSICS_PARTS_MAX 32
#define HOST_PHYSICS_LEVEL_TRIANGLES_MAX (1u << 20)
#define HOST_PHYSICS_STEP_MAX 0.25f

/* a pose: f position x, y, z, f rotation x, y, z, w */
#define HOST_PHYSICS_POSE_WORDS 7

/* shapes: a sphere (its radius), a capsule along its frame's x (its radius
and half its cylinder's length) and a box (its half extents) */
#define HOST_PHYSICS_SHAPE_SPHERE 0
#define HOST_PHYSICS_SHAPE_CAPSULE 1
#define HOST_PHYSICS_SHAPE_BOX 2

/* motions: moved by the guest each tick (BODY_MOVE), pushing what it meets;
moved by the world */
#define HOST_PHYSICS_MOTION_KINEMATIC 0
#define HOST_PHYSICS_MOTION_DYNAMIC 1

/* A ragdoll's part, in the array RAGDOLL_ADD names: i its parent (an
earlier part; -1 for the first, and only it), its shape (type, f size x,
y, z, and f its frame's pose in the part's), f mass, and its joint to the
parent, at the part's origin: f twist axis x, y, z and f plane axis x, y,
z in the part's frame, f twist min and max, f half the swing cone's angle
about the normal and about the plane axis (radians) */
#define HOST_PHYSICS_PART_WORDS 23

/* ---------- the world */

/* everything removed, the level too (a map change, a saved game loaded) */
#define HOST_PHYSICS_OP_CLEAR 1
#define HOST_PHYSICS_CLEAR_WORDS 1
/* vertices' guest address (f x, y, z each), vertex count, indices' guest
address (three a triangle), triangle count: the level's collision mesh,
replacing the last; a vertex count of 0 removes it */
#define HOST_PHYSICS_OP_LEVEL 2
#define HOST_PHYSICS_LEVEL_WORDS 5

/* ---------- ragdolls */

/* id, part count, parts' guest address, rest poses' guest address, poses'
guest address (each part's frame, at rest and now), f friction,
f restitution, f velocity x, y, z */
#define HOST_PHYSICS_OP_RAGDOLL_ADD 3
#define HOST_PHYSICS_RAGDOLL_ADD_WORDS 12
/* id */
#define HOST_PHYSICS_OP_RAGDOLL_REMOVE 4
#define HOST_PHYSICS_RAGDOLL_REMOVE_WORDS 2
/* id, f frequency (Hz), target poses' guest address, count: the joints'
motors driven toward the parts' rotations in the targets */
#define HOST_PHYSICS_OP_RAGDOLL_DRIVE 5
#define HOST_PHYSICS_RAGDOLL_DRIVE_WORDS 5
/* id: the motors off */
#define HOST_PHYSICS_OP_RAGDOLL_LIMP 6
#define HOST_PHYSICS_RAGDOLL_LIMP_WORDS 2
/* id, part, f impulse x, y, z, f point x, y, z */
#define HOST_PHYSICS_OP_RAGDOLL_IMPULSE 7
#define HOST_PHYSICS_RAGDOLL_IMPULSE_WORDS 9
/* id, part: the joint to its parent broken */
#define HOST_PHYSICS_OP_RAGDOLL_BREAK 8
#define HOST_PHYSICS_RAGDOLL_BREAK_WORDS 3

/* ---------- single bodies: debris, and the kinematic stand-ins for
vehicles and living bipeds */

/* id, motion, shape type, f size x, y, z, f mass, f friction,
f restitution, pose, f velocity x, y, z, f angular velocity x, y, z
(radians a second) */
#define HOST_PHYSICS_OP_BODY_ADD 9
#define HOST_PHYSICS_BODY_ADD_WORDS 23
/* id, pose: a kinematic body, to reach the pose by the end of the step */
#define HOST_PHYSICS_OP_BODY_MOVE 10
#define HOST_PHYSICS_BODY_MOVE_WORDS 9
/* id */
#define HOST_PHYSICS_OP_BODY_REMOVE 11
#define HOST_PHYSICS_BODY_REMOVE_WORDS 2
/* id, f impulse x, y, z, f point x, y, z */
#define HOST_PHYSICS_OP_BODY_IMPULSE 12
#define HOST_PHYSICS_BODY_IMPULSE_WORDS 8

#define HOST_PHYSICS_OP_COUNT 13

/* ---------- results */

/* the buffer was too small: the ragdolls and bodies that did not fit are
left out */
#define HOST_PHYSICS_RESULTS_TRUNCATED 1u

#define HOST_PHYSICS_RESULT_RAGDOLL_WORDS 4
#define HOST_PHYSICS_RESULT_BODY_WORDS (2 + HOST_PHYSICS_POSE_WORDS)

#endif
