#ifndef CONTROLLER_GLYPHS_H
#define CONTROLLER_GLYPHS_H

#include "math/real_math.h"

struct icon_hud_element_definition;
short controller_glyph_style(void);
struct bitmap_data *controller_glyph_bitmap(short style, short slot, real_rectangle2d *uv);
boolean controller_glyph_draw(short style, short index,
    struct icon_hud_element_definition const *icon, rectangle2d *cursor, pixel32 color);
/* Optional owner callback releases external bindings before bitmap deletion. */
boolean controller_glyph_draw_with_release(short style, short index,
    struct icon_hud_element_definition const *icon, rectangle2d *cursor, pixel32 color,
    void (*unbind)(unsigned long const *));
void controller_glyphs_dispose(void);

#endif
