/*
ANTENNA.H

header included in hcex build.
*/

#ifndef __ANTENNA_H
#define __ANTENNA_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/ANTENNA.C */

void antennas_initialize(
	void);
void antennas_initialize_for_new_map(
	void);
void antennas_dispose_from_old_map(
	void);
void antennas_dispose(
	void);

/* ---------- globals */

extern struct data_array *antenna_data;

/* ---------- public code */

#endif // __ANTENNA_H
