/*
RECORDED_ANIMATION_PLAYBACK.C

symbols in this file:
00081EF0 0090:
	_code_00081ef0 (0000)
00081F80 0090:
	_code_00081f80 (0000)
00082010 0090:
	_code_00082010 (0000)
000820A0 0090:
	_code_000820a0 (0000)
00082130 00a0:
	_code_00082130 (0000)
000821D0 0040:
	_code_000821d0 (0000)
00082210 0040:
	_code_00082210 (0000)
00082250 0040:
	_code_00082250 (0000)
00082290 0200:
	_code_00082290 (0000)
00082490 0200:
	_code_00082490 (0000)
00082690 0040:
	_recorded_animation_initialize_event_stream (0000)
000826D0 0040:
	_recorded_animation_initialize_event_stream_with_size (0000)
00082710 01f0:
	_recorded_animation_apply_event_stream (0000)
00082900 0010:
	_byte_swap_recording_stream (0000)
00258B64 001d:
	??_C@_0BN@BFIMCFJJ@vector_short_difference_data?$AA@ (0000)
00258B84 001c:
	??_C@_0BM@PGDPGFD@vector_char_difference_data?$AA@ (0000)
00258BA0 0014:
	??_C@_0BE@DKBHBGID@throttle_event_data?$AA@ (0000)
00258BB4 0018:
	??_C@_0BI@KMHECDMB@weapon_index_event_data?$AA@ (0000)
00258BCC 0019:
	??_C@_0BJ@FKCNELO@control_flags_event_data?$AA@ (0000)
00258BE8 0018:
	??_C@_0BI@LIFDCNFO@aiming_speed_event_data?$AA@ (0000)
00258C00 001b:
	??_C@_0BL@KNDJMAJG@animation_state_event_data?$AA@ (0000)
00258C1C 0032:
	??_C@_0DC@OJKPBELH@header?9?$DOevent_type?$DN?$DN_playback_an@ (0000)
00258C50 0008:
	??_C@_07MMBOFDEG@control?$AA@ (0000)
00258C58 0036:
	??_C@_0DG@CLKBONJL@c?3?2halo?2SOURCE?2cutscene?2recorded@ (0000)
00258C90 002f:
	??_C@_0CP@MMJOFKBA@header?9?$DOevent_type?$DN?$DN_playback_ai@ (0000)
00258CC0 0030:
	??_C@_0DA@LDGBKFOB@header?9?$DOevent_type?$DN?$DN_playback_co@ (0000)
00258CF0 002f:
	??_C@_0CP@PKMHGEMN@header?9?$DOevent_type?$DN?$DN_playback_we@ (0000)
00258D20 002b:
	??_C@_0CL@JMFIPONF@header?9?$DOevent_type?$DN?$DN_playback_th@ (0000)
00258D4C 0004:
	__real@3b4de32e (0000)
00258D50 0092:
	??_C@_0JC@CIBGBOCG@header?9?$DOevent_type?$DO?$DN_playback_ve@ (0000)
00258DE8 0094:
	??_C@_0JE@EDOLGDPM@header?9?$DOevent_type?$DO?$DN_playback_ve@ (0000)
00258E7C 0029:
	??_C@_0CJ@FIDPKLHA@header?9?$DOevent_type?$DMNUMBEROF?$CIappl@ (0000)
00258EA8 001d:
	??_C@_0BN@GHBCOJNB@time_delta?$DOUNSIGNED_CHAR_MAX?$AA@ (0000)
00258EC8 002c:
	??_C@_0CM@IOCGHDGB@time_delta?$DO1?$CG?$CGtime_delta?$DM?$DNUNSIGN@ (0000)
00258EF4 0011:
	??_C@_0BB@LIMNNLMB@?$CKplayback_stream?$AA@ (0000)
00258F08 0010:
	??_C@_0BA@IBBGIMIK@playback_stream?$AA@ (0000)
00258F18 0006:
	??_C@_05CDKPFCKO@ticks?$AA@ (0000)
002DCF20 0110:
	_data_002dcf20 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"

#undef memcpy
#include <string.h>

/* ---------- constants */

enum
{
	_playback_animation_state_set = 2,
	_playback_aiming_speed_set,
	_playback_control_flags_set,
	_playback_weapon_index_set,
	_playback_throttle_set,
	_playback_vector_char_difference_set,
	_playback_vector_short_difference_set = 15,
};

enum
{
	_control_vector_facing_bit,
	_control_vector_aiming_bit,
	_control_vector_looking_bit,
	NUMBER_OF_CONTROL_VECTORS,
};

/* ---------- macros */

/* ---------- structures */

struct direction_playback_controller
{
	short yaw;
	short pitch;
};

struct animation_event_header
{
	byte time_delta : 2;
	byte event_type : 6;
};

struct vector_char_difference_data
{
	char delta_yaw;
	char delta_pitch;
};

struct vector_short_difference_data
{
	short delta_yaw;
	short delta_pitch;
};

struct animation_playback_controller
{
	struct direction_playback_controller facing_control;
	struct direction_playback_controller aiming_control;
	struct direction_playback_controller looking_control;
};

struct recorded_unit_control
{
	byte byte_field0;
	byte byte_field1;
	short word_field2;
	short word_field4;
	short version2_field;
	short version3_field;
	short unused_field10;
	real_vector2d vector2d_field12;
	long long_field20;
	long version1_field;
	real_vector3d vector3d_field28;
	real_vector3d vector3d_field40;
	real_vector3d vector3d_field52;
};

/* ---------- prototypes */

void recorded_animation_initialize_unit_control(
	struct recorded_unit_control *unit_control,
	byte **stream,
	byte unit_control_data_version);

/* ---------- globals */

/* ---------- public code */

void recorded_animation_initialize_event_stream(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *unit_control,
	byte **playback_stream,
	byte unit_control_data_version)
{
	recorded_animation_initialize_unit_control(
		unit_control,
		playback_stream,
		unit_control_data_version);

	memcpy(animation_state, *playback_stream, sizeof(*animation_state));
	*playback_stream += sizeof(*animation_state);

	return;
}

void recorded_animation_initialize_event_stream_with_size(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *unit_control,
	byte **playback_stream)
{
	memcpy(unit_control, *playback_stream, sizeof(*unit_control));
	*playback_stream += sizeof(*unit_control);

	memcpy(animation_state, *playback_stream, sizeof(*animation_state));
	*playback_stream += sizeof(*animation_state);

	return;
}

void byte_swap_recording_stream(
	void *stream,
	long stream_size,
	byte unit_control_data_version)
{
	return;
}

/* ---------- private code */

void code_00081ef0(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *control,
	struct animation_event_header const *header,
	byte const **playback_stream)
{
	byte const *event_data = *playback_stream;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 25, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 25, header);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 25, header->event_type==_playback_animation_state_set);

	control->byte_field0 = *event_data;
	(*playback_stream)++;

	return;
}

void code_00081f80(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *control,
	struct animation_event_header const *header,
	byte const **playback_stream)
{
	byte const *event_data = *playback_stream;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 26, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 26, header);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 26, header->event_type==_playback_aiming_speed_set);

	control->byte_field1 = *event_data;
	(*playback_stream)++;

	return;
}

void code_00082010(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *control,
	struct animation_event_header const *header,
	byte const **playback_stream)
{
	byte const *event_data = *playback_stream;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 27, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 27, header);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 27, header->event_type==_playback_control_flags_set);

	memcpy(&control->word_field2, event_data, sizeof(control->word_field2));
	*playback_stream += sizeof(control->word_field2);

	return;
}

void code_000820a0(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *control,
	struct animation_event_header const *header,
	byte const **playback_stream)
{
	byte const *event_data = *playback_stream;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 28, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 28, header);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 28, header->event_type==_playback_weapon_index_set);

	memcpy(&control->word_field4, event_data, sizeof(control->word_field4));
	*playback_stream += sizeof(control->word_field4);

	return;
}

void code_00082130(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *control,
	struct animation_event_header const *header,
	byte const **playback_stream)
{
	byte const *event_data = *playback_stream;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 33, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 35, header);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 36, header->event_type==_playback_throttle_set);

	memcpy(&control->vector2d_field12, event_data, sizeof(control->vector2d_field12));
	control->long_field20 = 0;
	*playback_stream += sizeof(control->vector2d_field12);

	return;
}

static void code_000821d0(
	struct vector_char_difference_data const *event_data,
	struct direction_playback_controller *control)
{
	control->yaw += event_data->delta_yaw;
	if (control->yaw > 1000)
	{
		control->yaw -= 1000;
	}
	else if (control->yaw < -1000)
	{
		control->yaw += 1000;
	}
	control->pitch += event_data->delta_pitch;

	return;
}

static void code_00082210(
	struct vector_short_difference_data const *event_data,
	struct direction_playback_controller *control)
{
	control->yaw += event_data->delta_yaw;
	if (control->yaw > 1000)
	{
		control->yaw -= 1000;
	}
	else if (control->yaw < -1000)
	{
		control->yaw += 1000;
	}
	control->pitch += event_data->delta_pitch;

	return;
}

static void code_00082250(
	real_vector3d *vector,
	struct direction_playback_controller const *controller)
{
	real_euler_angles2d angles;

	angles.yaw = (real)controller->yaw * 0.0031415927f;
	angles.pitch = (real)controller->pitch * 0.0031415927f;
	vector3d_from_euler_angles2d(vector, &angles);

	return;
}

void code_00082290(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *control,
	struct animation_event_header const *header,
	byte const **playback_stream)
{
	void const *serialized_event_data = *playback_stream;
	struct vector_char_difference_data const *event_data = serialized_event_data;
	word event_type;
	word update_facing;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 100, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 102, header);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 103, header->event_type>=_playback_vector_char_difference_set&&header->event_type-_playback_vector_char_difference_set<FLAG(NUMBER_OF_CONTROL_VECTORS));

	event_type = header->event_type - _playback_vector_char_difference_set;
	update_facing = event_type & FLAG(_control_vector_facing_bit);
	if (update_facing)
	{
		code_000821d0(event_data, &animation_state->facing_control);
		code_00082250(&control->vector3d_field28, &animation_state->facing_control);
	}

	if (event_type & FLAG(_control_vector_aiming_bit))
	{
		if (update_facing)
		{
			animation_state->aiming_control = animation_state->facing_control;
			control->vector3d_field40 = control->vector3d_field28;
		}
		else
		{
			code_000821d0(event_data, &animation_state->aiming_control);
			code_00082250(&control->vector3d_field40, &animation_state->aiming_control);
		}
	}

	if (event_type & FLAG(_control_vector_looking_bit))
	{
		if (update_facing)
		{
			animation_state->looking_control = animation_state->facing_control;
			control->vector3d_field52 = control->vector3d_field28;
			*playback_stream += sizeof(struct vector_char_difference_data);
			return;
		}

		if (event_type & FLAG(_control_vector_aiming_bit))
		{
			animation_state->looking_control = animation_state->aiming_control;
			control->vector3d_field52 = control->vector3d_field40;
			*playback_stream += sizeof(struct vector_char_difference_data);
			return;
		}

		code_000821d0(event_data, &animation_state->looking_control);
		code_00082250(&control->vector3d_field52, &animation_state->looking_control);
	}

	*playback_stream += sizeof(struct vector_char_difference_data);

	return;
}

void code_00082490(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *control,
	struct animation_event_header const *header,
	byte const **playback_stream)
{
	void const *serialized_event_data = *playback_stream;
	struct vector_short_difference_data const *event_data = serialized_event_data;
	word event_type;
	word update_facing;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 160, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 162, header);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback.c", 163, header->event_type>=_playback_vector_short_difference_set&&header->event_type-_playback_vector_short_difference_set<FLAG(NUMBER_OF_CONTROL_VECTORS));

	event_type = header->event_type - _playback_vector_short_difference_set;
	update_facing = event_type & FLAG(_control_vector_facing_bit);
	if (update_facing)
	{
		code_00082210(event_data, &animation_state->facing_control);
		code_00082250(&control->vector3d_field28, &animation_state->facing_control);
	}

	if (event_type & FLAG(_control_vector_aiming_bit))
	{
		if (update_facing)
		{
			animation_state->aiming_control = animation_state->facing_control;
			control->vector3d_field40 = control->vector3d_field28;
		}
		else
		{
			code_00082210(event_data, &animation_state->aiming_control);
			code_00082250(&control->vector3d_field40, &animation_state->aiming_control);
		}
	}

	if (event_type & FLAG(_control_vector_looking_bit))
	{
		if (update_facing)
		{
			animation_state->looking_control = animation_state->facing_control;
			control->vector3d_field52 = control->vector3d_field28;
			*playback_stream += sizeof(struct vector_short_difference_data);
			return;
		}

		if (event_type & FLAG(_control_vector_aiming_bit))
		{
			animation_state->looking_control = animation_state->aiming_control;
			control->vector3d_field52 = control->vector3d_field40;
			*playback_stream += sizeof(struct vector_short_difference_data);
			return;
		}

		code_00082210(event_data, &animation_state->looking_control);
		code_00082250(&control->vector3d_field52, &animation_state->looking_control);
	}

	*playback_stream += sizeof(struct vector_short_difference_data);

	return;
}
