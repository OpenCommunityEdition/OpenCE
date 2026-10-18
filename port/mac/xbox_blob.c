#include "xbox_blob.h"
#include "xbox_geometry.h"
#include <string.h>
#include <stdlib.h>

static void (*visitor)(const char *, uint32_t, uint32_t, unsigned, const void *);
void mac_xbox_blob_set_visitor(void (*proc)(const char *, uint32_t, uint32_t, unsigned, const void *)) { visitor = proc; }

static uint32_t le16(const unsigned char *p) { return p[0] | (uint32_t)p[1] << 8; }
static uint32_t le32(const unsigned char *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
/* convert [offset, offset+bytes) of base, units of 2 or 4 bytes */
static void convert(const char *what, unsigned char *base, uint32_t offset, uint32_t bytes, unsigned unit)
{
    uint32_t at;
    if (!bytes) return;
    if (visitor) visitor(what, offset, bytes, unit, base);
    if (!mac_xbox_geometry_swap_enabled()) return;
    for (at = offset; at + unit <= offset + bytes; at += unit) {
        unsigned char *q = base + at, t;
        if (unit == 4) { t = q[0]; q[0] = q[3]; q[3] = t; t = q[1]; q[1] = q[2]; q[2] = t; }
        else { t = q[0]; q[0] = q[1]; q[1] = t; }
    }
}
void mac_xbox_swap_words16(void *p, uint32_t bytes) { convert("words16", (unsigned char *)p, 0, bytes & ~1u, 2); }
void mac_xbox_swap_words32(void *p, uint32_t bytes) { convert("words32", (unsigned char *)p, 0, bytes & ~3u, 4); }

int mac_xbox_swap_hs_syntax(void *p, uint32_t bytes, const char **error)
{
    /* the script data array: a 0x38-byte header (shorts at 0x20, 0x22, 0x2c, 0x2e, 0x30 and 0x32, words at
       0x28 and 0x34) and 19001 nodes of 20 bytes (four shorts, then three words), as cache_endian.c's codec
       lays it out; checked as that codec checks it before anything is converted */
    static const unsigned shorts[] = { 0x20, 0x22, 0x2c, 0x2e, 0x30, 0x32 };
    unsigned char *s = (unsigned char *)p;
    uint32_t capacity, count, actual, first, i;
    if (error) *error = NULL;
    if (bytes < 0x38) goto bad;
    capacity = le16(s + 0x20); count = le16(s + 0x2e); actual = le16(s + 0x30); first = le16(s + 0x2c);
    if (capacity != 19001 || bytes != 0x38 + capacity * 0x14 || le16(s + 0x22) != 0x14 || !s[0x24] ||
        le32(s + 0x28) != UINT32_C(0x64407440) || count > capacity || actual > count || first > capacity) goto bad;
    for (i = 0; i < sizeof(shorts) / sizeof(shorts[0]); ++i) convert("hs_syntax", s, shorts[i], 2, 2);
    convert("hs_syntax", s, 0x28, 4, 4);
    convert("hs_syntax", s, 0x34, 4, 4);
    for (i = 0; i < capacity; ++i) {
        convert("hs_syntax", s, 0x38 + i * 0x14, 8, 2);
        convert("hs_syntax", s, 0x38 + i * 0x14 + 8, 12, 4);
    }
    return 1;
bad:
    if (error) *error = "the script data array does not have the layout of a map's";
    return 0;
}

/* ---------- animations */
struct section { uint32_t at, size, unit; };
/* per frame: for each node, the rotation (4 words), translation (3 floats) and scale (1 float) the node
   animates (animated: the flag set) or that the defaults hold (flag clear) */
static void nodes(const struct mac_xbox_animation *a, unsigned char *p, const char *what, int animated, uint32_t frames, uint32_t frame_size)
{
    static const unsigned sizes[3] = { 8, 12, 4 };
    const uint32_t *masks[3];
    uint32_t frame, at = 0, node, kind;
    masks[0] = a->rotation_flags; masks[1] = a->translation_flags; masks[2] = a->scale_flags;
    for (frame = 0; frame < frames; ++frame) {
        at = frame * frame_size;
        for (node = 0; node < (uint32_t)a->node_count; ++node)
            for (kind = 0; kind < 3; ++kind) {
                if ((int)((masks[kind][node / 32] >> (node % 32)) & 1) != animated) continue;
                convert(what, p, at, sizes[kind], kind == 0 ? 2 : 4);
                at += sizes[kind];
            }
    }
}
static int count_nodes(const uint32_t *mask, int node_count)
{
    int i, n = 0;
    for (i = 0; i < node_count; ++i) if ((mask[i / 32] >> (i % 32)) & 1) ++n;
    return n;
}
/* a compressed animation: one blob whose 0x2c-byte header holds offsets (words) to the node
   headers, the keyframe indices (16 bit), the default and keyframe rotations (16 bit) and
   translations and scales (floats): found from the header and the node flags, each section checked
   to lie inside the blob and not overlap another, converted once (the same rules as the v7 converter) */
static int compressed(const struct mac_xbox_animation *a, unsigned char *p, uint32_t size)
{
    struct section sections[16], tmp;
    uint32_t header[11], i, j, count = 0, keys[3] = { 0, 0, 0 };
    unsigned counts[3];
    const uint32_t *masks[3];
    static const unsigned key_sizes[3] = { 6, 12, 4 };
    if (size < 0x2c || size > 16u * 1024 * 1024) return 0;
    for (i = 0; i < 11; ++i) header[i] = le32(p + i * 4);
    masks[0] = a->rotation_flags; masks[1] = a->translation_flags; masks[2] = a->scale_flags;
    for (i = 0; i < 3; ++i) counts[i] = (unsigned)count_nodes(masks[i], a->node_count);
    {
        uint32_t starts[3];
        starts[0] = 0x2c; starts[1] = header[3]; starts[2] = header[7];
        for (i = 0; i < 3; ++i) {
            if (!counts[i]) continue;
            if ((starts[i] & 1) || starts[i] > size || counts[i] * 4 > size - starts[i]) return 0;
            for (j = 0; j < counts[i]; ++j) {
                uint32_t v = le32(p + starts[i] + j * 4), first = v >> 12, n = v & 4095;
                if (first + n > 32767) return 0;
                if (first + n > keys[i]) keys[i] = first + n;
            }
            sections[count].at = starts[i]; sections[count].size = counts[i] * 4; sections[count++].unit = 4;
        }
    }
    sections[count].at = 0; sections[count].size = 0x2c; sections[count++].unit = 4;
    for (i = 0; i < 3; ++i) {
        static const unsigned first_header[3] = { 0, 4, 8 };
        uint32_t index_at = header[first_header[i]], default_at = header[first_header[i] + 1], key_at = header[first_header[i] + 2];
        uint32_t default_size = i == 2 && !counts[2] ? 0 : (uint32_t)a->node_count * key_sizes[i];
        if (keys[i]) {
            sections[count].at = index_at; sections[count].size = keys[i] * 2; sections[count++].unit = 2;
            sections[count].at = key_at; sections[count].size = keys[i] * key_sizes[i]; sections[count++].unit = i == 0 ? 2 : 4;
        }
        if (default_size) { sections[count].at = default_at; sections[count].size = default_size; sections[count++].unit = i == 0 ? 2 : 4; }
        if (count > 13) return 0;
    }
    for (i = 0; i < count; ++i)
        if ((sections[i].at & 1) || sections[i].at > size || sections[i].size > size - sections[i].at) return 0;
    for (i = 1; i < count; ++i) for (j = i; j > 0 && sections[j].at < sections[j - 1].at; --j) { tmp = sections[j]; sections[j] = sections[j - 1]; sections[j - 1] = tmp; }
    for (i = 1; i < count; ++i) if (sections[i].at < sections[i - 1].at + sections[i - 1].size) return 0;
    for (i = 0; i < count; ++i) convert("animation compressed", p, sections[i].at, sections[i].size, sections[i].unit);
    return 1;
}
int mac_xbox_swap_animation(const struct mac_xbox_animation *a, void *frame_info, uint32_t info_bytes,
    void *defaults, uint32_t default_bytes, void *data, uint32_t data_bytes, const char **error)
{
    static const unsigned sizes[3] = { 8, 12, 4 };
    uint32_t frame_size = 0, node, kind;
    const uint32_t *masks[3];
    if (error) *error = NULL;
    if (a->node_count < 1 || a->node_count > 64 || a->frame_count < 1 || a->frame_info_type < 0 || a->frame_info_type > 3) goto bad;
    if (info_bytes != (a->frame_info_type ? 4u * ((uint32_t)a->frame_info_type + 1) * (uint32_t)a->frame_count : 0)) goto bad;
    masks[0] = a->rotation_flags; masks[1] = a->translation_flags; masks[2] = a->scale_flags;
    for (node = 0; node < (uint32_t)a->node_count; ++node) for (kind = 0; kind < 3; ++kind) if ((masks[kind][node / 32] >> (node % 32)) & 1) frame_size += sizes[kind];
    if (a->flags & 1) {
        if (data_bytes && !compressed(a, (unsigned char *)data, data_bytes)) goto bad;
    } else {
        if (a->frame_size < 0 || frame_size != (uint32_t)a->frame_size || data_bytes != frame_size * (uint32_t)a->frame_count ||
            default_bytes != (uint32_t)a->node_count * 24 - frame_size) goto bad;
        nodes(a, (unsigned char *)defaults, "animation default", 0, 1, 0);
        if (frame_size) nodes(a, (unsigned char *)data, "animation frame", 1, (uint32_t)a->frame_count, frame_size);
    }
    convert("animation frame info", (unsigned char *)frame_info, 0, info_bytes, 4);
    return 1;
bad:
    if (error) *error = "an animation's data does not fit its node flags and counts";
    return 0;
}

/* ---------- recorded animation event streams */
int mac_xbox_swap_event_stream(void *p, uint32_t bytes, unsigned version, const char **error)
{
    unsigned char *s = (unsigned char *)p;
    /* the sizes of the events' data after the header: 0 0 1 1 2 2, a vector2d (two floats), 8 of two chars, 8 of two shorts */
    static const unsigned char data_size[23] = { 0, 0, 1, 1, 2, 2, 8, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4 };
    uint32_t at, header_size, end, step;
    unsigned type;
    unsigned fields = version < 1 ? 1 : version > 4 ? 4 : version;
    if (error) *error = NULL;
    /* the unit control: bytes 1,1; shorts 2,2,2; vector2d; 3 vector3d; version 2: a long; 3, 4: a short each */
    at = 0;
    end = 1 + 1 + 2 + 2 + 2 + 8 + 12 + 12 + 12 + (fields >= 2 ? 4 : 0) + (fields >= 3 ? 2 : 0) + (fields >= 4 ? 2 : 0) + 12;
    if (bytes < end) goto bad;
    convert("event unit control", s, 2, 6, 2);
    convert("event unit control", s, 8, 8 + 36, 4);
    at = 52;
    if (fields >= 2) { convert("event unit control", s, at, 4, 4); at += 4; }
    if (fields >= 3) { convert("event unit control", s, at, 2, 2); at += 2; }
    if (fields >= 4) { convert("event unit control", s, at, 2, 2); at += 2; }
    convert("event animation state", s, at, 12, 2);
    at += 12;
    while (at < bytes) {
        unsigned header = s[at], time_delta = header & 3;
        type = header >> 2;
        header_size = time_delta == 2 ? 2 : time_delta == 3 ? 3 : 1;
        if (type >= 23 || bytes - at < header_size) goto bad;
        if (time_delta == 3) convert("event time", s, at + 1, 2, 2);
        /* the two bit fields, first in the low bits on the console's compiler, in the high bits on GCC for PowerPC */
        if (visitor) visitor("event header", at, 1, 0, s);
        if (mac_xbox_geometry_swap_enabled()) s[at] = (unsigned char)(((header & 3) << 6) | (header >> 2));
        step = header_size;
        if (bytes - at - step < data_size[type]) goto bad;
        if (type == 4 || type == 5) convert("event short", s, at + step, 2, 2);
        else if (type == 6) convert("event vector2d", s, at + step, 8, 4);
        else if (type >= 15) convert("event shorts", s, at + step, 4, 2);
        at += step + data_size[type];
    }
    return 1;
bad:
    if (error) *error = "a recorded animation's event stream does not fit its events";
    return 0;
}
