/*
SUBTITLES.H

Subtitles for sound tags: the lines of a plain text file (subtitles.txt,
display.subtitles_file), each named by a sound tag's path, shown at the
bottom of the screen while that sound plays (subtitles.c).
*/

#ifndef __SUBTITLES_H
#define __SUBTITLES_H

/* display.subtitles, and the file read */
boolean subtitles_enabled(void);
/* (sound_manager.c's sound_new_impulse) a sound has been started, to be
heard after delay_milliseconds and to last about length_milliseconds: its
subtitle, if the file has one for the tag, is shown for that time */
void subtitles_sound_started(
	long definition_index,
	long delay_milliseconds,
	long length_milliseconds);
/* (interface.c's overlays, over the whole screen) */
void subtitles_draw(void);

#endif
