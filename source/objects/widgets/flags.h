/*
FLAGS.H

header included in hcex build.
*/

#ifndef __FLAGS_H
#define __FLAGS_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

void flags_initialize(
	void);
void flags_initialize_for_new_map(
	void);
void flags_dispose_from_old_map(
	void);
void flags_dispose(
	void);

/* ---------- globals */

extern struct data_array *flag_data;

/* ---------- public code */

#endif // __FLAGS_H
