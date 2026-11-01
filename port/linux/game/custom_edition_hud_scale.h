/* SPDX-License-Identifier: GPL-3.0-only
 * HUD flag composition adapted from SnowyMouse/Chimera's
 * src/chimera/fix/hud_bitmap_scale.cpp, check_half_scale_bitmap_flag.
 * ChupathingyCE #352 (Boom Pen / ugoboom) applies these rules to the
 * decompilation. Kept here for CE conversion and CE drawing only.
 */
#ifndef CUSTOM_EDITION_HUD_SCALE_H
#define CUSTOM_EDITION_HUD_SCALE_H

static float custom_edition_hud_scale(unsigned short bitmap_flags, int high_resolution)
{
	float scale = (high_resolution || (bitmap_flags & 0x80)) ? 0.5f : 1.0f;

	/* The bitmap's half-scale flag compounds with the high-resolution
	choice; forcing that choice does not apply it twice. */
	if (bitmap_flags & 0x10)
		scale *= 0.5f;
	return scale;
}

#endif
