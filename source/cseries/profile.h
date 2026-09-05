/*
PROFILE.H
*/

#ifndef __PROFILE_H
#define __PROFILE_H
#pragma once

/* ---------- constants */

enum
{
	MAXIMUM_PROFILE_HISTORY = 120,
};

/* ---------- macros */

#define profile_enter(section)							\
if (profile_global_enable && section.active)	\
{														\
	profile_enter_private(&section);					\
}														\

#define profile_exit(section)							\
if (profile_global_enable && section.active)	\
{														\
	profile_exit_private(&section);						\
}														\

/* ---------- structures */

struct profile_section
{
	const char *name;
	long section_index;
	boolean active;
	short stack_depth;
	long field_C;
	unsigned __int64 entry_timebase;
	long field_18;
	byte reserved1C[4];
	__int64 field_20;
	long field_28[MAXIMUM_PROFILE_HISTORY];
	__int64 field_208[MAXIMUM_PROFILE_HISTORY];
	long field_5C8;
	long frame_call_count;
	__int64 frame_elapsed_timebase;
	long field_5D8;
	byte reserved5DC[4];
	__int64 field_5E0;
	long field_5E8;
	byte reserved5EC[4];
	__int64 field_5F0;
};

typedef char profile_section_size_assert[
	sizeof(struct profile_section) == 0x5F8 ? 1 : -1];
typedef char profile_section_field_18_offset_assert[
	offsetof(struct profile_section, field_18) == 0x18 ? 1 : -1];
typedef char profile_section_reserved1C_offset_assert[
	offsetof(struct profile_section, reserved1C) == 0x1C ? 1 : -1];
typedef char profile_section_field_20_offset_assert[
	offsetof(struct profile_section, field_20) == 0x20 ? 1 : -1];
typedef char profile_section_field_28_offset_assert[
	offsetof(struct profile_section, field_28) == 0x28 ? 1 : -1];
typedef char profile_section_field_208_offset_assert[
	offsetof(struct profile_section, field_208) == 0x208 ? 1 : -1];
typedef char profile_section_field_5C8_offset_assert[
	offsetof(struct profile_section, field_5C8) == 0x5C8 ? 1 : -1];
typedef char profile_section_frame_call_count_offset_assert[
	offsetof(struct profile_section, frame_call_count) == 0x5CC ? 1 : -1];
typedef char profile_section_frame_elapsed_timebase_offset_assert[
	offsetof(struct profile_section, frame_elapsed_timebase) == 0x5D0 ? 1 : -1];
typedef char profile_section_field_5D8_offset_assert[
	offsetof(struct profile_section, field_5D8) == 0x5D8 ? 1 : -1];
typedef char profile_section_reserved5DC_offset_assert[
	offsetof(struct profile_section, reserved5DC) == 0x5DC ? 1 : -1];
typedef char profile_section_field_5E0_offset_assert[
	offsetof(struct profile_section, field_5E0) == 0x5E0 ? 1 : -1];
typedef char profile_section_field_5E8_offset_assert[
	offsetof(struct profile_section, field_5E8) == 0x5E8 ? 1 : -1];
typedef char profile_section_reserved5EC_offset_assert[
	offsetof(struct profile_section, reserved5EC) == 0x5EC ? 1 : -1];
typedef char profile_section_field_5F0_offset_assert[
	offsetof(struct profile_section, field_5F0) == 0x5F0 ? 1 : -1];

/* ---------- prototypes/PROFILE.C */

void profile_initialize(
	void);
void profile_tick_start(
	void);
void profile_tick_end(
	void);

void profile_enter_private(
	struct profile_section *section);
void profile_exit_private(
	struct profile_section *section);

void profile_render_window_start(
	boolean player_window);
void profile_render_window_end(void);
void profile_render_start(
	void);
void profile_render_end(
	void);
void profile_frame_start(
	void);
void profile_frame_end(
	void);
void profile_texture_start(
	void);
void profile_texture_end(
	void);

/* ---------- globals */

extern boolean profile_timebase_ticks;
extern boolean profile_global_enable;
extern boolean profile_dump_frames;
extern boolean profile_dump_lost_frames;

/* ---------- public code */

void profile_initialize(void);

#endif // __PROFILE_H
