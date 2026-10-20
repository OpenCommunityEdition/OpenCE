/*
 * host_physics_bridge_test.c - the physics bridge (port/macos/host/physics/
 * physics_bridge.c), natively, with guest memory of its own:
 * tools/test_macos_physics.py builds it with the physics library and runs
 * one case, given as its argument. It prints "ok", or what failed.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "host_physics.h"
#include "physics/physics_bridge.h"

static int failures;

#define CHECK(condition)                                                                 \
    do {                                                                                 \
        if (!(condition)) {                                                              \
            printf("%s:%d: %s\n", __FILE__, __LINE__, #condition);                       \
            failures++;                                                                  \
        }                                                                                \
    } while (0)

/* the guest's memory: 1 MB at GUEST_BASE */
#define GUEST_BASE 0x00100000u
#define GUEST_SIZE (1u << 20)
static unsigned char guest[GUEST_SIZE];

/* where the tests keep things in it */
#define BATCH (GUEST_BASE + 0x00000)
#define RESULTS (GUEST_BASE + 0x40000)
#define ARRAYS (GUEST_BASE + 0x80000)

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

/* ---------- writing batches */

struct writer {
    uint32_t *words;
    uint32_t length;
    uint32_t commands;
};

static uint32_t refusals;
static char last_refusal[256];

static void refused(void *context, uint32_t offset, uint32_t op, const char *reason)
{
    (void)context;
    (void)offset;
    (void)op;
    refusals++;
    snprintf(last_refusal, sizeof(last_refusal), "%s", reason);
}

static void begin(struct writer *writer)
{
    writer->words = at(BATCH);
    writer->length = HOST_PHYSICS_BATCH_HEADER_WORDS;
    writer->commands = 0;
}

static uint32_t *command(struct writer *writer, uint32_t op, uint32_t words)
{
    uint32_t *fields = writer->words + writer->length;

    fields[0] = HOST_PHYSICS_COMMAND(op, words);
    writer->length += words;
    writer->commands++;
    return fields + 1;
}

static float *floats(uint32_t *words) { return (float *)words; }

static uint32_t finish(struct writer *writer, float seconds)
{
    writer->words[0] = HOST_PHYSICS_BATCH_MAGIC;
    writer->words[1] = HOST_PHYSICS_VERSION;
    writer->words[2] = writer->length * 4;
    writer->words[3] = writer->commands;
    floats(writer->words)[4] = seconds;
    return writer->length * 4;
}

static int step(physics_bridge *bridge, struct writer *writer, float seconds)
{
    char reason[128];
    uint32_t size = finish(writer, seconds);

    refusals = 0;
    return physics_bridge_step(bridge, &memory, BATCH, size, RESULTS, 0x40000, refused, NULL, reason,
                               sizeof(reason));
}

static void write_pose(float *to, float x, float y, float z)
{
    to[0] = x;
    to[1] = y;
    to[2] = z;
    to[3] = 0.0f;
    to[4] = 0.0f;
    to[5] = 0.0f;
    to[6] = 1.0f;
}

/* a 20 unit square at z = 0, as two triangles */
static void add_level(struct writer *writer)
{
    static const float vertices[] = {-10, -10, 0, 10, -10, 0, 10, 10, 0, -10, 10, 0};
    static const uint32_t indices[] = {0, 1, 2, 0, 2, 3};
    uint32_t *fields = command(writer, HOST_PHYSICS_OP_LEVEL, HOST_PHYSICS_LEVEL_WORDS);

    memcpy(at(ARRAYS), vertices, sizeof(vertices));
    memcpy(at(ARRAYS + 0x100), indices, sizeof(indices));
    fields[0] = ARRAYS;
    fields[1] = 4;
    fields[2] = ARRAYS + 0x100;
    fields[3] = 2;
}

/* a box of 0.1 units' half extents at z */
static uint32_t *add_body(struct writer *writer, uint32_t id, uint32_t motion, float z)
{
    uint32_t *fields = command(writer, HOST_PHYSICS_OP_BODY_ADD, HOST_PHYSICS_BODY_ADD_WORDS);
    float *f = floats(fields);

    memset(fields, 0, (HOST_PHYSICS_BODY_ADD_WORDS - 1) * 4);
    fields[0] = id;
    fields[1] = motion;
    fields[2] = HOST_PHYSICS_SHAPE_BOX;
    f[3] = f[4] = f[5] = 0.1f;
    f[6] = 3.0f;
    f[7] = 0.8f;
    write_pose(f + 9, 0.0f, 0.0f, z);
    return fields;
}

/* a ragdoll of two capsules along x, the second joined at the first's end */
#define RAGDOLL_PARTS (ARRAYS + 0x1000)
#define RAGDOLL_REST (ARRAYS + 0x2000)

static void write_ragdoll_arrays(float z)
{
    float *parts = at(RAGDOLL_PARTS), *rest = at(RAGDOLL_REST);
    int i;

    memset(parts, 0, 2 * HOST_PHYSICS_PART_WORDS * 4);
    for (i = 0; i < 2; i++) {
        float *part = parts + i * HOST_PHYSICS_PART_WORDS;
        ((int32_t *)part)[0] = i - 1;
        ((uint32_t *)part)[1] = HOST_PHYSICS_SHAPE_CAPSULE;
        part[2] = 0.05f;
        part[3] = 0.1f;
        write_pose(part + 5, 0.15f, 0.0f, 0.0f);
        part[12] = 10.0f;
        part[13] = 1.0f;
        part[17] = 1.0f;
        part[19] = -0.3f;
        part[20] = 0.3f;
        part[21] = 0.5f;
        part[22] = 0.5f;
        write_pose(rest + i * HOST_PHYSICS_POSE_WORDS, 0.3f * (float)i, 0.0f, z);
    }
}

static uint32_t *add_ragdoll(struct writer *writer, uint32_t id, float z)
{
    uint32_t *fields = command(writer, HOST_PHYSICS_OP_RAGDOLL_ADD, HOST_PHYSICS_RAGDOLL_ADD_WORDS);
    float *f = floats(fields);

    write_ragdoll_arrays(z);
    fields[0] = id;
    fields[1] = 2;
    fields[2] = RAGDOLL_PARTS;
    fields[3] = RAGDOLL_REST;
    fields[4] = RAGDOLL_REST;
    f[5] = 0.8f;
    f[6] = 0.0f;
    f[7] = f[8] = f[9] = 0.0f;
    return fields;
}

/* ---------- reading results */

static const uint32_t *results(void) { return at(RESULTS); }

/* the pose of body id in the results, or NULL */
static const float *body_pose(uint32_t id)
{
    const uint32_t *words = results();
    uint32_t at_word = HOST_PHYSICS_RESULTS_HEADER_WORDS, i;

    for (i = 0; i < words[3]; i++)
        at_word += HOST_PHYSICS_RESULT_RAGDOLL_WORDS + words[at_word + 1] * HOST_PHYSICS_POSE_WORDS;
    for (i = 0; i < words[4]; i++, at_word += HOST_PHYSICS_RESULT_BODY_WORDS)
        if (words[at_word] == id)
            return (const float *)(words + at_word + 2);
    return NULL;
}

/* the first ragdoll's entry, or NULL */
static const uint32_t *first_ragdoll(void)
{
    const uint32_t *words = results();
    return words[3] ? words + HOST_PHYSICS_RESULTS_HEADER_WORDS : NULL;
}

/* ---------- the cases */

static void a_valid_batch_is_carried_out(void)
{
    physics_bridge *bridge = physics_bridge_create();
    struct writer writer;
    const float *pose;
    const uint32_t *ragdoll;
    int i;

    begin(&writer);
    add_level(&writer);
    add_body(&writer, 7, HOST_PHYSICS_MOTION_DYNAMIC, 1.0f);
    add_ragdoll(&writer, 3, 0.5f);
    CHECK(step(bridge, &writer, 1.0f / 30.0f) == 0);
    CHECK(refusals == 0);
    CHECK(physics_bridge_has_level(bridge));
    CHECK(physics_bridge_bodies(bridge) == 1);
    CHECK(physics_bridge_ragdolls(bridge) == 1);
    CHECK(results()[0] == HOST_PHYSICS_RESULTS_MAGIC);
    CHECK(results()[1] == HOST_PHYSICS_VERSION);
    CHECK(results()[3] == 1 && results()[4] == 1);
    CHECK(results()[5] == 0);
    CHECK(results()[2] == 4 * (HOST_PHYSICS_RESULTS_HEADER_WORDS + HOST_PHYSICS_RESULT_RAGDOLL_WORDS +
                               2 * HOST_PHYSICS_POSE_WORDS + HOST_PHYSICS_RESULT_BODY_WORDS));
    ragdoll = first_ragdoll();
    CHECK(ragdoll && ragdoll[0] == 3 && ragdoll[1] == 2 && ragdoll[2] == 1 && ragdoll[3] == 0);
    pose = body_pose(7);
    CHECK(pose && pose[2] < 1.0f && pose[2] > 0.9f);
    /* in world units: the box falls to rest on the level, its half extent up */
    for (i = 0; i < 90; i++) {
        begin(&writer);
        CHECK(step(bridge, &writer, 1.0f / 30.0f) == 0);
    }
    pose = body_pose(7);
    CHECK(pose && fabsf(pose[2] - 0.1f) < 0.005f);
    /* asleep, as its awake word (before its pose) says */
    CHECK(pose && ((const uint32_t *)pose)[-1] == 0);

    /* removed, and cleared */
    begin(&writer);
    command(&writer, HOST_PHYSICS_OP_BODY_REMOVE, HOST_PHYSICS_BODY_REMOVE_WORDS)[0] = 7;
    CHECK(step(bridge, &writer, 0.0f) == 0);
    CHECK(physics_bridge_bodies(bridge) == 0 && results()[4] == 0);
    begin(&writer);
    command(&writer, HOST_PHYSICS_OP_CLEAR, HOST_PHYSICS_CLEAR_WORDS);
    CHECK(step(bridge, &writer, 0.0f) == 0);
    CHECK(physics_bridge_ragdolls(bridge) == 0 && !physics_bridge_has_level(bridge));
    physics_bridge_destroy(bridge);
}

static void bad_batches_are_refused_whole(void)
{
    physics_bridge *bridge = physics_bridge_create();
    struct writer writer;
    char reason[128];
    uint32_t size;

    begin(&writer);
    size = finish(&writer, 0.0f);
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size, RESULTS, 0x1000, refused, NULL, reason,
                              sizeof(reason)) == 0);
    /* not guest memory */
    CHECK(physics_bridge_step(bridge, &memory, 0x1000, size, RESULTS, 0x1000, refused, NULL, reason,
                              sizeof(reason)) == -1);
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size, GUEST_BASE + GUEST_SIZE - 8, 0x1000, refused, NULL,
                              reason, sizeof(reason)) == -1);
    /* sizes */
    CHECK(physics_bridge_step(bridge, &memory, BATCH, 8, RESULTS, 0x1000, refused, NULL, reason,
                              sizeof(reason)) == -1);
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size + 4, RESULTS, 0x1000, refused, NULL, reason,
                              sizeof(reason)) == -1);
    CHECK(physics_bridge_step(bridge, &memory, BATCH, HOST_PHYSICS_MAX_BATCH_LENGTH + 4, RESULTS, 0x1000, refused,
                              NULL, reason, sizeof(reason)) == -1);
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size, RESULTS, 8, refused, NULL, reason,
                              sizeof(reason)) == -1);
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size, RESULTS + 2, 0x1000, refused, NULL, reason,
                              sizeof(reason)) == -1);
    /* the header */
    writer.words[0] = 0x12345678;
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size, RESULTS, 0x1000, refused, NULL, reason,
                              sizeof(reason)) == -1);
    begin(&writer);
    finish(&writer, 0.0f);
    writer.words[1] = HOST_PHYSICS_VERSION + 1;
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size, RESULTS, 0x1000, refused, NULL, reason,
                              sizeof(reason)) == -1);
    begin(&writer);
    finish(&writer, 0.0f);
    floats(writer.words)[4] = nanf("");
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size, RESULTS, 0x1000, refused, NULL, reason,
                              sizeof(reason)) == -1);
    floats(writer.words)[4] = 5.0f;
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size, RESULTS, 0x1000, refused, NULL, reason,
                              sizeof(reason)) == -1);
    floats(writer.words)[4] = -0.1f;
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size, RESULTS, 0x1000, refused, NULL, reason,
                              sizeof(reason)) == -1);
    CHECK(reason[0] != '\0');
    physics_bridge_destroy(bridge);
}

static void bad_commands_are_refused(void)
{
    physics_bridge *bridge = physics_bridge_create();
    struct writer writer;
    uint32_t *fields;
    float *f;

    /* ids: 0, past the limit, in use, unknown */
    begin(&writer);
    add_body(&writer, 0, HOST_PHYSICS_MOTION_DYNAMIC, 1.0f);
    add_body(&writer, HOST_PHYSICS_BODIES_MAX + 1, HOST_PHYSICS_MOTION_DYNAMIC, 1.0f);
    add_body(&writer, 5, HOST_PHYSICS_MOTION_DYNAMIC, 1.0f);
    add_body(&writer, 5, HOST_PHYSICS_MOTION_DYNAMIC, 1.0f);
    command(&writer, HOST_PHYSICS_OP_BODY_REMOVE, HOST_PHYSICS_BODY_REMOVE_WORDS)[0] = 6;
    add_ragdoll(&writer, 0, 1.0f);
    add_ragdoll(&writer, HOST_PHYSICS_RAGDOLLS_MAX + 1, 1.0f);
    command(&writer, HOST_PHYSICS_OP_RAGDOLL_LIMP, HOST_PHYSICS_RAGDOLL_LIMP_WORDS)[0] = 9;
    CHECK(step(bridge, &writer, 0.0f) == 7);
    CHECK(refusals == 7);
    CHECK(physics_bridge_bodies(bridge) == 1 && physics_bridge_ragdolls(bridge) == 0);

    /* numbers that are not finite, or out of range */
    begin(&writer);
    f = floats(add_body(&writer, 8, HOST_PHYSICS_MOTION_DYNAMIC, 1.0f));
    f[9] = nanf("");
    f = floats(add_body(&writer, 9, HOST_PHYSICS_MOTION_DYNAMIC, 1.0f));
    f[3] = INFINITY;
    f = floats(add_body(&writer, 10, HOST_PHYSICS_MOTION_DYNAMIC, 1.0f));
    f[6] = -1.0f;
    fields = add_body(&writer, 11, 7, 1.0f);
    fields = add_body(&writer, 12, HOST_PHYSICS_MOTION_DYNAMIC, 1.0f);
    fields[2] = 9;
    fields = command(&writer, HOST_PHYSICS_OP_BODY_IMPULSE, HOST_PHYSICS_BODY_IMPULSE_WORDS);
    memset(fields, 0, 7 * 4);
    fields[0] = 5;
    floats(fields)[1] = nanf("");
    CHECK(step(bridge, &writer, 0.0f) == 6);
    CHECK(physics_bridge_bodies(bridge) == 1);

    /* sizes: a command of the wrong size, an unknown op, and one that runs
       past the batch (which ends it) */
    begin(&writer);
    command(&writer, HOST_PHYSICS_OP_BODY_REMOVE, 3)[0] = 5;
    command(&writer, 99, 2);
    command(&writer, HOST_PHYSICS_OP_CLEAR, 0);
    writer.length++;
    add_body(&writer, 13, HOST_PHYSICS_MOTION_DYNAMIC, 1.0f);
    CHECK(step(bridge, &writer, 0.0f) == 3);
    CHECK(physics_bridge_bodies(bridge) == 1);
    begin(&writer);
    command(&writer, HOST_PHYSICS_OP_BODY_REMOVE, 200)[0] = 5;
    CHECK(step(bridge, &writer, 0.0f) == 1);
    CHECK(physics_bridge_bodies(bridge) == 1);
    begin(&writer);
    command(&writer, HOST_PHYSICS_OP_BODY_REMOVE, HOST_PHYSICS_BODY_REMOVE_WORDS)[0] = 5;
    writer.commands = 4;
    CHECK(step(bridge, &writer, 0.0f) == 1);
    CHECK(physics_bridge_bodies(bridge) == 0);

    /* the arrays a command names: outside guest memory, too many, and
       indices out of range */
    begin(&writer);
    add_ragdoll(&writer, 1, 1.0f)[2] = GUEST_BASE + GUEST_SIZE - 16;
    add_ragdoll(&writer, 2, 1.0f)[1] = HOST_PHYSICS_PARTS_MAX + 1;
    add_ragdoll(&writer, 3, 1.0f)[1] = 0;
    add_level(&writer);
    ((uint32_t *)at(ARRAYS + 0x100))[4] = 4;
    CHECK(step(bridge, &writer, 0.0f) == 4);
    begin(&writer);
    fields = command(&writer, HOST_PHYSICS_OP_LEVEL, HOST_PHYSICS_LEVEL_WORDS);
    fields[0] = 0x2000;
    fields[1] = 4;
    fields[2] = ARRAYS + 0x100;
    fields[3] = 2;
    fields = command(&writer, HOST_PHYSICS_OP_LEVEL, HOST_PHYSICS_LEVEL_WORDS);
    fields[0] = ARRAYS;
    fields[1] = 4;
    fields[2] = ARRAYS + 0x100;
    fields[3] = HOST_PHYSICS_LEVEL_TRIANGLES_MAX + 1;
    CHECK(step(bridge, &writer, 0.0f) == 2);
    CHECK(!physics_bridge_has_level(bridge) && physics_bridge_ragdolls(bridge) == 0);

    /* a ragdoll's own commands, on parts that are not its */
    begin(&writer);
    add_ragdoll(&writer, 4, 1.0f);
    fields = command(&writer, HOST_PHYSICS_OP_RAGDOLL_BREAK, HOST_PHYSICS_RAGDOLL_BREAK_WORDS);
    fields[0] = 4;
    fields[1] = 2;
    fields = command(&writer, HOST_PHYSICS_OP_RAGDOLL_IMPULSE, HOST_PHYSICS_RAGDOLL_IMPULSE_WORDS);
    memset(fields, 0, 8 * 4);
    fields[0] = 4;
    fields[1] = 5;
    fields = command(&writer, HOST_PHYSICS_OP_RAGDOLL_DRIVE, HOST_PHYSICS_RAGDOLL_DRIVE_WORDS);
    fields[0] = 4;
    floats(fields)[1] = 5.0f;
    fields[2] = RAGDOLL_REST;
    fields[3] = 3;
    fields = command(&writer, HOST_PHYSICS_OP_RAGDOLL_BREAK, HOST_PHYSICS_RAGDOLL_BREAK_WORDS);
    fields[0] = 4;
    fields[1] = 1;
    CHECK(step(bridge, &writer, 1.0f / 30.0f) == 3);
    CHECK(physics_bridge_ragdolls(bridge) == 1);
    CHECK(first_ragdoll() && first_ragdoll()[3] == 2);
    physics_bridge_destroy(bridge);
}

static void small_results_are_truncated(void)
{
    physics_bridge *bridge = physics_bridge_create();
    struct writer writer;
    char reason[128];
    uint32_t size, id;

    begin(&writer);
    for (id = 1; id <= 10; id++)
        add_body(&writer, id, HOST_PHYSICS_MOTION_DYNAMIC, (float)id);
    size = finish(&writer, 0.0f);
    CHECK(physics_bridge_step(bridge, &memory, BATCH, size, RESULTS,
                              4 * (HOST_PHYSICS_RESULTS_HEADER_WORDS + 3 * HOST_PHYSICS_RESULT_BODY_WORDS + 2),
                              refused, NULL, reason, sizeof(reason)) == 0);
    CHECK(results()[4] == 3);
    CHECK(results()[5] == HOST_PHYSICS_RESULTS_TRUNCATED);
    CHECK(results()[2] == 4 * (HOST_PHYSICS_RESULTS_HEADER_WORDS + 3 * HOST_PHYSICS_RESULT_BODY_WORDS));
    CHECK(body_pose(1) && body_pose(3) && !body_pose(4));
    physics_bridge_destroy(bridge);
}

/* the same batches give the same results, to the bit */
static void run_twice(uint32_t *out, uint32_t words)
{
    physics_bridge *bridge = physics_bridge_create();
    struct writer writer;
    uint32_t *fields;
    int i;

    begin(&writer);
    add_level(&writer);
    add_body(&writer, 1, HOST_PHYSICS_MOTION_DYNAMIC, 0.5f);
    add_body(&writer, 2, HOST_PHYSICS_MOTION_KINEMATIC, 0.2f);
    add_ragdoll(&writer, 1, 0.8f);
    step(bridge, &writer, 1.0f / 30.0f);
    for (i = 0; i < 60; i++) {
        begin(&writer);
        fields = command(&writer, HOST_PHYSICS_OP_BODY_MOVE, HOST_PHYSICS_BODY_MOVE_WORDS);
        fields[0] = 2;
        write_pose(floats(fields) + 1, 0.01f * (float)i, 0.0f, 0.2f);
        if (i == 10) {
            fields = command(&writer, HOST_PHYSICS_OP_RAGDOLL_IMPULSE, HOST_PHYSICS_RAGDOLL_IMPULSE_WORDS);
            memset(fields, 0, 8 * 4);
            fields[0] = 1;
            fields[1] = 1;
            floats(fields)[2] = 3.0f;
            floats(fields)[7] = 0.3f;
        }
        CHECK(step(bridge, &writer, 1.0f / 30.0f) == 0);
    }
    memcpy(out, results(), words * 4);
    physics_bridge_destroy(bridge);
}

static void runs_repeat(void)
{
    static uint32_t first[256], second[256];
    uint32_t words;

    run_twice(first, 256);
    words = first[2] / 4;
    CHECK(words <= 256);
    run_twice(second, 256);
    CHECK(memcmp(first, second, words * 4) == 0);
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        void (*run)(void);
    } cases[] = {
        {"a_valid_batch_is_carried_out", a_valid_batch_is_carried_out},
        {"bad_batches_are_refused_whole", bad_batches_are_refused_whole},
        {"bad_commands_are_refused", bad_commands_are_refused},
        {"small_results_are_truncated", small_results_are_truncated},
        {"runs_repeat", runs_repeat},
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
