#include "mac_index_decode_cache.h"

#include <stdlib.h>
#include <string.h>

#define MAC_INDEX_DECODE_CACHE_SENTINEL 0xffffU
#define MAC_INDEX_DECODE_CACHE_KEY_MAX  0xffffU

int mac_index_decode_cache_begin(struct mac_index_decode_cache *cache,
    unsigned count, unsigned minimum, unsigned maximum)
{
    unsigned span;
    size_t bytes;
    unsigned short *grown;

    if (!cache)
        return 0;
    cache->active = 0;
    cache->count = count;
    cache->minimum = minimum;
    cache->span = 0;
    cache->unique_count = 0;
    cache->reuse_count = 0;

    if (!count || count > 65535U || minimum > maximum ||
        maximum > MAC_INDEX_DECODE_CACHE_KEY_MAX)
        return 0;
    span = maximum - minimum + 1U;
    if (!span || count <= span)
        return 0;

    bytes = (size_t)span * sizeof(*cache->first_output);
    if ((bytes / sizeof(*cache->first_output)) != span)
        return 0;
    if (cache->capacity < span) {
        grown = (unsigned short *)realloc(cache->first_output, bytes);
        if (!grown)
            return 0;
        cache->first_output = grown;
        cache->capacity = span;
    }
    memset(cache->first_output, 0xff, bytes);
    cache->span = span;
    cache->active = 1;
    return 1;
}

int mac_index_decode_cache_lookup(struct mac_index_decode_cache *cache,
    unsigned key, unsigned *first_output)
{
    unsigned offset;
    unsigned short slot;
    if (!cache || !first_output)
        return -1;
    if (!cache->active)
        return 0;
    if (key < cache->minimum || key - cache->minimum >= cache->span)
        return -1;
    offset = key - cache->minimum;
    slot = cache->first_output[offset];
    if (slot == MAC_INDEX_DECODE_CACHE_SENTINEL)
        return 0;
    if (slot >= cache->count)
        return -1;
    *first_output = slot;
    ++cache->reuse_count;
    return 1;
}

int mac_index_decode_cache_record(struct mac_index_decode_cache *cache,
    unsigned key, unsigned output_slot)
{
    unsigned offset;
    if (!cache)
        return -1;
    if (!cache->active)
        return 0;
    if (key < cache->minimum || key - cache->minimum >= cache->span ||
        output_slot >= cache->count || output_slot >= MAC_INDEX_DECODE_CACHE_SENTINEL)
        return -1;
    offset = key - cache->minimum;
    if (cache->first_output[offset] != MAC_INDEX_DECODE_CACHE_SENTINEL)
        return -1;
    cache->first_output[offset] = (unsigned short)output_slot;
    ++cache->unique_count;
    return 1;
}

void mac_index_decode_cache_end(struct mac_index_decode_cache *cache)
{
    if (cache) {
        cache->active = 0;
        cache->count = 0;
        cache->minimum = 0;
        cache->span = 0;
        cache->unique_count = 0;
        cache->reuse_count = 0;
    }
}

void mac_index_decode_cache_dispose(struct mac_index_decode_cache *cache)
{
    if (cache) {
        free(cache->first_output);
        memset(cache, 0, sizeof(*cache));
    }
}
