/*
VIRTUAL_KEYBOARD.H

header included in hcex build.
*/

#ifndef __VIRTUAL_KEYBOARD_H
#define __VIRTUAL_KEYBOARD_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

boolean virtual_keyboard_initialize(
	void);
void virtual_keyboard_dispose(
	void);
boolean virtual_keyboard_launch(
	wchar_t *text_buffer,
	word buffer_size,
	short caption_index);
boolean virtual_keyboard_active(
	void);
void virtual_keyboard_close(
	void);
boolean virtual_keyboard_last_exit_saved_text(
	void);
void virtual_keyboard_process(
	void);
void virtual_keyboard_render(
	void);

/* applies a click or tap to the keyboard; a key takes the focus and is
pressed as A presses the focused key; BACK cancels as B does; ENTER goes to
Done and presses it as Start does, not as A does (a touch has no focused key
to confirm with); keys that span several cells take the focus at their first;
hit (may be NULL) receives which rectangle matched as its index in
virtual_keyboard_target_rectangles (the keys, then BACK, then ENTER), or
NONE; the debug log relies on this order; returns TRUE if the click was on a
key or the BACK or ENTER legend, which then acted */
boolean virtual_keyboard_click(
	short x,
	short y,
	long *hit);

/* lists the rectangles that virtual_keyboard_click hit-tests, for the debug
view of the touch targets (debug.touch_targets); rectangles are the keys',
then BACK and ENTER legends', in that order */
long virtual_keyboard_target_rectangles(
	rectangle2d *rectangles,
	long maximum);

/* ---------- globals */

/* ---------- public code */

#endif // __VIRTUAL_KEYBOARD_H
