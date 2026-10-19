/* Bounded, context-owned cache for exact translated GLSL source pairs. */
#include "mac_gl_program_cache.h"
#include "mac_load_timing.h" /* port: load timing */
#include <OpenGL/OpenGL.h>
#include <OpenGL/glext.h>
#undef malloc
#undef calloc
#undef free
#undef memcpy
#undef memset
#undef strlen
#undef strcmp
#undef sprintf
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAC_GL_DIAGNOSTIC_LIMIT 64
#define MAC_GL_DIAGNOSTIC_LOG_BYTES 1024
#define MAC_GL_PROGRAM_IDENTITY_ALIAS_COUNT 8

struct mac_gl_program_identity_alias {
    struct mac_shader_source_identity identity;
    uintptr_t vertex_source_hint, fragment_source_hint;
    int valid;
};

struct mac_gl_program_entry {
    char *vertex_source;
    char *fragment_source;
    unsigned long attribute_mask;
    unsigned long active_attribute_mask;
    int active_attribute_locations_valid;
    unsigned long last_used;
    unsigned long hash;
    GLuint program;
    uintptr_t vertex_source_hint, fragment_source_hint;
    struct mac_gl_program_identity_alias identity_aliases[
        MAC_GL_PROGRAM_IDENTITY_ALIAS_COUNT];
    unsigned long identity_alias_count;
    unsigned long next_identity_alias;
    unsigned long uniform_count;
    char uniform_names[MAC_GL_PROGRAM_CACHE_UNIFORM_COUNT]
        [MAC_GL_PROGRAM_CACHE_UNIFORM_NAME_BYTES];
    GLint uniform_locations[MAC_GL_PROGRAM_CACHE_UNIFORM_COUNT];
};

static int mac_gl_cache_identity_valid(
    const struct mac_shader_source_identity *identity)
{
    return identity && identity->cache_epoch && identity->vertex_entry &&
        identity->pixel_entry;
}

static int mac_gl_cache_identity_equal(
    const struct mac_shader_source_identity *left,
    const struct mac_shader_source_identity *right)
{
    return left->cache_epoch == right->cache_epoch &&
        left->vertex_entry == right->vertex_entry &&
        left->pixel_entry == right->pixel_entry;
}

/* Retain only issuer tokens and the exact immutable source pointers that
   accompanied them. The aliases own no source-cache storage and die with
   this program entry. */
static void mac_gl_cache_identity_alias_add(
    struct mac_gl_program_entry *entry,
    const struct mac_shader_source_identity *identity,
    const char *vertex_source, const char *fragment_source)
{
    unsigned long i, slot;
    if (!entry || !mac_gl_cache_identity_valid(identity) ||
        !vertex_source || !fragment_source) return;
    for (i = 0; i < entry->identity_alias_count; ++i) {
        struct mac_gl_program_identity_alias *alias =
            &entry->identity_aliases[i];
        if (alias->valid &&
            mac_gl_cache_identity_equal(&alias->identity, identity) &&
            alias->vertex_source_hint == (uintptr_t)vertex_source &&
            alias->fragment_source_hint == (uintptr_t)fragment_source)
            return;
    }
    if (entry->identity_alias_count < MAC_GL_PROGRAM_IDENTITY_ALIAS_COUNT) {
        slot = entry->identity_alias_count++;
    } else {
        slot = entry->next_identity_alias++ %
            MAC_GL_PROGRAM_IDENTITY_ALIAS_COUNT;
    }
    entry->identity_aliases[slot].identity = *identity;
    entry->identity_aliases[slot].vertex_source_hint = (uintptr_t)vertex_source;
    entry->identity_aliases[slot].fragment_source_hint = (uintptr_t)fragment_source;
    entry->identity_aliases[slot].valid = 1;
}

static void mac_gl_cache_error(char *error, size_t size, const char *message)
{
    size_t length;
    if (!error || !size) return;
    length = strlen(message);
    if (length >= size) length = size - 1;
    memcpy(error, message, length);
    error[length] = '\0';
}

static void mac_gl_cache_diagnostics_init(struct mac_gl_program_cache *cache)
{
    const char *enabled;
    const char *vendor, *renderer, *version, *shading_version, *cursor;
    GLint texture_size = 0, texture_units = 0;
    GLint image_units = -1, combined_units = -1, vertex_attribs = -1;
    GLint vertex_uniforms = -1, fragment_uniforms = -1, varying_floats = -1;
    unsigned major = 0;
    if (!cache || cache->diagnostics_initialized) return;
    cache->diagnostics_initialized = 1;
    enabled = getenv("HALO_MAC_GL_DIAGNOSTICS");
    cache->diagnostics_enabled = enabled && enabled[0] == '1' && enabled[1] == '\0';
    if (!cache->diagnostics_enabled) return;

    vendor = (const char *)glGetString(GL_VENDOR);
    renderer = (const char *)glGetString(GL_RENDERER);
    version = (const char *)glGetString(GL_VERSION);
    shading_version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (version) {
        cursor = version;
        while (*cursor >= '0' && *cursor <= '9') {
            major = major * 10 + (unsigned)(*cursor++ - '0');
            if (major > 99) break;
        }
    }
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &texture_units);
    if (major >= 2) {
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &image_units);
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &combined_units);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &vertex_attribs);
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &vertex_uniforms);
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &fragment_uniforms);
        glGetIntegerv(GL_MAX_VARYING_FLOATS, &varying_floats);
    }
    fprintf(stderr,
        "native GL diagnostics: vendor=%s renderer=%s version=%s glsl=%s limits={texture=%d units=%d image_units=%d combined_units=%d vertex_attribs=%d vertex_uniform_components=%d fragment_uniform_components=%d varying_floats=%d}\n",
        vendor ? vendor : "unknown", renderer ? renderer : "unknown",
        version ? version : "unknown", shading_version ? shading_version : "unknown",
        (int)texture_size, (int)texture_units, (int)image_units,
        (int)combined_units, (int)vertex_attribs, (int)vertex_uniforms,
        (int)fragment_uniforms, (int)varying_floats);
    ++cache->diagnostic_messages;
}

static void mac_gl_cache_success_log(struct mac_gl_program_cache *cache,
    GLenum kind, GLuint object, unsigned long hash, int is_program)
{
    GLint log_length = 0;
    GLsizei written = 0;
    char log[MAC_GL_DIAGNOSTIC_LOG_BYTES];
    const char *kind_name;
    if (!cache || !cache->diagnostics_enabled ||
        cache->diagnostic_messages >= MAC_GL_DIAGNOSTIC_LIMIT) return;
    if (!is_program) {
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length <= 1) return;
        log[0] = '\0';
        glGetShaderInfoLog(object, (GLsizei)sizeof(log), &written, log);
        kind_name = kind == GL_VERTEX_SHADER ? "vertex shader" : "fragment shader";
    } else {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length <= 1) return;
        log[0] = '\0';
        glGetProgramInfoLog(object, (GLsizei)sizeof(log), &written, log);
        kind_name = "program";
    }
    if (written < 0) written = 0;
    if ((size_t)written >= sizeof(log)) written = (GLsizei)sizeof(log) - 1;
    log[written] = '\0';
    if (written > 0) {
        fprintf(stderr, "native GL successful %s log (source=%08lx): %s\n",
            kind_name, hash, log);
        ++cache->diagnostic_messages;
    }
}

static unsigned long mac_gl_cache_hash(const char *vertex, const char *fragment,
    unsigned long attribute_mask)
{
    const unsigned char *p;
    unsigned long hash = 2166136261UL;
    unsigned long i;
    for (p = (const unsigned char *)vertex; *p; ++p) {
        hash ^= *p;
        hash *= 16777619UL;
    }
    hash ^= 0xffUL;
    hash *= 16777619UL;
    for (p = (const unsigned char *)fragment; *p; ++p) {
        hash ^= *p;
        hash *= 16777619UL;
    }
    hash ^= 0xfeUL;
    hash *= 16777619UL;
    for (i = 0; i < sizeof(attribute_mask); ++i) {
        hash ^= (unsigned char)(attribute_mask >> (i * 8));
        hash *= 16777619UL;
    }
    return hash;
}

static char *mac_gl_cache_strdup(const char *source)
{
    size_t size = strlen(source) + 1;
    char *copy = (char *)malloc(size);
    if (copy) memcpy(copy, source, size);
    return copy;
}

static void mac_gl_cache_entry_free(struct mac_gl_program_entry *entry)
{
    if (!entry) return;
    if (entry->program) glDeleteProgram(entry->program);
    free(entry->vertex_source);
    free(entry->fragment_source);
    free(entry);
}

static void mac_gl_cache_advance_clock(struct mac_gl_program_cache *cache)
{
    unsigned long i;
    if (cache->clock != ULONG_MAX) {
        ++cache->clock;
        return;
    }
    /* Preserve LRU order if the 32-bit counter ever wraps. */
    cache->clock = 0;
    for (i = 0; i < MAC_GL_PROGRAM_CACHE_CAPACITY; ++i) {
        struct mac_gl_program_entry *entry =
            (struct mac_gl_program_entry *)cache->entries[i];
        if (entry && entry->last_used > cache->clock)
            cache->clock = entry->last_used;
    }
    if (cache->clock == ULONG_MAX) {
        for (i = 0; i < MAC_GL_PROGRAM_CACHE_CAPACITY; ++i) {
            struct mac_gl_program_entry *entry =
                (struct mac_gl_program_entry *)cache->entries[i];
            if (entry) entry->last_used >>= 1;
        }
        cache->clock >>= 1;
    }
    ++cache->clock;
}

static GLuint mac_gl_cache_compile(GLenum kind, const char *source,
    char *error, size_t error_size, struct mac_gl_program_cache *cache,
    unsigned long hash)
{
    GLuint shader;
    GLint compiled = GL_FALSE;
    GLint source_length;
    size_t length = strlen(source);
    double load_begin = mac_load_begin();
    if (length > INT_MAX) {
        mac_gl_cache_error(error, error_size, "shader source is too large");
        return 0;
    }
    source_length = (GLint)length;
    shader = glCreateShader(kind);
    if (!shader) {
        mac_gl_cache_error(error, error_size, "OpenGL shader allocation failed");
        return 0;
    }
    glShaderSource(shader, 1, &source, &source_length);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    mac_load_end(MAC_LOAD_SLOT_PROGRAMS, load_begin);
    if (compiled != GL_TRUE) {
        char log[1024];
        GLsizei written = 0;
        log[0] = '\0';
        glGetShaderInfoLog(shader, (GLsizei)sizeof(log), &written, log);
        if (written < 0) written = 0;
        if ((size_t)written >= sizeof(log)) written = (GLsizei)sizeof(log) - 1;
        log[written] = '\0';
        mac_gl_cache_error(error, error_size, log[0] ? log : "OpenGL shader compile failed");
        glDeleteShader(shader);
        return 0;
    }
    mac_gl_cache_success_log(cache, kind, shader, hash, 0);
    return shader;
}

void mac_gl_program_cache_init(struct mac_gl_program_cache *cache)
{
    if (cache) memset(cache, 0, sizeof(*cache));
}

static int mac_gl_program_cache_get_internal(struct mac_gl_program_cache *cache,
    const char *vertex_source, const char *fragment_source,
    unsigned long attribute_mask,
    const struct mac_shader_source_identity *identity,
    GLuint *program, char *error,
    size_t error_size)
{
    unsigned long hash, i, j, slot, first_empty = MAC_GL_PROGRAM_CACHE_CAPACITY;
    unsigned long oldest_slot = MAC_GL_PROGRAM_CACHE_CAPACITY;
    unsigned long oldest = ULONG_MAX;
    struct mac_gl_program_entry *entry, *replacement;
    GLuint vertex_shader = 0, fragment_shader = 0, linked_program = 0;
    GLint linked = GL_FALSE;
    if (program) *program = 0;
    if (error && error_size) error[0] = '\0';
    if (!cache || !vertex_source || !fragment_source || !program) {
        mac_gl_cache_error(error, error_size, "invalid program-cache arguments");
        return 0;
    }
    if (attribute_mask & ~((1UL << MAC_GL_PROGRAM_CACHE_ATTRIBUTE_COUNT) - 1UL)) {
        mac_gl_cache_error(error, error_size, "attribute mask exceeds native vertex locations");
        return 0;
    }
    mac_gl_cache_diagnostics_init(cache);
    /* Issuer tokens plus the exact pinned source pointers may skip content
       comparison. Address-only hints always compare owned strings, so generic
       mutable callers and allocator address reuse cannot select stale code. */
    for (i = 0; i < MAC_GL_PROGRAM_CACHE_CAPACITY; ++i) {
        entry = (struct mac_gl_program_entry *)cache->entries[i];
        if (entry && mac_gl_cache_identity_valid(identity) &&
            entry->attribute_mask == attribute_mask) {
            for (j = 0; j < entry->identity_alias_count; ++j) {
                struct mac_gl_program_identity_alias *alias =
                    &entry->identity_aliases[j];
                if (alias->valid &&
                    mac_gl_cache_identity_equal(&alias->identity, identity) &&
                    alias->vertex_source_hint == (uintptr_t)vertex_source &&
                    alias->fragment_source_hint == (uintptr_t)fragment_source) {
                    mac_gl_cache_advance_clock(cache);
                    entry->last_used = cache->clock;
                    ++cache->hits;
                    ++cache->identity_hits;
                    *program = entry->program;
                    return 1;
                }
            }
        }
        if (entry && entry->attribute_mask == attribute_mask &&
            entry->vertex_source_hint == (uintptr_t)vertex_source &&
            entry->fragment_source_hint == (uintptr_t)fragment_source &&
            strcmp(entry->vertex_source, vertex_source) == 0 &&
            strcmp(entry->fragment_source, fragment_source) == 0) {
            mac_gl_cache_identity_alias_add(entry, identity,
                vertex_source, fragment_source);
            mac_gl_cache_advance_clock(cache);
            entry->last_used = cache->clock;
            ++cache->hits;
            *program = entry->program;
            return 1;
        }
    }
    hash = mac_gl_cache_hash(vertex_source, fragment_source, attribute_mask);
    for (i = 0; i < MAC_GL_PROGRAM_CACHE_CAPACITY; ++i) {
        entry = (struct mac_gl_program_entry *)cache->entries[i];
        if (!entry) {
            if (first_empty == MAC_GL_PROGRAM_CACHE_CAPACITY) first_empty = i;
            continue;
        }
        if (entry->hash == hash && entry->attribute_mask == attribute_mask &&
            strcmp(entry->vertex_source, vertex_source) == 0 &&
            strcmp(entry->fragment_source, fragment_source) == 0) {
            entry->vertex_source_hint = (uintptr_t)vertex_source;
            entry->fragment_source_hint = (uintptr_t)fragment_source;
            mac_gl_cache_identity_alias_add(entry, identity,
                vertex_source, fragment_source);
            mac_gl_cache_advance_clock(cache);
            entry->last_used = cache->clock;
            ++cache->hits;
            *program = entry->program;
            return 1;
        }
        if (entry->last_used < oldest) {
            oldest = entry->last_used;
            oldest_slot = i;
        }
    }
    slot = first_empty != MAC_GL_PROGRAM_CACHE_CAPACITY ? first_empty : oldest_slot;
    ++cache->misses;
    replacement = (struct mac_gl_program_entry *)calloc(1, sizeof(*replacement));
    if (!replacement) {
        mac_gl_cache_error(error, error_size, "program-cache entry allocation failed");
        return 0;
    }
    replacement->vertex_source = mac_gl_cache_strdup(vertex_source);
    replacement->fragment_source = mac_gl_cache_strdup(fragment_source);
    if (!replacement->vertex_source || !replacement->fragment_source) {
        mac_gl_cache_error(error, error_size, "program-cache source allocation failed");
        mac_gl_cache_entry_free(replacement);
        return 0;
    }
    vertex_shader = mac_gl_cache_compile(GL_VERTEX_SHADER, vertex_source, error,
        error_size, cache, hash);
    if (!vertex_shader) goto failed;
    fragment_shader = mac_gl_cache_compile(GL_FRAGMENT_SHADER, fragment_source,
        error, error_size, cache, hash);
    if (!fragment_shader) goto failed;
    linked_program = glCreateProgram();
    if (!linked_program) {
        mac_gl_cache_error(error, error_size, "OpenGL program allocation failed");
        goto failed;
    }
    glAttachShader(linked_program, vertex_shader);
    glAttachShader(linked_program, fragment_shader);
    for (i = 0; i < MAC_GL_PROGRAM_CACHE_ATTRIBUTE_COUNT; ++i) {
        if (attribute_mask & (1UL << i)) {
            char name[24];
            sprintf(name, "v%lu_in", i);
            glBindAttribLocation(linked_program, (GLuint)i, name);
        }
    }
    {
        double link_begin = mac_load_begin();
        glLinkProgram(linked_program);
        glGetProgramiv(linked_program, GL_LINK_STATUS, &linked);
        mac_load_end(MAC_LOAD_SLOT_PROGRAMS, link_begin);
    }
    if (linked != GL_TRUE) {
        char log[1024];
        GLsizei written = 0;
        log[0] = '\0';
        glGetProgramInfoLog(linked_program, (GLsizei)sizeof(log), &written, log);
        if (written < 0) written = 0;
        if ((size_t)written >= sizeof(log)) written = (GLsizei)sizeof(log) - 1;
        log[written] = '\0';
        mac_gl_cache_error(error, error_size, log[0] ? log : "OpenGL program link failed");
        goto failed;
    }
    mac_gl_cache_success_log(cache, 0, linked_program, hash, 1);
    /* Query linked inputs once: source declarations include unused registers.
       Their fixed locations retain the expanded D3D attribute values. */
    replacement->active_attribute_locations_valid = 1;
    for (i = 0; i < MAC_GL_PROGRAM_CACHE_ATTRIBUTE_COUNT; ++i) {
        char name[24];
        GLint location;
        sprintf(name, "v%lu_in", i);
        location = glGetAttribLocation(linked_program, name);
        if (location < 0) continue;
        if (location != (GLint)i)
            replacement->active_attribute_locations_valid = 0;
        replacement->active_attribute_mask |= 1UL << i;
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    vertex_shader = fragment_shader = 0;
    replacement->program = linked_program;
    linked_program = 0;
    replacement->attribute_mask = attribute_mask;
    replacement->vertex_source_hint = (uintptr_t)vertex_source;
    replacement->fragment_source_hint = (uintptr_t)fragment_source;
    mac_gl_cache_identity_alias_add(replacement, identity,
        vertex_source, fragment_source);
    replacement->hash = hash;
    mac_gl_cache_advance_clock(cache);
    replacement->last_used = cache->clock;
    if (slot == MAC_GL_PROGRAM_CACHE_CAPACITY) {
        mac_gl_cache_error(error, error_size, "program-cache replacement slot unavailable");
        goto failed;
    }
    if (cache->entries[slot]) {
        mac_gl_cache_entry_free((struct mac_gl_program_entry *)cache->entries[slot]);
        ++cache->evictions;
    } else ++cache->entry_count;
    cache->entries[slot] = replacement;
    *program = replacement->program;
    return 1;

failed:
    if (linked_program) glDeleteProgram(linked_program);
    if (vertex_shader) glDeleteShader(vertex_shader);
    if (fragment_shader) glDeleteShader(fragment_shader);
    mac_gl_cache_entry_free(replacement);
    return 0;
}

int mac_gl_program_cache_get(struct mac_gl_program_cache *cache,
    const char *vertex_source, const char *fragment_source,
    unsigned long attribute_mask, GLuint *program, char *error,
    size_t error_size)
{
    return mac_gl_program_cache_get_internal(cache, vertex_source,
        fragment_source, attribute_mask, NULL, program, error, error_size);
}

int mac_gl_program_cache_get_identified(struct mac_gl_program_cache *cache,
    const char *vertex_source, const char *fragment_source,
    unsigned long attribute_mask,
    const struct mac_shader_source_identity *identity, GLuint *program,
    char *error, size_t error_size)
{
    return mac_gl_program_cache_get_internal(cache, vertex_source,
        fragment_source, attribute_mask, identity, program, error, error_size);
}

void mac_gl_program_cache_get_stats(const struct mac_gl_program_cache *cache,
    struct mac_gl_program_cache_stats *stats)
{
    if (!stats) return;
    memset(stats, 0, sizeof(*stats));
    if (!cache) return;
    stats->entries = cache->entry_count;
    stats->hits = cache->hits;
    stats->identity_hits = cache->identity_hits;
    stats->misses = cache->misses;
    stats->evictions = cache->evictions;
    stats->uniform_lookups = cache->uniform_lookups;
    stats->uniform_hits = cache->uniform_hits;
}

int mac_gl_program_cache_active_attributes(const struct mac_gl_program_cache *cache,
    GLuint program, unsigned long *mask)
{
    unsigned long i;
    if (mask) *mask = 0;
    if (!cache || !program || !mask) return 0;
    for (i = 0; i < MAC_GL_PROGRAM_CACHE_CAPACITY; ++i) {
        const struct mac_gl_program_entry *entry =
            (const struct mac_gl_program_entry *)cache->entries[i];
        if (entry && entry->program == program) {
            if (!entry->active_attribute_locations_valid) return 0;
            *mask = entry->active_attribute_mask;
            return 1;
        }
    }
    return 0;
}

GLint mac_gl_program_cache_uniform_location(struct mac_gl_program_cache *cache,
    GLuint program, const char *name)
{
    unsigned long i, j;
    size_t length;
    struct mac_gl_program_entry *entry;
    if (!cache || !program || !name || !name[0]) return -1;
    length = strlen(name);
    for (i = 0; i < MAC_GL_PROGRAM_CACHE_CAPACITY; ++i) {
        entry = (struct mac_gl_program_entry *)cache->entries[i];
        if (!entry || entry->program != program) continue;
        for (j = 0; j < entry->uniform_count; ++j) {
            if (strcmp(entry->uniform_names[j], name) == 0) {
                ++cache->uniform_hits;
                return entry->uniform_locations[j];
            }
        }
        ++cache->uniform_lookups;
        if (length >= MAC_GL_PROGRAM_CACHE_UNIFORM_NAME_BYTES)
            return glGetUniformLocation(program, name);
        {
            GLint location = glGetUniformLocation(program, name);
            if (entry->uniform_count < MAC_GL_PROGRAM_CACHE_UNIFORM_COUNT) {
                j = entry->uniform_count++;
                memcpy(entry->uniform_names[j], name, length + 1);
                entry->uniform_locations[j] = location;
            }
            return location;
        }
    }
    /* A live program the cache did not link (the renderer's ARB vertex
       programs pair with fragment-only programs it makes itself). */
    return glGetUniformLocation(program, name);
}

void mac_gl_program_cache_diagnostics_draw(struct mac_gl_program_cache *cache,
    unsigned long vertex_id, GLuint program, unsigned long vertex_count,
    unsigned long attribute_mask)
{
    unsigned long i;
    CGLContextObj context;
    GLint vertex_gpu = -1, fragment_gpu = -1;
    CGLError vertex_error, fragment_error;
    unsigned long active_attributes = 0;
    if (!cache || !cache->diagnostics_enabled || !program) return;
    for (i = 0; i < cache->diagnostic_draw_count; ++i)
        if (cache->diagnostic_draw_programs[i] == program) return;
    if (cache->diagnostic_draw_count >= 64) return;
    cache->diagnostic_draw_programs[cache->diagnostic_draw_count++] = program;
    context = CGLGetCurrentContext();
    if (!context) {
        fprintf(stderr,
            "native GL draw GPU status: vertex=%lu program=%lu count=%lu source_attributes=0x%lx active_attributes=unknown context=missing\n",
            vertex_id, (unsigned long)program, vertex_count, attribute_mask);
        return;
    }
    for (i = 0; i < MAC_GL_PROGRAM_CACHE_ATTRIBUTE_COUNT; ++i) {
        char name[16];
        sprintf(name, "v%lu_in", i);
        if (glGetAttribLocation(program, name) >= 0)
            active_attributes |= 1UL << i;
    }
    vertex_error = CGLGetParameter(context, kCGLCPGPUVertexProcessing, &vertex_gpu);
    fragment_error = CGLGetParameter(context, kCGLCPGPUFragmentProcessing, &fragment_gpu);
    fprintf(stderr,
        "native GL draw GPU status: vertex=%lu program=%lu count=%lu source_attributes=0x%lx active_attributes=0x%lx vertex_gpu=%d vertex_query=%d fragment_gpu=%d fragment_query=%d\n",
        vertex_id, (unsigned long)program, vertex_count, attribute_mask,
        active_attributes,
        (int)vertex_gpu, (int)vertex_error, (int)fragment_gpu,
        (int)fragment_error);
}

void mac_gl_program_cache_dispose(struct mac_gl_program_cache *cache)
{
    unsigned long i;
    if (!cache) return;
    for (i = 0; i < MAC_GL_PROGRAM_CACHE_CAPACITY; ++i) {
        mac_gl_cache_entry_free((struct mac_gl_program_entry *)cache->entries[i]);
        cache->entries[i] = NULL;
    }
    cache->clock = cache->entry_count = cache->hits = cache->identity_hits =
        cache->misses = cache->evictions = 0;
    cache->uniform_lookups = cache->uniform_hits = 0;
    cache->diagnostic_messages = 0;
    cache->diagnostics_initialized = 0;
    cache->diagnostics_enabled = 0;
    cache->diagnostic_draw_count = 0;
    memset(cache->diagnostic_draw_programs, 0,
        sizeof(cache->diagnostic_draw_programs));
}
