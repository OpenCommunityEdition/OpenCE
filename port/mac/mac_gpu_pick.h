#ifndef HALO_MAC_GPU_PICK_H
#define HALO_MAC_GPU_PICK_H
/* What the Mac build chooses for a machine when its settings say "auto":
   pure functions of the video memory (megabytes), the CPU and the map, so
   they are tested without a graphics card (tools/mac_gpu_pick_probe.c). */
#include <string.h>

enum { MAC_DETAIL_HIGH, MAC_DETAIL_LOW, MAC_DETAIL_LOWEST, MAC_DETAIL_MEDIUM };

/* Detail for mac.detail "auto": a PowerPC Mac with 32 MB of video memory or
   less (the iBook's Radeon 9550) draws the low preset, one with 64 MB or less
   (the PowerBook's Radeon 9600 and 9700) the medium preset; an Intel Mac, or a
   PowerPC Mac with a larger card, the stock detail. */
static __inline__ int mac_gpu_pick_preset(int vram_mb, int is_ppc)
{
    if (!is_ppc) return MAC_DETAIL_HIGH;
    if (vram_mb <= 32) return MAC_DETAIL_LOW;
    return vram_mb <= 64 ? MAC_DETAIL_MEDIUM : MAC_DETAIL_HIGH;
}

/* The levels of the campaign (their map names), which hold far more texture
   than a multiplayer map */
static __inline__ int mac_gpu_map_is_campaign(const char *name)
{
    static const char *const levels[] = { "a10", "a30", "a50", "b30", "b40",
        "c10", "c20", "c40", "d20", "d40", 0 };
    int index;
    for (index = 0; levels[index]; ++index)
        if (name && !strcmp(name, levels[index])) return 1;
    return 0;
}

/* The maps whose textures do not fit a small card's cache at full size */
static __inline__ int mac_gpu_map_is_heavy(const char *name)
{
    return name && (mac_gpu_map_is_campaign(name) || !strcmp(name, "longest") ||
        !strcmp(name, "hangemhigh") || !strcmp(name, "prisoner"));
}

/* The texture cache's megabytes for mac.texture_cache_mb 0 */
static __inline__ int mac_gpu_pick_texture_cache_mb(int vram_mb, int campaign)
{
    if (vram_mb < 64) return 16;
    return campaign ? 48 : 32;
}

/* The largest bitmap levels left out for mac.texture_skip_levels -1 */
static __inline__ int mac_gpu_pick_texture_skip_levels(int vram_mb, const char *map)
{
    return vram_mb <= 32 && mac_gpu_map_is_heavy(map) ? 1 : 0;
}

/* The video memory of a card by the name its driver gives (the answer when
   the system does not say), for the cards the Macs had; 32 when unknown */
static __inline__ int mac_gpu_vram_from_name(const char *renderer)
{
    static const struct { const char *name; int mb; } cards[] = {
        { "Radeon 7500", 32 }, { "Radeon 9200", 32 }, { "Radeon 9550", 32 },
        { "Radeon 9600", 64 }, { "Radeon 9700", 64 }, { "Radeon 9800", 128 },
        { "Radeon X1600", 128 }, { "GeForce FX 5200", 64 },
        { "GeForce 6600", 128 }, { "GMA 950", 64 }, { "GMA X3100", 128 },
        { "GeForce 9400", 256 }, { "GeForce 9600", 256 }, { 0, 0 } };
    int index;
    if (!renderer) return 32;
    for (index = 0; cards[index].name; ++index)
        if (strstr(renderer, cards[index].name)) return cards[index].mb;
    return 32;
}
#endif
