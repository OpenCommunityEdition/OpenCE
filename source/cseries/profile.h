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
	unsigned __int64 field_10;
	long field_18;
	long field_1C;
	__int64 field_20;
	long field_28[MAXIMUM_PROFILE_HISTORY];
	__int64 field_208[MAXIMUM_PROFILE_HISTORY];
	long field_5C8;
	long field_5CC;
	__int64 field_5D0;
	long field_5D8;
	long field_5DC;
	__int64 field_5E0;
	long field_5E8;
	long field_5EC;
	__int64 field_5F0;
};

typedef char profile_section_size_assert[
	sizeof(struct profile_section) == 0x5F8 ? 1 : -1];
typedef char profile_section_field_18_offset_assert[
	offsetof(struct profile_section, field_18) == 0x18 ? 1 : -1];
typedef char profile_section_field_1C_offset_assert[
	offsetof(struct profile_section, field_1C) == 0x1C ? 1 : -1];
typedef char profile_section_field_20_offset_assert[
	offsetof(struct profile_section, field_20) == 0x20 ? 1 : -1];
typedef char profile_section_field_28_offset_assert[
	offsetof(struct profile_section, field_28) == 0x28 ? 1 : -1];
typedef char profile_section_field_208_offset_assert[
	offsetof(struct profile_section, field_208) == 0x208 ? 1 : -1];
typedef char profile_section_field_5C8_offset_assert[
	offsetof(struct profile_section, field_5C8) == 0x5C8 ? 1 : -1];
typedef char profile_section_field_5CC_offset_assert[
	offsetof(struct profile_section, field_5CC) == 0x5CC ? 1 : -1];
typedef char profile_section_field_5D0_offset_assert[
	offsetof(struct profile_section, field_5D0) == 0x5D0 ? 1 : -1];
typedef char profile_section_field_5D8_offset_assert[
	offsetof(struct profile_section, field_5D8) == 0x5D8 ? 1 : -1];
typedef char profile_section_field_5DC_offset_assert[
	offsetof(struct profile_section, field_5DC) == 0x5DC ? 1 : -1];
typedef char profile_section_field_5E0_offset_assert[
	offsetof(struct profile_section, field_5E0) == 0x5E0 ? 1 : -1];
typedef char profile_section_field_5E8_offset_assert[
	offsetof(struct profile_section, field_5E8) == 0x5E8 ? 1 : -1];
typedef char profile_section_field_5EC_offset_assert[
	offsetof(struct profile_section, field_5EC) == 0x5EC ? 1 : -1];
typedef char profile_section_field_5F0_offset_assert[
	offsetof(struct profile_section, field_5F0) == 0x5F0 ? 1 : -1];

/* ---------- prototypes/PROFILE.C */

void profile_initialize(void);

void profile_enter_private(struct profile_section *objects_update_section);
void profile_exit_private(struct profile_section *objects_update_section);

void profile_render_window_start(void *arg0);
void profile_render_window_end(void);

/* ---------- globals */

extern boolean profile_timebase_ticks;
extern boolean profile_global_enable;
extern boolean profile_dump_frames;
extern boolean profile_dump_lost_frames;

/* comm */
boolean profile_global_enable;

/* ---------- public code */

void profile_initialize(void);

#endif // __PROFILE_H
