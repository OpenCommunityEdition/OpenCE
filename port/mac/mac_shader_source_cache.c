/* CPU-only cache for the deterministic NV2A-to-GLSL conversion path. */
#undef memset
#undef memcpy
#undef memcmp
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef strlen
#undef strncpy
#include "mac_shader_source_cache.h"
#include "mac_glsl120.h"
#include "mac_flags.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct mac_shader_source_entry
{
    int vertex;
    unsigned long used, references;
    unsigned long identity;
    /* key_hash() of the instructions or the pixel key: entries whose hash
       differs are not compared */
    unsigned long hash;
    unsigned long packed_mask, instruction_count;
    unsigned long input_mask;
    int bone_gather_enabled;
    struct mac_bone_gather_plan bone_gather;
    struct nv2a_vertex_lighting lighting;
    int has_lighting;
    struct nv2a_pixel_shader_key pixel_key;
    DWORD *instructions;
    char *source;
    size_t bytes;
};

static unsigned long source_cache_next_epoch = 1;
static int source_cache_epoch_exhausted;

static unsigned long reserve_source_cache_epoch(void)
{
    unsigned long epoch;
    if (source_cache_epoch_exhausted) return 0;
    epoch = source_cache_next_epoch;
    if (source_cache_next_epoch == ULONG_MAX) source_cache_epoch_exhausted = 1;
    else ++source_cache_next_epoch;
    return epoch;
}

static unsigned long key_hash(const void *data, size_t bytes, unsigned long seed)
{
    const unsigned char *p = (const unsigned char *)data;
    unsigned long h = 2166136261UL ^ seed;
    unsigned int word;
    size_t i;
    for (i = 0; i + 4 <= bytes; i += 4) {
        memcpy(&word, p + i, 4);
        h = (h ^ word) * 16777619UL;
    }
    for (; i < bytes; ++i) h = (h ^ p[i]) * 16777619UL;
    return h;
}

static void free_entry(struct mac_shader_source_cache *cache,
    struct mac_shader_source_entry *entry)
{
    unsigned long i;
    if (!entry) return;
    if (cache->last_vertex == entry) cache->last_vertex = NULL;
    if (cache->last_pixel == entry) cache->last_pixel = NULL;
    for (i = 0; i < 8; ++i)
        if (cache->memo[i].entry == entry) cache->memo[i].entry = NULL;
    if (entry->vertex) {
        for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i)
            if (cache->vertex[i] == entry) { cache->vertex[i] = NULL; break; }
    } else {
        for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i)
            if (cache->pixel[i] == entry) { cache->pixel[i] = NULL; break; }
    }
    if (cache->bytes >= entry->bytes) cache->bytes -= entry->bytes;
    else cache->bytes = 0;
    free(entry->instructions); free(entry->source); free(entry);
}

static void evict_one(struct mac_shader_source_cache *cache, int vertex_only)
{
    struct mac_shader_source_entry *oldest = NULL;
    unsigned long i;
    for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i) {
        struct mac_shader_source_entry *entry = cache->vertex[i];
        if (entry && !entry->references && (!oldest || entry->used < oldest->used))
            oldest = entry;
    }
    if (!vertex_only) for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i) {
        struct mac_shader_source_entry *entry = cache->pixel[i];
        if (entry && !entry->references && (!oldest || entry->used < oldest->used))
            oldest = entry;
    }
    if (oldest) free_entry(cache, oldest);
}

static unsigned long next_use(struct mac_shader_source_cache *cache)
{
    if (++cache->clock == 0) {
        unsigned long i;
        cache->clock = 1;
        for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i) {
            if (cache->vertex[i]) cache->vertex[i]->used = 0;
            if (cache->pixel[i]) cache->pixel[i]->used = 0;
        }
    }
    return cache->clock;
}

void mac_shader_source_cache_init(struct mac_shader_source_cache *cache)
{
    if (!cache) return;
    memset(cache, 0, sizeof(*cache));
    cache->memo_enabled = mac_flag("HALO_MAC_PERF_SRC_MEMO", 0); /* (no gain on a G4: off) */
    cache->identity_epoch = reserve_source_cache_epoch();
    cache->next_entry_identity = cache->identity_epoch ? 1 : 0;
}

void mac_shader_source_cache_set_bone_gather(struct mac_shader_source_cache *cache,
    int enabled)
{ if (cache) cache->bone_gather_enabled = enabled != 0; }

void mac_shader_source_cache_dispose(struct mac_shader_source_cache *cache)
{
    unsigned long i;
    if (!cache) return;
    for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i) {
        if (cache->vertex[i]) free_entry(cache, cache->vertex[i]);
        if (cache->pixel[i]) free_entry(cache, cache->pixel[i]);
    }
    memset(cache, 0, sizeof(*cache));
}

void mac_shader_source_result_dispose(struct mac_shader_source_result *result)
{
    if (!result) return;
    if (result->vertex_pin && result->vertex_pin->references)
        --result->vertex_pin->references;
    if (result->pixel_pin && result->pixel_pin->references)
        --result->pixel_pin->references;
    free(result->owned_vertex_source); free(result->owned_pixel_source);
    free(result->modern_vertex_source);
    memset(result, 0, sizeof(*result));
}

static int vertex_key_equal(const struct mac_shader_source_entry *entry,
    const DWORD *instructions, unsigned long count, unsigned long packed_mask,
    const struct nv2a_vertex_lighting *lighting)
{
    int has_lighting = lighting != NULL;
    if (!entry || !entry->vertex || entry->instruction_count != count ||
        entry->packed_mask != packed_mask || entry->has_lighting != has_lighting)
        return 0;
    if (count && memcmp(entry->instructions, instructions,
        (size_t)count * 4 * sizeof(*instructions)) != 0) return 0;
    if (has_lighting && (entry->lighting.lights != lighting->lights ||
        entry->lighting.normal_instruction != lighting->normal_instruction ||
        entry->lighting.normal_register != lighting->normal_register ||
        entry->lighting.position_instruction != lighting->position_instruction ||
        entry->lighting.position_register != lighting->position_register)) return 0;
    return 1;
}

static struct mac_shader_source_entry *find_vertex(
    struct mac_shader_source_cache *cache, const DWORD *instructions,
    unsigned long count, unsigned long packed_mask,
    const struct nv2a_vertex_lighting *lighting)
{
    unsigned long i, hash;
    struct mac_shader_source_entry *last = cache->last_vertex;
    if (last && last->bone_gather_enabled == cache->bone_gather_enabled &&
        vertex_key_equal(last, instructions, count, packed_mask, lighting))
        return last;
    if (cache->memo_enabled) {
        for (i = 0; i < 8; ++i) {
            struct mac_shader_source_memo *memo = &cache->memo[i];
            if (memo->entry && memo->instructions == instructions && memo->count == count &&
                memo->packed_mask == packed_mask && memo->has_lighting == (lighting != NULL) &&
                memo->bone_gather_enabled == cache->bone_gather_enabled &&
                (!lighting || (memo->lighting.lights == lighting->lights &&
                memo->lighting.normal_instruction == lighting->normal_instruction &&
                memo->lighting.normal_register == lighting->normal_register &&
                memo->lighting.position_instruction == lighting->position_instruction &&
                memo->lighting.position_register == lighting->position_register)))
                return cache->last_vertex = memo->entry;
        }
    }
    hash = key_hash(instructions, (size_t)count * 4 * sizeof(*instructions), count);
    for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i)
        if (cache->vertex[i] && cache->vertex[i]->hash == hash &&
            cache->vertex[i]->bone_gather_enabled == cache->bone_gather_enabled &&
            vertex_key_equal(cache->vertex[i], instructions, count,
            packed_mask, lighting)) {
            if (cache->memo_enabled) {
                struct mac_shader_source_memo *memo = &cache->memo[cache->memo_next++ & 7];
                memo->instructions = instructions; memo->count = count;
                memo->packed_mask = packed_mask; memo->has_lighting = lighting != NULL;
                memo->bone_gather_enabled = cache->bone_gather_enabled;
                if (lighting) memo->lighting = *lighting;
                memo->entry = cache->vertex[i];
            }
            return cache->last_vertex = cache->vertex[i];
        }
    return NULL;
}

static struct mac_shader_source_entry *find_pixel(
    struct mac_shader_source_cache *cache,
    const struct nv2a_pixel_shader_key *key)
{
    unsigned long i, hash;
    struct mac_shader_source_entry *last = cache->last_pixel;
    if (last && !last->vertex && memcmp(&last->pixel_key, key, sizeof(*key)) == 0)
        return last;
    hash = key_hash(key, sizeof(*key), 0);
    for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i)
        if (cache->pixel[i] && cache->pixel[i]->hash == hash && !cache->pixel[i]->vertex &&
            memcmp(&cache->pixel[i]->pixel_key, key, sizeof(*key)) == 0)
            return cache->last_pixel = cache->pixel[i];
    return NULL;
}

static struct mac_shader_source_entry *insert_entry(
    struct mac_shader_source_cache *cache, int vertex, const DWORD *instructions,
    unsigned long count, unsigned long packed_mask,
    const struct nv2a_vertex_lighting *lighting,
    const struct nv2a_pixel_shader_key *pixel_key, const char *source)
{
    struct mac_shader_source_entry *entry;
    size_t source_size, instruction_size = 0, total;
    unsigned long i;
    source_size = strlen(source) + 1;
    if (vertex) {
        if (count > (size_t)-1 / (4 * sizeof(*instructions))) return NULL;
        instruction_size = (size_t)count * 4 * sizeof(*instructions);
    }
    if (instruction_size > (size_t)-1 - sizeof(*entry)) return NULL;
    if (source_size > (size_t)-1 - sizeof(*entry) - instruction_size) return NULL;
    total = sizeof(*entry) + instruction_size + source_size;
    if (total > MAC_SHADER_SOURCE_CACHE_BYTES) return NULL;
    while (cache->bytes > MAC_SHADER_SOURCE_CACHE_BYTES - total) {
        size_t before = cache->bytes; evict_one(cache, 0);
        if (before == cache->bytes) return NULL;
    }
    for (;;) {
        int free_slot = 0;
        struct mac_shader_source_entry **slots = vertex ? cache->vertex : cache->pixel;
        for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i)
            if (!slots[i]) { free_slot = 1; break; }
        if (free_slot) break;
        {
            struct mac_shader_source_entry *oldest = NULL;
            for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i)
                if (slots[i] && !slots[i]->references &&
                    (!oldest || slots[i]->used < oldest->used)) oldest = slots[i];
            if (!oldest) return NULL;
            free_entry(cache, oldest);
        }
    }
    entry = (struct mac_shader_source_entry *)calloc(1, sizeof(*entry));
    if (!entry) return NULL;
    if (instruction_size) {
        entry->instructions = (DWORD *)malloc(instruction_size);
        if (!entry->instructions) { free(entry); return NULL; }
        memcpy(entry->instructions, instructions, instruction_size);
    }
    entry->source = (char *)malloc(source_size);
    if (!entry->source) { free(entry->instructions); free(entry); return NULL; }
    memcpy(entry->source, source, source_size);
    entry->vertex = vertex; entry->instruction_count = count;
    if (cache->identity_epoch && cache->next_entry_identity) {
        entry->identity = cache->next_entry_identity;
        if (cache->next_entry_identity == ULONG_MAX)
            cache->next_entry_identity = 0;
        else ++cache->next_entry_identity;
    }
    entry->bone_gather_enabled = cache->bone_gather_enabled;
    entry->packed_mask = packed_mask; entry->has_lighting = lighting != NULL;
    if (lighting) memcpy(&entry->lighting, lighting, sizeof(*lighting));
    if (pixel_key) memcpy(&entry->pixel_key, pixel_key, sizeof(*pixel_key));
    entry->hash = vertex ? key_hash(instructions, instruction_size, count) :
        key_hash(&entry->pixel_key, sizeof(entry->pixel_key), 0);
    entry->bytes = total; entry->used = next_use(cache);
    for (i = 0; i < MAC_SHADER_SOURCE_CACHE_ENTRIES; ++i) {
        struct mac_shader_source_entry **slots = vertex ? cache->vertex : cache->pixel;
        if (!slots[i]) { slots[i] = entry; break; }
    }
    cache->bytes += total;
    return entry;
}

static void pin_entry(struct mac_shader_source_cache *cache,
    struct mac_shader_source_entry *entry)
{ ++entry->references; entry->used = next_use(cache); }

int mac_shader_source_cache_get(struct mac_shader_source_cache *cache,
    const DWORD *instructions, unsigned long instruction_count,
    unsigned long packed_mask, const struct nv2a_vertex_lighting *lighting,
    const struct nv2a_pixel_shader_key *pixel_key,
    struct mac_shader_source_result *result)
{
    struct mac_shader_source_entry *entry;
    char *modern = NULL, *legacy = NULL;
    char error[160];
    unsigned long mask = 0;
    unsigned long stage;
    struct nv2a_pixel_shader_key source_key;
    if (!result) return 0;
    memset(result, 0, sizeof(*result)); error[0] = 0;
    if (!cache || !instructions || !instruction_count || !pixel_key) {
        result->failure = MAC_SHADER_SOURCE_VERTEX_GENERATE_FAILED;
        return 0;
    }
    if (instruction_count > (size_t)-1 / (4 * sizeof(*instructions))) {
        result->failure = MAC_SHADER_SOURCE_VERTEX_GENERATE_FAILED;
        return 0;
    }
    entry = find_vertex(cache, instructions, instruction_count, packed_mask, lighting);
    if (entry) {
        ++cache->vertex_hits; pin_entry(cache, entry); result->vertex_pin = entry;
        result->vertex_source = entry->source;
        result->input_mask = entry->input_mask;
        result->bone_gather = entry->bone_gather;
        result->identity.vertex_entry = entry->identity;
    } else {
        ++cache->vertex_misses;
        modern = nv2a_vertex_shader_to_glsl(instructions, instruction_count,
            packed_mask, lighting);
        if (!modern) { result->failure = MAC_SHADER_SOURCE_VERTEX_GENERATE_FAILED; return 0; }
        if (cache->bone_gather_enabled && !packed_mask) {
            char *gathered = NULL;
            int recognized = mac_bone_gather_rewrite(modern, &result->bone_gather,
                &gathered, error, sizeof(error));
            if (recognized < 0) {
                free(modern);
                result->failure = MAC_SHADER_SOURCE_VERTEX_GENERATE_FAILED;
                strncpy(result->error, error, sizeof(result->error) - 1);
                return 0;
            }
            if (recognized > 0) { free(modern); modern = gathered; }
        }
        if (!mac_glsl120_vertex(modern, &legacy, &mask, error, sizeof(error))) {
            result->failure = MAC_SHADER_SOURCE_VERTEX_COMPAT_FAILED;
            strncpy(result->error, error, sizeof(result->error) - 1);
            result->error[sizeof(result->error) - 1] = 0;
            result->modern_vertex_source = modern;
            return 0;
        }
        free(modern);
        entry = insert_entry(cache, 1, instructions, instruction_count,
            packed_mask, lighting, NULL, legacy);
        if (entry) {
            entry->input_mask = mask;
            entry->bone_gather = result->bone_gather;
            pin_entry(cache, entry); result->vertex_pin = entry;
            result->vertex_source = entry->source;
            free(legacy);
            result->identity.vertex_entry = entry->identity;
        } else {
            result->owned_vertex_source = legacy;
            result->vertex_source = legacy;
        }
        result->input_mask = mask;
    }
    /* These colors are uploaded as uniforms on every draw. The shared pixel
       generator reads the combiner operations, but never these color words. */
    memcpy(&source_key, pixel_key, sizeof(source_key));
    for (stage = 0; stage < 8; ++stage) {
        source_key.combiner_state[D3DRS_PSCONSTANT0_0 + stage] = 0;
        source_key.combiner_state[D3DRS_PSCONSTANT1_0 + stage] = 0;
    }
    source_key.combiner_state[D3DRS_PSFINALCOMBINERCONSTANT0] = 0;
    source_key.combiner_state[D3DRS_PSFINALCOMBINERCONSTANT1] = 0;
    entry = find_pixel(cache, &source_key);
    if (entry) {
        ++cache->pixel_hits; pin_entry(cache, entry); result->pixel_pin = entry;
        result->pixel_source = entry->source;
        result->identity.pixel_entry = entry->identity;
    } else {
        ++cache->pixel_misses;
        modern = nv2a_pixel_shader_to_glsl(pixel_key);
        if (!modern) {
            result->failure = MAC_SHADER_SOURCE_PIXEL_GENERATE_FAILED;
            return 0;
        }
        if (!mac_glsl120_pixel(modern, &legacy, error, sizeof(error))) {
            result->failure = MAC_SHADER_SOURCE_PIXEL_COMPAT_FAILED;
            strncpy(result->error, error, sizeof(result->error) - 1);
            result->error[sizeof(result->error) - 1] = 0;
            free(modern);
            return 0;
        }
        free(modern);
        entry = insert_entry(cache, 0, NULL, 0, 0, NULL, &source_key, legacy);
        if (entry) {
            pin_entry(cache, entry); result->pixel_pin = entry;
            result->pixel_source = entry->source; free(legacy);
            result->identity.pixel_entry = entry->identity;
        } else {
            result->owned_pixel_source = legacy; result->pixel_source = legacy;
        }
    }
    if (!result->vertex_pin || !result->pixel_pin ||
        !result->identity.vertex_entry || !result->identity.pixel_entry)
        memset(&result->identity, 0, sizeof(result->identity));
    else
        result->identity.cache_epoch = cache->identity_epoch;
    return 1;
}
