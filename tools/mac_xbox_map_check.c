/* Opens Xbox (version 5) maps the way the Mac port does and checks their
   index: header, tag header, tags, scenario, bsps and buffer tables.
   usage: mac_xbox_map_check map...   (exit 1 if any map is refused) */
#include "xbox_map.h"
#include <stdio.h>
int main(int argc, char **argv)
{
    int a, bad = 0;
    for (a = 1; a < argc; ++a) {
        struct mac_mapped_file map;
        struct mac_xbox_map_info info;
        const char *error = NULL;
        unsigned b;
        if (!mac_xbox_map_path_version(argv[a])) { printf("%s: not an Xbox cache\n", argv[a]); ++bad; continue; }
        if (!mac_xbox_map_open(argv[a], &map, &error)) { printf("%s: open: %s\n", argv[a], error); ++bad; continue; }
        if (!mac_xbox_map_inspect(map.data, map.size, &info, &error)) {
            printf("%s: refused: %s\n", argv[a], error); ++bad; mac_mapped_file_close(&map); continue;
        }
        printf("%s: %s %s image=%lu tagdata=%08lx+%lu tags=%lu vb=%lu ib=%lu type=%lu bsps=%lu", argv[a],
            info.name, info.build, (unsigned long)map.size, (unsigned long)info.tag_data_offset,
            (unsigned long)info.tag_data_size, (unsigned long)info.tag_count,
            (unsigned long)info.vertex_buffers, (unsigned long)info.index_buffers,
            (unsigned long)info.scenario_type, (unsigned long)info.bsp_count);
        for (b = 0; b < info.bsp_count; ++b)
            printf(" [%lx+%lx vb%lu/%lu]", (unsigned long)info.bsps[b].file_offset, (unsigned long)info.bsps[b].size,
                (unsigned long)info.bsps[b].vertex_buffers, (unsigned long)info.bsps[b].index_buffers);
        printf("\n");
        mac_mapped_file_close(&map);
    }
    return bad ? 1 : 0;
}
