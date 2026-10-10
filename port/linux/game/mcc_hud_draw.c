/* The MCC canvas is 960p, while native UI/HUD coordinates are 480p.
 * Keep full-resolution texture samples and convert only screen geometry. */
#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "game/players.h"
#include "interface/hud_definitions.h"
#include "interface/hud_draw.h"
#include "text/draw_string.h"
#include "mcc_cache.h"
#include "mcc_hud_draw.h"

real mcc_hud_canvas_scale(void)
{
    return mcc_cache_tags_loaded() ? 0.5f : 1.0f;
}

real mcc_hud_bitmap_scale(long bitmap_group_index)
{
    struct bitmap_group const *group;
    if (!mcc_cache_tags_loaded() || bitmap_group_index==NONE) return 1.0f;
    group=bitmap_group_get(bitmap_group_index);
    return group->flags&(0x10|0x80) ? 0.25f : 0.5f;
}

boolean mcc_hud_icon_draw(long bitmap_group_index, struct bitmap_data const *bitmap,
    real_rectangle2d const *clip, rectangle2d *cursor, pixel32 color,
    struct icon_hud_element_definition const *icon, boolean menu)
{
    real viewport_scale, pixel_scale, offset_scale, advance;
    point2d point;
    if (!mcc_cache_tags_loaded()) return FALSE;
    viewport_scale=!menu && local_player_count()>1 ? 0.75f : 1.0f;
    pixel_scale=viewport_scale*mcc_hud_bitmap_scale(bitmap_group_index);
    /* Icon offsets and extra text advances are font-relative, unlike the
     * HUD placement anchors. MCC tags retain the native values here. */
    offset_scale=viewport_scale;
    point.x=(short)(cursor->x0+icon->offset.x*offset_scale+(menu ? 1.0f : 0.0f));
    point.y=(short)(cursor->y1-icon->offset.y*offset_scale-(menu ? 2.0f : 0.0f));
    if (menu) {
        /* A map's HUD icon baseline may be custom-authored for its messages.
         * Native pause labels use the selected UI font instead. Align their
         * glyphs to its capitals, without resizing or changing the font. The
         * empty string measures this same line without introducing wrapping. */
        real height=(clip ? clip->y1-clip->y0 : 1.0f)*bitmap->height*pixel_scale;
        point.y=(short)(draw_unicode_string_capital_middle(cursor,L"")+height*0.5f);
    }
    hud_draw_bitmap_direct(bitmap,_hud_anchor_bottom_left,&point,clip,pixel_scale,0.0f,
        icon->flags&2 ? icon->color : color,FALSE);
    advance=icon->width_offset*offset_scale;
    if (!(icon->flags&4)) advance+=(clip ? clip->x1-clip->x0 : 1.0f)*bitmap->width*pixel_scale;
    cursor->x0=(short)(point.x+advance);
    return TRUE;
}
