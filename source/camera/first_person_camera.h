/*
FIRST_PERSON_CAMERA.H

header included in hcex build.
*/

#ifndef __FIRST_PERSON_CAMERA_H
#define __FIRST_PERSON_CAMERA_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct first_person_camera
{
	real field_of_view;
};

/* ---------- prototypes/FIRST_PERSON_CAMERA.C */

void first_person_camera_new(
	struct first_person_camera *camera);

/* ---------- globals */

/* ---------- public code */

#endif // __FIRST_PERSON_CAMERA_H
