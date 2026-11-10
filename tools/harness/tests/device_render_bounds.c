/* A device's render sphere grown to hold its model wherever its position and power animations carry it
(port/linux/game/device_render_bounds.c). The fake animations move a node by a translation along z and a scale,
as a30's beam emitter's position animation moves its beam; node matrices have no rotation. */
#include "harness.h"
#include <float.h>
#include <math.h>

typedef unsigned short word;
typedef struct { short x, y; } point2d;
typedef struct { real x, y; } real_point2d;
typedef struct { real i, j, k, w; } real_quaternion;
typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;
typedef struct { real_quaternion rotation; real_point3d translation; real scale; } real_orientation;
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))
#define VALID_INDEX(index, count) ((index) >= 0 && (index) < (count))
#define UNSIGNED_SHORT_MAX 0xFFFF
#define MAXIMUM_NODES_PER_MODEL 64
#define MAXIMUM_GEOMETRIES_PER_MODEL 256
#define MAXIMUM_PARTS_PER_MODEL_GEOMETRY 32
#define MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART UNSIGNED_SHORT_MAX
#define DEVICE_DEFINITION_TAG 0x64657669
#define D3DLOCK_READONLY 0x10
struct tag_block { long count; void *address; void *definition; };
struct tag_reference { long group_tag; char *name; long name_length; long index; };
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)((char *)(block)->address + (index) * sizeof(type)))
struct vertex_buffer { short type; word pad; long count; long offset; void *base_address; void *hardware_format; };
struct triangle_buffer { short type; word pad; long count; void *base_address; void *hardware_format; };
#include "under_test_types.inc"

static real_point3d const origin = { 0.f, 0.f, 0.f };
static real_vector3d const forward = { 1.f, 0.f, 0.f }, up = { 0.f, 0.f, 1.f };
static real_point3d const *global_origin3d = &origin;
static real_vector3d const *global_forward3d = &forward, *global_up3d = &up;

static real square_root(real value) { return sqrtf(value); }
static real distance_squared3d(real_point3d const *a, real_point3d const *b)
{
    real x = a->x - b->x, y = a->y - b->y, z = a->z - b->z;

    return x * x + y * y + z * z;
}
/* (no rotations: a scale and a translation) */
static void matrix4x3_transform_point(real_matrix4x3 const *matrix, real_point3d const *point, real_point3d *out)
{
    real_point3d in = *point;

    out->x = matrix->scale * in.x + matrix->position.x;
    out->y = matrix->scale * in.y + matrix->position.y;
    out->z = matrix->scale * in.z + matrix->position.z;
}
static void matrix4x3_multiply(real_matrix4x3 const *a, real_matrix4x3 const *b, real_matrix4x3 *out)
{
    real_matrix4x3 product = *a;

    product.scale = a->scale * b->scale;
    matrix4x3_transform_point(a, &b->position, &product.position);
    *out = product;
}

/* the model, its graph and the device */
struct model_node { real_point3d default_translation; real_matrix4x3 runtime_default_inverse_matrix; };
struct model { struct tag_block nodes; struct tag_block geometries; };
struct animation_graph_animation_index { short animation_index; };
#define animation_graph_animation_index_get(block) ((struct animation_graph_animation_index *)((block)->address))
/* a fake animation: per frame, a node's z translation and scale */
struct animation { short frame_count; short node_count; short node; real z[4]; real scale[4]; };
struct animation_graph { struct tag_block device_animations; struct tag_block animations; struct tag_block nodes; };
struct device_definition
{
    struct { struct tag_reference model; struct tag_reference animation_graph; real_point3d bounding_offset;
        real render_bounding_radius; } object;
    struct { unsigned int flags; } device;
};

static struct model model;
static struct model_node nodes[2];
static struct model_geometry geometry;
static struct model_geometry_part parts[2];
static struct model_vertex_compressed vertices[8];
static struct model_vertex_uncompressed uncompressed_vertices[8];
static struct animation_graph graph;
static struct device_render_bounds_animations device_animations;
static struct animation_graph_animation_index animation_indices[2];
static struct animation animations[2];
static struct device_definition device;
static long continuous_calls, frame_calls;

static struct model *model_definition_get(long index) { CHECK(index == 3, "the device's model"); return &model; }
static struct animation_graph *animation_graph_definition_get(long index) { CHECK(index == 4, "its graph"); return &graph; }
static struct device_definition *device_definition_get(long index) { CHECK(index == 9, "the device"); return &device; }
struct tag_iterator { long next; };
static void tag_iterator_new(struct tag_iterator *iterator, long group_tag)
{
    CHECK(group_tag == DEVICE_DEFINITION_TAG, "devices");
    iterator->next = 9;
}
static long tag_iterator_next(struct tag_iterator *iterator) { long next = iterator->next; iterator->next = NONE; return next; }

static void model_get_node_orientations(struct model const *in, real_orientation *orientations)
{
    long index;

    for (index = 0; index < in->nodes.count; index++)
    {
        memset(&orientations[index], 0, sizeof(orientations[index]));
        orientations[index].rotation.w = 1.f;
        orientations[index].translation = nodes[index].default_translation;
        orientations[index].scale = 1.f;
    }
}
static void animation_frame_apply(struct animation const *animation, real frame, real_orientation *orientations)
{
    long first = (long)floorf(frame) % animation->frame_count;
    long second = (first + 1) % animation->frame_count;
    real fraction = frame - floorf(frame);

    orientations[animation->node].translation.z += animation->z[first] + (animation->z[second] - animation->z[first]) * fraction;
    orientations[animation->node].scale *=
        animation->scale[first] + (animation->scale[second] - animation->scale[first]) * fraction;
}
static void overlay_animation_apply_continuous(struct animation const *animation, real frame, real_orientation *orientations)
{
    continuous_calls++;
    animation_frame_apply(animation, frame, orientations);
}
static void overlay_animation_apply(struct animation const *animation, short frame, real_orientation *orientations)
{
    frame_calls++;
    animation_frame_apply(animation, (real)frame, orientations);
}
/* (by the model's nodes: each node's matrix its own, then the root's) */
static void model_node_matrices_from_orientations(struct model const *in, real_matrix4x3 *matrices,
    real_orientation const *orientations, real_point3d const *at, real_vector3d const *facing, real_vector3d const *upward)
{
    long index;

    CHECK(in == &model && at == global_origin3d && facing == global_forward3d && upward == global_up3d,
        "the model, at rest in the object's space");
    for (index = 0; index < in->nodes.count; index++)
    {
        memset(&matrices[index], 0, sizeof(matrices[index]));
        matrices[index].scale = orientations[index].scale;
        matrices[index].position = orientations[index].translation;
        if (index)
            matrix4x3_multiply(&matrices[0], &matrices[index], &matrices[index]);
    }
}
static void IDirect3DVertexBuffer8_Lock(void *buffer, int offset, int size, byte **data, int flags)
{
    (void)offset; (void)size; (void)flags;
    *data = buffer;
}
static void IDirect3DVertexBuffer8_Unlock(void *buffer) { (void)buffer; }
static long rasterizer_geometry_get_vertex_size(short type)
{
    return type == _rasterizer_vertex_type_model_compressed ?
        (long)sizeof(struct model_vertex_compressed) : (long)sizeof(struct model_vertex_uncompressed);
}

#include "under_test.inc"

static void identity(real_matrix4x3 *matrix)
{
    memset(matrix, 0, sizeof(*matrix));
    matrix->scale = 1.f;
    matrix->forward.i = matrix->left.j = matrix->up.k = 1.f;
}

static void vertex(long index, real x, real y, real z, byte node0, byte node1)
{
    memset(&vertices[index], 0, sizeof(vertices[index]));
    vertices[index].position.x = x;
    vertices[index].position.y = y;
    vertices[index].position.z = z;
    vertices[index].nodes[0] = (byte)(node0 * 3);
    vertices[index].nodes[1] = (byte)(node1 * 3);
    vertices[index].node_weight = 32767;
}

/* a30's beam emitter: its beam (node 1) from 341.33 below to 14.13 above, a sphere of 50 around 58 above it, and its
position animation, from a beam half as large at rest to one 1089.28 up */
static void beam_emitter(void)
{
    short index;

    memset(&model, 0, sizeof(model));
    memset(nodes, 0, sizeof(nodes));
    for (index = 0; index < 2; index++)
        identity(&nodes[index].runtime_default_inverse_matrix);
    model.nodes.count = 2;
    model.nodes.address = nodes;
    model.geometries.count = 1;
    model.geometries.address = &geometry;
    memset(&geometry, 0, sizeof(geometry));
    geometry.parts.count = 1;
    geometry.parts.address = parts;
    memset(parts, 0, sizeof(parts));
    parts[0].vertex_buffer.type = _rasterizer_vertex_type_model_compressed;
    parts[0].vertex_buffer.count = 4;
    parts[0].vertex_buffer.hardware_format = vertices;
    vertex(0, 1.83f, 0.f, -341.33f, 1, 1);
    vertex(1, -1.83f, 0.f, -341.33f, 1, 1);
    vertex(2, 0.f, 1.83f, 14.13f, 1, 1);
    vertex(3, 0.f, -1.83f, 14.13f, 1, 1);

    /* (a30's graph names no nodes: the model's are posed) */
    memset(&graph, 0, sizeof(graph));
    graph.device_animations.count = 1;
    graph.device_animations.address = &device_animations;
    memset(&device_animations, 0, sizeof(device_animations));
    device_animations.animations.count = 1;
    device_animations.animations.address = animation_indices;
    animation_indices[0].animation_index = 0;
    animation_indices[1].animation_index = 1;
    graph.animations.count = 2;
    graph.animations.address = animations;
    memset(animations, 0, sizeof(animations));
    animations[0].frame_count = 2;
    animations[0].node_count = 2;
    animations[0].node = 1;
    animations[0].z[1] = 1089.28f;
    animations[0].scale[0] = 0.495f;
    animations[0].scale[1] = 1.f;
    animations[1] = animations[0];
    animations[1].scale[0] = animations[1].scale[1] = 1.f;
    animations[1].z[1] = 0.f;

    memset(&device, 0, sizeof(device));
    device.object.model.index = 3;
    device.object.animation_graph.index = 4;
    device.object.bounding_offset.z = 58.f;
    device.object.render_bounding_radius = 50.f;
    continuous_calls = frame_calls = 0;
}

/* the farthest a vertex goes from the sphere's middle, at the animation's top */
#define BEAM_TOP_RADIUS 1045.41f

int main(int argc, char **argv)
{
    char const *case_name = argc > 1 ? argv[1] : "";
    CASE("beam") {
        beam_emitter();
        device_render_bounds_tags_loaded();
        CHECK(fabsf(device.object.render_bounding_radius - BEAM_TOP_RADIUS) < 0.2f,
            "the beam at the top of its travel (%f)", device.object.render_bounding_radius);
        CHECK(continuous_calls == DEVICE_RENDER_BOUNDS_STEPS + 1 && frame_calls == 0,
            "its whole travel, interpolated (%ld, %ld)", continuous_calls, frame_calls);
        device_render_bounds_tags_loaded();
        CHECK(fabsf(device.object.render_bounding_radius - BEAM_TOP_RADIUS) < 0.2f, "loaded again, the same");
    }
    CASE("grows only") {
        beam_emitter();
        device.object.render_bounding_radius = 2000.f;
        device_render_bounds_tags_loaded();
        CHECK(device.object.render_bounding_radius == 2000.f, "a sphere large enough already");
    }
    CASE("not moving") {
        /* a device without a position animation, or one that does not move its model: as it was */
        beam_emitter();
        graph.device_animations.count = 0;
        device_render_bounds_tags_loaded();
        CHECK(device.object.render_bounding_radius == 50.f, "no device animations");
        beam_emitter();
        animation_indices[0].animation_index = 7;
        device_render_bounds_tags_loaded();
        CHECK(device.object.render_bounding_radius == 50.f, "an animation the graph doesn't have");
        beam_emitter();
        animations[0].z[1] = 0.f;
        animations[0].scale[0] = 1.f;
        vertex(0, 0.f, 0.f, 20.f, 1, 1);
        vertex(1, 0.f, 0.f, 90.f, 1, 1);
        parts[0].vertex_buffer.count = 2;
        device_render_bounds_tags_loaded();
        CHECK(device.object.render_bounding_radius == 50.f, "a model inside its sphere, still");
    }
    CASE("both nodes") {
        /* a vertex of the base (node 0), half on the beam (node 1) */
        beam_emitter();
        vertex(0, 0.f, 0.f, 10.f, 0, 1);
        vertices[0].node_weight = 16384;
        parts[0].vertex_buffer.count = 1;
        device_render_bounds_tags_loaded();
        CHECK(device.object.render_bounding_radius > 1000.f,
            "as far as either of its nodes carries it (%f)", device.object.render_bounding_radius);
    }
    CASE("power") {
        /* a position animation that stays, and a power animation that lifts the beam 500 */
        beam_emitter();
        animations[0].z[1] = 0.f;
        animations[0].scale[0] = 1.f;
        animations[1].z[1] = 500.f;
        device_animations.animations.count = 2;
        device_render_bounds_tags_loaded();
        CHECK(fabsf(device.object.render_bounding_radius - (500.f + 14.13f - 58.f)) < 0.5f,
            "its power animation's travel too (%f)", device.object.render_bounding_radius);
        CHECK(continuous_calls == 2 * (DEVICE_RENDER_BOUNDS_STEPS + 1) * (DEVICE_RENDER_BOUNDS_STEPS + 1),
            "each position with each power (%ld)", continuous_calls);
    }
    CASE("root moves") {
        /* the whole model carried by its root: the sphere's middle with it */
        beam_emitter();
        animations[0].node = 0;
        animations[0].scale[0] = 1.f;
        device_render_bounds_tags_loaded();
        CHECK(fabsf(device.object.render_bounding_radius - 399.34f) < 0.2f,
            "around the bounding offset where the root carries it (%f)", device.object.render_bounding_radius);
    }
    CASE("not interpolated") {
        beam_emitter();
        device.device.flags = FLAG(_device_position_animation_not_interpolated_bit);
        device_render_bounds_tags_loaded();
        CHECK(frame_calls == DEVICE_RENDER_BOUNDS_STEPS + 1 && continuous_calls == 0, "its frames only");
        CHECK(fabsf(device.object.render_bounding_radius - BEAM_TOP_RADIUS) < 0.2f, "to its last frame");
    }
    CASE("unreadable") {
        beam_emitter();
        parts[0].flags = FLAG(_model_geometry_part_local_nodes_bit);
        device_render_bounds_tags_loaded();
        CHECK(device.object.render_bounding_radius == 50.f, "a part with its own node map");
        beam_emitter();
        parts[0].vertex_buffer.type = _rasterizer_vertex_type_model_compressed + 2;
        device_render_bounds_tags_loaded();
        CHECK(device.object.render_bounding_radius == 50.f, "a buffer not a model's");
        beam_emitter();
        vertices[2].position.z = NAN;
        device_render_bounds_tags_loaded();
        CHECK(device.object.render_bounding_radius == 50.f, "a vertex not a number");
        beam_emitter();
        animations[0].node_count = 3;
        device_render_bounds_tags_loaded();
        CHECK(device.object.render_bounding_radius == 50.f, "an animation of more nodes than the model");
    }
    CASE("uncompressed") {
        beam_emitter();
        memset(uncompressed_vertices, 0, sizeof(uncompressed_vertices));
        uncompressed_vertices[0].position.z = 14.13f;
        uncompressed_vertices[0].nodes[0] = 1;
        uncompressed_vertices[0].nodes[1] = NONE;
        parts[0].vertex_buffer.type = _rasterizer_vertex_type_model_uncompressed;
        parts[0].vertex_buffer.count = 1;
        parts[0].vertex_buffer.hardware_format = uncompressed_vertices;
        device_render_bounds_tags_loaded();
        CHECK(fabsf(device.object.render_bounding_radius - (1089.28f + 14.13f - 58.f)) < 0.2f,
            "an uncompressed vertex, its nodes not tripled (%f)", device.object.render_bounding_radius);
    }
    return 0;
}
