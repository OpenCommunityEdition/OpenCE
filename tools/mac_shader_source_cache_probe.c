#include <OpenGL/OpenGL.h>
#include "../port/mac/mac_shader_source_cache.h"
#include "../source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.h"
#undef memset
#undef memcpy
#undef memcmp
#undef malloc
#undef free
#undef strcmp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); } } while (0)

static int fetch(struct mac_shader_source_cache *cache, unsigned long index,
    struct mac_shader_source_result *result)
{
    const DWORD *function = (const DWORD *)vertex_shader_table[index].code;
    unsigned long count = function[0] >> 16;
    struct nv2a_pixel_shader_key key;
    memset(&key, 0, sizeof(key));
    key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x04200000;
    key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = 0x00001400;
    return mac_shader_source_cache_get(cache, function + 1, count, 0, NULL,
        &key, result);
}

int main(void)
{
    struct mac_shader_source_cache cache;
    struct mac_shader_source_result first, second, current;
    const char *saved_vertex, *saved_pixel;
    unsigned long i;
    mac_shader_source_cache_init(&cache);
    CHECK(fetch(&cache, 0, &first));
    CHECK(first.failure == MAC_SHADER_SOURCE_OK && first.vertex_source && first.pixel_source);
    CHECK(first.identity.cache_epoch && first.identity.vertex_entry &&
        first.identity.pixel_entry);
    saved_vertex = first.vertex_source; saved_pixel = first.pixel_source;
    CHECK(fetch(&cache, 0, &second));
    CHECK(second.vertex_source == saved_vertex && second.pixel_source == saved_pixel);
    CHECK(second.identity.cache_epoch == first.identity.cache_epoch &&
        second.identity.vertex_entry == first.identity.vertex_entry &&
        second.identity.pixel_entry == first.identity.pixel_entry);
    CHECK(cache.vertex_hits == 1 && cache.pixel_hits == 1);
    mac_shader_source_result_dispose(&second);

    /* Keep both returned entries pinned while all other retail programs are
       admitted; their pointers must remain valid until result disposal. */
    for (i = 1; i < NUMBER_OF_VERTEX_SHADERS; ++i) {
        if (fetch(&cache, i, &current)) {
            CHECK(current.vertex_source && current.pixel_source);
            mac_shader_source_result_dispose(&current);
        }
    }
    CHECK(strcmp(first.vertex_source, saved_vertex) == 0);
    CHECK(strcmp(first.pixel_source, saved_pixel) == 0);
    CHECK(cache.bytes <= MAC_SHADER_SOURCE_CACHE_BYTES);
    {
        unsigned long vertices = 0, pixels = 0;
        for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i) {
            if (cache.vertex[i]) ++vertices;
            if (cache.pixel[i]) ++pixels;
        }
        CHECK(vertices <= MAC_SHADER_SOURCE_CACHE_ENTRIES);
        CHECK(pixels <= MAC_SHADER_SOURCE_CACHE_ENTRIES);
    }
    mac_shader_source_result_dispose(&first);

    /* Each member of the vertex key must distinguish a lookup. An invalid
       mutation is still a miss and must report failure rather than reuse. */
    {
        const DWORD *function = (const DWORD *)vertex_shader_table[0].code;
        unsigned long count = function[0] >> 16;
        DWORD *copy = (DWORD *)malloc((size_t)count * 4 * sizeof(DWORD));
        struct nv2a_vertex_lighting lighting;
        struct nv2a_pixel_shader_key key;
        memset(&lighting, 0, sizeof(lighting));
        memset(&key, 0, sizeof(key));
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x04200000;
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = 0x00001400;
        memcpy(copy, function + 1, (size_t)count * 4 * sizeof(DWORD));
        copy[(size_t)(count - 1) * 4 + 3] ^= 1;
        memset(&current, 0, sizeof(current));
        (void)mac_shader_source_cache_get(&cache, copy, count, 0, NULL, &key, &current);
        CHECK(cache.vertex_misses > NUMBER_OF_VERTEX_SHADERS);
        mac_shader_source_result_dispose(&current);
        memcpy(copy, function + 1, (size_t)count * 4 * sizeof(DWORD));
        memset(&current, 0, sizeof(current));
        (void)mac_shader_source_cache_get(&cache, copy, count, 1, NULL, &key, &current);
        CHECK(cache.vertex_misses > NUMBER_OF_VERTEX_SHADERS + 1);
        mac_shader_source_result_dispose(&current);
        free(copy);
    }
    mac_shader_source_cache_dispose(&cache);
    CHECK(cache.bytes == 0);

    /* Eviction never recycles an entry token within an epoch, and reinit gets
       a new epoch even if the cache object and allocator addresses are reused. */
    {
        struct mac_shader_source_identity old_identity, evicted_identity;
        const DWORD *function = (const DWORD *)vertex_shader_table[0].code;
        struct nv2a_pixel_shader_key key;
        struct nv2a_vertex_lighting lighting;
        mac_shader_source_cache_init(&cache);
        CHECK(fetch(&cache, 0, &first));
        old_identity = first.identity;
        mac_shader_source_result_dispose(&first);
        memset(&key, 0, sizeof(key));
        memset(&lighting, 0, sizeof(lighting));
        lighting.lights = 1;
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x04200000;
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = 0x00001400;
        for (i = 1; i <= MAC_SHADER_SOURCE_CACHE_ENTRIES + 1; ++i) {
            CHECK(mac_shader_source_cache_get(&cache, function + 1,
                function[0] >> 16, 0, &lighting, &key, &current));
            lighting.normal_instruction = i;
            mac_shader_source_result_dispose(&current);
        }
        CHECK(fetch(&cache, 0, &current));
        evicted_identity = current.identity;
        CHECK(evicted_identity.cache_epoch == old_identity.cache_epoch);
        CHECK(evicted_identity.vertex_entry != old_identity.vertex_entry);
        mac_shader_source_result_dispose(&current);
        mac_shader_source_cache_dispose(&cache);
        mac_shader_source_cache_init(&cache);
        CHECK(fetch(&cache, 0, &current));
        CHECK(current.identity.cache_epoch != old_identity.cache_epoch);
        mac_shader_source_result_dispose(&current);
        mac_shader_source_cache_dispose(&cache);
    }

    mac_shader_source_cache_init(&cache);
    CHECK(fetch(&cache, 0, &first));
    mac_shader_source_result_dispose(&first);
    {
        const DWORD *function = (const DWORD *)vertex_shader_table[0].code;
        unsigned long count = function[0] >> 16;
        struct nv2a_vertex_lighting lighting;
        struct nv2a_pixel_shader_key key;
        memset(&lighting, 0, sizeof(lighting));
        memset(&key, 0, sizeof(key));
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x04200000;
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = 0x00001400;
        memset(&current, 0, sizeof(current));
        (void)mac_shader_source_cache_get(&cache, function + 1, count, 0,
            &lighting, &key, &current);
        CHECK(cache.vertex_misses == 2);
        mac_shader_source_result_dispose(&current);
        lighting.lights = 1;
        memset(&current, 0, sizeof(current));
        (void)mac_shader_source_cache_get(&cache, function + 1, count, 0,
            &lighting, &key, &current);
        CHECK(cache.vertex_misses == 3);
        mac_shader_source_result_dispose(&current);
        memset(&current, 0, sizeof(current));
        key.texture_modes = 1;
        (void)mac_shader_source_cache_get(&cache, function + 1, count, 0,
            NULL, &key, &current);
        CHECK(cache.pixel_misses == 2);
        mac_shader_source_result_dispose(&current);
    }
    mac_shader_source_cache_dispose(&cache);
    /* Uniform-only color changes must neither change generated code nor
       consume source-cache entries. Compare the uncached generator as well. */
    mac_shader_source_cache_init(&cache);
    {
        const DWORD *function = (const DWORD *)vertex_shader_table[0].code;
        struct nv2a_pixel_shader_key key;
        unsigned long word, slot;
        char *reference, *generated;
        memset(&key, 0, sizeof(key));
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x04200000;
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = 0x00001400;
        reference = nv2a_pixel_shader_to_glsl(&key);
        CHECK(reference != NULL);
        CHECK(mac_shader_source_cache_get(&cache, function + 1, function[0] >> 16,
            0, NULL, &key, &first));
        for (word = 0; word < 18; ++word) {
            slot = word < 16 ? D3DRS_PSCONSTANT0_0 + word :
                D3DRS_PSFINALCOMBINERCONSTANT0 + word - 16;
            key.combiner_state[slot] = 0x193b5d7fUL + word;
            generated = nv2a_pixel_shader_to_glsl(&key);
            CHECK(reference && generated && strcmp(reference, generated) == 0);
            CHECK(mac_shader_source_cache_get(&cache, function + 1, function[0] >> 16,
                0, NULL, &key, &current));
            CHECK(current.pixel_source == first.pixel_source && cache.pixel_misses == 1);
            mac_shader_source_result_dispose(&current);
            free(generated);
        }
        CHECK(cache.pixel_hits == 18);
        mac_shader_source_result_dispose(&first);
        free(reference);
    }
    mac_shader_source_cache_dispose(&cache);
    /* Gather variants must not alias the ordinary source cache entry. */
    mac_shader_source_cache_init(&cache);
    {
        const DWORD *function = (const DWORD *)vertex_shader_table[9].code;
        struct nv2a_pixel_shader_key key;
        memset(&key, 0, sizeof(key));
        CHECK(mac_shader_source_cache_get(&cache, function + 1, function[0] >> 16,
            0, NULL, &key, &first));
        CHECK(first.bone_gather.version == 0);
        mac_shader_source_cache_set_bone_gather(&cache, 1);
        CHECK(mac_shader_source_cache_get(&cache, function + 1, function[0] >> 16,
            0, NULL, &key, &current));
        CHECK(current.bone_gather.version != 0);
        CHECK((current.input_mask & 0xfc00UL) == 0xfc00UL);
        CHECK(current.vertex_source && first.vertex_source &&
            strcmp(current.vertex_source, first.vertex_source) != 0);
        mac_shader_source_result_dispose(&current);
        CHECK(mac_shader_source_cache_get(&cache, function + 1, function[0] >> 16,
            0, NULL, &key, &current));
        CHECK(current.bone_gather.version != 0 && cache.vertex_misses == 2);
        mac_shader_source_result_dispose(&current);
        mac_shader_source_cache_set_bone_gather(&cache, 0);
        CHECK(mac_shader_source_cache_get(&cache, function + 1, function[0] >> 16,
            0, NULL, &key, &current));
        CHECK(current.bone_gather.version == 0 &&
            current.vertex_source == first.vertex_source && cache.vertex_hits == 2);
        mac_shader_source_result_dispose(&current);
        mac_shader_source_result_dispose(&first);
        {
            unsigned long shader, recognized = 0;
            mac_shader_source_cache_set_bone_gather(&cache, 1);
            printf("bone gather recognized retail shaders:");
            for (shader = 0; shader < NUMBER_OF_VERTEX_SHADERS; ++shader) {
                function = (const DWORD *)vertex_shader_table[shader].code;
                CHECK(mac_shader_source_cache_get(&cache, function + 1,
                    function[0] >> 16, 0, NULL, &key, &current));
                if (current.bone_gather.version) {
                    ++recognized; printf(" %lu", shader);
                    CHECK(current.vertex_source &&
                        !strstr(current.vertex_source, "clamp(float(a0)"));
                    CHECK((current.input_mask & 0xfc00UL) == 0xfc00UL);
                }
                mac_shader_source_result_dispose(&current);
            }
            printf(" (%lu total)\n", recognized);
            CHECK(recognized >= 2);
        }
    }
    mac_shader_source_cache_dispose(&cache);
    printf("shader source cache checks=%d failures=%d bytes=%lu\n",
        checks, failures, (unsigned long)cache.bytes);
    return failures ? 1 : 0;
}
