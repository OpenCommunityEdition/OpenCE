/*
DECAL_LIMIT.C

How many decals (bullet holes, scorch marks, blood) a map keeps before the
oldest give way: game.decals, "default" (the game's own, 2048), "4x", "8x"
or "insane" (twelve times). The game's own decals live in the game state;
the larger limits' live outside it (decals.c, rasterizer_xbox_decals.c),
allocated as a map begins with such a limit and let go when one begins
without, so that "default" costs nothing and saved games keep the game's
layout. A change in the menu takes at the next map. Desktop builds only.
*/

#include "cseries/cseries.h"
#include "decal_limit.h"

const char *config_string(const char *name);

long decal_limit_multiplier(
	void)
{
#if defined(HALO_ANDROID) || defined(HALO_WEB)
	return 1;
#else
	const char *limit = config_string("game.decals");

	if (!limit)
		return 1;
	if (!strcmp(limit, "4x"))
		return 4;
	if (!strcmp(limit, "8x"))
		return 8;
	if (!strcmp(limit, "insane"))
		return 12;
	return 1;
#endif
}
