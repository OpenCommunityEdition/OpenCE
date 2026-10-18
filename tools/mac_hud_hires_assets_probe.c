/* Verify the explicit empty high-resolution HUD asset capability. */
#include "engine_hud_hires_assets.h"
#include <stdio.h>

int main(void)
{
    unsigned int checks=0, failures=0;
#define CHECK(v) do { ++checks; if(!(v)) { ++failures; fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#v); } } while(0)
    CHECK(hud_hires_asset_count()==0);
    CHECK(hud_hires_asset_tag(0)==NULL);
    CHECK(hud_hires_asset_bitmap(0)==NONE);
    CHECK(!hud_hires_asset_fits(0,128,128));
    CHECK(hud_hires_asset_sprites(0)==0);
    CHECK(!hud_hires_sprites_drawable(0,0,0));
    hud_hires_register_placeholder(0,NULL);
    printf("native high-res HUD assets: %u checks, %u failures; no replacements installed\n",checks,failures);
    return failures?1:0;
}
