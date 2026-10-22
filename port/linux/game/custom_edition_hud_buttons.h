/* Original controller sprites for CE maps whose messaging sheet holds weapons.
 * Resource tag addresses are offsets into that item, not engine pointers. */
#ifndef CUSTOM_EDITION_HUD_BUTTONS_H
#define CUSTOM_EDITION_HUD_BUTTONS_H
#include "cache_file_formats.h"
#include "custom_edition_cache.h"
#include "interface/hud_draw.h"

static struct
{
	byte tag[8192];
	unsigned long size;
	struct bitmap_data *bitmaps[2];
	boolean attempted;
} ce_hud_buttons;

static unsigned long ce_hud_button_u32(byte const *p)
{
	return (unsigned long)p[0] | ((unsigned long)p[1] << 8) |
		((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

static byte const *ce_hud_button_range(unsigned long offset, unsigned long count, unsigned long stride)
{
	if (offset > ce_hud_buttons.size || count > (ce_hud_buttons.size - offset) / stride)
		return NULL;
	return ce_hud_buttons.tag + offset;
}

static void ce_hud_buttons_forget(void)
{
	short i;
	for (i = 0; i < 2; i++)
		if (ce_hud_buttons.bitmaps[i]) bitmap_delete(ce_hud_buttons.bitmaps[i]);
	csmemset(&ce_hud_buttons, 0, sizeof(ce_hud_buttons));
}

static boolean ce_hud_button_draw(struct icon_hud_element_definition const *icon,
	rectangle2d *cursor, pixel32 color)
{
	struct resource_map const *resources = custom_edition_cache_bitmap_resources();
	byte const *sequences, *sequence, *sprites, *data;
	struct bitmap_group_sprite sprite;
	struct bitmap_data *bitmap;
	unsigned long count, frame, bitmap_index, offset, bytes;
	long i;
	real viewport, scale, width, height;
	point2d point;
	if (!resources) return FALSE;
	if (!ce_hud_buttons.attempted)
	{
		ce_hud_buttons.attempted = TRUE;
		for (i = 0; i < resources->item_count; i++)
		{
			struct resource_map_item const *item = &resources->items[i];
			if (strcmp(item->name, "ui\\hud\\bitmaps\\hud_msg_icons_sm")) continue;
			if (item->size >= 108 && item->size <= sizeof(ce_hud_buttons.tag) &&
				resources->source->read(resources->source->context, item->data_offset,
					item->size, ce_hud_buttons.tag)) ce_hud_buttons.size = item->size;
			break;
		}
	}
	if (!ce_hud_buttons.size || icon->sequence_index < 0) return FALSE;
	count = ce_hud_button_u32(ce_hud_buttons.tag + 84);
	sequences = ce_hud_button_range(ce_hud_button_u32(ce_hud_buttons.tag + 88), count, 64);
	if (!sequences || (unsigned long)icon->sequence_index >= count) return FALSE;
	sequence = sequences + icon->sequence_index * 64;
	count = ce_hud_button_u32(sequence + 52);
	sprites = ce_hud_button_range(ce_hud_button_u32(sequence + 56), count, 32);
	if (!sprites || !count) return FALSE;
	frame = icon->frame_rate > 0 ? (unsigned long)(game_time_get() / icon->frame_rate) % count : 0;
	csmemcpy(&sprite, sprites + frame * 32, sizeof(sprite));
	if (!(sprite.bounds.x0 >= 0 && sprite.bounds.x1 <= 1 && sprite.bounds.x1 > sprite.bounds.x0 &&
		sprite.bounds.y0 >= 0 && sprite.bounds.y1 <= 1 && sprite.bounds.y1 > sprite.bounds.y0)) return FALSE;
	bitmap_index = (unsigned long)sprite.bitmap_index;
	count = ce_hud_button_u32(ce_hud_buttons.tag + 96);
	data = ce_hud_button_range(ce_hud_button_u32(ce_hud_buttons.tag + 100), count, 48);
	if (!data || bitmap_index >= count || bitmap_index >= 2) return FALSE;
	data += bitmap_index * 48;
	/* The stock controller sheet is two 128-square, linear ARGB images.
	 * Refuse other layouts rather than interpreting custom resource data. */
	if (ce_hud_button_u32(data + 4) != 0x00800080 ||
		ce_hud_button_u32(data + 8) != 1 || (data[12] != 11 || data[13]) ||
		(data[14] & (2 | 4 | 8 | 16))) return FALSE;
	bitmap = ce_hud_buttons.bitmaps[bitmap_index];
	if (!bitmap)
	{
		offset = ce_hud_button_u32(data + 24);
		bytes = ce_hud_button_u32(data + 28);
		if (bytes < 65536 || offset > resources->source->size ||
			65536 > resources->source->size - offset) return FALSE;
		bitmap = bitmap_2d_new(128, 128, 0, 11);
		if (!bitmap) return FALSE;
		if (!bitmap->base_address || !resources->source->read(resources->source->context,
			offset, 65536, bitmap->base_address) || !rasterizer_bitmap_new(bitmap))
		{
			bitmap_delete(bitmap);
			return FALSE;
		}
		rasterizer_bitmap_changed(bitmap);
		ce_hud_buttons.bitmaps[bitmap_index] = bitmap;
	}
	/* Match the native gameplay face's 26-pixel height: the CE resource
	 * uses 20-pixel faces. Align to the active font's capitals, not the
	 * stock font's baseline offsets. Keep all sprite proportions/frames. */
	viewport = local_player_count() > 1 ? 0.75f : 1.0f;
	scale = viewport * (26.0f / 20.0f);
	width = (sprite.bounds.x1 - sprite.bounds.x0) * bitmap->width * scale;
	height = (sprite.bounds.y1 - sprite.bounds.y0) * bitmap->height * scale;
	point.x = (short)(cursor->x0 + 4.0f * viewport);
	point.y = (short)(draw_unicode_string_capital_middle(cursor, L"") + height * 0.5f);
	hud_draw_bitmap_direct(bitmap, _hud_anchor_bottom_left, &point, &sprite.bounds,
		scale, 0.0f, icon->flags & 2 ? icon->color : color, FALSE);
	/* The following text run supplies its own three-pixel leading inset. */
	cursor->x0 = (short)(point.x + width - 3.0f * viewport);
	return TRUE;
}
#endif
