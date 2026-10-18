#ifndef HALO_MAC_ENGINE_HUD_HIRES_ASSETS_H
#define HALO_MAC_ENGINE_HUD_HIRES_ASSETS_H

/* The native diagnostic runtime has no embedded high-resolution HUD or title
   texture set. It therefore advertises an empty, truthful asset registry. */
long hud_hires_asset_count(void);
char const *hud_hires_asset_tag(long asset);
long hud_hires_asset_bitmap(long asset);
long hud_hires_asset_fits(long asset, long width, long height);
unsigned long hud_hires_asset_sprites(long asset);
int hud_hires_sprites_drawable(long asset, unsigned long address,
    unsigned long level0_size);
void hud_hires_register_placeholder(long asset, unsigned long const *texture);

#endif
