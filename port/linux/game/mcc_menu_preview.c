/* MCC owns its decoded pictures and a bounded four-entry texture cache.
 * Failed reads are cached too; rescanning and menu unload discard both. */
#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
#include "interface/ui_widget.h"
#include "tag_files/tag_groups.h"
#include "mcc_maps.h"
#include "mcc_menu_preview.h"
#include "../src/mcc_preview_image.h"
#include <stdlib.h>
#include <string.h>

static struct preview_entry {
    char level[64];
    struct bitmap_data *bitmap;
    short width, height;
} previews[4];
static unsigned next_preview;

void mcc_menu_preview_clear(void)
{
    unsigned i;
    for (i=0; i<NUMBEROF(previews); i++) if (previews[i].bitmap) bitmap_delete(previews[i].bitmap);
    memset(previews,0,sizeof(previews)); next_preview=0;
}

short mcc_menu_preview_index(long tag, short frame)
{
    if (frame < MCC_MAPS_FIRST_DISPLAY_INDEX || frame >= MCC_MAPS_FIRST_DISPLAY_INDEX+MCC_MAPS_MAXIMUM || tag == NONE) return NONE;
    return !strcmp(tag_get_name(tag),"ui\\shell\\bitmaps\\mp_map_grafix") ? frame-MCC_MAPS_FIRST_DISPLAY_INDEX : NONE;
}

static struct preview_entry *picture(short index)
{
    char const *level = mcc_maps_level_name(index);
    struct preview_entry *entry;
    struct mcc_preview_image image;
    unsigned i, x, y, texture_width=1, texture_height=1;
    int width, height;
    uint32_t *pixels;
    if (!level) return NULL;
    for (i=0; i<NUMBEROF(previews); i++) if (!strcmp(level,previews[i].level)) return &previews[i];
    entry=&previews[next_preview++ % NUMBEROF(previews)];
    if (entry->bitmap) bitmap_delete(entry->bitmap);
    memset(entry,0,sizeof(*entry)); strcpy(entry->level,level);
    if (!mcc_preview_image_load(level,mcc_maps_campaign(index),&image)) return entry;
    mcc_preview_fit(image.width,image.height,512,512,&width,&height);
    while (texture_width < (unsigned)width) texture_width *= 2;
    while (texture_height < (unsigned)height) texture_height *= 2;
    /* Native bitmap format 11 is uncompressed A8R8G8B8. */
    entry->bitmap=bitmap_2d_new((short)texture_width,(short)texture_height,0,11);
    if (entry->bitmap && entry->bitmap->base_address) {
        pixels=(uint32_t *)(uintptr_t)entry->bitmap->base_address;
        memset(pixels,0,texture_width*texture_height*sizeof(*pixels));
        for (y=0; y<(unsigned)height; y++) for (x=0; x<(unsigned)width; x++)
            pixels[y*texture_width+x]=image.pixels[(y*image.height/(unsigned)height)*image.width+x*image.width/(unsigned)width];
        bitmap_rebuild(entry->bitmap);
    }
    mcc_preview_image_free(&image);
    if (entry->bitmap && !entry->bitmap->hardware_format) { bitmap_delete(entry->bitmap); entry->bitmap=NULL; }
    entry->width=(short)width; entry->height=(short)height;
    return entry;
}

boolean mcc_menu_preview_draw(short index, long widget_tag, rectangle2d const *box, rectangle2d const *parent_clip,
    pixel32 color, struct rasterizer_dynamic_screen_geometry_parameters *parameters)
{
    struct preview_entry *entry=picture(index);
    rectangle2d bounds=*box, clip=*box, texels;
    int width, height;
    boolean crop;
    if (!entry || !entry->bitmap || !mcc_preview_fit(entry->width,entry->height,box->x1-box->x0,box->y1-box->y0,&width,&height)) return FALSE;
    bounds.x0 += (short)((box->x1-box->x0-width)/2);
    bounds.y0 += (short)((box->y1-box->y0-height)/2);
    bounds.x1=bounds.x0+(short)width; bounds.y1=bounds.y0+(short)height;
    if (parent_clip) {
        clip.x0=MAX(clip.x0,parent_clip->x0); clip.y0=MAX(clip.y0,parent_clip->y0);
        clip.x1=MIN(clip.x1,parent_clip->x1); clip.y1=MIN(clip.y1,parent_clip->y1);
    }
    set_rectangle2d(&texels,0,0,entry->width,entry->height);
    /* Native Xbox cards fill their 140x114 thumbnail. Crop only this MCC
     * presentation to that aspect; PC-style panels retain their fitted image. */
    crop = widget_tag != NONE && !strncmp(tag_get_name(widget_tag), "pc\\mcc_xbox\\", 12);
    if (crop) {
        bounds = *box;
        width = entry->width;
        height = entry->height;
        if (width * (box->y1-box->y0) > height * (box->x1-box->x0))
            width = height * (box->x1-box->x0) / (box->y1-box->y0);
        else
            height = width * (box->y1-box->y0) / (box->x1-box->x0);
        texels.x0 = (short)((entry->width-width)/2);
        texels.y0 = (short)((entry->height-height)/2);
        texels.x1 = texels.x0+(short)MAX(width,1);
        texels.y1 = texels.y0+(short)MAX(height,1);
    }
    if (clip.x1 > clip.x0 && clip.y1 > clip.y0)
        draw_bitmap_in_rect(entry->bitmap,&bounds,&texels,&clip,color,parameters,FALSE);
    return TRUE;
}
