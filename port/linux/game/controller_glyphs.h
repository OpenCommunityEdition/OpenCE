#ifndef CONTROLLER_GLYPHS_H
#define CONTROLLER_GLYPHS_H

#include "math/integer_math.h"

struct icon_hud_element_definition;
short controller_glyph_style(void);
boolean controller_glyph_draw(short style, short index,
    struct icon_hud_element_definition const *icon, rectangle2d *cursor, pixel32 color,
    void (*unbind)(unsigned long const *));
void controller_glyphs_dispose(void);
boolean controller_glyph_draw_menu(short style, short index, rectangle2d *cursor, pixel32 color);

#endif
