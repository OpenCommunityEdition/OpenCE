/*
 * physics_bridge.c - the guest's physics batches (port/macos/include/
 * host_physics.h), checked and carried out on a physics world.
 *
 * The batch is copied out of guest memory before any of it is read, so that
 * the guest cannot change it between its check and its use; so is every
 * array a command names. The guest names its ragdolls and bodies by ids of
 * its own, which the bridge maps to the world's handles. Lengths become
 * metres here and world units again in the results.
 */

#include "physics/physics_bridge.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host_physics.h"
#include "physics/physics.h"

#define UNIT HOST_PHYSICS_METRES_PER_UNIT
#define BATCH_WORDS_MAX (HOST_PHYSICS_MAX_BATCH_LENGTH / 4)
#define LEVEL_VERTICES_MAX (3u * HOST_PHYSICS_LEVEL_TRIANGLES_MAX)
/* the world's largest numbers, in world units: positions, sizes, impulses */
#define POSITION_MAX 100000.0f
#define SIZE_MAX_UNITS 30.0f
#define IMPULSE_MAX 100000.0f
#define VELOCITY_MAX 300.0f

struct physics_bridge {
    physics_world *world;
    int has_level;
    uint32_t ragdoll_count;
    uint32_t body_count;
    /* by the guest's id; PHYSICS_NONE for none */
    uint32_t ragdolls[HOST_PHYSICS_RAGDOLLS_MAX + 1];
    uint8_t ragdoll_parts[HOST_PHYSICS_RAGDOLLS_MAX + 1];
    uint32_t bodies[HOST_PHYSICS_BODIES_MAX + 1];
    /* the batch, copied */
    uint32_t batch[BATCH_WORDS_MAX];
    /* a command's arrays, copied */
    float arrays[2 * HOST_PHYSICS_PARTS_MAX * HOST_PHYSICS_PART_WORDS];
    physics_part parts[HOST_PHYSICS_PARTS_MAX];
    physics_pose rest[HOST_PHYSICS_PARTS_MAX];
    physics_pose poses[HOST_PHYSICS_PARTS_MAX];
};

/* the words each op's command has */
static const uint8_t command_words[HOST_PHYSICS_OP_COUNT] = {
    [HOST_PHYSICS_OP_CLEAR] = HOST_PHYSICS_CLEAR_WORDS,
    [HOST_PHYSICS_OP_LEVEL] = HOST_PHYSICS_LEVEL_WORDS,
    [HOST_PHYSICS_OP_RAGDOLL_ADD] = HOST_PHYSICS_RAGDOLL_ADD_WORDS,
    [HOST_PHYSICS_OP_RAGDOLL_REMOVE] = HOST_PHYSICS_RAGDOLL_REMOVE_WORDS,
    [HOST_PHYSICS_OP_RAGDOLL_DRIVE] = HOST_PHYSICS_RAGDOLL_DRIVE_WORDS,
    [HOST_PHYSICS_OP_RAGDOLL_LIMP] = HOST_PHYSICS_RAGDOLL_LIMP_WORDS,
    [HOST_PHYSICS_OP_RAGDOLL_IMPULSE] = HOST_PHYSICS_RAGDOLL_IMPULSE_WORDS,
    [HOST_PHYSICS_OP_RAGDOLL_BREAK] = HOST_PHYSICS_RAGDOLL_BREAK_WORDS,
    [HOST_PHYSICS_OP_BODY_ADD] = HOST_PHYSICS_BODY_ADD_WORDS,
    [HOST_PHYSICS_OP_BODY_MOVE] = HOST_PHYSICS_BODY_MOVE_WORDS,
    [HOST_PHYSICS_OP_BODY_REMOVE] = HOST_PHYSICS_BODY_REMOVE_WORDS,
    [HOST_PHYSICS_OP_BODY_IMPULSE] = HOST_PHYSICS_BODY_IMPULSE_WORDS,
};

physics_bridge *physics_bridge_create(void)
{
    physics_world_settings settings;
    physics_bridge *bridge = calloc(1, sizeof(*bridge));
    uint32_t i;

    if (bridge == NULL)
        return NULL;
    memset(&settings, 0, sizeof(settings));
    settings.gravity[2] = -9.81f;
    settings.bodies_max = HOST_PHYSICS_RAGDOLLS_MAX * HOST_PHYSICS_PARTS_MAX + HOST_PHYSICS_BODIES_MAX;
    /* two a tick of the game's 30 */
    settings.collision_steps = 2;
    bridge->world = physics_world_create(&settings);
    if (bridge->world == NULL) {
        free(bridge);
        return NULL;
    }
    for (i = 0; i <= HOST_PHYSICS_RAGDOLLS_MAX; i++)
        bridge->ragdolls[i] = PHYSICS_NONE;
    for (i = 0; i <= HOST_PHYSICS_BODIES_MAX; i++)
        bridge->bodies[i] = PHYSICS_NONE;
    return bridge;
}

void physics_bridge_destroy(physics_bridge *bridge)
{
    if (bridge == NULL)
        return;
    physics_world_destroy(bridge->world);
    free(bridge);
}

uint32_t physics_bridge_ragdolls(const physics_bridge *bridge) { return bridge->ragdoll_count; }
uint32_t physics_bridge_bodies(const physics_bridge *bridge) { return bridge->body_count; }
int physics_bridge_has_level(const physics_bridge *bridge) { return bridge->has_level; }

/* ---------- reading what the guest sent */

static float word_float(uint32_t word)
{
    float value;
    memcpy(&value, &word, sizeof(value));
    return value;
}

static int finite_within(float value, float limit) { return isfinite(value) && fabsf(value) <= limit; }

/* three floats within a limit, scaled */
static int read_vector(const uint32_t *words, float limit, float scale, float out[3])
{
    int i;
    for (i = 0; i < 3; i++) {
        float value = word_float(words[i]);
        if (!finite_within(value, limit))
            return 0;
        out[i] = value * scale;
    }
    return 1;
}

static int read_pose(const uint32_t *words, physics_pose *pose)
{
    int i;
    if (!read_vector(words, POSITION_MAX, UNIT, pose->position))
        return 0;
    for (i = 0; i < 4; i++) {
        pose->rotation[i] = word_float(words[3 + i]);
        if (!finite_within(pose->rotation[i], 2.0f))
            return 0;
    }
    return 1;
}

static int read_shape(const uint32_t *words, physics_shape *shape)
{
    switch (words[0]) {
    case HOST_PHYSICS_SHAPE_SPHERE: shape->type = PHYSICS_SHAPE_SPHERE; break;
    case HOST_PHYSICS_SHAPE_CAPSULE: shape->type = PHYSICS_SHAPE_CAPSULE; break;
    case HOST_PHYSICS_SHAPE_BOX: shape->type = PHYSICS_SHAPE_BOX; break;
    default: return 0;
    }
    return read_vector(words + 1, SIZE_MAX_UNITS, UNIT, shape->size);
}

/* count elements of size bytes at a guest address, copied to to */
static int copy_in(const physics_bridge_memory *memory, uint32_t address, uint32_t count, uint32_t size, void *to)
{
    const void *from;
    uint64_t bytes = (uint64_t)count * size;

    if (count == 0 || bytes > UINT32_MAX || (address & 3) != 0)
        return 0;
    from = memory->pointer(memory->context, address, (uint32_t)bytes);
    if (from == NULL)
        return 0;
    memcpy(to, from, (size_t)bytes);
    return 1;
}

/* count poses at a guest address, into poses */
static int copy_poses(physics_bridge *bridge, const physics_bridge_memory *memory, uint32_t address, uint32_t count,
                      physics_pose *poses)
{
    const uint32_t *words = (const uint32_t *)bridge->arrays;
    uint32_t i;

    if (!copy_in(memory, address, count, HOST_PHYSICS_POSE_WORDS * 4, bridge->arrays))
        return 0;
    for (i = 0; i < count; i++)
        if (!read_pose(words + i * HOST_PHYSICS_POSE_WORDS, &poses[i]))
            return 0;
    return 1;
}

static int copy_parts(physics_bridge *bridge, const physics_bridge_memory *memory, uint32_t address, uint32_t count)
{
    const uint32_t *words = (const uint32_t *)bridge->arrays;
    uint32_t i;

    if (!copy_in(memory, address, count, HOST_PHYSICS_PART_WORDS * 4, bridge->arrays))
        return 0;
    for (i = 0; i < count; i++) {
        const uint32_t *at = words + i * HOST_PHYSICS_PART_WORDS;
        physics_part *part = &bridge->parts[i];
        int j;

        part->parent = (int32_t)at[0];
        if (!read_shape(at + 1, &part->shape) || !read_pose(at + 5, &part->shape.offset)
            || !read_vector(at + 13, 2.0f, 1.0f, part->twist_axis) || !read_vector(at + 16, 2.0f, 1.0f, part->plane_axis))
            return 0;
        part->mass = word_float(at[12]);
        part->twist_min = word_float(at[19]);
        part->twist_max = word_float(at[20]);
        part->swing_normal = word_float(at[21]);
        part->swing_plane = word_float(at[22]);
        for (j = 19; j <= 22; j++)
            if (!finite_within(word_float(at[j]), 4.0f))
                return 0;
        if (!isfinite(part->mass))
            return 0;
    }
    return 1;
}

static int read_material(const uint32_t *words, physics_material *material)
{
    material->friction = word_float(words[0]);
    material->restitution = word_float(words[1]);
    return finite_within(material->friction, 10.0f) && finite_within(material->restitution, 1.0f);
}

/* ---------- the commands */

static void clear(physics_bridge *bridge)
{
    uint32_t i;

    for (i = 1; i <= HOST_PHYSICS_RAGDOLLS_MAX; i++)
        if (bridge->ragdolls[i] != PHYSICS_NONE) {
            physics_ragdoll_remove(bridge->world, bridge->ragdolls[i]);
            bridge->ragdolls[i] = PHYSICS_NONE;
        }
    for (i = 1; i <= HOST_PHYSICS_BODIES_MAX; i++)
        if (bridge->bodies[i] != PHYSICS_NONE) {
            physics_body_remove(bridge->world, bridge->bodies[i]);
            bridge->bodies[i] = PHYSICS_NONE;
        }
    physics_world_clear_level(bridge->world);
    bridge->ragdoll_count = bridge->body_count = 0;
    bridge->has_level = 0;
}

static const char *set_level(physics_bridge *bridge, const physics_bridge_memory *memory, const uint32_t *fields)
{
    uint32_t vertex_count = fields[1], triangle_count = fields[3], i;
    float *vertices;
    uint32_t *indices;
    int set;

    if (vertex_count == 0) {
        physics_world_clear_level(bridge->world);
        bridge->has_level = 0;
        return NULL;
    }
    if (vertex_count > LEVEL_VERTICES_MAX || triangle_count == 0
        || triangle_count > HOST_PHYSICS_LEVEL_TRIANGLES_MAX)
        return "a level of too many vertices or triangles, or none";
    vertices = malloc((size_t)vertex_count * 12);
    indices = malloc((size_t)triangle_count * 12);
    if (vertices == NULL || indices == NULL || !copy_in(memory, fields[0], vertex_count, 12, vertices)
        || !copy_in(memory, fields[2], triangle_count, 12, indices)) {
        free(vertices);
        free(indices);
        return "a level's arrays outside guest memory";
    }
    for (i = 0; i < 3 * vertex_count; i++) {
        if (!finite_within(vertices[i], POSITION_MAX)) {
            free(vertices);
            free(indices);
            return "a level's vertex not finite";
        }
        vertices[i] *= UNIT;
    }
    /* the world checks the indices; a mesh refused leaves no level */
    set = physics_world_set_level(bridge->world, vertices, vertex_count, indices, triangle_count);
    free(vertices);
    free(indices);
    if (set != 0) {
        physics_world_clear_level(bridge->world);
        bridge->has_level = 0;
        return "a level the world refused";
    }
    bridge->has_level = 1;
    return NULL;
}

/* a ragdoll's slot for an id, or NULL; in use or free as wanted */
static uint32_t *ragdoll_slot(physics_bridge *bridge, uint32_t id, int in_use)
{
    if (id == 0 || id > HOST_PHYSICS_RAGDOLLS_MAX)
        return NULL;
    if ((bridge->ragdolls[id] != PHYSICS_NONE) != in_use)
        return NULL;
    return &bridge->ragdolls[id];
}

static uint32_t *body_slot(physics_bridge *bridge, uint32_t id, int in_use)
{
    if (id == 0 || id > HOST_PHYSICS_BODIES_MAX)
        return NULL;
    if ((bridge->bodies[id] != PHYSICS_NONE) != in_use)
        return NULL;
    return &bridge->bodies[id];
}

static const char *add_ragdoll(physics_bridge *bridge, const physics_bridge_memory *memory, const uint32_t *fields)
{
    uint32_t *slot = ragdoll_slot(bridge, fields[0], 0), count = fields[1], handle;
    physics_material material;
    float velocity[3];

    if (slot == NULL)
        return "a ragdoll id out of range or in use";
    if (count == 0 || count > HOST_PHYSICS_PARTS_MAX)
        return "a ragdoll of too many parts, or none";
    if (!read_material(fields + 5, &material) || !read_vector(fields + 7, VELOCITY_MAX, UNIT, velocity))
        return "a ragdoll's material or velocity out of range";
    if (!copy_parts(bridge, memory, fields[2], count) || !copy_poses(bridge, memory, fields[3], count, bridge->rest)
        || !copy_poses(bridge, memory, fields[4], count, bridge->poses))
        return "a ragdoll's arrays outside guest memory, or not finite";
    handle = physics_ragdoll_add(bridge->world, bridge->parts, count, bridge->rest, bridge->poses, &material);
    if (handle == PHYSICS_NONE)
        return "a ragdoll the world refused";
    physics_ragdoll_set_velocity(bridge->world, handle, velocity);
    *slot = handle;
    bridge->ragdoll_parts[fields[0]] = (uint8_t)count;
    bridge->ragdoll_count++;
    return NULL;
}

static const char *ragdoll_command(physics_bridge *bridge, const physics_bridge_memory *memory, uint32_t op,
                                   const uint32_t *fields)
{
    uint32_t *slot = ragdoll_slot(bridge, fields[0], 1), parts;
    float impulse[3], point[3], frequency;

    if (slot == NULL)
        return "a ragdoll id that names none";
    parts = bridge->ragdoll_parts[fields[0]];
    switch (op) {
    case HOST_PHYSICS_OP_RAGDOLL_REMOVE:
        physics_ragdoll_remove(bridge->world, *slot);
        *slot = PHYSICS_NONE;
        bridge->ragdoll_count--;
        return NULL;
    case HOST_PHYSICS_OP_RAGDOLL_DRIVE:
        frequency = word_float(fields[1]);
        if (!isfinite(frequency) || frequency <= 0.0f || frequency > 100.0f || fields[3] != parts)
            return "a drive's frequency or count out of range";
        if (!copy_poses(bridge, memory, fields[2], parts, bridge->poses))
            return "a drive's poses outside guest memory, or not finite";
        return physics_ragdoll_drive(bridge->world, *slot, bridge->poses, parts, frequency) == 0
                   ? NULL : "a drive the world refused";
    case HOST_PHYSICS_OP_RAGDOLL_LIMP:
        physics_ragdoll_go_limp(bridge->world, *slot);
        return NULL;
    case HOST_PHYSICS_OP_RAGDOLL_IMPULSE:
        if (fields[1] >= parts || !read_vector(fields + 2, IMPULSE_MAX, UNIT, impulse)
            || !read_vector(fields + 5, POSITION_MAX, UNIT, point))
            return "an impulse's part or numbers out of range";
        return physics_ragdoll_add_impulse(bridge->world, *slot, fields[1], impulse, point) == 0
                   ? NULL : "an impulse the world refused";
    case HOST_PHYSICS_OP_RAGDOLL_BREAK:
        if (fields[1] >= parts)
            return "a break's part out of range";
        return physics_ragdoll_break_joint(bridge->world, *slot, fields[1]) == 0
                   ? NULL : "a joint that is the root's, or broken";
    }
    return "an op that is not a ragdoll's";
}

static const char *add_body(physics_bridge *bridge, const uint32_t *fields)
{
    uint32_t *slot = body_slot(bridge, fields[0], 0), handle;
    physics_shape shape;
    physics_pose pose;
    physics_material material;
    float mass = word_float(fields[6]), velocity[3], angular[3];

    if (slot == NULL)
        return "a body id out of range or in use";
    if (fields[1] != HOST_PHYSICS_MOTION_KINEMATIC && fields[1] != HOST_PHYSICS_MOTION_DYNAMIC)
        return "a body's motion that is none";
    memset(&shape, 0, sizeof(shape));
    shape.offset.rotation[3] = 1.0f;
    if (!read_shape(fields + 2, &shape) || !isfinite(mass) || !read_material(fields + 7, &material)
        || !read_pose(fields + 9, &pose) || !read_vector(fields + 16, VELOCITY_MAX, UNIT, velocity)
        || !read_vector(fields + 19, 1000.0f, 1.0f, angular))
        return "a body's shape or numbers out of range";
    handle = physics_body_add(bridge->world, &shape, &pose,
                              fields[1] == HOST_PHYSICS_MOTION_KINEMATIC ? PHYSICS_MOTION_KINEMATIC
                                                                         : PHYSICS_MOTION_DYNAMIC,
                              mass, &material);
    if (handle == PHYSICS_NONE)
        return "a body the world refused";
    if (fields[1] == HOST_PHYSICS_MOTION_DYNAMIC)
        physics_body_set_velocity(bridge->world, handle, velocity, angular);
    *slot = handle;
    bridge->body_count++;
    return NULL;
}

static const char *body_command(physics_bridge *bridge, uint32_t op, const uint32_t *fields, float seconds)
{
    uint32_t *slot = body_slot(bridge, fields[0], 1);
    physics_pose pose;
    float impulse[3], point[3];

    if (slot == NULL)
        return "a body id that names none";
    switch (op) {
    case HOST_PHYSICS_OP_BODY_MOVE:
        if (!read_pose(fields + 1, &pose))
            return "a move's pose out of range";
        if (seconds <= 0.0f)
            return "a move in a batch that does not step";
        return physics_body_move(bridge->world, *slot, &pose, seconds) == 0 ? NULL : "a move the world refused";
    case HOST_PHYSICS_OP_BODY_REMOVE:
        physics_body_remove(bridge->world, *slot);
        *slot = PHYSICS_NONE;
        bridge->body_count--;
        return NULL;
    case HOST_PHYSICS_OP_BODY_IMPULSE:
        if (!read_vector(fields + 1, IMPULSE_MAX, UNIT, impulse) || !read_vector(fields + 4, POSITION_MAX, UNIT, point))
            return "an impulse's numbers out of range";
        return physics_body_add_impulse(bridge->world, *slot, impulse, point) == 0 ? NULL
                                                                                  : "an impulse the world refused";
    }
    return "an op that is not a body's";
}

static const char *run_command(physics_bridge *bridge, const physics_bridge_memory *memory, uint32_t op,
                               const uint32_t *fields, float seconds)
{
    switch (op) {
    case HOST_PHYSICS_OP_CLEAR:
        clear(bridge);
        return NULL;
    case HOST_PHYSICS_OP_LEVEL:
        return set_level(bridge, memory, fields);
    case HOST_PHYSICS_OP_RAGDOLL_ADD:
        return add_ragdoll(bridge, memory, fields);
    case HOST_PHYSICS_OP_BODY_ADD:
        return add_body(bridge, fields);
    case HOST_PHYSICS_OP_RAGDOLL_REMOVE:
    case HOST_PHYSICS_OP_RAGDOLL_DRIVE:
    case HOST_PHYSICS_OP_RAGDOLL_LIMP:
    case HOST_PHYSICS_OP_RAGDOLL_IMPULSE:
    case HOST_PHYSICS_OP_RAGDOLL_BREAK:
        return ragdoll_command(bridge, memory, op, fields);
    default:
        return body_command(bridge, op, fields, seconds);
    }
}

/* ---------- the results */

static uint32_t float_word(float value)
{
    uint32_t word;
    memcpy(&word, &value, sizeof(word));
    return word;
}

static void write_pose(uint32_t *to, const physics_pose *pose)
{
    int i;
    for (i = 0; i < 3; i++)
        to[i] = float_word(pose->position[i] / UNIT);
    for (i = 0; i < 4; i++)
        to[3 + i] = float_word(pose->rotation[i]);
}

static void write_results(physics_bridge *bridge, uint32_t *out, uint32_t words_max)
{
    uint32_t length = HOST_PHYSICS_RESULTS_HEADER_WORDS, ragdolls = 0, bodies = 0, flags = 0, id, i;
    physics_pose pose;

    for (id = 1; id <= HOST_PHYSICS_RAGDOLLS_MAX && !flags; id++) {
        uint32_t handle = bridge->ragdolls[id], parts = bridge->ragdoll_parts[id], broken = 0;
        if (handle == PHYSICS_NONE)
            continue;
        if (length + HOST_PHYSICS_RESULT_RAGDOLL_WORDS + parts * HOST_PHYSICS_POSE_WORDS > words_max) {
            flags = HOST_PHYSICS_RESULTS_TRUNCATED;
            break;
        }
        physics_ragdoll_get_poses(bridge->world, handle, bridge->poses, parts);
        for (i = 1; i < parts; i++)
            if (physics_ragdoll_joint_broken(bridge->world, handle, i) == 1)
                broken |= 1u << i;
        out[length] = id;
        out[length + 1] = parts;
        out[length + 2] = physics_ragdoll_active(bridge->world, handle) == 1;
        out[length + 3] = broken;
        length += HOST_PHYSICS_RESULT_RAGDOLL_WORDS;
        for (i = 0; i < parts; i++, length += HOST_PHYSICS_POSE_WORDS)
            write_pose(out + length, &bridge->poses[i]);
        ragdolls++;
    }
    for (id = 1; id <= HOST_PHYSICS_BODIES_MAX && !flags; id++) {
        uint32_t handle = bridge->bodies[id];
        if (handle == PHYSICS_NONE)
            continue;
        if (length + HOST_PHYSICS_RESULT_BODY_WORDS > words_max) {
            flags = HOST_PHYSICS_RESULTS_TRUNCATED;
            break;
        }
        physics_body_get_pose(bridge->world, handle, &pose);
        out[length] = id;
        out[length + 1] = physics_body_active(bridge->world, handle) == 1;
        write_pose(out + length + 2, &pose);
        length += HOST_PHYSICS_RESULT_BODY_WORDS;
        bodies++;
    }
    out[0] = HOST_PHYSICS_RESULTS_MAGIC;
    out[1] = HOST_PHYSICS_VERSION;
    out[2] = length * 4;
    out[3] = ragdolls;
    out[4] = bodies;
    out[5] = flags;
    out[6] = physics_world_active_bodies(bridge->world);
}

/* ---------- a step */

static int refuse_whole(char *reason, uint32_t reason_size, const char *why)
{
    if (reason != NULL && reason_size != 0)
        snprintf(reason, reason_size, "%s", why);
    return -1;
}

int physics_bridge_step(physics_bridge *bridge, const physics_bridge_memory *memory, uint32_t batch, uint32_t size,
                        uint32_t results, uint32_t results_size, physics_bridge_refused refused, void *context,
                        char *reason, uint32_t reason_size)
{
    const uint32_t *words = bridge->batch;
    uint32_t *out;
    const void *from;
    uint32_t length, commands, offset, n;
    int refusals = 0;
    float seconds;

    if (reason != NULL && reason_size != 0)
        reason[0] = '\0';
    if (size < HOST_PHYSICS_BATCH_HEADER_WORDS * 4 || size > HOST_PHYSICS_MAX_BATCH_LENGTH || (size & 3) != 0
        || (batch & 3) != 0)
        return refuse_whole(reason, reason_size, "a batch of a size out of range");
    if (results_size < HOST_PHYSICS_RESULTS_HEADER_WORDS * 4 || (results & 3) != 0)
        return refuse_whole(reason, reason_size, "a results buffer too small or not aligned");
    from = memory->pointer(memory->context, batch, size);
    out = memory->pointer(memory->context, results, results_size);
    if (from == NULL || out == NULL)
        return refuse_whole(reason, reason_size, "a batch or results outside guest memory");
    memcpy(bridge->batch, from, size);
    length = size / 4;
    if (words[0] != HOST_PHYSICS_BATCH_MAGIC || words[1] != HOST_PHYSICS_VERSION)
        return refuse_whole(reason, reason_size, "a batch of another magic or version");
    if (words[2] != size)
        return refuse_whole(reason, reason_size, "a batch whose size is not its header's");
    seconds = word_float(words[4]);
    if (!isfinite(seconds) || seconds < 0.0f || seconds > HOST_PHYSICS_STEP_MAX)
        return refuse_whole(reason, reason_size, "a step out of range");
    commands = words[3];

    offset = HOST_PHYSICS_BATCH_HEADER_WORDS;
    for (n = 0; n < commands; n++) {
        uint32_t op, count;
        const char *why;

        if (offset >= length) {
            if (refused != NULL)
                refused(context, offset * 4, 0, "the batch ends before its commands");
            refusals++;
            break;
        }
        op = HOST_PHYSICS_COMMAND_OP(words[offset]);
        count = HOST_PHYSICS_COMMAND_WORDS(words[offset]);
        if (count == 0 || count > length - offset) {
            if (refused != NULL)
                refused(context, offset * 4, op, "a command that runs past the batch");
            refusals++;
            break;
        }
        if (op >= HOST_PHYSICS_OP_COUNT || command_words[op] == 0)
            why = "an op that is none";
        else if (count != command_words[op])
            why = "a command of the wrong size";
        else
            why = run_command(bridge, memory, op, words + offset + 1, seconds);
        if (why != NULL) {
            if (refused != NULL)
                refused(context, offset * 4, op, why);
            refusals++;
        }
        offset += count;
    }
    if (seconds > 0.0f)
        physics_world_step(bridge->world, seconds);
    write_results(bridge, out, results_size / 4);
    return refusals;
}
