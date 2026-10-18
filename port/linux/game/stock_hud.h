/*
STOCK_HUD.H

The singleplayer HUD on a Custom Edition map (stock_hud.c).
*/

#ifndef STOCK_HUD_H
#define STOCK_HUD_H
#pragma once

#include "cseries.h"

/* (scenario_tags_load, after the menus' tags, on a Custom Edition map
only) */
void stock_hud_tags_loaded(
	void);
/* (scenario_tags_unload, after the texture cache has closed and before the
menus put the map's own tag table back) */
void stock_hud_tags_unloaded(
	void);
/* a read cache_file_read made of one of the injected bitmap tags: the
pixels are the stock map's. FALSE leaves the read to the loaded map */
boolean stock_hud_read(
	long tag_index,
	long offset,
	long size,
	void *buffer);

#endif
