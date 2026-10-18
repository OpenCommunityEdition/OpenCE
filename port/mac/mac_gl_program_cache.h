#ifndef MAC_GL_PROGRAM_CACHE_H
#define MAC_GL_PROGRAM_CACHE_H

#include <OpenGL/gl.h>
#include <stddef.h>
#include "mac_shader_identity.h"

#define MAC_GL_PROGRAM_CACHE_CAPACITY 128
#define MAC_GL_PROGRAM_CACHE_ATTRIBUTE_COUNT 16
#define MAC_GL_PROGRAM_CACHE_UNIFORM_COUNT 64
#define MAC_GL_PROGRAM_CACHE_UNIFORM_NAME_BYTES 64

/* Attribute bits map exactly as in engine_device_state.c: bit i binds v{i}_in
   to location i. A cache belongs to one current OpenGL context. Callers must
   serialize access on that context's thread and dispose it before destroying
   the context. Returned program names remain valid until eviction/disposal. */
struct mac_gl_program_cache {
    void *entries[MAC_GL_PROGRAM_CACHE_CAPACITY];
    unsigned long clock;
    unsigned long entry_count;
    unsigned long hits;
    unsigned long identity_hits;
    unsigned long misses;
    unsigned long evictions;
    unsigned long uniform_lookups;
    unsigned long uniform_hits;
    unsigned long diagnostic_messages;
    int diagnostics_initialized;
    int diagnostics_enabled;
    unsigned long diagnostic_draw_count;
    GLuint diagnostic_draw_programs[64];
};

struct mac_gl_program_cache_stats {
    unsigned long entries;
    unsigned long hits;
    unsigned long identity_hits;
    unsigned long misses;
    unsigned long evictions;
    unsigned long uniform_lookups;
    unsigned long uniform_hits;
};

void mac_gl_program_cache_init(struct mac_gl_program_cache *cache);
int mac_gl_program_cache_get(struct mac_gl_program_cache *cache,
    const char *vertex_source, const char *fragment_source,
    unsigned long attribute_mask, GLuint *program, char *error,
    size_t error_size);
/* Fast path for immutable, cache-owned strings. The token must come from a
   pinned mac_shader_source_result and accompany those exact source pointers.
   Invalid tokens transparently use the ordinary content-checked path. */
int mac_gl_program_cache_get_identified(struct mac_gl_program_cache *cache,
    const char *vertex_source, const char *fragment_source,
    unsigned long attribute_mask,
    const struct mac_shader_source_identity *identity, GLuint *program,
    char *error, size_t error_size);
void mac_gl_program_cache_diagnostics_draw(struct mac_gl_program_cache *cache,
    unsigned long vertex_id, GLuint program, unsigned long vertex_count,
    unsigned long attribute_mask);
/* Zero is valid. Refuses unknown programs or inputs without fixed locations. */
int mac_gl_program_cache_active_attributes(const struct mac_gl_program_cache *cache,
    GLuint program, unsigned long *mask);
/* Locations, including absent-uniform -1, are cached with the live program.
   Overlong names are looked up but not cached; unknown programs and invalid
   inputs return -1. */
GLint mac_gl_program_cache_uniform_location(struct mac_gl_program_cache *cache,
    GLuint program, const char *name);
void mac_gl_program_cache_get_stats(const struct mac_gl_program_cache *cache,
    struct mac_gl_program_cache_stats *stats);
void mac_gl_program_cache_dispose(struct mac_gl_program_cache *cache);

#endif
