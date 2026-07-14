/*
GARBAGE.C

symbols in this file:
000E6010 0040:
	_garbage_update (0000)
000E6050 0050:
	_garbage_new (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "garbage.h"

#include "items/items.h"
#include "math/real_math.h"
#include "objects/object_types.h"
#include "objects/objects.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

boolean garbage_update(
	long garbage_index)
{
	struct garbage_datum *garbage = (struct garbage_datum *)object_get_and_verify_type(garbage_index, _object_mask_garbage);
	boolean active;

	--garbage->lifetime_ticks;
	active = garbage->lifetime_ticks > 0;

	if (!active)
		object_delete(garbage_index);

	return active;
}

/* ---------- private code */
