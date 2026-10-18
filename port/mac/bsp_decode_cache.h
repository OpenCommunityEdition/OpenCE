#ifndef BSP_DECODE_CACHE_V2_H
#define BSP_DECODE_CACHE_V2_H

#include <stddef.h>
#include <stdint.h>

#define BSP_DECODE_ATTRIBUTE_COUNT 16U
#define BSP_DECODE_RECORD_FLOATS (BSP_DECODE_ATTRIBUTE_COUNT * 4U)
#define BSP_DECODE_MAX_DECLARATION_BYTES 256U
#define BSP_DECODE_MAX_STREAMS 16U
#define BSP_DECODE_MAX_VERTICES 65536U
#define BSP_DECODE_MAX_CACHE_BYTES (32U * 1024U * 1024U)
#define BSP_DECODE_HASH_BUCKETS 256U

struct bsp_decode_stream_key {
    unsigned char slot;
    unsigned long long owner_token;
    unsigned long long write_generation;
    unsigned long base_offset;
    unsigned long stride;
    unsigned long vertex_capacity;
};

struct bsp_decode_cache_entry;

struct bsp_decode_cache {
    struct bsp_decode_cache_entry *buckets[BSP_DECODE_HASH_BUCKETS];
    struct bsp_decode_cache_entry *newest;
    struct bsp_decode_cache_entry *oldest;
    size_t byte_cap;
    size_t bytes_used;
    size_t entry_count;
    unsigned long long use_serial;
};

struct bsp_decode_cache_view {
    struct bsp_decode_cache *cache;
    struct bsp_decode_cache_entry *entry;
};

enum bsp_decode_cache_result {
    BSP_DECODE_CACHE_INVALID = -1,
    BSP_DECODE_CACHE_BYPASS = 0,
    BSP_DECODE_CACHE_HIT = 1,
    BSP_DECODE_CACHE_MISS = 2
};

int bsp_decode_cache_init(struct bsp_decode_cache *cache, size_t byte_cap);
void bsp_decode_cache_dispose(struct bsp_decode_cache *cache);
size_t bsp_decode_cache_invalidate_owner(struct bsp_decode_cache *cache,
    unsigned long long owner_token);

/* Called once per draw with the complete ordered set of certified streams.
   Declaration bytes must be canonical, and mask must describe exactly their
   source attributes. Every used stream needs a nonzero stable owner/generation
   and its current base offset, stride and certified capacity. A stream change
   or any uncertified stream disables caching for that draw. The caller still
   validates every source allocation on every draw before using a hit.
   The returned view is ephemeral: do not use it after another begin_draw can
   evict its entry, invalidate_owner, or dispose the cache. Owner tokens must
   not be reused for another allocation lifetime; invalidate before teardown. */
int bsp_decode_cache_begin_draw(struct bsp_decode_cache *cache,
    const void *declaration, size_t declaration_bytes, unsigned long declared_mask,
    const struct bsp_decode_stream_key *streams, size_t stream_count,
    struct bsp_decode_cache_view *view);

/* Per-index operations are direct array/bitset accesses after begin_draw.
   Hits copy this draw's defaults, then overlay only attributes in the mask. */
int bsp_decode_cache_lookup_vertex(const struct bsp_decode_cache_view *view,
    unsigned long source_index, const float *draw_defaults, float *output_record);
int bsp_decode_cache_store_vertex(const struct bsp_decode_cache_view *view,
    unsigned long source_index, const float *decoded_record);

#endif
