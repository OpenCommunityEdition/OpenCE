/*
TALKING_TIMER_CLIPS.H
*/

#ifndef TALKING_TIMER_CLIPS_H
#define TALKING_TIMER_CLIPS_H

#include "talking_timer_audio.h"

struct talking_timer_clip
{
	const unsigned int *words;
	unsigned long size;
};

extern const struct talking_timer_clip talking_timer_clips[NUMBER_OF_TALKING_TIMER_LINES];

#endif
