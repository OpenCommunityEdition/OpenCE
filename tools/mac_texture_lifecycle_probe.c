/* Exercise the CPU-resident texture-cache adapter against a real bound
   session bitmap-owner registry. */
#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
#include "cache/texture_cache.h"

/* Source-private bitmap flag order, witnessed in engine_texture_lifecycle.c. */
enum
{
    _bitmap_has_power_of_two_dimensions_bit,
    _bitmap_compressed_bit,
    _bitmap_palettized_bit,
    _bitmap_swizzled_bit,
    _bitmap_linear_bit,
    _bitmap_v16u16_bit,
    _bitmap_allocated_bit,
    _bitmap_cached_bit,
    NUMBER_OF_BITMAP_FLAGS
};
#include "engine_texture_lifecycle.h"

static int exercise_bound_texture_lifecycle(void)
{
    struct bitmap_data bitmap;
    byte sentinel;
    void *stolen;
    if(!mac_engine_texture_lifecycle_ready() || !mac_engine_texture_lifecycle_is_bound() ||
       !mac_native_bitmap_cache_is_bound())return 0;
    texture_cache_idle();
    texture_cache_flush();
    csmemset(&bitmap,0,sizeof(bitmap));
    bitmap.flags=FLAG(_bitmap_cached_bit);
    bitmap.cache_block_index=NONE;
    bitmap.base_address=&sentinel;
    texture_cache_bitmap_delete(&bitmap);
    if(TEST_FLAG(bitmap.flags,_bitmap_cached_bit) ||
       bitmap.cache_block_index!=NONE || bitmap.base_address!=NULL)
        return 0;
    stolen=texture_cache_steal_memory(4096);
    if(stolen!=NULL)return 0;
    return 1;
}

int mac_texture_lifecycle_bound_probe(void)
{
    return exercise_bound_texture_lifecycle();
}

int mac_texture_lifecycle_probe(void)
{
    if(!mac_engine_texture_lifecycle_ready() || !mac_native_bitmap_cache_is_bound())return 0;
    if(!mac_engine_texture_lifecycle_bind())return 0;
    if(mac_engine_texture_lifecycle_bind())return 0;
    if(!exercise_bound_texture_lifecycle())return 0;
    if(!mac_engine_texture_lifecycle_unbind())return 0;
    return mac_native_bitmap_cache_is_bound();
}
