/*
SOUND_DSOUND.H

file has inline function assertions.
*/

#ifndef __SOUND_DSOUND_H
#define __SOUND_DSOUND_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct IDirectSound;
struct platform_sound_channel_properties;
struct platform_sound_listener_properties;
struct sound_location;
struct sound_permutation;
struct sound_platform_definition;
struct sound_preferences;

/* ---------- prototypes/SOUND_DSOUND_XBOX.C */

unsigned long sound_samples_per_second(
	short sample_rate);
long dsound_frequency_from_pitch(
	long samples_per_second,
	real pitch);
long dsound_angle_from_angle(
	real angle);
long dsound_occlusion_from_occlusion(
	real occlusion);
long dsound_obstruction_from_obstruction(
	real obstruction);
struct IDirectSound *dsound_get(
	void);
/* the next sound queued on an idle channel starts this far into its samples
(a block boundary; 0 starts at the beginning, which is the usual queue) */
void dsound_queue_from_offset(
	long byte_offset);

/* ---------- globals */

extern struct sound_platform_definition platform_sound_dsound;

/* ---------- public code */

#endif // __SOUND_DSOUND_H
