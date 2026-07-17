/*
DRAW_STRING.H

header included in hcex build.
*/

#ifndef __DRAW_STRING_H
#define __DRAW_STRING_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/DRAW_STRING.C */

void draw_string_initialize(
	void);
void draw_string_initialize_for_new_map(
	void);
void draw_string_dispose_from_old_map(
	void);
void draw_string_dispose(
	void);

char *draw_string_get_string(
	short index);
void draw_string_set_tab_stops(
	short const *tab_stops,
	short count);
void draw_string_set_indents(
	short initial_indent,
	short paragraph_indent);
void draw_string_set_color(
	real_argb_color const *color);
void draw_string_get_color(
	real_argb_color *color);
void draw_string_set_font(
	long font_index);
void draw_string_set_format(
	short style,
	short justification,
	unsigned long flags);
void draw_string_set_draw_mode(
	long font_index,
	short style,
	short justification,
	unsigned long flags,
	real_argb_color const *color);
void draw_string_set_highlight(
	short start,
	short end);

/* ---------- globals */

/* ---------- public code */

#endif // __DRAW_STRING_H
