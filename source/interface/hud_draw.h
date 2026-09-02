/*
HUD_DRAW.H

Narrow cross-translation-unit interface owned by HUD_DRAW.C.
*/

#ifndef __HUD_DRAW_H
#define __HUD_DRAW_H
#pragma once

#include "math/integer_math.h"
#include "math/real_math.h"

struct bitmap_data;
struct hud_absolute_placement_definition;
struct number_hud_element_definition;

void hud_retrieve_bitmap_and_bounding_rect(
	long bitmap_group_index,
	short sequence_index,
	short frame_index,
	struct bitmap_data **bitmap,
	real_rectangle2d const **clip);
long fast_ftol_C(
	real value);
pixel32 real_rgb_color_to_pixel32(
	real_rgb_color const *color);
void hud_draw_bitmap_direct(
	struct bitmap_data const *bitmap,
	short placement,
	point2d const *point,
	real_rectangle2d const *clip,
	real scale,
	real theta,
	pixel32 color,
	boolean is_interface_bitmap);
void hud_draw_numbers(
	short local_player_index,
	struct hud_absolute_placement_definition const *absolute_placement,
	struct number_hud_element_definition const *numbers,
	short value,
	short decimal_value,
	short draw_flags,
	long flash_reference_time,
	real override_scale);

#endif /* __HUD_DRAW_H */
