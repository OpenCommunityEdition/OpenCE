/*
CUSTOM_EDITION_MAPS.H

The Halo Custom Edition maps in the multiplayer menus of the native builds.
With the game.custom_edition setting on, the multiplayer level list
(source/interface/ui_widget_event_handler_functions.c) offers the Custom
Edition multiplayer maps in the maps folder after the Xbox levels. The menus
that show a level (ui_widget_game_data_input_functions.c) know a level by an
index into the strings and frames of their own tags; these maps get display
indices beyond those, and text_group.c and ui_widget.c ask this unit for
their names, descriptions and pictures (custom_edition_maps.c).
*/

#ifndef __CUSTOM_EDITION_MAPS_H
#define __CUSTOM_EDITION_MAPS_H

/* ---------- structures */

struct bitmap_data;

/* ---------- prototypes/CUSTOM_EDITION_MAPS.C */

/* Looks for the Custom Edition multiplayer maps anew and returns the levels
the level list offers: the `xbox_level_count` levels of `xbox_levels`, then
those maps in the order of their names; `*level_count` is how many there
are. */
char **custom_edition_maps_level_list(
	char **xbox_levels,
	short xbox_level_count,
	short *level_count);

/* The display index of level `level_index` of the latest level list: an
Xbox level's own index, or a Custom Edition map's display index. */
short custom_edition_maps_level_display_index(
	short level_index);

/* The display index of the Custom Edition map the level name `level_name`
(a network game's map name) names, or NONE when it names none of the maps
found; the maps are looked for the first time this is asked. */
short custom_edition_maps_display_index(
	char const *level_name);

/* A Custom Edition map's name and description, for the display index
`display_index`, or NULL when that is no map's (text_group.c). */
wchar_t *custom_edition_maps_name(
	short display_index);
wchar_t *custom_edition_maps_description(
	short display_index);

/* What a bitmap widget showing frame `*frame_index` of the bitmap tag
`bitmap_tag_index` draws instead (ui_widget.c): when that is the level
pictures' tag and the frame a Custom Edition map's display index, the map's
picture, to be drawn over the whole widget, or NULL with `*frame_index` made
the unknown level's frame when the map has no picture. NULL, the frame
unchanged, for any other frame or tag. */
struct bitmap_data *custom_edition_maps_picture(
	long bitmap_tag_index,
	short *frame_index);

#endif
