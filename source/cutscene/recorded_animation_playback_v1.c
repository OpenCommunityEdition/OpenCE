/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "math/real_math.h"
#include "memory/byte_swapping.h"

/* ---------- constants */

enum
{
	_playback_v1_nothing,
	_playback_v1_end,
	_playback_v1_animation_state_set,
	_playback_v1_aiming_speed_set,
	_playback_v1_control_flags_set,
	_playback_v1_weapon_index_set,
	_playback_v1_throttle_set,
	_playback_v1_vectors_synchronize,
	_playback_v1_vectors_desynchronize,
	_playback_v1_facing_vector_set,
	_playback_v1_aiming_vector_set,
	_playback_v1_looking_vector_set,
	_playback_v1_facing_aiming_vector_set,
	_playback_v1_facing_looking_vector_set,
	_playback_v1_aiming_looking_vector_set,
	_playback_v1_facing_aiming_looking_vector_set,
	_playback_v1_facing_angles_set,
	_playback_v1_aiming_angles_set,
	_playback_v1_looking_angles_set,
	_playback_v1_facing_aiming_angles_set,
	_playback_v1_facing_looking_angles_set,
	_playback_v1_aiming_looking_angles_set,
	_playback_v1_facing_aiming_looking_angles_set,
};

/* ---------- macros */

/* ---------- structures */

struct animation_playback_controller;

struct recorded_animation_event_v1
{
	short type;
	word time_delta;
};

struct recorded_animation_state_set_event_v1
{
	struct recorded_animation_event_v1 event;
	byte animation_state;
	byte unused5;
};

struct recorded_aiming_speed_set_event_v1
{
	struct recorded_animation_event_v1 event;
	byte aiming_speed;
	byte unused5;
};

struct recorded_control_flags_set_event_v1
{
	struct recorded_animation_event_v1 event;
	word control_flags;
};

struct recorded_weapon_index_set_event_v1
{
	struct recorded_animation_event_v1 event;
	short weapon_index;
};

struct recorded_throttle_set_event_v1
{
	struct recorded_animation_event_v1 event;
	real_vector2d throttle;
};

struct recorded_multi_vector_set_event_v1
{
	struct recorded_animation_event_v1 event;
	real_vector3d vector;
};

struct recorded_angle_vector_set_event_v1
{
	struct recorded_animation_event_v1 event;
	real_euler_angles2d angles;
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

typedef void (*recorded_animation_apply_event_v1_proc)(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);

struct recorded_animation_playback_v1_data
{
	recorded_animation_apply_event_v1_proc apply_funcs[23];
	byte_swap_code animation_event_v1_codes[2];
	struct byte_swap_definition animation_event_v1_definition;
	byte_swap_code animation_state_set_event_v1_codes[1];
	struct byte_swap_definition animation_state_set_event_v1_definition;
	byte_swap_code aiming_speed_set_event_v1_codes[1];
	struct byte_swap_definition aiming_speed_set_event_v1_definition;
	byte_swap_code control_flags_set_event_v1_codes[1];
	struct byte_swap_definition control_flags_set_event_v1_definition;
	byte_swap_code weapon_index_set_event_v1_codes[1];
	struct byte_swap_definition weapon_index_set_event_v1_definition;
	byte_swap_code throttle_set_event_v1_codes[2];
	struct byte_swap_definition throttle_set_event_v1_definition;
	byte_swap_code multi_vector_set_event_v1_codes[3];
	struct byte_swap_definition multi_vector_set_event_v1_definition;
	byte_swap_code angle_vector_set_event_v1_codes[2];
	struct byte_swap_definition angle_vector_set_event_v1_definition;
};

/* ---------- prototypes */

void recorded_animation_initialize_unit_control(
	struct recorded_unit_control *unit_control,
	byte **stream,
	byte unit_control_data_version);
long recorded_animation_unit_control_size(
	byte unit_control_data_version);
static boolean recorded_animation_stream_damaged_v1(
	void);

static void apply_animation_state(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);
static void apply_aiming_speed(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);
static void apply_control_flags(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);
static void apply_weapon_index(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);
static void apply_throttle(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);
static void apply_facing_vector(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);
static void apply_aiming_vector(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);
static void apply_looking_vector(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);
static void apply_angle_vector(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);
static void apply_multi_vector(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *event,
	byte const **playback_stream);

/* ---------- globals */

static struct recorded_animation_playback_v1_data data_002dd030 =
{
	{
		NULL,
		NULL,
		apply_animation_state,
		apply_aiming_speed,
		apply_control_flags,
		apply_weapon_index,
		apply_throttle,
		NULL,
		NULL,
		apply_facing_vector,
		apply_aiming_vector,
		apply_looking_vector,
		apply_multi_vector,
		apply_multi_vector,
		apply_multi_vector,
		apply_multi_vector,
		apply_angle_vector,
		apply_angle_vector,
		apply_angle_vector,
		apply_angle_vector,
		apply_angle_vector,
		apply_angle_vector,
		apply_angle_vector,
	},
	{ _2byte, _2byte },
	{
		"animation_event_v1",
		sizeof(struct recorded_animation_event_v1),
		data_002dd030.animation_event_v1_codes,
		BYTE_SWAP_DEFINITION_SIGNATURE,
		FALSE,
	},
	{ _1byte },
	{
		"animation_state_set_event_v1",
		sizeof(struct recorded_animation_state_set_event_v1),
		data_002dd030.animation_state_set_event_v1_codes,
		BYTE_SWAP_DEFINITION_SIGNATURE,
		FALSE,
	},
	{ _1byte },
	{
		"aiming_speed_set_event_v1",
		sizeof(struct recorded_aiming_speed_set_event_v1),
		data_002dd030.aiming_speed_set_event_v1_codes,
		BYTE_SWAP_DEFINITION_SIGNATURE,
		FALSE,
	},
	{ _2byte },
	{
		"control_flags_set_event_v1",
		sizeof(struct recorded_control_flags_set_event_v1),
		data_002dd030.control_flags_set_event_v1_codes,
		BYTE_SWAP_DEFINITION_SIGNATURE,
		FALSE,
	},
	{ _2byte },
	{
		"weapon_index_set_event_v1",
		sizeof(struct recorded_weapon_index_set_event_v1),
		data_002dd030.weapon_index_set_event_v1_codes,
		BYTE_SWAP_DEFINITION_SIGNATURE,
		FALSE,
	},
	{ _4byte, _4byte },
	{
		"throttle_set_event_v1",
		sizeof(struct recorded_throttle_set_event_v1),
		data_002dd030.throttle_set_event_v1_codes,
		BYTE_SWAP_DEFINITION_SIGNATURE,
		FALSE,
	},
	{ _4byte, _4byte, _4byte },
	{
		"multi_vector_set_event_v1",
		sizeof(struct recorded_multi_vector_set_event_v1),
		data_002dd030.multi_vector_set_event_v1_codes,
		BYTE_SWAP_DEFINITION_SIGNATURE,
		FALSE,
	},
	{ _4byte, _4byte },
	{
		"angle_vector_set_event_v1",
		sizeof(struct recorded_angle_vector_set_event_v1),
		data_002dd030.angle_vector_set_event_v1_codes,
		BYTE_SWAP_DEFINITION_SIGNATURE,
		FALSE,
	},
};

#define apply_funcs data_002dd030.apply_funcs

/* port: the bytes each event takes, its header included (what its apply proc
reads, or the header alone), so an event is applied only when it is all in
the stream */
static byte const event_sizes_v1[NUMBEROF(apply_funcs)] =
{
	sizeof(struct recorded_animation_event_v1),
	sizeof(struct recorded_animation_event_v1),
	sizeof(struct recorded_animation_state_set_event_v1),
	sizeof(struct recorded_aiming_speed_set_event_v1),
	sizeof(struct recorded_control_flags_set_event_v1),
	sizeof(struct recorded_weapon_index_set_event_v1),
	sizeof(struct recorded_throttle_set_event_v1),
	sizeof(struct recorded_animation_event_v1),
	sizeof(struct recorded_animation_event_v1),
	sizeof(struct recorded_multi_vector_set_event_v1),
	sizeof(struct recorded_multi_vector_set_event_v1),
	sizeof(struct recorded_multi_vector_set_event_v1),
	sizeof(struct recorded_multi_vector_set_event_v1),
	sizeof(struct recorded_multi_vector_set_event_v1),
	sizeof(struct recorded_multi_vector_set_event_v1),
	sizeof(struct recorded_multi_vector_set_event_v1),
	sizeof(struct recorded_angle_vector_set_event_v1),
	sizeof(struct recorded_angle_vector_set_event_v1),
	sizeof(struct recorded_angle_vector_set_event_v1),
	sizeof(struct recorded_angle_vector_set_event_v1),
	sizeof(struct recorded_angle_vector_set_event_v1),
	sizeof(struct recorded_angle_vector_set_event_v1),
	sizeof(struct recorded_angle_vector_set_event_v1),
};

/* ---------- public code */

/* port: FALSE (and nothing read) when the unit control isn't inside the
stream: it can't be played */
boolean recorded_animation_initialize_event_stream_v1(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *unit_control,
	byte **playback_stream,
	byte unit_control_data_version,
	byte const *playback_stream_end)
{
	long unit_control_size = recorded_animation_unit_control_size(unit_control_data_version);

	if (unit_control_size == NONE ||
		playback_stream_end - *playback_stream < unit_control_size)
	{
		return FALSE;
	}

	recorded_animation_initialize_unit_control(
		unit_control,
		playback_stream,
		unit_control_data_version);

	return TRUE;
}

/* port: playback_stream_end is the stream's end: no read goes past it */
boolean recorded_animation_apply_event_stream_v1(
	struct animation_playback_controller *animation_state,
	struct recorded_unit_control *control,
	long *ticks,
	byte const **playback_stream,
	byte const *playback_stream_end)
{
	struct recorded_animation_event_v1 const *anim_event_v1;
	recorded_animation_apply_event_v1_proc apply;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0xA2, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0xA3, ticks);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0xA4, playback_stream);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0xA5, *playback_stream);

	/* port: an event header inside the stream */
	if (playback_stream_end - *playback_stream < (long)sizeof(*anim_event_v1))
		return recorded_animation_stream_damaged_v1();

	anim_event_v1 = (struct recorded_animation_event_v1 const *)*playback_stream;
	while (*ticks >= anim_event_v1->time_delta)
	{
		if (anim_event_v1->type == _playback_v1_end)
			break;

		/* port: an event the table has, all inside the stream, and the
		next event's header too */
		if (!VALID_INDEX(anim_event_v1->type, (short)NUMBEROF(apply_funcs)) ||
			playback_stream_end - *playback_stream < event_sizes_v1[anim_event_v1->type] + (long)sizeof(*anim_event_v1))
		{
			return recorded_animation_stream_damaged_v1();
		}

		apply = apply_funcs[anim_event_v1->type];
		if (apply)
		{
			apply(control, anim_event_v1, playback_stream);
		}
		else
		{
			*playback_stream = (byte const *)(anim_event_v1 + 1);
		}

		*ticks -= anim_event_v1->time_delta;
		anim_event_v1 = (struct recorded_animation_event_v1 const *)*playback_stream;
	}

	if (anim_event_v1->type == _playback_v1_end &&
		*ticks == anim_event_v1->time_delta)
	{
		return FALSE;
	}

	return TRUE;
}

void byte_swap_recording_stream_v1(
	void *stream,
	long stream_size,
	byte unit_control_data_version)
{
	return;
}

/* ---------- private code */

/* port: a stream that runs out before its end event, or holds an event the
table doesn't have, stops playing (a map's stream; said once) */
static boolean recorded_animation_stream_damaged_v1(
	void)
{
	static boolean reported = FALSE;

	if (!reported)
	{
		error(_error_silent, "a recorded animation's event stream is damaged (it stops playing)");
		reported = TRUE;
	}

	return FALSE;
}

static void apply_animation_state(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *anim_event_v1,
	byte const **playback_stream)
{
	struct recorded_animation_state_set_event_v1 const *event =
		(struct recorded_animation_state_set_event_v1 const *)anim_event_v1;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x19, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x19, anim_event_v1);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x19, anim_event_v1->type==_playback_v1_animation_state_set);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x19, playback_stream);

	control->byte_field0 = event->animation_state;
	*playback_stream += sizeof(*event);

	return;
}

static void apply_aiming_speed(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *anim_event_v1,
	byte const **playback_stream)
{
	struct recorded_aiming_speed_set_event_v1 const *event =
		(struct recorded_aiming_speed_set_event_v1 const *)anim_event_v1;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1A, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1A, anim_event_v1);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1A, anim_event_v1->type==_playback_v1_aiming_speed_set);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1A, playback_stream);

	control->byte_field1 = event->aiming_speed;
	*playback_stream += sizeof(*event);

	return;
}

static void apply_control_flags(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *anim_event_v1,
	byte const **playback_stream)
{
	struct recorded_control_flags_set_event_v1 const *event =
		(struct recorded_control_flags_set_event_v1 const *)anim_event_v1;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1B, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1B, anim_event_v1);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1B, anim_event_v1->type==_playback_v1_control_flags_set);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1B, playback_stream);

	control->word_field2 = event->control_flags;
	*playback_stream += sizeof(*event);

	return;
}

static void apply_weapon_index(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *anim_event_v1,
	byte const **playback_stream)
{
	struct recorded_weapon_index_set_event_v1 const *event =
		(struct recorded_weapon_index_set_event_v1 const *)anim_event_v1;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1C, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1C, anim_event_v1);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1C, anim_event_v1->type==_playback_v1_weapon_index_set);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x1C, playback_stream);

	control->word_field4 = event->weapon_index;
	*playback_stream += sizeof(*event);

	return;
}

static void apply_throttle(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *anim_event_v1,
	byte const **playback_stream)
{
	struct recorded_throttle_set_event_v1 const *event =
		(struct recorded_throttle_set_event_v1 const *)anim_event_v1;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x21, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x22, anim_event_v1);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x23, anim_event_v1->type==_playback_v1_throttle_set);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x24, playback_stream);

	control->vector2d_field12 = event->throttle;
	control->long_field20 = 0;
	*playback_stream += sizeof(*event);

	return;
}

static void apply_facing_vector(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *anim_event_v1,
	byte const **playback_stream)
{
	struct recorded_multi_vector_set_event_v1 const *event =
		(struct recorded_multi_vector_set_event_v1 const *)anim_event_v1;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2C, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2C, anim_event_v1);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2C, anim_event_v1->type==_playback_v1_facing_vector_set);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2C, playback_stream);

	control->vector3d_field28 = event->vector;
	*playback_stream += sizeof(*event);

	return;
}

static void apply_aiming_vector(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *anim_event_v1,
	byte const **playback_stream)
{
	struct recorded_multi_vector_set_event_v1 const *event =
		(struct recorded_multi_vector_set_event_v1 const *)anim_event_v1;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2D, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2D, anim_event_v1);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2D, anim_event_v1->type==_playback_v1_aiming_vector_set);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2D, playback_stream);

	control->vector3d_field40 = event->vector;
	*playback_stream += sizeof(*event);

	return;
}

static void apply_looking_vector(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *anim_event_v1,
	byte const **playback_stream)
{
	struct recorded_multi_vector_set_event_v1 const *event =
		(struct recorded_multi_vector_set_event_v1 const *)anim_event_v1;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2E, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2E, anim_event_v1);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2E, anim_event_v1->type==_playback_v1_looking_vector_set);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x2E, playback_stream);

	control->vector3d_field52 = event->vector;
	*playback_stream += sizeof(*event);

	return;
}

static void apply_angle_vector(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *anim_event_v1,
	byte const **playback_stream)
{
	struct recorded_angle_vector_set_event_v1 const *event =
		(struct recorded_angle_vector_set_event_v1 const *)anim_event_v1;
	real_vector3d angle_vector;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x38, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x39, anim_event_v1);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x3A, anim_event_v1->type>=_playback_v1_facing_angles_set && anim_event_v1->type<=_playback_v1_facing_aiming_looking_angles_set);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x3B, playback_stream);

	vector3d_from_euler_angles2d(&angle_vector, &event->angles);
	if (anim_event_v1->type != _playback_v1_aiming_looking_angles_set)
	{
		control->vector3d_field28 = angle_vector;
	}

	if (anim_event_v1->type != _playback_v1_facing_looking_angles_set)
	{
		control->vector3d_field40 = angle_vector;
	}

	if (anim_event_v1->type != _playback_v1_facing_aiming_angles_set)
	{
		control->vector3d_field52 = angle_vector;
	}

	*playback_stream += sizeof(*event);

	return;
}

static void apply_multi_vector(
	struct recorded_unit_control *control,
	struct recorded_animation_event_v1 const *anim_event_v1,
	byte const **playback_stream)
{
	struct recorded_multi_vector_set_event_v1 const *event =
		(struct recorded_multi_vector_set_event_v1 const *)anim_event_v1;

	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x56, control);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x57, anim_event_v1);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x58, anim_event_v1->type>=_playback_v1_facing_aiming_vector_set&&anim_event_v1->type<=_playback_v1_facing_aiming_looking_vector_set);
	match_assert("c:\\halo\\SOURCE\\cutscene\\recorded_animation_playback_v1.c", 0x59, playback_stream);

	if (anim_event_v1->type != _playback_v1_aiming_looking_vector_set)
	{
		control->vector3d_field28 = event->vector;
	}

	if (anim_event_v1->type != _playback_v1_facing_looking_vector_set)
	{
		control->vector3d_field40 = event->vector;
	}

	if (anim_event_v1->type != _playback_v1_facing_aiming_vector_set)
	{
		control->vector3d_field52 = event->vector;
	}

	*playback_stream += sizeof(*event);

	return;
}
