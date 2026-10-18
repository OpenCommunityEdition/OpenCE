#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
#include "cache/texture_cache.h"
#include "engine_texture_lifecycle.h"
#include "engine_cache_files.h"
/* port: the engine's own texture cache (source/cache/xbox_texture_cache.c,
   tools/mac_xbox_texture_cache_view.py names its entry points xbox_...),
   which Xbox maps use: their bitmaps are already the console's */
void xbox_texture_cache_new(void);
void xbox_texture_cache_delete(void);
void xbox_texture_cache_open(void);
void xbox_texture_cache_close(void);
void xbox_texture_cache_idle(void);
void xbox_texture_cache_flush(void);
void xbox_texture_cache_bitmap_delete(struct bitmap_data *bitmap);
void *xbox_texture_cache_steal_memory(long size);
void xbox_texture_cache_return_memory(void);
#include <stdio.h>
#include <stdlib.h>

/* Mirror the source-private bitmap flag order in xbox_texture_cache.c. */
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
typedef char native_bitmap_flag_witness[NUMBER_OF_BITMAP_FLAGS == 8 ? 1 : -1];

/* The retail source defines this private two-boolean type in
   xbox_texture_cache.c; HS globals address the same two bytes. */
struct native_texture_cache_debug_options { boolean graph; boolean list; };
typedef char native_texture_debug_options_size[
    sizeof(struct native_texture_cache_debug_options)==2?1:-1];
struct native_texture_cache_debug_options texture_cache_debug_options={FALSE,FALSE};

static int lifecycle_bound;
static int lifecycle_initialized;
static int cache_open;

static void require_lifecycle(const char *operation)
{
    if(!lifecycle_initialized) {
        fprintf(stderr,"native CPU texture lifecycle: %s before texture_cache_new\n",operation);
        abort();
    }
}

int mac_engine_texture_lifecycle_bind(void)
{
    if(lifecycle_bound || !mac_native_bitmap_cache_is_bound())return 0;
    lifecycle_bound=1;
    return 1;
}

int mac_engine_texture_lifecycle_unbind(void)
{
    if(!lifecycle_bound || !mac_native_bitmap_cache_is_bound())return 0;
    lifecycle_bound=0;
    return 1;
}

int mac_engine_texture_lifecycle_ready(void)
{
    return lifecycle_initialized;
}

int mac_engine_texture_lifecycle_is_bound(void)
{
    return lifecycle_bound;
}

void texture_cache_new(void)
{
    if(mac_xbox_mode()){xbox_texture_cache_new();lifecycle_initialized=1;cache_open=0;return;}
    if(lifecycle_initialized) {
        fprintf(stderr,"native CPU texture lifecycle: duplicate texture_cache_new\n");
        abort();
    }
    lifecycle_initialized=1;
    cache_open=0;
}

void texture_cache_delete(void)
{
    if(mac_xbox_mode()){xbox_texture_cache_delete();lifecycle_initialized=0;return;}
    require_lifecycle("delete");
    if(cache_open || lifecycle_bound) {
        fprintf(stderr,"native CPU texture lifecycle: delete while map cache is open\n");
        abort();
    }
    lifecycle_initialized=0;
}

void texture_cache_open(void)
{
    if(mac_xbox_mode()){xbox_texture_cache_open();return;}
    require_lifecycle("open");
    if(cache_open) {
        fprintf(stderr,"native CPU texture lifecycle: duplicate texture_cache_open\n");
        abort();
    }
    cache_open=1;
}

void texture_cache_close(void)
{
    if(mac_xbox_mode()){xbox_texture_cache_close();return;}
    require_lifecycle("close");
    if(!cache_open) {
        fprintf(stderr,"native CPU texture lifecycle: close without open\n");
        abort();
    }
    texture_cache_flush();
    cache_open=0;
}

void texture_cache_idle(void)
{
    if(mac_xbox_mode()){xbox_texture_cache_idle();return;}
    require_lifecycle("idle");
    /* Session-owned CPU images are immutable and pinned; there is no LRU or
       pending GPU upload queue to age here. */
}

void texture_cache_flush(void)
{
    if(mac_xbox_mode()){xbox_texture_cache_flush();return;}
    require_lifecycle("flush");
    /* The native owner exposes decoded CPU pixels only. It has no deferred
       D3D texture uploads or command buffer to flush. */
}

void texture_cache_bitmap_delete(struct bitmap_data *bitmap)
{
    if(mac_xbox_mode()){xbox_texture_cache_bitmap_delete(bitmap);return;}
    require_lifecycle("bitmap delete");
    if(!bitmap) {
        fprintf(stderr,"native CPU texture lifecycle: null bitmap delete\n");
        abort();
    }
    if(TEST_FLAG(bitmap->flags,_bitmap_cached_bit)) {
        if(bitmap->cache_block_index!=NONE) {
            fprintf(stderr,"native CPU texture lifecycle: bitmap has an Xbox cache block\n");
            abort();
        }
        SET_FLAG(bitmap->flags,_bitmap_cached_bit,FALSE);
        bitmap->cache_block_index=NONE;
        bitmap->base_address=NULL;
    }
}

void *texture_cache_steal_memory(long size)
{
    if(mac_xbox_mode())return xbox_texture_cache_steal_memory(size);
    require_lifecycle("steal memory");
    (void)size;
    /* Xbox cache-page protection/guard semantics have no native equivalent.
       Returning NULL lets the real Bink caller take its existing no-memory
       path; this provider never lends unrelated session allocations. */
    fprintf(stderr,"native CPU texture lifecycle: Xbox texture memory lending is unsupported\n");
    return NULL;
}

void texture_cache_return_memory(void)
{
    if(mac_xbox_mode()){xbox_texture_cache_return_memory();return;}
    require_lifecycle("return memory");
    /* A successful steal is impossible above. A direct return is a lifecycle
       error rather than an invitation to resize or protect foreign memory. */
    fprintf(stderr,"native CPU texture lifecycle: no borrowed texture memory to return\n");
    abort();
}
