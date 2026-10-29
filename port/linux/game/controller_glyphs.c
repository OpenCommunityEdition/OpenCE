/* Optional controller-family artwork. Legacy Xbox renderers remain the fallback;
 * callers identify their own map's controller entries before reaching this code. */
#include "cseries.h"
#include "memory/data.h"
#include "controller_buttons.h"
#include "controller_glyphs.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "game/players.h"
#include "game/game.h"
#include "render/render.h"
#include "interface/hud_definitions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.h"
#include "text/draw_string.h"
#include "hud_button_layout.h"
#include <string.h>

char const *config_string(char const *name);
long hud_hires_asset_count(void);
char const *hud_hires_asset_tag(long asset);
void hud_hires_register_placeholder(long asset, unsigned long const *texture);

struct controller_glyph_asset
{
    char const *tag;
    short bounds[4];
};
#include "controller_glyph_assets.h"

/* Key by artwork rather than the current player: mixed-controller split screen
 * never invalidates another viewport's textures. Released at map teardown. */
static struct
{
    struct bitmap_data *bitmap;
    long asset;
    boolean attempted;
} controller_glyph_cache[NUMBEROF(controller_glyph_assets)];

short controller_glyph_style(void)
{
    short port = render.local_player_index;
    long player_index;
    if (port >= 0 && port < 4)
    {
        player_index = local_player_get_player_index(port);
        if (player_index != NONE) port = player_get(player_index)->local_player_index;
    }
    return (short)controller_button_style_for_port(port);
}

void controller_glyphs_dispose(void)
{
    long i;
    for (i = 0; i < NUMBEROF(controller_glyph_cache); i++)
    {
        if (controller_glyph_cache[i].bitmap)
        {
            hud_hires_register_placeholder(controller_glyph_cache[i].asset, NULL);
            bitmap_delete(controller_glyph_cache[i].bitmap);
        }
    }
    memset(controller_glyph_cache, 0, sizeof(controller_glyph_cache));
}

static short controller_glyph_asset_index(short style, short slot)
{
    short family;
    /* Older Xbox styles can use the supplied modern bumpers independently of
     * their face buttons. The caller has already resolved automatic detection. */
    if (style >= 0 && style < 3 && (slot == 4 || slot == 5) &&
        !strcmp(config_string("display.hud_bumper_style"), "modern"))
        return (short)(32 + slot - 4);
    if (style < 3 || style >= 11 || slot < 0 || slot >= 14) return NONE;
    if (slot < 4) return (short)((style - 3) * 4 + slot);
    family = (short)(style < 4 ? 0 : style < 8 ? 1 : 2);
    return (short)(32 + family * 10 + slot - 4);
}

boolean controller_glyph_draw(short style, short index,
    struct icon_hud_element_definition const *icon, rectangle2d *cursor, pixel32 color)
{
    struct rasterizer_dynamic_screen_geometry_parameters parameters;
    struct dynamic_screen_vertex vertices[4];
    struct controller_glyph_asset const *entry;
    struct bitmap_data *bitmap;
    short slot, key, i;
    long asset;
    real scale, cell_width, cell_height, width, height, fit, x, y, center;
    slot = index < 6 ? index : hud_button_control_slot(index,
        icon->frame_rate > 0 ? game_time_get() / icon->frame_rate : 0);
    key = controller_glyph_asset_index(style, slot);
    if (key == NONE) return FALSE;
    entry = &controller_glyph_assets[key];
    if (!controller_glyph_cache[key].attempted)
    {
        controller_glyph_cache[key].attempted = TRUE;
        for (asset = 0; asset < hud_hires_asset_count(); asset++)
        {
            if (strcmp(hud_hires_asset_tag(asset), entry->tag)) continue;
            bitmap = bitmap_2d_new(64, 64, 0, 11);
            if (!bitmap) break;
            if (!rasterizer_bitmap_new(bitmap)) {bitmap_delete(bitmap); break;}
            hud_hires_register_placeholder(asset, bitmap->hardware_format);
            controller_glyph_cache[key].asset = asset;
            controller_glyph_cache[key].bitmap = bitmap;
            break;
        }
    }
    bitmap = controller_glyph_cache[key].bitmap;
    if (!bitmap) return FALSE;
    scale = local_player_count() > 1 ? 0.75f : 1.0f;
    cell_width = (slot < 6 ? 21.0f : slot < 8 ? 44.0f : 32.0f) * scale;
    cell_height = (slot < 6 ? 26.0f : slot < 8 ? 33.0f : 34.0f) * scale;
    width = entry->bounds[2] - entry->bounds[0];
    height = entry->bounds[3] - entry->bounds[1];
    fit = MIN(cell_width / width, cell_height / height);
    center = draw_unicode_string_capital_middle(cursor, L"");
    x = cursor->x0 + 4.0f * scale + (cell_width - width * fit) * 0.5f;
    y = center - height * fit * 0.5f;
    x -= entry->bounds[0] * fit;
    y -= entry->bounds[1] * fit;
    memset(&parameters, 0, sizeof(parameters));
    parameters.map[0] = bitmap;
    parameters.map_texture_scale[0].i = parameters.map_texture_scale[0].j = 1.0f;
    parameters.map_scale[0].i = parameters.map_scale[0].j = 1.0f;
    parameters.framebuffer_blend_function = 0; /* supplied straight-alpha artwork */
    color = ((icon->flags & 2 ? icon->color : color) & 0xff000000UL) | 0xffffffUL;
    for (i = 0; i < 4; i++)
    {
        boolean right = ((i + 1) & 2) != 0, bottom = i > 1;
        vertices[i].position.x = x + (right ? 512.0f * fit : 0.0f);
        vertices[i].position.y = y + (bottom ? 512.0f * fit : 0.0f);
        vertices[i].texture_coordinates.x = right ? 1.0f : 0.0f;
        vertices[i].texture_coordinates.y = bottom ? 1.0f : 0.0f;
        vertices[i].color = color;
    }
    rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);
    cursor->x0 += (short)((slot < 4 ? 22.0f : slot < 6 ? 20.0f : slot < 8 ? 45.0f : 33.0f) * scale);
    return TRUE;
}
