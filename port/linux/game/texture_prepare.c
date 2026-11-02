/* Optional Xbox/CE preparation through the existing validated texture cache.
 * MCC has its own allocation enumeration, budget and ownership. */
#include "cseries.h"
#include "cseries/errors.h"
#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "cache/texture_cache.h"
#include "mcc_cache.h"
#include "../src/port_config.h"
#include <stdlib.h>

#ifndef HALO_GLES
#define TEXTURE_PREPARE_LIMIT 8192
struct texture_candidate {
    struct bitmap_data *bitmap;
    unsigned long charge;
};

/* Bound both guest cache residency and worst-case RGBA GPU storage, including
 * mip levels and cube faces. The native loader still validates the resource. */
static unsigned long texture_prepare_charge(struct bitmap_data const *bitmap)
{
    unsigned long long bytes=0;
    unsigned long width,height,depth,level;
    if (!(bitmap->flags&(1U<<7)) || bitmap->type<0 || bitmap->type>2 ||
        bitmap->format<0 || bitmap->format>=17 || bitmap->pixels_size<=0 ||
        bitmap->width<=0 || bitmap->width>4096 || bitmap->height<=0 || bitmap->height>4096 ||
        bitmap->depth<=0 || bitmap->depth>512 || bitmap->mipmap_count<0 || bitmap->mipmap_count>12) return 0;
    width=bitmap->width;height=bitmap->height;depth=bitmap->depth;
    for (level=0;level<=(unsigned)bitmap->mipmap_count;level++) {
        /* Native linear rows are rounded to the Xbox 64-byte pitch. */
        bytes+=(unsigned long long)((width*4+63)&~63UL)*height*depth;
        if (width>1) width>>=1;
        if (height>1) height>>=1;
        if (depth>1) depth>>=1;
    }
    if (bitmap->type==2) bytes*=6;
    if (bytes<(unsigned long)bitmap->pixels_size) bytes=bitmap->pixels_size;
    /* A cache allocation is page-rounded even for tiny textures. */
    bytes=(bytes+16383)&~16383ULL;
    return bytes>128ULL*1024*1024 ? 0 : (unsigned long)bytes;
}

static int texture_prepare_order(void const *left,void const *right)
{
    struct texture_candidate const *a=left,*b=right;
    return a->charge>b->charge ? -1 : a->charge<b->charge ? 1 : 0;
}
#endif

void texture_prepare_map(void)
{
#ifndef HALO_GLES
    struct texture_candidate *candidates;
    struct tag_iterator iterator;
    long index,megabytes=config_integer("display.texture_prepare_mb");
    unsigned count=0,item,prepared=0;
    unsigned long budget,spent=0;
    if (megabytes<=0 || mcc_cache_tags_loaded() || config_server_mode_enabled() ||
        config_boolean("debug.null_renderer")) return;
    /* At most half the established 256 MiB guest texture cache. */
    if (megabytes>128) megabytes=128;
    budget=(unsigned long)megabytes*1024*1024;
    candidates=malloc(TEXTURE_PREPARE_LIMIT*sizeof(*candidates));
    if (!candidates) return;
    tag_iterator_new(&iterator,BITMAP_GROUP_TAG);
    while ((index=tag_iterator_next(&iterator))!=NONE && count<TEXTURE_PREPARE_LIMIT) {
        struct bitmap_group *group=bitmap_group_get(index);
        long bitmap_index;
        for (bitmap_index=0;bitmap_index<group->bitmaps.count && count<TEXTURE_PREPARE_LIMIT;bitmap_index++) {
            struct bitmap_data *bitmap=(struct bitmap_data *)group->bitmaps.address+bitmap_index;
            unsigned long charge=texture_prepare_charge(bitmap);
            if (charge && charge<=budget) {
                candidates[count].bitmap=bitmap;candidates[count++].charge=charge;
            }
        }
    }
    qsort(candidates,count,sizeof(*candidates),texture_prepare_order);
    for (item=0;item<count;item++) {
        void *header;
        if (candidates[item].charge>budget-spent) continue;
        spent+=candidates[item].charge;
        header=_texture_cache_bitmap_get_hardware_format(candidates[item].bitmap,TRUE,TRUE);
        if (header && render_prepare_texture(header)) prepared++;
    }
    free(candidates);
    error(_error_silent,"texture preparation: uploaded %u textures (%lu KiB bounded storage)",prepared,spent/1024);
#endif
}
