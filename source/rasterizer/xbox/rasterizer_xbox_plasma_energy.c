/*
RASTERIZER_XBOX_PLASMA_ENERGY.C

symbols in this file:
0015E2B0 01b0:
	_code_0015e2b0 (0000)
0015E460 0050:
	_code_0015e460 (0000)
0015E4B0 0220:
	_code_0015e4b0 (0000)
0015E6D0 0060:
	_code_0015e6d0 (0000)
0015E730 0010:
	_code_0015e730 (0000)
0015E740 0590:
	_rasterizer_plasma_energy_draw (0000)
00291F10 0004:
	__real@3a03126f (0000)
00291F14 0033:
	??_C@_0DD@OEJHPIIG@plasma?9?$DOsecondary_noise_map_anim@ (0000)
00291F48 0031:
	??_C@_0DB@CGFOFGKP@plasma?9?$DOprimary_noise_map_animat@ (0000)
00291F7C 003f:
	??_C@_0DP@HOMIPMOO@c?3?2halo?2SOURCE?2rasterizer?2xbox?2r@ (0000)
*/

/* ---------- headers */

#include "cseries.h"

/* The January translation unit retains the XDK's out-of-line D3D wrappers.
 * This is the same source shape verified in rasterizer_xbox_text.c and
 * rasterizer_xbox_active_camouflage.c. */
#define D3DINLINE static
#include <xtl.h>

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

extern void *global_d3d_device;

/* ---------- public code */

/* NonMatching foundation: the calls below preserve the translation unit's
 * proven XDK wrapper set while the January plasma draw body is reconstructed.
 */
void rasterizer_plasma_energy_draw(
	long group_index)
{
	IDirect3DDevice8_SetRenderState(
		global_d3d_device,
		D3DRS_CULLMODE,
		D3DCULL_NONE);
	IDirect3DDevice8_SetTextureStageState(
		global_d3d_device,
		0,
		D3DTSS_ADDRESSU,
		D3DTADDRESS_WRAP);
	IDirect3DDevice8_SetVertexShaderConstant(
		global_d3d_device,
		-81,
		&group_index,
		1);
	return;
}

/* ---------- private code */
