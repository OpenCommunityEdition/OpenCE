/* Converts the raw data blobs of a real Xbox map (tools/mac_xbox_blob_list.py lists them)
   with port/mac/xbox_blob.c and checks every range it converted: read the way a big-endian
   host reads it, it equals the little-endian original; nothing else changed (an event header's
   bit fields are moved as documented). hs syntax data is checked by round trip through the codec.
   On an Intel Mac HALO_MAC_XBOX_FORCE_SWAP=1 reverses the words so the same reads apply.
   usage: mac_xbox_blob_probe map list */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbox_map.h"
#include "xbox_geometry.h"
#include "xbox_blob.h"
#include "cache_endian.h"

static unsigned char *work, *original, *touched;
static uint32_t blob_size;
static unsigned failures, ranges, bytes_checked;
static uint32_t le32(const unsigned char *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint32_t be32(const unsigned char *p) { return p[3] | (uint32_t)p[2] << 8 | (uint32_t)p[1] << 16 | (uint32_t)p[0] << 24; }
static uint32_t le16(const unsigned char *p) { return p[0] | (uint32_t)p[1] << 8; }
static uint32_t be16(const unsigned char *p) { return p[1] | (uint32_t)p[0] << 8; }
/* the ranges are checked after the conversion */
struct range { uint32_t offset, bytes; unsigned unit; };
static struct range *list; static unsigned list_count, list_capacity;
static void visit(const char *what, uint32_t offset, uint32_t bytes, unsigned unit, const void *base)
{
    (void)what;
    offset += (uint32_t)((const unsigned char *)base - work); /* (blobs laid end to end in one buffer) */
    if (list_count == list_capacity) { list_capacity = list_capacity ? list_capacity * 2 : 1024; list = (struct range *)realloc(list, list_capacity * sizeof(*list)); }
    list[list_count].offset = offset; list[list_count].bytes = bytes; list[list_count++].unit = unit;
}
static void check(const char *name)
{
    unsigned r;
    uint32_t i;
    memset(touched, 0, blob_size);
    for (r = 0; r < list_count; ++r) {
        uint32_t at = list[r].offset, end = at + list[r].bytes, unit = list[r].unit;
        if (end > blob_size) { ++failures; printf("%s: range outside\n", name); continue; }
        for (i = at; i < end; ++i) { if (touched[i] && unit) { ++failures; printf("%s: byte %lu converted twice\n", name, (unsigned long)i); break; } touched[i] = 1; }
        if (unit == 0) {
            unsigned char o = original[at], expected = (unsigned char)(((o & 3) << 6) | (o >> 2));
            if (work[at] != expected) { ++failures; if (failures < 10) printf("%s: event header at %lu\n", name, (unsigned long)at); }
            continue;
        }
        for (i = at; i + unit <= end; i += unit) {
            int same = unit == 4 ? be32(work + i) == le32(original + i) : be16(work + i) == le16(original + i);
            if (!same) { ++failures; if (failures < 10) printf("%s: word at %lu\n", name, (unsigned long)i); break; }
        }
        bytes_checked += list[r].bytes; ++ranges;
    }
    for (i = 0; i < blob_size; ++i) if (!touched[i] && work[i] != original[i]) { ++failures; if (failures < 10) printf("%s: byte %lu outside the ranges changed\n", name, (unsigned long)i); break; }
}
int main(int argc, char **argv)
{
    struct mac_mapped_file map;
    struct mac_xbox_map_info info;
    const char *error = NULL;
    FILE *list_file;
    char kind[32];
    unsigned long region, offset, size, a[24];
    unsigned entries = 0;
    if (argc < 3 || !mac_xbox_map_open(argv[1], &map, &error) || !mac_xbox_map_inspect(map.data, map.size, &info, &error) || !(list_file = fopen(argv[2], "r"))) { printf("open failed\n"); return 2; }
    mac_xbox_blob_set_visitor(visit);
    while (fscanf(list_file, "%31s %lu", kind, &region) == 2) {
        uint32_t base_offset;
        int fields = !strcmp(kind, "animation") ? 17 : !strcmp(kind, "events") ? 3 : 2;
        unsigned f;
        const char *name = kind;
        memset(a, 0, sizeof(a));
        for (f = 0; f < (unsigned)fields; ++f) if (fscanf(list_file, "%lu", &a[f]) != 1) { printf("bad list\n"); return 2; }
        base_offset = region ? info.bsps[region - 1].file_offset : info.tag_data_offset;
        list_count = 0;
        if (!strcmp(kind, "animation")) {
            /* fc fs fit nc flags tr0 tr1 ro0 ro1 sc0 sc1 isz ioff dsz doff asz aoff */
            struct mac_xbox_animation an;
            unsigned long isz = a[11], ioff = a[12], dsz = a[13], doff = a[14], asz = a[15], aoff = a[16];
            unsigned char *info_p, *def_p, *data_p;
            size_t total = isz + dsz + asz, cursor = 0;
            unsigned char *copy = (unsigned char *)malloc(total ? total : 1), *orig = (unsigned char *)malloc(total ? total : 1);
            an.frame_count = (int16_t)a[0]; an.frame_size = (int16_t)a[1]; an.frame_info_type = (int16_t)a[2]; an.node_count = (int16_t)a[3]; an.flags = (uint16_t)a[4];
            an.translation_flags[0] = (uint32_t)a[5]; an.translation_flags[1] = (uint32_t)a[6]; an.rotation_flags[0] = (uint32_t)a[7]; an.rotation_flags[1] = (uint32_t)a[8];
            an.scale_flags[0] = (uint32_t)a[9]; an.scale_flags[1] = (uint32_t)a[10];
            /* the three blobs laid end to end in one work buffer */
            memcpy(copy, map.data + base_offset + ioff, isz); info_p = copy; cursor = isz;
            memcpy(copy + cursor, map.data + base_offset + doff, dsz); def_p = copy + cursor; cursor += dsz;
            memcpy(copy + cursor, map.data + base_offset + aoff, asz); data_p = copy + cursor;
            memcpy(orig, copy, total);
            work = copy; original = orig; blob_size = (uint32_t)total; touched = (unsigned char *)calloc(total ? total : 1, 1);
            if (!mac_xbox_swap_animation(&an, isz ? info_p : NULL, (uint32_t)isz, dsz ? def_p : NULL, (uint32_t)dsz, asz ? data_p : NULL, (uint32_t)asz, &error)) { ++failures; if (failures < 10) printf("animation refused: %s\n", error); }
            else {
                check(name);
            }
            free(copy); free(orig); free(touched);
        } else {
            unsigned char *copy, *orig;
            if (!strcmp(kind, "events")) { offset = a[0]; size = a[1]; }
            else { offset = a[0]; size = a[1]; }
            copy = (unsigned char *)malloc(size); orig = (unsigned char *)malloc(size);
            memcpy(copy, map.data + base_offset + offset, size); memcpy(orig, copy, size);
            work = copy; original = orig; blob_size = (uint32_t)size; touched = (unsigned char *)calloc(size, 1);
            if (!strcmp(kind, "words16")) { mac_xbox_swap_words16(copy, (uint32_t)size); check(name); }
            else if (!strcmp(kind, "words32")) { mac_xbox_swap_words32(copy, (uint32_t)size); check(name); }
            else if (!strcmp(kind, "events")) {
                if (!mac_xbox_swap_event_stream(copy, (uint32_t)size, (unsigned)a[2], &error)) { ++failures; if (failures < 10) printf("events refused: %s\n", error); } else check(name);
            } else if (!strcmp(kind, "hs_syntax")) {
                if (!mac_xbox_swap_hs_syntax(copy, (uint32_t)size, &error)) { ++failures; printf("hs syntax refused: %s\n", error); }
                else check(name);

            }
            free(copy); free(orig); free(touched);
        }
        ++entries;
    }
    printf("%s: %u blobs, %u ranges, %u bytes checked, failures %u\n", argv[1], entries, ranges, bytes_checked, failures);
    mac_mapped_file_close(&map);
    return failures ? 1 : 0;
}
