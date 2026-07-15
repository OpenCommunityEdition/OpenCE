/*
RENDER_CONTRAILS.C

symbols in this file:
001777D0 0090:
	_contrail_fade (0000)
00177860 07a0:
	_code_00177860 (0000)
00178000 00b0:
	_code_00178000 (0000)
001780B0 0010:
	_render_contrails_ground_mapped (0000)
001780C0 0010:
	_render_contrails_media_mapped (0000)
001780D0 0010:
	_render_contrails_normal (0000)
0029FC28 002d:
	??_C@_0CN@ILEHFPFH@contrail?5?$CFs?5uses?5an?5unsupported?5@ (0000)
0029FC58 0016:
	??_C@_0BG@PBIEBPJJ@triangles?5?$CG?$CG?5vertices?$AA@ (0000)
0029FC70 0029:
	??_C@_0CJ@KBEDIECC@c?3?2halo?2SOURCE?2render?2render_con@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "render.h"

/* ---------- constants */

enum
{
	_contrail_render_mode_media_mapped = 1 << 2,
	_contrail_render_mode_ground_mapped = 1 << 3,
	_contrail_render_mode_normal = ~(_contrail_render_mode_media_mapped | _contrail_render_mode_ground_mapped),
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void code_00178000(
	unsigned long render_mode_flags);

/* ---------- globals */

/* ---------- public code */

void render_contrails_ground_mapped(
	void)
{
	code_00178000(_contrail_render_mode_ground_mapped);
	return;
}

void render_contrails_media_mapped(
	void)
{
	code_00178000(_contrail_render_mode_media_mapped);
	return;
}

void render_contrails_normal(
	void)
{
	code_00178000(_contrail_render_mode_normal);
	return;
}

/* ---------- private code */
