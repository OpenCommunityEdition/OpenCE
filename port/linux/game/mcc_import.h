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

/* Releases the operations and what they made. */
void mcc_import_operations_dispose(
	void);

#endif // __MCC_IMPORT_H
