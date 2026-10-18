#ifndef HALO_MAC_XBOX_MAP_H
#define HALO_MAC_XBOX_MAP_H
#include <stddef.h>
#include <stdint.h>
#include "mapped_file.h"

/* Xbox cache files (version 5), the maps of the user's own Halo disc image.
   On the disc a map is its 0x800-byte header followed by one zlib stream of
   the rest (the console inflates it as it copies the map to its disk); the
   header's file length is the inflated map's. Everything in a map is little
   endian. A map is opened whole and inflated in memory; every later check is
   against that image, never against the disc's file. Map files are untrusted
   input: nothing here follows a pointer it has not range-checked. */

#define MAC_XBOX_MAP_VERSION 5
#define MAC_XBOX_MAP_HEADER_BYTES 0x800
#define MAC_XBOX_MAP_MAX_BYTES UINT32_C(0x11600000)
/* where the tag cache is in the console's memory (physical_memory_map.c) and its size */
#define MAC_XBOX_TAG_BASE UINT32_C(0x803A6000)
#define MAC_XBOX_TAG_CACHE_BYTES UINT32_C(0x01600000)
#define MAC_XBOX_MAX_BSPS 32

struct mac_xbox_bsp {
    uint32_t file_offset, size, base, handle;
    uint32_t vertex_buffers, index_buffers; /* counts of the bsp's own buffer tables */
};
struct mac_xbox_map_info {
    uint32_t file_length, tag_data_offset, tag_data_size, tag_count, checksum;
    uint32_t scenario_handle, scenario_address, scenario_type;
    uint32_t vertex_buffers, index_buffers;
    uint32_t bsp_count;
    struct mac_xbox_bsp bsps[MAC_XBOX_MAX_BSPS];
    int compressed_on_disc;
    char name[32], build[32];
};

/* 5 if the file at path begins with an Xbox cache header, else 0. Reads 8 bytes. */
int mac_xbox_map_path_version(const char *path);
/* Opens the map (inflated if it is compressed) into out: a private read-only
   anonymous mapping of the whole map. Closing is mac_mapped_file_close. */
int mac_xbox_map_open(const char *path, struct mac_mapped_file *out, const char **error);
/* Checks the whole index of an opened map: header, tag header, every tag
   instance, the scenario, each bsp reference and its header and buffer
   tables, and each buffer's data, against the map and the tag cache. */
int mac_xbox_map_inspect(const unsigned char *map, size_t size,
    struct mac_xbox_map_info *out, const char **error);

/* port: called between the steps that inflate a map, if set (the loading screen's frames) */
extern void (*mac_xbox_map_pump)(void);
#endif
