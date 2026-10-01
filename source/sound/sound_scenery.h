/*
SOUND_SCENERY.H

header included in hcex build.
*/

#ifndef __SOUND_SCENERY_H
#define __SOUND_SCENERY_H
#pragma once

#include "objects/object_types.h"
#include "objects/objects.h"

/* ---------- constants */

/* ---------- macros */

#define sound_scenery_get(index) ((struct object_datum *)object_get_and_verify_type((index), _object_mask_sound_scenery))
#define sound_scenery_try_and_get(index) ((struct object_datum *)object_try_and_get_and_verify_type((index), _object_mask_sound_scenery))

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

boolean sound_scenery_new(
	long object_index);
void sound_scenery_delete(
	long object_index);

/* ---------- globals */

/* ---------- public code */

#endif // __SOUND_SCENERY_H
