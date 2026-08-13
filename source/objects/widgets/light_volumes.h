/*
LIGHT_VOLUMES.H

header included in hcex build.
*/

#ifndef __LIGHT_VOLUMES_H
#define __LIGHT_VOLUMES_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "memory/data.h"
#include "objects/widgets/widget_types.h"

/* ---------- constants */

/* ---------- macros */

#define light_volume_get(light_volume_index) \
	((struct light_volume_datum *)datum_get(bss_00456d90.light_volumes, (light_volume_index)))

/* ---------- structures */

struct light_volume_datum
{
	struct datum_header header;
	short __unknown2;
	long definition_index;
};

struct light_volume_globals
{
	byte __unknown0[0xB0];
	struct data_array *light_volumes;
};

typedef char light_volume_datum_size_assert[
	sizeof(struct light_volume_datum) == 0x8 ? 1 : -1];
typedef char light_volume_globals_size_assert[
	sizeof(struct light_volume_globals) == 0xB4 ? 1 : -1];

/* ---------- prototypes/EXAMPLE.C */

void light_volumes_initialize(
	void);
void light_volumes_dispose(
	void);
void light_volumes_initialize_for_new_map(
	void);
void light_volumes_dispose_from_old_map(
	void);

long light_volume_new(
	long definition_index);

void light_volume_delete(
	long light_volume_index);
void light_volume_submit(
	long object_index,
	long light_volume_index,
	struct render_lighting const *lighting,
	struct render_animation const *animation);

/* ---------- globals */

extern struct light_volume_globals bss_00456d90;

/* ---------- public code */

#endif // __LIGHT_VOLUMES_H
