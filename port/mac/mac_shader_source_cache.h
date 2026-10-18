#ifndef HALO_MAC_SHADER_SOURCE_CACHE_H
#define HALO_MAC_SHADER_SOURCE_CACHE_H

#include <stddef.h>
#include "mac_nv2a_draw_api.h"
#include "mac_bone_gather.h"
#include "mac_shader_identity.h"

#define MAC_SHADER_SOURCE_CACHE_ENTRIES 64
#define MAC_SHADER_SOURCE_CACHE_BYTES (2UL * 1024UL * 1024UL)

struct mac_shader_source_entry;
struct mac_shader_source_cache
{
    struct mac_shader_source_entry *vertex[MAC_SHADER_SOURCE_CACHE_ENTRIES];
    struct mac_shader_source_entry *pixel[MAC_SHADER_SOURCE_CACHE_ENTRIES];
    unsigned long clock;
    size_t bytes;
    unsigned long vertex_hits, vertex_misses;
    unsigned long pixel_hits, pixel_misses;
    int bone_gather_enabled;
    /* Identity values are never reused; initialized caches must not be copied. */
    unsigned long identity_epoch;
    unsigned long next_entry_identity;
    /* the entries last found, tried before the others */
    struct mac_shader_source_entry *last_vertex, *last_pixel;
    /* (HALO_MAC_PERF_SRC_MEMO) vertex programs found by the address of the
       instructions that were asked about: the device keeps its shaders'
       instructions unchanged and at one address until it stops, so a draw
       that asks again needs neither the hash nor the comparison of them */
    int memo_enabled;
    unsigned long memo_next;
    struct mac_shader_source_memo
    {
        const DWORD *instructions;
        unsigned long count, packed_mask;
        int has_lighting, bone_gather_enabled;
        struct nv2a_vertex_lighting lighting;
        struct mac_shader_source_entry *entry;
    } memo[8];
};

enum mac_shader_source_failure
{
    MAC_SHADER_SOURCE_OK = 0,
    MAC_SHADER_SOURCE_VERTEX_GENERATE_FAILED,
    MAC_SHADER_SOURCE_PIXEL_GENERATE_FAILED,
    MAC_SHADER_SOURCE_VERTEX_COMPAT_FAILED,
    MAC_SHADER_SOURCE_PIXEL_COMPAT_FAILED
};

struct mac_shader_source_result
{
    const char *vertex_source;
    const char *pixel_source;
    unsigned long input_mask;
    struct mac_bone_gather_plan bone_gather;
    enum mac_shader_source_failure failure;
    char error[160];
    /* Present on vertex compatibility failure for the existing diagnostic. */
    char *modern_vertex_source;
    struct mac_shader_source_entry *vertex_pin;
    struct mac_shader_source_entry *pixel_pin;
    struct mac_shader_source_identity identity;
    char *owned_vertex_source;
    char *owned_pixel_source;
};

/* Source-cache initialization/access is serialized on the native render
   thread (the epoch allocator is process-wide); initialized caches are not
   copyable. Dispose every result before its cache. Cached source pointers are
   immutable and remain pinned until the matching result is disposed. */
void mac_shader_source_cache_init(struct mac_shader_source_cache *cache);
void mac_shader_source_cache_dispose(struct mac_shader_source_cache *cache);
void mac_shader_source_cache_set_bone_gather(struct mac_shader_source_cache *cache,
    int enabled);
int mac_shader_source_cache_get(struct mac_shader_source_cache *cache,
    const DWORD *instructions, unsigned long instruction_count,
    unsigned long packed_mask, const struct nv2a_vertex_lighting *lighting,
    const struct nv2a_pixel_shader_key *pixel_key,
    struct mac_shader_source_result *result);
void mac_shader_source_result_dispose(struct mac_shader_source_result *result);

#endif
