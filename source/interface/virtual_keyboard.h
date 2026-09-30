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

/**
 * @brief Applies a click or tap, in the menus' 640x480 coordinates that the
 * keyboard draws in, to the keyboard.
 *
 * A key takes the focus and is pressed as A presses the focused key. BACK
 * cancels as B does. ENTER goes to Done and presses it as Start does, not as
 * A does: a touch has no focused key to confirm with. Keys that span several
 * cells take the focus at their first.
 *
 * @param x horizontal position of the click
 * @param y vertical position of the click
 * @return TRUE if the click was on a key or on the BACK or ENTER legend,
 * which then acted; FALSE otherwise
 */
boolean virtual_keyboard_click(
	short x,
	short y);

/* ---------- globals */

/* ---------- public code */

#endif // __VIRTUAL_KEYBOARD_H
