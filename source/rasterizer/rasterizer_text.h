/*
RASTERIZER_TEXT.H
*/

#ifndef __RASTERIZER_TEXT_H
#define __RASTERIZER_TEXT_H
#pragma once

/* ---------- headers */

#include "cseries.h"

/* ---------- structures */

struct bitmap_data;
struct dynamic_screen_vertex;
struct font_character;
struct font_header;
struct parse_string_state;
struct rasterizer_dynamic_screen_geometry_parameters;

/* ---------- prototypes/RASTERIZER_TEXT.C */

void lock_rasterizer_text_data(
	void);
void unlock_rasterizer_text_data(
	void);
boolean rasterizer_text_cache_initialize(
	void);
void rasterizer_text_draw_character(
	struct dynamic_screen_vertex const *vertices);
void rasterizer_text_begin(
	struct rasterizer_dynamic_screen_geometry_parameters const *parameters);
void rasterizer_text_end(
	void);
/* port: draws the characters of the string so far (rasterizer_xbox_text.c) */
void rasterizer_text_flush(
	void);

#endif // __RASTERIZER_TEXT_H
