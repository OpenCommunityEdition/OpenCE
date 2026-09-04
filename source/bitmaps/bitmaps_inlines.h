/*
BITMAPS_INLINES.H

file has inline function assertions.
*/

#ifndef __BITMAPS_INLINES_H
#define __BITMAPS_INLINES_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/integer_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

/* January retains an out-of-line copy of this inline in
 * rasterizer_xbox_active_camouflage.obj. Its assertion records this header
 * and line 291. The stack-local scale, 32-bit FISTP, and in-memory shift are
 * the characteristic packet of the original small x87 helper. */
__inline pixel32 real_alpha_to_pixel32(
	real alpha)
{
	real scale = 255.0f;
	pixel32 result;

	match_assert(
		"..\\bitmaps\\bitmaps_inlines.h",
		291,
		alpha>=0.0f && alpha<=1.0f);

	__asm
	{
		fld		alpha
		fld		scale
		fmulp	st(1), st
		fistp	result
		shl		result, 24
	}

	return result;
}

#endif // __BITMAPS_INLINES_H
