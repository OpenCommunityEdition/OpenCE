#include "xbox_tag_walk.h"
#include "xbox_tag_tables.h"
#include "xbox_blob.h"
#include <stdlib.h>
#include <string.h>

#define MAX_DEPTH 24
#define MAX_ELEMENTS 4000000UL

static uint32_t le32_(const unsigned char *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint32_t be32_(const unsigned char *p) { return p[3] | (uint32_t)p[2] << 8 | (uint32_t)p[1] << 16 | (uint32_t)p[0] << 24; }
/* the words as the map has them (little endian), or as a conversion left them (big endian: the verifying pass) */
#define le32(p) (w->read_big ? be32_(p) : le32_(p))
#define rd16(p) (w->read_big ? (uint32_t)((p)[1] | (p)[0] << 8) : (uint32_t)((p)[0] | (p)[1] << 8))
static void swap32(unsigned char *p) { unsigned char a = p[0], b = p[1]; p[0] = p[3]; p[1] = p[2]; p[2] = b; p[3] = a; }
static void swap16(unsigned char *p) { unsigned char a = p[0]; p[0] = p[1]; p[1] = a; }
static void swap64(unsigned char *p) { unsigned char t; int i; for (i = 0; i < 4; ++i) { t = p[i]; p[i] = p[7 - i]; p[7 - i] = t; } }

int mac_xbox_walk_begin(struct mac_xbox_walk *w, unsigned char *base, uint32_t virtual_base, uint32_t size, int swap)
{
    {
        uint32_t *bytes = w->layout_bytes, *elements = w->layout_elements;
        int big = w->read_big;
        memset(w, 0, sizeof(*w));
        w->layout_bytes = bytes; w->layout_elements = elements; w->read_big = big;
    }
    w->base = base; w->virtual_base = virtual_base; w->size = size; w->swap = swap;
    w->covered = (unsigned char *)calloc((size_t)size / 8 + 1, 1);
    return w->covered != NULL;
}
void (*mac_xbox_walk_bytes_probe)(const char *layout, uint32_t op_offset, uint32_t count, const unsigned char *bytes);
void mac_xbox_walk_end(struct mac_xbox_walk *w) { free(w->covered); w->covered = NULL; }

static int fail(struct mac_xbox_walk *w, const char *why, uint32_t offset, const char *layout)
{
    if (!w->error) { w->error = why; w->error_offset = offset; w->error_layout = layout; }
    return 0;
}
/* [offset, offset + bytes) is in the region */
static int inside(const struct mac_xbox_walk *w, uint32_t offset, uint32_t bytes)
{ return offset <= w->size && bytes <= w->size - offset; }
/* mark bytes as walked; a byte walked before is an overlap */
static int mark(struct mac_xbox_walk *w, uint32_t offset, uint32_t bytes, const char *layout)
{
    uint32_t i;
    if (!inside(w, offset, bytes)) return fail(w, "a field lies outside the tags", offset, layout);
    for (i = offset; i < offset + bytes; ++i) {
        unsigned char bit = (unsigned char)(1u << (i & 7));
        if (w->covered[i >> 3] & bit) { w->error_other = w->owners ? w->owners[i] : NULL; return fail(w, "bytes walked twice", i, layout); }
        w->covered[i >> 3] |= bit;
        if (w->owners) w->owners[i] = layout;
    }
    w->bytes_covered += bytes;
    return 1;
}
int mac_xbox_walk_is_covered(const struct mac_xbox_walk *w, uint32_t offset)
{ return (w->covered[offset >> 3] >> (offset & 7)) & 1; }
uint32_t mac_xbox_walk_uncovered(const struct mac_xbox_walk *w) { return w->size - w->bytes_covered; }
/* an address in the region as an offset, with room for bytes */
static int to_offset(const struct mac_xbox_walk *w, uint32_t address, uint32_t bytes, uint32_t *offset)
{
    if (address < w->virtual_base) return 0;
    *offset = address - w->virtual_base;
    return inside(w, *offset, bytes);
}
static void convert(struct mac_xbox_walk *w, uint32_t offset, unsigned width, uint32_t count)
{
    uint32_t i;
    if (!w->swap) return;
    for (i = 0; i < count; ++i) {
        unsigned char *p = w->base + offset + i * width;
        if (width == 2) swap16(p); else if (width == 4) swap32(p); else swap64(p);
    }
}

static int walk_struct(struct mac_xbox_walk *w, uint32_t offset, int layout, unsigned depth);

struct animation_blobs {
    struct mac_xbox_animation animation;
    uint32_t info_offset, info_bytes, default_offset, default_bytes, data_offset, data_bytes;
    int have;
};

/* a data field: its five words (size, pad, file offset, address, definition) and its bytes */
static int walk_data(struct mac_xbox_walk *w, uint32_t offset, int kind, const unsigned char *element, uint32_t element_offset,
    struct animation_blobs *animation, const char *layout)
{
    const unsigned char *p = w->base + offset;
    uint32_t size = le32(p), address = le32(p + 12), blob;
    if (!mark(w, offset, 20, layout)) return 0;
    convert(w, offset, 4, 5);
    if (size == 0) return 1;
    if (size > w->size) return fail(w, "a data field is larger than the tags", offset, layout);
    if (address == 0 || !to_offset(w, address, size, &blob)) {
        /* its bytes are in the map file (a bitmap's pixels, a sound's samples) */
        ++w->file_data;
        return 1;
    }
    if (!mark(w, blob, size, layout)) return 0;
    if (kind == MAC_XBOX_DATA_BYTES || !w->swap) {
        if (animation && animation->have) {
            if (kind == MAC_XBOX_DATA_ANIM_INFO) { animation->info_offset = blob; animation->info_bytes = size; }
            else if (kind == MAC_XBOX_DATA_ANIM_DEFAULT) { animation->default_offset = blob; animation->default_bytes = size; }
            else if (kind == MAC_XBOX_DATA_ANIM_DATA) { animation->data_offset = blob; animation->data_bytes = size; }
        }
        return 1;
    }
    switch (kind) {
    case MAC_XBOX_DATA_HS_SYNTAX: {
        const char *why = NULL;
        if (!mac_xbox_swap_hs_syntax(w->base + blob, size, &why)) return fail(w, why ? why : "script data", blob, layout);
        break; }
    case MAC_XBOX_DATA_WORDS32: mac_xbox_swap_words32(w->base + blob, size); break;
    case MAC_XBOX_DATA_WORDS16: mac_xbox_swap_words16(w->base + blob, size); break;
    case MAC_XBOX_DATA_EVENTS: {
        const char *why = NULL;
        unsigned version = element ? element[mac_xbox_tag_hook_offsets.unit_control_data_version] : 0;
        (void)element_offset;
        if (!mac_xbox_swap_event_stream(w->base + blob, size, version, &why)) return fail(w, why ? why : "event stream", blob, layout);
        break; }
    default:
        if (animation && animation->have) {
            if (kind == MAC_XBOX_DATA_ANIM_INFO) { animation->info_offset = blob; animation->info_bytes = size; }
            else if (kind == MAC_XBOX_DATA_ANIM_DEFAULT) { animation->default_offset = blob; animation->default_bytes = size; }
            else if (kind == MAC_XBOX_DATA_ANIM_DATA) { animation->data_offset = blob; animation->data_bytes = size; }
        }
        break;
    }
    return 1;
}

static int walk_struct(struct mac_xbox_walk *w, uint32_t offset, int layout, unsigned depth)
{
    const struct mac_xbox_tag_layout *L = &mac_xbox_tag_layouts[layout];
    const struct mac_xbox_tag_op *op = &mac_xbox_tag_ops[L->first_op];
    unsigned char *element = w->base + offset;
    struct animation_blobs animation;
    uint32_t i, k;
    if (depth > MAX_DEPTH) return fail(w, "tags nested too deeply", offset, L->name);
    if (!inside(w, offset, L->size)) return fail(w, "a structure lies outside the tags", offset, L->name);
    if (++w->elements > MAX_ELEMENTS) return fail(w, "too many structures", offset, L->name);
    if (w->layout_elements) w->layout_elements[layout]++;
    if (w->layout_bytes) w->layout_bytes[layout] += L->size;
    memset(&animation, 0, sizeof(animation));
    if (layout == mac_xbox_tag_hook_offsets.animation_layout) {
        const struct mac_xbox_tag_hooks *h = &mac_xbox_tag_hook_offsets;
        animation.animation.frame_count = (int16_t)rd16(element + h->frame_count);
        animation.animation.frame_size = (int16_t)rd16(element + h->frame_size);
        animation.animation.frame_info_type = (int16_t)rd16(element + h->frame_info_type);
        animation.animation.node_count = (int16_t)rd16(element + h->node_count);
        animation.animation.flags = (uint16_t)rd16(element + h->flags);
        for (k = 0; k < 2; ++k) {
            animation.animation.translation_flags[k] = le32(element + h->translation_flags + 4 * k);
            animation.animation.rotation_flags[k] = le32(element + h->rotation_flags + 4 * k);
            animation.animation.scale_flags[k] = le32(element + h->scale_flags + 4 * k);
        }
        animation.have = 1;
    }
    for (i = 0; i < L->op_count; ++i, ++op) {
        uint32_t at = offset + op->offset;
        switch (op->kind) {
        case MAC_XBOX_OP_BYTES:
            if (!mark(w, at, op->count, L->name)) return 0;
            if (mac_xbox_walk_bytes_probe) mac_xbox_walk_bytes_probe(L->name, op->offset, op->count, w->base + at);
            break;
        case MAC_XBOX_OP_SWAP16:
            if (!mark(w, at, op->count * 2u, L->name)) return 0;
            convert(w, at, 2, op->count);
            break;
        case MAC_XBOX_OP_SWAP32:
            if (!mark(w, at, op->count * 4u, L->name)) return 0;
            convert(w, at, 4, op->count);
            break;
        case MAC_XBOX_OP_SWAP64:
            if (!mark(w, at, op->count * 8u, L->name)) return 0;
            convert(w, at, 8, op->count);
            break;
        case MAC_XBOX_OP_FLOAT32:
            if (!mark(w, at, op->count * 4u, L->name)) return 0;
            for (k = 0; k < op->count; ++k) {
                uint32_t bits = le32(w->base + at + 4 * k);
                ++w->floats;
                if (bits == 0xffffffffu) { ++w->none_floats; continue; }   /* all ones: a field the tag leaves NONE */
                if ((bits & 0x7f800000u) == 0x7f800000u) { if (!w->bad_float_layout) { w->bad_float_layout = L->name; w->bad_float_offset = at + 4 * k; w->bad_float_bits = bits; } ++w->bad_floats; }
                else if ((bits & 0x7f800000u) >= 0x6e800000u) ++w->huge_floats;   /* beyond about 1e10 */
            }
            convert(w, at, 4, op->count);
            break;
        case MAC_XBOX_OP_REF:
            if (!mark(w, at, 16, L->name)) return 0;
            convert(w, at, 4, 4);
            break;
        case MAC_XBOX_OP_DATA:
            if (!walk_data(w, at, op->arg, element, offset, &animation, L->name)) return 0;
            break;
        case MAC_XBOX_OP_BLOCK: {
            const unsigned char *p = w->base + at;
            uint32_t count = le32(p), address = le32(p + 4), first, total, size;
            if (!mark(w, at, 12, L->name)) return 0;
            convert(w, at, 4, 3);
            if (count == 0) break;
            if (count > 0x100000u) return fail(w, "a block has too many elements", at, L->name);
            if (op->arg < 0) { ++w->opaque_blocks; w->opaque_elements += count; break; }
            size = mac_xbox_tag_layouts[op->arg].size;
            total = count * size;
            if (size && total / size != count) return fail(w, "a block's size overflows", at, L->name);
            if (!to_offset(w, address, total, &first)) return fail(w, "a block lies outside the tags", at, L->name);
            for (k = 0; k < count; ++k)
                if (!walk_struct(w, first + k * size, op->arg, depth + 1)) return 0;
            break; }
        case MAC_XBOX_OP_ARRAY: {
            uint32_t size = mac_xbox_tag_layouts[op->arg].size;
            for (k = 0; k < op->count; ++k)
                if (!walk_struct(w, at + k * size, op->arg, depth + 1)) return 0;
            break; }
        default:
            return fail(w, "an unknown op", at, L->name);
        }
    }
    if (animation.have && w->swap) {
        const char *why = NULL;
        if (!mac_xbox_swap_animation(&animation.animation,
                animation.info_bytes ? w->base + animation.info_offset : NULL, animation.info_bytes,
                animation.default_bytes ? w->base + animation.default_offset : NULL, animation.default_bytes,
                animation.data_bytes ? w->base + animation.data_offset : NULL, animation.data_bytes, &why))
            return fail(w, why ? why : "animation data", offset, L->name);
    }
    return 1;
}

static int find_layout(uint32_t tag)
{
    unsigned i;
    for (i = 0; i < mac_xbox_tag_group_count; ++i) if (mac_xbox_tag_groups[i].tag == tag) return mac_xbox_tag_groups[i].layout;
    return -1;
}

/* a table of buffer descriptors (three words each) */
static int walk_buffers(struct mac_xbox_walk *w, uint32_t count, uint32_t address)
{
    uint32_t offset;
    if (!count) return 1;
    if (count > 0x10000 || !to_offset(w, address, count * 12, &offset)) return fail(w, "a buffer table lies outside the tags", address, "buffers");
    if (!mark(w, offset, count * 12, "buffers")) return 0;
    convert(w, offset, 4, count * 3);
    return 1;
}

int mac_xbox_walk_tags(struct mac_xbox_walk *w, const struct mac_xbox_map_info *info)
{
    uint32_t count, table, t, vcount, vaddr, icount, iaddr, offset;
    (void)info;
    if (w->size < 0x24) return fail(w, "no room for the tag header", 0, "header");
    count = le32(w->base + 0xc); table = le32(w->base);
    vcount = le32(w->base + 0x10); vaddr = le32(w->base + 0x14); icount = le32(w->base + 0x18); iaddr = le32(w->base + 0x1c);
    if (!mark(w, 0, 0x24, "header")) return 0;
    convert(w, 0, 4, 9);
    if (count == 0 || count > 65535) return fail(w, "tag count", 0xc, "header");
    if (!to_offset(w, table, count * 32, &offset) || !mark(w, offset, count * 32, "instances")) return w->error ? 0 : fail(w, "the tag table lies outside the tags", 0, "header");
    /* the names, buffer tables and tags are found from the instances: read them before they are converted */
    for (t = 0; t < count; ++t) {
        const unsigned char *p = w->base + offset + t * 32;
        uint32_t name = le32(p + 16), n;
        if (!to_offset(w, name, 1, &n)) return fail(w, "a tag's name lies outside the tags", offset + t * 32, "instances");
        {
            uint32_t end = n;
            while (end < w->size && w->base[end] != 0) ++end;
            if (end >= w->size || end - n > 255) return fail(w, "a tag's name is not terminated", n, "names");
            if (!mark(w, n, end - n + 1, "names")) return 0;
        }
    }
    if (!walk_buffers(w, vcount, vaddr) || !walk_buffers(w, icount, iaddr)) return 0;
    for (t = 0; t < count; ++t) {
        unsigned char *p = w->base + offset + t * 32;
        uint32_t group = le32(p), root = le32(p + 20), r;
        int layout = find_layout(group);
        convert(w, offset + t * 32, 4, 8);
        if (layout < 0 || root == 0) { if (layout < 0 && root) ++w->unread_tags; continue; }
        if (!to_offset(w, root, mac_xbox_tag_layouts[layout].size, &r)) return fail(w, "a tag lies outside the tags", offset + t * 32, "instances");
        if (!walk_struct(w, r, layout, 0)) return 0;
    }
    return 1;
}

int mac_xbox_walk_bsp(struct mac_xbox_walk *w, const struct mac_xbox_bsp *bsp)
{
    uint32_t root, vcount, vaddr, lcount, laddr, r;
    int layout = find_layout(0x73627370);
    (void)bsp;
    if (w->size < 0x18 || layout < 0) return fail(w, "no room for a bsp header", 0, "bsp");
    root = le32(w->base); vcount = le32(w->base + 4); vaddr = le32(w->base + 8); lcount = le32(w->base + 0xc); laddr = le32(w->base + 0x10);
    if (!mark(w, 0, 0x18, "bsp header")) return 0;
    convert(w, 0, 4, 6);
    if (!walk_buffers(w, vcount, vaddr) || !walk_buffers(w, lcount, laddr)) return 0;
    if (!to_offset(w, root, mac_xbox_tag_layouts[layout].size, &r)) return fail(w, "the bsp's structure lies outside it", 0, "bsp");
    return walk_struct(w, r, layout, 0);
}

int mac_xbox_walk_mark(struct mac_xbox_walk *w, uint32_t offset, uint32_t bytes) { return mark(w, offset, bytes, "geometry"); }
