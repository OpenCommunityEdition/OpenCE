/*
PROFILE_CONSOLE.H

Expose the game recording console hooks.
*/

#ifndef __PROFILE_CONSOLE_H
#define __PROFILE_CONSOLE_H
#pragma once

#ifdef HALO_PROFILE

/* profile_record [seconds], profile_stop: TRUE
when the expression was one of them (hs.c, before the script compiler) */
boolean profile_console_command(char const *expression);
/* the first frame: the recording's seams, and the launch settings */
void profile_console_launch(void);
/* each frame, right after the frame boundary */
void profile_console_frame(void);
/* a map starts loading (game_load) */
void profile_console_map_loaded(char const *map_name);

#endif

#endif
