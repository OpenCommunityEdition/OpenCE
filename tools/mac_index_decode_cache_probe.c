#include "../port/mac/mac_index_decode_cache.h"

#include <stdio.h>
#include <string.h>

static unsigned checks, failures;
#define CHECK(value) do { ++checks; if (!(value)) { ++failures; \
    printf("FAIL line %d: %s\n", __LINE__, #value); } } while (0)

#define RECORD_FLOATS (16U * 4U)
#define MAX_RECORDS 8U

static void decode_source_record(unsigned source_index, float *record)
{
    unsigned attribute, component;
    /* Model the complete initialized vertex: default attributes, converted
     * declaration values and their implicit components are all copied. */
    for (attribute = 0; attribute < 16; ++attribute)
        for (component = 0; component < 4; ++component)
            record[attribute * 4 + component] =
                (float)(source_index * 17U + attribute * 4U + component) * 0.125f;
    record[0] = 1.0f;
    record[1] = (float)(source_index & 255U) * (1.0f / 255.0f);
    record[2] = (float)((source_index >> 3) & 255U) * (1.0f / 255.0f);
    record[3] = 1.0f;
    record[4 * 4 + 0] = (float)(short)source_index;
    record[4 * 4 + 1] = (float)(short)(source_index ^ 0x1555U);
    record[4 * 4 + 2] = 0.0f;
    record[4 * 4 + 3] = 1.0f;
}

static void check_expansion(void)
{
    static const unsigned short keys[] = { 5, 2, 6, 2, 5, 2 };
    const unsigned count = (unsigned)(sizeof(keys) / sizeof(keys[0]));
    const unsigned base_vertex = 0x1200U;
    struct mac_index_decode_cache cache = { 0 };
    float expected[MAX_RECORDS * RECORD_FLOATS];
    float actual[MAX_RECORDS * RECORD_FLOATS];
    unsigned i, first_slot, decoded = 0;
    int result;

    CHECK(mac_index_decode_cache_begin(&cache, count, 2, 6) == 1);
    CHECK(cache.span == 5 && cache.capacity >= 5 && cache.active);
    for (i = 0; i < count; ++i)
        decode_source_record(base_vertex + keys[i],
            expected + i * RECORD_FLOATS);

    for (i = 0; i < count; ++i) {
        result = mac_index_decode_cache_lookup(&cache, keys[i], &first_slot);
        if (result == 1) {
            memcpy(actual + i * RECORD_FLOATS,
                actual + first_slot * RECORD_FLOATS,
                RECORD_FLOATS * sizeof(float));
        } else {
            CHECK(result == 0);
            decode_source_record(base_vertex + keys[i],
                actual + i * RECORD_FLOATS);
            CHECK(mac_index_decode_cache_record(&cache, keys[i], i) == 1);
            ++decoded;
        }
    }
    CHECK(!memcmp(expected, actual, count * RECORD_FLOATS * sizeof(float)));
    CHECK(decoded == 3 && cache.unique_count == 3 && cache.reuse_count == 3);
    CHECK(mac_index_decode_cache_lookup(&cache, 1, &first_slot) == -1);
    CHECK(mac_index_decode_cache_record(&cache, 2, 0) == -1);

    /* A new draw using the same key must not see the preceding draw's slot. */
    CHECK(mac_index_decode_cache_begin(&cache, 3, 2, 2) == 1);
    CHECK(mac_index_decode_cache_lookup(&cache, 2, &first_slot) == 0);
    CHECK(mac_index_decode_cache_record(&cache, 2, 2) == 1);
    CHECK(mac_index_decode_cache_lookup(&cache, 2, &first_slot) == 1);
    CHECK(first_slot == 2);
    mac_index_decode_cache_end(&cache);
    CHECK(!cache.active && mac_index_decode_cache_lookup(&cache, 2, &first_slot) == 0);
    mac_index_decode_cache_dispose(&cache);
}

static void check_fallbacks(void)
{
    struct mac_index_decode_cache cache = { 0 };
    unsigned slot;
    CHECK(mac_index_decode_cache_begin(&cache, 3, 8, 10) == 0);
    CHECK(!cache.active);
    CHECK(mac_index_decode_cache_lookup(&cache, 8, &slot) == 0);
    /* Duplicates can exist even when count<=span; fallback is intentional. */
    CHECK(mac_index_decode_cache_begin(&cache, 3, 2, 4) == 0);
    CHECK(mac_index_decode_cache_begin(&cache, 0, 0, 0) == 0);
    CHECK(mac_index_decode_cache_begin(&cache, 65536U, 0, 1) == 0);
    CHECK(mac_index_decode_cache_begin(&cache, 3, 4, 2) == 0);
    CHECK(mac_index_decode_cache_begin(&cache, 3, 0, 65536U) == 0);
    mac_index_decode_cache_dispose(&cache);
}

static void check_sentinel_boundary(void)
{
    struct mac_index_decode_cache cache = { 0 };
    unsigned slot;
    CHECK(mac_index_decode_cache_begin(&cache, 65535U, 65535U, 65535U) == 1);
    CHECK(cache.span == 1 && cache.capacity >= 1);
    CHECK(mac_index_decode_cache_record(&cache, 65535U, 65534U) == 1);
    CHECK(mac_index_decode_cache_lookup(&cache, 65535U, &slot) == 1);
    CHECK(slot == 65534U);
    CHECK(cache.first_output[0] != 0xffffU);
    mac_index_decode_cache_dispose(&cache);
}

int main(void)
{
    check_expansion();
    check_fallbacks();
    check_sentinel_boundary();
    printf("native index decode reuse probe: %u checks, %u failures\n",
        checks, failures);
    return failures ? 1 : 0;
}
