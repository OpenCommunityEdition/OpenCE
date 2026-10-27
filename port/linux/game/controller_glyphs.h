/*
CONTROLLER_GLYPHS.H

The HUD's buttons in the styles of port/assets/controller_buttons
(controller_glyphs.c), for hud_hires_tags.c.
*/

#ifndef CONTROLLER_GLYPHS_H
#define CONTROLLER_GLYPHS_H

#include "math/integer_math.h"

struct icon_hud_element_definition;

/* the style the viewport drawn draws its buttons in (controller_buttons.h) */
int controller_glyph_style(void);
/* a button (index: the HUD's button icon's) drawn at the cursor, which it
moves past; FALSE where the style has no artwork of its own (the Xbox's
first three) */
boolean controller_glyph_draw(int style, short index, struct icon_hud_element_definition const *icon,
	rectangle2d *cursor, pixel32 color);
/* the textures made, let go of with the map */
void controller_glyphs_dispose(void);

#endif
