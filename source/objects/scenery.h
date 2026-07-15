/*
SCENERY.H

header included in hcex build.
*/

#ifndef __SCENERY_H
#define __SCENERY_H
#pragma once

#include "objects/object_types.h"
#include "objects/objects.h"

/* ---------- constants */

/* ---------- macros */

#define scenery_get(index) ((struct scenery_datum *)object_get_and_verify_type((index), _object_mask_scenery))
#define scenery_try_and_get(index) ((struct scenery_datum *)object_try_and_get_and_verify_type((index), _object_mask_scenery))

/* ---------- structures */

struct scenery_datum
{
	struct object_datum object;
	unsigned long flags;
};

typedef char verify_scenery_extension_offset[
	sizeof(struct object_datum) == 0x1A4 ? 1 : -1];

/* ---------- prototypes/EXAMPLE.C */

void scenery_initialize(void);
void scenery_initialize_for_new_map(void);
void scenery_dispose_from_old_map(void);
void scenery_dispose(void);
void scenery_delete(long object_index);
boolean scenery_update(long object_index);
short scenery_get_animation_time(long object_index);

/* ---------- globals */

/* ---------- public code */

#endif // __SCENERY_H
