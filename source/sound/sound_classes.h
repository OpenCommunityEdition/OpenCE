/*
SOUND_CLASSES.H

file has inline function assertions.
*/

#ifndef __SOUND_CLASSES_H
#define __SOUND_CLASSES_H
#pragma once

#include "real_math.h"
#include "objects.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct sound_class_definition;

/* ---------- prototypes/SOUND_CLASSES.C */

struct sound_class_definition *sound_class_get(
	short class_index);

real sound_class_get_gain(
	short class_index);

void sound_classes_dispose_from_old_map(
	void);
void sound_classes_dispose(
	void);
void sound_classes_update(
	long ticks);

/* ---------- globals */

/* ---------- public code */

#endif // __SOUND_CLASSES_H
