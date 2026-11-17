#ifndef KEYBOARD_GLYPHS_H
#define KEYBOARD_GLYPHS_H
#include <stddef.h>

#define KEYBOARD_GLYPH_COMPACT_TEXT_SCALE 0.85f
#define KEYBOARD_GLYPH_COMPACT_PADDING 8.0f

/* Uses the caller's font and baseline, advances its cursor, owns no map assets. */
union rectangle2d;
void keyboard_glyph_draw(char const *label, union rectangle2d *cursor, unsigned long color, int menu);
void keyboard_glyph_draw_compact(char const *label, union rectangle2d *cursor, unsigned long color);
int keyboard_glyph_background(int symbol, float x, float y, float width, float height, unsigned long color);
void keyboard_glyphs_dispose(void);
int keyboard_hud_prompt(short local_player, short icon, union rectangle2d *cursor, unsigned long color);
int keyboard_hud_inline(short icon, union rectangle2d *cursor);
void keyboard_score_hint(long player, wchar_t *text, long capacity);

#endif
