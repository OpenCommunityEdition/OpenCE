/*
SCENERY.H

header included in hcex build.
*/

#ifndef __SCENERY_H
#define __SCENERY_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

void scenery_initialize(void);
void scenery_initialize_for_new_map(void);
void scenery_dispose_from_old_map(void);
void scenery_dispose(void);
void scenery_delete(long object_index);

/* ---------- globals */

/* ---------- public code */

#endif // __SCENERY_H
