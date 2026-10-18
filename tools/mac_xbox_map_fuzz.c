/* Mutates the index of an Xbox map in memory (header, tag header, tag
   instances, scenario, bsp references and headers) and checks that the
   container reader refuses or accepts it without reading out of bounds
   (build with -fsanitize=address,undefined).
   usage: mac_xbox_map_fuzz map [iterations [seed]] */
#include "xbox_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long state = 1;
static unsigned long next(void)
{
    state = state * 1103515245UL + 12345UL;
    return (state >> 8) & 0xffffffUL;
}
int main(int argc, char **argv)
{
    struct mac_mapped_file map;
    struct mac_xbox_map_info info;
    const char *error = NULL;
    unsigned char *image;
    unsigned long iterations = argc > 2 ? strtoul(argv[2], NULL, 10) : 20000, i, accepted = 0, k;
    unsigned long regions[8][2], count = 0, size;
    if (argc < 2 || !mac_xbox_map_open(argv[1], &map, &error)) { fprintf(stderr, "open: %s\n", error ? error : "usage"); return 2; }
    if (argc > 3) state = strtoul(argv[3], NULL, 10);
    size = map.size;
    image = (unsigned char *)malloc(size);
    if (!image) return 2;
    memcpy(image, map.data, size);
    if (!mac_xbox_map_inspect(image, size, &info, &error)) { fprintf(stderr, "map refused: %s\n", error); return 2; }
    /* where the index is: header, tag header and instances, scenario's bsp references, each bsp header */
    regions[count][0] = 0; regions[count++][1] = 0x100;
    regions[count][0] = info.tag_data_offset; regions[count++][1] = 0x24;
    regions[count][0] = info.tag_data_offset + 0x24; regions[count++][1] = info.tag_count * 32UL;
    regions[count][0] = info.tag_data_offset + (info.scenario_address - MAC_XBOX_TAG_BASE); regions[count++][1] = 0x5b0;
    for (k = 0; k < info.bsp_count && count < 8; ++k) {
        regions[count][0] = info.bsps[k].file_offset; regions[count++][1] = 0x40;
    }
    for (i = 0; i < iterations; ++i) {
        unsigned long edits = 1 + next() % 4, e, saved_offset[4], r;
        unsigned char saved[4][4];
        for (e = 0; e < edits; ++e) {
            r = next() % count;
            saved_offset[e] = regions[r][0] + (next() % regions[r][1]);
            if (saved_offset[e] + 4 > size) saved_offset[e] = size - 4;
            memcpy(saved[e], image + saved_offset[e], 4);
            switch (next() % 4) {
            case 0: image[saved_offset[e]] = (unsigned char)next(); break;
            case 1: memset(image + saved_offset[e], 0xff, 4); break;
            case 2: memset(image + saved_offset[e], 0, 4); break;
            default: image[saved_offset[e] + 3] ^= (unsigned char)(1 << (next() % 8)); break;
            }
        }
        if (mac_xbox_map_inspect(image, size, &info, &error)) ++accepted;
        for (e = edits; e-- > 0;) memcpy(image + saved_offset[e], saved[e], 4);
    }
    printf("%lu mutations: %lu accepted, %lu refused, no fault\n", iterations, accepted, iterations - accepted);
    free(image);
    mac_mapped_file_close(&map);
    return 0;
}
