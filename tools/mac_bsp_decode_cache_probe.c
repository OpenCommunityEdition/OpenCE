#include "bsp_decode_cache.h"

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
static unsigned failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; \
    fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__, __LINE__, #x); } } while (0)

static void fill(float *record, float value)
{
    unsigned i;
    for (i = 0; i < BSP_DECODE_RECORD_FLOATS; ++i) record[i] = value + (float)i;
}

static void set_attribute(float *record, unsigned attribute, float value)
{
    unsigned i;
    for (i = 0; i < 4; ++i) record[attribute * 4 + i] = value + (float)i;
}

static int check_attribute(const float *record, unsigned attribute, float value)
{
    unsigned i;
    for (i = 0; i < 4; ++i)
        if (record[attribute * 4 + i] != value + (float)i) return 0;
    return 1;
}

static int same_record(const float *a, const float *b)
{
    unsigned i;
    for (i = 0; i < BSP_DECODE_RECORD_FLOATS; ++i) if (a[i] != b[i]) return 0;
    return 1;
}

static int begin(struct bsp_decode_cache *cache, struct bsp_decode_cache_view *view,
    const unsigned char *declaration, size_t declaration_bytes, unsigned long mask,
    const struct bsp_decode_stream_key *streams, size_t count)
{
    return bsp_decode_cache_begin_draw(cache, declaration, declaration_bytes,
        mask, streams, count, view);
}

int main(void)
{
    struct bsp_decode_cache cache;
    struct bsp_decode_cache_view view, second_view;
    struct bsp_decode_stream_key streams[2];
    unsigned char declaration[] = { 0, 0x21, 0, 0, 4, 0, 0, 0,
        1, 0x33, 8, 0, 4, 0, 0, 0 };
    unsigned char declaration_changed[] = { 0, 0x21, 4, 0, 4, 0, 0, 0,
        1, 0x33, 8, 0, 4, 0, 0, 0 };
    unsigned char too_large[BSP_DECODE_MAX_DECLARATION_BYTES + 1];
    float defaults[BSP_DECODE_RECORD_FLOATS], next_defaults[BSP_DECODE_RECORD_FLOATS];
    float decoded[BSP_DECODE_RECORD_FLOATS], bad_decoded[BSP_DECODE_RECORD_FLOATS];
    float output[BSP_DECODE_RECORD_FLOATS];
    unsigned long mask = (1UL << 0) | (1UL << 3);
    size_t entry_cost;
    unsigned i;

    memset(streams, 0, sizeof(streams));
    streams[0].slot = 0; streams[0].owner_token = 10; streams[0].write_generation = 1;
    streams[0].base_offset = 0; streams[0].stride = 32; streams[0].vertex_capacity = 16;
    streams[1].slot = 1; streams[1].owner_token = 20; streams[1].write_generation = 4;
    streams[1].base_offset = 64; streams[1].stride = 16; streams[1].vertex_capacity = 12;
    CHECK(!bsp_decode_cache_init(NULL, 4096));
    CHECK(!bsp_decode_cache_init(&cache, 0));
    CHECK(!bsp_decode_cache_init(&cache, BSP_DECODE_MAX_CACHE_BYTES + 1));
    CHECK(bsp_decode_cache_init(&cache, 1024 * 1024));
    fill(defaults, 1.0f); fill(next_defaults, 90.0f); fill(decoded, 20.0f);

    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_MISS);
    CHECK(view.entry != NULL && cache.entry_count == 1);
    CHECK(bsp_decode_cache_lookup_vertex(&view, 5, defaults, output) == BSP_DECODE_CACHE_MISS);
    CHECK(same_record(output, defaults));
    set_attribute(decoded, 0, 101.0f); set_attribute(decoded, 3, 303.0f);
    CHECK(bsp_decode_cache_store_vertex(&view, 5, decoded) == BSP_DECODE_CACHE_MISS);
    memcpy(bad_decoded, decoded, sizeof(bad_decoded));
    bad_decoded[0] = -999.0f;
    bad_decoded[15] = NAN; /* A late component must not partially replace a valid row. */
    CHECK(bsp_decode_cache_store_vertex(&view, 5, bad_decoded) == BSP_DECODE_CACHE_INVALID);
    CHECK(bsp_decode_cache_lookup_vertex(&view, 5, defaults, output) == BSP_DECODE_CACHE_HIT);
    CHECK(check_attribute(output, 0, 101.0f) && check_attribute(output, 3, 303.0f));
    CHECK(bsp_decode_cache_store_vertex(&view, 7, decoded) == BSP_DECODE_CACHE_MISS);
    CHECK(cache.entry_count == 1); /* Per-vertex data is one direct array, not LRU nodes. */
    CHECK(begin(&cache, &second_view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_HIT);
    CHECK(bsp_decode_cache_lookup_vertex(&second_view, 5, next_defaults, output) == BSP_DECODE_CACHE_HIT);
    CHECK(check_attribute(output, 0, 101.0f) && check_attribute(output, 3, 303.0f));
    for (i = 0; i < BSP_DECODE_ATTRIBUTE_COUNT; ++i)
        if (i != 0 && i != 3) CHECK(output[i * 4] == next_defaults[i * 4]);

    CHECK(bsp_decode_cache_lookup_vertex(&second_view, 12, defaults, output) == BSP_DECODE_CACHE_INVALID);
    CHECK(same_record(output, defaults));
    /* Start key-invalidation tests from one exact bundle so counts are clear. */
    bsp_decode_cache_dispose(&cache);
    CHECK(bsp_decode_cache_init(&cache, 1024 * 1024));
    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 1) == BSP_DECODE_CACHE_MISS);
    CHECK(begin(&cache, &view, declaration_changed, sizeof(declaration_changed), mask, streams, 2) == BSP_DECODE_CACHE_MISS);
    CHECK(begin(&cache, &view, declaration, sizeof(declaration), 1UL << 0, streams, 2) == BSP_DECODE_CACHE_MISS);
    bsp_decode_cache_dispose(&cache);
    CHECK(bsp_decode_cache_init(&cache, 1024 * 1024));
    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_MISS);
    {
        struct bsp_decode_stream_key changed[2];
        memcpy(changed, streams, sizeof(changed)); changed[1].write_generation++;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, changed, 2) == BSP_DECODE_CACHE_MISS);
        CHECK(cache.entry_count == 1); /* New generation drops stale bundle contents. */
        memcpy(changed, streams, sizeof(changed)); changed[0].base_offset = 32;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, changed, 2) == BSP_DECODE_CACHE_MISS);
        memcpy(changed, streams, sizeof(changed)); changed[0].owner_token = 11;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, changed, 2) == BSP_DECODE_CACHE_MISS);
        memcpy(changed, streams, sizeof(changed)); changed[0].stride = 40;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, changed, 2) == BSP_DECODE_CACHE_MISS);
        memcpy(changed, streams, sizeof(changed)); changed[0].vertex_capacity = 15;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, changed, 2) == BSP_DECODE_CACHE_MISS);
        CHECK(bsp_decode_cache_invalidate_owner(&cache, 20) == 4);
        CHECK(cache.entry_count == 0 && cache.bytes_used == 0);
    }

    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_MISS);
    CHECK(bsp_decode_cache_store_vertex(&view, 3, decoded) == BSP_DECODE_CACHE_MISS);
    CHECK(bsp_decode_cache_invalidate_owner(&cache, 10) == 1);
    CHECK(cache.entry_count == 0);

    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_MISS);
    entry_cost = cache.bytes_used;
    CHECK(entry_cost > 0 && entry_cost < cache.byte_cap);
    bsp_decode_cache_dispose(&cache);
    CHECK(bsp_decode_cache_init(&cache, entry_cost * 2));
    streams[0].owner_token = 30; streams[1].owner_token = 40;
    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_MISS);
    entry_cost = cache.bytes_used;
    streams[0].base_offset = 32;
    CHECK(begin(&cache, &second_view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_MISS);
    CHECK(cache.entry_count == 2 && cache.bytes_used <= cache.byte_cap);
    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_HIT);
    streams[0].base_offset = 64;
    CHECK(begin(&cache, &second_view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_MISS);
    CHECK(cache.entry_count == 2 && cache.bytes_used <= cache.byte_cap);
    streams[0].base_offset = 32;
    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_HIT);
    streams[0].base_offset = 0;
    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_MISS);
    CHECK(cache.entry_count == 2 && cache.bytes_used <= cache.byte_cap);
    streams[0].base_offset = 32;
    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_HIT);

    CHECK(begin(&cache, &view, too_large, sizeof(too_large), mask, streams, 2) == BSP_DECODE_CACHE_INVALID);
    CHECK(view.entry == NULL);
    {
        struct bsp_decode_stream_key bad[2];
        memcpy(bad, streams, sizeof(bad)); bad[0].owner_token = 0;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, bad, 2) == BSP_DECODE_CACHE_INVALID);
        memcpy(bad, streams, sizeof(bad)); bad[0].write_generation = 0;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, bad, 2) == BSP_DECODE_CACHE_INVALID);
        memcpy(bad, streams, sizeof(bad)); bad[1].stride = 0;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, bad, 2) == BSP_DECODE_CACHE_INVALID);
        memcpy(bad, streams, sizeof(bad)); bad[1].slot = bad[0].slot;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, bad, 2) == BSP_DECODE_CACHE_INVALID);
        memcpy(bad, streams, sizeof(bad)); bad[1].base_offset = UINT32_MAX - 1UL;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, bad, 2) == BSP_DECODE_CACHE_INVALID);
        memcpy(bad, streams, sizeof(bad)); bad[0].vertex_capacity = UINT32_MAX;
        CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, bad, 2) == BSP_DECODE_CACHE_INVALID);
    }
    CHECK(bsp_decode_cache_invalidate_owner(&cache, 30) == 2);

    /* A legal key whose dense direct array exceeds this cache's budget bypasses. */
    bsp_decode_cache_dispose(&cache);
    CHECK(bsp_decode_cache_init(&cache, 1024));
    streams[0].vertex_capacity = 65536; streams[1].vertex_capacity = 65536;
    streams[0].base_offset = streams[1].base_offset = 0;
    CHECK(begin(&cache, &view, declaration, sizeof(declaration), 0xffffUL, streams, 2) == BSP_DECODE_CACHE_BYPASS);
    CHECK(view.entry == NULL && cache.entry_count == 0);

    bsp_decode_cache_dispose(&cache);
    CHECK(cache.entry_count == 0 && cache.bytes_used == 0);
    CHECK(bsp_decode_cache_init(&cache, 4096));
    streams[0].vertex_capacity = streams[1].vertex_capacity = 8;
    CHECK(begin(&cache, &view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_MISS);
    cache.use_serial = ULLONG_MAX;
    CHECK(begin(&cache, &second_view, declaration, sizeof(declaration), mask, streams, 2) == BSP_DECODE_CACHE_BYPASS);
    CHECK(cache.entry_count == 0 && cache.bytes_used == 0);
    bsp_decode_cache_dispose(&cache);
    printf("BSP decoded-vertex cache v2: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
