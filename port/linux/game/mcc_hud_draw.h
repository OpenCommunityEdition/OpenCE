#ifndef MCC_HUD_DRAW_H
#define MCC_HUD_DRAW_H

#include "cseries.h"
#include "math/real_math.h"

struct bitmap_data;
struct hud_placement_definition;
struct icon_hud_element_definition;

/* Neutral for every non-MCC cache. No native or CE tag is rewritten. */
real mcc_hud_canvas_scale(void);
real mcc_hud_bitmap_scale(long bitmap_group_index);
boolean mcc_hud_icon_draw(long bitmap_group_index, struct bitmap_data const *bitmap,
    real_rectangle2d const *clip, rectangle2d *cursor, pixel32 color,
    struct icon_hud_element_definition const *icon, boolean menu);

boolean mcc_hud_anchor_point(short anchor, struct hud_placement_definition const *placement,
    struct bitmap_data const *bitmap, real scale, rectangle2d const *window,
    rectangle2d const *viewport, point2d *point);
boolean mcc_hud_anchor_bounds(short anchor, real width, real height, real_rectangle2d *bounds);
boolean mcc_hud_anchor_number(short anchor, real width, real origin, short *cursor);
short mcc_terminal_line_height(long font, short height);
boolean mcc_terminal_draw(long font, rectangle2d const *bounds, char const *text);

#endif
