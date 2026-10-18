#ifndef HALO_MAC_ENGINE_CACHE_FILES_H
#define HALO_MAC_ENGINE_CACHE_FILES_H
#include "mapped_file.h"
#include "retail_cache.h"
#include "xbox_map.h"
#include <stdint.h>
#define MAC_CACHE_DECODED_BASE UINT32_C(0x30000000)
#define MAC_CACHE_BITMAP_BASE UINT32_C(0x40000000)
#define MAC_CACHE_SOUND_BASE UINT32_C(0x60000000)
#define MAC_CACHE_OFFSET_LIMIT UINT32_C(0x80000000)
#define MAC_CACHE_MAP_DIRECTORY_MAX 1024
struct mac_cache_files {
    struct mac_mapped_file map,bitmaps,sounds;
    struct mac_retail_cache retail;
    struct mac_retail_resources resources;
    /* port: an Xbox (version 5) map, inflated in map; retail then holds only
       what the precache and the menus read (file, tag_count, name, build) */
    int xbox;
    struct mac_xbox_map_info xbox_info;
    unsigned long completed_reads;
    char map_directory[MAC_CACHE_MAP_DIRECTORY_MAX];
};
/* Engine-thread scope. The map is opened and validated as retail version 7;
   present external resource files are range-checked against its bitmap/sound
   descriptors. Paths are read-only and optional resource files may be NULL.
   Files must remain unchanged while mapped. Output must be empty. */
int mac_cache_files_open(const char *map,const char *bitmaps,const char *sounds,
    struct mac_cache_files *,const char **error);
int mac_cache_files_bind(struct mac_cache_files *);
int mac_cache_files_unbind(struct mac_cache_files *);
void mac_cache_files_close(struct mac_cache_files *);
/* Exact bytes only. Checked failures leave destination untouched. Decoded
   sound space is unsupported. This API does not publish engine completion. */
int mac_cache_files_read(struct mac_cache_files *,uint32_t offset,uint32_t size,
    void *destination,size_t capacity,const char **error);
/* The active session's selected map parent, with a trailing slash. Returns an
   empty string when no native cache scope is bound. */
const char *cache_files_map_directory(void);
/* port: whether this run plays Xbox (version 5) maps: set once, from the
   map the run starts with, before the engine starts. The services that are
   made at start-up (the texture cache) are the engine's own, or the native
   v7 session's, for the whole run. */
void mac_xbox_mode_set(int on);
int mac_xbox_mode(void);
/* Whether the bound scope is an Xbox map: the engine's own cache loader
   (scenario_tags_load) then reads its tags in place, as on the console. */
int mac_cache_files_xbox_active(void);
/* The bound Xbox map's image (header at 0), or NULL. */
const unsigned char *mac_cache_files_xbox_image(size_t *size);

/* Engine cache_file_read completes synchronously, returning completed slot 0.
   Its callers currently treat any nonzero failure flag as loaded, so invalid
   engine requests abort with a diagnostic instead of publishing failed data.
   This is file I/O only: no native sound tags, codecs or audio output. */
#endif
