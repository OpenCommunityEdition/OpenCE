/*
INTERFACE.C

symbols in this file:
000CE180 0020:
	_interface_initialize (0000)
000CE1A0 0010:
	_interface_dispose_from_old_map (0000)
000CE1B0 0020:
	_interface_dispose (0000)
000CE1D0 0080:
	_interface_get_tag_index (0000)
000CE250 0080:
	_interface_get_real_argb_color (0000)
000CE2D0 0050:
	_interface_set_bitmap_text_draw_mode (0000)
000CE320 01c0:
	_code_000ce320 (0000)
000CE4E0 03a0:
	_interface_draw_screen (0000)
000CE880 0080:
	_profile_graph_toggle (0000)
000CE900 0110:
	_code_000ce900 (0000)
000CEA10 08e0:
	_code_000cea10 (0000)
000CF2F0 00e0:
	_code_000cf2f0 (0000)
000CF3D0 0060:
	_interface_initialize_for_new_map (0000)
000CF430 0070:
	_interface_get_rgb_color (0000)
000CF4A0 0020:
	_interface_draw_fullscreen_overlays (0000)
000CF4C0 0180:
	_interface_draw_bitmap (0000)
000CF640 01c0:
	_interface_draw_bitmap_modulated (0000)
000CF800 0160:
	_interface_draw_bitmap_modulated_p32 (0000)
00270988 0047:
	??_C@_0EH@NDHILNFF@interface_tag_index?$DO?$DN0?5?$CG?$CG?5interf@ (0000)
002709D0 0025:
	??_C@_0CF@HPKANGD@c?3?2halo?2SOURCE?2interface?2interfa@ (0000)
002709F8 001f:
	??_C@_0BP@HBELBMEI@drawingbuf_counts?$FLindex?$FN?5?$DM?5512?$AA@ (0000)
00270A18 000d:
	??_C@_0N@KBDODLDN@?$HMn?$HMn?$HMn?$HMn?$HMn?$HMn?$AA@ (0000)
00270A28 0011:
	??_C@_0BB@EMOPPCDP@?$CF?55d?5particles?$HMn?$AA@ (0000)
00270A3C 002e:
	??_C@_0CO@GJNLBLAB@?$CF?55d?5active?5of?5?$CF?55d?5effects?5?$CI?$CF5d@ (0000)
00270A6C 002e:
	??_C@_0CO@ENADKIPF@?$CF?55d?5active?5of?5?$CF?55d?5objects?5?$CI?$CF?53@ (0000)
00270A9C 0020:
	??_C@_0CA@DMKIAGNI@?$CF?56?41fk?5free?5of?5?$CF?56?41fk?5total?$HMn?$AA@ (0000)
00270ABC 0004:
	__real@3a800000 (0000)
00270AC0 0010:
	??_C@_0BA@DAFKNDHA@window_count?$DN?$DN4?$AA@ (0000)
002E4C88 18918:
	_profile_game_value_count (0000)
	_profile_game_values (0008)
	_profile_frame_value_count (8308)
	_profile_frame_values (8310)
	_profile_graph_value_count (10610)
	_profile_graph_values (10618)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "interface/first_person_weapons.h"
#include "interface/interface.h"
#include "interface/hud_messaging.h"
#include "math/real_math.h"
#include "text/draw_string.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

union argb_color
{
	word n[4];
};
typedef union argb_color argb_color;

typedef char argb_color_size_assert[
	sizeof(argb_color) == 0x8 ? 1 : -1];

/* ---------- prototypes */

void terminal_initialize(
	void);
void hud_initialize(
	void);
void draw_string_initialize(
	void);
void draw_string_dispose_from_old_map(
	void);
void hud_dispose_from_old_map(
	void);
void draw_string_dispose(
	void);
void terminal_dispose(
	void);
void hud_dispose(
	void);
void cinematic_render(
	void);
void code_000cf2f0(
	void);
void terminal_draw(
	void);
void main_framerate_render(
	void);
void code_000cea10(
	void);
real_argb_color *interface_get_real_argb_color(
	short interface_color_table_index,
	short color_index,
	real_argb_color *color);

/* ---------- globals */

/* ---------- public code */

void interface_initialize(
	void)
{
	terminal_initialize();
	hud_initialize();
	draw_string_initialize();
	first_person_weapons_initialize();

	return;
}

void interface_dispose_from_old_map(
	void)
{
	draw_string_dispose_from_old_map();
	hud_dispose_from_old_map();
	first_person_weapons_dispose_from_old_map();

	return;
}

void interface_dispose(
	void)
{
	draw_string_dispose();
	terminal_dispose();
	hud_dispose();
	first_person_weapons_dispose();

	return;
}

void interface_set_bitmap_text_draw_mode(
	short interface_font_index,
	short style,
	short justification,
	unsigned long flags,
	short color_table_index,
	short color_index)
{
	long font_tag_index;
	real_argb_color color;

	font_tag_index = interface_get_tag_index(interface_font_index);
	interface_get_real_argb_color(color_table_index, color_index, &color);
	draw_string_set_draw_mode(font_tag_index, style, justification, flags, &color);

	return;
}

argb_color *interface_get_rgb_color(
	short interface_color_table_index,
	short color_index,
	argb_color *color)
{
	real_argb_color real_color;

	interface_get_real_argb_color(
		interface_color_table_index,
		color_index,
		&real_color);
	color->n[0] = (word)(real_color.n[0] * 65535.0f);
	color->n[1] = (word)(real_color.n[1] * 65535.0f);
	color->n[2] = (word)(real_color.n[2] * 65535.0f);
	color->n[3] = (word)(real_color.n[3] * 65535.0f);

	return color;
}

void interface_draw_fullscreen_overlays(
	void)
{
	cinematic_render();
	code_000cf2f0();
	hud_render_timer();
	terminal_draw();
	main_framerate_render();
	code_000cea10();

	return;
}

/* ---------- private code */
