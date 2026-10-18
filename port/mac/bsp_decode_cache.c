#include "bsp_decode_cache.h"

#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct bsp_decode_cache_entry {
    struct bsp_decode_cache_entry *hash_next;
    struct bsp_decode_cache_entry *newer;
    struct bsp_decode_cache_entry *older;
    unsigned long hash;
    unsigned long mask;
    unsigned long capacity;
    unsigned packed_attributes;
    size_t allocation_bytes;
    size_t declaration_bytes;
    size_t valid_offset;
    size_t values_offset;
    size_t stream_count;
    struct bsp_decode_stream_key streams[BSP_DECODE_MAX_STREAMS];
    unsigned char declaration[BSP_DECODE_MAX_DECLARATION_BYTES];
    unsigned char data[1];
};

static size_t data_offset(void)
{
    return offsetof(struct bsp_decode_cache_entry, data);
}

static size_t align_float(size_t size)
{
    size_t remainder = size % sizeof(float);
    return remainder ? size + sizeof(float) - remainder : size;
}

static unsigned popcount_mask(unsigned long mask)
{
    unsigned count = 0;
    while (mask) {
        count += (unsigned)(mask & 1UL);
        mask >>= 1;
    }
    return count;
}

static int finite_float(float value)
{
    return value == value && value <= FLT_MAX && value >= -FLT_MAX;
}

static void detach_lru(struct bsp_decode_cache *cache,
    struct bsp_decode_cache_entry *entry)
{
    if (entry->newer) entry->newer->older = entry->older;
    else cache->newest = entry->older;
    if (entry->older) entry->older->newer = entry->newer;
    else cache->oldest = entry->newer;
    entry->newer = NULL;
    entry->older = NULL;
}

static void touch_entry(struct bsp_decode_cache *cache,
    struct bsp_decode_cache_entry *entry)
{
    if (cache->newest == entry) return;
    if (entry->newer || entry->older || cache->oldest == entry)
        detach_lru(cache, entry);
    entry->older = cache->newest;
    entry->newer = NULL;
    if (cache->newest) cache->newest->newer = entry;
    else cache->oldest = entry;
    cache->newest = entry;
}

static void remove_entry(struct bsp_decode_cache *cache,
    struct bsp_decode_cache_entry *entry)
{
    unsigned bucket = entry->hash % BSP_DECODE_HASH_BUCKETS;
    struct bsp_decode_cache_entry **at = &cache->buckets[bucket];
    while (*at && *at != entry) at = &(*at)->hash_next;
    if (*at) *at = entry->hash_next;
    detach_lru(cache, entry);
    cache->bytes_used -= entry->allocation_bytes;
    --cache->entry_count;
    free(entry);
}

static void clear_entries(struct bsp_decode_cache *cache)
{
    struct bsp_decode_cache_entry *entry = cache->oldest;
    while (entry) {
        struct bsp_decode_cache_entry *next = entry->newer;
        free(entry);
        entry = next;
    }
    memset(cache->buckets, 0, sizeof(cache->buckets));
    cache->newest = cache->oldest = NULL;
    cache->bytes_used = cache->entry_count = 0;
}

static int advance_serial(struct bsp_decode_cache *cache)
{
    if (cache->use_serial == ULLONG_MAX) {
        clear_entries(cache);
        cache->use_serial = 0;
        return 0;
    }
    ++cache->use_serial;
    return 1;
}

static unsigned long hash_bytes(unsigned long hash,
    const unsigned char *bytes, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        hash ^= bytes[i];
        hash *= 16777619UL;
    }
    return hash;
}

static unsigned long hash_u64(unsigned long hash, unsigned long long value)
{
    unsigned i;
    for (i = 0; i < 8; ++i) {
        unsigned char byte = (unsigned char)(value & 0xffU);
        hash = hash_bytes(hash, &byte, 1);
        value >>= 8;
    }
    return hash;
}

static unsigned long hash_ulong(unsigned long hash, unsigned long value)
{
    unsigned i;
    for (i = 0; i < sizeof(value); ++i) {
        unsigned char byte = (unsigned char)(value & 0xffUL);
        hash = hash_bytes(hash, &byte, 1);
        value >>= 8;
    }
    return hash;
}

static unsigned long hash_key(const void *declaration, size_t declaration_bytes,
    unsigned long mask, const struct bsp_decode_stream_key *streams,
    size_t stream_count)
{
    unsigned long hash = 2166136261UL;
    size_t i;
    hash = hash_ulong(hash, mask);
    hash = hash_ulong(hash, (unsigned long)stream_count);
    hash = hash_bytes(hash, (const unsigned char *)declaration, declaration_bytes);
    for (i = 0; i < stream_count; ++i) {
        hash = hash_bytes(hash, &streams[i].slot, 1);
        hash = hash_u64(hash, streams[i].owner_token);
        hash = hash_u64(hash, streams[i].write_generation);
        hash = hash_ulong(hash, streams[i].base_offset);
        hash = hash_ulong(hash, streams[i].stride);
        hash = hash_ulong(hash, streams[i].vertex_capacity);
    }
    return hash;
}

static int validate_streams(const void *declaration, size_t declaration_bytes,
    unsigned long mask, const struct bsp_decode_stream_key *streams,
    size_t stream_count, unsigned long *capacity)
{
    unsigned long result = BSP_DECODE_MAX_VERTICES;
    size_t i;
    unsigned long seen_slots = 0;
    if (!declaration || !declaration_bytes ||
        declaration_bytes > BSP_DECODE_MAX_DECLARATION_BYTES ||
        !mask || (mask & ~0xffffUL) || !streams || !stream_count ||
        stream_count > BSP_DECODE_MAX_STREAMS) return 0;
    for (i = 0; i < stream_count; ++i) {
        const struct bsp_decode_stream_key *stream = &streams[i];
        unsigned long bytes;
        if (stream->slot >= BSP_DECODE_MAX_STREAMS ||
            (seen_slots & (1UL << stream->slot)) ||
            !stream->owner_token || !stream->write_generation ||
            !stream->stride || stream->stride > UINT32_MAX ||
            !stream->vertex_capacity || stream->vertex_capacity > BSP_DECODE_MAX_VERTICES ||
            stream->vertex_capacity > UINT32_MAX / stream->stride)
            return 0;
        bytes = stream->vertex_capacity * stream->stride;
        if (stream->base_offset > UINT32_MAX - bytes) return 0;
        seen_slots |= 1UL << stream->slot;
        if (stream->vertex_capacity < result) result = stream->vertex_capacity;
    }
    if (!result) return 0;
    *capacity = result;
    return 1;
}

static int streams_equal(const struct bsp_decode_cache_entry *entry,
    const struct bsp_decode_stream_key *streams, size_t stream_count)
{
    size_t i;
    if (entry->stream_count != stream_count) return 0;
    for (i = 0; i < stream_count; ++i) {
        const struct bsp_decode_stream_key *a = &entry->streams[i];
        const struct bsp_decode_stream_key *b = &streams[i];
        if (a->slot != b->slot || a->owner_token != b->owner_token ||
            a->write_generation != b->write_generation ||
            a->base_offset != b->base_offset || a->stride != b->stride ||
            a->vertex_capacity != b->vertex_capacity) return 0;
    }
    return 1;
}

static struct bsp_decode_cache_entry *find_entry(struct bsp_decode_cache *cache,
    const void *declaration, size_t declaration_bytes, unsigned long mask,
    const struct bsp_decode_stream_key *streams, size_t stream_count,
    unsigned long hash)
{
    struct bsp_decode_cache_entry *entry;
    for (entry = cache->buckets[hash % BSP_DECODE_HASH_BUCKETS]; entry;
         entry = entry->hash_next) {
        if (entry->hash == hash && entry->mask == mask &&
            entry->declaration_bytes == declaration_bytes &&
            memcmp(entry->declaration, declaration, declaration_bytes) == 0 &&
            streams_equal(entry, streams, stream_count)) return entry;
    }
    return NULL;
}

static int entry_stale_for(const struct bsp_decode_cache_entry *entry,
    const struct bsp_decode_stream_key *streams, size_t stream_count)
{
    size_t i, j;
    for (i = 0; i < entry->stream_count; ++i) {
        for (j = 0; j < stream_count; ++j) {
            const struct bsp_decode_stream_key *old = &entry->streams[i];
            const struct bsp_decode_stream_key *now = &streams[j];
            if (old->slot == now->slot && old->owner_token == now->owner_token &&
                old->base_offset == now->base_offset && old->write_generation != now->write_generation)
                return 1;
        }
    }
    return 0;
}

int bsp_decode_cache_init(struct bsp_decode_cache *cache, size_t byte_cap)
{
    if (!cache || !byte_cap || byte_cap > BSP_DECODE_MAX_CACHE_BYTES) return 0;
    memset(cache, 0, sizeof(*cache));
    cache->byte_cap = byte_cap;
    return 1;
}

void bsp_decode_cache_dispose(struct bsp_decode_cache *cache)
{
    if (!cache) return;
    clear_entries(cache);
    memset(cache, 0, sizeof(*cache));
}

size_t bsp_decode_cache_invalidate_owner(struct bsp_decode_cache *cache,
    unsigned long long owner_token)
{
    struct bsp_decode_cache_entry *entry;
    size_t removed = 0;
    if (!cache || !owner_token) return 0;
    entry = cache->oldest;
    while (entry) {
        struct bsp_decode_cache_entry *next = entry->newer;
        size_t i;
        for (i = 0; i < entry->stream_count; ++i)
            if (entry->streams[i].owner_token == owner_token) break;
        if (i < entry->stream_count) {
            remove_entry(cache, entry);
            ++removed;
        }
        entry = next;
    }
    return removed;
}

int bsp_decode_cache_begin_draw(struct bsp_decode_cache *cache,
    const void *declaration, size_t declaration_bytes, unsigned long declared_mask,
    const struct bsp_decode_stream_key *streams, size_t stream_count,
    struct bsp_decode_cache_view *view)
{
    struct bsp_decode_cache_entry *entry;
    unsigned long capacity, hash;
    size_t valid_bytes, packed_attributes, value_bytes, values_offset, allocation_bytes;
    if (view) { view->cache = NULL; view->entry = NULL; }
    if (!cache || !view || !cache->byte_cap ||
        !validate_streams(declaration, declaration_bytes, declared_mask,
            streams, stream_count, &capacity)) return BSP_DECODE_CACHE_INVALID;
    if (!advance_serial(cache)) return BSP_DECODE_CACHE_BYPASS;
    hash = hash_key(declaration, declaration_bytes, declared_mask, streams, stream_count);
    entry = find_entry(cache, declaration, declaration_bytes, declared_mask,
        streams, stream_count, hash);
    if (entry) {
        touch_entry(cache, entry);
        view->cache = cache;
        view->entry = entry;
        return BSP_DECODE_CACHE_HIT;
    }
    if ((size_t)capacity > SIZE_MAX / (popcount_mask(declared_mask) * 4U * sizeof(float)))
        return BSP_DECODE_CACHE_BYPASS;
    packed_attributes = popcount_mask(declared_mask);
    value_bytes = (size_t)capacity * packed_attributes * 4U * sizeof(float);
    valid_bytes = (size_t)(capacity / 8U) + (capacity % 8U != 0);
    if (valid_bytes > SIZE_MAX - data_offset()) return BSP_DECODE_CACHE_BYPASS;
    values_offset = align_float(data_offset() + valid_bytes);
    if (values_offset < data_offset() + valid_bytes || value_bytes > SIZE_MAX - values_offset)
        return BSP_DECODE_CACHE_BYPASS;
    allocation_bytes = values_offset + value_bytes;
    if (allocation_bytes > cache->byte_cap) return BSP_DECODE_CACHE_BYPASS;
    {
        struct bsp_decode_cache_entry *old = cache->oldest;
        while (old) {
            struct bsp_decode_cache_entry *next = old->newer;
            if (entry_stale_for(old, streams, stream_count)) remove_entry(cache, old);
            old = next;
        }
    }
    while (cache->bytes_used > cache->byte_cap - allocation_bytes && cache->oldest)
        remove_entry(cache, cache->oldest);
    entry = (struct bsp_decode_cache_entry *)malloc(allocation_bytes);
    if (!entry) return BSP_DECODE_CACHE_BYPASS;
    memset(entry, 0, data_offset());
    entry->hash = hash;
    entry->mask = declared_mask;
    entry->capacity = capacity;
    entry->packed_attributes = (unsigned)packed_attributes;
    entry->allocation_bytes = allocation_bytes;
    entry->declaration_bytes = declaration_bytes;
    entry->valid_offset = data_offset();
    entry->values_offset = values_offset;
    entry->stream_count = stream_count;
    memcpy(entry->declaration, declaration, declaration_bytes);
    memcpy(entry->streams, streams, stream_count * sizeof(*streams));
    memset((unsigned char *)entry + entry->valid_offset, 0, valid_bytes);
    {
        unsigned bucket = hash % BSP_DECODE_HASH_BUCKETS;
        entry->hash_next = cache->buckets[bucket];
        cache->buckets[bucket] = entry;
    }
    entry->older = cache->newest;
    if (cache->newest) cache->newest->newer = entry;
    else cache->oldest = entry;
    cache->newest = entry;
    cache->bytes_used += allocation_bytes;
    ++cache->entry_count;
    view->cache = cache;
    view->entry = entry;
    return BSP_DECODE_CACHE_MISS;
}

int bsp_decode_cache_lookup_vertex(const struct bsp_decode_cache_view *view,
    unsigned long source_index, const float *draw_defaults, float *output_record)
{
    const struct bsp_decode_cache_entry *entry;
    const unsigned char *valid;
    const float *values;
    unsigned attribute, packed = 0;
    if (!view || !draw_defaults || !output_record) return BSP_DECODE_CACHE_INVALID;
    {
        unsigned i;
        for (i = 0; i < BSP_DECODE_RECORD_FLOATS; ++i) output_record[i] = draw_defaults[i];
    }
    entry = view->entry;
    if (!view->cache || !entry) return BSP_DECODE_CACHE_BYPASS;
    if (source_index >= entry->capacity) return BSP_DECODE_CACHE_INVALID;
    valid = (const unsigned char *)entry + entry->valid_offset;
    if (!(valid[source_index / 8U] & (1U << (source_index % 8U))))
        return BSP_DECODE_CACHE_MISS;
    values = (const float *)((const unsigned char *)entry + entry->values_offset) +
        (size_t)source_index * entry->packed_attributes * 4U;
    for (attribute = 0; attribute < BSP_DECODE_ATTRIBUTE_COUNT; ++attribute) {
        unsigned component;
        if (!(entry->mask & (1UL << attribute))) continue;
        for (component = 0; component < 4; ++component)
            output_record[attribute * 4U + component] = values[packed * 4U + component];
        ++packed;
    }
    return BSP_DECODE_CACHE_HIT;
}

int bsp_decode_cache_store_vertex(const struct bsp_decode_cache_view *view,
    unsigned long source_index, const float *decoded_record)
{
    struct bsp_decode_cache_entry *entry;
    unsigned char *valid;
    float *values;
    unsigned attribute, component, packed = 0;
    if (!view || !decoded_record) return BSP_DECODE_CACHE_INVALID;
    entry = view->entry;
    if (!view->cache || !entry) return BSP_DECODE_CACHE_BYPASS;
    if (source_index >= entry->capacity) return BSP_DECODE_CACHE_INVALID;
    valid = (unsigned char *)entry + entry->valid_offset;
    values = (float *)((unsigned char *)entry + entry->values_offset) +
        (size_t)source_index * entry->packed_attributes * 4U;
    /* Validate the entire replacement before touching a previously valid row. */
    for (attribute = 0; attribute < BSP_DECODE_ATTRIBUTE_COUNT; ++attribute) {
        if (!(entry->mask & (1UL << attribute))) continue;
        for (component = 0; component < 4; ++component) {
            float value = decoded_record[attribute * 4U + component];
            if (!finite_float(value)) return BSP_DECODE_CACHE_INVALID;
        }
    }
    for (attribute = 0; attribute < BSP_DECODE_ATTRIBUTE_COUNT; ++attribute) {
        if (!(entry->mask & (1UL << attribute))) continue;
        for (component = 0; component < 4; ++component) {
            float value = decoded_record[attribute * 4U + component];
            values[packed * 4U + component] = value;
        }
        ++packed;
    }
    valid[source_index / 8U] |= (unsigned char)(1U << (source_index % 8U));
    return BSP_DECODE_CACHE_MISS;
}
