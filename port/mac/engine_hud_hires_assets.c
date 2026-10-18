/* Native runtime boundary for the PC port's optional replacement HUD art.
   This host has no embedded HUD/title textures or GL replacement-texture
   owner, so advertise no replacements and never claim sprite coverage. */
#include "cseries.h"
#include "engine_hud_hires_assets.h"
#include <stdio.h>
#include <stdlib.h>

long hud_hires_asset_count(void)
{
    return 0;
}

char const *hud_hires_asset_tag(long asset)
{
    (void)asset;
    return NULL;
}

long hud_hires_asset_bitmap(long asset)
{
    (void)asset;
    return NONE;
}

long hud_hires_asset_fits(long asset, long width, long height)
{
    (void)asset;
    (void)width;
    (void)height;
    return FALSE;
}

unsigned long hud_hires_asset_sprites(long asset)
{
    (void)asset;
    return 0;
}

int hud_hires_sprites_drawable(long asset, unsigned long address,
    unsigned long level0_size)
{
    (void)asset;
    (void)address;
    (void)level0_size;
    return FALSE;
}

void hud_hires_register_placeholder(long asset, unsigned long const *texture)
{
    (void)asset;
    /* The no-assets contract means this is reached only by teardown of an
       already-empty sprite registry; non-NULL registration is unsupported. */
    if(texture) {
        fprintf(stderr,"native high-res HUD: placeholder registration is unsupported\n");
        abort();
    }
}
