/*
CONTROLLER_GLYPHS.C

The HUD's buttons in the styles of port/assets/controller_buttons (Xbox
Modern, the PlayStation's and the Switch's: controller_buttons.h), drawn in
the cells of the Xbox's (hud_hires_tags.c, which draws the Xbox's first
three styles and calls here for the others). A style's artwork is found by
its names, as tools/controller_button_assets.py writes them
(controller_glyph_assets.h): "<style>_<slot>" for its A, B, X and Y, and
"<family>_control_<slot>" for its family's shoulders, triggers and sticks.

A drawn button's texture is kept by its artwork, not by its viewport, so
that split screen players with different controllers keep each other's; the
textures go with the map (controller_glyphs_dispose).
*/

#include "cseries.h"
#include "controller_buttons.h"
#include "controller_glyphs.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "game/game.h"
#include "game/players.h"
#include "interface/hud_definitions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "hud_button_layout.h"

#include <stdio.h>
#include <string.h>

long hud_hires_asset_count(void);
char const *hud_hires_asset_tag(long asset);
void hud_hires_register_placeholder(long asset, unsigned long const *texture);

/* the button slots: A, B, X and Y (0 to 3), then the shoulders, triggers
and sticks' frames (4 to 13: hud_button_control_slot) */
#define CONTROLLER_GLYPH_SLOTS 14
#define CONTROLLER_GLYPH_FACES 4
/* the artwork's side, 512 (8 times the HUD's 64), and its texture's */
#define CONTROLLER_GLYPH_ARTWORK_SIZE 512.0f
#define CONTROLLER_GLYPH_TEXTURE_SIZE 64

/* bitmaps.c's format of 32-bit color with alpha */
enum
{
	_bitmap_format_a8r8g8b8 = 11,
};

struct controller_glyph_asset
{
	char const *tag;
	/* the artwork's half-alpha bounds: left, top, right, bottom */
	short bounds[4];
};

#include "controller_glyph_assets.h"

static char const *const family_names[NUMBER_OF_CONTROLLER_FAMILIES] = { "", "xbox", "ps", "switch" };

static struct
{
	struct bitmap_data *bitmap;
	long asset;
	boolean attempted;
} controller_glyph_cache[NUMBEROF(controller_glyph_assets)];

/* a style's artwork for a slot: its index in controller_glyph_assets, or
NONE (the Xbox's first three styles, which have none here) */
static short controller_glyph_asset(int style, short slot)
{
	static short indices[NUMBER_OF_BUTTON_STYLES][CONTROLLER_GLYPH_SLOTS];
	static boolean found = FALSE;

	if (style < 0 || style >= NUMBER_OF_BUTTON_STYLES || slot < 0 || slot >= CONTROLLER_GLYPH_SLOTS)
		return NONE;
	if (!found)
	{
		int each_style;
		short each_slot, asset;
		char tag[64];

		for (each_style = 0; each_style < NUMBER_OF_BUTTON_STYLES; each_style++)
		{
			for (each_slot = 0; each_slot < CONTROLLER_GLYPH_SLOTS; each_slot++)
			{
				if (each_slot < CONTROLLER_GLYPH_FACES)
					snprintf(tag, sizeof(tag), "ui\\controller_buttons\\%s_%d",
						controller_button_style_name(each_style), each_slot);
				else
					snprintf(tag, sizeof(tag), "ui\\controller_buttons\\%s_control_%d",
						family_names[controller_button_style_family(each_style)], each_slot);
				indices[each_style][each_slot] = NONE;
				for (asset = 0; asset < (short)NUMBEROF(controller_glyph_assets); asset++)
				{
					if (!strcmp(controller_glyph_assets[asset].tag, tag))
					{
						indices[each_style][each_slot] = asset;
						break;
					}
				}
			}
			/* (a style without its own faces is drawn by hud_hires_tags.c,
			its shoulders and sticks too) */
			if (indices[each_style][0] == NONE)
			{
				for (each_slot = 0; each_slot < CONTROLLER_GLYPH_SLOTS; each_slot++)
					indices[each_style][each_slot] = NONE;
			}
		}
		found = TRUE;
	}
	return indices[style][slot];
}

int controller_glyph_style(void)
{
	return controller_button_style_for_port(render.local_player_index);
}

void controller_glyphs_dispose(void)
{
	long index;

	for (index = 0; index < (long)NUMBEROF(controller_glyph_cache); index++)
	{
		if (controller_glyph_cache[index].bitmap)
		{
			hud_hires_register_placeholder(controller_glyph_cache[index].asset, NULL);
			bitmap_delete(controller_glyph_cache[index].bitmap);
		}
	}
	memset(controller_glyph_cache, 0, sizeof(controller_glyph_cache));
}

/* an artwork's texture, made the first time it is drawn */
static struct bitmap_data *controller_glyph_bitmap(short key)
{
	long asset;

	if (!controller_glyph_cache[key].attempted)
	{
		controller_glyph_cache[key].attempted = TRUE;
		for (asset = 0; asset < hud_hires_asset_count(); asset++)
		{
			struct bitmap_data *bitmap;

			if (strcmp(hud_hires_asset_tag(asset), controller_glyph_assets[key].tag))
				continue;
			bitmap = bitmap_2d_new(CONTROLLER_GLYPH_TEXTURE_SIZE, CONTROLLER_GLYPH_TEXTURE_SIZE, 0, _bitmap_format_a8r8g8b8);
			if (!bitmap)
				break;
			if (!rasterizer_bitmap_new(bitmap))
			{
				bitmap_delete(bitmap);
				break;
			}
			hud_hires_register_placeholder(asset, bitmap->hardware_format);
			controller_glyph_cache[key].asset = asset;
			controller_glyph_cache[key].bitmap = bitmap;
			break;
		}
	}
	return controller_glyph_cache[key].bitmap;
}

boolean controller_glyph_draw(int style, short index, struct icon_hud_element_definition const *icon,
	rectangle2d *cursor, pixel32 color)
{
	struct rasterizer_dynamic_screen_geometry_parameters parameters;
	struct dynamic_screen_vertex vertices[4];
	struct controller_glyph_asset const *entry;
	struct bitmap_data *bitmap;
	short slot, key, vertex;
	real scale, cell_width, cell_height, advance, width, height, fit, x, y, middle;

	slot = index < 6 ? index : hud_button_control_slot(index,
		icon->frame_rate > 0 ? game_time_get() / icon->frame_rate : 0);
	key = controller_glyph_asset(style, slot);
	if (key == NONE)
		return FALSE;
	bitmap = controller_glyph_bitmap(key);
	if (!bitmap)
		return FALSE;
	entry = &controller_glyph_assets[key];
	/* the Xbox's cells: its faces 21 by 26, its shoulders' (4 and 5), its
	triggers' (6 and 7) and its sticks', each advancing the line by its
	own; three quarters of the size in split screen */
	scale = local_player_count() > 1 ? 0.75f : 1.0f;
	cell_width = (slot < 6 ? 21.0f : slot < 8 ? 44.0f : 32.0f) * scale;
	cell_height = (slot < 6 ? 26.0f : slot < 8 ? 33.0f : 34.0f) * scale;
	advance = (slot < CONTROLLER_GLYPH_FACES ? 22.0f : slot < 6 ? 20.0f : slot < 8 ? 45.0f : 33.0f) * scale;
	/* (the artwork's visible part fitted to the cell, centred on the line's
	capitals: its transparent margins move nothing) */
	width = entry->bounds[2] - entry->bounds[0];
	height = entry->bounds[3] - entry->bounds[1];
	fit = MIN(cell_width / width, cell_height / height);
	middle = draw_unicode_string_capital_middle(cursor, L"");
	x = cursor->x0 + 4.0f * scale + (cell_width - width * fit) * 0.5f - entry->bounds[0] * fit;
	y = middle - height * fit * 0.5f - entry->bounds[1] * fit;
	memset(&parameters, 0, sizeof(parameters));
	parameters.map[0] = bitmap;
	parameters.map_texture_scale[0].i = parameters.map_texture_scale[0].j = 1.0f;
	parameters.map_scale[0].i = parameters.map_scale[0].j = 1.0f;
	/* (the artwork's alpha is straight, not premultiplied) */
	parameters.framebuffer_blend_function = 0;
	/* (white, at the icon's or the message's opacity) */
	color = ((icon->flags & 2 ? icon->color : color) & 0xff000000UL) | 0xffffffUL;
	for (vertex = 0; vertex < 4; vertex++)
	{
		boolean right = ((vertex + 1) & 2) != 0;
		boolean bottom = vertex > 1;

		vertices[vertex].position.x = x + (right ? CONTROLLER_GLYPH_ARTWORK_SIZE * fit : 0.0f);
		vertices[vertex].position.y = y + (bottom ? CONTROLLER_GLYPH_ARTWORK_SIZE * fit : 0.0f);
		vertices[vertex].texture_coordinates.x = right ? 1.0f : 0.0f;
		vertices[vertex].texture_coordinates.y = bottom ? 1.0f : 0.0f;
		vertices[vertex].color = color;
	}
	rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);
	cursor->x0 += (short)advance;
	return TRUE;
}
