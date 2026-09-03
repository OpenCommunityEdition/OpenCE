/*
UNIT_DIALOGUE.C

symbols in this file:
001961B0 02f0:
	_unit_test_speech (0000)
001964A0 0030:
	_unit_is_speaking (0000)
001964D0 00b0:
	_unit_dialogue_determine_variant (0000)
00196580 0030:
	_unit_get_speech_priority_name (0000)
001965B0 0040:
	_unit_get_speech_priority_by_name (0000)
001965F0 0110:
	_unit_describe_speech (0000)
00196700 00d0:
	_code_00196700 (0000)
001967D0 01e0:
	_unit_speak (0000)
001969B0 00f0:
	_unit_notify_impulse_sound (0000)
00196AA0 0310:
	_unit_make_damage_sound (0000)
00196DB0 0180:
	_unit_scream (0000)
00196F30 00e0:
	_code_00196f30 (0000)
00197010 0060:
	_code_00197010 (0000)
00197070 0300:
	_unit_dialogue_update (0000)
002A4AF4 0044:
	_global_speech_override_priorities (0000)
	_global_speech_queue_times (0018)
002A4B38 0007:
	??_C@_06CHNPJHKA@scream?$AA@ (0000)
002A4B40 0005:
	??_C@_04PPONMOMN@pain?$AA@ (0000)
002A4B48 004f:
	??_C@_0EP@HLLDDEB@?$CIvocalization_type?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIvoc@ (0000)
002A4B98 0041:
	??_C@_0EB@BEHAGLIC@?$CIpriority?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIpriority?5?$DM?5N@ (0000)
002A4BDC 0021:
	??_C@_0CB@KLBJPFCB@sound_definition_index_reference@ (0000)
002A4C00 001c:
	??_C@_0BM@JBKHEHDK@vocalization_type_reference?$AA@ (0000)
002A4C1C 0025:
	??_C@_0CF@GDJDFIKG@c?3?2halo?2SOURCE?2units?2unit_dialog@ (0000)
002A4C44 0039:
	??_C@_0DJ@FLLOGMMH@unit_dialogue_determine_variant?5@ (0000)
002A4C80 0016:
	??_C@_0BG@NPMCLPNN@?$CFs?3?5lost?5?$CFs?5speech?5?$CFs?$AA@ (0000)
002A4C98 0007:
	??_C@_06DBNNGIJH@queued?$AA@ (0000)
002A4CA0 0008:
	??_C@_07BMPLEEBA@waiting?$AA@ (0000)
002A4CA8 000c:
	??_C@_0M@PNDDBCJG@speech_item?$AA@ (0000)
002A4CB4 0037:
	??_C@_0DH@LMOPNAFJ@unit?9?$DOunit?4speech?4current?4priori@ (0000)
002A4CEC 0021:
	??_C@_0CB@FIDKMHLI@AI_BEHAVIOR?$CIforce_vocalizations?$CJ@ (0000)
002A4D10 004b:
	??_C@_0EL@PILBDJLD@unit?9?$DOunit?4speech?4current?4sound_@ (0000)
002A4D60 0042:
	??_C@_0EC@EADCCPEN@?$CIscream_type?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIscream_ty@ (0000)
002A4DA4 0028:
	??_C@_0CI@HNAKBIJH@dialogue_definition_get?$CIdialogue@ (0000)
00315BD8 002c:
	_global_speech_priority_names (0000)
004CCB14 0004:
	_bss_004ccb14 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "units.h"

#include "ai/ai_communication.h"
#include "dialogue_definitions.h"
#include "sound/game_sound.h"
#include "unit_definitions.h"

/* ---------- constants */

enum unit_play_speech_type
{
	_unit_play_speech_none = 0,
	_unit_play_speech_queue,
	_unit_play_speech_immediate,
	_unit_play_speech_immediate_dequeue,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

static long unit_find_dialogue_variant(
	struct unit_definition *definition,
	short variant_number);
static void unit_dialogue_setup(
	long unit_index);

/* ---------- globals */

char const *global_speech_priority_names[NUMBER_OF_UNIT_SPEECH_PRIORITIES] =
{
	"none",
	"idle",
	"pain",
	"talk",
	"communicate",
	"shout",
	"script",
	"involuntary",
	"exclaim",
	"scream",
	"death",
};

long sequential_counter;

/* ---------- public code */

boolean unit_is_speaking(
	long unit_index)
{
	return unit_get(unit_index)->unit.speech.current.priority > 0;
}

void unit_dialogue_determine_variant(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	struct unit_definition *definition = unit_definition_get(unit->definition_index);

	if (unit->object.variant_number == 0)
	{
		short variants[16];
		short variant_count = 0;
		short variant_index;

		for (variant_index = 0; variant_index < definition->unit.dialogue_variants.count; variant_index++)
		{
			struct unit_dialogue_variant *variant = TAG_BLOCK_GET_ELEMENT(
				&definition->unit.dialogue_variants,
				variant_index,
				struct unit_dialogue_variant);

			if (variant->variant_number < 100)
			{
				if (variant_count >= NUMBEROF(variants))
				{
					error(_error_silent, "unit_dialogue_determine_variant overflowed variant array");
					break;
				}

				variants[variant_count++] = variant->variant_number;
			}
		}

		if (variant_count > 0)
		{
			unit->object.variant_number = variants[sequential_counter % variant_count];
			sequential_counter++;
		}
	}

	return;
}

char const *unit_get_speech_priority_name(
	short priority)
{
	char const *name = "<error>";

	if (priority >= _unit_speech_none && priority < NUMBER_OF_UNIT_SPEECH_PRIORITIES)
		name = global_speech_priority_names[priority];

	return name;
}

short unit_get_speech_priority_by_name(
	char const *name)
{
	short result = _unit_speech_none;
	short priority;

	for (priority = _unit_speech_none; priority < NUMBER_OF_UNIT_SPEECH_PRIORITIES; priority++)
	{
		if (csstrcmp(global_speech_priority_names[priority], name) == 0)
		{
			result = priority;
			break;
		}
	}

	return result;
}

void unit_dialogue_update(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	struct unit_speech *speech = &unit->unit.speech;

	if (TEST_FLAG(unit->unit.flags, _unit_must_set_up_dialogue_bit))
	{
		unit_dialogue_setup(unit_index);
		SET_FLAG(unit->unit.flags, _unit_must_set_up_dialogue_bit, FALSE);
	}

	if (speech->damage_minor_decay_timer > 0 && --speech->damage_minor_decay_timer == 0)
	{
		if (speech->damage_minor_sounds > 0)
		{
			speech->damage_minor_sounds--;
			speech->damage_minor_decay_timer = 22;
		}
	}

	if (speech->damage_minor_timer > 0)
		speech->damage_minor_timer--;
	if (speech->damage_minor_timer > 0)
		speech->damage_minor_timer--;

	if (speech->current.priority > _unit_speech_none)
	{
		if (speech->pre_delay_timer > 0)
		{
			speech->pre_delay_timer--;
		}
		else
		{
			if (!speech->played)
			{
				struct object_marker marker;
				real_point3d position;
				real_vector3d forward;
				short node_index;

				if (object_get_marker_by_name(unit_index, "head", &marker, 1))
				{
					node_index = marker.node_index;
					position = marker.node_matrix.position;
					forward = marker.node_matrix.forward;
				}
				else
				{
					node_index = 0;
					position = *global_origin3d;
					forward = *global_forward3d;
				}

				if (speech->current.sound_definition_index != NONE)
				{
					speech->impulse_sound_index = object_impulse_sound_new(
						unit_index,
						speech->current.sound_definition_index,
						node_index,
						&position,
						&forward,
						1.0f);
				}

				ai_communication_started(
					unit_index,
					speech->current.priority,
					speech->current.vocalization_type,
					&speech->current.ai);
				speech->played = TRUE;
			}

			if (speech->ai_delay_timer > 0)
				speech->ai_delay_timer--;

			if (speech->sound_timer > 0)
			{
				match_assert(
					"c:\\halo\\SOURCE\\units\\unit_dialogue.c",
					757,
					speech->current.priority > _unit_speech_none);
				if (--speech->sound_timer == 0)
					speech->impulse_sound_index = NONE;
			}
			else
			{
				if (!speech->finished)
				{
					ai_communication_finished(
						unit_index,
						speech->current.priority,
						speech->current.vocalization_type,
						FALSE,
						NONE,
						&speech->current.ai);
					speech->finished = TRUE;
				}

				if (speech->post_delay_timer > 0)
					speech->post_delay_timer--;
				if (speech->post_delay_timer == 0)
					speech->ai_delay_timer = 0;
			}
		}
	}

	if (speech->ai_delay_timer == 0 && !speech->notified_ai)
	{
		ai_communication_notify(
			unit_index,
			speech->current.priority,
			speech->current.vocalization_type,
			&speech->current.ai);
		speech->notified_ai = TRUE;
	}

	if (speech->current.priority > _unit_speech_none &&
		speech->sound_timer == 0 &&
		speech->post_delay_timer == 0)
	{
		speech->current.priority = _unit_speech_none;
	}

	if (speech->current.priority == _unit_speech_none &&
		speech->queued.priority > _unit_speech_none)
	{
		unit_speak(unit_index, _unit_play_speech_immediate_dequeue, &speech->queued);
	}

	return;
}

/* ---------- private code */

static long unit_find_dialogue_variant(
	struct unit_definition *definition,
	short variant_number)
{
	short variant_indices[16];
	long dialogue_index = NONE;
	short variant_count = 0;
	short variant_index;

	for (variant_index = 0; variant_index < definition->unit.dialogue_variants.count; variant_index++)
	{
		struct unit_dialogue_variant *variant = TAG_BLOCK_GET_ELEMENT(
			&definition->unit.dialogue_variants,
			variant_index,
			struct unit_dialogue_variant);

		if (variant_number == NONE || variant->variant_number == variant_number)
			variant_indices[variant_count++] = variant_index;
	}

	if (variant_count > 0)
	{
		short selected_variant_index;
		struct unit_dialogue_variant *selected_variant;

		if (variant_count == 1)
			selected_variant_index = variant_indices[0];
		else
		{
			selected_variant_index = variant_indices[seed_random_range(
				get_global_random_seed_address(),
				0,
				variant_count)];
		}

		selected_variant = TAG_BLOCK_GET_ELEMENT(
			&definition->unit.dialogue_variants,
			selected_variant_index,
			struct unit_dialogue_variant);
		dialogue_index = selected_variant->dialogue_index;
		match_assert(
			"c:\\halo\\SOURCE\\units\\unit_dialogue.c",
			1032,
			dialogue_definition_get(dialogue_index));

	}

	return dialogue_index;
}

static void unit_dialogue_setup(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	struct unit_definition *definition = unit_definition_get(unit->definition_index);
	short variant_number = unit->object.variant_number;
	long dialogue_index = NONE;

	if (variant_number > 0)
		dialogue_index = unit_find_dialogue_variant(definition, variant_number);

	if (dialogue_index == NONE)
	{
		dialogue_index = unit_find_dialogue_variant(definition, 0);
		if (dialogue_index == NONE)
			dialogue_index = unit_find_dialogue_variant(definition, NONE);
	}

	unit->unit.dialogue_index = dialogue_index;

	return;
}
