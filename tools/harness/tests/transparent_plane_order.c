/* CPU fake buffers and camera; production queue, skinning and math below. */
#include "harness.h"
#include <math.h>
#include <stddef.h>

typedef unsigned short word;
typedef struct { short x, y; } point2d;
typedef struct { real x, y; } real_point2d;
typedef struct { real i, j; } real_vector2d;
typedef struct { real red, green, blue; } real_rgb_color;
typedef struct { real alpha; real_rgb_color rgb; } real_argb_color;
typedef struct { real_vector3d n; real d; } real_plane3d;
#define TAG_STRING_LENGTH 31
#define VALID_INDEX(i, n) ((i) >= 0 && (i) < (n))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PIN(x, a, b) MIN(MAX(x, a), b)
#define D3DLOCK_READONLY 16
#define __cdecl
#include "types.inc"
typedef struct real_matrix4x3 real_matrix4x3;

static struct transparent_geometry_group storage[RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS];
static struct transparent_geometry_group *transparent_geometry_groups = storage;
static short order[RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS];
static short *transparent_geometry_group_sorted_indices = order;
static short transparent_geometry_group_count;
static struct { struct { real_point3d position; } camera; } global_window_parameters;
static struct shader_transparent_glass_definition glass;
static struct shader_transparent_generic_definition materials[NUMBER_OF_SHADER_TYPES];
static struct vertex_buffer buffers[RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS];
static struct model_vertex_uncompressed vertices[RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS][2];
static struct model_vertex_compressed compressed[RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS][2];
static real_matrix4x3 matrices[RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS][2];
static long locks, unlocks;

static void *rasterizer_transparent_geometry_get_group_from_presorted_index(short index)
{
    CHECK(VALID_INDEX(index, transparent_geometry_group_count), "invalid group lookup %d", index);
    return storage + index;
}

static long rasterizer_geometry_get_vertex_size(short type)
{
    return type == _rasterizer_vertex_type_model_compressed ? sizeof(struct model_vertex_compressed) :
        sizeof(struct model_vertex_uncompressed);
}

static void IDirect3DVertexBuffer8_Lock(void *buffer, long offset, long size, byte **data, long flags)
{
    CHECK(offset == 0 && size == 0 && flags == D3DLOCK_READONLY, "read-only buffer lock");
    ++locks;
    *data = buffer;
}

static void IDirect3DVertexBuffer8_Unlock(void *buffer)
{
    CHECK(buffer != NULL, "unlock valid buffer");
    ++unlocks;
}

/* The fake Chicago material only needs its common transparent flag prefix. */
#define SHADER_GET_TRANSPARENT_GENERIC(s) (&((struct shader_transparent_generic_definition *)(s))->generic)
#define SHADER_GET_TRANSPARENT_CHICAGO(s) SHADER_GET_TRANSPARENT_GENERIC(s)
#define _shader_transparent_draw_before_water_bit _shader_transparent_flag_draw_before_water_bit

#include "under_test.inc"

static real_matrix4x3 identity(void)
{
    real_matrix4x3 result = {0};
    result.scale = 1;
    result.forward.i = result.left.j = result.up.k = 1;
    return result;
}

static void reset(short count)
{
    short i;
    memset(storage, 0, sizeof(storage));
    memset(buffers, 0, sizeof(buffers));
    memset(vertices, 0, sizeof(vertices));
    memset(compressed, 0, sizeof(compressed));
    memset(&glass, 0, sizeof(glass));
    memset(materials, 0, sizeof(materials));
    glass.shader.base.type = _shader_type_transparent_glass;
    transparent_geometry_group_count = count;
    for (i = 0; i < NUMBER_OF_SHADER_TYPES; ++i)
    {
        materials[i].shader.type = i;
    }
    for (i = 0; i < count; ++i)
    {
        order[i] = i;
        storage[i].previous_group_presorted_index = NONE;
        storage[i].next_group_presorted_index = NONE;
        matrices[i][0] = matrices[i][1] = identity();
    }
    global_window_parameters.camera.position = (real_point3d){0, 0, 5};
    locks = unlocks = 0;
}

static void floor_at(short index, real height)
{
    storage[index].shader = &glass.shader;
    storage[index].geometry_flags = FLAG(_rasterizer_geometry_no_sort_bit);
    storage[index].plane.n.k = 1;
    storage[index].plane.d = height;
}

static void model_at(short index, real low, real high)
{
    storage[index].shader = (struct shader *)&materials[_shader_type_transparent_generic];
    storage[index].object_index = index + 100;
    storage[index].vertex_buffer = buffers + index;
    storage[index].node_matrices = matrices[index];
    storage[index].node_matrix_count = 1;
    buffers[index].type = _rasterizer_vertex_type_model_uncompressed;
    buffers[index].count = 2;
    buffers[index].hardware_format = vertices[index];
    vertices[index][0].position = (real_point3d){-1, -1, low};
    vertices[index][1].position = (real_point3d){1, 1, high};
    vertices[index][0].nodes[1] = vertices[index][1].nodes[1] = NONE;
    vertices[index][0].node_weights[0] = vertices[index][1].node_weights[0] = 1;
}

static void link_parts(short first, short second)
{
    storage[first].next_group_presorted_index = second;
    storage[second].previous_group_presorted_index = first;
    storage[second].object_index = storage[first].object_index;
}

static void expect(short const *expected, long count)
{
    long i;
    rasterizer_transparent_geometry_order_models(order, count);
    for (i = 0; i < count; ++i)
    {
        CHECK(order[i] == expected[i], "order[%ld]=%d, expected %d", i, order[i], expected[i]);
    }
    CHECK(locks == unlocks, "balanced CPU buffer access");
}

#define EXPECT(...) do { short expected[] = {__VA_ARGS__}; expect(expected, sizeof(expected)/sizeof(*expected)); } while (0)

int main(int argc, char **argv)
{
    char const *case_name;
    short i;
    CHECK(argc == 2, "one named case");
    case_name = argv[1];
    reset(2);
    model_at(0, 0, 2);
    floor_at(1, 0);

    CASE("same-side") { EXPECT(1, 0); return 0; }
    CASE("opposite-side")
    {
        order[0] = 1; order[1] = 0;
        global_window_parameters.camera.position.z = -5;
        EXPECT(0, 1);
        return 0;
    }
    CASE("reverse-normal")
    {
        storage[1].plane.n.k = -1;
        EXPECT(1, 0);
        return 0;
    }
    CASE("unrelated")
    {
        reset(3); floor_at(0, 0); model_at(2, 1, 2);
        global_window_parameters.camera.position.z = -5;
        EXPECT(2, 0, 1); /* The intervening widget stays behind the floor. */
        reset(3); model_at(0, 1, 2); floor_at(2, 0);
        EXPECT(1, 2, 0); /* Opposite move: widget stays before the floor. */
        reset(3); floor_at(1, 0); model_at(2, 1, 2);
        global_window_parameters.camera.position.z = -5;
        EXPECT(0, 2, 1); /* Move beside the glass, after an existing widget. */
        reset(3); model_at(0, 1, 2); floor_at(1, 0);
        EXPECT(1, 0, 2); /* The arriving model precedes the following widget. */
        return 0;
    }
    CASE("multiple-models")
    {
        reset(5); floor_at(0, 0); model_at(2, 1, 2); model_at(4, 1, 2);
        global_window_parameters.camera.position.z = -5;
        EXPECT(2, 4, 0, 1, 3);
        return 0;
    }
    CASE("multiple-planes")
    {
        reset(5); floor_at(0, 0); floor_at(2, 2); model_at(4, .5f, 1.5f);
        EXPECT(0, 1, 4, 2, 3);
        return 0;
    }
    CASE("conflict")
    {
        reset(3); floor_at(0, 2); floor_at(1, 0); model_at(2, .5f, 1.5f);
        EXPECT(0, 1, 2);
        return 0;
    }
    CASE("intersect")
    {
        vertices[0][0].position.z = -1;
        EXPECT(0, 1);
        order[0] = 1; order[1] = 0;
        EXPECT(1, 0);
        return 0;
    }
    CASE("camera-plane")
    {
        global_window_parameters.camera.position.z = 0;
        EXPECT(0, 1);
        return 0;
    }
    CASE("invalid-plane")
    {
        real values[] = {0, .2f, 2, NAN, INFINITY};
        for (i = 0; i < 5; ++i)
        {
            storage[1].plane.n.k = values[i];
            EXPECT(0, 1);
        }
        CHECK(locks == 0, "invalid planes do not read models");
        storage[1].plane.n.k = 1;
        storage[1].plane.d = NAN;
        EXPECT(0, 1);
        return 0;
    }
    CASE("invalid-model")
    {
        vertices[0][0].position.x = NAN;
        EXPECT(0, 1);
        vertices[0][0].position.x = -1;
        matrices[0][0].scale = INFINITY;
        EXPECT(0, 1);
        matrices[0][0].scale = 1;
        buffers[0].offset = 4;
        EXPECT(0, 1);
        buffers[0].offset = 0;
        buffers[0].count = 0;
        EXPECT(0, 1);
        return 0;
    }
    CASE("materials")
    {
        short types[] = {_shader_type_transparent_generic, _shader_type_transparent_chicago,
            _shader_type_transparent_glass, _shader_type_transparent_meter, _shader_type_transparent_plasma};
        for (i = 0; i < 5; ++i)
        {
            order[0] = 0; order[1] = 1;
            storage[0].shader = (struct shader *)(materials + types[i]);
            EXPECT(1, 0);
        }
        return 0;
    }
    CASE("rigid-transform")
    {
        real bounds[2][3];
        matrices[0][0].forward = (real_vector3d){0, 0, 1};
        matrices[0][0].up = (real_vector3d){-1, 0, 0};
        matrices[0][0].scale = 2;
        matrices[0][0].position.z = 3;
        CHECK(transparent_model_world_bounds(storage, bounds), "rotated and scaled bounds");
        CHECK(bounds[0][2] == 1 && bounds[1][2] == 5, "real matrix transform applied");
        EXPECT(1, 0);
        return 0;
    }
    CASE("skinned")
    {
        real bounds[2][3];
        storage[0].node_matrix_count = 2;
        matrices[0][0].position.z = -8;
        matrices[0][1].position.z = 8;
        for (i = 0; i < 2; ++i)
        {
            vertices[0][i].nodes[1] = 1;
            vertices[0][i].node_weights[0] = .25f;
            vertices[0][i].node_weights[1] = .75f;
        }
        CHECK(transparent_model_world_bounds(storage, bounds), "weighted two-node bounds");
        CHECK(bounds[0][2] == 4 && bounds[1][2] == 6, "actual weights and both nodes");
        EXPECT(1, 0);
        vertices[0][0].nodes[1] = 2;
        order[0] = 0; order[1] = 1;
        EXPECT(0, 1);
        vertices[0][0].nodes[1] = 1;
        vertices[0][0].node_weights[0] = NAN;
        EXPECT(0, 1);
        return 0;
    }
    CASE("compressed-skinning")
    {
        real bounds[2][3];
        storage[0].node_matrix_count = 2;
        matrices[0][0].position.z = -8;
        matrices[0][1].position.z = 8;
        buffers[0].type = _rasterizer_vertex_type_model_compressed;
        buffers[0].hardware_format = compressed[0];
        for (i = 0; i < 2; ++i)
        {
            compressed[0][i].position = vertices[0][i].position;
            compressed[0][i].nodes[1] = 3;
            compressed[0][i].node_weight = 8192;
        }
        CHECK(transparent_model_world_bounds(storage, bounds), "compressed node encoding");
        CHECK(fabs(bounds[0][2] - (8.f - 16.f * 8192.f/32767.f)) < .00001f, "signed normalized short matches GPU declaration");
        EXPECT(1, 0);
        order[0] = 0; order[1] = 1;
        compressed[0][0].nodes[1] = 4;
        EXPECT(0, 1);
        return 0;
    }
    CASE("linked")
    {
        reset(4); model_at(0, 1, 2); floor_at(1, 0); model_at(3, 0, 3);
        link_parts(0, 3);
        EXPECT(1, 0, 3, 2);
        reset(4); model_at(0, 1, 2); floor_at(1, 0); model_at(3, 0, 3);
        link_parts(0, 3);
        order[0] = 3; order[1] = 1; order[2] = 2; order[3] = 0;
        EXPECT(1, 0, 3, 2); /* The earliest queued member dispatches the same chain. */
        return 0;
    }
    CASE("linked-union")
    {
        reset(3); model_at(0, -2, -1); model_at(1, 1, 2); floor_at(2, 0);
        link_parts(0, 1);
        EXPECT(0, 1, 2); /* Whole chain intersects; bounding only the last part is unsafe. */
        storage[0].effect_type = 1;
        EXPECT(0, 1, 2); /* Unsupported first member invalidates the entire chain. */
        return 0;
    }
    CASE("invalid-links")
    {
        storage[0].previous_group_presorted_index = 5;
        EXPECT(0, 1);
        storage[0].previous_group_presorted_index = NONE;
        storage[0].next_group_presorted_index = 5;
        EXPECT(0, 1);
        reset(3); floor_at(0, 0); model_at(1, 1, 2); model_at(2, 1, 2);
        link_parts(1, 2); link_parts(2, 1);
        EXPECT(0, 1, 2);
        return 0;
    }
    CASE("full-queue")
    {
        short seen[RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS] = {0};
        reset(RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS);
        floor_at(0, 0);
        global_window_parameters.camera.position.z = -5;
        for (i = 1; i < transparent_geometry_group_count; ++i)
        {
            model_at(i, 1, 2);
        }
        rasterizer_transparent_geometry_order_models(order, transparent_geometry_group_count);
        CHECK(order[transparent_geometry_group_count-1] == 0, "full queue moves models before glass");
        for (i = 0; i < transparent_geometry_group_count; ++i)
        {
            CHECK(VALID_INDEX(order[i], transparent_geometry_group_count) && !seen[order[i]]++, "permutation preserved");
            CHECK(i == transparent_geometry_group_count-1 || order[i] == i+1, "stable model order");
        }
        CHECK(locks == transparent_geometry_group_count-1 && unlocks == locks, "one scan per model");
        return 0;
    }
    CASE("four-views")
    {
        for (i = 0; i < 4; ++i)
        {
            order[0] = 0; order[1] = 1;
            global_window_parameters.camera.position.z = i & 1 ? -5 : 5;
            if (i & 1) { EXPECT(0, 1); } else { EXPECT(1, 0); }
        }
        return 0;
    }
    CASE("no-glass")
    {
        storage[1].shader = NULL;
        EXPECT(0, 1);
        CHECK(!locks, "no glass has no model scan cost");
        return 0;
    }
    CASE("external-sort")
    {
        storage[0].z_sort = -20;
        storage[1].z_sort = -10;
        rasterizer_sort_external();
        CHECK(order[0] == 1 && order[1] == 0, "correction runs after actual qsort");
        CHECK(storage[1].sorted_index == 0 && storage[0].sorted_index == 1, "published sorted indices agree");
        return 0;
    }
    CASE("fallback-flags")
    {
        short flags[] = {_rasterizer_geometry_no_sort_bit, _rasterizer_geometry_no_queue_bit,
            _rasterizer_geometry_no_zbuffer_bit, _rasterizer_geometry_sky_bit,
            _rasterizer_geometry_viewspace_bit, _rasterizer_geometry_first_person_bit};
        for (i = 0; i < 6; ++i)
        {
            storage[0].geometry_flags = FLAG(flags[i]);
            EXPECT(0, 1);
        }
        storage[0].geometry_flags = 0;
        storage[0].active_camouflage_transparent_source_object_index = 100;
        EXPECT(0, 1);
        storage[0].active_camouflage_transparent_source_object_index = 0;
        materials[_shader_type_transparent_generic].generic.flags = FLAG(_shader_transparent_flag_draw_before_water_bit);
        EXPECT(0, 1);
        materials[_shader_type_transparent_generic].generic.flags = 0;
        storage[0].shader = (struct shader *)&materials[_shader_type_transparent_water];
        EXPECT(0, 1);
        storage[0].shader = (struct shader *)&materials[_shader_type_transparent_generic];
        glass.flags = FLAG(_shader_transparent_glass_flag_decal_bit);
        EXPECT(0, 1);
        glass.flags = 0;
        glass.reflection_type = _shader_transparent_glass_reflection_type_dynamic_mirror;
        EXPECT(0, 1);
        return 0;
    }
    CASE("invalid-order")
    {
        order[0] = order[1] = 0;
        EXPECT(0, 0);
        order[0] = 0; order[1] = 2;
        EXPECT(0, 2);
        CHECK(locks == 0, "invalid order rejected before buffer access");
        return 0;
    }
    CHECK(FALSE, "unknown case %s", case_name);
    return 1;
}
