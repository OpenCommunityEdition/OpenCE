/* CE warning graphics use crosshair states but are not aiming reticles. */
#ifndef CUSTOM_EDITION_HUD_H
#define CUSTOM_EDITION_HUD_H
#include "cache_file_formats.h"
#include "custom_edition_cache.h"

static boolean custom_edition_hud_warning(short state)
{
	return custom_edition_cache_tags_loaded() &&
		((state >= 3 && state <= 15) || state == 18);
}

/* Legacy HUDs placed warnings beside corner panels using offsets from a
 * 640x480 canvas center with PC's 8-pixel HUD margins (Chimera's
 * hud_bitmap_scale.hpp). Translate to the current title-safe frame, just
 * as the corner panels do. Centered warnings remain centered. */
static void custom_edition_hud_warning_placement(short state, short width, short height,
	struct hud_placement_definition *placement)
{
	long extra;
	if (!custom_edition_hud_warning(state) ||
		!custom_edition_cache_relies_on(_custom_edition_behaviour_old_widescreen_fix))
		return;
	extra = (width - 624) / 2;
	if (placement->offset.x > 0)
		placement->offset.x = (short)PIN(placement->offset.x + extra, SHORT_MIN, SHORT_MAX);
	else if (placement->offset.x < 0)
		placement->offset.x = (short)PIN(placement->offset.x - extra, SHORT_MIN, SHORT_MAX);
	extra = (height - 464) / 2;
	if (placement->offset.y > 0)
		placement->offset.y = (short)PIN(placement->offset.y + extra, SHORT_MIN, SHORT_MAX);
	else if (placement->offset.y < 0)
		placement->offset.y = (short)PIN(placement->offset.y - extra, SHORT_MIN, SHORT_MAX);
}

#endif
