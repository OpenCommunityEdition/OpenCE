/* Decodes the uncompressed bitmaps an Xbox map lists (tools/mac_xbox_texture_list.py)
   with the shared Xbox texture decoder (the one the Mac's GPU path uses) and prints
   a hash of each decoded level. The texels in a map are little endian; the decoder
   reads them byte by byte, so the hashes must be the same on a big-endian host.
   usage: mac_xbox_texture_probe map list */

#include "native_bitmap_tags.h"
#include "engine_device_resources.h"
#include "native_resource_memory.h"
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include "engine_device_state.h"
#undef malloc
#undef calloc
#undef free
#undef memcpy
#undef memcmp
#undef memset
#undef strlen
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mac_xgpu_texture_decode_view.inc"
#include "xbox_map.h"

int main(int argc, char **argv)
{
    struct mac_mapped_file map;
    const char *error = NULL;
    FILE *list;
    unsigned d3d, w, h, offset, linear, lines = 0, failures = 0;
    if (argc < 3 || !mac_xbox_map_open(argv[1], &map, &error) || !(list = fopen(argv[2], "r"))) { printf("open failed\n"); return 2; }
    while (fscanf(list, "%u %u %u %u %u", &d3d, &w, &h, &offset, &linear) == 5) {
        struct xgpu_texture_description description;
        DWORD format_word, size_word = 0;
        unsigned long *decoded, hash = 2166136261UL, i;
        unsigned lw = 0, lh = 0;
        while ((1u << lw) < w) ++lw;
        while ((1u << lh) < h) ++lh;
        if (linear) {
            /* the pitch is 64-byte aligned in a map's linear rows */
            size_word = (((w * 4 + 63) / 64 - 1) << D3DSIZE_PITCH_SHIFT) | ((h - 1) << D3DSIZE_HEIGHT_SHIFT) | (w - 1);
            format_word = (d3d << D3DFORMAT_FORMAT_SHIFT) | (1 << D3DFORMAT_MIPMAP_SHIFT) | (2 << D3DFORMAT_DIMENSION_SHIFT);
        } else
            format_word = (lw << D3DFORMAT_USIZE_SHIFT) | (lh << D3DFORMAT_VSIZE_SHIFT) | (d3d << D3DFORMAT_FORMAT_SHIFT) |
                (1 << D3DFORMAT_MIPMAP_SHIFT) | (2 << D3DFORMAT_DIMENSION_SHIFT);
        mac_xgpu_texture_describe(format_word, size_word, &description);
        if (description.compressed || offset >= map.size || (size_t)w * h * 4 > map.size - offset) { ++failures; continue; }
        decoded = (unsigned long *)malloc((size_t)w * h * sizeof(*decoded));
        if (!decoded || !mac_decode_level(&description, 0, map.data + offset, NULL, decoded)) { ++failures; free(decoded); continue; }
        for (i = 0; i < (unsigned long)w * h; ++i) {
            /* ARGB as bytes, so that the hash does not depend on the word's byte order */
            hash = (hash ^ ((decoded[i] >> 24) & 255)) * 16777619UL; hash = (hash ^ ((decoded[i] >> 16) & 255)) * 16777619UL;
            hash = (hash ^ ((decoded[i] >> 8) & 255)) * 16777619UL; hash = (hash ^ (decoded[i] & 255)) * 16777619UL;
        }
        printf("%u %ux%u %u %08lx\n", d3d, w, h, linear, hash & 0xffffffffUL);
        free(decoded);
        ++lines;
    }
    printf("%u textures, %u skipped\n", lines, failures);
    mac_mapped_file_close(&map);
    return failures ? 1 : 0;
}
