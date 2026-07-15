/*
BORED_CAMERA.H

header included in hcex build.
*/

#ifndef __BORED_CAMERA_H
#define __BORED_CAMERA_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct bored_camera
{
	unsigned long last_update_milliseconds;
	long timer_milliseconds;
	long boredom_count;
};

/* ---------- prototypes/BORED_CAMERA.C */

void bored_camera_new(
	struct bored_camera *camera);

/* ---------- globals */

/* ---------- public code */

#endif // __BORED_CAMERA_H
