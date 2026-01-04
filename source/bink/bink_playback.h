/*
BINK_PLAYBACK.H

header included in hcex build.
*/

#ifndef __BINK_PLAYBACK_H
#define __BINK_PLAYBACK_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/BINK_PLAYBACK.C */

boolean bink_playback_active(void);
boolean bink_playback_ui_rendering_inhibited(void);
boolean bink_playback_in_progress(void);
void bink_playback_initialize(void);
void bink_playback_stop(void);
void bink_playback_dispose(void);
void bink_playback_render(void);
void bink_playback_start(const char *full_pathname, unsigned long flags);
void bink_playback_update(void);

/* ---------- globals */

/* ---------- public code */

#endif // __BINK_PLAYBACK_H
