/* Modern frontend prompts reuse the player's existing glyph preferences and
 * embedded art. Classic Xbox faces need an independent cache: the HUD's cache
 * requires map-owned messaging icons and its high-res HUD setting. */
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

long hud_hires_asset_count(void);
char const *hud_hires_asset_tag(long asset);
void hud_hires_register_placeholder(long asset, unsigned long const *texture);

static struct { struct bitmap_data *bitmap; long asset; boolean attempted; } frontend_faces[3][4];
static char const *frontend_face_names[3][4] = {
    {"ui\\hud\\buttons\\classic_a", "ui\\hud\\buttons\\classic_b", "ui\\hud\\buttons\\classic_x", "ui\\hud\\buttons\\classic_y"},
    {"ui\\hud\\buttons\\gem_a", "ui\\hud\\buttons\\gem_b", "ui\\hud\\buttons\\gem_x", "ui\\hud\\buttons\\gem_y"},
    {"ui\\shell\\bitmaps\\a_butn", "ui\\shell\\bitmaps\\b_butn", "ui\\shell\\bitmaps\\x_butn", "ui\\shell\\bitmaps\\y_butn"}
};

void frontend_glyphs_dispose(void)
{
    int style, button;
    for (style = 0; style < 3; style++) for (button = 0; button < 4; button++)
        if (frontend_faces[style][button].bitmap)
        {
            hud_hires_register_placeholder(frontend_faces[style][button].asset, NULL);
            bitmap_delete(frontend_faces[style][button].bitmap);
        }
    memset(frontend_faces, 0, sizeof(frontend_faces));
}

boolean frontend_glyph_draw(short port, short button, rectangle2d *cursor, pixel32 color)
{
    struct icon_hud_element_definition icon;
    struct rasterizer_dynamic_screen_geometry_parameters parameters;
    struct dynamic_screen_vertex vertices[4];
    struct bitmap_data *bitmap;
    short style, i;
    long asset;
    real x, y, width = 21.0f, height = 26.0f;
    if (button < 0 || button > 3 || port < 0 || port > 3) return FALSE;
    style = (short)controller_button_style_for_port(port);
    memset(&icon, 0, sizeof(icon));
    if (style >= 3) return controller_glyph_draw(style, button, &icon, cursor, color, NULL);
    if (style < 0) return FALSE;
    if (!frontend_faces[style][button].attempted)
    {
        frontend_faces[style][button].attempted = TRUE;
        for (asset = 0; asset < hud_hires_asset_count(); asset++)
        {
            if (strcmp(hud_hires_asset_tag(asset), frontend_face_names[style][button])) continue;
            bitmap = bitmap_2d_new(32, 32, 0, 11);
            if (!bitmap) break;
            if (!rasterizer_bitmap_new(bitmap)) {bitmap_delete(bitmap); break;}
            hud_hires_register_placeholder(asset, bitmap->hardware_format);
            frontend_faces[style][button].asset = asset;
            frontend_faces[style][button].bitmap = bitmap;
            break;
        }
    }
    bitmap = frontend_faces[style][button].bitmap;
    if (!bitmap) return FALSE;
    x = cursor->x0 + 4.0f;
    y = draw_unicode_string_capital_middle(cursor, L"") - 13.0f;
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
    cursor->x0 += 22;
    return TRUE;
}
