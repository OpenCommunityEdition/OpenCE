/*
MCC_IMPORT.H

The MCC import (mcc_import.c) and the changes it makes to an MCC map's
tags before it writes them as the disc's (mcc_import_patch.c).
*/

#ifndef __MCC_IMPORT_H
#define __MCC_IMPORT_H
#pragma once

/* ---------- structures */

/* a tag of the map being imported, as a cache's tag table lists it */
struct mcc_import_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

typedef char mcc_import_tag_instance_size[sizeof(struct mcc_import_tag_instance) == 0x20 ? 1 : -1];

/* the loaded tags (a copy of the loaded table, with room for tags the
operations add) */
struct mcc_import_tags
{
	struct mcc_import_tag_instance *instances;
	long count;
	long maximum_count;
	long scenario_index;
};

/* a structure BSP as the import wrote it: its bytes, loaded at address */
struct mcc_fingerprint_bsp
{
	unsigned long address;
	unsigned char *bytes;
	unsigned long size;
};

/* ---------- prototypes/MCC_IMPORT.C */

int mcc_import_run(
	char const *output_folder,
	char const *const *map_names,
	int map_count);

/* ---------- prototypes/MCC_IMPORT_PATCH.C */

/* The importer's own fixes of what MCC changed (kind A: values Bungie's
tools computed, Halo PC's flag bits, MCC's multiplayer scripts), on every
tag. Logs what it changed. */
void mcc_import_fixes_apply(
	struct mcc_import_tags *tags);

/* Reads the map's operations (d:\mcc_patches\<map>.p2), which give MCC's
tags the disc's values where they differ: FALSE if the map has none. */
boolean mcc_import_operations_read(
	char const *map_name);

/* Applies the operations to the tags (bsp_name NULL), or to the structure
BSP named bsp_name ("sbsp:levels\..."), whose root is bsp_root. A `tag`
operation adds a tag to the table. Returns the number that failed. */
long mcc_import_operations_apply(
	struct mcc_import_tags *tags,
	char const *bsp_name,
	void *bsp_root);

/* Gives each model whose permutations name more levels of detail than it
has geometries its last geometry again in their place. */
void mcc_import_models_complete(
	struct mcc_import_tags *tags);

/* The disc's size of the HUD bitmap `name` (its first image: the
operations' bitmap_size lines), to which the import halves MCC's: FALSE when
the operations do not give it. */
boolean mcc_import_operations_bitmap_size(
	char const *name,
	short *width,
	short *height);

/* The import's file `file_name` ("<map>.p2", the disc's values as operations,
or "<map>.fp", the fingerprint of the disc's map): the data root's
mcc_patches folder's, or the game's (port/assets/mcc), NUL-terminated, its
size in *size; NULL when neither has it. Released with mcc_import_data_free. */
char *mcc_import_data_read(
	char const *file_name,
	long *size);

void mcc_import_data_free(
	char *text);

/* The maps the game has the disc's values of (port/assets/mcc), at most
maximum_count of them: the maps an import writes when none is named. */
int mcc_import_default_maps(
	char names[][64],
	int maximum_count);

/* Writes the tag `tag` ("group:path") of the MCC map `donor_map`, as its
fixes and operations leave it, for the maps whose donor lines name it. */
boolean mcc_import_operations_export(
	struct mcc_import_tags const *tags,
	char const *donor_map,
	char const *tag);

/* The donor lines of the operations read whose tags are not written yet:
each one's map (donor_maps) and tag (tags), at most maximum_count. */
long mcc_import_operations_donors_missing(
	char const **donor_maps,
	char const **tags,
	long maximum_count);

/* The disc's sizes of the HUD sprite sheet `name`'s sprites (the first of each
sequence, "<width>x<height>" separated by commas: the operations' sprite_sizes
line), or NULL. */
char const *mcc_import_operations_sprite_sizes(
	char const *name);

/* Whether the operations give the disc's HUD placements of the HUD interface
`tag` ("group:path"), or of any when NULL (its hud_values line): their scales
then stand for bitmaps of the disc's size. */
boolean mcc_import_operations_hud_values(
	char const *tag);

/* Releases the operations and what they made. */
void mcc_import_operations_dispose(
	void);

/* ---------- prototypes/MCC_IMPORT_SOUNDS.C */

/* writes `size` bytes of samples into the map's body: their offset in the
file */
typedef unsigned long (*mcc_import_sound_write_proc)(void *context, void const *bytes, unsigned long size);

/* Reads MCC's sound banks (the data root's sound\pc): FALSE when they
cannot be, and the maps' sounds are then silent. */
boolean mcc_import_sounds_open(
	void);
void mcc_import_sounds_close(
	void);
/* (mcc_sounds_muted_proc) a sound MCC's load muted, and its pitch ranges */
void mcc_import_sounds_muted(
	unsigned char *sound,
	long pitch_range_count);
/* Writes the samples of every muted sound of the tags written
(old_to_new: NONE for a tag left out), from the banks, and gives the
sounds the disc's shape. Returns how many; mcc_import_sounds_silent how
many stay silent. */
long mcc_import_sounds_write(
	struct mcc_import_tags *tags,
	long const *old_to_new,
	mcc_import_sound_write_proc write,
	void *context);
long mcc_import_sounds_silent(
	void);
/* Lets go of what the map's sounds needed, once the map is written. */
void mcc_import_sounds_map_done(
	void);

/* ---------- prototypes/MCC_IMPORT_FINGERPRINT.C */

/* The fingerprint of the map whose tags are at tag_data (as an Xbox map's,
at 0x803A6000), the disc's being its first disc_tag_count tags, and whose
structure BSPs are bsps (in the scenario's order): its lines, one per part,
the last the whole map's ("MAP\t<scenario>\t<hash>"). Valid until the next
call. */
char const *mcc_import_fingerprint(
	void *tag_data,
	unsigned long tag_data_size,
	long disc_tag_count,
	struct mcc_fingerprint_bsp const *bsps,
	long bsp_count);

#endif // __MCC_IMPORT_H
