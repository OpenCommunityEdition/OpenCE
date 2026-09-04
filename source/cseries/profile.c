/*
PROFILE.C

symbols in this file:
0007DCA0 0080:
	_profile_initialize (0000)
0007DD20 0160:
	_code_0007dd20 (0000)
0007DE80 0020:
	_profile_rasterizer_stats (0000)
0007DEA0 0040:
	_code_0007dea0 (0000)
0007DEE0 0010:
	_profile_seconds_elapsed (0000)
0007DEF0 0040:
	_profile_lapsed_frames (0000)
0007DF30 0020:
	_profile_lapsed_msec (0000)
0007DF50 0160:
	_find_profile_section (0000)
0007E0B0 0080:
	_profile_enter_private (0000)
0007E130 00a0:
	_profile_exit_private (0000)
0007E1D0 0610:
	_code_0007e1d0 (0000)
0007E7E0 0010:
	_code_0007e7e0 (0000)
0007E7F0 0050:
	_code_0007e7f0 (0000)
0007E840 0120:
	_compare_profile_sections (0000)
0007E960 0360:
	_profile_dump (0000)
0007ECC0 0080:
	_profile_dump_to_file (0000)
0007ED40 0080:
	_code_0007ed40 (0000)
0007EDC0 0040:
	_code_0007edc0 (0000)
0007EE00 0030:
	_code_0007ee00 (0000)
0007EE30 00a0:
	_code_0007ee30 (0000)
0007EED0 0020:
	_profile_sections_activate (0000)
0007EEF0 0020:
	_profile_sections_deactivate (0000)
0007EF10 0430:
	_profile_find_frame_value (0000)
0007F340 0040:
	_profile_find_game_value (0000)
0007F380 03a0:
	_profile_frame_get_value (0000)
0007F720 0060:
	_profile_frame_iterator_new (0000)
0007F780 0080:
	_profile_frame_iterator_next (0000)
0007F800 0090:
	_profile_frame_get_messages (0000)
0007F890 00a0:
	_profile_frame_get_stalls (0000)
0007F930 0090:
	_profile_rasterizer_stalls (0000)
0007F9C0 0030:
	_code_0007f9c0 (0000)
0007F9F0 0060:
	_code_0007f9f0 (0000)
0007FA50 0090:
	_profile_tick_start (0000)
0007FAE0 00a0:
	_profile_tick_end (0000)
0007FB80 0030:
	_profile_render_start (0000)
0007FBB0 0070:
	_profile_render_end (0000)
0007FC20 0090:
	_profile_render_window_start (0000)
0007FCB0 00a0:
	_profile_render_window_end (0000)
0007FD50 0030:
	_profile_texture_start (0000)
0007FD80 0070:
	_profile_texture_end (0000)
0007FDF0 0080:
	_profile_frame_start (0000)
0007FE70 0370:
	_profile_frame_end (0000)
000801E0 0030:
	_profile_idle_start (0000)
00080210 0070:
	_profile_idle_end (0000)
00257E54 0033:
	??_C@_0DD@GJHCEALK@?$HMl?$CFs?$HMt?$HMr?$CF?53?42f?1?$CF?54ld?$HMt?$CF?53?42f?1?$CF?54@ (0000)
00257E88 0044:
	??_C@_0EE@LGPEPOLG@?$CF?950s?$CF6ld?5?1?5?$CF7?43f?5?5?5?5?5?5?5?5?5?5?5?5?$CF5?4@ (0000)
00257ECC 0020:
	??_C@_0CA@IFDPKCJJ@?$HMt?$HMrthis?5frame?$HMtaverage?$HMtpeak?$HMn?$AA@ (0000)
00257EF0 0074:
	??_C@_0HE@OCELNIBL@section?7?5?5?5?5?5?5?5?5?5?5?5?5?5?5?5?5?5?5?5?5?5?5?5?5@ (0000)
00257F68 0041:
	??_C@_0EB@DNCFGCLL@parent_timesection?9?$DOself_msec?5?$DO?$DN@ (0000)
00257FAC 0021:
	??_C@_0CB@JIHDKIHN@c?3?2halo?2SOURCE?2cseries?2profile?4c@ (0000)
00257FD0 0037:
	??_C@_0DH@IAAOFPKE@profile_globals?4section_count?$DMMA@ (0000)
00258008 0039:
	??_C@_0DJ@MGEBDNKL@don?8t?5call?5profile_enter_private@ (0000)
00258044 0010:
	??_C@_0BA@OFIJJPPF@section?9?$DOactive?$AA@ (0000)
00258054 0008:
	??_C@_07BNGFJMOB@section?$AA@ (0000)
0025805C 001b:
	??_C@_0BL@DCGIKKDM@section?9?$DOstack_depth?$DN?$DNNONE?$AA@ (0000)
00258078 0032:
	??_C@_0DC@EBAGECGL@section?9?$DOstack_depth?$DN?$DNprofile_gl@ (0000)
002580AC 000e:
	??_C@_0O@CAINCP@f?9misc?5?$CF6?42f?5?$AA@ (0000)
002580BC 000e:
	??_C@_0O@HPDODOFI@r?9misc?5?$CF6?42f?5?$AA@ (0000)
002580CC 000b:
	??_C@_0L@CCBFPMDD@?5?5?5?5?5?5?5?5?5?5?$AA@ (0000)
002580D8 000a:
	??_C@_09DIDNDMFB@tex?$CF6?42f?5?$AA@ (0000)
002580E4 000d:
	??_C@_0N@ONKJJLFA@?5?5?5?5?5?5?5?5?5?5?5?5?$AA@ (0000)
002580F4 000c:
	??_C@_0M@CIJICDBL@stall?$CF6?42f?5?$AA@ (0000)
00258100 000a:
	??_C@_09LLNJGGAC@?5?5?5?5?5?5?5?$CFs?$AA@ (0000)
0025810C 000a:
	??_C@_09MEJENFBG@?$CFs?$CF6?42f?$CFs?$AA@ (0000)
00258118 0002:
	??_C@_01EFFIKLCJ@n?$AA@ (0000)
0025811C 0002:
	??_C@_01JBBJJEPG@p?$AA@ (0000)
00258120 000d:
	??_C@_0N@KKPPKAFO@?5render?$CF6?42f?$AA@ (0000)
00258130 0002:
	??_C@_01PKGAHCOL@?$CJ?$AA@ (0000)
00258134 0009:
	??_C@_08PBAHBJAK@?5?5?5?5?5?5?$CFs?$AA@ (0000)
00258140 0008:
	??_C@_07CCGGFFIN@?$CF6?42f?$CFs?$AA@ (0000)
00258148 0003:
	??_C@_02GFKOMOKH@?5?$CI?$AA@ (0000)
0025814C 0009:
	??_C@_08GLEMDNF@game?$CF2d?5?$AA@ (0000)
00258158 000c:
	??_C@_0M@CKHGFAEI@?5?5?5?5?5?5?5?5?5?5?5?$AA@ (0000)
00258164 000b:
	??_C@_0L@PHBKFODP@idle?$CF6?42f?5?$AA@ (0000)
00258170 000a:
	??_C@_09CIKJCKOO@?$CIsynced?$CJ?5?$AA@ (0000)
0025817C 000a:
	??_C@_09CCMCFOJI@?$CIslowed?$CJ?5?$AA@ (0000)
00258188 000a:
	??_C@_09CLBOIIGO@?$CIf?4?$CF3dms?$CJ?$AA@ (0000)
00258194 000a:
	??_C@_09LODAACPA@?$CIl?4?$CF3dms?$CJ?$AA@ (0000)
002581A0 000a:
	??_C@_09OAGDEMOJ@?$CIfree?$CF3d?$CJ?$AA@ (0000)
002581AC 000a:
	??_C@_09FOHHJKEH@?$CIlost?$CF3d?$CJ?$AA@ (0000)
002581B8 001e:
	??_C@_0BO@KFBPKJAE@frame?5?$CF5d?5vbl?5?$CF5I64d?5tot?$CF6?42f?$AA@ (0000)
002581D8 0008:
	__real@408f400000000000 (0000)
002581E0 0018:
	??_C@_0BI@LMKLONDN@maximum_section_count?$DO0?$AA@ (0000)
002581F8 0042:
	??_C@_0EC@FFADJNIP@format_mode?$DO?$DN0?5?$CG?$CG?5format_mode?$DMNU@ (0000)
0025823C 0037:
	??_C@_0DH@ICGEGPLO@sort_mode?$DO?$DN0?5?$CG?$CG?5sort_mode?$DMNUMBER@ (0000)
00258274 0005:
	??_C@_04LLEBNMDN@?$CFs?$AN?6?$AA@ (0000)
0025827C 000f:
	??_C@_0P@NHLICBHE@d?3?2profile?4txt?$AA@ (0000)
0025828C 0011:
	??_C@_0BB@MGJFLJKK@d?3?2framedump?4txt?$AA@ (0000)
002582A0 0003:
	??_C@_02GMLFBBN@wb?$AA@ (0000)
002582A4 0002:
	??_C@_01NBENCBCI@?$CK?$AA@ (0000)
002582A8 000b:
	??_C@_0L@KMPGOPBB@pushbuffer?$AA@ (0000)
002582B4 0004:
	??_C@_03HOPDAKLK@gpu?$AA@ (0000)
002582B8 0003:
	??_C@_02EDDPJOD@dt?$AA@ (0000)
002582BC 0008:
	??_C@_07JDHEGGGP@texture?$AA@ (0000)
002582C4 0006:
	??_C@_05MEMGOBLF@stall?$AA@ (0000)
002582CC 000c:
	??_C@_0M@BFMMLLIC@game_render?$AA@ (0000)
002582D8 000c:
	??_C@_0M@HBFEAABD@render0_3np?$AA@ (0000)
002582E4 000a:
	??_C@_09PKCOAFOC@render0_3?$AA@ (0000)
002582F0 000a:
	??_C@_09ODDFDEKD@render0_2?$AA@ (0000)
002582FC 000a:
	??_C@_09MIBIGHGA@render0_1?$AA@ (0000)
00258308 0008:
	??_C@_07HHKBOHGC@render0?$AA@ (0000)
00258310 0007:
	??_C@_06IAAOMEKN@render?$AA@ (0000)
00258318 000a:
	??_C@_09OHMOODOI@nonplayer?$AA@ (0000)
00258324 0008:
	??_C@_07OCBHBHOJ@player3?$AA@ (0000)
0025832C 0008:
	??_C@_07PLAMCGKI@player2?$AA@ (0000)
00258334 0008:
	??_C@_07NACBHFGL@player1?$AA@ (0000)
0025833C 0008:
	??_C@_07MJDKEECK@player0?$AA@ (0000)
00258344 0006:
	??_C@_05OIMJLJGC@game7?$AA@ (0000)
0025834C 0006:
	??_C@_05PBNCIICD@game6?$AA@ (0000)
00258354 0006:
	??_C@_05NKPPNLOA@game5?$AA@ (0000)
0025835C 0006:
	??_C@_05MDOEOKKB@game4?$AA@ (0000)
00258364 0006:
	??_C@_05IMKFHMGG@game3?$AA@ (0000)
0025836C 0006:
	??_C@_05JFLOENCH@game2?$AA@ (0000)
00258374 0006:
	??_C@_05LOJDBOOE@game1?$AA@ (0000)
0025837C 0006:
	??_C@_05KHIICPKF@game0?$AA@ (0000)
00258384 0005:
	??_C@_04EONOHKEP@load?$AA@ (0000)
0025838C 0006:
	??_C@_05MIJNFGED@frame?$AA@ (0000)
00258394 0020:
	??_C@_0CA@LFOMCIIE@name?5?$CG?$CG?5section_index_reference?$AA@ (0000)
002583B4 0004:
	__real@42055555 (0000)
002583B8 0004:
	__real@35aaaaab (0000)
002583C0 004e:
	??_C@_0EO@JBFLBLLD@iterator?9?$DOcurrent_buffer_index?5?$CB@ (0000)
00258410 0078:
	??_C@_0HI@EANDMEO@?$CIiterator?9?$DOcurrent_buffer_index?5@ (0000)
00258488 0087:
	??_C@_0IH@HJNMEKLP@?$CIprofile_globals?4current_frame?4g@ (0000)
00258510 0074:
	??_C@_0HE@FKBCHJEP@?$CIprofile_globals?4current_frame?4w@ (0000)
00258588 0075:
	??_C@_0HF@JPMAIONJ@?$CIprofile_globals?4current_frame?4w@ (0000)
00258600 0088:
	??_C@_0II@PFDJGDHA@?$CIprofile_globals?4current_frame?4g@ (0000)
002DCD30 0010:
	_header_strings (0000)
	_format_strings (0008)
0031DF40 113d54:
	_bss_0031df40 (0000)
	_profile_timebase_ticks (113d50)
	_profile_global_enable (113d51)
	_profile_dump_frames (113d52)
	_profile_dump_lost_frames (113d53)
*/

/* ---------- headers */

#include "cseries.h"
#include "profile.h"

#include "math/real_math.h"

/* ---------- constants */

enum
{
	MAXIMUM_PROFILE_SECTIONS = 256,
	MAXIMUM_GAME_TICKS_PER_FRAME = 150,
	MAXIMUM_PROFILE_FRAMES = 256,
	MAXIMUM_PROFILE_WINDOWS = 4,
};

/* ---------- macros */

#define QUERY_TIMEBASE(timebase) \
{ \
	__asm push eax \
	__asm push edx \
	__asm rdtsc \
	__asm mov dword ptr timebase, eax \
	__asm mov dword ptr timebase+4, edx \
	__asm pop edx \
	__asm pop eax \
}

/* ---------- structures */

struct profile_timer
{
	__int64 start;
	__int64 end;
	real total;
	real frame_total;
};

struct profile_frame
{
	byte __unknown0000[4];
	long frame_index;
	long rasterizer_frame_index;
	long vertical_blank_index;
	short game_tick_count;
	short window_count;
	byte window_ids[MAXIMUM_PROFILE_WINDOWS];
	struct profile_timer frame;
	struct profile_timer game_ticks[MAXIMUM_GAME_TICKS_PER_FRAME];
	struct profile_timer windows[MAXIMUM_PROFILE_WINDOWS];
	struct profile_timer render;
	struct profile_timer stall;
	struct profile_timer texture;
	struct profile_timer idle;
	long seconds_elapsed;
	short lapsed_frames;
	byte __unknown0F06[2];
	long lapsed_msec;
	boolean lapsed_msec_valid;
	char lapsed_reason[0x203];
	real rasterizer_gpu_time;
	unsigned long rasterizer_pushbuffer_size;
	long stall_count;
	short stall_index;
	byte __unknown111E[2];
	real stall_msec;
	byte __unknown1124[4];
};

struct profile_frame_iterator
{
	short current_buffer_index;
	short next_buffer_index;
};

struct profile_frame_info
{
	long rasterizer_frame_index;
	long vertical_blank_index;
};

struct profile_globals
{
	__int64 timebase_frequency;
	short stack_depth;
	boolean initialized;
	long history_index;
	short section_count;
	struct profile_section* sections[MAXIMUM_PROFILE_SECTIONS];
	FILE* framedump_file;
	short compare_type;
	long lost_frame_count;
	boolean unk;
	short current_frame_history_count;
	short current_frame_history_index;
	struct profile_frame frames[MAXIMUM_PROFILE_FRAMES];
	struct profile_frame current_frame;
};

/* ---------- prototypes */

void code_0007dd20(
	void);
void find_profile_section(
	struct profile_section *section);
void code_0007edc0(
	void);
static void code_0007ee30(
	const char *name,
	boolean active);
void profile_dump(
	const char *name,
	long use_name,
	long format_mode,
	long maximum_section_count,
	char *buffer);

/* ---------- globals */

static struct profile_globals profile_globals = {0};
boolean profile_timebase_ticks = FALSE;
boolean profile_global_enable = FALSE;
boolean profile_dump_frames = FALSE;
boolean profile_dump_lost_frames = FALSE;

/* ---------- public code */

void profile_dump_to_file(
	const char *name)
{
	char buffer[0x2000];
	long use_name;
	FILE *file;

	use_name = name && csstrlen(name);

	file = fopen("d:\\profile.txt", "a+b");
	if (file)
	{
		profile_dump(name, use_name, 0, 256, buffer);
		fprintf(file, "%s\r\n", buffer);
	}
	fclose(file);

	return;
}

void profile_rasterizer_stalls(
	long stall_count,
	short stall_index,
	unsigned long stall_ticks,
	long unused,
	__int64 stall_timebase)
{
	real msec = (real)(stall_timebase*1000.0f/profile_globals.timebase_frequency);

	profile_globals.current_frame.stall.start = 0;
	profile_globals.current_frame.stall.end = stall_timebase;
	profile_globals.current_frame.stall_count = stall_count;
	profile_globals.current_frame.stall_index = stall_index;
	profile_globals.current_frame.stall.total += msec;
	profile_globals.current_frame.stall.frame_total += msec;
	profile_globals.current_frame.stall_msec = (real)(stall_ticks*1000.0f/profile_globals.timebase_frequency);

	return;
}

void profile_frame_get_messages(
	struct profile_frame_iterator *iterator)
{
	match_assert("c:\\halo\\SOURCE\\cseries\\profile.c", 1463, iterator);
	match_assert("c:\\halo\\SOURCE\\cseries\\profile.c", 1464, (iterator->current_buffer_index >= 0) && (iterator->current_buffer_index < profile_globals.current_frame_history_count));
	match_assert("c:\\halo\\SOURCE\\cseries\\profile.c", 1465, iterator->current_buffer_index != profile_globals.current_frame_history_index);

	return;
}

long profile_frame_get_stalls(
	struct profile_frame_iterator *iterator,
	short *stall_index,
	real *stall_msec)
{
	struct profile_frame *frame = &profile_globals.frames[iterator->current_buffer_index];

	match_assert("c:\\halo\\SOURCE\\cseries\\profile.c", 1480, (iterator->current_buffer_index >= 0) && (iterator->current_buffer_index < profile_globals.current_frame_history_count));
	match_assert("c:\\halo\\SOURCE\\cseries\\profile.c", 1481, iterator->current_buffer_index != profile_globals.current_frame_history_index);

	*stall_index = frame->stall_index;
	*stall_msec = frame->stall_msec;

	return frame->stall_count;
}

void profile_lapsed_frames(
	short frames,
	boolean lapsed,
	const char *reason)
{
	profile_globals.current_frame.lapsed_frames = frames;
	profile_globals.current_frame.lapsed_msec_valid = frames>0 || !lapsed;

	if (reason)
	{
		csstrcpy(profile_globals.current_frame.lapsed_reason, reason);
	}

	return;
}

short profile_find_game_value(
	const char *name,
	short *section_index_reference)
{
	match_assert("c:\\halo\\SOURCE\\cseries\\profile.c", 1224, name && section_index_reference);

	*section_index_reference = NONE;

	return NONE;
}

void profile_sections_activate(
	const char *name)
{
	code_0007ee30(name, TRUE);

	return;
}

void profile_sections_deactivate(
	const char *name)
{
	code_0007ee30(name, FALSE);

	return;
}

void profile_frame_iterator_new(
	struct profile_frame_iterator *iterator)
{
	match_assert("c:\\halo\\SOURCE\\cseries\\profile.c", 1419, iterator);

	iterator->current_buffer_index = NONE;
	iterator->next_buffer_index = (profile_globals.current_frame_history_index+MAXIMUM_PROFILE_FRAMES-1)%MAXIMUM_PROFILE_FRAMES;

	return;
}

boolean profile_frame_iterator_next(
	struct profile_frame_iterator *iterator,
	struct profile_frame_info *info)
{
	short buffer_index = iterator->next_buffer_index;
	boolean result = FALSE;

	iterator->current_buffer_index = buffer_index;

	if (buffer_index!=NONE &&
		buffer_index<profile_globals.current_frame_history_count)
	{
		result = TRUE;

		if (info)
		{
			info->rasterizer_frame_index = profile_globals.frames[buffer_index].rasterizer_frame_index;
			info->vertical_blank_index = profile_globals.frames[buffer_index].vertical_blank_index;
		}

		iterator->next_buffer_index = (iterator->current_buffer_index+MAXIMUM_PROFILE_FRAMES-1)%MAXIMUM_PROFILE_FRAMES;
		if (iterator->next_buffer_index==profile_globals.current_frame_history_index)
		{
			iterator->next_buffer_index = NONE;
		}
	}

	return result;
}

void profile_seconds_elapsed(
	long seconds)
{
	profile_globals.current_frame.seconds_elapsed = seconds;

	return;
}

void profile_lapsed_msec(
	long msec)
{
	profile_globals.current_frame.lapsed_msec = msec;
	profile_globals.current_frame.lapsed_msec_valid = msec>0;

	return;
}

void profile_rasterizer_stats(
	real gpu_time,
	__int64 pushbuffer_size)
{
	profile_globals.current_frame.rasterizer_gpu_time = gpu_time;
	profile_globals.current_frame.rasterizer_pushbuffer_size = (unsigned long)pushbuffer_size;

	return;
}

void profile_initialize(
	void)
{
	short section_index = 0;

	profile_globals.timebase_frequency = 733333333;

	while (section_index<profile_globals.section_count)
	{
		profile_globals.sections[section_index++]->section_index = NONE;
	}

	profile_global_enable = TRUE;
	profile_globals.section_count = 0;
	profile_globals.stack_depth = 0;
	profile_globals.initialized = TRUE;
	profile_globals.history_index = 0;
	profile_globals.current_frame_history_count = 0;
	profile_globals.current_frame_history_index = 0;
	profile_globals.lost_frame_count = 999;
	profile_globals.framedump_file = NULL;

	return;
}

void profile_render_start(
	void)
{
	__int64 timebase;

	profile_globals.current_frame.window_count = 0;
	QUERY_TIMEBASE(timebase);
	profile_globals.current_frame.render.start = timebase;

	return;
}

void profile_render_end(
	void)
{
	__int64 timebase;
	real msec;

	QUERY_TIMEBASE(timebase);
	profile_globals.current_frame.render.end = timebase;
	msec = (real)((timebase-profile_globals.current_frame.render.start)*1000.0f/
		profile_globals.timebase_frequency);
	profile_globals.current_frame.render.total += msec;
	profile_globals.current_frame.render.frame_total += msec;

	return;
}

void profile_texture_start(
	void)
{
	__int64 timebase;

	QUERY_TIMEBASE(timebase);
	profile_globals.current_frame.texture.start = timebase;

	return;
}

void profile_texture_end(
	void)
{
	__int64 timebase;
	real msec;

	QUERY_TIMEBASE(timebase);
	profile_globals.current_frame.texture.end = timebase;
	msec = (real)((timebase-profile_globals.current_frame.texture.start)*1000.0f/
		profile_globals.timebase_frequency);
	profile_globals.current_frame.texture.total += msec;
	profile_globals.current_frame.texture.frame_total += msec;

	return;
}

void profile_idle_start(
	void)
{
	__int64 timebase;

	QUERY_TIMEBASE(timebase);
	profile_globals.current_frame.idle.start = timebase;

	return;
}

void profile_idle_end(
	void)
{
	__int64 timebase;
	real msec;

	QUERY_TIMEBASE(timebase);
	profile_globals.current_frame.idle.end = timebase;
	msec = (real)((timebase-profile_globals.current_frame.idle.start)*1000.0f/
		profile_globals.timebase_frequency);
	profile_globals.current_frame.idle.total += msec;
	profile_globals.current_frame.idle.frame_total += msec;

	return;
}

/* ---------- private code */

void code_0007dd20(
	void)
{
	if (profile_global_enable)
	{
		short index;

		for (index = 0; index<profile_globals.section_count; index++)
		{
			struct profile_section *section = profile_globals.sections[index];

			if (section->active)
			{
				section->field_20 -= section->field_208[profile_globals.history_index];
				section->field_18 -= section->field_28[profile_globals.history_index];
				section->field_208[profile_globals.history_index] = section->field_5D0;
				section->field_28[profile_globals.history_index] = section->field_5CC;
				section->field_20 += section->field_5D0;
				section->field_18 += section->field_5CC;

				if (section->field_5D0>section->field_5F0)
					section->field_5F0 = section->field_5D0;
				if (section->field_5CC>section->field_5E8)
					section->field_5E8 = section->field_5CC;

				section->field_5E0 += section->field_5D0;
				section->field_5D8 += section->field_5CC;
				section->field_5D0 = 0;
				section->field_5CC = 0;
				section->field_5C8++;
			}
		}
	}

	profile_globals.history_index = (profile_globals.history_index+1)%MAXIMUM_PROFILE_HISTORY;
	profile_globals.initialized = FALSE;

	return;
}

void find_profile_section(
	struct profile_section *section)
{
	match_assert("c:\\halo\\SOURCE\\cseries\\profile.c", 559, section);
	match_assert("c:\\halo\\SOURCE\\cseries\\profile.c", 560, section->active);

	if (section->section_index!=NONE)
	{
		match_vassert("c:\\halo\\SOURCE\\cseries\\profile.c", 566,
			section->section_index >= 0 &&
			section->section_index < profile_globals.section_count &&
			profile_globals.sections[section->section_index] == section,
			"don't call profile_enter_private(), call profile_enter()");
	}
	else
	{
		match_assert("c:\\halo\\SOURCE\\cseries\\profile.c", 570, profile_globals.section_count<MAXIMUM_PROFILE_SECTIONS);

		section->section_index = profile_globals.section_count;
		profile_globals.section_count++;
		profile_globals.sections[section->section_index] = section;

		csmemset(section->field_208, 0, sizeof(section->field_208));
		csmemset(section->field_28, 0, sizeof(section->field_28));

		section->field_18 = 0;
		section->field_20 = 0;
		section->stack_depth = NONE;
		section->field_5C8 = 0;
		section->field_5D0 = 0;
		section->field_5CC = 0;
		section->field_5E0 = 0;
		section->field_5D8 = 0;
		section->field_5F0 = 0;
		section->field_5E8 = 0;
	}

	return;
}

void code_0007edc0(
	void)
{
	if (profile_globals.unk)
	{
		if (profile_globals.framedump_file)
		{
			fprintf(profile_globals.framedump_file, "\r\n");
			fflush(profile_globals.framedump_file);
		}

		profile_globals.unk = FALSE;
	}

	return;
}

static void code_0007ee30(
	const char *name,
	boolean active)
{
	boolean all = csstrcmp(name, "*")==0;
	boolean prefix = name[0]=='_';
	short index;

	for (index = 0; index<profile_globals.section_count; index++)
	{
		struct profile_section *section = profile_globals.sections[index];

		if (!all)
		{
			if (prefix)
			{
				const char *pattern = name+1;

				if (*pattern)
				{
					const char *section_name = section->name;

					do
					{
						if (*pattern!=*section_name)
							goto next_section;
						pattern++;
						section_name++;
					}
					while (*pattern);
				}
			}
			else if (!strstr(section->name, name))
			{
				goto next_section;
			}
		}

		section->active = active;

next_section:
		;
	}

	return;
}
