/*
CUSTOM_EDITION_CACHE.H

What the native builds' cache file loader (source/cache/cache_files.c and
cache_files_windows.c) does with Halo Custom Edition caches (OpenSauce ones
are refused). By default it finds them, says what they are, and refuses
them. With the game.custom_edition setting on it loads and runs them
instead, which is experimental: their tags are laid out for Halo PC, and
custom_edition_cache.c and custom_edition_geometry.c convert what they know
to differ (docs/custom_edition_caches.md). The format itself is read by
cache_file_formats.c.
*/

#ifndef __CUSTOM_EDITION_CACHE_H
#define __CUSTOM_EDITION_CACHE_H

/* ---------- macros */

/* DamnationCE's 64-bit builds keep Xbox addresses apart from pointers; in
this build's they are the same */
#ifndef XBOX_POINTER
#define xbox_pointer(address) ((void *)(address))
#define XBOX_POINTER(type, address) ((type *)(address))
#define XBOX_ADDRESS(pointer) ((void *)(pointer))
#endif

/* ---------- constants */

/* Custom Edition maps and their resource maps (bitmaps.map, sounds.map,
loc.map) are kept apart from the game's own maps: in the data root's
custom_maps folder, then a Halo Custom Edition install's maps folder (the
platform's h:\ drive, when paths.custom_edition names one). */
#define CUSTOM_EDITION_MAP_DIRECTORY "d:\\custom_maps\\"
#define CUSTOM_EDITION_INSTALL_MAP_DIRECTORY "h:\\maps\\"

/* A Custom Edition map's level name is this and its file's name
(custom_maps\a30 for custom_maps\a30.map), which the game's own levels'
never are: a map named as one of them (a30, bloodgulch) stays apart from it,
in the menus, the loader and network games. The game engine keeps 63
characters of a level name (game_engine.c, struct game_engine_stage). */
#define CUSTOM_EDITION_LEVEL_NAME_PREFIX "custom_maps\\"
#define CUSTOM_EDITION_MAXIMUM_LEVEL_NAME_LENGTH 63

/* ---------- structures */

struct cache_file_tag_header;
struct custom_edition_load_report;
struct scenario_object_datum;
struct structure_bsp;

/* ---------- prototypes/CUSTOM_EDITION_CACHE.C */

/* Whether a tag of the Custom Edition map loaded is one of Halo PC's own, as
it shipped: read from bitmaps.map, sounds.map or loc.map. FALSE when no
Custom Edition map is loaded. */
boolean custom_edition_cache_stock_tag(
	long tag_index);

/* When `header` (CACHE_FILE_HEADER_BYTES bytes, with `build` its build
string field) is a Custom Edition cache header that reached the Xbox loader,
logs why the file at `path` cannot run and returns TRUE: the map is refused,
not the game stopped. */
boolean custom_edition_cache_refuse(
	void const *header,
	char const *build,
	char const *path);

/* Whether a level name is a Custom Edition map's
(CUSTOM_EDITION_LEVEL_NAME_PREFIX): the cache file loader then reads it from
the Custom Edition maps folders alone, never from the game's own. */
boolean custom_edition_level_name(
	char const *level_name);

/* TRUE when Custom Edition maps may run (game.custom_edition) and the level
`level_name` is a Custom Edition map's whose file is a Custom Edition cache:
it is then read in place, never copied to the cache partition. */
boolean custom_edition_cache_playable(
	char const *level_name);
/* Whether the map `map_name` (a level name or a file name) has a file in the
Custom Edition maps folders, whatever it holds. */
boolean custom_edition_map_file_present(
	char const *map_name);
/* The checksum in the header of the level's map file, which differs between
versions of a map; 0 if it has no file or the file is not a Custom Edition
cache. */
unsigned long custom_edition_map_checksum(
	char const *level_name);
/* Whether this machine can play the level `level_name` (custom_maps\<name>),
as a network game's client must: its file present and a Custom Edition
cache of the host's version (`checksum`, custom_edition_map_checksum's on
the host; 0 for any), Custom Edition maps able to run, and the resource maps
present. When not, `message` (`message_size` characters) says why, for the
player, and what to do. */
boolean custom_edition_cache_present(
	char const *level_name,
	unsigned long checksum,
	char *message,
	long message_size);

/* Whether the file `file_name` (custom_maps\<file_name>.map) is a Custom
Edition cache of a campaign map (a solo scenario, played alone or as
network co-op), or of a multiplayer map (custom_edition_maps.c). */
boolean custom_edition_cache_campaign(
	char const *file_name);
boolean custom_edition_cache_multiplayer(
	char const *file_name);

/* Loads the Custom Edition map `map_name` names into its tag cache and
converts its tags for this build, copying its cache header to `header`
(CACHE_FILE_HEADER_BYTES bytes); returns its tag index header, or NULL after
logging why the map cannot be loaded. */
struct cache_file_tag_header *custom_edition_cache_tags_load(
	char const *map_name,
	void *header);

/* whether a Halo Custom Edition install's maps folder is to be looked in
(paths.custom_edition) */
boolean custom_edition_install_present(
	void);
boolean custom_edition_cache_tags_loaded(
	void);
/* Whether the structure bsp reference (of the loaded map's scenario) names
bytes of the map that load to the top of its tag cache, above its tags:
logged when not. */
struct scenario_structure_bsp_reference;
boolean custom_edition_structure_bsp_reference_valid(
	struct scenario_structure_bsp_reference const *reference);
/* Whether the Custom Edition map loaded relies on a Halo PC behaviour (enum
custom_edition_behaviour, cache_file_formats.h); FALSE when none is loaded. */
boolean custom_edition_cache_relies_on(
	short behaviour);
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
/* Decodes every sound permutation of a tag cache custom_edition_cache_load
filled (its resource offsets combined) that this build would not play: Ogg
Vorbis, uncompressed, or mono at 44 kHz. Each is encoded again in memory as
Xbox ADPCM in a format this build plays, its samples at decoded_offset and
on in the combined offset space, where custom_edition_sounds_read serves
them, up to decoded_limit bytes of them. A sound that cannot be converted,
or would go past the limit, is silenced. custom_edition_sounds_dispose lets
the samples go. */
boolean custom_edition_sounds_decode(
	byte *tag_cache,
	unsigned long loaded_bytes,
	long decoded_offset,
	unsigned long decoded_limit);
/* the bytes of the samples custom_edition_sounds_decode made */
unsigned long custom_edition_sounds_decoded_bytes(
	void);
boolean custom_edition_sounds_read(
	long offset,
	long size,
	void *buffer);
void custom_edition_sounds_dispose(
	void);

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

/* The nodes a model part's vertices name, by its vertex buffer, when its
model has more nodes than the renderer skins at once
(rasterizer_model_part_skinning): their number, or 0 for any other part. */
struct vertex_buffer;
short custom_edition_part_palette(
	struct vertex_buffer const *vertex_buffer,
	byte const **nodes);

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
	byte *model_data);
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
