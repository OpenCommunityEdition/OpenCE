/*
CUSTOM_EDITION_CACHE.H

What the native builds' cache file loader (source/cache/cache_files.c and
cache_files_windows.c) does with Halo Custom Edition caches, OpenSauce ".yelo"
caches among them. By default it finds them, says what they are, and refuses
them. With the game.custom_edition setting on it loads and runs them
instead, which is experimental: their tags are laid out for Halo PC, and
custom_edition_cache.c and custom_edition_geometry.c convert what they know
to differ (docs/custom_edition_caches.md). The format itself is read by
cache_file_formats.c.
*/

#ifndef __CUSTOM_EDITION_CACHE_H
#define __CUSTOM_EDITION_CACHE_H

/* ---------- structures */

struct cache_file_tag_header;
struct custom_edition_load_report;
struct scenario_object_datum;
struct structure_bsp;

/* ---------- prototypes/CUSTOM_EDITION_CACHE.C */

/* When `header` (CACHE_FILE_HEADER_BYTES bytes, with `build` its build
string field) is a Custom Edition cache header, logs what it is, asserts when
`fatal` as cache_file_header_verify does, and returns TRUE. */
boolean custom_edition_cache_refuse(
	void const *header,
	char const *build,
	char const *scenario_name,
	boolean fatal);

/* When the ".map" file `path` names does not exist but an OpenSauce ".yelo"
file of the same name does, makes `path` (`path_size` characters) name that
instead. */
void opensauce_cache_path_find(
	char *path,
	long path_size);

/* TRUE when Custom Edition maps may run (game.custom_edition) and the map
`map_name` names is a Custom Edition cache whose resource maps are present:
it is then read in place, never copied to the cache partition. */
boolean custom_edition_cache_playable(
	char const *map_name);

/* Loads the Custom Edition map `map_name` names into its tag cache and
converts its tags for this build, copying its cache header to `header`
(CACHE_FILE_HEADER_BYTES bytes); returns its tag index header, or NULL after
logging why the map cannot be loaded. */
struct cache_file_tag_header *custom_edition_cache_tags_load(
	char const *map_name,
	void *header);

boolean custom_edition_cache_tags_loaded(
	void);
void custom_edition_cache_tags_unload(
	void);

/* Reads `size` bytes of the loaded map at `offset`, which counts in the
combined space of the map, its bitmaps.map and its sounds.map
(custom_edition_cache_combine_resource_offsets), for the tag `tag_index` as
cache_file_read names it (NONE, or a tag handle, or for sounds whatever the
permutation holds); bitmap pixels are converted as they arrive. */
void custom_edition_cache_read(
	long tag_index,
	long offset,
	long size,
	void *buffer);

/* ---------- prototypes/CUSTOM_EDITION_BITMAPS.C */

/* Whether this build can draw every bitmap of a tag cache
custom_edition_cache_load filled (`loaded_bytes` of it in use) from
Custom Edition pixels; logs the first it cannot. */
boolean custom_edition_bitmaps_verify(
	byte *tag_cache,
	unsigned long loaded_bytes);

/* Finds the bitmaps of a tag cache custom_edition_cache_load filled whose
channels Halo PC keeps elsewhere (multipurpose maps and HUD meters), which
the renderer is to sample in this build's order as their pixels arrive;
FALSE after logging why it cannot. custom_edition_bitmaps_dispose forgets
them either way. */
boolean custom_edition_reordered_bitmaps_find(
	byte *tag_cache,
	unsigned long loaded_bytes);
void custom_edition_bitmaps_dispose(
	void);

/* Called when `pixels` were read from `offset` for the tag `tag_index`
names: when those are the pixels of one of its bitmaps, lays them out as
the texture cache expects of an Xbox bitmap. */
void custom_edition_bitmap_pixels_arrived(
	byte *tag_cache,
	unsigned long loaded_bytes,
	long tag_index,
	long offset,
	void *pixels);

/* ---------- prototypes/CUSTOM_EDITION_SCRIPTS.C */

/* Gives every function call and engine global reference of the scripts of
a tag cache custom_edition_cache_load filled this build's index for the
name the script keeps; FALSE after logging why when a script uses one this
build does not have or does not keep its name. */
boolean custom_edition_scripts_convert(
	byte *tag_cache,
	unsigned long loaded_bytes);

/* ---------- prototypes/CUSTOM_EDITION_OBJECTS.C */

/* Whether a Custom Edition map is running a multiplayer game whose vehicles
are chosen by their placements' spawn flags (also declared for the game in
halo_linux_source_fixups.h). */
boolean custom_edition_vehicles_by_placement(
	void);
/* Whether the vehicle placement `placement` is placed at the start of the
running game. */
boolean custom_edition_vehicle_placement_allowed(
	struct scenario_object_datum const *placement);

/* ---------- prototypes/CUSTOM_EDITION_GEOMETRY.C */

/* Gives every model of a tag cache custom_edition_cache_load filled
(`loaded_bytes` of it in use) this build's layout and compressed geometry in
buffers of its own, made from `model_data`, the model data the report
describes. Returns FALSE after logging why when a model cannot be converted:
the tags are then partly converted and must not be used.
custom_edition_models_dispose releases the buffers either way. */
boolean custom_edition_models_convert(
	byte *tag_cache,
	unsigned long loaded_bytes,
	struct custom_edition_load_report const *report,
	byte const *model_data);
void custom_edition_models_dispose(
	void);

/* Called by scenario_structure_bsp_load and scenario_structure_bsp_unload
for the structure BSPs of a Custom Edition map: gives its materials
compressed vertices and buffers, or releases them. The load returns FALSE
after logging why when that fails. */
boolean custom_edition_structure_bsp_load(
	struct structure_bsp *structure_bsp);
void custom_edition_structure_bsp_unload(
	void);

#endif
