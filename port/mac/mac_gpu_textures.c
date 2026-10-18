/* Per-draw native texture uploads for the four NV2A pixel-shader stages. */
#include "engine_device_state.h"
#include "native_bitmap_tags.h"
#include "engine_device_resources.h"
#include "native_resource_memory.h"
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include "mac_gpu_textures.h"
#include "mac_hud_hires.h"
#include "mac_flags.h"
#undef malloc
#undef calloc
#undef free
#undef memcpy
#undef memcmp
#undef memset
#undef strlen
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include "mac_xgpu_texture_decode_view.inc"
#include "../linux/src/texture_pack.h" /* port: texture packs (port/linux/src/texture_pack.c); weak, as the tools that link this file do not all link it */
extern int texture_pack_open(const char *, const char *, unsigned int, int, int, size_t, char *, size_t) __attribute__((weak));
extern int texture_pack_load(const char *, unsigned int, unsigned int, unsigned int, unsigned int, int, struct texture_pack_image *) __attribute__((weak));
extern void texture_pack_image_free(struct texture_pack_image *) __attribute__((weak));
extern const unsigned char *texture_pack_image_level(const struct texture_pack_image *, unsigned int, unsigned int, unsigned int *, unsigned int *, size_t *) __attribute__((weak));
extern void texture_pack_reset(void) __attribute__((weak));
extern size_t texture_pack_level_bytes(unsigned int, unsigned int, unsigned int) __attribute__((weak));
extern void texture_pack_dds_header(unsigned char *, unsigned int, unsigned int, unsigned int, unsigned int, int) __attribute__((weak));
extern void *texture_pack_dump_open(const char *, const char *, unsigned int) __attribute__((weak));
extern int mac_engine_files_data_root(char *, size_t) __attribute__((weak));
extern int mac_gpu_info_vram_mb(void) __attribute__((weak));
extern const char *config_string(const char *) __attribute__((weak));

#define MAC_GPU_TEXTURE_BYTES_LIMIT (256U * 1024U * 1024U)
#define MAC_GPU_RETAIL_CACHE_BYTES_DEFAULT (8U * 1024U * 1024U)
#define MAC_GPU_RETAIL_CACHE_BYTES_LARGE (16U * 1024U * 1024U)
#define MAC_GPU_RETAIL_CACHE_ENTRY_DEFAULT 128U
#define MAC_GPU_RETAIL_CACHE_ENTRY_LARGE 256U
#define MAC_TEXTURE_MODE_MAX 0x12U
enum { MAC_SAMPLER_NONE = 0, MAC_SAMPLER_2D = MAC_GPU_TEXTURE_SAMPLER_2D,
    MAC_SAMPLER_3D = MAC_GPU_TEXTURE_SAMPLER_3D,
    MAC_SAMPLER_CUBE = MAC_GPU_TEXTURE_SAMPLER_CUBE };
static mac_gpu_texture_borrowed_resolver borrowed_resolver;
static void *borrowed_resolver_context;
struct retail_sampler_key {
    DWORD address_u, address_v, address_w;
    DWORD min_filter, mag_filter, mip_filter;
    DWORD max_mip_level, border_color;
};
struct retail_texture_entry {
    const void *hardware;
    unsigned long generation;
    GLuint name;
    GLenum target;
    unsigned char sampler;
    unsigned levels;
    struct retail_sampler_key sampler_key;
    unsigned pins;
    size_t bytes;
    unsigned long last_use;
    int live;
    /* the sampler parameters last written to this texture object, so that
       re-keying it writes only those that differ (Apple's driver re-uploads
       a texture's levels when its parameters are written) */
    int applied_valid;
    GLint applied_u, applied_v, applied_w, applied_min, applied_mag, applied_max_level;
    GLfloat applied_min_lod, applied_border[4];
};
static struct retail_texture_entry retail_texture_cache[MAC_GPU_RETAIL_CACHE_ENTRY_LARGE];
static unsigned long retail_parameter_writes, retail_parameter_skips;
/* port: rtflip: the back buffer's turned-over copies made and reused (back_buffer_flipped) */
static unsigned long back_buffer_flip_copies, back_buffer_flip_reused;
static int back_buffer_flip_valid;
static size_t retail_texture_cache_bytes;
static size_t retail_texture_cache_bytes_limit = MAC_GPU_RETAIL_CACHE_BYTES_DEFAULT;
static unsigned retail_texture_cache_entry_limit = MAC_GPU_RETAIL_CACHE_ENTRY_DEFAULT;
static int retail_texture_cache_configured;
/* the draw being bound is a HUD meter's (set by the device) */
static int gpu_meter_draw;
void mac_gpu_textures_meter_draw(int meter) { gpu_meter_draw = meter; }
/* (HALO_MAC_TEXTURE_SKIP_LEVELS=n, like the PC version's lower texture
   quality) Mipmapped bitmaps are uploaded without their n largest levels,
   keeping at least an 8-pixel edge: a quarter of the memory per level on
   GPUs with little VRAM. Bitmaps without mipmaps (the HUD's) are whole. */
static int retail_texture_skip_levels = -1;
/* port/mac/mac_gpu_info.c, when linked: mac.texture_skip_levels and
   mac.texture_cache_mb, their auto resolved */
extern int mac_gpu_info_texture_skip_levels(void) __attribute__((weak));
extern long mac_gpu_info_texture_cache_mb(void) __attribute__((weak));
static unsigned texture_levels_to_skip(unsigned levels, uint32_t width, uint32_t height)
{
    unsigned skip = 0;
    if (retail_texture_skip_levels < 0) {
        const char *value = getenv("HALO_MAC_TEXTURE_SKIP_LEVELS");
        retail_texture_skip_levels = value ? atoi(value) : 0;
        if (!value && mac_gpu_info_texture_skip_levels)
            retail_texture_skip_levels = mac_gpu_info_texture_skip_levels();
        if (retail_texture_skip_levels < 0 || retail_texture_skip_levels > 4) retail_texture_skip_levels = 0;
    }
    while (skip < (unsigned)retail_texture_skip_levels && levels - skip > 1 &&
           (width >> (skip + 1)) >= 8 && (height >> (skip + 1)) >= 8) ++skip;
    return skip;
}
static unsigned long retail_texture_generation;
/* texture packs (below): -1 none, 1 open, 0 not asked yet; and their uploads */
static int texture_pack_state;
static unsigned long texture_pack_uploads, texture_pack_gl_fallbacks;
static unsigned long hud_uploads, hud_bytes; /* port: the high-res HUD's textures made, and their bytes */
static int retail_s3tc_supported = -1;
static unsigned long retail_compressed_uploads;
/* textures uploaded for one draw and deleted after it */
static unsigned long transient_uploads, transient_upload_bytes;
static unsigned long retail_texture_use_serial;
static unsigned long retail_texture_hits, retail_texture_misses, retail_texture_evictions;
static unsigned long long gl_texture_binds, gl_active_calls, gl_bias_writes;
static unsigned long long stage_memo_hits; /* (draws that rebound a stage's last texture) */
static unsigned long retail_texture_sampler_reuses;
static void set_error(const char **error, const char *message);
static void tex_shadow_forget_name(GLuint name);
static void xbox_textures_drop_all(void);
static size_t xbox_texture_bytes;
static unsigned long xbox_texture_serial, xbox_texture_bind_serial;
/* HALO_MAC_XBOX_TEXTURE_SIMPLE=1 (a measurement): every bind looks the texture up and writes its sampler state, as before the memo */
static int xbox_texture_simple = -1;
static unsigned long xbox_texture_memo_hits;
static unsigned long xbox_texture_hits, xbox_texture_uploads, xbox_texture_evictions, xbox_texture_compressed;
static unsigned long xbox_texture_failures;

static int retail_texture_cache_configure(const char **error)
{
    const char *setting;
    int auto_cache = 1;
    if (retail_texture_cache_configured) return 1;
    setting = getenv("HALO_MAC_RETAIL_TEXTURE_CACHE_16M");
    /* unset: the larger cache, the measured default; "0": the smaller */
    if (!setting) setting = "1";
    else auto_cache = 0;
    if (setting[0] == '0' && setting[1] == '\0') {
        retail_texture_cache_bytes_limit = MAC_GPU_RETAIL_CACHE_BYTES_DEFAULT;
        retail_texture_cache_entry_limit = MAC_GPU_RETAIL_CACHE_ENTRY_DEFAULT;
    } else if (setting[0] == '1' && setting[1] == '\0') {
        retail_texture_cache_bytes_limit = MAC_GPU_RETAIL_CACHE_BYTES_LARGE;
        retail_texture_cache_entry_limit = MAC_GPU_RETAIL_CACHE_ENTRY_LARGE;
    } else {
        set_error(error, "HALO_MAC_RETAIL_TEXTURE_CACHE_16M must be 0 or 1");
        return 0;
    }
    {
        /* (HALO_MAC_RETAIL_TEXTURE_CACHE_MB=4..48) a size of the card's
           choosing: maps with more full-resolution textures than 16 MB
           (Longest, Hang 'Em High) evict and upload them every frame
           otherwise. Whatever it is, it must leave the card room. */
        const char *megabytes = getenv("HALO_MAC_RETAIL_TEXTURE_CACHE_MB");
        if (megabytes && *megabytes) {
            long value = atol(megabytes);
            if (value < 4 || value > 48) {
                set_error(error, "HALO_MAC_RETAIL_TEXTURE_CACHE_MB must be 4 to 48");
                return 0;
            }
            retail_texture_cache_bytes_limit = (size_t)value * 1024U * 1024U;
            retail_texture_cache_entry_limit = MAC_GPU_RETAIL_CACHE_ENTRY_LARGE;
        } else if (auto_cache && mac_gpu_info_texture_cache_mb) {
            /* mac.texture_cache_mb, or the card's size when it is 0 */
            retail_texture_cache_bytes_limit = (size_t)mac_gpu_info_texture_cache_mb() * 1024U * 1024U;
            retail_texture_cache_entry_limit = MAC_GPU_RETAIL_CACHE_ENTRY_LARGE;
        }
    }
    retail_texture_cache_configured = 1;
    return 1;
}

static void retail_texture_cache_drop(unsigned index, int eviction)
{
    struct retail_texture_entry *entry = &retail_texture_cache[index];
    if (!entry->live) return;
    if (entry->pins) abort();
    tex_shadow_forget_name(entry->name);
    glDeleteTextures(1, &entry->name);
    if (retail_texture_cache_bytes >= entry->bytes)
        retail_texture_cache_bytes -= entry->bytes;
    else retail_texture_cache_bytes = 0;
    if (eviction) ++retail_texture_evictions;
    memset(entry, 0, sizeof(*entry));
}

static void retail_sampler_key_make(
    const DWORD state[D3DTSS_MAX],
    struct retail_sampler_key *key)
{
    key->address_u = state[D3DTSS_ADDRESSU];
    key->address_v = state[D3DTSS_ADDRESSV];
    key->address_w = state[D3DTSS_ADDRESSW];
    key->min_filter = state[D3DTSS_MINFILTER];
    key->mag_filter = state[D3DTSS_MAGFILTER];
    key->mip_filter = state[D3DTSS_MIPFILTER];
    key->max_mip_level = state[D3DTSS_MAXMIPLEVEL];
    key->border_color = state[D3DTSS_BORDERCOLOR];
}

/* The key of a texture of this target: a 2D texture has no third
   coordinate, so its address mode there does not matter (the game sets it
   differently from one draw to the next) */
static void retail_sampler_key_for(
    const DWORD state[D3DTSS_MAX], GLenum target,
    struct retail_sampler_key *key)
{
    retail_sampler_key_make(state, key);
    if (target == GL_TEXTURE_2D) key->address_w = 0;
}

static int retail_sampler_key_equal(
    const struct retail_sampler_key *left,
    const struct retail_sampler_key *right)
{
    return left->address_u == right->address_u &&
        left->address_v == right->address_v &&
        left->address_w == right->address_w &&
        left->min_filter == right->min_filter &&
        left->mag_filter == right->mag_filter &&
        left->mip_filter == right->mip_filter &&
        left->max_mip_level == right->max_mip_level &&
        left->border_color == right->border_color;
}

/* The entry last found for a texture and sampler state, by a hash of them:
   tried first, and used only if it still matches in full, so that most
   draws do not scan the cache. */
#define RETAIL_TEXTURE_HINTS 512U
/* (HALO_MAC_PERF_TEX_HINTS) a table sixteen times as large, so that fewer
   textures share a hint and fall to the scan of the cache */
#define RETAIL_TEXTURE_HINTS_LARGE 8192U
static unsigned short retail_texture_hints[RETAIL_TEXTURE_HINTS_LARGE];
static unsigned retail_texture_hint_count;

static unsigned retail_texture_hint_slot(const void *hardware, unsigned char sampler,
    const struct retail_sampler_key *key)
{
    unsigned long h = (unsigned long)hardware >> 4;
    h = h * 31UL + sampler;
    h = h * 31UL + (unsigned long)key->address_u;
    h = h * 31UL + (unsigned long)key->address_v;
    h = h * 31UL + (unsigned long)key->min_filter;
    h = h * 31UL + (unsigned long)key->mag_filter;
    h = h * 31UL + (unsigned long)key->mip_filter;
    h ^= h >> 9;
    if (!retail_texture_hint_count)
        retail_texture_hint_count = mac_perf_flag("HALO_MAC_PERF_TEX_HINTS") ?
            RETAIL_TEXTURE_HINTS_LARGE : RETAIL_TEXTURE_HINTS;
    return (unsigned)(h % retail_texture_hint_count);
}

static int retail_texture_entry_matches(const struct retail_texture_entry *entry,
    const void *hardware, unsigned char sampler, GLenum target,
    const struct retail_sampler_key *key, unsigned long generation)
{
    return entry->live && entry->generation == generation &&
        entry->hardware == hardware && entry->sampler == sampler &&
        entry->target == target &&
        retail_sampler_key_equal(&entry->sampler_key, key);
}

static int retail_texture_cache_find(const void *hardware, unsigned char sampler,
    GLenum target, const DWORD sampler_state[D3DTSS_MAX],
    unsigned long generation, unsigned *index_out)
{
    unsigned i, slot, hint;
    struct retail_sampler_key key;
    retail_sampler_key_for(sampler_state, target, &key);
    slot = retail_texture_hint_slot(hardware, sampler, &key);
    hint = retail_texture_hints[slot];
    if (hint < retail_texture_cache_entry_limit &&
        retail_texture_entry_matches(&retail_texture_cache[hint], hardware,
        sampler, target, &key, generation)) {
        *index_out = hint;
        return 1;
    }
    for (i = 0; i < retail_texture_cache_entry_limit; ++i) {
        if (retail_texture_entry_matches(&retail_texture_cache[i], hardware,
            sampler, target, &key, generation)) {
            retail_texture_hints[slot] = (unsigned short)i;
            *index_out = i;
            return 1;
        }
    }
    return 0;
}

static int retail_texture_cache_find_reusable(const void *hardware,
    unsigned char sampler, GLenum target, unsigned long generation,
    unsigned *index_out)
{
    unsigned i;
    for (i = 0; i < retail_texture_cache_entry_limit; ++i) {
        struct retail_texture_entry *entry = &retail_texture_cache[i];
        if (entry->live && !entry->pins &&
            entry->generation == generation &&
            entry->hardware == hardware && entry->sampler == sampler &&
            entry->target == target) {
            *index_out = i;
            return 1;
        }
    }
    return 0;
}

static int retail_texture_cache_pin(struct mac_gpu_texture_scope *scope,
    unsigned index)
{
    unsigned i;
    for (i = 0; i < scope->cache_entry_count; ++i)
        if (scope->cache_entries[i] == index) return 1;
    if (scope->cache_entry_count >= MAC_GPU_TEXTURE_SCOPE_CACHE_ENTRIES)
        return 0;
    ++retail_texture_cache[index].pins;
    scope->cache_entries[scope->cache_entry_count++] = index;
    return 1;
}

static int retail_texture_cache_make_room(size_t bytes, unsigned *index_out)
{
    unsigned i, free_index = retail_texture_cache_entry_limit;
    if (bytes > retail_texture_cache_bytes_limit) return 0;
    for (;;) {
        unsigned oldest = retail_texture_cache_entry_limit;
        unsigned long oldest_use = (unsigned long)-1;
        free_index = retail_texture_cache_entry_limit;
        for (i = 0; i < retail_texture_cache_entry_limit; ++i) {
            struct retail_texture_entry *entry = &retail_texture_cache[i];
            if (!entry->live) { if (free_index == retail_texture_cache_entry_limit) free_index = i; }
            else if (!entry->pins && entry->last_use < oldest_use) {
                oldest = i; oldest_use = entry->last_use;
            }
        }
        if (retail_texture_cache_bytes <= retail_texture_cache_bytes_limit - bytes &&
            free_index != retail_texture_cache_entry_limit)
            break;
        if (oldest == retail_texture_cache_entry_limit) return 0;
        retail_texture_cache_drop(oldest, 1);
    }
    *index_out = free_index;
    return 1;
}

int mac_gpu_textures_set_borrowed_resolver(
    mac_gpu_texture_borrowed_resolver resolver, void *context)
{
    if (!resolver && context) return 0;
    borrowed_resolver = resolver;
    borrowed_resolver_context = context;
    return 1;
}

/* Whether a native texture is linear (its coordinates are in texels, which
   the pixel shader scales by 1/size) rather than swizzled (its coordinates
   are normalized already). A borrowed texture's resolver gives the same
   scale as a texture of the resource's own would have. */
int mac_gpu_textures_resource_is_linear(const void *resource)
{
    struct xgpu_texture_description description;
    const D3DBaseTexture *native = (const D3DBaseTexture *)resource;
    if (!native) return 0;
    mac_xgpu_texture_describe(native->Format, native->Size, &description);
    return description.linear != 0;
}

int mac_gpu_textures_bitmap_cache_invalidate(void)
{
    unsigned i;
    for (i = 0; i < MAC_GPU_RETAIL_CACHE_ENTRY_LARGE; ++i)
        if (retail_texture_cache[i].live && retail_texture_cache[i].pins)
            return 0;
    for (i = 0; i < MAC_GPU_RETAIL_CACHE_ENTRY_LARGE; ++i)
        retail_texture_cache_drop(i, 0);
    retail_texture_cache_bytes = 0;
    xbox_textures_drop_all();
    if (texture_pack_reset) texture_pack_reset();
    retail_texture_generation = mac_native_bitmap_cache_generation();
    return 1;
}

int mac_gpu_textures_dispose(void)
{
    struct mac_gpu_texture_cache_stats stats;
    mac_gpu_textures_cache_get_stats(&stats);
    fprintf(stderr, "native retail GL texture cache: entries=%u/%u bytes=%lu/%lu hits=%lu misses=%lu evictions=%lu sampler_reuses=%lu\n",
        stats.entries, stats.entry_limit, (unsigned long)stats.bytes,
        (unsigned long)stats.bytes_limit, stats.hits, stats.misses,
        stats.evictions, stats.sampler_reuses);
    fprintf(stderr, "native retail GL texture stage repeats: %llu\n", stage_memo_hits);
    if (xbox_texture_uploads || xbox_texture_hits)
        fprintf(stderr, "native xbox GL textures: uploads=%lu (dxt %lu) hits=%lu (stage memo %lu) evictions=%lu bytes=%lu\n",
            xbox_texture_uploads, xbox_texture_compressed, xbox_texture_hits, xbox_texture_memo_hits, xbox_texture_evictions,
            (unsigned long)xbox_texture_bytes);
    fprintf(stderr, "native GL texture calls: binds=%llu active=%llu bias=%llu\n", gl_texture_binds, gl_active_calls, gl_bias_writes);
    if (!mac_gpu_textures_bitmap_cache_invalidate()) return 0;
    fprintf(stderr, "native retail compressed uploads: %lu\n", retail_compressed_uploads);
    if (texture_pack_state > 0)
        fprintf(stderr, "texture pack uploads: %lu replaced, %lu fell back to the maps' own after an OpenGL error\n",
            texture_pack_uploads, texture_pack_gl_fallbacks);
    if (hud_uploads)
        fprintf(stderr, "high-res hud: %lu textures made, %lu bytes\n", hud_uploads, hud_bytes);
    fprintf(stderr, "native transient texture uploads: %lu bytes=%lu\n", transient_uploads, transient_upload_bytes);
    fprintf(stderr, "native retail sampler parameters: written=%lu skipped=%lu\n",
        retail_parameter_writes, retail_parameter_skips);
    if (getenv("HALO_MAC_FLIP_LOG"))
        fprintf(stderr, "flip log: back buffer copies made=%lu reused=%lu\n",
            back_buffer_flip_copies, back_buffer_flip_reused);
    back_buffer_flip_copies = back_buffer_flip_reused = 0;
    back_buffer_flip_valid = 0;
    retail_s3tc_supported = -1;
    retail_compressed_uploads = 0;
    retail_texture_cache_configured = 0;
    retail_texture_cache_bytes_limit = MAC_GPU_RETAIL_CACHE_BYTES_DEFAULT;
    retail_texture_cache_entry_limit = MAC_GPU_RETAIL_CACHE_ENTRY_DEFAULT;
    return 1;
}

void mac_gpu_textures_cache_get_stats(struct mac_gpu_texture_cache_stats *stats)
{
    unsigned i;
    if (!stats) return;
    memset(stats, 0, sizeof(*stats));
    stats->bytes = retail_texture_cache_bytes;
    stats->hits = retail_texture_hits;
    stats->misses = retail_texture_misses;
    stats->evictions = retail_texture_evictions;
    stats->sampler_reuses = retail_texture_sampler_reuses;
    stats->bytes_limit = retail_texture_cache_bytes_limit;
    stats->entry_limit = retail_texture_cache_entry_limit;
    for (i = 0; i < MAC_GPU_RETAIL_CACHE_ENTRY_LARGE; ++i)
        if (retail_texture_cache[i].live) ++stats->entries;
}

static int texture_mode_has_sampler(unsigned mode)
{
    /* Matches the NV2A pixel-shader mode enum: passthru, clipplane,
       dot_zw, and dot_product consume coordinates but no texture. */
    if (mode == 0 || mode == 0x04 || mode == 0x05 || mode == 0x0a || mode == 0x11)
        return 0;
    return mode <= MAC_TEXTURE_MODE_MAX;
}

static void set_error(const char **error, const char *message)
{ if (error) *error = message; }

static int finite_float(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7f800000U) != 0x7f800000U;
}

static GLenum address_mode(DWORD value)
{
    switch (value) {
    case D3DTADDRESS_WRAP: return GL_REPEAT;
    case D3DTADDRESS_MIRROR: return GL_MIRRORED_REPEAT;
    case D3DTADDRESS_CLAMP:
    case D3DTADDRESS_CLAMPTOEDGE: return GL_CLAMP_TO_EDGE;
    case D3DTADDRESS_BORDER: return GL_CLAMP_TO_BORDER;
    default: return 0;
    }
}

static int sampler_filters(DWORD min_filter, DWORD mag_filter, DWORD mip_filter,
    unsigned long levels, GLint *min_out, GLint *mag_out)
{
    GLint base, mip;
    if (min_filter == D3DTEXF_POINT) base = GL_NEAREST;
    else if (min_filter == D3DTEXF_LINEAR) base = GL_LINEAR;
    else return 0;
    if (mag_filter == D3DTEXF_POINT) *mag_out = GL_NEAREST;
    else if (mag_filter == D3DTEXF_LINEAR) *mag_out = GL_LINEAR;
    else return 0;
    if (mip_filter == D3DTEXF_NONE || levels <= 1) mip = 0;
    else if (mip_filter == D3DTEXF_POINT) mip = 1;
    else if (mip_filter == D3DTEXF_LINEAR) mip = 2;
    else return 0;
    if (!mip) *min_out = base;
    else if (mip == 1) *min_out = base == GL_NEAREST ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_NEAREST;
    else *min_out = base == GL_NEAREST ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR;
    return 1;
}

static unsigned long mip_dimension(unsigned long value, unsigned level)
{ value >>= level; return value ? value : 1; }

/* (lean) The sampler parameters last written to borrowed textures (render
   targets, the text atlas) by name: their owners set parameters only when
   they create them, and report deletions (mac_gpu_textures_forget), so
   rewriting identical parameters, which makes Apple's driver resynchronise
   the texture, can be skipped. */
#define BORROWED_SHADOW_SLOTS 32
struct borrowed_shadow {
    GLuint name; GLenum target; int valid;
    GLint u, v, w, min, mag, max_level; GLfloat min_lod, border[4];
};
static struct borrowed_shadow borrowed_shadows[BORROWED_SHADOW_SLOTS];
static int lean_texture_scope;
/* (lean) The texture units' state as the draws last set it: the active
   unit, each unit's bound texture per target and its LOD bias, so that a
   draw calls GL only for what differs (each call makes the driver validate
   the unit again). A deleted name is forgotten; the whole is forgotten at a
   frame's start and after any scope that is not lean. */
static int tex_shadow_active = -1;
static GLuint tex_shadow_bound[D3DTSS_MAXSTAGES][3];
static unsigned char tex_shadow_bound_valid[D3DTSS_MAXSTAGES][3];
static float tex_shadow_bias[D3DTSS_MAXSTAGES];
static unsigned char tex_shadow_bias_valid[D3DTSS_MAXSTAGES];
static int tex_shadow_target(GLenum target)
{ return target == GL_TEXTURE_2D ? 0 : target == GL_TEXTURE_CUBE_MAP ? 1 : target == GL_TEXTURE_3D ? 2 : -1; }
void mac_gpu_textures_shadow_reset(void)
{
    tex_shadow_active = -1;
    memset(tex_shadow_bound_valid, 0, sizeof(tex_shadow_bound_valid));
    memset(tex_shadow_bias_valid, 0, sizeof(tex_shadow_bias_valid));
}
static void tex_shadow_forget_name(GLuint name)
{
    unsigned unit, target;
    for (unit = 0; unit < D3DTSS_MAXSTAGES; ++unit)
        for (target = 0; target < 3; ++target)
            if (tex_shadow_bound[unit][target] == name) tex_shadow_bound_valid[unit][target] = 0;
}
static void tex_active(unsigned unit)
{
    if (lean_texture_scope && tex_shadow_active == (int)unit) return;
    glActiveTextureARB(GL_TEXTURE0_ARB + unit); ++gl_active_calls;
    tex_shadow_active = (int)unit;
}
static void tex_bind(GLenum target, GLuint name)
{
    int index = tex_shadow_target(target), unit = tex_shadow_active;
    if (lean_texture_scope && unit >= 0 && unit < D3DTSS_MAXSTAGES && index >= 0 &&
        tex_shadow_bound_valid[unit][index] && tex_shadow_bound[unit][index] == name) return;
    glBindTexture(target, name); ++gl_texture_binds;
    if (unit >= 0 && unit < D3DTSS_MAXSTAGES && index >= 0) {
        tex_shadow_bound[unit][index] = name; tex_shadow_bound_valid[unit][index] = 1;
    }
}
static void tex_bias(float bias)
{
    int unit = tex_shadow_active;
    if (lean_texture_scope && unit >= 0 && unit < D3DTSS_MAXSTAGES &&
        tex_shadow_bias_valid[unit] && tex_shadow_bias[unit] == bias) return;
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, bias); ++gl_bias_writes;
    if (unit >= 0 && unit < D3DTSS_MAXSTAGES) { tex_shadow_bias[unit] = bias; tex_shadow_bias_valid[unit] = 1; }
}
/* (lean) As tex_bind and tex_bias for a given unit, which is made active
   only if the call is needed: a stage that rebinds its last texture costs GL
   nothing, not even the unit's selection. */
static void tex_bind_unit(unsigned unit, GLenum target, GLuint name)
{
    int index = tex_shadow_target(target);
    if (unit < D3DTSS_MAXSTAGES && index >= 0 &&
        tex_shadow_bound_valid[unit][index] && tex_shadow_bound[unit][index] == name) return;
    tex_active(unit);
    tex_bind(target, name);
}
static void tex_bias_unit(unsigned unit, float bias)
{
    if (unit < D3DTSS_MAXSTAGES && tex_shadow_bias_valid[unit] && tex_shadow_bias[unit] == bias) return;
    tex_active(unit);
    tex_bias(bias);
}
/* (HALO_MAC_FF_COMBINERS) The fixed-function combiner draws run on texture
   units whose 2D texture the game may not have bound; they take a white one
   of their own. Through the same shadow as the game's bindings, so that a
   later draw rebinds what it samples. */
static GLuint ff_dummy_texture;
void mac_gpu_textures_ff_select(unsigned unit)
{
    if (unit < D3DTSS_MAXSTAGES) tex_active(unit);
}
void mac_gpu_textures_ff_dummy(unsigned unit)
{
    if (unit >= D3DTSS_MAXSTAGES) return;
    if (!ff_dummy_texture) {
        static const unsigned char white[4] = { 255, 255, 255, 255 };
        glGenTextures(1, &ff_dummy_texture);
        tex_active(unit);
        glBindTexture(GL_TEXTURE_2D, ff_dummy_texture); ++gl_texture_binds;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        tex_shadow_bound[unit][0] = ff_dummy_texture; tex_shadow_bound_valid[unit][0] = 1;
        return;
    }
    tex_bind_unit(unit, GL_TEXTURE_2D, ff_dummy_texture);
}
void mac_gpu_textures_call_counts(unsigned long long *binds, unsigned long long *active, unsigned long long *bias)
{
    *binds = gl_texture_binds; *active = gl_active_calls; *bias = gl_bias_writes;
}

void mac_gpu_textures_forget(GLuint name)
{
    unsigned i;
    for (i = 0; i < BORROWED_SHADOW_SLOTS; ++i)
        if (borrowed_shadows[i].name == name) memset(&borrowed_shadows[i], 0, sizeof(borrowed_shadows[i]));
    tex_shadow_forget_name(name);
}
/* whether two stages sample alike: their addressing, filters, mip limits
   and border colour (the rest of a stage's state, its texture coordinates
   and combiners, may differ, as the sun glow's blur gives its four stages
   one texture at four offsets) */
static int sampler_states_equal(const DWORD *a, const DWORD *b)
{
    static const unsigned fields[] = {
        /* (not ADDRESSW: render targets are 2D) */
        D3DTSS_ADDRESSU, D3DTSS_ADDRESSV, D3DTSS_MAGFILTER,
        D3DTSS_MINFILTER, D3DTSS_MIPFILTER, D3DTSS_MIPMAPLODBIAS,
        D3DTSS_MAXMIPLEVEL, D3DTSS_MAXANISOTROPY, D3DTSS_BORDERCOLOR
    };
    unsigned i;
    for (i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        if (a[fields[i]] != b[fields[i]]) {
            static unsigned reported;
            if (reported < 4) {
                ++reported;
                fprintf(stderr, "native texture: one render target, two stages: state %u is %lu and %lu\n",
                    fields[i], (unsigned long)a[fields[i]], (unsigned long)b[fields[i]]);
            }
            return 0;
        }
    return 1;
}

static void save_borrowed_parameters(struct mac_gpu_texture_scope *scope,
    GLuint name, GLenum target, unsigned stage)
{
    unsigned i;
    for (i = 0; i < scope->borrowed_count; ++i) {
        if (scope->borrowed[i].name == name && scope->borrowed[i].target == target) {
            if (!sampler_states_equal(scope->borrowed[i].source_state, D3D__TextureState[stage]))
                scope->active = -1; /* a shared texture cannot have two sampler states here */
            return;
        }
    }
    if (scope->borrowed_count >= 4) {
        fprintf(stderr, "native texture: more than four render targets in one draw\n");
        scope->active = -1; return;
    }
    i = scope->borrowed_count++;
    scope->borrowed[i].name = name; scope->borrowed[i].target = target;
    memcpy(scope->borrowed[i].source_state, D3D__TextureState[stage],
        sizeof(scope->borrowed[i].source_state));
    /* (lean: nothing is written back, so nothing need be read) */
    if (lean_texture_scope) return;
    glGetTexParameteriv(target, GL_TEXTURE_WRAP_S, &scope->borrowed[i].wrap_s);
    glGetTexParameteriv(target, GL_TEXTURE_WRAP_T, &scope->borrowed[i].wrap_t);
    glGetTexParameteriv(target, GL_TEXTURE_MIN_FILTER, &scope->borrowed[i].min_filter);
    glGetTexParameteriv(target, GL_TEXTURE_MAG_FILTER, &scope->borrowed[i].mag_filter);
    glGetTexParameteriv(target, GL_TEXTURE_BASE_LEVEL, &scope->borrowed[i].base_level);
    glGetTexParameteriv(target, GL_TEXTURE_MAX_LEVEL, &scope->borrowed[i].max_level);
    glGetTexParameterfv(target, GL_TEXTURE_MIN_LOD, &scope->borrowed[i].min_lod);
    glGetTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, scope->borrowed[i].border);
    if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_3D)
        glGetTexParameteriv(target, GL_TEXTURE_WRAP_R, &scope->borrowed[i].wrap_r);
    else scope->borrowed[i].wrap_r = GL_REPEAT;
}

/* The texture the game wraps around the back buffer (the rasterizer's
   render_primary target, which the sun's glow copies from): the color
   texture of the framebuffer the frame is drawn into, or 0 when that is a
   renderbuffer or the back buffer is itself the target being drawn. */
static GLuint back_buffer_copy;
static unsigned back_buffer_copy_width, back_buffer_copy_height;

/* A visible window draws into the window's own framebuffer (0), which has
   no texture: the frame so far is copied into one, each time it is
   sampled (the sun's glow does so twice a frame, 640x480). */
static unsigned back_buffer_copied(const struct mac_device_state *device)
{
    GLint old_framebuffer = 0, old_texture = 0;
    unsigned width, height;
    /* port: the pixels the frame is drawn with (more than the game's where mac.render_resolution
       asks); the game addresses the texture by fractions of it, so any size serves */
    mac_device_render_region(&width, &height);
    if (!width || !height) return 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &old_framebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_texture);
    while (glGetError() != GL_NO_ERROR) {} /* port: as back_buffer_flipped */
    if (!back_buffer_copy) glGenTextures(1, &back_buffer_copy);
    glBindTexture(GL_TEXTURE_2D, back_buffer_copy);
    if (width != back_buffer_copy_width || height != back_buffer_copy_height) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        back_buffer_copy_width = width; back_buffer_copy_height = height;
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, (GLsizei)width, (GLsizei)height);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, (GLuint)old_framebuffer);
    glBindTexture(GL_TEXTURE_2D, (GLuint)old_texture);
    return glGetError() == GL_NO_ERROR ? back_buffer_copy : 0;
}

/* port: rtflip. A render target's rows are top-down, as the Xbox's are
   (engine_device_state.c, mac_target_flipped), so the back buffer, which
   is drawn as the window is, is sampled from a copy turned over: the
   framebuffer blitted with its rows reversed (a row at a time where the
   driver has no blit). The copy is made once for each state of the back
   buffer, which the draws and clears into it count. */
#ifndef GL_READ_FRAMEBUFFER_EXT
#define GL_READ_FRAMEBUFFER_EXT 0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT 0x8CA9
#endif
typedef void (*back_buffer_blit_proc)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
static GLuint back_buffer_flip_texture, back_buffer_flip_framebuffer;
static unsigned back_buffer_flip_width, back_buffer_flip_height;
static unsigned long back_buffer_flip_writes, back_buffer_flip_presents;
static int back_buffer_blit_checked;
static back_buffer_blit_proc back_buffer_blit;

static void back_buffer_blit_find(void)
{
    const char *extensions = (const char *)glGetString(GL_EXTENSIONS);
    back_buffer_blit_checked = 1;
    if (extensions && strstr(extensions, "GL_EXT_framebuffer_blit"))
        back_buffer_blit = (back_buffer_blit_proc)dlsym(RTLD_DEFAULT, "glBlitFramebufferEXT");
    if (getenv("HALO_MAC_RENDER_TRACE") || getenv("HALO_MAC_FLIP_LOG"))
        fprintf(stderr, "back buffer copies: %s\n", back_buffer_blit ? "framebuffer blit" : "row by row");
}

static unsigned back_buffer_flipped(const struct mac_device_state *device)
{
    GLint old_framebuffer = 0, old_texture = 0;
    unsigned width, height;
    GLuint source = (GLuint)device->gpu_default_framebuffer;
    mac_device_render_region(&width, &height); /* port: as back_buffer_copied */
    if (!width || !height) return 0;
    if (back_buffer_flip_texture && back_buffer_flip_valid &&
        back_buffer_flip_writes == device->gpu_back_buffer_writes &&
        back_buffer_flip_presents == device->gpu_present_count &&
        width == back_buffer_flip_width && height == back_buffer_flip_height) {
        ++back_buffer_flip_reused;
        return back_buffer_flip_texture;
    }
    if (!back_buffer_blit_checked) back_buffer_blit_find();
    /* port: errors an earlier call left unread (a driver may raise some in the lean draws, which
       never ask) are not this copy's: with them the copy was counted a failure, and made again
       for each sampling (a Radeon 9550: the sun's glow aborted a10) */
    while (glGetError() != GL_NO_ERROR) {}
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &old_framebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_texture);
    if (!back_buffer_flip_texture) glGenTextures(1, &back_buffer_flip_texture);
    if (!back_buffer_flip_framebuffer) glGenFramebuffersEXT(1, &back_buffer_flip_framebuffer);
    glBindTexture(GL_TEXTURE_2D, back_buffer_flip_texture);
    if (width != back_buffer_flip_width || height != back_buffer_flip_height) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, back_buffer_flip_framebuffer);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
            back_buffer_flip_texture, 0);
        back_buffer_flip_width = width; back_buffer_flip_height = height;
    }
    if (back_buffer_blit) {
        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, source);
        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, back_buffer_flip_framebuffer);
        back_buffer_blit(0, 0, (GLint)width, (GLint)height, 0, (GLint)height, (GLint)width, 0,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    } else {
        unsigned row;
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, source);
        for (row = 0; row < height; ++row)
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLint)(height - 1 - row), 0, (GLint)row,
                (GLsizei)width, 1);
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, (GLuint)old_framebuffer);
    glBindTexture(GL_TEXTURE_2D, (GLuint)old_texture);
    back_buffer_flip_writes = device->gpu_back_buffer_writes;
    back_buffer_flip_presents = device->gpu_present_count;
    {
        GLenum copy_error = glGetError();
        back_buffer_flip_valid = copy_error == GL_NO_ERROR;
        if (getenv("HALO_MAC_FLIP_LOG")) {
            static unsigned logged;
            if (logged < 16) {
                ++logged;
                fprintf(stderr, "back buffer copy %u: %ux%u %s, GL error 0x%04x\n", logged, width, height,
                    back_buffer_blit ? "blit" : "row by row", (unsigned)copy_error);
            }
        }
    }
    ++back_buffer_flip_copies;
    return back_buffer_flip_valid ? back_buffer_flip_texture : 0;
}

static unsigned back_buffer_texture(const struct mac_device_state *device, const void *resource)
{
    const D3DBaseTexture *native = (const D3DBaseTexture *)resource;
    GLint type = 0, name = 0, old_framebuffer = 0;
    if (!native || !device->back_buffer.Data || native->Data != device->back_buffer.Data ||
        device->render_target == &device->back_buffer)
        return 0;
    if (device->gpu_flip_targets) return back_buffer_flipped(device);
    if (!device->gpu_default_framebuffer) return back_buffer_copied(device);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &old_framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, (GLuint)device->gpu_default_framebuffer);
    glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT, &type);
    if (type == GL_TEXTURE)
        glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &name);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, (GLuint)old_framebuffer);
    return type == GL_TEXTURE && name > 0 ? (unsigned)name : 0;
}

static int is_attached_render_target(const void *resource, unsigned *texture_id)
{
    const struct mac_device_state *device = mac_device_state_get();
    D3DSurface *surface = NULL;
    unsigned framebuffer, texture, width, height;
    unsigned active_framebuffer, active_texture, active_width, active_height;
    int target = 0;
    *texture_id = 0;
    (void)surface; (void)framebuffer; (void)width; (void)height;
    if (!device || !device->render_target || !resource) return 0;
    /* (the level-0 surface's target, without making the surface) */
    if (mac_engine_device_resources_texture_target((const D3DTexture *)resource, &texture)) {
        *texture_id = texture;
        if (mac_engine_device_resources_target_info(device->render_target, &active_framebuffer,
            &active_texture, &active_width, &active_height) && texture && texture == active_texture)
            target = 1;
    }
    else *texture_id = back_buffer_texture(device, resource);
    return target;
}

/* Only the resident retail decoder supplies these original little-endian
   DXT blocks. Never upload the PPC host-sampling view as compressed data. */
static int retail_s3tc_available(void)
{
    const char *extensions, *match;
    const char *name = "GL_EXT_texture_compression_s3tc";
    size_t length = strlen(name);
#ifdef MAC_GPU_RETAIL_RGBA_ONLY
    return 0;
#endif
    if (retail_s3tc_supported >= 0) return retail_s3tc_supported;
    retail_s3tc_supported = 0;
    extensions = (const char *)glGetString(GL_EXTENSIONS);
    if (!extensions) return 0;
    match = extensions;
    while ((match = strstr(match, name)) != NULL) {
        if ((match == extensions || match[-1] == ' ') &&
            (!match[length] || match[length] == ' ')) {
            retail_s3tc_supported = 1; break;
        }
        match += length;
    }
    return retail_s3tc_supported;
}

/* A bitmap's cube map faces are stored in the tag's order (+X, +Y, -X, -Y,
   +Z, -Z); the game's rasterizer_bitmap_new copies them into the hardware's
   (face_mapping_table in rasterizer_xbox_hardware_bitmaps.c), which GL's
   faces follow: +X, -X, +Y, -Y, +Z, -Z. The uploads here read the tag's
   faces directly, so each GL face takes the tag's face that the game would
   have copied to it (the mapping is its own inverse). Without it the +Y and
   -X faces are swapped: half of the water's reflection, and of every other
   environment map, was drawn from the wrong side. */
static unsigned cube_tag_face(unsigned sampler, unsigned face)
{
    static const unsigned char tag_face[6] = { 0, 2, 1, 3, 4, 5 };
    return sampler == MAC_SAMPLER_CUBE && face < 6 ? tag_face[face] : face;
}

static GLuint upload_bitmap_compressed(struct mac_gpu_texture_scope *scope,
    const void *resource, unsigned char sampler, unsigned levels,
    size_t *bytes_out, unsigned *levels_out)
{
    unsigned faces, face, level, skip;
    uint32_t format, first_format = 0, width, height, base_width = 0, base_height = 0;
    size_t bytes, total = 0;
    const unsigned char *encoded;
    GLuint object = 0;
    GLenum target, internal_format;
    if (sampler == MAC_SAMPLER_3D || !retail_s3tc_available() ||
        scope->owned_count >= D3DTSS_MAXSTAGES) return 0;
    faces = sampler == MAC_SAMPLER_CUBE ? 6U : 1U;
    target = faces == 6 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    for (face = 0; face < faces; ++face) for (level = 0; level < levels; ++level) {
        encoded = mac_native_bitmap_cache_encoded_face(resource, cube_tag_face(sampler, face), level,
            &format, &width, &height, &bytes);
        if (!encoded || format < 14 || format > 16 || !width || !height ||
            width > 4096 || height > 4096) return 0;
        if (!face && !level) {
            first_format = format; base_width = width; base_height = height;
        }
        if (format != first_format || width != mip_dimension(base_width, level) ||
            height != mip_dimension(base_height, level) ||
            bytes != (size_t)((width + 3) / 4) * ((height + 3) / 4) *
                (format == 14 ? 8U : 16U) || total > SIZE_MAX - bytes) return 0;
        total += bytes;
    }
    skip = texture_levels_to_skip(levels, base_width, base_height);
    if (skip) {
        /* (only the levels uploaded count) */
        total = 0;
        for (face = 0; face < faces; ++face) for (level = skip; level < levels; ++level) {
            (void)mac_native_bitmap_cache_encoded_face(resource, cube_tag_face(sampler, face), level, &format, &width, &height, &bytes);
            total += bytes;
        }
    }
    if (total > MAC_GPU_TEXTURE_BYTES_LIMIT ||
        scope->uploaded_bytes > MAC_GPU_TEXTURE_BYTES_LIMIT - total) return 0;
    internal_format = first_format == 14 ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT :
        first_format == 15 ? GL_COMPRESSED_RGBA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    glGenTextures(1, &object);
    if (!object) return 0;
    tex_bind(target, object);
    for (face = 0; face < faces; ++face) for (level = skip; level < levels; ++level) {
        GLenum face_target = faces == 6 ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : target;
        encoded = mac_native_bitmap_cache_encoded_face(resource, cube_tag_face(sampler, face), level,
            &format, &width, &height, &bytes);
        if (!encoded) goto fallback;
        glCompressedTexImage2D(face_target, (GLint)(level - skip), internal_format,
            (GLsizei)width, (GLsizei)height, 0, (GLsizei)bytes, encoded);
        if (glGetError() != GL_NO_ERROR) goto fallback;
    }
    scope->owned_textures[scope->owned_count++] = object;
    scope->uploaded_bytes += total;
    *bytes_out = total;
    *levels_out = levels - skip;
    ++retail_compressed_uploads;
    return object;
fallback:
    tex_shadow_forget_name(object); glDeleteTextures(1, &object);
    while (glGetError() != GL_NO_ERROR) {}
    return 0; /* An unsupported driver retains the validated RGBA path. */
}

/* Texture packs (display.texture_pack; texture_pack.h): opened on the first
   upload, when the card can be asked what it takes. -1: none. */
static int texture_pack_ready(void)
{
    const char *pack;
    char root[1024], message[256];
    const char *error = NULL;
    GLint max_texture = 0;
    int vram;
    unsigned max_dimension;
    if (texture_pack_state) return texture_pack_state > 0;
    texture_pack_state = -1;
    if (!texture_pack_open || !texture_pack_load) return 0;
    pack = getenv("HALO_TEXTURE_PACK");
    if (!pack || !pack[0]) pack = config_string ? config_string("display.texture_pack") : NULL;
    if (!pack || !pack[0]) return 0;
    if (!mac_engine_files_data_root || !mac_engine_files_data_root(root, sizeof(root))) {
        fprintf(stderr, "texture pack \"%s\": the data root is unknown; using the stock textures\n", pack);
        return 0;
    }
    if (!retail_texture_cache_configure(&error)) return 0;
    /* 32 MB cards: 1024 pixels and DXT only; the larger, as far as GL goes */
    vram = mac_gpu_info_vram_mb ? mac_gpu_info_vram_mb() : 32;
    max_dimension = vram <= 32 ? 1024U : vram <= 128 ? 2048U : 4096U;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture);
    if (max_texture > 0 && (unsigned)max_texture < max_dimension) max_dimension = (unsigned)max_texture;
    {
        /* (HALO_MAC_TEXTURE_PACK_MAX_EDGE=n, a diagnostic: a smaller limit, to try a small card's) */
        const char *edge = getenv("HALO_MAC_TEXTURE_PACK_MAX_EDGE");
        if (edge && atoi(edge) > 0 && (unsigned)atoi(edge) < max_dimension) max_dimension = (unsigned)atoi(edge);
    }
    if (!texture_pack_open(root, pack, max_dimension, retail_s3tc_available(), vram > 32,
        retail_texture_cache_bytes_limit, message, sizeof(message))) {
        fprintf(stderr, "texture pack \"%s\" refused: %s; using the stock textures\n", pack, message);
        return 0;
    }
    texture_pack_state = 1;
    return 1;
}

/* HALO_MAC_TEXTURE_PACK_DUMP=dir: write each DXT bitmap uploaded as a DDS file
   in the pack's layout, for pack authors to start from (existing files are kept) */
static void texture_pack_dump_stock(const void *resource, unsigned char sampler, unsigned levels)
{
    static const char *dump_dir; static int asked;
    const char *name; uint32_t index, width, height, format, w, h, first_format = 0;
    size_t bytes; int cube; unsigned face, level;
    FILE *file;
    if (!asked) { asked = 1; dump_dir = getenv("HALO_MAC_TEXTURE_PACK_DUMP"); if (dump_dir && !dump_dir[0]) dump_dir = NULL; }
    if (!dump_dir || !texture_pack_dump_open || !texture_pack_dds_header || sampler == MAC_SAMPLER_3D) return;
    if (!mac_native_bitmap_cache_identify(resource, &name, &index, &width, &height, &cube)) return;
    if (!mac_native_bitmap_cache_encoded_face(resource, 0, 0, &first_format, &w, &h, &bytes) ||
        first_format < 14 || first_format > 16) return;
    file = (FILE *)texture_pack_dump_open(dump_dir, name, index);
    if (!file) return;
    {
        unsigned char header[128];
        texture_pack_dds_header(header, first_format, width, height, levels, cube);
        fwrite(header, 1, sizeof(header), file);
    }
    for (face = 0; face < (cube ? 6U : 1U); ++face) for (level = 0; level < levels; ++level) {
        const unsigned char *encoded = mac_native_bitmap_cache_encoded_face(resource,
            cube_tag_face(sampler, face), level, &format, &w, &h, &bytes);
        if (encoded) fwrite(encoded, 1, bytes, file);
    }
    fclose(file);
}

/* The pack's replacement for a bitmap, uploaded as the stock bitmap would be
   (the same levels, plus the larger ones the pack adds; the cache and the
   sampler see only a texture). 0: none, or anything wrong with it: the
   stock bitmap is used. */
static GLuint upload_bitmap_override(struct mac_gpu_texture_scope *scope,
    const void *resource, unsigned char sampler, unsigned levels,
    size_t *bytes_out, unsigned *levels_out)
{
    const char *name; uint32_t index, width, height;
    int cube;
    struct texture_pack_image image;
    unsigned face, level, skip;
    size_t total = 0;
    GLuint object = 0;
    GLenum target, internal_format = 0;
    if (sampler == MAC_SAMPLER_3D || scope->owned_count >= D3DTSS_MAXSTAGES || !texture_pack_ready()) return 0;
    if (!mac_native_bitmap_cache_identify(resource, &name, &index, &width, &height, &cube) ||
        cube != (sampler == MAC_SAMPLER_CUBE)) return 0;
    if (!texture_pack_load(name, index, width, height, levels, cube, &image)) return 0;
    skip = texture_levels_to_skip(image.levels, image.width, image.height);
    for (level = skip; level < image.levels; ++level) {
        unsigned w = image.width >> level, h = image.height >> level;
        total += texture_pack_level_bytes(image.format, w ? w : 1, h ? h : 1) * image.faces;
    }
    if (total > MAC_GPU_TEXTURE_BYTES_LIMIT || scope->uploaded_bytes > MAC_GPU_TEXTURE_BYTES_LIMIT - total) {
        texture_pack_image_free(&image); return 0;
    }
    target = image.faces == 6 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    if (image.format == TEXTURE_PACK_FORMAT_DXT1) internal_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    else if (image.format == TEXTURE_PACK_FORMAT_DXT3) internal_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    else if (image.format == TEXTURE_PACK_FORMAT_DXT5) internal_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    glGenTextures(1, &object);
    if (!object) { texture_pack_image_free(&image); return 0; }
    tex_bind(target, object);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    while (glGetError() != GL_NO_ERROR) {}
    for (face = 0; face < image.faces; ++face) for (level = skip; level < image.levels; ++level) {
        GLenum face_target = image.faces == 6 ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : target;
        unsigned w, h; size_t bytes;
        const unsigned char *pixels = texture_pack_image_level(&image, face, level, &w, &h, &bytes);
        if (!pixels) goto fallback;
        if (internal_format)
            glCompressedTexImage2D(face_target, (GLint)(level - skip), internal_format,
                (GLsizei)w, (GLsizei)h, 0, (GLsizei)bytes, pixels);
        else
            glTexImage2D(face_target, (GLint)(level - skip), GL_RGBA8, (GLsizei)w, (GLsizei)h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        if (glGetError() != GL_NO_ERROR) goto fallback;
    }
    *levels_out = image.levels - skip;
    ++texture_pack_uploads;
    texture_pack_image_free(&image);
    scope->owned_textures[scope->owned_count++] = object;
    scope->uploaded_bytes += total;
    *bytes_out = total;
    return object;
fallback:
    ++texture_pack_gl_fallbacks;
    tex_shadow_forget_name(object); glDeleteTextures(1, &object);
    while (glGetError() != GL_NO_ERROR) {}
    texture_pack_image_free(&image);
    return 0;
}

static GLuint upload_bitmap(struct mac_gpu_texture_scope *scope,
    const void *resource, unsigned char sampler, int meter, unsigned *levels_out,
    size_t *bytes_out, const char **error)
{
    unsigned faces, face, level, expected_levels, skip;
    unsigned long width = 0, height = 0, depth = 1;
    size_t total = 0, bytes;
    const unsigned char *pixels;
    uint32_t w, h, d;
    GLenum target;
    GLuint object = 0;

    expected_levels = (((const D3DBaseTexture *)resource)->Format &
        D3DFORMAT_MIPMAP_MASK) >> D3DFORMAT_MIPMAP_SHIFT;
    if (!expected_levels || expected_levels > 16) {
        set_error(error, "native bitmap declares an invalid mip count"); return 0;
    }
    /* a meter's bitmap is decoded, to swap its channels (below) */
    meter = meter && sampler == MAC_SAMPLER_2D;
    if (!meter) {
        texture_pack_dump_stock(resource, sampler, expected_levels);
        object = upload_bitmap_override(scope, resource, sampler, expected_levels, bytes_out, levels_out);
        if (object) return object;
        object = upload_bitmap_compressed(scope, resource, sampler, expected_levels, bytes_out, levels_out);
    }
    if (object) return object;
    faces = sampler == MAC_SAMPLER_CUBE ? 6U : 1U;
    target = sampler == MAC_SAMPLER_CUBE ? GL_TEXTURE_CUBE_MAP :
        sampler == MAC_SAMPLER_3D ? GL_TEXTURE_3D : GL_TEXTURE_2D;
    for (face = 0; face < faces; ++face) {
        unsigned found = 0;
        for (level = 0; level < expected_levels; ++level) {
            w = h = 0; d = 1; bytes = 0;
            pixels = sampler == MAC_SAMPLER_3D ?
                mac_native_bitmap_cache_rgba_volume(resource, level, &w, &h, &d, &bytes) :
                mac_native_bitmap_cache_rgba_face(resource, cube_tag_face(sampler, face), level, &w, &h, &bytes);
            if (!pixels) {
                set_error(error, "native bitmap declared mip is unavailable"); return 0;
            }
            if (!w || !h || !d || w > 4096 || h > 4096 || d > 512 ||
                (size_t)w > SIZE_MAX / (size_t)h / 4 ||
                (size_t)w * h * 4 > SIZE_MAX / d || bytes != (size_t)w * h * d * 4) {
                set_error(error, "native bitmap RGBA mip has invalid dimensions or byte count"); return 0;
            }
            if (!face && !found) { width = w; height = h; depth = d; }
            if (w != mip_dimension(width, level) || h != mip_dimension(height, level) ||
                d != mip_dimension(depth, level)) {
                set_error(error, "native bitmap face/mip dimensions disagree"); return 0;
            }
            if (total > SIZE_MAX - bytes) { set_error(error, "native texture size overflows"); return 0; }
            total += bytes; ++found;
        }
        if (found != expected_levels) {
            set_error(error, "native bitmap has an empty/inconsistent face mip chain"); return 0;
        }
    }
    skip = sampler == MAC_SAMPLER_3D ? 0 :
        texture_levels_to_skip(expected_levels, (uint32_t)width, (uint32_t)height);
    for (level = 0; level < skip; ++level) {
        size_t level_bytes = (size_t)mip_dimension(width, level) * mip_dimension(height, level) * 4 * faces;
        total = total > level_bytes ? total - level_bytes : 0;
    }
    if (total > MAC_GPU_TEXTURE_BYTES_LIMIT ||
        scope->uploaded_bytes > MAC_GPU_TEXTURE_BYTES_LIMIT - total) {
        set_error(error, "native per-draw texture uploads exceed bounded memory"); return 0;
    }
    glGenTextures(1, &object);
    if (!object) { set_error(error, "OpenGL texture allocation failed"); return 0; }
    tex_bind(target, object);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (face = 0; face < faces; ++face) {
        for (level = skip; level < expected_levels; ++level) {
            GLenum face_target = target == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : target;
            w = h = 0; d = 1; bytes = 0;
            pixels = sampler == MAC_SAMPLER_3D ?
                mac_native_bitmap_cache_rgba_volume(resource, level, &w, &h, &d, &bytes) :
                mac_native_bitmap_cache_rgba_face(resource, cube_tag_face(sampler, face), level, &w, &h, &bytes);
            if (!pixels || w != mip_dimension(width, level) ||
                h != mip_dimension(height, level) || d != mip_dimension(depth, level) ||
                bytes != (size_t)w * h * d * 4) {
                tex_shadow_forget_name(object); glDeleteTextures(1, &object);
                set_error(error, "native bitmap mip disappeared during upload"); return 0;
            }
            if (sampler == MAC_SAMPLER_3D)
                glTexImage3D(target, (GLint)level, GL_RGBA8, (GLsizei)w,
                    (GLsizei)h, (GLsizei)d, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            else if (meter) {
                /* Halo PC: fill order in alpha, shape in red; the meter shader
                   reads the order in color and the shape in alpha
                   (xbox_textures.c: custom_edition_channel_sources) */
                unsigned char *swapped = (unsigned char *)malloc(bytes);
                size_t texel;
                if (!swapped) {
                    tex_shadow_forget_name(object); glDeleteTextures(1, &object);
                    set_error(error, "native meter bitmap allocation failed"); return 0;
                }
                for (texel = 0; texel < bytes; texel += 4) {
                    swapped[texel] = swapped[texel + 1] = swapped[texel + 2] = pixels[texel + 3];
                    swapped[texel + 3] = pixels[texel];
                }
                glTexImage2D(face_target, (GLint)(level - skip), GL_RGBA8, (GLsizei)w,
                    (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, swapped);
                free(swapped);
            } else
                glTexImage2D(face_target, (GLint)(level - skip), GL_RGBA8, (GLsizei)w,
                    (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        }
    }
    if (glGetError() != GL_NO_ERROR) {
        tex_shadow_forget_name(object); glDeleteTextures(1, &object); set_error(error, "OpenGL rejected native bitmap mip upload"); return 0;
    }
    if (scope->owned_count >= D3DTSS_MAXSTAGES) {
        tex_shadow_forget_name(object); glDeleteTextures(1, &object); set_error(error, "native texture scope capacity exceeded"); return 0;
    }
    scope->owned_textures[scope->owned_count++] = object;
    scope->uploaded_bytes += total;
    *levels_out = expected_levels - skip; *bytes_out = total;
    return object;
}

/* Decode D3D-created A4R4G4B4 resources with the checked CPU-only slice of
   xbox_textures.c. This keeps swizzle, volume, mip, and face layout semantics
   shared with the native ports without borrowing their persistent GL cache. */
static GLuint upload_shared_texture(struct mac_gpu_texture_scope *scope,
    const void *resource, GLenum *target_out,
    unsigned char *sampler_out, unsigned *levels_out, size_t *bytes_out,
    float scale_out[4], const char **error)
{
    const D3DBaseTexture *native = (const D3DBaseTexture *)resource;
    struct xgpu_texture_description description;
    const unsigned char *source_bytes;
    unsigned long face_size, face_offset;
    unsigned faces, face, level;
    unsigned long width, height, depth;
    size_t level_bytes, total = 0, texel_count;
    unsigned char *pixels;
    unsigned long *decoded;
    unsigned long texel;
    unsigned format;
    GLuint object;
    if (!native->Data) { set_error(error, "native texture has no data"); return 0; }
    mac_xgpu_texture_describe(native->Format, native->Size, &description);
    if (!texture_size_supported(&description) || !description.width || !description.height || !description.depth ||
        !description.levels || description.levels > 16 || description.width > 4096 ||
        description.height > 4096 || description.depth > 512) {
        set_error(error, "native texture dimensions/mip count exceed decoder bounds"); return 0;
    }
    format = (native->Format & D3DFORMAT_FORMAT_MASK) >> D3DFORMAT_FORMAT_SHIFT;
    if (format != 0x04 || description.compressed) {
        /* port: which one (a render target's texture the native targets do not hold, or a format of
           bitmap this path lacks), to name what to support */
        static unsigned reported;
        if (reported < 8) {
            ++reported;
            fprintf(stderr, "native D3D-created texture %p: format 0x%02x %lux%lux%lu levels %lu%s%s%s common 0x%08lx format word 0x%08lx size 0x%08lx data 0x%08lx\n",
                resource, format, (unsigned long)description.width, (unsigned long)description.height,
                (unsigned long)description.depth, (unsigned long)description.levels,
                description.compressed ? " compressed" : "", description.cube_map ? " cube" : "",
                description.linear ? " linear" : "", (unsigned long)native->Common,
                (unsigned long)native->Format, (unsigned long)native->Size, (unsigned long)native->Data);
        }
        set_error(error, "D3D-created native texture format is not yet supported"); return 0;
    }
    face_size = mac_xgpu_texture_face_size(&description);
    faces = description.cube_map ? 6U : 1U;
    if ((description.cube_map && (description.depth != 1 || description.width != description.height)) ||
        (!description.cube_map && description.depth > 1 && description.width > 512)) {
        set_error(error, "native cube/volume dimensions are inconsistent"); return 0;
    }
    if (description.levels > 1) {
        unsigned long largest = description.width;
        unsigned max_levels = 1;
        if (description.height > largest) largest = description.height;
        if (description.depth > largest) largest = description.depth;
        while (largest > 1) { largest >>= 1; ++max_levels; }
        if (description.levels > max_levels) {
            set_error(error, "native texture mip count exceeds its dimensions"); return 0;
        }
    }
    if (!face_size || face_size > MAC_GPU_TEXTURE_BYTES_LIMIT / faces ||
        !mac_resource_range_is_allocated(PLATFORM_PHYSICAL_TO_VIRTUAL(native->Data), face_size * faces)) {
        set_error(error, "native texture data is outside an owned allocation"); return 0;
    }
    source_bytes = (const unsigned char *)PLATFORM_PHYSICAL_TO_VIRTUAL(native->Data);
    *sampler_out = description.cube_map ? MAC_SAMPLER_CUBE : description.depth > 1 ? MAC_SAMPLER_3D : MAC_SAMPLER_2D;
    *target_out = description.cube_map ? GL_TEXTURE_CUBE_MAP : description.depth > 1 ? GL_TEXTURE_3D : GL_TEXTURE_2D;
    *levels_out = description.levels;
    if (description.linear) {
        scale_out[0] = 1.0f / (float)description.width;
        scale_out[1] = 1.0f / (float)description.height;
    }
    for (level = 0; level < description.levels; ++level) {
        width = description.width >> level; if (!width) width = 1;
        height = description.height >> level; if (!height) height = 1;
        depth = description.depth >> level; if (!depth) depth = 1;
        if ((size_t)width > SIZE_MAX / (size_t)height / 4 ||
            (size_t)width * height > SIZE_MAX / (size_t)depth / 4) {
            set_error(error, "decoded native mip byte count overflows"); return 0;
        }
        if ((size_t)width > SIZE_MAX / (size_t)height ||
            (size_t)width * (size_t)height > SIZE_MAX / (size_t)depth) {
            set_error(error, "native texel count overflows"); return 0;
        }
        texel_count = (size_t)width * (size_t)height * (size_t)depth;
        if (texel_count > SIZE_MAX / 4) {
            set_error(error, "native decoded mip byte count overflows"); return 0;
        }
        level_bytes = texel_count * 4;
        if (level_bytes > MAC_GPU_TEXTURE_BYTES_LIMIT / faces ||
            total > MAC_GPU_TEXTURE_BYTES_LIMIT / faces - level_bytes) {
            set_error(error, "decoded native texture exceeds per-draw memory bound"); return 0;
        }
        total += level_bytes;
    }
    if (total > MAC_GPU_TEXTURE_BYTES_LIMIT / faces ||
        scope->uploaded_bytes > MAC_GPU_TEXTURE_BYTES_LIMIT - total * faces) {
        set_error(error, "decoded native texture exceeds per-draw memory bound"); return 0;
    }
    object = 0;
    glGenTextures(1, &object);
    if (!object) { set_error(error, "OpenGL transient texture allocation failed"); return 0; }
    if (scope->owned_count >= D3DTSS_MAXSTAGES) {
        tex_shadow_forget_name(object); glDeleteTextures(1, &object); set_error(error, "native texture scope capacity exceeded"); return 0;
    }
    scope->owned_textures[scope->owned_count++] = object;
    tex_bind(*target_out, object);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (face = 0; face < faces; ++face) {
        face_offset = (unsigned long)face * face_size;
        for (level = 0; level < description.levels; ++level) {
            GLenum face_target = description.cube_map ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : *target_out;
            width = description.width >> level; if (!width) width = 1;
            height = description.height >> level; if (!height) height = 1;
            depth = description.depth >> level; if (!depth) depth = 1;
            texel_count = (size_t)width * (size_t)height * (size_t)depth;
            level_bytes = texel_count * 4;
            pixels = (unsigned char *)malloc(level_bytes);
            decoded = (unsigned long *)malloc(texel_count * sizeof(*decoded));
            if (!pixels || !decoded) {
                free(pixels); free(decoded);
                set_error(error, "native decoded mip allocation failed"); return 0;
            }
            {
                unsigned long level_offset = mac_xgpu_texture_level_offset(&description, level);
                unsigned long next_offset = mac_xgpu_texture_level_offset(&description, level + 1);
                if (level_offset > face_size || next_offset < level_offset || next_offset > face_size) {
                    free(pixels); free(decoded);
                    set_error(error, "native mip range exceeds its bounded face allocation"); return 0;
                }
                if ((size_t)(next_offset - level_offset) < (size_t)width * height * depth * 2U) {
                    free(pixels); free(decoded);
                    set_error(error, "native mip storage is shorter than A4R4G4B4 texels"); return 0;
                }
            }
            if (!mac_decode_level(&description, level,
                source_bytes + face_offset + mac_xgpu_texture_level_offset(&description, level),
                NULL, decoded)) {
                free(pixels); free(decoded);
                set_error(error, "shared Xbox CPU texture decoder failed"); return 0;
            }
            for (texel = 0; texel < texel_count; ++texel) {
                unsigned long argb = decoded[texel];
                pixels[texel * 4 + 0] = (unsigned char)(argb >> 16);
                pixels[texel * 4 + 1] = (unsigned char)(argb >> 8);
                pixels[texel * 4 + 2] = (unsigned char)argb;
                pixels[texel * 4 + 3] = (unsigned char)(argb >> 24);
            }
            if (*target_out == GL_TEXTURE_3D)
                glTexImage3D(*target_out, (GLint)level, GL_RGBA8, (GLsizei)width,
                    (GLsizei)height, (GLsizei)depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            else
                glTexImage2D(face_target, (GLint)level, GL_RGBA8, (GLsizei)width,
                    (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            free(pixels);
            free(decoded);
            if (glGetError() != GL_NO_ERROR) { set_error(error, "OpenGL failed uploading decoded native mip"); return 0; }
        }
    }
    *bytes_out = total * faces;
    scope->uploaded_bytes += total * faces;
    return object;
}


/* ---------- Xbox maps' textures
   An Xbox map's bitmaps are in the console's memory already: the engine's
   texture cache (source/cache/xbox_texture_cache.c) reads each into its pages
   and registers a hardware format for it, as on the console. They are swizzled
   or linear, uncompressed or DXT, as the console's hardware takes them. Each
   is made a GL texture once and kept, keyed by where its texels are; the
   pages are written only by cache_file_read, which announces them
   (memory_watch_prepare_write), so a texture is made again when its pages'
   generation has passed the one it was made at. */
extern int mac_xbox_mode(void);
#define XBOX_TEXTURE_ENTRIES 2048U
#define XBOX_TEXTURE_PROBES 8U
struct xbox_texture_entry {
    unsigned long data, generation, format_word, size_word, last_use, palette;
    long hud_asset; /* port: the high-res HUD texture standing for the bitmap, or -1 */
    int hud_on;     /* ... and whether it was drawn in its place */
    GLuint name;
    GLenum target;
    unsigned levels;
    size_t bytes;
    float scale[2];
    unsigned char sampler;
    unsigned char live;
    /* port: the sampler state written to the GL texture last; a draw whose state is the same
       writes nothing (as the retail cache's entries) */
    struct retail_sampler_key sampler_key;
    int sampler_key_valid;
    unsigned long total;   /* the bytes of its texels in the console's memory (all faces) */
    int palettized;
};
static struct xbox_texture_entry xbox_textures[XBOX_TEXTURE_ENTRIES];

/* ---- port: the high-res HUD (mac_hud_hires.c, display.high_res_hud) */
static const struct mac_gpu_hud_provider *hud_provider;
void mac_gpu_textures_hud_provider(const struct mac_gpu_hud_provider *provider)
{
    hud_provider = provider;
}
/* the texels reduced to half, a level of the chain: the average of 2x2 */
static unsigned char *hud_texels_halved(const unsigned char *texels, unsigned width, unsigned height,
    unsigned *half_width, unsigned *half_height)
{
    unsigned w = width > 1 ? width / 2 : 1, h = height > 1 ? height / 2 : 1, x, y, c;
    unsigned char *half = (unsigned char *)malloc((size_t)w * h * 4);
    if (!half) return NULL;
    for (y = 0; y < h; ++y) for (x = 0; x < w; ++x) {
        unsigned x0 = width > 1 ? 2 * x : 0, x1 = width > 1 ? 2 * x + 1 : 0;
        unsigned y0 = height > 1 ? 2 * y : 0, y1 = height > 1 ? 2 * y + 1 : 0;
        for (c = 0; c < 4; ++c)
            half[((size_t)y * w + x) * 4 + c] = (unsigned char)(((unsigned)texels[((size_t)y0 * width + x0) * 4 + c] +
                texels[((size_t)y0 * width + x1) * 4 + c] + texels[((size_t)y1 * width + x0) * 4 + c] +
                texels[((size_t)y1 * width + x1) * 4 + c] + 2) / 4);
    }
    *half_width = w; *half_height = h;
    return half;
}
/* the number of levels of a chain down to 1x1, and the bytes of the levels from skip */
static unsigned hud_chain(unsigned width, unsigned height, unsigned skip, size_t *bytes)
{
    unsigned levels = 1, w = width, h = height;
    size_t total = 0;
    while (w > 1 || h > 1) { w = w > 1 ? w / 2 : 1; h = h > 1 ? h / 2 : 1; ++levels; }
    for (w = width, h = height; skip--; ) { w = w > 1 ? w / 2 : 1; h = h > 1 ? h / 2 : 1; }
    for (; ; ) { total += (size_t)w * h * 4; if (w == 1 && h == 1) break; w = w > 1 ? w / 2 : 1; h = h > 1 ? h / 2 : 1; }
    *bytes = total;
    return levels;
}

static void xbox_texture_drop(struct xbox_texture_entry *entry)
{
    if (!entry->live) return;
    tex_shadow_forget_name(entry->name);
    glDeleteTextures(1, &entry->name);
    if (xbox_texture_bytes >= entry->bytes) xbox_texture_bytes -= entry->bytes; else xbox_texture_bytes = 0;
    memset(entry, 0, sizeof(*entry));
}
static void xbox_textures_drop_all(void)
{
    unsigned i;
    for (i = 0; i < XBOX_TEXTURE_ENTRIES; ++i) xbox_texture_drop(&xbox_textures[i]);
    xbox_texture_bytes = 0;
}
static unsigned xbox_texture_slot(unsigned long data, unsigned probe)
{
    /* (a texture's pages start on 16 KB: the product's low bits are all alike, so its high bits are used;
       with the low ones only 256 of the slots could be reached, and a map with more textures than that,
       b30 after a few hundred frames, made and dropped textures all the time) */
    return (unsigned)((((data >> 8) * 2654435761UL) >> 12) + probe) % XBOX_TEXTURE_ENTRIES;
}
/* a slot for data: the entry that is its own, else a free one, else the least
   recently used of the probes (never one used by the draw in progress) */
static struct xbox_texture_entry *xbox_texture_find(unsigned long data, int *found)
{
    struct xbox_texture_entry *free_entry = NULL, *oldest = NULL;
    unsigned probe;
    *found = 0;
    for (probe = 0; probe < XBOX_TEXTURE_PROBES; ++probe) {
        struct xbox_texture_entry *entry = &xbox_textures[xbox_texture_slot(data, probe)];
        if (entry->live && entry->data == data) { *found = 1; return entry; }
        if (!entry->live) { if (!free_entry) free_entry = entry; }
        else if (entry->last_use != xbox_texture_bind_serial && (!oldest || entry->last_use < oldest->last_use)) oldest = entry;
    }
    return free_entry ? free_entry : oldest;
}
static GLenum xbox_dxt_format(unsigned format)
{
    return format == 0x0c ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT :
        format == 0x0e ? GL_COMPRESSED_RGBA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
}
/* The texture of an Xbox hardware format: found, or made from its texels. Owned
   by the cache; 0 with *error set if it cannot be made. */
static GLuint xbox_texture_get(const void *resource, unsigned char sampler, unsigned stage,
    GLenum *target_out, unsigned *levels_out, float scale_out[4], struct xbox_texture_entry **entry_out,
    const char **error)
{
    const D3DBaseTexture *native = (const D3DBaseTexture *)resource;
    struct xgpu_texture_description description;
    struct xbox_texture_entry *entry;
    const unsigned char *source;
    unsigned long face_size, total, generation, face, level, faces;
    unsigned skip = 0, first_level;
    int found, as_dxt, hud_on = 0;
    long hud_asset = -1;
    unsigned char *hud_texels = NULL;
    unsigned hud_width = 0, hud_height = 0, hud_levels = 0, hud_skip = 0;
    GLenum target;
    GLuint object = 0;
    size_t gl_bytes = 0;
    const D3DCOLOR *palette = NULL;
    const struct mac_device_state *state = mac_device_state_get();
    if (!native->Data) { set_error(error, "native texture has no data"); return 0; }
    /* (the usual case first: a texture made already, its pages not written since, nothing to work out) */
    if (xbox_texture_simple < 0) { const char *v = getenv("HALO_MAC_XBOX_TEXTURE_SIMPLE"); xbox_texture_simple = v && v[0] == '1'; }
    entry = xbox_texture_find(native->Data, &found);
    if (!xbox_texture_simple && found && !entry->palettized && entry->format_word == native->Format && entry->size_word == native->Size &&
        entry->sampler == sampler &&
        (entry->hud_asset < 0 || entry->hud_on == (hud_provider && hud_provider->enabled())) &&
        mac_resource_range_generation(PLATFORM_PHYSICAL_TO_VIRTUAL(native->Data), entry->total) <= entry->generation) {
        ++xbox_texture_hits;
        entry->last_use = xbox_texture_bind_serial;
        *target_out = entry->target; *levels_out = entry->levels;
        scale_out[0] = entry->scale[0]; scale_out[1] = entry->scale[1];
        *entry_out = entry;
        return entry->name;
    }
    mac_xgpu_texture_describe(native->Format, native->Size, &description);
    if (!texture_size_supported(&description) || !description.width || !description.height || !description.depth ||
        !description.levels || description.levels > 16) {
        set_error(error, "native texture dimensions/mip count exceed decoder bounds"); return 0;
    }
    faces = description.cube_map ? 6UL : 1UL;
    if ((description.cube_map && (description.depth != 1 || description.width != description.height))) {
        set_error(error, "native cube/volume dimensions are inconsistent"); return 0;
    }
    face_size = mac_xgpu_texture_face_size(&description);
    if (!face_size || face_size > MAC_GPU_TEXTURE_BYTES_LIMIT / faces) {
        set_error(error, "native texture size exceeds the bound"); return 0;
    }
    total = face_size * faces;
    source = (const unsigned char *)PLATFORM_PHYSICAL_TO_VIRTUAL(native->Data);
    if (!mac_resource_range_is_allocated(source, total)) {
        set_error(error, "native texture data is outside an owned allocation"); return 0;
    }
    target = description.cube_map ? GL_TEXTURE_CUBE_MAP : description.depth > 1 ? GL_TEXTURE_3D : GL_TEXTURE_2D;
    generation = mac_resource_range_generation(source, total);
    /* a palettized texture (the P8 bump maps) reads the palette the game set for its stage */
    if (description.format == 0x0b && state && stage < D3DTSS_MAXSTAGES && state->palettes[stage] && state->palettes[stage]->Data)
        palette = (const D3DCOLOR *)PLATFORM_PHYSICAL_TO_VIRTUAL(state->palettes[stage]->Data);
    entry = xbox_texture_find(native->Data, &found);
    if (found && entry->palette == (unsigned long)palette && entry->format_word == native->Format && entry->size_word == native->Size &&
        entry->generation >= generation && entry->sampler == sampler &&
        (entry->hud_asset < 0 || entry->hud_on == (hud_provider && hud_provider->enabled()))) {
        ++xbox_texture_hits;
        entry->last_use = xbox_texture_bind_serial;
        *target_out = entry->target; *levels_out = entry->levels;
        scale_out[0] = entry->scale[0]; scale_out[1] = entry->scale[1];
        *entry_out = entry;
        return entry->name;
    }
    if (found) xbox_texture_drop(entry);
    /* port: a bitmap the high-res HUD has a texture for (the bitmap's pixels as that
       was drawn for): its texels in place of the bitmap's, whole levels down to 1x1 */
    hud_asset = -1;
    if (hud_provider && description.depth == 1 && !description.cube_map && description.format != 0x0b)
        hud_asset = hud_provider->find((unsigned long)source, description.width, description.height,
            description.levels > 1 ? mac_xgpu_texture_level_offset(&description, 1) : face_size);
    hud_on = hud_asset >= 0 && hud_provider->enabled();
    if (hud_on) {
        hud_texels = hud_provider->decode(hud_asset, &hud_width, &hud_height);
        if (hud_texels) {
            hud_levels = hud_chain(hud_width, hud_height, 0, &gl_bytes);
            hud_skip = texture_levels_to_skip(hud_levels, hud_width, hud_height);
            hud_levels = hud_chain(hud_width, hud_height, hud_skip, &gl_bytes) - hud_skip;
        } else { hud_on = 0; hud_asset = -1; }
    }
    as_dxt = !hud_on && description.compressed && description.depth == 1 && retail_s3tc_available();
    if (description.depth == 1 && !hud_on)
        skip = texture_levels_to_skip(description.levels, (uint32_t)description.width, (uint32_t)description.height);
    first_level = skip;
    /* room: the bytes it will hold (compressed as is, else four bytes a texel) */
    if (!hud_on) {
    for (level = first_level; level < description.levels; ++level) {
        unsigned long w = mip_dimension(description.width, (unsigned)level), h = mip_dimension(description.height, (unsigned)level);
        unsigned long d = mip_dimension(description.depth, (unsigned)level);
        gl_bytes += as_dxt ? (size_t)((w + 3) / 4) * ((h + 3) / 4) * (description.format == 0x0c ? 8U : 16U) : (size_t)w * h * d * 4;
    }
    gl_bytes *= faces;
    }
    /* room: the least recently used go, but never one this draw has bound */
    while (xbox_texture_bytes + gl_bytes > retail_texture_cache_bytes_limit) {
        struct xbox_texture_entry *victim = NULL;
        unsigned i;
        for (i = 0; i < XBOX_TEXTURE_ENTRIES; ++i)
            if (xbox_textures[i].live && xbox_textures[i].last_use != xbox_texture_bind_serial &&
                (!victim || xbox_textures[i].last_use < victim->last_use)) victim = &xbox_textures[i];
        if (!victim) break;
        xbox_texture_drop(victim);
        ++xbox_texture_evictions;
    }
    entry = xbox_texture_find(native->Data, &found);
    if (!entry) { set_error(error, "native Xbox texture table is full"); return 0; }
    if (entry->live) { xbox_texture_drop(entry); ++xbox_texture_evictions; }
    glGenTextures(1, &object);
    if (!object) { set_error(error, "OpenGL texture allocation failed"); return 0; }
    tex_bind(target, object);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (hud_on) {
        /* the texels, and a level at a time the average of the one above, to 1x1 */
        const unsigned char *texels = hud_texels;
        unsigned char *owned = NULL;
        unsigned w = hud_width, h = hud_height, upload_level = 0, step;
        for (step = 0; ; ++step) {
            if (step >= hud_skip) {
                glTexImage2D(GL_TEXTURE_2D, (GLint)upload_level, GL_RGBA8, (GLsizei)w, (GLsizei)h, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, texels);
                ++upload_level;
            }
            if (w == 1 && h == 1) break;
            {
                unsigned half_width, half_height;
                unsigned char *half = hud_texels_halved(texels, w, h, &half_width, &half_height);
                free(owned);
                if (!half) { free(hud_texels); tex_shadow_forget_name(object); glDeleteTextures(1, &object);
                    set_error(error, "high-res HUD level allocation failed"); return 0; }
                owned = half; texels = half; w = half_width; h = half_height;
            }
        }
        free(owned);
        free(hud_texels); hud_texels = NULL;
        if (glGetError() != GL_NO_ERROR) {
            tex_shadow_forget_name(object); glDeleteTextures(1, &object);
            set_error(error, "OpenGL rejected a high-res HUD texture"); return 0;
        }
        ++hud_uploads; hud_bytes += gl_bytes;
    } else
    for (face = 0; face < faces; ++face) {
        for (level = first_level; level < description.levels; ++level) {
            GLenum face_target = description.cube_map ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + (GLenum)face : target;
            unsigned long width = mip_dimension(description.width, (unsigned)level);
            unsigned long height = mip_dimension(description.height, (unsigned)level);
            unsigned long depth = mip_dimension(description.depth, (unsigned)level);
            unsigned long level_offset = mac_xgpu_texture_level_offset(&description, (unsigned)level);
            unsigned long next_offset = mac_xgpu_texture_level_offset(&description, (unsigned)level + 1);
            const unsigned char *texels = source + face * face_size + level_offset;
            if (level_offset > face_size || next_offset < level_offset || next_offset > face_size) {
                tex_shadow_forget_name(object); glDeleteTextures(1, &object);
                set_error(error, "native mip range exceeds its bounded face"); return 0;
            }
            if (as_dxt) {
                glCompressedTexImage2D(face_target, (GLint)(level - first_level), xbox_dxt_format(description.format),
                    (GLsizei)width, (GLsizei)height, 0, (GLsizei)(next_offset - level_offset), texels);
            } else {
                size_t texel_count = (size_t)width * height * depth, texel;
                unsigned long *decoded;
                unsigned char *pixels;
                if (description.compressed) {
                    /* (no S3TC on the card: decoded on the CPU from the blocks) */
                    tex_shadow_forget_name(object); glDeleteTextures(1, &object);
                    set_error(error, "native compressed texture needs GL_EXT_texture_compression_s3tc"); return 0;
                }
                if ((size_t)(next_offset - level_offset) < texel_count * (description.linear ? 1U : 1U)) {
                    tex_shadow_forget_name(object); glDeleteTextures(1, &object);
                    set_error(error, "native mip storage is shorter than its texels"); return 0;
                }
                pixels = (unsigned char *)malloc(texel_count * 4);
                decoded = (unsigned long *)malloc(texel_count * sizeof(*decoded));
                if (!pixels || !decoded) {
                    free(pixels); free(decoded); tex_shadow_forget_name(object); glDeleteTextures(1, &object);
                    set_error(error, "native decoded mip allocation failed"); return 0;
                }
                if (!mac_decode_level(&description, (unsigned long)level, texels, palette, decoded)) {
                    free(pixels); free(decoded); tex_shadow_forget_name(object); glDeleteTextures(1, &object);
                    set_error(error, "shared Xbox CPU texture decoder failed"); return 0;
                }
                for (texel = 0; texel < texel_count; ++texel) {
                    unsigned long argb = decoded[texel];
                    pixels[texel * 4 + 0] = (unsigned char)(argb >> 16);
                    pixels[texel * 4 + 1] = (unsigned char)(argb >> 8);
                    pixels[texel * 4 + 2] = (unsigned char)argb;
                    pixels[texel * 4 + 3] = (unsigned char)(argb >> 24);
                }
                if (target == GL_TEXTURE_3D)
                    glTexImage3D(target, (GLint)(level - first_level), GL_RGBA8, (GLsizei)width,
                        (GLsizei)height, (GLsizei)depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                else
                    glTexImage2D(face_target, (GLint)(level - first_level), GL_RGBA8, (GLsizei)width,
                        (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                free(pixels); free(decoded);
            }
            if (glGetError() != GL_NO_ERROR) {
                tex_shadow_forget_name(object); glDeleteTextures(1, &object);
                set_error(error, "OpenGL rejected an Xbox texture mip"); return 0;
            }
        }
    }
    memset(entry, 0, sizeof(*entry));
    entry->data = native->Data; entry->format_word = native->Format; entry->size_word = native->Size;
    entry->generation = generation; entry->name = object; entry->target = target;
    entry->levels = hud_on ? hud_levels : (unsigned)(description.levels - first_level); entry->bytes = gl_bytes;
    entry->hud_asset = hud_asset; entry->hud_on = hud_on;
    entry->sampler = sampler; entry->palette = (unsigned long)palette; entry->live = 1; entry->last_use = xbox_texture_bind_serial;
    entry->total = total; entry->palettized = description.format == 0x0b;
    entry->scale[0] = entry->scale[1] = 1.0f;
    if (description.linear) {
        entry->scale[0] = 1.0f / (float)description.width;
        entry->scale[1] = 1.0f / (float)description.height;
    }
    xbox_texture_bytes += gl_bytes;
    ++xbox_texture_uploads;
    if (as_dxt) ++xbox_texture_compressed;
    *target_out = target; *levels_out = entry->levels;
    scale_out[0] = entry->scale[0]; scale_out[1] = entry->scale[1];
    *entry_out = entry;
    return object;
}

struct prepared_sampler {
    GLenum u, v, w;
    GLint min_filter, mag_filter;
    GLfloat border[4];
    float lod_bias;
};

static int prepare_sampler(GLenum target, unsigned stage, unsigned levels,
    struct prepared_sampler *prepared, const char **error)
{
    DWORD *state = D3D__TextureState[stage];
    prepared->u = address_mode(state[D3DTSS_ADDRESSU]);
    prepared->v = address_mode(state[D3DTSS_ADDRESSV]);
    prepared->w = address_mode(state[D3DTSS_ADDRESSW]);
    if (!prepared->u || !prepared->v ||
        ((target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_3D) && !prepared->w) ||
        !sampler_filters(state[D3DTSS_MINFILTER], state[D3DTSS_MAGFILTER],
            state[D3DTSS_MIPFILTER], levels, &prepared->min_filter,
            &prepared->mag_filter)) {
        set_error(error, "native texture address/filter mode is unsupported"); return 0;
    }
    if (state[D3DTSS_MINFILTER] == D3DTEXF_ANISOTROPIC ||
        state[D3DTSS_MAGFILTER] == D3DTEXF_ANISOTROPIC || state[D3DTSS_MAXANISOTROPY] > 1) {
        set_error(error, "native anisotropic filtering is unsupported by this texture scope"); return 0;
    }
    if (state[D3DTSS_MAXMIPLEVEL] >= levels && retail_texture_skip_levels <= 0) {
        set_error(error, "native maximum mip LOD exceeds decoded chain"); return 0;
    }
    memcpy(&prepared->lod_bias, &state[D3DTSS_MIPMAPLODBIAS], sizeof(prepared->lod_bias));
    if (!finite_float(prepared->lod_bias) || prepared->lod_bias < -16.0f || prepared->lod_bias > 16.0f) {
        set_error(error, "native mip LOD bias is invalid"); return 0;
    }
    prepared->border[0] = ((state[D3DTSS_BORDERCOLOR] >> 16) & 255) / 255.0f;
    prepared->border[1] = ((state[D3DTSS_BORDERCOLOR] >> 8) & 255) / 255.0f;
    prepared->border[2] = (state[D3DTSS_BORDERCOLOR] & 255) / 255.0f;
    prepared->border[3] = ((state[D3DTSS_BORDERCOLOR] >> 24) & 255) / 255.0f;
    return 1;
}

/* A cache entry's parameters: only those that differ from what was last
   written to its object (base level is always 0 there). */
static int apply_entry_sampler(GLenum target, unsigned stage, unsigned levels,
    struct retail_texture_entry *entry, const struct prepared_sampler *prepared,
    const char **error)
{
    DWORD *state = D3D__TextureState[stage];
    GLint max_level = (GLint)(levels - 1);
    GLfloat min_lod = (GLfloat)(state[D3DTSS_MAXMIPLEVEL] < levels ? state[D3DTSS_MAXMIPLEVEL] : levels - 1);
    int all = !entry->applied_valid;
    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_CUBE_MAP && target != GL_TEXTURE_3D) {
        set_error(error, "native sampler target is unsupported"); return 0;
    }
#define ENTRY_PARAMETER(field, value, call) \
    if (all || entry->field != (value)) { call; entry->field = (value); ++retail_parameter_writes; } \
    else ++retail_parameter_skips;
    ENTRY_PARAMETER(applied_u, (GLint)prepared->u, glTexParameteri(target, GL_TEXTURE_WRAP_S, (GLint)prepared->u))
    ENTRY_PARAMETER(applied_v, (GLint)prepared->v, glTexParameteri(target, GL_TEXTURE_WRAP_T, (GLint)prepared->v))
    if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_3D) {
        ENTRY_PARAMETER(applied_w, (GLint)prepared->w, glTexParameteri(target, GL_TEXTURE_WRAP_R, (GLint)prepared->w))
    }
    ENTRY_PARAMETER(applied_min, prepared->min_filter, glTexParameteri(target, GL_TEXTURE_MIN_FILTER, prepared->min_filter))
    ENTRY_PARAMETER(applied_mag, prepared->mag_filter, glTexParameteri(target, GL_TEXTURE_MAG_FILTER, prepared->mag_filter))
    if (all) { glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0); ++retail_parameter_writes; }
    else ++retail_parameter_skips;
    ENTRY_PARAMETER(applied_max_level, max_level, glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, max_level))
    ENTRY_PARAMETER(applied_min_lod, min_lod, glTexParameterf(target, GL_TEXTURE_MIN_LOD, min_lod))
#undef ENTRY_PARAMETER
    if (all || memcmp(entry->applied_border, prepared->border, sizeof(entry->applied_border))) {
        glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, prepared->border);
        memcpy(entry->applied_border, prepared->border, sizeof(entry->applied_border));
        ++retail_parameter_writes;
    } else ++retail_parameter_skips;
    entry->applied_valid = 1;
    tex_bias(prepared->lod_bias);
    if (glGetError() != GL_NO_ERROR) {
        /* (the object's parameters are now unknown) */
        entry->applied_valid = 0;
        set_error(error, "OpenGL rejected native sampler state"); return 0;
    }
    return 1;
}

static int apply_prepared_sampler(GLenum target, unsigned stage, unsigned levels,
    int update_texture_object, const struct prepared_sampler *prepared,
    const char **error)
{
    DWORD *state = D3D__TextureState[stage];
    if (update_texture_object &&
        (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_3D)) {
        glTexParameteri(target, GL_TEXTURE_WRAP_S, (GLint)prepared->u);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, (GLint)prepared->v);
        if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_3D) glTexParameteri(target, GL_TEXTURE_WRAP_R, (GLint)prepared->w);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, prepared->min_filter);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, prepared->mag_filter);
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, (GLint)(levels - 1));
        glTexParameterf(target, GL_TEXTURE_MIN_LOD,
            (GLfloat)(state[D3DTSS_MAXMIPLEVEL] < levels ? state[D3DTSS_MAXMIPLEVEL] : levels - 1));
        glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, prepared->border);
    }
    tex_bias(prepared->lod_bias);
    /* (lean, with only the checked bias set: the draw checks errors after) */
    if ((!lean_texture_scope || update_texture_object) && glGetError() != GL_NO_ERROR) {
        set_error(error, "OpenGL rejected native sampler state"); return 0;
    }
    return 1;
}

static int apply_borrowed_sampler(GLenum target, unsigned stage, unsigned levels,
    GLuint name, const char **error)
{
    struct prepared_sampler prepared;
    struct borrowed_shadow *shadow = NULL;
    DWORD *state = D3D__TextureState[stage];
    GLint max_level = (GLint)(levels - 1);
    GLfloat min_lod = (GLfloat)(state[D3DTSS_MAXMIPLEVEL] < levels ? state[D3DTSS_MAXMIPLEVEL] : levels - 1);
    unsigned i;
    int all;
    if (!prepare_sampler(target, stage, levels, &prepared, error)) return 0;
    for (i = 0; i < BORROWED_SHADOW_SLOTS; ++i)
        if (borrowed_shadows[i].valid && borrowed_shadows[i].name == name &&
            borrowed_shadows[i].target == target) { shadow = &borrowed_shadows[i]; break; }
    if (!shadow) {
        for (i = 0; i < BORROWED_SHADOW_SLOTS && borrowed_shadows[i].valid; ++i) {}
        if (i == BORROWED_SHADOW_SLOTS) i = (unsigned)(name % BORROWED_SHADOW_SLOTS);
        shadow = &borrowed_shadows[i];
        memset(shadow, 0, sizeof(*shadow));
        shadow->name = name; shadow->target = target;
    }
    all = !shadow->valid;
#define SHADOW_PARAMETER(field, value, call) \
    if (all || shadow->field != (value)) { call; shadow->field = (value); ++retail_parameter_writes; } \
    else ++retail_parameter_skips;
    SHADOW_PARAMETER(u, (GLint)prepared.u, glTexParameteri(target, GL_TEXTURE_WRAP_S, (GLint)prepared.u))
    SHADOW_PARAMETER(v, (GLint)prepared.v, glTexParameteri(target, GL_TEXTURE_WRAP_T, (GLint)prepared.v))
    if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_3D) {
        SHADOW_PARAMETER(w, (GLint)prepared.w, glTexParameteri(target, GL_TEXTURE_WRAP_R, (GLint)prepared.w))
    }
    SHADOW_PARAMETER(min, prepared.min_filter, glTexParameteri(target, GL_TEXTURE_MIN_FILTER, prepared.min_filter))
    SHADOW_PARAMETER(mag, prepared.mag_filter, glTexParameteri(target, GL_TEXTURE_MAG_FILTER, prepared.mag_filter))
    if (all) glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
    SHADOW_PARAMETER(max_level, max_level, glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, max_level))
    SHADOW_PARAMETER(min_lod, min_lod, glTexParameterf(target, GL_TEXTURE_MIN_LOD, min_lod))
#undef SHADOW_PARAMETER
    if (all || memcmp(shadow->border, prepared.border, sizeof(shadow->border))) {
        glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, prepared.border);
        memcpy(shadow->border, prepared.border, sizeof(shadow->border));
    }
    shadow->valid = 1;
    tex_bias(prepared.lod_bias);
    if (glGetError() != GL_NO_ERROR) {
        shadow->valid = 0;
        set_error(error, "OpenGL rejected native sampler state"); return 0;
    }
    return 1;
}

static int apply_sampler(GLenum target, unsigned stage, unsigned levels,
    int update_texture_object, const char **error)
{
    struct prepared_sampler prepared;
    if (!prepare_sampler(target, stage, levels, &prepared, error)) return 0;
    return apply_prepared_sampler(target, stage, levels, update_texture_object,
        &prepared, error);
}

/* (HALO_MAC_LEAN_DRAW) Each draw binds every stage it samples and sets its
   LOD bias, and nothing else samples textures: the stages' previous bindings
   and LOD bias need not be saved or put back. */
void mac_gpu_textures_set_lean(int lean) { lean_texture_scope = lean != 0; }

void mac_gpu_textures_end(struct mac_gpu_texture_scope *scope)
{
    unsigned stage, index;
    if (!scope || !scope->active) return;
    if (scope->borrowed_count) {
        for (stage = 0; stage < D3DTSS_MAXSTAGES; ++stage)
            if (scope->changed_stages & (1U << stage)) break;
        if (stage < D3DTSS_MAXSTAGES)
            tex_active(stage);
    }
    for (index = 0; !lean_texture_scope && index < scope->borrowed_count; ++index) {
        GLenum target = scope->borrowed[index].target;
        glBindTexture(target, scope->borrowed[index].name);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, scope->borrowed[index].wrap_s);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, scope->borrowed[index].wrap_t);
        if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_3D)
            glTexParameteri(target, GL_TEXTURE_WRAP_R, scope->borrowed[index].wrap_r);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, scope->borrowed[index].min_filter);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, scope->borrowed[index].mag_filter);
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, scope->borrowed[index].base_level);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, scope->borrowed[index].max_level);
        glTexParameterf(target, GL_TEXTURE_MIN_LOD, scope->borrowed[index].min_lod);
        glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, scope->borrowed[index].border);
    }
    for (stage = 0; !lean_texture_scope && stage < D3DTSS_MAXSTAGES; ++stage) {
        if (!(scope->changed_stages & (1U << stage))) continue;
        glActiveTextureARB(GL_TEXTURE0_ARB + stage);
        glBindTexture(GL_TEXTURE_2D, (GLuint)scope->previous_bindings[stage][0]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, (GLuint)scope->previous_bindings[stage][1]);
        glBindTexture(GL_TEXTURE_3D, (GLuint)scope->previous_bindings[stage][2]);
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, scope->previous_lod_bias[stage]);
    }
    for (index = 0; index < scope->owned_count; ++index) {
        tex_shadow_forget_name(scope->owned_textures[index]);
        glDeleteTextures(1, &scope->owned_textures[index]);
        ++transient_uploads;
    }
    if (scope->owned_count) transient_upload_bytes += (unsigned long)scope->uploaded_bytes;
    for (index = 0; index < scope->cache_entry_count; ++index) {
        unsigned cache_index = scope->cache_entries[index];
        if (cache_index < MAC_GPU_RETAIL_CACHE_ENTRY_LARGE &&
            retail_texture_cache[cache_index].live && retail_texture_cache[cache_index].pins)
            --retail_texture_cache[cache_index].pins;
    }
    /* (lean: only an upload changed the unpack alignment) */
    if (!lean_texture_scope || scope->uploaded_bytes || scope->owned_count) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, scope->previous_unpack_alignment);
        glPixelStorei(GL_PACK_ALIGNMENT, scope->previous_pack_alignment);
    }
    if (lean_texture_scope) tex_active((unsigned)(scope->previous_active_texture - GL_TEXTURE0_ARB));
    else {
        glActiveTextureARB((GLenum)scope->previous_active_texture);
        mac_gpu_textures_shadow_reset();
    }
    memset(scope, 0, sizeof(*scope));
}

/* (lean) What each stage last bound from the retail cache, with the stage
   state it was bound for: a draw that samples the same texture with the same
   state again only pins the entry and binds its object, once the entry is
   found still live and still keyed for that state (another stage may have
   re-keyed it in between). */
struct stage_memo {
    int valid;
    const void *texture;
    unsigned char sampler;
    unsigned long generation;
    DWORD state[D3DTSS_MAX];
    unsigned cache_index;
    GLuint object;
    GLenum target;
    struct retail_sampler_key key;
    float lod_bias;
    /* port: an Xbox map's texture (the entry in xbox_textures; the texels' pages and size, whose
       generation says whether the texture is still current) */
    struct xbox_texture_entry *xbox;
    const void *xbox_source;
    unsigned long xbox_total;
};
static struct stage_memo stage_memos[D3DTSS_MAXSTAGES];

int mac_gpu_textures_bind(struct mac_gpu_texture_scope *scope,
    const void *textures[D3DTSS_MAXSTAGES], DWORD texture_modes,
    unsigned char sampler_type[D3DTSS_MAXSTAGES],
    float texture_scale[D3DTSS_MAXSTAGES][4], const char **error)
{
    unsigned stage, texture_id, cache_index, old_owned_count;
    unsigned long owner_generation;
    DWORD mode;
    GLenum target, borrowed_target;
    GLint min_filter, mag_filter;
    unsigned levels, borrowed_levels;
    int borrowed_result, persistent_retail, retail_cache_hit, retail_sampler_prepared;
    int borrowed_stage;
    size_t uploaded_bytes;
    GLuint object;
    struct xbox_texture_entry *xbox_stage_entry;
    if (error) *error = NULL;
    if (!scope || !textures || !sampler_type || !texture_scale) {
        set_error(error, "native texture scope arguments are null"); return 0;
    }
    if (!retail_texture_cache_configure(error)) return 0;
    memset(scope, 0, sizeof(*scope));
    ++xbox_texture_bind_serial;
    owner_generation = mac_native_bitmap_cache_generation();
    if (!retail_texture_generation) retail_texture_generation = owner_generation;
    else if (retail_texture_generation != owner_generation) {
        if (!mac_gpu_textures_bitmap_cache_invalidate()) {
            set_error(error, "retail bitmap cache generation changed during a draw");
            return 0;
        }
    }
    if (lean_texture_scope) {
        /* (lean: nothing else changes these; the draw checks errors after) */
        scope->previous_active_texture = GL_TEXTURE0_ARB;
        scope->previous_unpack_alignment = 4;
        scope->previous_pack_alignment = 4;
    } else {
        while (glGetError() != GL_NO_ERROR) {}
        glGetIntegerv(GL_ACTIVE_TEXTURE, &scope->previous_active_texture);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &scope->previous_unpack_alignment);
        glGetIntegerv(GL_PACK_ALIGNMENT, &scope->previous_pack_alignment);
    }
    for (stage = 0; stage < D3DTSS_MAXSTAGES; ++stage) {
        mode = (texture_modes >> (5 * stage)) & 0x1f;
        if (texture_mode_has_sampler(mode))
            scope->changed_stages |= 1U << stage;
    }
    for (stage = 0; !lean_texture_scope && stage < D3DTSS_MAXSTAGES; ++stage) {
        if (!(scope->changed_stages & (1U << stage))) continue;
        glActiveTextureARB(GL_TEXTURE0_ARB + stage);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &scope->previous_bindings[stage][0]);
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &scope->previous_bindings[stage][1]);
        glGetIntegerv(GL_TEXTURE_BINDING_3D, &scope->previous_bindings[stage][2]);
        glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, &scope->previous_lod_bias[stage]);
    }
    scope->active = 1;
    for (stage = 0; stage < D3DTSS_MAXSTAGES; ++stage) {
        mode = (texture_modes >> (5 * stage)) & 0x1f;
        sampler_type[stage] = MAC_SAMPLER_NONE;
        texture_scale[stage][0] = texture_scale[stage][1] = texture_scale[stage][2] = texture_scale[stage][3] = 1.0f;
        if (mode > MAC_TEXTURE_MODE_MAX) {
            set_error(error, "native texture mode is outside the known NV2A mode range"); goto fail;
        }
        if (!texture_mode_has_sampler(mode)) continue;
        if (!lean_texture_scope) tex_active(stage);
        if (!textures[stage]) { set_error(error, "active native texture stage has no resource"); goto fail; }
        {
            const D3DBaseTexture *native = (const D3DBaseTexture *)textures[stage];
            unsigned dimension = (native->Format & D3DFORMAT_DIMENSION_MASK) >> D3DFORMAT_DIMENSION_SHIFT;
            if (!native->Data) { set_error(error, "active native texture has no data storage"); goto fail; }
            if (dimension < 2 || dimension > 3) { set_error(error, "native texture has an unsupported dimension code"); goto fail; }
            sampler_type[stage] = (native->Format & D3DFORMAT_CUBEMAP) ? MAC_SAMPLER_CUBE : dimension == 3 ? MAC_SAMPLER_3D : MAC_SAMPLER_2D;
        }
        if (lean_texture_scope) {
            struct stage_memo *memo = &stage_memos[stage];
            unsigned char memo_sampler = (unsigned char)(sampler_type[stage] |
                (stage == 0 && gpu_meter_draw && sampler_type[stage] == MAC_SAMPLER_2D ? 0x40 : 0));
            /* port: an Xbox map's texture bound by the previous draw with the same state, its
               pages not written since: nothing to look up or set but the binding and the bias */
            if (memo->valid && memo->xbox && memo->texture == textures[stage] && memo->sampler == sampler_type[stage] &&
                !memcmp(memo->state, D3D__TextureState[stage], sizeof(memo->state))) {
                struct xbox_texture_entry *entry = memo->xbox;
                const D3DBaseTexture *native = (const D3DBaseTexture *)textures[stage];
                if (entry->live && entry->name == memo->object && entry->data == native->Data &&
                    entry->format_word == native->Format && entry->size_word == native->Size &&
                    (entry->hud_asset < 0 || entry->hud_on == (hud_provider && hud_provider->enabled())) &&
                    mac_resource_range_generation(memo->xbox_source, memo->xbox_total) <= entry->generation) {
                    entry->last_use = xbox_texture_bind_serial;
                    ++xbox_texture_hits; ++xbox_texture_memo_hits;
                    texture_scale[stage][0] = entry->scale[0]; texture_scale[stage][1] = entry->scale[1];
                    tex_bind_unit(stage, entry->target, entry->name);
                    tex_bias_unit(stage, memo->lod_bias);
                    continue;
                }
            }
            if (memo->valid && !memo->xbox && memo->texture == textures[stage] &&
                memo->sampler == sampler_type[stage] && memo->generation == owner_generation &&
                memo->cache_index < retail_texture_cache_entry_limit &&
                !memcmp(memo->state, D3D__TextureState[stage], sizeof(memo->state))) {
                struct retail_texture_entry *entry = &retail_texture_cache[memo->cache_index];
                if (entry->live && entry->hardware == textures[stage] && entry->sampler == memo_sampler && entry->name == memo->object &&
                    entry->target == memo->target && entry->generation == owner_generation &&
                    retail_sampler_key_equal(&entry->sampler_key, &memo->key)) {
                    if (!retail_texture_cache_pin(scope, memo->cache_index)) {
                        set_error(error, "retail texture scope cache pin limit exceeded");
                        goto fail;
                    }
                    entry->last_use = ++retail_texture_use_serial;
                    ++retail_texture_hits; ++stage_memo_hits;
                    tex_bind_unit(stage, memo->target, memo->object);
                    tex_bias_unit(stage, memo->lod_bias);
                    continue;
                }
            }
            memo->valid = 0;
            tex_active(stage);
        }
        texture_id = 0;
        persistent_retail = 0;
        retail_cache_hit = 0;
        retail_sampler_prepared = 0;
        borrowed_stage = 0;
        xbox_stage_entry = NULL;
        if (sampler_type[stage] == MAC_SAMPLER_2D &&
            is_attached_render_target(textures[stage], &texture_id)) {
            set_error(error, "native texture stage samples the currently attached render target"); goto fail;
        }
        target = sampler_type[stage] == MAC_SAMPLER_CUBE ? GL_TEXTURE_CUBE_MAP :
            sampler_type[stage] == MAC_SAMPLER_3D ? GL_TEXTURE_3D : GL_TEXTURE_2D;
        if (texture_id) {
            object = (GLuint)texture_id;
            borrowed_stage = 1;
            tex_bind(target, object);
            save_borrowed_parameters(scope, object, target, stage);
            if (scope->active < 0) { set_error(error, "same render target uses incompatible stage sampler states"); goto fail; }
            levels = 1;
            if (textures[stage] && ((const D3DBaseTexture *)textures[stage])->Size) {
                const D3DBaseTexture *native = (const D3DBaseTexture *)textures[stage];
                unsigned long width = (native->Size & D3DSIZE_WIDTH_MASK) + 1;
                unsigned long height = ((native->Size & D3DSIZE_HEIGHT_MASK) >> D3DSIZE_HEIGHT_SHIFT) + 1;
                texture_scale[stage][0] = 1.0f / (float)width;
                texture_scale[stage][1] = 1.0f / (float)height;
            }
        } else {
            borrowed_result = 0;
            object = 0;
            borrowed_target = target;
            borrowed_levels = 0;
            if (borrowed_resolver) {
                if (error) *error = NULL;
                borrowed_result = borrowed_resolver(borrowed_resolver_context,
                    textures[stage], sampler_type[stage], &object, &borrowed_target,
                    &borrowed_levels, texture_scale[stage], error);
                if (borrowed_result < 0) {
                    if (error && !*error)
                        set_error(error, "native borrowed texture resolver failed");
                    goto fail;
                }
                if (borrowed_result > 1) {
                    set_error(error, "native borrowed texture resolver returned an invalid status");
                    goto fail;
                }
                if (borrowed_result == 0 && error) *error = NULL;
                if (borrowed_result > 0) {
                    borrowed_stage = 1;
                    if (error) *error = NULL;
                    if (!object || borrowed_target != target || !borrowed_levels || borrowed_levels > 16) {
                        set_error(error, "native borrowed texture description is invalid");
                        goto fail;
                    }
                    if (!glIsTexture(object)) {
                        set_error(error, "native borrowed texture name is not a live GL object");
                        goto fail;
                    }
                    for (texture_id = 0; texture_id < 4; ++texture_id)
                        if (!finite_float(texture_scale[stage][texture_id])) {
                            set_error(error, "native borrowed texture scale is not finite");
                            goto fail;
                        }
                    levels = borrowed_levels;
                    tex_bind(borrowed_target, object);
                    save_borrowed_parameters(scope, object, borrowed_target, stage);
                    if (scope->active < 0) {
                        set_error(error, "same borrowed texture uses incompatible stage sampler states");
                        goto fail;
                    }
                }
            }
            if (!borrowed_result && mac_xbox_mode()) {
                /* an Xbox map's bitmaps: in the console's memory, as its hardware takes them */
                struct xbox_texture_entry *xbox_entry = NULL;
                struct retail_sampler_key xbox_key;
                object = xbox_texture_get(textures[stage], sampler_type[stage], stage, &target,
                    &levels, texture_scale[stage], &xbox_entry, error);
                if (!object) goto fail;
                tex_bind(target, object);
                /* the texture's own sampler state is written only when the game's has changed */
                retail_sampler_key_for(D3D__TextureState[stage], target, &xbox_key);
                persistent_retail = 1;
                if (!xbox_texture_simple && xbox_entry->sampler_key_valid && retail_sampler_key_equal(&xbox_entry->sampler_key, &xbox_key))
                    retail_cache_hit = 1;
                else {
                    xbox_entry->sampler_key = xbox_key; xbox_entry->sampler_key_valid = 1;
                    retail_cache_hit = 0;
                }
                xbox_stage_entry = xbox_texture_simple ? NULL : xbox_entry;
            } else if (!borrowed_result) {
                /* (the cache holds only the native bitmaps' textures, made
                   while they were contained: other textures skip its scan) */
                int contained = mac_native_bitmap_cache_contains(textures[stage]);
                /* a meter's draw has its own texture of a bitmap (channels
                   swapped), apart from a static's of the same bitmap */
                int meter = stage == 0 && gpu_meter_draw && sampler_type[stage] == MAC_SAMPLER_2D;
                unsigned char cache_sampler = (unsigned char)(sampler_type[stage] | (meter ? 0x40 : 0));
                if (contained && retail_texture_cache_find(textures[stage], cache_sampler,
                    target, D3D__TextureState[stage], owner_generation,
                    &cache_index)) {
                    struct retail_texture_entry *entry = &retail_texture_cache[cache_index];
                    ++retail_texture_hits;
                    entry->last_use = ++retail_texture_use_serial;
                    levels = entry->levels;
                    object = entry->name;
                    retail_cache_hit = 1;
                    if (!retail_texture_cache_pin(scope, cache_index)) {
                        set_error(error, "retail texture scope cache pin limit exceeded");
                        goto fail;
                    }
                    persistent_retail = 1;
                } else if (contained) {
                    if (retail_texture_cache_find_reusable(textures[stage],
                        cache_sampler, target, owner_generation,
                        &cache_index)) {
                        struct retail_texture_entry *entry = &retail_texture_cache[cache_index];
                        struct prepared_sampler prepared;
                        levels = entry->levels;
                        if (!prepare_sampler(target, stage, levels, &prepared, error))
                            goto fail;
                        object = entry->name;
                        tex_bind(target, object);
                        if (!apply_entry_sampler(target, stage, levels, entry,
                            &prepared, error)) {
                            /* GL may have accepted only some parameter writes.
                               Drop the unpinned object instead of retaining a
                               key that no longer describes its state. */
                            retail_texture_cache_drop(cache_index, 0);
                            goto fail;
                        }
                        {
                            /* Diagnostic only (HALO_MAC_DIAG_SAMPLER_REUSE=1):
                               the first re-keyings, old and new keys */
                            static int diag=-1;static unsigned logged;
                            struct retail_sampler_key next;
                            if(diag<0){const char *v=getenv("HALO_MAC_DIAG_SAMPLER_REUSE");diag=v&&v[0]=='1';}
                            retail_sampler_key_for(D3D__TextureState[stage],target,&next);
                            if(diag&&logged<60) {
                                ++logged;
                                fprintf(stderr,"HALO_MAC_SAMPLER_REUSE %p stage=%u old=%lu,%lu,%lu,%lu,%lu,%lu,%lu,%08lx new=%lu,%lu,%lu,%lu,%lu,%lu,%lu,%08lx\n",
                                    textures[stage],stage,
                                    (unsigned long)entry->sampler_key.address_u,(unsigned long)entry->sampler_key.address_v,(unsigned long)entry->sampler_key.address_w,
                                    (unsigned long)entry->sampler_key.min_filter,(unsigned long)entry->sampler_key.mag_filter,(unsigned long)entry->sampler_key.mip_filter,
                                    (unsigned long)entry->sampler_key.max_mip_level,(unsigned long)entry->sampler_key.border_color,
                                    (unsigned long)next.address_u,(unsigned long)next.address_v,(unsigned long)next.address_w,
                                    (unsigned long)next.min_filter,(unsigned long)next.mag_filter,(unsigned long)next.mip_filter,
                                    (unsigned long)next.max_mip_level,(unsigned long)next.border_color);
                            }
                        }
                        retail_sampler_key_for(D3D__TextureState[stage],
                            target, &entry->sampler_key);
                        entry->last_use = ++retail_texture_use_serial;
                        if (!retail_texture_cache_pin(scope, cache_index)) {
                            retail_texture_cache_drop(cache_index, 0);
                            set_error(error, "retail texture scope cache pin limit exceeded");
                            goto fail;
                        }
                        ++retail_texture_hits;
                        ++retail_texture_sampler_reuses;
                        retail_sampler_prepared = 1;
                        persistent_retail = 1;
                    } else {
                    ++retail_texture_misses;
                    old_owned_count = scope->owned_count;
                    object = upload_bitmap(scope, textures[stage], sampler_type[stage], meter,
                        &levels, &uploaded_bytes, error);
                    if (!object) goto fail;
                    if (uploaded_bytes <= retail_texture_cache_bytes_limit &&
                        scope->owned_count == old_owned_count + 1 &&
                        scope->owned_textures[old_owned_count] == object) {
                        if (!apply_sampler(target, stage, levels, 1, error)) goto fail;
                        retail_sampler_prepared = 1;
                        if (retail_texture_cache_make_room(uploaded_bytes, &cache_index)) {
                            struct retail_texture_entry *entry = &retail_texture_cache[cache_index];
                            memset(entry, 0, sizeof(*entry));
                            entry->hardware = textures[stage];
                            entry->generation = owner_generation;
                            entry->name = object;
                            entry->target = target;
                            entry->sampler = cache_sampler;
                            entry->levels = levels;
                            retail_sampler_key_for(D3D__TextureState[stage],
                                target, &entry->sampler_key);
                            entry->bytes = uploaded_bytes;
                            entry->last_use = ++retail_texture_use_serial;
                            entry->live = 1;
                            if (retail_texture_cache_pin(scope, cache_index)) {
                                retail_texture_cache_bytes += uploaded_bytes;
                                --scope->owned_count; /* transfer GL ownership to the cache */
                                persistent_retail = 1;
                            } else memset(entry, 0, sizeof(*entry));
                        }
                    }
                    }
                } else {
                    object = upload_shared_texture(scope, textures[stage], &target,
                        &sampler_type[stage], &levels, &uploaded_bytes, texture_scale[stage], error);
                }
                if (!object) goto fail;
                tex_bind(target, object);
            }
        }
        if (!sampler_filters(D3D__TextureState[stage][D3DTSS_MINFILTER],
            D3D__TextureState[stage][D3DTSS_MAGFILTER], D3D__TextureState[stage][D3DTSS_MIPFILTER],
            levels, &min_filter, &mag_filter)) {
            set_error(error, "native texture sampler filters are unsupported"); goto fail;
        }
        if (!retail_sampler_prepared) {
            if (lean_texture_scope && borrowed_stage) {
                if (!apply_borrowed_sampler(target, stage, levels, object, error)) goto fail;
            } else if (!apply_sampler(target, stage, levels,
                !(persistent_retail && retail_cache_hit), error)) goto fail;
        }
        if (!lean_texture_scope && glGetError() != GL_NO_ERROR) { set_error(error, "OpenGL failed binding a native texture stage"); goto fail; }
        if (lean_texture_scope && xbox_stage_entry && !borrowed_stage && !xbox_stage_entry->palette) {
            struct stage_memo *memo = &stage_memos[stage];
            const D3DBaseTexture *native = (const D3DBaseTexture *)textures[stage];
            struct xgpu_texture_description description;
            mac_xgpu_texture_describe(native->Format, native->Size, &description);
            memo->texture = textures[stage]; memo->sampler = sampler_type[stage];
            memcpy(memo->state, D3D__TextureState[stage], sizeof(memo->state));
            memo->object = xbox_stage_entry->name; memo->target = xbox_stage_entry->target;
            memcpy(&memo->lod_bias, &D3D__TextureState[stage][D3DTSS_MIPMAPLODBIAS], sizeof(memo->lod_bias));
            memo->xbox = xbox_stage_entry;
            memo->xbox_source = PLATFORM_PHYSICAL_TO_VIRTUAL(native->Data);
            memo->xbox_total = mac_xgpu_texture_face_size(&description) * (description.cube_map ? 6UL : 1UL);
            memo->valid = 1;
        } else if (lean_texture_scope && persistent_retail && !borrowed_stage &&
            cache_index < retail_texture_cache_entry_limit &&
            retail_texture_cache[cache_index].live && retail_texture_cache[cache_index].name == object) {
            struct stage_memo *memo = &stage_memos[stage];
            memo->texture = textures[stage]; memo->sampler = sampler_type[stage];
            memo->generation = owner_generation;
            memcpy(memo->state, D3D__TextureState[stage], sizeof(memo->state));
            memo->cache_index = cache_index; memo->object = object; memo->xbox = NULL;
            memo->target = retail_texture_cache[cache_index].target;
            memo->key = retail_texture_cache[cache_index].sampler_key;
            memcpy(&memo->lod_bias, &D3D__TextureState[stage][D3DTSS_MIPMAPLODBIAS], sizeof(memo->lod_bias));
            memo->valid = 1;
        }
    }
    return 1;
fail:
    mac_gpu_textures_end(scope);
    return 0;
}
