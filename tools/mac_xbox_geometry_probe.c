/* Converts the raw geometry of a real Xbox map (vertex and index data, little
   endian) with port/mac/xbox_geometry.c and checks every buffer: each field read
   the way a big-endian host reads it equals the little-endian original, and no
   other byte of the tag data or the bsps changed. On a big-endian host the
   conversion is the host's own; on an Intel Mac set HALO_MAC_XBOX_FORCE_SWAP=1,
   which reverses the words so the same big-endian reads apply.
   usage: mac_xbox_geometry_probe map... (tools/mac_xbox_geometry_checks.sh builds it) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbox_map.h"
#include "xbox_geometry.h"

static unsigned char *region, *original;
static uint32_t region_base, region_size, buffers, failures, words, covered_words;
static unsigned char *touched;
static uint32_t le32(const unsigned char *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint32_t be32(const unsigned char *p) { return p[3] | (uint32_t)p[2] << 8 | (uint32_t)p[1] << 16 | (uint32_t)p[0] << 24; }
static uint32_t le16(const unsigned char *p) { return p[0] | (uint32_t)p[1] << 8; }
static uint32_t be16(const unsigned char *p) { return p[1] | (uint32_t)p[0] << 8; }
static FILE *list_out; static uint32_t current_region; static int current_map;
static unsigned char *work;
static void visit(int kind, uint32_t address, uint32_t count)
{
    uint32_t offset = address - region_base, i, k, bytes;
    /* the data is converted right after this call: remember what to compare */
    bytes = kind == 100 ? count * 2 : kind == 3 ? count * 8 : count * 32;
    ++buffers;
    if (list_out) fprintf(list_out, "%d %lu %d %lu %lu\n", current_map, (unsigned long)current_region, kind, (unsigned long)address, (unsigned long)count);
    for (i = 0; i < bytes; ++i) touched[offset + i] = (unsigned char)(kind == 100 ? 2 : kind == 3 ? 3 : kind == 5 ? 5 : 1);
    (void)k;
}
static int host_is_big(void) { union { uint32_t v; unsigned char b[4]; } u; u.v = 1; return u.b[0] == 0; }
static void verify(void)
{
    uint32_t offset, i, k;
    int force = mac_xbox_geometry_swap_enabled();
    (void)host_is_big;
    for (offset = 0; offset < region_size;) {
        unsigned char kind = touched[offset];
        if (!kind) {
            if (region[offset] != original[offset]) { if (failures < 10) printf("byte outside a buffer changed at %08lx\n", (unsigned long)offset); ++failures; }
            ++offset; continue;
        }
        if (kind == 2) {
            for (i = 0; i < 1 && touched[offset + 2 * i] == 2; ++i) { }
            if (be16(region + offset) != le16(original + offset) && force) { ++failures; if (failures < 10) printf("index at %08lx\n", (unsigned long)offset); }
            ++words; offset += 2; continue;
        }
        if (kind == 3) {
            if (be32(region + offset) != le32(original + offset) || be16(region + offset + 4) != le16(original + offset + 4) ||
                be16(region + offset + 6) != le16(original + offset + 6)) { ++failures; if (failures < 10) printf("lightmap vertex at %08lx\n", (unsigned long)offset); }
            ++words; offset += 8; continue;
        }
        if (kind == 5) {
            for (k = 0; k < 6; ++k) if (be32(region + offset + 4 * k) != le32(original + offset + 4 * k)) { ++failures; if (failures < 10) printf("model vertex word %lu at %08lx\n", (unsigned long)k, (unsigned long)offset); break; }
            if (be16(region + offset + 0x18) != le16(original + offset + 0x18) || be16(region + offset + 0x1a) != le16(original + offset + 0x1a) ||
                be16(region + offset + 0x1e) != le16(original + offset + 0x1e) || region[offset + 0x1c] != original[offset + 0x1c] || region[offset + 0x1d] != original[offset + 0x1d]) { ++failures; if (failures < 10) printf("model vertex tail at %08lx\n", (unsigned long)offset); }
            ++words; offset += 0x20; continue;
        }
        for (k = 0; k < 8; ++k) if (be32(region + offset + 4 * k) != le32(original + offset + 4 * k)) { ++failures; if (failures < 10) printf("environment vertex word %lu at %08lx\n", (unsigned long)k, (unsigned long)offset); break; }
        ++words; offset += 0x20;
    }
}
/* a list of buffers (region, kind, address, count) from an Intel run of the walker:
   converted here directly, for a host whose tag data cannot be walked yet (the
   tags are not swapped for it) */
static void apply(unsigned char *p, int kind, uint32_t count)
{
    if (kind == 100) mac_xbox_swap_indices16(p, count);
    else if (kind == 3) mac_xbox_swap_lightmap_vertices(p, count);
    else if (kind == 5) mac_xbox_swap_model_vertices(p, count);
    else mac_xbox_swap_environment_vertices(p, count);
}
int main(int argc, char **argv)
{
    int a;
    const char *list_path = getenv("MAC_XBOX_GEOMETRY_LIST"), *mode = getenv("MAC_XBOX_GEOMETRY_MODE");
    FILE *list_in = NULL;
    int from_list = mode && !strcmp(mode, "apply");
    if (list_path && from_list) list_in = fopen(list_path, "r");
    else if (list_path) list_out = fopen(list_path, "w");
    if (from_list && !list_in) { printf("no buffer list\n"); return 2; }
    mac_xbox_geometry_set_visitor(visit);
    for (a = 1; a < argc; ++a) {
        struct mac_mapped_file map;
        current_map = a;
        struct mac_xbox_map_info info;
        const char *error = NULL;
        uint32_t b;
        if (!mac_xbox_map_open(argv[a], &map, &error) || !mac_xbox_map_inspect(map.data, map.size, &info, &error)) { printf("%s: %s\n", argv[a], error); ++failures; continue; }
        buffers = words = 0;
        /* the tag data, then each bsp: each its own region */
        for (b = 0; b <= info.bsp_count; ++b) {
            uint32_t offset = b ? info.bsps[b - 1].file_offset : info.tag_data_offset, size = b ? info.bsps[b - 1].size : info.tag_data_size;
            region_base = b ? info.bsps[b - 1].base : MAC_XBOX_TAG_BASE; region_size = size; current_region = b;
            region = (unsigned char *)malloc(size); original = (unsigned char *)malloc(size); touched = (unsigned char *)calloc(size, 1);
            memcpy(region, map.data + offset, size); memcpy(original, map.data + offset, size);
            if (from_list) {
                unsigned long m, r, kind, address, count;
                long mark = ftell(list_in);
                while (fscanf(list_in, "%lu %lu %lu %lu %lu", &m, &r, &kind, &address, &count) == 5) {
                    uint32_t bytes = kind == 100 ? count * 2 : kind == 3 ? count * 8 : count * 32;
                    if ((int)m != a || r != b) { fseek(list_in, mark, SEEK_SET); break; }
                    mark = ftell(list_in);
                    if (address < region_base || address - region_base > region_size || bytes > region_size - (address - region_base)) { ++failures; continue; }
                    visit((int)kind, (uint32_t)address, (uint32_t)count);
                    apply(region + (address - region_base), (int)kind, (uint32_t)count);
                }
                verify();
            } else if (!(b ? mac_xbox_geometry_bsp_to_native(region, size, &info.bsps[b - 1], &error) : mac_xbox_geometry_tags_to_native(region, size, &info, &error))) {
                printf("%s: region %lu refused: %s\n", argv[a], (unsigned long)b, error); ++failures;
            } else verify();
            free(region); free(original); free(touched);
        }
        printf("%s: %lu buffers, %lu elements converted (swap %s), failures so far %lu\n", argv[a], (unsigned long)buffers, (unsigned long)words,
            mac_xbox_geometry_swap_enabled() ? "on" : "off", (unsigned long)failures);
        mac_mapped_file_close(&map);
    }
    if (list_out) fclose(list_out);
    return failures ? 1 : 0;
}
