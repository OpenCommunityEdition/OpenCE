#ifndef HALO_MAC_HUD_HIRES_H
#define HALO_MAC_HUD_HIRES_H

/* The high-res HUD on the Mac (display.high_res_hud): the hand-drawn redraws
   of the HUD's sheets (port/assets/hud), reduced to twice the size of the
   bitmaps they stand for (tools/mac_hud_reduce.py) and embedded in the
   build (tools/mac_hud_assets_view.py). The game's own sources decide which
   bitmaps they stand for (port/linux/game/hud_hires_tags.c, through the
   hud_hires_asset_... functions); the texture cache of the Xbox maps
   (mac_gpu_textures.c) draws one in place of its bitmap when the bitmap's
   pixels are the ones it was drawn for. */

/* an embedded texture: an 8-bit RGBA PNG, and the bitmap it stands for (its
   bitmap group tag's name, its index there, and the CRC-32 of its first mip
   level's pixels as the English Xbox maps have them) */
struct mac_hud_asset
{
    const char *tag;
    int bitmap;
    unsigned int width, height;
    unsigned int crc;
    const unsigned char *png;
    unsigned int png_size;
};

extern const struct mac_hud_asset mac_hud_assets[];
extern const unsigned int mac_hud_assets_count;

/* what the texture cache of the Xbox maps (mac_gpu_textures.c) asks, as it makes
   the texture of a bitmap whose texels are at address (in the engine's memory),
   width by height, its first mip level level0_size bytes: find, which
   asset stands for it (its number), or -1; enabled, whether the setting is on;
   decode, the asset's texels, RGBA with the top row first (allocated; the
   cache frees them), or NULL */
struct mac_gpu_hud_provider
{
    long (*find)(unsigned long address, unsigned long width, unsigned long height, unsigned long level0_size);
    int (*enabled)(void);
    unsigned char *(*decode)(long asset, unsigned *width, unsigned *height);
};

/* makes the Mac's texture cache draw them (once, at the start) */
void mac_hud_hires_install(void);

#endif
