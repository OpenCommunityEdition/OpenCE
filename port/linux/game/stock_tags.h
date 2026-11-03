/*
STOCK_TAGS.H

The tags a mutator needs that a Custom Edition map lacks, taken from the
player's own stock maps (stock_tags.c, docs/mutators.md). Nothing here runs
on a stock map, or when no mutator asks for a tag the map lacks.
*/

#ifndef __STOCK_TAGS_H
#define __STOCK_TAGS_H
#pragma once

/* ---------- prototypes/STOCK_TAGS.C */

struct bitmap_data;

/* the tag of a group and name, with every tag it uses that the map lacks,
added to a Custom Edition map's tags from a stock map: its tag index, or NONE
(said) if it cannot be, or at once if the map is a stock map. An add-on's
imports are found by stock_tags_find_before_menus, as the map's tags load;
a tag edit's by stock_tags_find, whenever the edits come */
long stock_tags_find_before_menus(
	long group_tag,
	char const *name);
long stock_tags_find(
	long group_tag,
	char const *name);
/* the stock map held for those finds let go of: call after each set of them */
void stock_tags_finish(
	void);

/* whether any tag was taken from a stock map for this map */
extern boolean stock_tags_in_map;
/* whether the bitmap is one of these tags', drawn as the Xbox's are (a Custom
Edition map's others are drawn as Halo PC's) */
boolean stock_tags_bitmap_contains(
	struct bitmap_data const *bitmap);
/* a bitmap's pixels read into buffer at once: FALSE if it is not one of
these tags' */
boolean stock_tags_bitmaps_read(
	struct bitmap_data *bitmap,
	void *buffer);
/* a read of one of these tags' pixels or samples: FALSE if it is not one */
boolean stock_tags_read(
	long tag_index,
	long offset,
	long size,
	void *buffer);

/* before the menus' tags go, which those of stock_tags_find follow */
void stock_tags_menus_unloading(
	void);
/* after the add-ons' tags go: all of it taken back */
void stock_tags_unloaded(
	void);

#endif // __STOCK_TAGS_H
