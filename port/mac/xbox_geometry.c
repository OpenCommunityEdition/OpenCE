#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "models/model_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "xbox_geometry.h"
#undef memcpy
#undef memset
#include <string.h>
#include <stdlib.h>

/* the part of a model's geometry as models.c and the rasterizer define it for themselves */
struct xbox_model_geometry { byte reserved[0x24]; struct tag_block parts; };
struct xbox_model_part
{
    unsigned long flags;
    short shader_index;
    char previous_part_index;
    char next_part_index;
    short centroid_primary_node_index;
    short centroid_secondary_node_index;
    real centroid_primary_node_weight;
    real centroid_secondary_node_weight;
    real_point3d centroid;
    struct tag_block uncompressed_vertices;
    struct tag_block compressed_vertices;
    struct tag_block triangles;
    struct triangle_buffer triangle_buffer;
    struct vertex_buffer vertex_buffer;
};
typedef char xbox_geometry_part_size[sizeof(struct xbox_model_part)==0x68?1:-1];
typedef char xbox_geometry_geometry_size[sizeof(struct xbox_model_geometry)==0x30?1:-1];
typedef char xbox_geometry_pointers[sizeof(void *)==4?1:-1];

static int force_swap = -1, forced_now;
void mac_xbox_geometry_force(int on) { forced_now = on; }
int mac_xbox_geometry_swap_enabled(void)
{
    if (forced_now) return 1;
    if (force_swap < 0) {
        const char *value = getenv("HALO_MAC_XBOX_FORCE_SWAP");
        force_swap = value && value[0] == '1';
#ifdef __BIG_ENDIAN__
        force_swap = 1;
#endif
    }
    return force_swap;
}
static void swap32(unsigned char *p) { unsigned char a = p[0], b = p[1]; p[0] = p[3]; p[1] = p[2]; p[2] = b; p[3] = a; }
static void swap16(unsigned char *p) { unsigned char a = p[0]; p[0] = p[1]; p[1] = a; }

static void (*visitor)(int kind, uint32_t address, uint32_t count);
void mac_xbox_geometry_set_visitor(void (*proc)(int kind, uint32_t address, uint32_t count)) { visitor = proc; }
static uint32_t counts_model, counts_environment, counts_lightmap, counts_indices;
void mac_xbox_geometry_counts(uint32_t *m, uint32_t *e, uint32_t *l, uint32_t *i)
{ *m = counts_model; *e = counts_environment; *l = counts_lightmap; *i = counts_indices; }

void mac_xbox_swap_model_vertices(void *p, uint32_t count)
{
    unsigned char *v = (unsigned char *)p;
    uint32_t i, k;
    if (!mac_xbox_geometry_swap_enabled()) return;
    for (i = 0; i < count; ++i, v += 0x20) {
        for (k = 0; k < 6; ++k) swap32(v + 4 * k);
        swap16(v + 0x18); swap16(v + 0x1a);
        swap16(v + 0x1e);
    }
}
void mac_xbox_swap_environment_vertices(void *p, uint32_t count)
{
    unsigned char *v = (unsigned char *)p;
    uint32_t i, k;
    if (!mac_xbox_geometry_swap_enabled()) return;
    for (i = 0; i < count; ++i, v += 0x20)
        for (k = 0; k < 8; ++k) swap32(v + 4 * k);
}
void mac_xbox_swap_lightmap_vertices(void *p, uint32_t count)
{
    unsigned char *v = (unsigned char *)p;
    uint32_t i;
    if (!mac_xbox_geometry_swap_enabled()) return;
    for (i = 0; i < count; ++i, v += 8) { swap32(v); swap16(v + 4); swap16(v + 6); }
}
void mac_xbox_swap_indices16(void *p, uint32_t count)
{
    unsigned char *v = (unsigned char *)p;
    uint32_t i;
    if (!mac_xbox_geometry_swap_enabled()) return;
    for (i = 0; i < count; ++i, v += 2) swap16(v);
}

/* ---------- walking the tags */
struct region { unsigned char *base; uint32_t virtual_base, size; };
static unsigned char *at(const struct region *r, uint32_t address, uint32_t bytes)
{
    uint32_t offset;
    if (address < r->virtual_base) return NULL;
    offset = address - r->virtual_base;
    if (offset > r->size || bytes > r->size - offset) return NULL;
    return r->base + offset;
}
static uint32_t word_at(const struct region *r, uint32_t address)
{
    const unsigned char *p = at(r, address, 4);
    uint32_t v;
    if (!p) return 0;
    memcpy(&v, p, 4);
    return v;
}
/* converted addresses, so that a buffer two descriptors name is done once */
#define DONE_CAPACITY 8192
struct done { uint32_t address[DONE_CAPACITY]; uint32_t count; };
static int done_test_add(struct done *d, uint32_t address)
{
    uint32_t i;
    for (i = 0; i < d->count; ++i) if (d->address[i] == address) return 0;
    if (d->count == DONE_CAPACITY) return 0;
    d->address[d->count++] = address;
    return 1;
}
/* a buffer descriptor (Common, Data, Lock): the address of its data, which is the
   full virtual address before the buffers are registered, an offset after */
static uint32_t descriptor_data(const struct region *r, uint32_t descriptor)
{
    uint32_t data = descriptor ? word_at(r, descriptor + 4) : 0;
    if (data && data < 0x80000000UL) data |= 0x80000000UL;
    return data;
}
static int convert_vertices(const struct region *r, struct done *d, const struct vertex_buffer *buffer, int *error)
{
    uint32_t data = descriptor_data(r, (uint32_t)buffer->hardware_format), size;
    unsigned char *p;
    if (buffer->count <= 0 || !data) return 1;
    switch (buffer->type) {
    case _rasterizer_vertex_type_model_compressed: case _rasterizer_vertex_type_environment_compressed: size = 0x20; break;
    case _rasterizer_vertex_type_environment_lightmap_compressed: size = 8; break;
    default: return 1; /* not a map's: nothing converted */
    }
    if ((uint32_t)buffer->count > 0xffffffffUL / size) { *error = 1; return 0; }
    p = at(r, data, (uint32_t)buffer->count * size);
    if (!p) { *error = 1; return 0; }
    if (!done_test_add(d, data)) return 1;
    if (visitor) visitor(buffer->type, data, (uint32_t)buffer->count);
    if (buffer->type == _rasterizer_vertex_type_model_compressed) { mac_xbox_swap_model_vertices(p, (uint32_t)buffer->count); counts_model++; }
    else if (buffer->type == _rasterizer_vertex_type_environment_compressed) { mac_xbox_swap_environment_vertices(p, (uint32_t)buffer->count); counts_environment++; }
    else { mac_xbox_swap_lightmap_vertices(p, (uint32_t)buffer->count); counts_lightmap++; }
    return 1;
}

int mac_xbox_geometry_tags_to_native(unsigned char *data, uint32_t size,
    const struct mac_xbox_map_info *info, const char **error)
{
    struct region r;
    static struct done d;
    uint32_t instances, count, t;
    int bad = 0;
    (void)info;
    counts_model = counts_environment = counts_lightmap = counts_indices = 0;
    if (error) *error = NULL;
    if (!mac_xbox_geometry_swap_enabled()) return 1;
    r.base = data; r.virtual_base = MAC_XBOX_TAG_BASE; r.size = size;
    d.count = 0;
    instances = word_at(&r, MAC_XBOX_TAG_BASE); count = word_at(&r, MAC_XBOX_TAG_BASE + 0xc);
    if (count > 65535) bad = 1;
    for (t = 0; !bad && t < count; ++t) {
        uint32_t instance = instances + t * 32, address, g, p, geometry_count, geometry_address;
        struct model *model;
        if (!at(&r, instance, 32)) { bad = 1; break; }
        if (word_at(&r, instance) != MODELS_GROUP_TAG) continue;
        address = word_at(&r, instance + 0x14);
        model = (struct model *)at(&r, address, sizeof(struct model));
        if (!model) { bad = 1; break; }
        geometry_count = (uint32_t)model->geometries.count; geometry_address = (uint32_t)model->geometries.address;
        if (geometry_count > MAXIMUM_GEOMETRIES_PER_MODEL ||
            (geometry_count && !at(&r, geometry_address, geometry_count * sizeof(struct xbox_model_geometry)))) { bad = 1; break; }
        for (g = 0; !bad && g < geometry_count; ++g) {
            struct xbox_model_geometry *geometry = (struct xbox_model_geometry *)at(&r, geometry_address + g * sizeof(*geometry), sizeof(*geometry));
            uint32_t part_count = (uint32_t)geometry->parts.count, part_address = (uint32_t)geometry->parts.address;
            if (part_count > MAXIMUM_PARTS_PER_MODEL_GEOMETRY ||
                (part_count && !at(&r, part_address, part_count * sizeof(struct xbox_model_part)))) { bad = 1; break; }
            for (p = 0; p < part_count; ++p) {
                struct xbox_model_part *part = (struct xbox_model_part *)at(&r, part_address + p * sizeof(*part), sizeof(*part));
                uint32_t index_data = descriptor_data(&r, (uint32_t)part->triangle_buffer.hardware_format);
                if (!convert_vertices(&r, &d, &part->vertex_buffer, &bad)) break;
                if (part->triangle_buffer.count > 0 && part->triangle_buffer.count < 0x10000 && index_data) {
                    unsigned char *q = at(&r, index_data, ((uint32_t)part->triangle_buffer.count + 2) * 2);
                    if (!q) { bad = 1; break; }
                    if (done_test_add(&d, index_data)) { if (visitor) visitor(100, index_data, (uint32_t)part->triangle_buffer.count + 2); mac_xbox_swap_indices16(q, (uint32_t)part->triangle_buffer.count + 2); counts_indices++; }
                }
            }
        }
    }
    if (bad && error) *error = "a model's vertex or index buffer lies outside the tag data";
    return !bad;
}

int mac_xbox_geometry_bsp_to_native(unsigned char *data, uint32_t size,
    const struct mac_xbox_bsp *bsp, const char **error)
{
    struct region r;
    static struct done d;
    struct structure_bsp *root;
    uint32_t l, m;
    int bad = 0;
    if (error) *error = NULL;
    if (!mac_xbox_geometry_swap_enabled()) return 1;
    r.base = data; r.virtual_base = bsp->base; r.size = size;
    d.count = 0;
    root = (struct structure_bsp *)at(&r, word_at(&r, bsp->base), sizeof(struct structure_bsp));
    if (!root) { if (error) *error = "the bsp's root lies outside it"; return 0; }
    if ((uint32_t)root->lightmaps.count > 4096 ||
        ((uint32_t)root->lightmaps.count && !at(&r, (uint32_t)root->lightmaps.address, (uint32_t)root->lightmaps.count * sizeof(struct structure_lightmap)))) bad = 1;
    for (l = 0; !bad && l < (uint32_t)root->lightmaps.count; ++l) {
        struct structure_lightmap *lightmap = (struct structure_lightmap *)at(&r, (uint32_t)root->lightmaps.address + l * sizeof(*lightmap), sizeof(*lightmap));
        uint32_t material_count = (uint32_t)lightmap->materials.count, material_address = (uint32_t)lightmap->materials.address;
        if (material_count > 65535 || (material_count && !at(&r, material_address, material_count * sizeof(struct structure_material)))) { bad = 1; break; }
        for (m = 0; m < material_count; ++m) {
            struct structure_material *material = (struct structure_material *)at(&r, material_address + m * sizeof(*material), sizeof(*material));
            if (!convert_vertices(&r, &d, &material->vertices, &bad) ||
                !convert_vertices(&r, &d, &material->lightmap_vertices, &bad)) break;
        }
    }
    if (bad && error) *error = "a bsp's vertex buffer lies outside the bsp";
    return !bad;
}
