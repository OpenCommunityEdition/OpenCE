/*
 * physics.h - the host's physics world, for show only (docs/macos_host.md,
 * "Physics"), and the native engine's (engine/crates/physics, which builds
 * this file and Jolt into the engine).
 *
 * A C interface to Jolt Physics (port/third_party/jolt), in physics.cpp, so
 * that the rest of the host stays C. A world holds the level's collision
 * mesh, the ragdolls of dead bipeds, the debris pieces and the kinematic
 * bodies that stand for vehicles and living bipeds. It is stepped on one
 * thread, in Jolt's cross-platform deterministic mode: the same calls give
 * the same poses on every run.
 *
 * Units are metres, seconds and kilograms; the caller chooses the axes (the
 * host uses the game's, z up) through the gravity it gives. Rotations are
 * unit quaternions (x, y, z, w).
 *
 * Every call checks its arguments: a handle that names nothing, a count out
 * of range or a number that is not finite is refused (PHYSICS_NONE or -1),
 * never trusted.
 */

#ifndef OPENCE_PHYSICS_H
#define OPENCE_PHYSICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PHYSICS_NONE 0xffffffffu
/* the most parts a ragdoll has */
#define PHYSICS_RAGDOLL_PARTS_MAX 32

typedef struct physics_world physics_world;

typedef struct physics_pose {
    float position[3];
    float rotation[4];
} physics_pose;

enum physics_shape_type {
    PHYSICS_SHAPE_SPHERE,
    /* along its frame's x axis */
    PHYSICS_SHAPE_CAPSULE,
    PHYSICS_SHAPE_BOX,
};

typedef struct physics_shape {
    int32_t type;
    /* a sphere's radius; a capsule's radius and half its cylinder's length;
       a box's half extents */
    float size[3];
    /* the shape's frame in its body's */
    physics_pose offset;
} physics_shape;

enum physics_motion {
    /* moved by the caller each step (physics_body_move), pushing what it meets */
    PHYSICS_MOTION_KINEMATIC,
    /* moved by the world: a debris piece */
    PHYSICS_MOTION_DYNAMIC,
};

typedef struct physics_material {
    float friction;
    float restitution;
} physics_material;

/* A ragdoll's part: one body, joined to an earlier part (its parent) at its
   own frame's origin. The joint twists about twist_axis and swings about
   the plane axis and the normal (their cross product), each given in the
   part's frame; the angles are in radians. */
typedef struct physics_part {
    int32_t parent;
    physics_shape shape;
    float mass;
    float twist_axis[3];
    float plane_axis[3];
    float twist_min;
    float twist_max;
    /* half the cone's angle about the normal, and about the plane axis */
    float swing_normal;
    float swing_plane;
} physics_part;

typedef struct physics_world_settings {
    float gravity[3];
    /* the most bodies of every kind at once */
    uint32_t bodies_max;
    /* collision steps a physics_world_step takes */
    uint32_t collision_steps;
} physics_world_settings;

physics_world *physics_world_create(const physics_world_settings *settings);
void physics_world_destroy(physics_world *world);

/* Replaces the level's collision mesh (a structure BSP's triangles; three
   indices each). Degenerate triangles are dropped. 0, or -1 for a mesh
   refused (an index out of range, a vertex not finite, no triangle left). */
int physics_world_set_level(physics_world *world, const float *vertices, uint32_t vertex_count,
                            const uint32_t *indices, uint32_t triangle_count);
void physics_world_clear_level(physics_world *world);

/* 0, or -1 for a step that is not positive and finite */
int physics_world_step(physics_world *world, float seconds);

/* bodies whose simulation is awake (the rest sleep, at no cost) */
uint32_t physics_world_active_bodies(const physics_world *world);
uint32_t physics_world_bodies(const physics_world *world);

/* ---------- single bodies: debris and kinematic bodies */

uint32_t physics_body_add(physics_world *world, const physics_shape *shape, const physics_pose *pose,
                          int32_t motion, float mass, const physics_material *material);
void physics_body_remove(physics_world *world, uint32_t body);
/* 0, or -1 for a body that is not one */
int physics_body_get_pose(const physics_world *world, uint32_t body, physics_pose *pose);
int physics_body_get_velocity(const physics_world *world, uint32_t body, float linear[3], float angular[3]);
/* a kinematic body, to reach the pose by the end of the next step */
int physics_body_move(physics_world *world, uint32_t body, const physics_pose *pose, float seconds);
/* an impulse (kg m/s) at a point in the world */
int physics_body_add_impulse(physics_world *world, uint32_t body, const float impulse[3], const float point[3]);
int physics_body_set_velocity(physics_world *world, uint32_t body, const float linear[3], const float angular[3]);
/* 1 awake, 0 asleep, -1 not a body */
int physics_body_active(const physics_world *world, uint32_t body);

/* ---------- casts (the native engine's: engine/crates/physics) */

/* what a cast may hit */
enum physics_cast_layers {
    PHYSICS_CAST_LEVEL = 1,
    /* ragdoll parts, debris and kinematic bodies */
    PHYSICS_CAST_BODIES = 2,
};

typedef struct physics_hit {
    /* how far along the cast, 0 to 1 (0 for a shape that starts touching) */
    float fraction;
    float point[3];
    /* the surface's, toward the cast's start */
    float normal[3];
    /* the body hit: a body's handle, or PHYSICS_NONE for the level's mesh
       and a ragdoll's part */
    uint32_t body;
    /* 1 for the level's mesh */
    int32_t level;
} physics_hit;

/* The first surface on the segment from origin to origin + direction.
   1 hit, 0 none, -1 for arguments refused. */
int physics_world_cast_ray(const physics_world *world, const float origin[3], const float direction[3],
                           uint32_t layers, physics_hit *hit);
/* The first surface a shape meets moved from pose by direction; the same. */
int physics_world_cast_shape(const physics_world *world, const physics_shape *shape, const physics_pose *pose,
                             const float direction[3], uint32_t layers, physics_hit *hit);

/* ---------- characters: a biped's collision as it walks (the native
   engine's movement sets its velocity; the character slides it along what
   it meets, steps it up onto ledges and keeps it on the floor) */

typedef struct physics_character_settings {
    /* its pill: its radius, and its height from its feet to its top */
    float radius;
    float height;
    /* the steepest ground it stands on (radians) */
    float maximum_slope;
    /* the highest ledge it steps onto, and how far down it follows the
       floor as it walks off a step (0 for neither) */
    float step_up;
    float stick_down;
    /* what it collides with (physics_cast_layers) */
    uint32_t layers;
} physics_character_settings;

enum physics_ground {
    PHYSICS_GROUND_STANDING,
    /* on ground steeper than its maximum slope: sliding */
    PHYSICS_GROUND_STEEP,
    /* touching ground that does not hold it up */
    PHYSICS_GROUND_UNSUPPORTED,
    PHYSICS_GROUND_AIRBORNE,
};

typedef struct physics_character_state {
    /* its feet */
    float position[3];
    /* as the collisions left it */
    float velocity[3];
    /* its ground's normal (up when airborne) */
    float ground_normal[3];
    int32_t ground;
} physics_character_state;

/* a character with its feet at position, or PHYSICS_NONE */
uint32_t physics_character_add(physics_world *world, const physics_character_settings *settings,
                               const float position[3]);
void physics_character_remove(physics_world *world, uint32_t character);
/* Moves it at velocity for seconds, colliding; then its state. 0, or -1. */
int physics_character_move(physics_world *world, uint32_t character, const float velocity[3], float seconds,
                           physics_character_state *state);
/* puts its feet at position, as it stands still; 0, or -1 */
int physics_character_set_position(physics_world *world, uint32_t character, const float position[3]);
/* its state, unmoved; 0, or -1 */
int physics_character_get_state(const physics_world *world, uint32_t character, physics_character_state *state);
/* Its pill's new height (crouching or standing). 1 changed, 0 for no room
   (standing up beneath a ceiling), -1 refused. */
int physics_character_set_height(physics_world *world, uint32_t character, float height);

/* ---------- ragdolls */

/* A ragdoll of count parts, built in rest (each part's frame with the
   joints at rest, so that the limits centre on them) and placed at pose
   (each part's frame now, in the world). Its parts do not collide with
   their parents; they do with every other body. */
uint32_t physics_ragdoll_add(physics_world *world, const physics_part *parts, uint32_t count,
                             const physics_pose *rest, const physics_pose *pose,
                             const physics_material *material);
void physics_ragdoll_remove(physics_world *world, uint32_t ragdoll);
/* each part's frame in the world; 0, or -1 */
int physics_ragdoll_get_poses(const physics_world *world, uint32_t ragdoll, physics_pose *poses, uint32_t count);
/* Drives each joint's motor toward its part's rotation in target (world
   frames, as physics_ragdoll_get_poses gives), springing at frequency (Hz),
   until physics_ragdoll_go_limp. */
int physics_ragdoll_drive(physics_world *world, uint32_t ragdoll, const physics_pose *target, uint32_t count,
                          float frequency);
/* the motors off: only the joints' friction holds the body */
int physics_ragdoll_go_limp(physics_world *world, uint32_t ragdoll);
int physics_ragdoll_add_impulse(physics_world *world, uint32_t ragdoll, uint32_t part, const float impulse[3],
                                const float point[3]);
/* every part moving at a velocity (the death animation's motion) */
int physics_ragdoll_set_velocity(physics_world *world, uint32_t ragdoll, const float linear[3]);
/* Breaks the joint between a part and its parent: the part and its
   children go on as a limb of their own, colliding with the rest. 0, or -1
   for the root, a part out of range or a joint already broken. */
int physics_ragdoll_break_joint(physics_world *world, uint32_t ragdoll, uint32_t part);
/* 1 broken, 0 whole, -1 */
int physics_ragdoll_joint_broken(const physics_world *world, uint32_t ragdoll, uint32_t part);
/* 1 if any part is awake, 0 asleep, -1 */
int physics_ragdoll_active(const physics_world *world, uint32_t ragdoll);
/* the furthest any whole joint is past its limits, in radians (0 inside
   them), or -1 for a ragdoll that is not one */
float physics_ragdoll_limit_error(const physics_world *world, uint32_t ragdoll);

#ifdef __cplusplus
}
#endif

#endif
