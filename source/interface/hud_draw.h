/*
HUD_DRAW.H

Narrow cross-translation-unit interface owned by HUD_DRAW.C.
*/

#ifndef __HUD_DRAW_H
#define __HUD_DRAW_H
#pragma once

#include "math/integer_math.h"
#include "math/real_math.h"

enum hud_draw_flags
{
	_hud_draw_flashing_bit = 0,
	_hud_draw_disabled_bit,
	_hud_draw_in_multiplayer_bit,

	NUMBER_OF_HUD_DRAW_FLAGS
};

struct bitmap_data;
struct hud_absolute_placement_definition;
struct hud_color_definition;
struct hud_placement_definition;
struct meter_hud_element_definition;
struct number_hud_element_definition;
struct static_hud_element_definition;

long get_return_eip(
	void);
real hud_globals_get_scale(
	boolean in_multiplayer);
void hud_retrieve_bitmap_and_bounding_rect(
	long bitmap_group_index,
	short sequence_index,
	short frame_index,
	struct bitmap_data **bitmap,
	real_rectangle2d const **clip);
long fast_ftol_C(
	real value);
pixel32 real_argb_color_to_pixel32(
	real_argb_color const *color);
pixel32 real_alpha_intensity_to_pixel32(
	real alpha,
	real intensity);
pixel32 real_rgb_color_to_pixel32(
	real_rgb_color const *color);
void hud_calculate_point(
	short local_player_index,
	struct hud_absolute_placement_definition const *absolute_placement,
	struct hud_placement_definition const *placement,
	struct bitmap_data const *bitmap_data,
	boolean in_multiplayer,
	real override_scale,
	point2d *result);
long get_flash_duration(
	struct hud_color_definition const *hud_color);
void hud_draw_meter(
	short local_player_index,
	struct hud_absolute_placement_definition const *placement,
	struct meter_hud_element_definition const *meter,
	byte min_value,
	byte max_value,
	short draw_flags,
	real reference_time,
	real reference_value);
void hud_draw_static_element(
	short local_player_index,
	struct hud_absolute_placement_definition const *placement,
	struct static_hud_element_definition const *static_element,
	short draw_flags,
	long flash_reference_time);
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
