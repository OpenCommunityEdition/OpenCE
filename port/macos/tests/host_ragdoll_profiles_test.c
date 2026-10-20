/*
 * host_ragdoll_profiles_test.c - the stock bipeds' ragdolls on the physics
 * bridge (port/macos/host/physics/physics_bridge.c), natively, with guest
 * memory of its own: tools/test_ragdoll_physics.py writes the bodies, in
 * their models' default poses, and the frag grenade's and the warthog's
 * values, all read from the maps, into ragdoll_profiles.inc, builds this
 * with the physics library and runs one case, given as its argument. It
 * prints a line for each body it measures, then "ok", or what failed.
 *
 * Everything is in the game's world units, as the guest sends it, and a
 * batch steps a tick (1/30 s).
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "host_physics.h"
#include "physics/physics_bridge.h"

struct profile {
    const char *name;
    uint32_t part_count;
    float mass;
    float acceleration_scale;
    /* each part as physics_jolt_part_words writes it, and its frame */
    uint32_t parts[HOST_PHYSICS_PARTS_MAX * HOST_PHYSICS_PART_WORDS];
    float poses[HOST_PHYSICS_PARTS_MAX * HOST_PHYSICS_POSE_WORDS];
};

#include "ragdoll_profiles.inc"

#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define TICK (1.0f / 30.0f)

static int failures;

#define CHECK(condition, ...)                                                            \
    do {                                                                                 \
        if (!(condition)) {                                                              \
            printf("%s:%d: %s: ", __FILE__, __LINE__, #condition);                       \
            printf(__VA_ARGS__);                                                         \
            printf("\n");                                                                \
            failures++;                                                                  \
        }                                                                                \
    } while (0)

/* the guest's memory: 4 MB at GUEST_BASE */
#define GUEST_BASE 0x00100000u
#define GUEST_SIZE (4u << 20)
static unsigned char guest[GUEST_SIZE];

#define BATCH (GUEST_BASE + 0x000000)
#define RESULTS (GUEST_BASE + 0x100000)
#define RESULTS_SIZE 0x100000u
#define ARRAYS (GUEST_BASE + 0x200000)
/* each ragdoll's parts and poses, by id */
#define RAGDOLL_ARRAYS(id) (ARRAYS + 0x1000 + (uint32_t)(id) * 0x2000)

static void *guest_pointer(void *context, uint32_t address, uint32_t size)
{
    uint64_t end = (uint64_t)address + size;

    (void)context;
    if (address < GUEST_BASE || end > (uint64_t)GUEST_BASE + GUEST_SIZE)
        return NULL;
    return guest + (address - GUEST_BASE);
}

static const physics_bridge_memory memory = {guest_pointer, NULL};

static void *at(uint32_t address) { return guest + (address - GUEST_BASE); }

static void refused(void *context, uint32_t offset, uint32_t op, const char *reason)
{
    (void)context;
    printf("refused: op %u at %u: %s\n", op, offset, reason);
    failures++;
}

/* ---------- batches */

static uint32_t batch_length, batch_commands;

static uint32_t *command(uint32_t op, uint32_t words)
{
    uint32_t *fields = (uint32_t *)at(BATCH) + batch_length;

    fields[0] = HOST_PHYSICS_COMMAND(op, words);
    memset(fields + 1, 0, (words - 1) * 4);
    batch_length += words;
    batch_commands++;
    return fields + 1;
}

/* the commands written since the last, and a tick */
static void tick(physics_bridge *bridge)
{
    uint32_t *words = at(BATCH);
    char reason[128];
    int result;

    words[0] = HOST_PHYSICS_BATCH_MAGIC;
    words[1] = HOST_PHYSICS_VERSION;
    words[2] = batch_length * 4;
    words[3] = batch_commands;
    ((float *)words)[4] = TICK;
    result = physics_bridge_step(bridge, &memory, BATCH, batch_length * 4, RESULTS, RESULTS_SIZE, refused, NULL,
                                 reason, sizeof(reason));
    CHECK(result == 0, "the batch: %d %s", result, result < 0 ? reason : "");
    batch_length = HOST_PHYSICS_BATCH_HEADER_WORDS;
    batch_commands = 0;
}

static physics_bridge *new_bridge(void)
{
    batch_length = HOST_PHYSICS_BATCH_HEADER_WORDS;
    batch_commands = 0;
    return physics_bridge_create();
}

/* a 400 unit square at z = 0 */
static void add_ground(void)
{
    static const float vertices[] = {-200, -200, 0, 200, -200, 0, 200, 200, 0, -200, 200, 0};
    static const uint32_t indices[] = {0, 1, 2, 0, 2, 3};
    uint32_t *fields = command(HOST_PHYSICS_OP_LEVEL, HOST_PHYSICS_LEVEL_WORDS);

    memcpy(at(ARRAYS), vertices, sizeof(vertices));
    memcpy(at(ARRAYS + 0x100), indices, sizeof(indices));
    fields[0] = ARRAYS;
    fields[1] = 4;
    fields[2] = ARRAYS + 0x100;
    fields[3] = 2;
}

/* ---------- the bodies */

static const struct profile *profile_named(const char *name)
{
    int index;

    for (index = 0; index < COUNT(profiles); index++)
        if (!strcmp(profiles[index].name, name))
            return &profiles[index];
    printf("no body for %s\n", name);
    failures++;
    return NULL;
}

static const float *part_words(const struct profile *profile, uint32_t part)
{
    return (const float *)&profile->parts[part * HOST_PHYSICS_PART_WORDS];
}

static void transform(const float *pose, const float point[3], float result[3])
{
    /* p + w t + v x t, t = 2 (v x p) */
    const float *v = pose + 3;
    float w = pose[6];
    float t[3] = {2.0f * (v[1] * point[2] - v[2] * point[1]), 2.0f * (v[2] * point[0] - v[0] * point[2]),
                  2.0f * (v[0] * point[1] - v[1] * point[0])};

    result[0] = pose[0] + point[0] + w * t[0] + (v[1] * t[2] - v[2] * t[1]);
    result[1] = pose[1] + point[1] + w * t[1] + (v[2] * t[0] - v[0] * t[2]);
    result[2] = pose[2] + point[2] + w * t[2] + (v[0] * t[1] - v[1] * t[0]);
}

/* a part's shape's centre in the world */
static void part_center(const struct profile *profile, const float *poses, uint32_t part, float center[3])
{
    transform(poses + part * HOST_PHYSICS_POSE_WORDS, part_words(profile, part) + 5, center);
}

/* how far below its centre a part's shape reaches, at most */
static float part_reach(const struct profile *profile, uint32_t part)
{
    const float *words = part_words(profile, part);
    uint32_t shape = profile->parts[part * HOST_PHYSICS_PART_WORDS + 1];

    if (shape == HOST_PHYSICS_SHAPE_CAPSULE)
        return words[2] + words[3];
    if (shape == HOST_PHYSICS_SHAPE_BOX)
        return sqrtf(words[2] * words[2] + words[3] * words[3] + words[4] * words[4]);
    return words[2];
}

/* The parts' masses as Jolt holds them: RagdollSettings::Stabilize keeps
the body's mass but limits each part's to 0.8 to 1.2 times its parent's,
along the chain from the root (a parent always comes before its parts) */
static void stabilized_masses(const struct profile *profile, float *masses)
{
    float ratios[HOST_PHYSICS_PARTS_MAX], total = 0.0f, ratio_total = 0.0f;
    uint32_t part;

    for (part = 0; part < profile->part_count; part++) {
        total += part_words(profile, part)[12];
        if (part == 0) {
            ratios[part] = 1.0f;
        } else {
            int32_t parent = (int32_t)profile->parts[part * HOST_PHYSICS_PART_WORDS];

            ratios[part] = ratios[parent] *
                           fminf(fmaxf(part_words(profile, part)[12] / part_words(profile, parent)[12], 0.8f), 1.2f);
        }
        ratio_total += ratios[part];
    }
    for (part = 0; part < profile->part_count; part++)
        masses[part] = profile->part_count > 1 ? ratios[part] * total / ratio_total : total;
}

static float total_mass(const struct profile *profile)
{
    float total = 0.0f;
    uint32_t part;

    for (part = 0; part < profile->part_count; part++)
        total += part_words(profile, part)[12];
    return total;
}

static void mass_center(const struct profile *profile, const float *poses, float center[3])
{
    float masses[HOST_PHYSICS_PARTS_MAX], total = total_mass(profile);
    uint32_t part;
    int axis;

    stabilized_masses(profile, masses);
    center[0] = center[1] = center[2] = 0.0f;
    for (part = 0; part < profile->part_count; part++) {
        float point[3];

        part_center(profile, poses, part, point);
        for (axis = 0; axis < 3; axis++)
            center[axis] += point[axis] * masses[part] / total;
    }
}

/* The body in its default pose, moved by offset, as RAGDOLL_ADD gives it:
its parts' frames at rest and now. With lowest, moved up first so that its
lowest shape is that far above z = 0. The poses it was given. */
static const float *add_ragdoll(uint32_t id, const struct profile *profile, const float offset[3], int lowest,
                                float above)
{
    uint32_t parts = RAGDOLL_ARRAYS(id), poses = parts + HOST_PHYSICS_PARTS_MAX * HOST_PHYSICS_PART_WORDS * 4;
    float *pose = at(poses);
    float lift = 0.0f;
    uint32_t *fields;
    uint32_t part;

    memcpy(at(parts), profile->parts, profile->part_count * HOST_PHYSICS_PART_WORDS * 4);
    memcpy(pose, profile->poses, profile->part_count * HOST_PHYSICS_POSE_WORDS * 4);
    if (lowest) {
        float bottom = INFINITY;

        for (part = 0; part < profile->part_count; part++) {
            float center[3];

            part_center(profile, pose, part, center);
            bottom = fminf(bottom, center[2] - part_reach(profile, part));
        }
        lift = above - bottom;
    }
    for (part = 0; part < profile->part_count; part++) {
        pose[part * HOST_PHYSICS_POSE_WORDS + 0] += offset[0];
        pose[part * HOST_PHYSICS_POSE_WORDS + 1] += offset[1];
        pose[part * HOST_PHYSICS_POSE_WORDS + 2] += offset[2] + lift;
    }
    fields = command(HOST_PHYSICS_OP_RAGDOLL_ADD, HOST_PHYSICS_RAGDOLL_ADD_WORDS);
    fields[0] = id;
    fields[1] = profile->part_count;
    fields[2] = parts;
    fields[3] = poses;
    fields[4] = poses;
    ((float *)fields)[5] = RAGDOLL_FRICTION;
    ((float *)fields)[6] = RAGDOLL_RESTITUTION;
    return pose;
}

/* the ragdoll's poses and whether it is awake in the last results; 0 if
they have none of it */
static int ragdoll_result(uint32_t id, float *poses, int *awake)
{
    const uint32_t *words = at(RESULTS);
    uint32_t count = words[3], index, offset = HOST_PHYSICS_RESULTS_HEADER_WORDS;

    for (index = 0; index < count; index++) {
        uint32_t part_count = words[offset + 1];

        if (words[offset] == id) {
            memcpy(poses, words + offset + HOST_PHYSICS_RESULT_RAGDOLL_WORDS,
                   part_count * HOST_PHYSICS_POSE_WORDS * 4);
            if (awake)
                *awake = (int)words[offset + 2];
            return 1;
        }
        offset += HOST_PHYSICS_RESULT_RAGDOLL_WORDS + part_count * HOST_PHYSICS_POSE_WORDS;
    }
    return 0;
}

static int finite_poses(const struct profile *profile, const float *poses)
{
    uint32_t index;

    for (index = 0; index < profile->part_count * HOST_PHYSICS_POSE_WORDS; index++)
        if (!isfinite(poses[index]))
            return 0;
    return 1;
}

/* how far the furthest joint has come apart: each part's origin from where
its parent's frame holds it at rest */
static float joint_gap(const struct profile *profile, const float *rest, const float *poses)
{
    float worst = 0.0f;
    uint32_t part;

    for (part = 1; part < profile->part_count; part++) {
        int32_t parent = (int32_t)profile->parts[part * HOST_PHYSICS_PART_WORDS];
        const float *parent_rest = rest + parent * HOST_PHYSICS_POSE_WORDS;
        const float *mine = rest + part * HOST_PHYSICS_POSE_WORDS;
        /* the part's origin in its parent's frame at rest: the inverse
        rotation of the offset */
        float inverse[7] = {0, 0, 0, -parent_rest[3], -parent_rest[4], -parent_rest[5], parent_rest[6]};
        float offset[3] = {mine[0] - parent_rest[0], mine[1] - parent_rest[1], mine[2] - parent_rest[2]};
        float local[3], held[3];
        const float *now = poses + part * HOST_PHYSICS_POSE_WORDS;

        transform(inverse, offset, local);
        transform(poses + parent * HOST_PHYSICS_POSE_WORDS, local, held);
        worst = fmaxf(worst, sqrtf((held[0] - now[0]) * (held[0] - now[0]) + (held[1] - now[1]) * (held[1] - now[1]) +
                                   (held[2] - now[2]) * (held[2] - now[2])));
    }
    return worst;
}

/* ---------- the cases */

/* Every body, dropped a little above the ground in its default pose, side
by side, comes to rest on it in 8 seconds: whole, not through the ground,
and near where it fell. */
static void every_body_comes_to_rest(void)
{
    static float rest[COUNT(profiles)][HOST_PHYSICS_PARTS_MAX * HOST_PHYSICS_POSE_WORDS];
    static float poses[HOST_PHYSICS_PARTS_MAX * HOST_PHYSICS_POSE_WORDS];
    static float second_ago[COUNT(profiles)][HOST_PHYSICS_PARTS_MAX * HOST_PHYSICS_POSE_WORDS];
    physics_bridge *bridge = new_bridge();
    int index, ticks;

    add_ground();
    for (index = 0; index < COUNT(profiles); index++) {
        float offset[3] = {(float)(index % 8) * 6.0f, (float)(index / 8) * 6.0f, 0.0f};

        memcpy(rest[index], add_ragdoll((uint32_t)index + 1, &profiles[index], offset, 1, 0.05f),
               sizeof(rest[index]));
    }
    for (ticks = 0; ticks < 8 * 30; ticks++) {
        tick(bridge);
        if (ticks == 7 * 30 - 1)
            for (index = 0; index < COUNT(profiles); index++)
                ragdoll_result((uint32_t)index + 1, second_ago[index], NULL);
    }
    for (index = 0; index < COUNT(profiles); index++) {
        const struct profile *profile = &profiles[index];
        float before[3], after[3], gap, bottom = INFINITY, still = 0.0f;
        int awake = 1;
        uint32_t part;

        CHECK(ragdoll_result((uint32_t)index + 1, poses, &awake), "%s: in the results", profile->name);
        if (!finite_poses(profile, poses)) {
            CHECK(0, "%s: finite", profile->name);
            continue;
        }
        mass_center(profile, rest[index], before);
        mass_center(profile, poses, after);
        gap = joint_gap(profile, rest[index], poses);
        for (part = 0; part < profile->part_count; part++) {
            float center[3];

            float earlier[3];

            part_center(profile, poses, part, center);
            part_center(profile, second_ago[index], part, earlier);
            bottom = fminf(bottom, center[2]);
            still = fmaxf(still, sqrtf((center[0] - earlier[0]) * (center[0] - earlier[0]) +
                                       (center[1] - earlier[1]) * (center[1] - earlier[1]) +
                                       (center[2] - earlier[2]) * (center[2] - earlier[2])));
        }
        printf("%-48s %2u parts %6.1f kg: asleep %s, the last second %.4f, moved %.3f, lowest centre %.3f, "
               "joints apart %.4f\n",
               profile->name, profile->part_count, profile->mass, awake ? "no " : "yes", still,
               hypotf(after[0] - before[0], after[1] - before[1]), bottom, gap);
        CHECK(!awake || still < 0.01f, "%s: at rest: %f in the last second", profile->name, still);
        CHECK(bottom > -0.01f, "%s: a part through the ground: %f", profile->name, bottom);
        CHECK(after[2] < before[2], "%s: fallen", profile->name);
        CHECK(hypotf(after[0] - before[0], after[1] - before[1]) < 1.5f, "%s: moved %f", profile->name,
              hypotf(after[0] - before[0], after[1] - before[1]));
        CHECK(gap < 0.03f, "%s: a joint apart %f", profile->name, gap);
    }
    physics_bridge_destroy(bridge);
}

/* The push ragdolls.c sends for a frag grenade beside a biped (damage.c's:
its direction lifted, its instantaneous acceleration times the biped's
scale, the body's mass; half of it when it kills, all of it on a corpse)
moves the body's centre of mass at that speed, as the game moves a biped:
measured in the air, against the same body unpushed. */
static void a_frag_grenade_pushes_as_the_game_does(void)
{
    static const float shares[2] = {0.5f, 1.0f};
    int index, share, ticks;

    for (index = 0; index < COUNT(pushed); index++) {
        const struct profile *profile = profile_named(pushed[index]);

        if (!profile)
            continue;
        for (share = 0; share < 2; share++) {
            static float poses[2][HOST_PHYSICS_PARTS_MAX * HOST_PHYSICS_POSE_WORDS];
            float lifted = sqrtf(1.0f + PUSH_LIFT * PUSH_LIFT);
            float speed = shares[share] * FRAG_ACCELERATION * profile->acceleration_scale;
            float expected[3] = {speed / lifted, 0.0f, speed * PUSH_LIFT / lifted};
            float centers[2][3], velocity[3], error;
            int pushed_too;

            for (pushed_too = 0; pushed_too < 2; pushed_too++) {
                physics_bridge *bridge = new_bridge();
                float offset[3] = {0.0f, 0.0f, 20.0f};
                const float *start = add_ragdoll(1, profile, offset, 0, 0.0f);

                if (pushed_too) {
                    uint32_t *fields = command(HOST_PHYSICS_OP_RAGDOLL_IMPULSE, HOST_PHYSICS_RAGDOLL_IMPULSE_WORDS);
                    float *f = (float *)fields;
                    float point[3];

                    /* at the pelvis, its first part */
                    part_center(profile, start, 0, point);
                    fields[0] = 1;
                    fields[1] = 0;
                    f[2] = expected[0] * total_mass(profile);
                    f[3] = 0.0f;
                    f[4] = expected[2] * total_mass(profile);
                    f[5] = point[0];
                    f[6] = point[1];
                    f[7] = point[2];
                }
                for (ticks = 0; ticks < 3; ticks++)
                    tick(bridge);
                CHECK(ragdoll_result(1, poses[pushed_too], NULL), "%s: in the results", profile->name);
                mass_center(profile, poses[pushed_too], centers[pushed_too]);
                physics_bridge_destroy(bridge);
            }
            velocity[0] = (centers[1][0] - centers[0][0]) / (3 * TICK);
            velocity[1] = (centers[1][1] - centers[0][1]) / (3 * TICK);
            velocity[2] = (centers[1][2] - centers[0][2]) / (3 * TICK);
            error = sqrtf((velocity[0] - expected[0]) * (velocity[0] - expected[0]) + velocity[1] * velocity[1] +
                          (velocity[2] - expected[2]) * (velocity[2] - expected[2]));
            printf("%-32s %s: %.3f %.3f units a second, %.3f %.3f expected\n", profile->name,
                   share ? "a corpse " : "a killing", velocity[0], velocity[2], expected[0], expected[2]);
            CHECK(error < 0.02f * speed, "%s: off by %f", profile->name, error);
        }
    }
}

/* The warthog's hull, a kinematic stand-in driven along the ground at 10
metres a second, meets a marine lying where he fell and shoves him along
ahead of it or over it, not through. */
static void a_warthog_shoves_a_marine(void)
{
    static float poses[HOST_PHYSICS_PARTS_MAX * HOST_PHYSICS_POSE_WORDS];
    const struct profile *marine = profile_named("characters/marine/marine");
    physics_bridge *bridge;
    float offset[3] = {0.0f, 0.0f, 0.0f}, before[3], after[3], hull[3];
    float speed = 10.0f / HOST_PHYSICS_METRES_PER_UNIT;
    uint32_t *fields;
    int ticks, axis, inside = 0;

    if (!marine)
        return;
    bridge = new_bridge();
    add_ground();
    add_ragdoll(1, marine, offset, 1, 0.05f);
    for (ticks = 0; ticks < 4 * 30; ticks++)
        tick(bridge);
    CHECK(ragdoll_result(1, poses, NULL), "the marine");
    mass_center(marine, poses, before);
    /* the hull's box: its bottom on the ground, a unit behind the marine */
    hull[0] = before[0] - 1.0f - warthog_half_extents[0];
    hull[1] = before[1];
    hull[2] = warthog_half_extents[2] + 0.01f;
    fields = command(HOST_PHYSICS_OP_BODY_ADD, HOST_PHYSICS_BODY_ADD_WORDS);
    fields[0] = 1;
    fields[1] = HOST_PHYSICS_MOTION_KINEMATIC;
    fields[2] = HOST_PHYSICS_SHAPE_BOX;
    for (axis = 0; axis < 3; axis++) {
        ((float *)fields)[3 + axis] = warthog_half_extents[axis];
        ((float *)fields)[9 + axis] = hull[axis];
    }
    ((float *)fields)[6] = 1.0f;
    ((float *)fields)[7] = STAND_IN_FRICTION;
    ((float *)fields)[8] = STAND_IN_RESTITUTION;
    ((float *)fields)[15] = 1.0f;
    ((float *)fields)[16] = speed;
    tick(bridge);
    for (ticks = 0; ticks < 45; ticks++) {
        hull[0] += speed * TICK;
        fields = command(HOST_PHYSICS_OP_BODY_MOVE, HOST_PHYSICS_BODY_MOVE_WORDS);
        fields[0] = 1;
        for (axis = 0; axis < 3; axis++)
            ((float *)fields)[1 + axis] = hull[axis];
        ((float *)fields)[7] = 1.0f;
        tick(bridge);
        CHECK(ragdoll_result(1, poses, NULL), "the marine");
        mass_center(marine, poses, after);
        inside += fabsf(after[0] - hull[0]) < warthog_half_extents[0] - 0.05f &&
                  fabsf(after[1] - hull[1]) < warthog_half_extents[1] - 0.05f &&
                  fabsf(after[2] - hull[2]) < warthog_half_extents[2] - 0.05f;
    }
    printf("the marine: shoved %.3f units, %.3f up; the hull at %.3f, he at %.3f; ticks inside it %d\n",
           after[0] - before[0], after[2] - before[2], hull[0], after[0], inside);
    CHECK(finite_poses(marine, poses), "finite");
    CHECK(after[0] - before[0] > 1.0f, "shoved %f", after[0] - before[0]);
    CHECK(inside == 0, "inside the hull %d ticks", inside);
    physics_bridge_destroy(bridge);
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        void (*run)(void);
    } cases[] = {
        {"every_body_comes_to_rest", every_body_comes_to_rest},
        {"a_frag_grenade_pushes_as_the_game_does", a_frag_grenade_pushes_as_the_game_does},
        {"a_warthog_shoves_a_marine", a_warthog_shoves_a_marine},
    };
    int index;

    for (index = 0; index < COUNT(cases); index++) {
        if (argc > 1 && !strcmp(argv[1], cases[index].name)) {
            cases[index].run();
            if (!failures)
                printf("ok\n");
            return failures ? 1 : 0;
        }
    }
    printf("unknown case\n");
    return 2;
}
