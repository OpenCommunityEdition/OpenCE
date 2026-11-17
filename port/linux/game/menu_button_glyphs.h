#ifndef MENU_BUTTON_GLYPHS_H
#define MENU_BUTTON_GLYPHS_H
#include "math/integer_math.h"

/* Recognized stock face-button legends only. Returning false preserves native art. */
boolean menu_button_glyph_draw(short button, short port, rectangle2d const *bounds,
    rectangle2d const *clip, pixel32 color);
boolean menu_button_glyph_inline(short icon, short port, rectangle2d *cursor);

/* Keep independent join instructions; suppress only duplicated legend actions. */
wchar_t const *menu_button_glyph_join_hint(boolean can_join, short port);

boolean menu_button_glyph_join_draw(wchar_t const *text, short port, rectangle2d const *bounds);

boolean menu_button_glyph_keyboard_active(short port);

short menu_button_glyph_key_width(short button);
short menu_button_glyph_compact_key_width(short button);

#endif
