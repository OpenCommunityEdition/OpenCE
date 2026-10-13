/*
PROGRESS_BAR.H

header included in hcex build.
*/

#ifndef __PROGRESS_BAR_H
#define __PROGRESS_BAR_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/PROGRESS_BAR.C */

void progress_bar_initialize(
	void);
void progress_bar_dispose(
	void);
void progress_bar_begin(
	boolean skip_frame_capture);
void progress_bar_end(
	void);
boolean progress_bar_is_active(
	void);
void progress_bar_enable(
	boolean enabled);
void progress_bar_eachframe(
	void);
void progress_bar_display(
	real progress);
boolean progress_bar_is_stuff_ready(
	void);
/* port: the screen of a map's download over the menus
(port/linux/game/download_screen.c) */
void progress_bar_download_display(
	real progress);
void progress_bar_download_end(
	void);

/* ---------- globals */

/* ---------- public code */

#endif // __PROGRESS_BAR_H
