#ifndef HALO_MAC_RETAIL_CACHE_H
#define HALO_MAC_RETAIL_CACHE_H
#include <stddef.h>
#include <stdint.h>
#define MAC_RETAIL_BASE UINT32_C(0x40440000)
#define MAC_RETAIL_CACHE_BYTES UINT32_C(0x01700000)

/* Bounded read-only version 7 view. Source bytes stay LE and must remain
   immutable/alive for the view's lifetime. This is metadata loading, not
   permission to cast map bytes to engine structures or run gameplay. */
struct mac_retail_bsp {
    uint32_t file_offset, size, base, root, handle;
};
struct mac_retail_cache {
    const unsigned char *file, *tags;
    uint32_t file_size, tag_offset, tag_size, tag_checksum, instances_offset, tags_end;
    uint32_t tag_count, scenario_handle, scenario_offset;
    uint32_t model_offset, model_size, model_indices_offset, bsp_count;
    struct mac_retail_bsp bsps[32];
    char name[32], build[32];
};
struct mac_retail_tag {
    uint32_t group, parents[2], handle, address;
    const char *name;
};
/* On failure, output is zeroed and error points to a static diagnostic. */
int mac_retail_open(const void *file, size_t size,
    struct mac_retail_cache *out, const char **error);
int mac_retail_tag_get(const struct mac_retail_cache *cache, uint32_t index,
    struct mac_retail_tag *tag);
const unsigned char *mac_retail_tag_span(const struct mac_retail_cache *cache,
    uint32_t address, uint32_t size);
struct mac_retail_resources {
    uint32_t bitmap_entries, sound_entries, external_bitmaps, external_sounds;
};
/* Checks nested pixel/sample ranges against actual file sizes. Missing
   resource files have size zero. Does not decode textures or audio. */
int mac_retail_resources_check(const struct mac_retail_cache *cache,
    uint32_t bitmap_file_size, uint32_t sound_file_size,
    struct mac_retail_resources *out, const char **error);
#endif
