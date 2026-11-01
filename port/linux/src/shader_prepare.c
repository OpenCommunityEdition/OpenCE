/* Map-scoped shader recipes. Only observed pairs are retained; unused prepared
 * programs are released at the next map boundary. The normal renderer owns
 * programs transferred by shader_prepare_take. Recording performs no file I/O. */
#include "shader_prepare.h"
#include "xgpu.h"
#include "port_config.h"
#include "sdl_platform.h"
#include "posix.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HALO_GLES
#include <SDL3/SDL.h>
#define PREPARE_MAGIC 0x31525053u
#define PREPARE_COUNT 512
#define PREPARE_SOURCE 65536u
#define PREPARE_BYTES (16u * 1024u * 1024u)
#define PREPARE_COMPLETION 0x91B1

struct prepare_header { uint32_t magic, count; uint64_t map; };
struct prepare_pair {
    uint64_t vertex, fragment;
    uint32_t vertex_bytes, fragment_bytes;
};
struct prepare_entry {
    struct prepare_pair pair;
    char *vertex, *fragment;
    GLuint program;
    GLuint vertex_shader, fragment_shader;
    int linked;
    unsigned next;
};
static struct {
    struct prepare_entry entries[PREPARE_COUNT];
    unsigned buckets[PREPARE_COUNT];
    unsigned count, bytes;
    uint64_t map;
    int active;
    char path[512];
    PFNGLGETSHADERSOURCEPROC source;
    PFNGLDETACHSHADERPROC detach;
} prepare;

static unsigned prepare_bucket(uint64_t vertex, uint64_t fragment)
{
    return (unsigned)(vertex ^ fragment ^ (vertex >> 32) ^ (fragment >> 32)) % PREPARE_COUNT;
}

static struct prepare_entry *prepare_find(uint64_t vertex, uint64_t fragment)
{
    unsigned i;
    for (i = prepare.buckets[prepare_bucket(vertex, fragment)]; i; i = prepare.entries[i - 1].next) {
        struct prepare_entry *entry = &prepare.entries[i - 1];
        if (entry->pair.vertex == vertex && entry->pair.fragment == fragment) return entry;
    }
    return NULL;
}

static void prepare_index(unsigned index)
{
    struct prepare_entry *entry = &prepare.entries[index];
    unsigned bucket = prepare_bucket(entry->pair.vertex, entry->pair.fragment);
    entry->next = prepare.buckets[bucket];
    prepare.buckets[bucket] = index + 1;
}

static uint64_t prepare_hash(char const *text)
{
    uint64_t hash = 14695981039346656037ULL;
    while (*text) hash = (hash ^ (unsigned char)*text++) * 1099511628211ULL;
    return hash;
}

static void prepare_clear(void)
{
    unsigned i;
    for (i = 0; i < prepare.count; i++) {
        struct prepare_entry *entry = &prepare.entries[i];
        if (entry->program) glDeleteProgram(entry->program);
        free(entry->vertex);
        free(entry->fragment);
    }
    memset(prepare.entries, 0, sizeof(prepare.entries));
    memset(prepare.buckets, 0, sizeof(prepare.buckets));
    prepare.count = prepare.bytes = 0;
}

/* SDL's quit/exit-after paths can exit without unloading the map. Save only
 * CPU data at exit: a context need not still be current on that thread. */
static void prepare_save(void)
{
    FILE *file;
    struct prepare_header header;
    unsigned i;
    header.magic = PREPARE_MAGIC; header.count = prepare.count; header.map = prepare.map;
    if (prepare.active && header.count && (file = fopen(prepare.path, "wb")) != NULL) {
        int good = fwrite(&header, sizeof(header), 1, file) == 1;
        for (i = 0; good && i < prepare.count; i++) {
            struct prepare_entry *entry = &prepare.entries[i];
            good = fwrite(&entry->pair, sizeof(entry->pair), 1, file) == 1 &&
                fwrite(entry->vertex, entry->pair.vertex_bytes, 1, file) == 1 &&
                fwrite(entry->fragment, entry->pair.fragment_bytes, 1, file) == 1;
        }
        if (fclose(file)) good = 0;
        if (!good) platform_log("shader preparation: could not save all recipes");
    }
}

void shader_prepare_end(void)
{
    prepare_save();
    prepare_clear();
    memset(&prepare, 0, sizeof(prepare));
}

/* Validate every bounded record before submitting any source to the driver. */
static int prepare_read(FILE *file)
{
    struct prepare_header header;
    unsigned i;
    if (fread(&header, sizeof(header), 1, file) != 1 || header.magic != PREPARE_MAGIC ||
        header.map != prepare.map || header.count > PREPARE_COUNT) return 0;
    for (i = 0; i < header.count; i++) {
        struct prepare_entry *entry = &prepare.entries[prepare.count++];
        if (fread(&entry->pair, sizeof(entry->pair), 1, file) != 1 ||
            entry->pair.vertex_bytes < 2 || entry->pair.vertex_bytes > PREPARE_SOURCE ||
            entry->pair.fragment_bytes < 2 || entry->pair.fragment_bytes > PREPARE_SOURCE ||
            entry->pair.vertex_bytes + entry->pair.fragment_bytes > PREPARE_BYTES - prepare.bytes) return 0;
        entry->vertex = malloc(entry->pair.vertex_bytes);
        entry->fragment = malloc(entry->pair.fragment_bytes);
        if (!entry->vertex || !entry->fragment ||
            fread(entry->vertex, entry->pair.vertex_bytes, 1, file) != 1 ||
            fread(entry->fragment, entry->pair.fragment_bytes, 1, file) != 1 ||
            entry->vertex[entry->pair.vertex_bytes - 1] || entry->fragment[entry->pair.fragment_bytes - 1] ||
            strlen(entry->vertex) != entry->pair.vertex_bytes - 1 ||
            strlen(entry->fragment) != entry->pair.fragment_bytes - 1 ||
            prepare_hash(entry->vertex) != entry->pair.vertex ||
            prepare_hash(entry->fragment) != entry->pair.fragment) return 0;
        if (prepare_find(entry->pair.vertex, entry->pair.fragment)) return 0;
        prepare.bytes += entry->pair.vertex_bytes + entry->pair.fragment_bytes;
        prepare_index(i);
    }
    return fgetc(file) == EOF && !ferror(file);
}

void shader_prepare_begin(char const *map, shader_prepare_load_fn load, shader_prepare_store_fn store)
{
    static int exit_registered;
    FILE *file;
    unsigned i, loaded = 0, linked = 0;
    int parallel;
    Uint64 started;
    shader_prepare_end();
    if (!map || !*map || !config_boolean("display.shader_prepare")) return;
    prepare.source = (PFNGLGETSHADERSOURCEPROC)SDL_GL_GetProcAddress("glGetShaderSource");
    prepare.detach = (PFNGLDETACHSHADERPROC)SDL_GL_GetProcAddress("glDetachShader");
    if (!prepare.source || !prepare.detach) return;
    prepare.map = prepare_hash(map);
    if (snprintf(prepare.path, sizeof(prepare.path), "%s/shader_cache", platform_save_root()) >= sizeof(prepare.path)) return;
    posix_make_directory(prepare.path);
    if (snprintf(prepare.path, sizeof(prepare.path), "%s/shader_cache/recipes-%016llx.bin",
        platform_save_root(), (unsigned long long)prepare.map) >= sizeof(prepare.path)) return;
    file = fopen(prepare.path, "rb");
    if (file) {
        int valid = prepare_read(file);
        fclose(file);
        if (!valid) {
            platform_log("shader preparation: invalid recipe file ignored");
            prepare_clear();
        }
    }
    prepare.active = 1;
    if (!exit_registered) { atexit(prepare_save); exit_registered = 1; }
    started = SDL_GetTicksNS();
    parallel = SDL_GL_ExtensionSupported("GL_ARB_parallel_shader_compile") ||
        SDL_GL_ExtensionSupported("GL_KHR_parallel_shader_compile");
    /* Submit all missing pairs before waiting. Deleted attached shaders remain
     * alive through linking. A refused recipe falls back to normal translation. */
    for (i = 0; i < prepare.count; i++) {
        struct prepare_entry *entry = &prepare.entries[i];
        GLuint vertex, fragment;
        entry->program = load ? load(entry->pair.vertex, entry->pair.fragment) : 0;
        if (entry->program) { loaded++; continue; }
        vertex = glCreateShader(GL_VERTEX_SHADER);
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        entry->program = glCreateProgram();
        if (vertex && fragment && entry->program) {
            glShaderSource(vertex, 1, (char const *const *)&entry->vertex, NULL);
            glShaderSource(fragment, 1, (char const *const *)&entry->fragment, NULL);
            glCompileShader(vertex); glCompileShader(fragment);
            glAttachShader(entry->program, vertex); glAttachShader(entry->program, fragment);
            entry->vertex_shader = vertex; entry->fragment_shader = fragment;
            glProgramParameteri(entry->program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
            glLinkProgram(entry->program);
            entry->linked = 1;
            linked++;
        }
        else if (entry->program) {
            glDeleteProgram(entry->program);
            entry->program = 0;
        }
        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
    }
    for (i = 0; i < prepare.count; i++) {
        struct prepare_entry *entry = &prepare.entries[i];
        GLint ready = 0, status = 0;
        if (!entry->program) continue;
        if (parallel) {
            do {
                glGetProgramiv(entry->program, PREPARE_COMPLETION, &ready);
                if (!ready) SDL_Delay(1);
            } while (!ready);
        }
        glGetProgramiv(entry->program, GL_LINK_STATUS, &status);
        /* These temporary compiler objects must not acquire the transferred
         * program's context lifetime. They were marked for deletion above. */
        if (entry->vertex_shader) prepare.detach(entry->program, entry->vertex_shader);
        if (entry->fragment_shader) prepare.detach(entry->program, entry->fragment_shader);
        entry->vertex_shader = entry->fragment_shader = 0;
        if (!status) { glDeleteProgram(entry->program); entry->program = 0; }
        else if (store && entry->linked) store(entry->program, entry->pair.vertex, entry->pair.fragment);
    }
    platform_log("shader preparation: %u recipes, %u cached, %u linked in %.1f ms",
        prepare.count, loaded, linked, (SDL_GetTicksNS() - started) / 1e6);
}

unsigned int shader_prepare_take(shader_prepare_hash vertex, shader_prepare_hash fragment)
{
    struct prepare_entry *entry;
    if (!prepare.active) return 0;
    entry = prepare_find(vertex, fragment);
    if (entry) {
        GLuint program = entry->program;
        entry->program = 0;
        return program;
    }
    return 0;
}

void shader_prepare_record(unsigned int vertex, unsigned int fragment,
    shader_prepare_hash vertex_hash, shader_prepare_hash fragment_hash)
{
    struct prepare_entry entry;
    GLint vertex_bytes = 0, fragment_bytes = 0;
    if (!prepare.active || !vertex || !fragment ||
        prepare_find(vertex_hash, fragment_hash)) return;
    glGetShaderiv(vertex, GL_SHADER_SOURCE_LENGTH, &vertex_bytes);
    glGetShaderiv(fragment, GL_SHADER_SOURCE_LENGTH, &fragment_bytes);
    if (vertex_bytes < 2 || fragment_bytes < 2 || vertex_bytes > PREPARE_SOURCE || fragment_bytes > PREPARE_SOURCE ||
        (unsigned)(vertex_bytes + fragment_bytes) > PREPARE_BYTES - prepare.bytes || prepare.count == PREPARE_COUNT) return;
    memset(&entry, 0, sizeof(entry));
    entry.vertex = calloc(vertex_bytes, 1); entry.fragment = calloc(fragment_bytes, 1);
    if (!entry.vertex || !entry.fragment) { free(entry.vertex); free(entry.fragment); return; }
    prepare.source(vertex, vertex_bytes, NULL, entry.vertex);
    prepare.source(fragment, fragment_bytes, NULL, entry.fragment);
    entry.pair.vertex = prepare_hash(entry.vertex); entry.pair.fragment = prepare_hash(entry.fragment);
    if (entry.pair.vertex != vertex_hash || entry.pair.fragment != fragment_hash) {
        free(entry.vertex); free(entry.fragment); return;
    }
    entry.pair.vertex_bytes = (uint32_t)strlen(entry.vertex) + 1;
    entry.pair.fragment_bytes = (uint32_t)strlen(entry.fragment) + 1;
    prepare.bytes += entry.pair.vertex_bytes + entry.pair.fragment_bytes;
    prepare.entries[prepare.count++] = entry;
    prepare_index(prepare.count - 1);
}
#else
void shader_prepare_begin(char const *map, shader_prepare_load_fn load, shader_prepare_store_fn store) {(void)map;(void)load;(void)store;}
void shader_prepare_end(void) {}
void shader_prepare_record(unsigned int vertex, unsigned int fragment,
    shader_prepare_hash vertex_hash, shader_prepare_hash fragment_hash)
{(void)vertex;(void)fragment;(void)vertex_hash;(void)fragment_hash;}
unsigned int shader_prepare_take(shader_prepare_hash vertex, shader_prepare_hash fragment) {(void)vertex;(void)fragment;return 0;}
#endif
