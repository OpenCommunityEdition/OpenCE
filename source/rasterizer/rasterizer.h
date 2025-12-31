/*
RASTERIZER.H

header included in hcex build.
*/

#ifndef __RASTERIZER_H
#define __RASTERIZER_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/RASTERIZER.C */

boolean rasterizer_initialize(void);
void rasterizer_dispose(void);

void rasterizer_decals_update_function_pointers(void);

/* ---------- globals */

/* ---------- public code */

#endif // __RASTERIZER_H
