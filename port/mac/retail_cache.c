#include "retail_cache.h"
#include <string.h>

/* The PC-style index/BSP layouts are documented in cache_file_formats.c.
   These offsets have also been checked on the user's installed v7 Blood
   Gulch. Deliberately separate from CE's mutating, LE-output converter. */
static uint32_t u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 |
        (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static int fits(uint32_t offset, uint32_t bytes, uint32_t limit)
{ return offset <= limit && bytes <= limit - offset; }
static int disjoint(uint32_t a, uint32_t an, uint32_t b, uint32_t bn)
{ return a <= b ? an <= b-a : bn <= a-b; }
const unsigned char *mac_retail_tag_span(const struct mac_retail_cache *c,
    uint32_t address, uint32_t size)
{
    uint32_t offset;
    if (!c || !c->tags || address < MAC_RETAIL_BASE || !size) return NULL;
    offset = address - MAC_RETAIL_BASE;
    if (offset < c->tags_end || !fits(offset, size, c->tag_size)) return NULL;
    return c->tags + offset;
}
int mac_retail_tag_get(const struct mac_retail_cache *c, uint32_t index,
    struct mac_retail_tag *tag)
{
    const unsigned char *p;
    if (!c || !c->tags || !tag || index >= c->tag_count) return 0;
    p = c->tags + c->instances_offset + index*32;
    tag->group = u32(p); tag->parents[0] = u32(p+4); tag->parents[1] = u32(p+8);
    tag->handle = u32(p+12); tag->address = u32(p+20);
    tag->name = (const char *)(c->tags + (u32(p+16)-MAC_RETAIL_BASE));
    return 1;
}
int mac_retail_open(const void *file, size_t size,
    struct mac_retail_cache *out, const char **error)
{
    struct mac_retail_cache c;
    const unsigned char *p = (const unsigned char *)file, *t, *scenario, *refs;
    uint32_t address, i, j, n, handle, namesize;
    struct mac_retail_tag tag;
    if (error) *error = NULL;
    if (!out) { if (error) *error = "missing output"; return 0; }
    memset(out, 0, sizeof(*out)); memset(&c, 0, sizeof(c));
#define REJECT(message) do { if (error) *error = (message); return 0; } while (0)
    if (!p || size < 0x800 || size > UINT32_C(0x30000000)) REJECT("file size");
    if (u32(p) != UINT32_C(0x68656164) || u32(p+0x7fc) != UINT32_C(0x666f6f74)) REJECT("header signatures");
    if (u32(p+4) != 7) REJECT("requires retail version 7");
    c.file_size = u32(p+8);
    if (c.file_size < 0x800 || c.file_size > size || u32(p+12)) REJECT("declared/compressed length");
    if (!memchr(p+0x20, 0, 32) || !memchr(p+0x40, 0, 32)) REJECT("header string");
    c.tag_offset = u32(p+16); c.tag_size = u32(p+20);
    if (c.tag_offset < 0x800 || c.tag_size < 40 || c.tag_size > MAC_RETAIL_CACHE_BYTES ||
        !fits(c.tag_offset, c.tag_size, c.file_size)) REJECT("tag data range");
    c.file = p; c.tags = p+c.tag_offset; t = c.tags;
    if (u32(t+36) != UINT32_C(0x74616773)) REJECT("PC tag-index signature");
    /* TAG_INDEX_CHECKSUM_OFFSET (cache_file_formats.c) is the source
       tag-header checksum consumed by tag_groups_checksum(). */
    c.tag_checksum = u32(t+8);
    c.tag_count = u32(t+12); address = u32(t);
    if (!c.tag_count || c.tag_count > 65536 || address < MAC_RETAIL_BASE) REJECT("tag count/address");
    c.instances_offset = address-MAC_RETAIL_BASE;
    if (c.instances_offset < 40 || (c.instances_offset & 3) ||
        !fits(c.instances_offset, c.tag_count*32, c.tag_size)) REJECT("tag table range");
    c.tags_end = c.instances_offset+c.tag_count*32;
    c.model_offset = u32(t+20); c.model_indices_offset = u32(t+28); c.model_size = u32(t+32);
    if (c.model_offset < 0x800 || !fits(c.model_offset, c.model_size, c.file_size) ||
        c.model_indices_offset > c.model_size ||
        !disjoint(c.model_offset, c.model_size, c.tag_offset, c.tag_size)) REJECT("model data range");
    for (i = 0; i < c.tag_count; ++i) {
        t = c.tags+c.instances_offset+i*32;
        if ((u32(t+12)&65535) != i) REJECT("tag handle");
        address = u32(t+16);
        if (!mac_retail_tag_span(&c, address, 1)) REJECT("tag name address");
        namesize = c.tag_size-(address-MAC_RETAIL_BASE);
        if (namesize > 256) namesize = 256;
        if (!memchr(c.tags+(address-MAC_RETAIL_BASE), 0, namesize)) REJECT("tag name termination");
        /* Retail Blood Gulch keeps definitions local, including bitmap/sound
           definitions. Their external payloads are a separate resource task. */
        if (u32(t+24)) REJECT("indexed resource definitions not supported yet");
        address = u32(t+20);
        if (u32(t) == UINT32_C(0x73627370)) {
            if (address) REJECT("unexpected BSP tag address");
        } else if ((address & 3) || !mac_retail_tag_span(&c, address, 1)) REJECT("tag definition address");
    }
    c.scenario_handle = u32(c.tags+4); i = c.scenario_handle&65535;
    if (!mac_retail_tag_get(&c, i, &tag) || tag.handle != c.scenario_handle ||
        tag.group != UINT32_C(0x73636e72)) REJECT("scenario handle/group");
    scenario = mac_retail_tag_span(&c, tag.address, 0x5b0);
    if (!scenario) REJECT("scenario range");
    c.scenario_offset = tag.address-MAC_RETAIL_BASE;
    c.bsp_count = u32(scenario+0x5a4); address = u32(scenario+0x5a8);
    if (c.bsp_count > 32) REJECT("BSP count");
    refs = c.bsp_count ? mac_retail_tag_span(&c, address, c.bsp_count*32) : NULL;
    if (c.bsp_count && ((address & 3) || !refs)) REJECT("BSP references");
    for (i = 0; i < c.bsp_count; ++i) {
        struct mac_retail_bsp *b = &c.bsps[i];
        t = refs+i*32;
        b->file_offset = u32(t); b->size = u32(t+4); b->base = u32(t+8); b->handle = u32(t+28);
        handle = b->handle; n = handle&65535;
        if (!mac_retail_tag_get(&c, n, &tag) || tag.handle != handle ||
            tag.group != UINT32_C(0x73627370)) REJECT("BSP tag handle");
        if (b->file_offset < 0x800 || b->size < 24 || !fits(b->file_offset, b->size, c.file_size) ||
            !disjoint(b->file_offset, b->size, c.tag_offset, c.tag_size) ||
            !disjoint(b->file_offset, b->size, c.model_offset, c.model_size) ||
            b->base < MAC_RETAIL_BASE+c.tag_size || (b->base & 3) ||
            !fits(b->base-MAC_RETAIL_BASE, b->size, MAC_RETAIL_CACHE_BYTES)) REJECT("BSP file/virtual range");
        for (j = 0; j < i; ++j)
            if (!disjoint(b->file_offset, b->size, c.bsps[j].file_offset, c.bsps[j].size)) REJECT("overlapping BSP file ranges");
        t = p+b->file_offset; b->root = u32(t);
        if (u32(t+20) != UINT32_C(0x73627370) || u32(t+4) || u32(t+8) || u32(t+12) || u32(t+16) ||
            b->root < b->base+24 || (b->root & 3) ||
            !fits(b->root-b->base, 0x288, b->size)) REJECT("PC BSP header/root");
    }
    memcpy(c.name, p+0x20, 32); memcpy(c.build, p+0x40, 32);
    *out = c;
    return 1;
#undef REJECT
}

static int block(const struct mac_retail_cache *c, const unsigned char *field,
    uint32_t stride, uint32_t *count, const unsigned char **data, uint32_t *budget)
{
    uint32_t address = u32(field+4);
    *count = u32(field); *data = NULL;
    if (*count > *budget || *count > c->tag_size/stride) return 0;
    *budget -= *count;
    if (!*count) return 1;
    *data = mac_retail_tag_span(c, address, *count*stride);
    return !(address & 3) && *data != NULL;
}
int mac_retail_resources_check(const struct mac_retail_cache *c,
    uint32_t bitmap_file_size, uint32_t sound_file_size,
    struct mac_retail_resources *out, const char **error)
{
    struct mac_retail_resources r;
    struct mac_retail_tag tag;
    const unsigned char *root, *items, *perms, *entry;
    uint32_t i, j, k, count, perm_count, flags, limit, bytes, offset;
    uint32_t budget = 1000000; /* Bound work even when malicious blocks alias. */
    if (error) *error = NULL;
    if (!out) { if (error) *error = "missing resource output"; return 0; }
    memset(out, 0, sizeof(*out)); memset(&r, 0, sizeof(r));
#define REJECT(message) do { if (error) *error = (message); return 0; } while (0)
    if (!c || !c->tags) REJECT("missing validated retail view");
    for (i = 0; i < c->tag_count; ++i) {
        if (!mac_retail_tag_get(c, i, &tag)) REJECT("resource tag index");
        if (tag.group == UINT32_C(0x6269746d)) {
            root = mac_retail_tag_span(c, tag.address, 0x6c);
            if (!root || !block(c, root+0x60, 0x30, &count, &items, &budget)) REJECT("bitmap block");
            for (j = 0; j < count; ++j) {
                entry = items+j*0x30;
                flags = (uint32_t)entry[14] | (uint32_t)entry[15]<<8;
                limit = flags & 256 ? bitmap_file_size : c->file_size;
                if (flags & 256) { if (!limit) REJECT("missing bitmaps.map"); ++r.external_bitmaps; }
                if (!fits(u32(entry+24), u32(entry+28), limit)) REJECT("bitmap payload range");
                ++r.bitmap_entries;
            }
        } else if (tag.group == UINT32_C(0x736e6421)) {
            root = mac_retail_tag_span(c, tag.address, 0xa4);
            if (!root || !block(c, root+0x98, 0x48, &count, &items, &budget)) REJECT("sound pitch ranges");
            for (j = 0; j < count; ++j) {
                if (!block(c, items+j*0x48+0x3c, 0x7c, &perm_count, &perms, &budget)) REJECT("sound permutations");
                for (k = 0; k < perm_count; ++k) {
                    entry = perms+k*0x7c+0x40;
                    bytes = u32(entry); flags = u32(entry+4); offset = u32(entry+8);
                    limit = flags & 1 ? sound_file_size : c->file_size;
                    if (flags & 1) { if (!limit) REJECT("missing sounds.map"); ++r.external_sounds; }
                    if (bytes > UINT32_C(0x7fffffff) || !fits(offset, bytes, limit)) REJECT("sound payload range");
                    ++r.sound_entries;
                }
            }
        }
    }
    *out = r;
    return 1;
#undef REJECT
}
