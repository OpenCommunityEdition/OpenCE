/*
HUD_HIRES_TAGS.C

The bitmaps the high-res HUD's textures stand for (port/linux/src/hud_hires.c),
found in each map's tags as it loads (scenario_tags_load): every map holds
its own copy of the HUD's bitmap groups.

A bitmap's pixels are loaded into the texture cache's memory at its
base_address (xbox_texture_cache.c), which it keeps until its cache block is
reused (when cache_block_index and base_address are cleared). The texture
cache of the platform layer uploads the pixels at an address whenever they
are written there, and asks hud_hires_asset_at which bitmap they are: a block
reused for another bitmap, or a map unloaded, is a write, which asks again.

A texture drawn for only some of a bitmap's sprites (hud_msg_icons_sm's
buttons) does not stand for its bitmap: the game draws those sprites from a
placeholder (hud_hires_sprite_bitmap), a bitmap of the same size and format,
so the same coordinates, made when first needed and deleted with the map;
the texture cache draws the texture for it (hud_hires_placeholder_texture).
When the texture cannot be drawn, the sprites are drawn from their bitmap.
*/

#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmap_group_lookup.h"
#include "bitmaps/bitmaps.h"
#include "rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.h"
#include "tag_files/tag_groups.h"
#include "game/players.h"
#include "game/game.h"
#include "interface/hud.h"
#include "interface/hud_definitions.h"
#include "rasterizer/rasterizer.h"
#include "text/draw_string.h"
#include "hud_button_layout.h"

/* the platform layer's (port/linux/src) */
void platform_log(char const *format, ...);
unsigned long config_changes(void);
int config_boolean(char const *name);
char const *config_string(char const *name);
long hud_hires_asset_count(void);
char const *hud_hires_asset_tag(long asset);
long hud_hires_asset_bitmap(long asset);
long hud_hires_asset_fits(long asset, long width, long height);
unsigned long hud_hires_asset_sprites(long asset);
int hud_hires_sprites_drawable(long asset, unsigned long address, unsigned long level0_size);
void hud_hires_register_placeholder(long asset, unsigned long const *texture);

void hud_hires_tags_loaded(void);
void hud_hires_tags_unloaded(void);
long hud_hires_asset_at(unsigned long address, long width, long height);
struct bitmap_data const *hud_hires_sprite_bitmap(struct bitmap_data const *bitmap, short sequence_index);

/* ---------- constants */

enum
{
	MAXIMUM_HIRES_BITMAPS = 128,
	MAXIMUM_HIRES_SPRITE_BITMAPS = 8,
};

/* ---------- globals */

static struct
{
	struct bitmap_data *bitmap;
	long asset;
} hires_bitmaps[MAXIMUM_HIRES_BITMAPS];
static long hires_bitmap_count = 0;

/* Sheets without redraws: retain their original lettering and sprite layout. */
static struct { struct bitmap_data *bitmap; long kind; } upscale_bitmaps[16];
static long upscale_bitmap_count;

/* the bitmaps some of whose sprites have a texture, with their placeholder,
and whether the texture can be drawn for the pixels at checked_address
(checked again when the texture cache loads them elsewhere, or the settings
change) */
static struct
{
	struct bitmap_data *bitmap;
	long asset;
	struct bitmap_data *placeholder;
	boolean placeholder_failed;
	void *checked_address;
	unsigned long checked_at;
	boolean drawable;
} hires_sprite_bitmaps[MAXIMUM_HIRES_SPRITE_BITMAPS];
static long hires_sprite_bitmap_count = 0;

/* Original Xbox controller faces, shared with the menu redraws. */
static struct
{
	struct bitmap_data *bitmap;
	long asset;
	boolean attempted;
} hud_buttons[14];
static short hud_buttons_style;

static void hud_buttons_forget(void)
{
	long index;
	for (index = 0; index < 14; index++)
	{
		if (hud_buttons[index].bitmap)
		{
			hud_hires_register_placeholder(hud_buttons[index].asset, NULL);
			bitmap_delete(hud_buttons[index].bitmap);
		}
	}
	csmemset(hud_buttons, 0, sizeof(hud_buttons));
}

/* ---------- private code */

static void hud_hires_sprite_bitmaps_forget(
	void)
{
	long index;

	for (index = 0; index < hires_sprite_bitmap_count; index++)
	{
		if (hires_sprite_bitmaps[index].placeholder)
		{
			hud_hires_register_placeholder(hires_sprite_bitmaps[index].asset, NULL);
			bitmap_delete(hires_sprite_bitmaps[index].placeholder);
		}
	}
	hires_sprite_bitmap_count = 0;
	hud_buttons_forget();

	return;
}

/* ---------- public code */

void hud_hires_tags_loaded(
	void)
{
	long asset_count = hud_hires_asset_count();
	long asset;
	long missing = 0;

	hires_bitmap_count = 0;
	hud_hires_sprite_bitmaps_forget();
	upscale_bitmap_count = 0;
	{
		static struct { char const *tag; short first, last; } const sheets[] = {
			{ "ui\\hud\\bitmaps\\combined\\hud_unit_backgrounds", 8, 12 },
			{ "ui\\hud\\bitmaps\\combined\\hud_reticle_warnings", 0, 0 },
			{ "ui\\hud\\bitmaps\\combined\\hud_msg_icons", 0, 3 },
			{ "ui\\hud\\bitmaps\\hud_msg_icons_sm", 0, 1 }
		};
		long sheet;
		short index;

		for (sheet = 0; sheet < NUMBEROF(sheets); sheet++)
		{
			long group = tag_loaded(BITMAP_GROUP_TAG, sheets[sheet].tag);

			if (group == NONE)
				continue;
			for (index = sheets[sheet].first; index <= sheets[sheet].last; index++)
			{
				struct bitmap_data *bitmap = bitmap_group_try_and_get_bitmap(group, index);

				if (bitmap && upscale_bitmap_count < NUMBEROF(upscale_bitmaps))
				{
					upscale_bitmaps[upscale_bitmap_count].bitmap = bitmap;
					upscale_bitmaps[upscale_bitmap_count++].kind = sheet == 0 ?
						(index == 9 || index == 10 ? 3 : 2) : 1;
				}
			}
		}
	}
	for (asset = 0; asset < asset_count && hires_bitmap_count < MAXIMUM_HIRES_BITMAPS; asset++)
	{
		long group_index = tag_loaded(BITMAP_GROUP_TAG, hud_hires_asset_tag(asset));
		struct bitmap_data *bitmap = group_index == NONE ? NULL :
			bitmap_group_try_and_get_bitmap(group_index, (short)hud_hires_asset_bitmap(asset));

		if (!bitmap)
		{
			/* (the main menu's map has only some of the HUD) */
			missing++;
		}
		else if (!hud_hires_asset_fits(asset, bitmap->width, bitmap->height))
		{
			platform_log("high-res hud: %s bitmap %ld is %dx%d here, which its texture does not fit",
				hud_hires_asset_tag(asset), hud_hires_asset_bitmap(asset), bitmap->width, bitmap->height);
		}
		else if (hud_hires_asset_sprites(asset))
		{
			if (hires_sprite_bitmap_count < MAXIMUM_HIRES_SPRITE_BITMAPS)
			{
				csmemset(&hires_sprite_bitmaps[hires_sprite_bitmap_count], 0, sizeof(hires_sprite_bitmaps[0]));
				hires_sprite_bitmaps[hires_sprite_bitmap_count].bitmap = bitmap;
				hires_sprite_bitmaps[hires_sprite_bitmap_count].asset = asset;
				hires_sprite_bitmap_count++;
			}
		}
		else
		{
			hires_bitmaps[hires_bitmap_count].bitmap = bitmap;
			hires_bitmaps[hires_bitmap_count].asset = asset;
			hires_bitmap_count++;
		}
	}
	platform_log("high-res hud: %ld of %ld bitmaps in this map, %ld for some of their sprites (%ld not in it)",
		hires_bitmap_count + hires_sprite_bitmap_count, asset_count, hires_sprite_bitmap_count, missing);

	return;
}

void hud_hires_tags_unloaded(
	void)
{
	hires_bitmap_count = 0;
	upscale_bitmap_count = 0;
	hud_hires_sprite_bitmaps_forget();

	return;
}

struct bitmap_data const *hud_hires_sprite_bitmap(
	struct bitmap_data const *bitmap,
	short sequence_index)
{
	long index;

	for (index = 0; index < hires_sprite_bitmap_count; index++)
	{
		struct bitmap_data *sprite_bitmap = hires_sprite_bitmaps[index].bitmap;

		if (sprite_bitmap != bitmap ||
			sequence_index < 0 ||
			sequence_index >= 32 ||
			!(hud_hires_asset_sprites(hires_sprite_bitmaps[index].asset) & (1UL << sequence_index)))
		{
			continue;
		}
		/* (checked on its pixels in the texture cache, once they are loaded) */
		if (sprite_bitmap->cache_block_index == NONE || !sprite_bitmap->base_address)
			break;
		if (hires_sprite_bitmaps[index].checked_address != sprite_bitmap->base_address ||
			hires_sprite_bitmaps[index].checked_at != config_changes())
		{
			hires_sprite_bitmaps[index].checked_address = sprite_bitmap->base_address;
			hires_sprite_bitmaps[index].checked_at = config_changes();
			hires_sprite_bitmaps[index].drawable = hud_hires_sprites_drawable(
				hires_sprite_bitmaps[index].asset,
				(unsigned long)sprite_bitmap->base_address,
				(unsigned long)bitmap_mipmap_get_pixel_data_size(sprite_bitmap, 0)) != 0;
		}
		if (hires_sprite_bitmaps[index].drawable &&
			!hires_sprite_bitmaps[index].placeholder &&
			!hires_sprite_bitmaps[index].placeholder_failed)
		{
			struct bitmap_data *placeholder = bitmap_2d_new(
				sprite_bitmap->width,
				sprite_bitmap->height,
				0,
				sprite_bitmap->format);

			if (placeholder && !rasterizer_bitmap_new(placeholder))
			{
				bitmap_delete(placeholder);
				placeholder = NULL;
			}
			if (placeholder)
			{
				hud_hires_register_placeholder(
					hires_sprite_bitmaps[index].asset,
					(unsigned long const *)placeholder->hardware_format);
			}
			else
			{
				platform_log("high-res hud: no placeholder for %s's sprites: drawn as they are",
					hud_hires_asset_tag(hires_sprite_bitmaps[index].asset));
				hires_sprite_bitmaps[index].placeholder_failed = TRUE;
			}
			hires_sprite_bitmaps[index].placeholder = placeholder;
		}
		if (hires_sprite_bitmaps[index].drawable && hires_sprite_bitmaps[index].placeholder)
			return hires_sprite_bitmaps[index].placeholder;
		break;
	}

	return bitmap;
}

long hud_hires_asset_at(
	unsigned long address,
	long width,
	long height)
{
	long index;

	for (index = 0; index < hires_bitmap_count; index++)
	{
		struct bitmap_data *bitmap = hires_bitmaps[index].bitmap;

		if (bitmap->cache_block_index != NONE &&
			(unsigned long)bitmap->base_address == address &&
			bitmap->width == width &&
			bitmap->height == height)
		{
			return hires_bitmaps[index].asset;
		}
	}

	return NONE;
}

long hud_hires_upscale_at(unsigned long address, long width, long height)
{
	long index;

	for (index = 0; index < upscale_bitmap_count; index++)
	{
		struct bitmap_data const *bitmap = upscale_bitmaps[index].bitmap;

		if (bitmap->cache_block_index != NONE && (unsigned long)bitmap->base_address == address &&
			bitmap->width == width && bitmap->height == height)
			return upscale_bitmaps[index].kind;
	}
	return FALSE;
}

/* Match the controller entry itself: weapon icons may reuse its sequence.
 * Draw the existing Xbox menu faces with straight alpha, independently of
 * the messaging sheet and its tint. Disabling the HUD keeps the map's art. */
boolean hud_hires_button_draw(struct icon_hud_element_definition const *icon,
	rectangle2d *cursor, pixel32 color)
{
	static char const *names[] = {
		"ui\\shell\\bitmaps\\a_butn", "ui\\shell\\bitmaps\\b_butn",
		"ui\\shell\\bitmaps\\x_butn", "ui\\shell\\bitmaps\\y_butn",
		"ui\\shell\\bitmaps\\black_butn", "ui\\shell\\bitmaps\\white_butn"};
	static char const *classic_names[] = {
		"ui\\hud\\buttons\\classic_a", "ui\\hud\\buttons\\classic_b",
		"ui\\hud\\buttons\\classic_x", "ui\\hud\\buttons\\classic_y",
		"ui\\hud\\buttons\\classic_black", "ui\\hud\\buttons\\classic_white"};
	static char const *gem_names[] = {
		"ui\\hud\\buttons\\gem_a", "ui\\hud\\buttons\\gem_b",
		"ui\\hud\\buttons\\gem_x", "ui\\hud\\buttons\\gem_y",
		"ui\\shell\\bitmaps\\black_butn", "ui\\shell\\bitmaps\\white_butn"};
	struct icon_hud_element_definition const *buttons;
	struct rasterizer_dynamic_screen_geometry_parameters parameters;
	struct dynamic_screen_vertex vertices[4];
	struct bitmap_data *bitmap;
	real viewport, scale, x, y, width, height, u, v;
	short advance;
	long index, asset;
	short i;
	short style, slot;

	if (!config_boolean("display.high_res_hud") || !hud_globals)
		return FALSE;
	buttons = hud_globals->messaging.button_icons.address;
	if (!buttons) return FALSE;
	for (index = 0; index < 18 && index < hud_globals->messaging.button_icons.count; index++)
		if (icon == buttons + index) break;
	if (index >= 18 || index >= hud_globals->messaging.button_icons.count)
		return FALSE;
	slot = index < 6 ? (short)index : hud_button_control_slot((short)index,
		icon->frame_rate > 0 ? game_time_get() / icon->frame_rate : 0);
	if (slot < 0) return FALSE;
	style = (!strcmp(config_string("display.hud_button_style"), "dark_gems") || !strcmp(config_string("display.hud_button_style"), "menu_gems")) ? 2 :
		!strcmp(config_string("display.hud_button_style"), "gems");
	if (style != hud_buttons_style)
	{
		hud_buttons_forget();
		hud_buttons_style = style;
	}
	if (!hud_buttons[slot].attempted)
	{
		hud_buttons[slot].attempted = TRUE;
		for (asset = 0; asset < hud_hires_asset_count(); asset++)
		{
			if (strcmp(hud_hires_asset_tag(asset), slot >= 6 ? hud_button_control_name(slot) : style == 0 ? classic_names[index] :
				style == 1 ? gem_names[index] : names[index])) continue;
			bitmap = bitmap_2d_new(slot >= 6 ? 64 : 32, slot >= 6 ? 64 : 32, 0, 11);
			if (!bitmap) break;
			if (!rasterizer_bitmap_new(bitmap))
			{
				bitmap_delete(bitmap);
				break;
			}
			hud_hires_register_placeholder(asset, bitmap->hardware_format);
			hud_buttons[slot].asset = asset;
			hud_buttons[slot].bitmap = bitmap;
			break;
		}
	}
	bitmap = hud_buttons[slot].bitmap;
	if (!bitmap) return FALSE;
	viewport = local_player_count() > 1 ? 0.75f : 1.0f;
	scale = viewport;
	/* Account for the text renderer's leading inset on the following run. */
	x = cursor->x0 + 4.0f * viewport;
	y = draw_unicode_string_capital_middle(cursor, L"") + 13.0f * viewport;
	width = 21.0f * scale;
	height = 26.0f * scale;
	advance = (short)(cursor->x0 + (index < 4 ? 22.0f : 20.0f) * viewport);
	y -= height;
	hud_button_face_bounds(style, (short)index, &x, &y, &width, &height);
	u = 21.0f / 32.0f;
	v = 26.0f / 32.0f;
	if (slot >= 6)
	{
		width = (slot < 8 ? 44.0f : 32.0f) * scale;
		height = (slot < 8 ? 33.0f : 34.0f) * scale;
		x = cursor->x0 + 4.0f * viewport;
		y = draw_unicode_string_capital_middle(cursor, L"") - height * 0.5f;
		u = (slot < 8 ? 50.0f : 36.0f) / 64.0f;
		v = (slot < 8 ? 35.0f : 40.0f) / 64.0f;
		advance = (short)(cursor->x0 + width + viewport);
		hud_button_control_bounds(slot, &x, &y, &width, &height);
	}
	csmemset(&parameters, 0, sizeof(parameters));
	parameters.map[0] = bitmap;
	parameters.map_texture_scale[0].i = parameters.map_texture_scale[0].j = 1.0f;
	parameters.map_scale[0].i = parameters.map_scale[0].j = 1.0f;
	/* Stick/trigger art uses the original HUD blend, with premultiplied edges. */
	parameters.framebuffer_blend_function = slot >= 6 ? 7 : 0;
	/* Keep the icon's authored opacity, as the low-resolution path does.
	 * The embedded artwork supplies RGB; prompt text may be translucent. */
	if (icon->flags & 2) color = icon->color;
	for (i = 0; i < 4; i++)
	{
		boolean right = ((i + 1) & 2) != 0, bottom = i > 1;
		vertices[i].position.x = x + (right ? width : 0.0f);
		vertices[i].position.y = y + (bottom ? height : 0.0f);
		vertices[i].texture_coordinates.x = right ? u : 0.0f;
		vertices[i].texture_coordinates.y = bottom ? v : 0.0f;
		vertices[i].color = slot >= 6 ? color : (color & 0xFF000000UL) | 0xFFFFFFUL;
	}
	rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);
	cursor->x0 = advance;
	return TRUE;
}
