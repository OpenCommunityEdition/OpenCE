#ifndef __DECAL_LIMIT_H__
#define __DECAL_LIMIT_H__

/* game.decals: how many decals a map keeps, as a multiple of the game's
(port/linux/game/decal_limit.c) */
long decal_limit_multiplier(void);

/* source/rasterizer/xbox/rasterizer_xbox_decals.c */
boolean rasterizer_decals_port_storage(long multiplier);
void rasterizer_decals_port_flush(void);

/* source/effects/decals.c */
void decals_port_after_load(void);

#endif
