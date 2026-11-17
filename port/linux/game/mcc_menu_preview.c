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
    unsigned source_width, source_height, source_x=0, source_y=0;
    int width, height;
    uint32_t *pixels;
    if (!level) return NULL;
    for (i=0; i<NUMBEROF(previews); i++) if (!strcmp(level,previews[i].level)) return &previews[i];
    entry=&previews[next_preview++ % NUMBEROF(previews)];
    if (entry->bitmap) bitmap_delete(entry->bitmap);
    memset(entry,0,sizeof(*entry)); strcpy(entry->level,level);
    if (!mcc_preview_image_load(level,mcc_maps_campaign(index),&image)) return entry;
    source_width=image.width; source_height=image.height;
    /* Every frontend uses the native 140x114 question-mark rectangle.
     * draw_bitmap_in_rect samples from texture origin, regardless of the
     * source rectangle's origin. Prepare the center crop here instead. */
    if (source_width*114 > source_height*140) source_width=MAX(1,source_height*140/114);
    else source_height=MAX(1,source_width*114/140);
    source_x=(image.width-source_width)/2; source_y=(image.height-source_height)/2;
    mcc_preview_fit(140,114,512,512,&width,&height);
    while (texture_width < (unsigned)width) texture_width *= 2;
    while (texture_height < (unsigned)height) texture_height *= 2;
    /* Native bitmap format 11 is uncompressed A8R8G8B8. */
    entry->bitmap=bitmap_2d_new((short)texture_width,(short)texture_height,0,11);
    if (entry->bitmap && entry->bitmap->base_address) {
        pixels=(uint32_t *)(uintptr_t)entry->bitmap->base_address;
        memset(pixels,0,texture_width*texture_height*sizeof(*pixels));
        for (y=0; y<(unsigned)height; y++) for (x=0; x<(unsigned)width; x++)
            pixels[y*texture_width+x]=image.pixels[(source_y+y*source_height/(unsigned)height)*image.width+
                source_x+x*source_width/(unsigned)width];
        bitmap_rebuild(entry->bitmap);
    }
    mcc_preview_image_free(&image);
    if (entry->bitmap && !entry->bitmap->hardware_format) { bitmap_delete(entry->bitmap); entry->bitmap=NULL; }
    entry->width=(short)width; entry->height=(short)height;
    return entry;
}

boolean mcc_menu_preview_draw(short index, rectangle2d const *box, rectangle2d const *parent_clip,
    pixel32 color, struct rasterizer_dynamic_screen_geometry_parameters *parameters)
{
    struct preview_entry *entry;
    rectangle2d bounds=*box, clip=*box, texels;
    if (box->x1<=box->x0 || box->y1<=box->y0) return FALSE;
    entry=picture(index);
    if (!entry || !entry->bitmap) return FALSE;
    if (parent_clip) {
        clip.x0=MAX(clip.x0,parent_clip->x0); clip.y0=MAX(clip.y0,parent_clip->y0);
        clip.x1=MIN(clip.x1,parent_clip->x1); clip.y1=MIN(clip.y1,parent_clip->y1);
    }
    set_rectangle2d(&texels,0,0,entry->width,entry->height);
    if (clip.x1 > clip.x0 && clip.y1 > clip.y0)
        draw_bitmap_in_rect(entry->bitmap,&bounds,&texels,&clip,color,parameters,FALSE);
    return TRUE;
}
