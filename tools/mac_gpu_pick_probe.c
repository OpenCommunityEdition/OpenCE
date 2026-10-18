/* The Mac's "auto" choices (port/mac/mac_gpu_pick.h), checked without a
   graphics card. */
#include <stdio.h>
#include "mac_gpu_pick.h"
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"line %d failed: %s\n",__LINE__,#x); } } while (0)
int main(void)
{
    /* PowerPC: the iBook's 32 MB draws low, the PowerBook's 64 MB low too */
    CHECK(mac_gpu_pick_preset(32, 1) == MAC_DETAIL_LOW);
    CHECK(mac_gpu_pick_preset(64, 1) == MAC_DETAIL_LOW);
    CHECK(mac_gpu_pick_preset(33, 1) == MAC_DETAIL_LOW);
    CHECK(mac_gpu_pick_preset(16, 1) == MAC_DETAIL_LOW);
    CHECK(mac_gpu_pick_preset(128, 1) == MAC_DETAIL_HIGH);
    CHECK(mac_gpu_pick_preset(256, 1) == MAC_DETAIL_HIGH);
    /* Intel: high whatever the memory */
    CHECK(mac_gpu_pick_preset(16, 0) == MAC_DETAIL_HIGH);
    CHECK(mac_gpu_pick_preset(32, 0) == MAC_DETAIL_HIGH);
    CHECK(mac_gpu_pick_preset(64, 0) == MAC_DETAIL_HIGH);
    CHECK(mac_gpu_pick_preset(256, 0) == MAC_DETAIL_HIGH);
    /* the texture cache */
    CHECK(mac_gpu_pick_texture_cache_mb(32, 0) == 16);
    CHECK(mac_gpu_pick_texture_cache_mb(63, 0) == 16);
    CHECK(mac_gpu_pick_texture_cache_mb(64, 0) == 32);
    CHECK(mac_gpu_pick_texture_cache_mb(256, 0) == 32);
    CHECK(mac_gpu_pick_texture_cache_mb(64, 1) == 48);
    CHECK(mac_gpu_pick_texture_cache_mb(32, 1) == 16);
    /* the levels skipped */
    CHECK(mac_gpu_pick_texture_skip_levels(32, "longest") == 1);
    CHECK(mac_gpu_pick_texture_skip_levels(32, "hangemhigh") == 1);
    CHECK(mac_gpu_pick_texture_skip_levels(32, "prisoner") == 1);
    CHECK(mac_gpu_pick_texture_skip_levels(32, "b30") == 1);
    CHECK(mac_gpu_pick_texture_skip_levels(32, "bloodgulch") == 0);
    CHECK(mac_gpu_pick_texture_skip_levels(64, "longest") == 0);
    CHECK(mac_gpu_pick_texture_skip_levels(32, "") == 0);
    CHECK(mac_gpu_pick_texture_skip_levels(32, NULL) == 0);
    CHECK(mac_gpu_map_is_campaign("a50") && !mac_gpu_map_is_campaign("longest"));
    /* names */
    CHECK(mac_gpu_vram_from_name("ATI Radeon 9550 OpenGL Engine") == 32);
    CHECK(mac_gpu_vram_from_name("ATI Radeon 9700 OpenGL Engine") == 64);
    CHECK(mac_gpu_vram_from_name("Unknown card") == 32);
    CHECK(mac_gpu_vram_from_name(NULL) == 32);
    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
