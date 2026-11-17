/* ---------- headers */

#include "cseries.h"
#include "cutscene/recorded_animation_definitions.h"
#include "cutscene/recorded_animation_playback.h"
#include "cutscene/recorded_animation_playback_v1.h"
#include "math/real_math.h"
#include "scenario/scenario_definitions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

static void byte_swap_recording(
	struct recorded_animation_definition const *animation,
	void *stream,
	long stream_size);

/* ---------- globals */

struct recorded_animation_event_stream_definition
{
	struct tag_data_definition data;
	struct tag_field fields[10];
};

struct recorded_animation_event_stream_definition recorded_animation_event_stream_data =
{
	{
		"recorded_animation_event_stream_data",
		0,
		0x200000,
		byte_swap_recording,
	},
	{
		{ _tag_field_string, 0, "name^", NULL },
		{ _tag_field_char_integer, 0, "version*", NULL },
		{ _tag_field_char_integer, 0, "raw animation data*", NULL },
		{ _tag_field_char_integer, 0, "unit control data version*", NULL },
		{ _tag_field_pad, 0, NULL, (void *)1 },
		{ _tag_field_short_integer, 0, "length of animation*:ticks", NULL },
		{ _tag_field_pad, 0, NULL, (void *)2 },
		{ _tag_field_pad, 0, NULL, (void *)4 },
		{ _tag_field_data, 0, "recorded animation event stream*", &recorded_animation_event_stream_data.data },
		{ _tag_field_terminator, 0, NULL, NULL },
	},
};

struct tag_block_definition recorded_animation_block =
{
	"recorded_animation_block",
	0,
	1024,
	sizeof(struct recorded_animation_definition),
	NULL,
	recorded_animation_event_stream_data.fields,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

/* ---------- public code */

short scenario_get_animation_by_name(struct scenario const *scenario, char const *name)
{
	short animation_index;
	short result = NONE;

	/* port: no more than the short counter reaches (a map's count; past it
	the counter wraps and the loop never ends) */
	for (animation_index = 0; animation_index < MIN(scenario->recorded_animations.count, SHORT_MAX); animation_index++)
	{
		struct recorded_animation_definition const *animation = TAG_BLOCK_GET_ELEMENT(
			&scenario->recorded_animations,
			animation_index,
			struct recorded_animation_definition);

		if (!_stricmp(animation->name, name))
		{
			result = animation_index;
			break;
		}
	}

	return result;
}

/* ---------- private code */

static void byte_swap_recording(
	struct recorded_animation_definition const *animation,
	void *stream,
	long stream_size)
{
	if (animation->version > 0)
	{
		if (animation->version > 3)
		{
			if (animation->version == 4)
				byte_swap_recording_stream(stream, stream_size, animation->unit_control_data_version);
		}
		else
		{
			byte_swap_recording_stream_v1(stream, stream_size, animation->unit_control_data_version);
		}
	}
}
