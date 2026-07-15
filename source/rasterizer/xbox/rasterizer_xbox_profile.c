/*
RASTERIZER_XBOX_PROFILE.C

symbols in this file:
0015ECD0 0080:
	_code_0015ecd0 (0000)
0015ED50 0110:
	_code_0015ed50 (0000)
0015EE60 0090:
	_code_0015ee60 (0000)
0015EEF0 0020:
	_code_0015eef0 (0000)
0015EF10 0070:
	_rasterizer_profile_initialize (0000)
0015EF80 0150:
	_rasterizer_profile_frame_begin (0000)
0015F0D0 0020:
	_rasterizer_profile_window_begin (0000)
0015F0F0 0070:
	__rasterizer_profile_enable (0000)
0015F160 0130:
	_rasterizer_profile_begin (0000)
0015F290 0140:
	_rasterizer_profile_end (0000)
0015F3D0 0050:
	_rasterizer_profile_get_string (0000)
0015F420 0120:
	_rasterizer_profile_query (0000)
0015F540 00e0:
	_rasterizer_profile_query_pushbuffer (0000)
0015F620 00e0:
	_rasterizer_profile_frame_end (0000)
0015F700 0010:
	_rasterizer_profile_window_end (0000)
0015F710 0010:
	_rasterizer_profile_dispose (0000)
00291FBC 000d:
	??_C@_0N@NPLHKNPM@screen?5flash?$AA@ (0000)
00291FCC 0004:
	??_C@_03HOHJDGGL@HUD?$AA@ (0000)
00291FD0 000e:
	??_C@_0O@KEPDLHCB@screen?5effect?$AA@ (0000)
00291FE0 000c:
	??_C@_0M@MPLNCIAM@lens?5flares?$AA@ (0000)
00291FEC 0017:
	??_C@_0BH@JMHHLHBN@lens?5flare?5occl?4?5query?$AA@ (0000)
00292004 0018:
	??_C@_0BI@BJCKIOBM@lens?5flare?5occl?4?5submit?$AA@ (0000)
0029201C 0014:
	??_C@_0BE@PGACMIOM@queued?5transparents?$AA@ (0000)
00292030 000f:
	??_C@_0P@NADPEMMH@detail?5objects?$AA@ (0000)
00292040 0011:
	??_C@_0BB@JKPKBHGC@env?5decals?5water?$AA@ (0000)
00292054 000f:
	??_C@_0P@CMGCGEPL@env?5fog?5screen?$AA@ (0000)
00292064 0008:
	??_C@_07LCJPKBIL@env?5fog?$AA@ (0000)
0029206C 0010:
	??_C@_0BA@POKLBKIL@env?5transparent?$AA@ (0000)
0029207C 0010:
	??_C@_0BA@NCEEKADL@env?5reflections?$AA@ (0000)
0029208C 0017:
	??_C@_0BH@HJJDMAAJ@env?5reflection?5mirrors?$AA@ (0000)
002920A4 0017:
	??_C@_0BH@BIDFOOOJ@env?5lightmaps?5ref?4mask?$AA@ (0000)
002920BC 0017:
	??_C@_0BH@JPGDBMFC@env?5lightmaps?5specular?$AA@ (0000)
002920D4 0014:
	??_C@_0BE@NMLCJCAM@env?5lights?5specular?$AA@ (0000)
002920E8 0015:
	??_C@_0BF@MCHCJCJA@env?5decals?5secondary?$AA@ (0000)
00292100 0013:
	??_C@_0BD@OMPHEFCG@env?5decals?5primary?$AA@ (0000)
00292114 000d:
	??_C@_0N@LLIEDNIN@env?5textures?$AA@ (0000)
00292124 0018:
	??_C@_0BI@LPGLMPFG@env?5decals?5alpha?9tested?$AA@ (0000)
0029213C 0011:
	??_C@_0BB@NPLFGCFP@env?5decals?5light?$AA@ (0000)
00292150 000b:
	??_C@_0L@OBJNHOKI@env?5lights?$AA@ (0000)
0029215C 000c:
	??_C@_0M@EMKKMCPM@env?5shadows?$AA@ (0000)
00292168 000e:
	??_C@_0O@OEMAEMFI@env?5lightmaps?$AA@ (0000)
00292178 0007:
	??_C@_06FEMFHOOG@models?$AA@ (0000)
00292180 000a:
	??_C@_09ELJLJCBG@model?5sky?$AA@ (0000)
0029218C 0020:
	??_C@_0CA@HHAEBLHJ@?$CD?$CD?$CD?5PROFILE?3?5?$CFs?5?9?9?5tell?5Bernie?$CB?$AA@ (0000)
002921AC 0026:
	??_C@_0CG@CABPNPIN@?$CD?$CD?$CD?5PROFILE?5?$CI?$CD?$CFd?$CJ?3?5?$CFs?5?9?9?5tell?5Be@ (0000)
002921D4 0039:
	??_C@_0DJ@EMCMCPAB@c?3?2halo?2SOURCE?2rasterizer?2xbox?2r@ (0000)
00292210 0011:
	??_C@_0BB@LCIAJAAI@end?5out?9of?9synch?$AA@ (0000)
00292224 0013:
	??_C@_0BD@PDKEIIAE@begin?5out?9of?9synch?$AA@ (0000)
00292238 0022:
	??_C@_0CC@GJPJJFMA@callback?5recieved?5invalid?5contex@ (0000)
0029225C 0019:
	??_C@_0BJ@HIIHLGIF@local_profile_enable?$DM100?$AA@ (0000)
00292278 0017:
	??_C@_0BH@CGAGFGCG@local_profile_enable?$DO0?$AA@ (0000)
00292290 002c:
	??_C@_0CM@MDDANJMO@profile?5begin?1end?5pairing?5incorr@ (0000)
002922BC 0029:
	??_C@_0CJ@FEPBBPKK@profile?5duplication?5within?5frame@ (0000)
002922E8 0034:
	??_C@_0DE@PLFGBHLC@profile?$DO?$DN0?5?$CG?$CG?5profile?$DMNUMBER_OF_@ (0000)
0029231C 002a:
	??_C@_0CK@GJJPMKBP@profile?5begin?1end?5pairing?5incorr@ (0000)
00292348 0027:
	??_C@_0CH@BKOLJHAH@profile?5duplication?5within?5frame@ (0000)
00292370 001e:
	??_C@_0BO@KDABKJGE@profile?5not?5completed?5?$CIquery?$CJ?$AA@ (0000)
0030CF00 0084:
	_data_0030cf00 (0000)
00465E28 0462:
	_bss_00465e28 (0000)
*/

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

enum
{
	NUMBER_OF_RASTERIZER_PROFILES = 29,
	MAXIMUM_RASTERIZER_PROFILE_CALLBACKS = 16
};

/* ---------- macros */

/* ---------- structures */

struct rasterizer_profile_globals
{
	__int64 performance_counter_frequency;
	short active_profile_index;
	short pad_a;
	short window_index;
	short pad_e;
	const char *profile_names[NUMBER_OF_RASTERIZER_PROFILES];
};

struct rasterizer_window_parameters
{
	short rasterizer_target;
	short window_index;
};

struct rasterizer_profile_elapsed_state
{
	__int64 callback_start_times[MAXIMUM_RASTERIZER_PROFILE_CALLBACKS];
	__int64 active_window_masks[NUMBER_OF_RASTERIZER_PROFILES];
	volatile __int64 elapsed_times[NUMBER_OF_RASTERIZER_PROFILES];
};

/* ---------- prototypes */

long __stdcall QueryPerformanceFrequency(
	__int64 *frequency);

/* ---------- globals */

static volatile __int64 rasterizer_profile_start_times[NUMBER_OF_RASTERIZER_PROFILES];
static struct rasterizer_profile_elapsed_state rasterizer_profile_elapsed_state;
static volatile __int64 rasterizer_profile_callback_end_times[MAXIMUM_RASTERIZER_PROFILE_CALLBACKS];
static volatile __int64 rasterizer_profile_callback_elapsed_times[MAXIMUM_RASTERIZER_PROFILE_CALLBACKS];

extern struct rasterizer_profile_globals data_0030cf00;
extern struct rasterizer_window_parameters global_window_parameters;

/* ---------- public code */

boolean rasterizer_profile_initialize(
	void)
{
	long profile_count;
	long profile_index;

	profile_count = NUMBER_OF_RASTERIZER_PROFILES;
	profile_index = 0;
	do
	{
		rasterizer_profile_start_times[profile_index] = 0;
		rasterizer_profile_elapsed_state.elapsed_times[profile_index] = 0;
		profile_index++;
	}
	while (--profile_count);

	csmemset(rasterizer_profile_elapsed_state.callback_start_times, 0, sizeof(rasterizer_profile_elapsed_state.callback_start_times));
	csmemset((void *)rasterizer_profile_callback_end_times, 0, sizeof(rasterizer_profile_callback_end_times));
	csmemset((void *)rasterizer_profile_callback_elapsed_times, 0, sizeof(rasterizer_profile_callback_elapsed_times));
	QueryPerformanceFrequency(&data_0030cf00.performance_counter_frequency);

	return TRUE;
}

void rasterizer_profile_window_begin(
	void)
{
	data_0030cf00.window_index = global_window_parameters.window_index;
	data_0030cf00.active_profile_index = NONE;

	return;
}

const char *rasterizer_profile_get_string(
	short profile)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
		363,
		profile>=0 && profile<NUMBER_OF_RASTERIZER_PROFILES);

	return data_0030cf00.profile_names[profile];
}

void rasterizer_profile_window_end(
	void)
{
	return;
}

void rasterizer_profile_dispose(
	void)
{
	return;
}

/* ---------- private code */
