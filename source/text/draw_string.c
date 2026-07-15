/*
DRAW_STRING.C

symbols in this file:
0018AB70 0010:
	_draw_string_initialize (0000)
0018AB80 0070:
	_draw_string_initialize_for_new_map (0000)
0018ABF0 0010:
	_draw_string_dispose_from_old_map (0000)
0018AC00 0010:
	_draw_string_dispose (0000)
0018AC10 0070:
	_code_0018ac10 (0000)
0018AC80 00e0:
	_code_0018ac80 (0000)
0018AD60 0050:
	_draw_string_get_string (0000)
0018ADB0 0070:
	_draw_string_set_tab_stops (0000)
0018AE20 0070:
	_draw_string_set_indents (0000)
0018AE90 0150:
	_draw_string_set_color (0000)
0018AFE0 0050:
	_draw_string_get_color (0000)
0018B030 0020:
	_draw_string_set_font (0000)
0018B050 00b0:
	_draw_string_set_format (0000)
0018B100 0040:
	_draw_string_set_draw_mode (0000)
0018B140 0020:
	_draw_string_set_highlight (0000)
0018B160 03b0:
	_code_0018b160 (0000)
0018B510 0070:
	_code_0018b510 (0000)
0018B580 0100:
	_code_0018b580 (0000)
0018B680 0270:
	_code_0018b680 (0000)
0018B8F0 0110:
	_code_0018b8f0 (0000)
0018BA00 0210:
	_code_0018ba00 (0000)
0018BC10 0210:
	_code_0018bc10 (0000)
0018BE20 0390:
	_draw_string (0000)
0018C1B0 0390:
	_draw_unicode_string (0000)
0018C540 00c0:
	_draw_string_compute_bounds (0000)
0018C600 00c0:
	_draw_unicode_string_compute_bounds (0000)
0018C6C0 0050:
	_draw_string_pick (0000)
0018C710 0130:
	_bitmap_draw_string (0000)
002A26E4 002d:
	??_C@_0CN@NJAONNEI@internal?5string?5localization?5tag@ (0000)
002A2714 0029:
	??_C@_0CJ@LDLNPDDM@index?$DO?$DN0?5?$CG?$CG?5index?$DMNUMBER_OF_TEXT@ (0000)
002A2740 0022:
	??_C@_0CC@EBHGCODL@c?3?2halo?2SOURCE?2text?2draw_string?4@ (0000)
002A2764 002e:
	??_C@_0CO@LAOAKNHK@count?$DO?$DN0?5?$CG?$CG?5count?$DMMAXIMUM_NUMBER@ (0000)
002A2794 0014:
	??_C@_0BE@FGPECGKA@paragraph_indent?$DO?$DN0?$AA@ (0000)
002A27A8 0012:
	??_C@_0BC@DNGFHNA@initial_indent?$DO?$DN0?$AA@ (0000)
002A27BC 002d:
	??_C@_0CN@CGOGANEE@?$CIcolor?9?$DOblue?5?$DO?$DN?50?4f?$CJ?5?$CG?$CG?5?$CIcolor?9?$DO@ (0000)
002A27EC 002f:
	??_C@_0CP@FADKAPOG@?$CIcolor?9?$DOgreen?5?$DO?$DN?50?4f?$CJ?5?$CG?$CG?5?$CIcolor?9@ (0000)
002A281C 002b:
	??_C@_0CL@COFHKNKB@?$CIcolor?9?$DOred?5?$DO?$DN?50?4f?$CJ?5?$CG?$CG?5?$CIcolor?9?$DOr@ (0000)
002A2848 002f:
	??_C@_0CP@DFGKLIFB@?$CIcolor?9?$DOalpha?5?$DO?$DN?50?4f?$CJ?5?$CG?$CG?5?$CIcolor?9@ (0000)
002A2878 0040:
	??_C@_0EA@DLFMIHAM@justification?$DO?$DN0?5?$CG?$CG?5justificatio@ (0000)
002A28B8 0046:
	??_C@_0EG@IPKHNIBH@style?$DN?$DN_text_style_plain?5?$HM?$HM?5?$CIsty@ (0000)
002A2900 0029:
	??_C@_0CJ@CIJKMAAN@VALID_FLAGS?$CIflags?0?5NUMBER_OF_TEX@ (0000)
002A292C 003e:
	??_C@_0DO@NHIHGJAE@x0?$DO?$DN0?5?$CG?$CG?5x0?$CLdx?$DM?$DNdraw_character_s@ (0000)
002A296C 003c:
	??_C@_0DM@BMLHMCCB@y0?$DO?$DN0?5?$CG?$CG?5y0?$DM?$DNdraw_character_soft@ (0000)
002A29A8 000d:
	??_C@_0N@HNKPGPMM@result?$CB?$DNNONE?$AA@ (0000)
002A29B8 0049:
	??_C@_0EJ@CJKOFANJ@tab_stop_index?$DO?$DN0?5?$CG?$CG?5tab_stop_in@ (0000)
004C1908 00f8:
	_bss_004c1908 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "interface/interface.h"
#include "math/real_math.h"
#include "text/draw_string.h"
#include "text/international_strings.h"
#include "text/text_group.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct draw_string_globals
{
	byte __unknown0[0x20];
	long localization_string_list_index;
	long font_index;
	unsigned long flags;
	short style;
	short justification;
	real_argb_color color;
	short tab_stop_count;
	short tab_stops[16];
	short highlight_start;
	short highlight_end;
	short initial_indent;
	short paragraph_indent;
	byte __unknown6A[0x8E];
};

/* ---------- prototypes */

/* ---------- globals */

struct draw_string_globals bss_004c1908;

#define draw_string_globals bss_004c1908

/* ---------- public code */

void draw_string_initialize(
	void)
{
	return;
}

void draw_string_initialize_for_new_map(
	void)
{
	long localization_string_list_index = interface_get_tag_index(_interface_string_list_localization);
	draw_string_globals.localization_string_list_index = localization_string_list_index;

	if (localization_string_list_index != NONE)
	{
		set_language_code((short)atoi(string_list_get_string(localization_string_list_index, 0)));
		draw_string_globals.tab_stop_count = 0;
		draw_string_globals.flags = 0;
		draw_string_globals.justification = 0;
		draw_string_globals.initial_indent = 0;
		draw_string_globals.paragraph_indent = 0;
		draw_string_globals.font_index = NONE;
	}
	else
	{
		error(_error_immediate, "internal string localization tag is missing.");
	}

	return;
}

void draw_string_dispose_from_old_map(
	void)
{
	draw_string_globals.localization_string_list_index = NONE;
	return;
}

void draw_string_dispose(
	void)
{
	return;
}

/* ---------- private code */
