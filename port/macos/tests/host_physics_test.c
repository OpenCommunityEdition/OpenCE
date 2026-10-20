/*
 * host_physics_test.c - the host's physics world (port/macos/host/physics),
 * natively: tools/test_macos_physics.py builds it with the physics library
 * (build/macos/libhalo_physics.a) and runs one case, given as its argument.
 * It prints "ok", or what failed.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "physics/physics.h"

static int failures;

#define CHECK(condition)                                                                 \
    do {                                                                                 \
        if (!(condition)) {                                                              \
            printf("%s:%d: %s\n", __FILE__, __LINE__, #condition);                       \
            failures++;                                                                  \
        }                                                                                \
    } while (0)

static const physics_material material = {0.8f, 0.0f};
static const physics_pose identity = {{0, 0, 0}, {0, 0, 0, 1}};

static physics_world *new_world(void)
{
    physics_world_settings settings = {{0, 0, -9.81f}, 1024, 2};
    return physics_world_create(&settings);
}

/* a 40 m square at z = 0, as two triangles */
static void add_ground(physics_world *world)
{
    static const float vertices[] = {-20, -20, 0, 20, -20, 0, 20, 20, 0, -20, 20, 0};
    static const uint32_t indices[] = {0, 1, 2, 0, 2, 3};
    CHECK(physics_world_set_level(world, vertices, 4, indices, 2) == 0);
}

static physics_shape box_shape(float half)
{
    physics_shape shape;
    memset(&shape, 0, sizeof(shape));
    shape.type = PHYSICS_SHAPE_BOX;
    shape.size[0] = shape.size[1] = shape.size[2] = half;
    shape.offset = identity;
    return shape;
}

static void step_for(physics_world *world, float seconds)
{
    int step;
    for (step = 0; step < (int)(seconds * 30.0f); step++)
        CHECK(physics_world_step(world, 1.0f / 30.0f) == 0);
}

/* a box dropped from 2 m, its pose after five seconds */
static void drop_a_box(physics_pose *pose, int *active)
{
    physics_world *world = new_world();
    physics_shape shape = box_shape(0.25f);
    physics_pose start = {{0.3f, -0.2f, 2.0f}, {0.0998f, 0.0998f, 0.0f, 0.99f}};
    uint32_t body;
    float length = sqrtf(0.0998f * 0.0998f * 2 + 0.99f * 0.99f);
    int i;

    for (i = 0; i < 4; i++)
        start.rotation[i] /= length;
    add_ground(world);
    body = physics_body_add(world, &shape, &start, PHYSICS_MOTION_DYNAMIC, 4.0f, &material);
    CHECK(body != PHYSICS_NONE);
    CHECK(physics_body_active(world, body) == 1);
    step_for(world, 5.0f);
    CHECK(physics_body_get_pose(world, body, pose) == 0);
    *active = physics_body_active(world, body);
    physics_world_destroy(world);
}

static void box_comes_to_rest(void)
{
    physics_pose first, second;
    int active;

    drop_a_box(&first, &active);
    /* on the ground, flat on a face, and asleep */
    CHECK(fabsf(first.position[2] - 0.25f) < 0.01f);
    CHECK(active == 0);
    /* the same run again gives the same pose, to the bit */
    drop_a_box(&second, &active);
    CHECK(memcmp(&first, &second, sizeof(first)) == 0);
}

static void bad_arguments_are_refused(void)
{
    physics_world *world = new_world();
    physics_shape shape = box_shape(0.25f);
    physics_pose pose = identity;
    static const float vertices[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    static const uint32_t out_of_range[] = {0, 1, 3};
    static const uint32_t degenerate[] = {0, 1, 1};
    float nan = nanf("");
    physics_world_settings settings = {{0, 0, -9.81f}, 0, 1};

    CHECK(physics_world_create(&settings) == NULL);
    CHECK(physics_world_create(NULL) == NULL);
    CHECK(physics_world_set_level(world, vertices, 3, out_of_range, 1) == -1);
    CHECK(physics_world_set_level(world, vertices, 3, degenerate, 1) == -1);
    CHECK(physics_world_set_level(world, NULL, 3, degenerate, 1) == -1);
    CHECK(physics_world_step(world, 0.0f) == -1);
    CHECK(physics_world_step(world, nan) == -1);
    pose.position[0] = nan;
    CHECK(physics_body_add(world, &shape, &pose, PHYSICS_MOTION_DYNAMIC, 1.0f, &material) == PHYSICS_NONE);
    pose = identity;
    pose.rotation[3] = 0.0f;
    CHECK(physics_body_add(world, &shape, &pose, PHYSICS_MOTION_DYNAMIC, 1.0f, &material) == PHYSICS_NONE);
    pose = identity;
    shape.size[1] = -1.0f;
    CHECK(physics_body_add(world, &shape, &pose, PHYSICS_MOTION_DYNAMIC, 1.0f, &material) == PHYSICS_NONE);
    shape = box_shape(0.25f);
    shape.type = 7;
    CHECK(physics_body_add(world, &shape, &pose, PHYSICS_MOTION_DYNAMIC, 1.0f, &material) == PHYSICS_NONE);
    shape = box_shape(0.25f);
    CHECK(physics_body_add(world, &shape, &pose, 9, 1.0f, &material) == PHYSICS_NONE);
    CHECK(physics_body_add(world, &shape, &pose, PHYSICS_MOTION_DYNAMIC, 0.0f, &material) == PHYSICS_NONE);
    CHECK(physics_body_get_pose(world, 12345, &pose) == -1);
    CHECK(physics_body_get_pose(world, PHYSICS_NONE, &pose) == -1);
    CHECK(physics_body_active(world, 12345) == -1);
    CHECK(physics_world_bodies(world) == 0);
    physics_body_remove(world, 12345);
    physics_world_destroy(world);
}

static void a_full_world_refuses_bodies(void)
{
    physics_world_settings settings = {{0, 0, -9.81f}, 4, 1};
    physics_world *world = physics_world_create(&settings);
    physics_shape shape = box_shape(0.1f);
    uint32_t bodies[4];
    int i;

    for (i = 0; i < 4; i++) {
        bodies[i] = physics_body_add(world, &shape, &identity, PHYSICS_MOTION_DYNAMIC, 1.0f, &material);
        CHECK(bodies[i] != PHYSICS_NONE);
    }
    CHECK(physics_body_add(world, &shape, &identity, PHYSICS_MOTION_DYNAMIC, 1.0f, &material) == PHYSICS_NONE);
    physics_body_remove(world, bodies[2]);
    /* a removed body's handle names nothing, even once its slot is used again */
    CHECK(physics_body_active(world, bodies[2]) == -1);
    CHECK(physics_body_add(world, &shape, &identity, PHYSICS_MOTION_DYNAMIC, 1.0f, &material) != PHYSICS_NONE);
    CHECK(physics_body_active(world, bodies[2]) == -1);
    physics_world_destroy(world);
}

/* ---------- a ragdoll of five parts: a pelvis, a spine, a head and two
   thighs, the spine and head up z and the thighs down it, in the shape of a
   biped's (capsules along each part's x) */

enum { PELVIS, SPINE, HEAD, LEFT_THIGH, RIGHT_THIGH, PART_COUNT };

/* a part's frame turned so its x runs up (or down) z */
static const float up[4] = {0.0f, -0.70710678f, 0.0f, 0.70710678f};
static const float down[4] = {0.0f, 0.70710678f, 0.0f, 0.70710678f};

static void set_rotation(physics_pose *pose, const float rotation[4])
{
    memcpy(pose->rotation, rotation, sizeof(pose->rotation));
}

static void make_ragdoll(physics_part parts[PART_COUNT], physics_pose rest[PART_COUNT], float height)
{
    static const int32_t parents[PART_COUNT] = {-1, PELVIS, SPINE, PELVIS, PELVIS};
    static const float origins[PART_COUNT][3] = {
        {0, 0, 1.0f}, {0, 0, 1.1f}, {0, 0, 1.55f}, {0, 0.12f, 0.95f}, {0, -0.12f, 0.95f}};
    static const float radii[PART_COUNT] = {0.14f, 0.15f, 0.11f, 0.08f, 0.08f};
    static const float half_lengths[PART_COUNT] = {0.06f, 0.15f, 0.04f, 0.18f, 0.18f};
    static const float masses[PART_COUNT] = {12, 20, 5, 9, 9};
    int i;

    memset(parts, 0, sizeof(physics_part) * PART_COUNT);
    for (i = 0; i < PART_COUNT; i++) {
        physics_part *part = &parts[i];
        part->parent = parents[i];
        part->shape.type = PHYSICS_SHAPE_CAPSULE;
        part->shape.size[0] = radii[i];
        part->shape.size[1] = half_lengths[i];
        part->shape.offset = identity;
        /* the capsule from the joint along the part */
        part->shape.offset.position[0] = half_lengths[i] + (i == PELVIS ? 0.0f : radii[i]);
        part->mass = masses[i];
        part->twist_axis[0] = 1.0f;
        part->plane_axis[1] = 1.0f;
        part->twist_min = -0.4f;
        part->twist_max = 0.4f;
        part->swing_normal = 0.6f;
        part->swing_plane = 0.5f;
        rest[i] = identity;
        memcpy(rest[i].position, origins[i], sizeof(rest[i].position));
        rest[i].position[2] += height;
        set_rotation(&rest[i], i >= LEFT_THIGH ? down : up);
    }
}

static uint32_t add_ragdoll(physics_world *world, float height)
{
    physics_part parts[PART_COUNT];
    physics_pose rest[PART_COUNT];

    make_ragdoll(parts, rest, height);
    return physics_ragdoll_add(world, parts, PART_COUNT, rest, rest, &material);
}

/* a ragdoll dropped on the ground, its poses after a time */
static void drop_a_ragdoll(physics_pose poses[PART_COUNT], float *worst_error, int *active)
{
    physics_world *world = new_world();
    float push[3] = {0, 60, 0}, at[3] = {0, 0, 1.6f};
    uint32_t ragdoll;
    int step;

    add_ground(world);
    ragdoll = add_ragdoll(world, 0.0f);
    CHECK(ragdoll != PHYSICS_NONE);
    CHECK(physics_world_bodies(world) == PART_COUNT);
    CHECK(physics_ragdoll_go_limp(world, ragdoll) == 0);
    CHECK(physics_ragdoll_add_impulse(world, ragdoll, HEAD, push, at) == 0);
    *worst_error = 0.0f;
    for (step = 0; step < 8 * 30; step++) {
        float error;
        CHECK(physics_world_step(world, 1.0f / 30.0f) == 0);
        error = physics_ragdoll_limit_error(world, ragdoll);
        CHECK(error >= 0.0f);
        if (error > *worst_error)
            *worst_error = error;
    }
    CHECK(physics_ragdoll_get_poses(world, ragdoll, poses, PART_COUNT) == 0);
    *active = physics_ragdoll_active(world, ragdoll);
    physics_world_destroy(world);
}

static void ragdoll_falls_and_keeps_its_limits(void)
{
    physics_pose first[PART_COUNT], second[PART_COUNT];
    float worst_error;
    int active, i;

    drop_a_ragdoll(first, &worst_error, &active);
    /* fallen over, lying on the ground, asleep, its joints within (a
       solver's slack of) their limits */
    CHECK(first[HEAD].position[2] < 0.4f);
    for (i = 0; i < PART_COUNT; i++)
        CHECK(first[i].position[2] > 0.0f);
    CHECK(worst_error < 0.1f);
    CHECK(active == 0);
    /* and again, to the bit */
    drop_a_ragdoll(second, &worst_error, &active);
    CHECK(memcmp(first, second, sizeof(first)) == 0);
}

static void a_driven_ragdoll_holds_its_pose(void)
{
    physics_world *world = new_world();
    physics_part parts[PART_COUNT];
    physics_pose rest[PART_COUNT], poses[PART_COUNT];
    uint32_t ragdoll;
    float rise;

    add_ground(world);
    /* the pelvis a box on the ground's level would stand on: it is held up
       by nothing, so the drive is tested by the spine and head staying above
       the pelvis as it all falls */
    make_ragdoll(parts, rest, 0.0f);
    ragdoll = physics_ragdoll_add(world, parts, PART_COUNT, rest, rest, &material);
    CHECK(physics_ragdoll_drive(world, ragdoll, rest, PART_COUNT, 6.0f) == 0);
    step_for(world, 0.5f);
    CHECK(physics_ragdoll_get_poses(world, ragdoll, poses, PART_COUNT) == 0);
    rise = poses[HEAD].position[2] - poses[PELVIS].position[2];
    CHECK(rise > 0.45f);
    CHECK(physics_ragdoll_drive(world, ragdoll, rest, PART_COUNT - 1, 6.0f) == -1);
    CHECK(physics_ragdoll_drive(world, ragdoll, rest, PART_COUNT, 0.0f) == -1);
    physics_world_destroy(world);
}

static void a_broken_joint_lets_a_limb_go(void)
{
    physics_world *world = new_world();
    physics_pose poses[PART_COUNT];
    float fling[3] = {0, 0, 0};
    uint32_t ragdoll;
    float dx, dy, dz;

    add_ground(world);
    ragdoll = add_ragdoll(world, 0.0f);
    CHECK(physics_ragdoll_joint_broken(world, ragdoll, HEAD) == 0);
    CHECK(physics_ragdoll_break_joint(world, ragdoll, HEAD) == 0);
    CHECK(physics_ragdoll_joint_broken(world, ragdoll, HEAD) == 1);
    CHECK(physics_ragdoll_break_joint(world, ragdoll, HEAD) == -1);
    CHECK(physics_ragdoll_break_joint(world, ragdoll, PELVIS) == -1);
    CHECK(physics_ragdoll_break_joint(world, ragdoll, PART_COUNT) == -1);
    CHECK(physics_ragdoll_get_poses(world, ragdoll, poses, PART_COUNT) == 0);
    fling[0] = 40.0f;
    CHECK(physics_ragdoll_add_impulse(world, ragdoll, HEAD, fling, poses[HEAD].position) == 0);
    step_for(world, 3.0f);
    CHECK(physics_ragdoll_get_poses(world, ragdoll, poses, PART_COUNT) == 0);
    /* the head far from where the spine's end is */
    dx = poses[HEAD].position[0] - poses[SPINE].position[0];
    dy = poses[HEAD].position[1] - poses[SPINE].position[1];
    dz = poses[HEAD].position[2] - poses[SPINE].position[2];
    CHECK(sqrtf(dx * dx + dy * dy + dz * dz) > 1.0f);
    CHECK(physics_ragdoll_limit_error(world, ragdoll) < 0.1f);
    physics_world_destroy(world);
}

static void bad_ragdolls_are_refused(void)
{
    physics_world *world = new_world();
    physics_part parts[PART_COUNT];
    physics_pose rest[PART_COUNT], poses[PART_COUNT];
    uint32_t ragdoll;

    make_ragdoll(parts, rest, 0.0f);
    CHECK(physics_ragdoll_add(world, parts, 0, rest, rest, &material) == PHYSICS_NONE);
    CHECK(physics_ragdoll_add(world, parts, PHYSICS_RAGDOLL_PARTS_MAX + 1, rest, rest, &material) == PHYSICS_NONE);
    parts[HEAD].parent = HEAD;
    CHECK(physics_ragdoll_add(world, parts, PART_COUNT, rest, rest, &material) == PHYSICS_NONE);
    parts[HEAD].parent = 9;
    CHECK(physics_ragdoll_add(world, parts, PART_COUNT, rest, rest, &material) == PHYSICS_NONE);
    make_ragdoll(parts, rest, 0.0f);
    parts[SPINE].parent = -1;
    CHECK(physics_ragdoll_add(world, parts, PART_COUNT, rest, rest, &material) == PHYSICS_NONE);
    make_ragdoll(parts, rest, 0.0f);
    parts[SPINE].plane_axis[0] = 1.0f;
    parts[SPINE].plane_axis[1] = 0.0f;
    CHECK(physics_ragdoll_add(world, parts, PART_COUNT, rest, rest, &material) == PHYSICS_NONE);
    make_ragdoll(parts, rest, 0.0f);
    parts[SPINE].twist_min = 0.5f;
    CHECK(physics_ragdoll_add(world, parts, PART_COUNT, rest, rest, &material) == PHYSICS_NONE);
    make_ragdoll(parts, rest, 0.0f);
    parts[SPINE].mass = nanf("");
    CHECK(physics_ragdoll_add(world, parts, PART_COUNT, rest, rest, &material) == PHYSICS_NONE);
    CHECK(physics_world_bodies(world) == 0);

    ragdoll = add_ragdoll(world, 0.0f);
    CHECK(ragdoll != PHYSICS_NONE);
    CHECK(physics_ragdoll_get_poses(world, ragdoll, poses, PART_COUNT + 1) == -1);
    /* a ragdoll's parts are not single bodies */
    CHECK(physics_body_active(world, 0) == -1);
    physics_ragdoll_remove(world, ragdoll);
    CHECK(physics_world_bodies(world) == 0);
    /* a removed ragdoll's handle names nothing, even once its slot is used again */
    CHECK(physics_ragdoll_active(world, ragdoll) == -1);
    CHECK(add_ragdoll(world, 0.0f) != ragdoll);
    CHECK(physics_ragdoll_active(world, ragdoll) == -1);
    CHECK(physics_ragdoll_limit_error(world, ragdoll) == -1.0f);
    CHECK(physics_ragdoll_break_joint(world, PHYSICS_NONE, HEAD) == -1);
    physics_world_destroy(world);
}

static void a_kinematic_body_pushes_debris(void)
{
    physics_world *world = new_world();
    physics_shape wall = box_shape(0.5f), crate = box_shape(0.2f);
    physics_pose pose = identity, crate_pose = identity;
    uint32_t pusher, piece;
    int step;

    add_ground(world);
    pose.position[2] = 0.5f;
    pose.position[0] = -2.0f;
    crate_pose.position[2] = 0.2f;
    pusher = physics_body_add(world, &wall, &pose, PHYSICS_MOTION_KINEMATIC, 1000.0f, &material);
    piece = physics_body_add(world, &crate, &crate_pose, PHYSICS_MOTION_DYNAMIC, 2.0f, &material);
    CHECK(pusher != PHYSICS_NONE && piece != PHYSICS_NONE);
    /* a dynamic body cannot be moved; a kinematic one cannot be pushed */
    CHECK(physics_body_move(world, piece, &pose, 1.0f / 30.0f) == -1);
    for (step = 0; step < 60; step++) {
        pose.position[0] += 2.0f / 30.0f;
        CHECK(physics_body_move(world, pusher, &pose, 1.0f / 30.0f) == 0);
        CHECK(physics_world_step(world, 1.0f / 30.0f) == 0);
    }
    CHECK(physics_body_get_pose(world, piece, &crate_pose) == 0);
    /* the wall's front is at x = 2.5; the crate ahead of it */
    CHECK(crate_pose.position[0] > 2.5f);
    physics_world_destroy(world);
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        void (*run)(void);
    } cases[] = {
        {"box_comes_to_rest", box_comes_to_rest},
        {"bad_arguments_are_refused", bad_arguments_are_refused},
        {"a_full_world_refuses_bodies", a_full_world_refuses_bodies},
        {"ragdoll_falls_and_keeps_its_limits", ragdoll_falls_and_keeps_its_limits},
        {"a_driven_ragdoll_holds_its_pose", a_driven_ragdoll_holds_its_pose},
        {"a_broken_joint_lets_a_limb_go", a_broken_joint_lets_a_limb_go},
        {"bad_ragdolls_are_refused", bad_ragdolls_are_refused},
        {"a_kinematic_body_pushes_debris", a_kinematic_body_pushes_debris},
    };
    unsigned i;

    if (argc > 1 && strcmp(argv[1], "--list") == 0) {
        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
            printf("%s\n", cases[i].name);
        return 0;
    }
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        if (argc > 1 && strcmp(argv[1], cases[i].name) != 0)
            continue;
        cases[i].run();
    }
    if (failures == 0)
        printf("ok\n");
    return failures != 0;
}
