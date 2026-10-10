#ifndef MCC_CACHE_H
#define MCC_CACHE_H

struct cache_file_tag_header;
struct scenario_structure_bsp_reference;
struct bitmap_data;

/* Path ownership is decided before stripping a filename. MCC never searches
 * maps/, custom_maps/ or the optional Custom Edition installation. */
boolean mcc_level_name(char const *name);
boolean mcc_cache_available(char const *name);
boolean mcc_cache_present(char const *name, unsigned long checksum,
    char *message, long message_size);
boolean mcc_cache_require(char const *name, unsigned long checksum);
unsigned long mcc_cache_checksum(char const *name);
struct cache_file_tag_header *mcc_cache_tags_load(char const *name, void *header);
boolean mcc_cache_tags_loaded(void);
void mcc_cache_tags_unload(void);
boolean mcc_cache_contains(void const *address, long bytes);
boolean mcc_cache_read(long tag, long offset, long bytes, void *out);
boolean mcc_cache_bsp_load(struct scenario_structure_bsp_reference const *reference,
    void **header, void **structure);
void mcc_cache_bsp_unload(void);
short mcc_cache_part_palette(void const *buffer, byte const **nodes);
boolean mcc_cache_bitmap_valid(struct bitmap_data *bitmap);
void const *mcc_cache_bitmap_pixels(struct bitmap_data const *bitmap, unsigned long *bytes);

#endif
