/*
GARBAGE.H

header included in hcex build.
*/

#ifndef __GARBAGE_H
#define __GARBAGE_H
#pragma once

#include "items/items.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct garbage_datum
{
	long definition_index;
	struct _object_datum object;
	struct _item_datum item;
	short lifetime_ticks;
};

/* ---------- prototypes/GARBAGE.C */

boolean garbage_update(long garbage_index);
boolean garbage_new(long garbage_index);

/* ---------- globals */

/* ---------- public code */

#endif // __GARBAGE_H
