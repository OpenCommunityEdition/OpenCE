/* Modern and stock PC menus share the existing artwork owner with gameplay;
 * the prompt dispatcher gates their use on High-Res HUD. */
#include "cseries.h"
#include "frontend_menu.h"
#include "frontend_prompts.h"
#include "controller_buttons.h"
#include "controller_glyphs.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "interface/hud_definitions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.h"
#include "text/draw_string.h"
#include "hud_button_layout.h"
#include <string.h>

struct bitmap_data *hud_hires_button_bitmap(short style, short slot);

boolean frontend_glyph_draw(short port, short button, rectangle2d *cursor, pixel32 color)
{
    struct rasterizer_dynamic_screen_geometry_parameters parameters;
    struct dynamic_screen_vertex vertices[4];
    struct bitmap_data *bitmap;
    short style, i;
    real x, y, width = 16.0f, height = 20.0f;
    if (button < 0 || (button > 3 && button != 16) || port < 0 || port > 3) return FALSE;
    style = (short)controller_button_style_for_port(port);
    if (button == 16) return controller_glyph_draw_menu(style < 3 ? 3 : style, button, cursor, color);
    if (style >= 3) return controller_glyph_draw_menu(style, button, cursor, color);
    if (style < 0) return FALSE;
    bitmap = hud_hires_button_bitmap(style, button);
    if (!bitmap) return FALSE;
    x = cursor->x0 + 4.0f;
    y = draw_unicode_string_capital_middle(cursor, L"") - height * 0.5f;
    hud_button_face_bounds(style, button, &x, &y, &width, &height);
    memset(&parameters, 0, sizeof(parameters));
    parameters.map[0] = bitmap;
    parameters.map_texture_scale[0].i = parameters.map_texture_scale[0].j = 1.0f;
    parameters.map_scale[0].i = parameters.map_scale[0].j = 1.0f;
    for (i = 0; i < 4; i++)
    {
        boolean right = ((i + 1) & 2) != 0, bottom = i > 1;
        vertices[i].position.x = x + (right ? width : 0.0f);
        vertices[i].position.y = y + (bottom ? height : 0.0f);
        vertices[i].texture_coordinates.x = right ? 21.0f / 32.0f : 0.0f;
        vertices[i].texture_coordinates.y = bottom ? 26.0f / 32.0f : 0.0f;
        vertices[i].color = (color & 0xff000000UL) | 0xffffffUL;
    }
    rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);
    cursor->x0 += 26;
    return TRUE;
}
