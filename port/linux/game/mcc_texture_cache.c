/* MCC-only hardware descriptors over immutable converted pixel allocations.
 * Neither the Xbox/CE bitmap descriptors nor their LRU state are rewritten. */
#include "cseries.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "rasterizer/rasterizer_swizzle.h"
#include "mcc_cache.h"
#include "mcc_texture_cache.h"
#include "../src/mcc_texture_bridge.h"
#include <xtl.h>

void *mcc_texture_cache_get(struct bitmap_data const *bitmap,boolean load)
{
    static long const formats[2][18]={
        {D3DFMT_A8,D3DFMT_L8,D3DFMT_AL8,D3DFMT_A8L8,-1,-1,D3DFMT_R5G6B5,-1,
         D3DFMT_A1R5G5B5,D3DFMT_A4R4G4B4,D3DFMT_X8R8G8B8,D3DFMT_A8R8G8B8,
         -1,-1,D3DFMT_DXT1,D3DFMT_DXT3,D3DFMT_DXT5,D3DFMT_P8},
        {D3DFMT_LIN_A8,D3DFMT_LIN_L8,D3DFMT_LIN_AL8,D3DFMT_LIN_A8L8,-1,-1,
         D3DFMT_LIN_R5G6B5,-1,D3DFMT_LIN_A1R5G5B5,D3DFMT_LIN_A4R4G4B4,
         D3DFMT_LIN_X8R8G8B8,D3DFMT_LIN_A8R8G8B8,-1,-1,-1,-1,-1,-1}};
    unsigned long format,size=0,bytes,pitch;
    boolean linear;
    void const *pixels;
    void *registered;
    if (!bitmap || !mcc_cache_tags_loaded()) return NULL;
    registered=mcc_texture_bridge_find(bitmap);
    if (registered) {
        /* CPU object lighting samples the same packed pixels after asking
         * the hardware cache whether the lightmap/diffuse map is ready. */
        if (!bitmap->base_address) {
            pixels=mcc_cache_bitmap_pixels(bitmap,&bytes);
            if (!pixels) return NULL;
            ((struct bitmap_data *)bitmap)->base_address=(void *)pixels;
        }
        return registered;
    }
    if (!load) return NULL;
    pixels=mcc_cache_bitmap_pixels(bitmap,&bytes);
    if (!pixels || bitmap->format<0 || bitmap->format>=18) return NULL;
    linear=(bitmap->flags&(1<<4))!=0;
    if (formats[linear][bitmap->format]<0) return NULL;
    format=((unsigned long)formats[linear][bitmap->format]<<D3DFORMAT_FORMAT_SHIFT)|
        D3DFORMAT_BORDERSOURCE_COLOR|D3DFORMAT_DMACHANNEL_A;
    if (linear) {
        pitch=(unsigned long)bitmap_mipmap_get_row_pitch((struct bitmap_data *)bitmap,0);
        pitch=(pitch+D3DTEXTURE_PITCH_ALIGNMENT-1)/D3DTEXTURE_PITCH_ALIGNMENT;
        if (!pitch || pitch>(D3DSIZE_PITCH_MASK>>D3DSIZE_PITCH_SHIFT)+1) return NULL;
        format|=(1<<D3DFORMAT_MIPMAP_SHIFT)|(2<<D3DFORMAT_DIMENSION_SHIFT);
        size=((pitch-1)<<D3DSIZE_PITCH_SHIFT)|
            ((bitmap->height-1)<<D3DSIZE_HEIGHT_SHIFT)|(bitmap->width-1);
    } else {
        format|=(floor_log2(bitmap->depth)<<D3DFORMAT_PSIZE_SHIFT)|
            (floor_log2(bitmap->height)<<D3DFORMAT_VSIZE_SHIFT)|
            (floor_log2(bitmap->width)<<D3DFORMAT_USIZE_SHIFT)|
            ((bitmap->type==1 ? 3 : 2)<<D3DFORMAT_DIMENSION_SHIFT)|
            ((rasterizer_xbox_bitmap_get_max_mipmap_count((struct bitmap_data *)bitmap)+1)<<D3DFORMAT_MIPMAP_SHIFT)|
            (bitmap->type==2 ? D3DFORMAT_CUBEMAP : 0);
    }
    registered=mcc_texture_bridge_register(bitmap,format,size,pixels,bytes);
    if (registered) ((struct bitmap_data *)bitmap)->base_address=(void *)pixels;
    return registered;
}

void mcc_texture_cache_dispose(void) {mcc_texture_bridge_dispose();}
