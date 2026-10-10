#ifndef MCC_HUD_DRAW_H
#define MCC_HUD_DRAW_H

#include "cseries.h"
#include "math/real_math.h"

struct bitmap_data;
struct icon_hud_element_definition;

/* Neutral for every non-MCC cache. No native or CE tag is rewritten. */
real mcc_hud_canvas_scale(void);
real mcc_hud_bitmap_scale(long bitmap_group_index);
boolean mcc_hud_icon_draw(long bitmap_group_index, struct bitmap_data const *bitmap,
    real_rectangle2d const *clip, rectangle2d *cursor, pixel32 color,
    struct icon_hud_element_definition const *icon, boolean menu);

#endif
