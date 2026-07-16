/*
CINEMATICS.H

header included in hcex build.
*/

#ifndef __CINEMATICS_H
#define __CINEMATICS_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/CINEMATICS.C */

void cinematic_initialize(
	void);
void cinematic_dispose(
	void);
void cinematic_initialize_for_new_map(
	void);
void cinematic_skip_start(
	void);
void cinematic_skip_stop(
	void);
void cinematic_dispose_from_old_map(
	void);
void cinematic_start(
	void);
boolean cinematic_can_be_skipped(
	void);
void cinematic_show_letterbox(
	boolean show);
void cinematic_force_title(
	unsigned short title_index);
void cinematic_suppress_bsp_object_creation(
	boolean suppress);
boolean cinematic_in_progress(
	void);
void cinematic_stop(
	void);
void cinematic_set_title(
	unsigned short title_index);

/* ---------- globals */

/* ---------- public code */

#endif // __CINEMATICS_H
