/*
CAMERA_SCRIPTING.H

header included in hcex build.
*/

#ifndef __CAMERA_SCRIPTING_H
#define __CAMERA_SCRIPTING_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/CAMERA_SCRIPTING.C */

struct camera_control;
struct dead_camera;
struct scripted_camera_command;

void scripted_camera_enable(
	boolean enabled);
void scripted_camera_update(
	struct dead_camera *camera,
	struct camera_control const *controls,
	struct scripted_camera_command *result);

/* ---------- globals */

/* ---------- public code */

#endif // __CAMERA_SCRIPTING_H
