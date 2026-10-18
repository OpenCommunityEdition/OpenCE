#ifndef HALO_MAC_GPU_TEXTURES_H
#define HALO_MAC_GPU_TEXTURES_H

#include "engine_device_state.h"
#include <OpenGL/gl.h>

/* Per-draw texture scope. Retail bitmap storage may be retained by the bounded
   GPU cache; other uploaded resources remain temporary. Every touched binding,
   unpack setting, and sampler mutation is restored by end(). */
#define MAC_GPU_TEXTURE_SCOPE_CACHE_ENTRIES 4
struct mac_gpu_texture_scope {
    GLint previous_active_texture;
    GLint previous_unpack_alignment;
    GLint previous_pack_alignment;
    GLint previous_bindings[D3DTSS_MAXSTAGES][3];
    GLfloat previous_lod_bias[D3DTSS_MAXSTAGES];
    unsigned changed_stages;
    GLuint owned_textures[D3DTSS_MAXSTAGES];
    unsigned owned_count;
    size_t uploaded_bytes;
    unsigned cache_entry_count;
    unsigned cache_entries[MAC_GPU_TEXTURE_SCOPE_CACHE_ENTRIES];
    unsigned borrowed_count;
    struct {
        GLuint name;
        GLenum target;
        GLint wrap_s, wrap_t, wrap_r, min_filter, mag_filter, base_level, max_level;
        GLfloat min_lod, border[4];
        DWORD source_state[D3DTSS_MAX];
    } borrowed[4];
    int active;
};

#define MAC_GPU_TEXTURE_SAMPLER_2D 1
#define MAC_GPU_TEXTURE_SAMPLER_3D 2
#define MAC_GPU_TEXTURE_SAMPLER_CUBE 3

/* Optional resolver for persistent native GL textures, such as the shared
   high-resolution font atlas. Return 0 when resource is not owned; return 1
   with a borrowed GL name/target/level count and texture scale; return -1
   when the resource is recognized but cannot be supplied. The binder saves
   and restores sampler state and never deletes a resolved name. A resolver
   must not change GL state; its name must already be a live texture with the
   reported target. Calls and registration are serialized on the owning GL
   context thread. */
typedef int (*mac_gpu_texture_borrowed_resolver)(void *context,
    const void *resource, unsigned char sampler, GLuint *name, GLenum *target,
    unsigned *levels, float texture_scale[4], const char **error);

int mac_gpu_textures_set_borrowed_resolver(
    mac_gpu_texture_borrowed_resolver resolver, void *context);

/* port: the high-res HUD's textures (display.high_res_hud; mac_hud_hires.h,
   whose provider a build with them installs): drawn by the texture cache of the
   Xbox maps in place of the bitmaps they stand for */
struct mac_gpu_hud_provider;
void mac_gpu_textures_hud_provider(const struct mac_gpu_hud_provider *provider);

/* Whether a texture's coordinates are in texels (linear) or normalized (swizzled). */
int mac_gpu_textures_resource_is_linear(const void *resource);

int mac_gpu_textures_bind(struct mac_gpu_texture_scope *scope,
    const void *textures[D3DTSS_MAXSTAGES], DWORD texture_modes,
    unsigned char sampler_type[D3DTSS_MAXSTAGES],
    float texture_scale[D3DTSS_MAXSTAGES][4], const char **error);
void mac_gpu_textures_end(struct mac_gpu_texture_scope *scope);
/* (HALO_MAC_FF_COMBINERS) Select a texture unit through the lean shadow; bind
   the combiner path's own white 2D texture to a unit; GL call counts. */
void mac_gpu_textures_ff_select(unsigned unit);
void mac_gpu_textures_ff_dummy(unsigned unit);
void mac_gpu_textures_call_counts(unsigned long long *binds, unsigned long long *active, unsigned long long *bias);
/* Skip saving and restoring stage bindings (the renderer's lean draw mode). */
void mac_gpu_textures_set_lean(int lean);
/* Forget the texture units' state recorded for lean draws (a frame's start). */
void mac_gpu_textures_shadow_reset(void);
/* A texture name is being deleted: forget any parameters recorded for it. */
void mac_gpu_textures_forget(GLuint name);

/* Owner generations make stale map entries unusable on the next bind. Device
   teardown calls dispose with the owning GL context current, before closing it.
   Both operations reject invalidation while a texture scope is active. */
int mac_gpu_textures_bitmap_cache_invalidate(void);
int mac_gpu_textures_dispose(void);
struct mac_gpu_texture_cache_stats {
    size_t bytes;
    size_t bytes_limit;
    unsigned entries;
    unsigned entry_limit;
    unsigned long hits;
    unsigned long misses;
    unsigned long evictions;
    unsigned long sampler_reuses;
};
void mac_gpu_textures_cache_get_stats(struct mac_gpu_texture_cache_stats *);

#endif
