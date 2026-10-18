/* What the Mac build knows of the machine: the video memory and the CPU,
   and from them what the settings "auto" mean (mac.detail,
   mac.texture_cache_mb, mac.texture_skip_levels). */
#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mac_gpu_info.h"

/* port/linux/src/port_config.c, when linked */
extern const char *config_string(const char *name) __attribute__((weak));
extern long config_integer(const char *name) __attribute__((weak));

static int info_vram_mb;
static int info_resolved;
static char info_renderer[128];
static char info_map[64];
static unsigned long info_serial;
static int info_logged_detail;

int mac_gpu_info_is_ppc(void)
{
#if defined(__ppc__) || defined(__ppc64__)
    return 1;
#else
    return 0;
#endif
}

/* the largest video memory of the accelerated renderers of the main display */
static int vram_from_cgl(void)
{
    CGLRendererInfoObj info = 0;
    GLint count = 0, index, best = 0;
    if (CGLQueryRendererInfo(CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay), &info, &count) != kCGLNoError ||
        !info)
        return 0;
    for (index = 0; index < count; ++index) {
        GLint accelerated = 0, memory = 0;
        if (CGLDescribeRenderer(info, index, kCGLRPAccelerated, &accelerated) != kCGLNoError || !accelerated)
            continue;
        if (CGLDescribeRenderer(info, index, kCGLRPVideoMemory, &memory) != kCGLNoError || memory <= 0)
            continue;
        /* (the answer is in bytes; a value this small is already megabytes) */
        if (memory >= 1048576) memory /= 1048576;
        if (memory > best) best = memory;
    }
    CGLDestroyRendererInfo(info);
    return best;
}

static void info_resolve(void)
{
    const char *renderer = 0;
    int vram;
    if (info_resolved) return;
    if (CGLGetCurrentContext()) renderer = (const char *)glGetString(GL_RENDERER);
    if (renderer) {
        strncpy(info_renderer, renderer, sizeof(info_renderer) - 1);
        info_renderer[sizeof(info_renderer) - 1] = 0;
    }
    vram = vram_from_cgl();
    if (vram > 0) {
        info_vram_mb = vram;
    } else if (renderer) {
        info_vram_mb = mac_gpu_vram_from_name(renderer);
    } else {
        /* no context to ask yet: 32, and ask again later */
        return;
    }
    info_resolved = 1;
    ++info_serial;
    fprintf(stderr, "mac settings: %s, %d MB video memory, %s\n", info_renderer[0] ? info_renderer : "renderer unknown",
        info_vram_mb, mac_gpu_info_is_ppc() ? "PowerPC" : "Intel");
}

int mac_gpu_info_vram_mb(void)
{
    info_resolve();
    return info_resolved ? info_vram_mb : 32;
}

const char *mac_gpu_info_renderer(void)
{
    info_resolve();
    return info_renderer;
}

void mac_gpu_info_set_map(const char *name)
{
    size_t length;
    if (!name) name = "";
    length = strlen(name);
    if (length >= 4 && !strcmp(name + length - 4, ".map")) length -= 4;
    if (length >= sizeof(info_map)) length = sizeof(info_map) - 1;
    memcpy(info_map, name, length);
    info_map[length] = 0;
    ++info_serial;
}

int mac_gpu_info_campaign(void)
{
    const char *flag = getenv("HALO_MAC_CAMPAIGN");
    return (flag && flag[0] == '1') || mac_gpu_map_is_campaign(info_map);
}

unsigned long mac_gpu_info_serial(void)
{
    info_resolve();
    return info_serial;
}

/* A number in the environment, for measuring a preset's parts (HALO_MAC_PRESET_<name>); else the preset's */
static double info_preset_number(const char *name, double fallback)
{
    char variable[48];
    const char *value;
    sprintf(variable, "HALO_MAC_PRESET_%s", name);
    value = getenv(variable);
    return value && value[0] ? atof(value) : fallback;
}

int mac_gpu_info_detail(struct mac_detail_preset *preset)
{
    const char *mode = config_string ? config_string("mac.detail") : "auto";
    int level = MAC_DETAIL_HIGH;
    int campaign = mac_gpu_info_campaign();
    if (!mode || !mode[0] || !strcmp(mode, "auto")) {
        level = mac_gpu_pick_preset(mac_gpu_info_vram_mb(), mac_gpu_info_is_ppc());
        if (!info_logged_detail && info_resolved) {
            info_logged_detail = 1;
            fprintf(stderr, "mac settings: detail auto picks %s\n",
                level == MAC_DETAIL_LOW ? "low" : level == MAC_DETAIL_MEDIUM ? "medium" : "high");
        }
    } else if (!strcmp(mode, "medium")) {
        level = MAC_DETAIL_MEDIUM;
    } else if (!strcmp(mode, "low")) {
        level = MAC_DETAIL_LOW;
    } else if (!strcmp(mode, "lowest")) {
        level = MAC_DETAIL_LOWEST;
    }
    if (level == MAC_DETAIL_HIGH) return 0;
    preset->specular = preset->shadows = preset->decals = 0;
    preset->particles = level == MAC_DETAIL_LOWEST ? 2 : 1;
    preset->lod_scale = 1.0;
    preset->particle_pixels = 0.0;
    preset->particle_mode = 0;
    if (level == MAC_DETAIL_MEDIUM) {
        /* the PowerBook: the stock picture less the specular, shadows and decals,
           smaller scenery and the smallest of the effects' particles left out */
        preset->scenery_pixels = campaign ? 24.0 : 8.0;
        preset->object_pixels = 0.0;
        preset->lens_flares = 1;
    } else {
        /* in the campaign, whose levels hold hundreds of small plants and rocks and a
           crowd of objects (c10: 6 ms an iBook G4 frame at 64 and 12), more are left out */
        preset->scenery_pixels = campaign ? 64.0 : 16.0;
        preset->object_pixels = campaign ? 12.0 : 6.0;
        preset->lens_flares = 0;
        /* a particle (dust, smoke) is drawn no wider than 160 pixels: one close to the camera
           covers the screen, more pixels than a Radeon 9550 fills in a frame (4.5 ms on Damnation) */
        preset->particle_pixels = 160.0;
    }
    preset->scenery_pixels = info_preset_number("SCENERY", preset->scenery_pixels);
    preset->object_pixels = info_preset_number("OBJECT", preset->object_pixels);
    preset->lod_scale = info_preset_number("LOD", preset->lod_scale);
    preset->lens_flares = info_preset_number("FLARES", preset->lens_flares) != 0.0;
    preset->particle_pixels = info_preset_number("PARTICLE", preset->particle_pixels);
    preset->particle_mode = (int)info_preset_number("PARTICLE_MODE", preset->particle_mode);
    return 1;
}

long mac_gpu_info_texture_cache_mb(void)
{
    long megabytes = config_integer ? config_integer("mac.texture_cache_mb") : 0;
    if (megabytes >= 4 && megabytes <= 48) return megabytes;
    if (megabytes) fprintf(stderr, "mac settings: mac.texture_cache_mb must be 4 to 48; using auto\n");
    megabytes = mac_gpu_pick_texture_cache_mb(mac_gpu_info_vram_mb(), mac_gpu_info_campaign());
    fprintf(stderr, "mac settings: texture cache %ld MB\n", megabytes);
    return megabytes;
}

int mac_gpu_info_texture_skip_levels(void)
{
    long levels = config_integer ? config_integer("mac.texture_skip_levels") : -1;
    if (levels >= 0 && levels <= 4) return (int)levels;
    levels = mac_gpu_pick_texture_skip_levels(mac_gpu_info_vram_mb(), info_map);
    if (levels) fprintf(stderr, "mac settings: texture levels skipped: %ld\n", levels);
    return (int)levels;
}
