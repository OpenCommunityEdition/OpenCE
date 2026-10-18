/* Unit proof of the native CPU-resident texture lifecycle adapter. */
#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
#include "cache/texture_cache.h"
#include "engine_texture_lifecycle.h"
#include <stdio.h>

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

static int bitmap_owner_bound;
static unsigned int checks, failures;
#define CHECK(v) do { ++checks; if(!(v)) { ++failures; fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#v); } } while(0)

/* This isolated lifecycle test supplies only the read-only owner-state query;
   the integrated session probe uses the real native bitmap registry. */
int mac_native_bitmap_cache_is_bound(void)
{
    return bitmap_owner_bound;
}

int main(void)
{
    struct bitmap_data bitmap;
    byte pixel;
    CHECK(!mac_engine_texture_lifecycle_ready());
    CHECK(!mac_engine_texture_lifecycle_bind());
    texture_cache_new();
    CHECK(mac_engine_texture_lifecycle_ready());
    texture_cache_idle();
    texture_cache_flush();
    texture_cache_open();
    texture_cache_close();
    texture_cache_open();
    texture_cache_close();
    bitmap_owner_bound=1;
    CHECK(mac_engine_texture_lifecycle_bind());
    CHECK(!mac_engine_texture_lifecycle_bind());
    bitmap.flags=FLAG(_bitmap_cached_bit);
    bitmap.cache_block_index=NONE;
    bitmap.base_address=&pixel;
    texture_cache_bitmap_delete(&bitmap);
    CHECK(!TEST_FLAG(bitmap.flags,_bitmap_cached_bit));
    CHECK(bitmap.cache_block_index==NONE);
    CHECK(bitmap.base_address==NULL);
    CHECK(texture_cache_steal_memory(4096)==NULL);
    CHECK(mac_engine_texture_lifecycle_unbind());
    bitmap_owner_bound=0;
    texture_cache_delete();
    CHECK(!mac_engine_texture_lifecycle_ready());
    printf("native texture lifecycle: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
