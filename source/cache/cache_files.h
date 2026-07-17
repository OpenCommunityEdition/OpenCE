/*
CACHE_FILES.H

header included in hcex build.
*/

#ifndef __CACHE_FILES_H
#define __CACHE_FILES_H
#pragma once

/* ---------- headers */

#include "real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/CACHE_FILES.C */

const char *cache_files_map_directory(
	void);

unsigned long cache_files_get_checksum(void);
void scenario_structure_bsp_unload(
	struct scenario_structure_bsp_reference *reference);
void scenario_tags_unload(
	void);
void tag_files_close(
	void);
void tag_files_open(
	void);

/* ---------- prototypes/CACHE_FILES_WINDOWS.C */

boolean cache_files_precache_in_progress(void);

short cache_files_precache_map_status(real *progress);

void cache_files_precache_map_end(void);

void cache_files_precache_map_begin(
	char const *map_name,
	boolean copy_map);

unsigned long tag_get_group_tag(long tag_index);

/* ---------- globals */

/* ---------- public code */

#endif // __CACHE_FILES_H
