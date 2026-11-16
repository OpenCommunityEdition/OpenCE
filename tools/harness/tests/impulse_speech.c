/*
IMPULSE_SPEECH.C (test)

The real unit_notify_impulse_sound (unit_dialogue.c): a script's line
played from a unit (sound_impulse_start) is made its current speech
(unit_speak) and marked played as that line. A dead unit takes no line but
its death's, and the original then asserted that the line was its current
speech, halting a debug build: c10's flashback plays lines of Johnson and
Bisenti once they are dead. A dead unit's current speech is left as it was;
a live unit's is the line, as before.
test_impulse_speech.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#undef match_assert
static int test_assertions_failed;
#define match_assert(file, line, condition) ((void)((condition) || (test_assertions_failed++, 0)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define csmemset memset

enum
{
	_unit_speech_none = 0,
	_unit_speech_idle,
	_unit_speech_scripted = 6,
	_unit_speech_death = 9
};

enum
{
	_unit_play_speech_queue = 1,
	_unit_play_speech_immediate = 2
};

enum
{
	_object_dead_bit = 3
};

struct ai_communication_packet
{
	long type;
};

struct unit_speech_item
{
	short priority;
	short vocalization_type;
	long sound_definition_index;
	short pause_time;
	struct ai_communication_packet ai;
};

struct unit_datum
{
	struct
	{
		unsigned long damage_flags;
	} object;
	struct
	{
		struct
		{
			struct unit_speech_item current;
			long impulse_sound_index;
			boolean played;
			short pre_delay_timer;
		} speech;
	} unit;
};

static struct unit_datum test_unit;
static int test_communications_started;

static struct unit_datum *unit_get(long unit_index)
{
	(void)unit_index;
	return &test_unit;
}

static short unit_test_speech(long unit_index, short priority, boolean a, boolean b, void *c, short *vocalization_type,
	long *sound_definition_index)
{
	(void)unit_index;
	(void)priority;
	(void)a;
	(void)b;
	(void)c;
	(void)vocalization_type;
	(void)sound_definition_index;
	return _unit_play_speech_queue;
}

static void ai_communication_packet_new(struct ai_communication_packet *packet)
{
	packet->type = 0;
}

/* as the original: a dead unit takes nothing but its death's speech */
static void unit_speak(long unit_index, short play_type, struct unit_speech_item const *speech_item)
{
	struct unit_datum *unit = unit_get(unit_index);

	if (TEST_FLAG(unit->object.damage_flags, _object_dead_bit) && speech_item->priority != _unit_speech_death)
		return;
	if (play_type >= _unit_play_speech_immediate)
	{
		unit->unit.speech.current = *speech_item;
		unit->unit.speech.played = FALSE;
		unit->unit.speech.impulse_sound_index = NONE;
	}
}

static void ai_communication_started(long unit_index, short priority, long a, struct ai_communication_packet *packet)
{
	(void)unit_index;
	(void)priority;
	(void)a;
	(void)packet;
	test_communications_started++;
}

#include "under_test.inc"

/* a unit whose current speech is its death's (dead) or nothing (alive) */
static void unit_reset(boolean dead)
{
	memset(&test_unit, 0, sizeof(test_unit));
	test_unit.object.damage_flags = dead ? FLAG(_object_dead_bit) : 0;
	test_unit.unit.speech.current.priority = dead ? _unit_speech_death : _unit_speech_none;
	test_unit.unit.speech.current.sound_definition_index = dead ? 77 : NONE;
	test_unit.unit.speech.impulse_sound_index = dead ? 5 : NONE;
	test_unit.unit.speech.played = TRUE;
	test_unit.unit.speech.pre_delay_timer = 9;
	test_assertions_failed = 0;
	test_communications_started = 0;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a live unit: the line is its current speech, played */
	CASE("alive")
	{
		unit_reset(FALSE);
		unit_notify_impulse_sound(1, 42, 3);
		CHECK(!test_assertions_failed, "%d assertions failed", test_assertions_failed);
		CHECK(test_unit.unit.speech.current.sound_definition_index == 42, "current speech %ld",
			test_unit.unit.speech.current.sound_definition_index);
		CHECK(test_unit.unit.speech.impulse_sound_index == 3 && test_unit.unit.speech.played &&
			!test_unit.unit.speech.pre_delay_timer, "not marked played as the line");
		CHECK(test_communications_started == 1, "%d communications", test_communications_started);
		return 0;
	}
	/* a dead unit: no assertion, its death's speech left as it was */
	CASE("dead")
	{
		unit_reset(TRUE);
		unit_notify_impulse_sound(1, 42, 3);
		CHECK(!test_assertions_failed, "%d assertions failed", test_assertions_failed);
		CHECK(test_unit.unit.speech.current.sound_definition_index == 77 &&
			test_unit.unit.speech.current.priority == _unit_speech_death, "its death's speech replaced");
		CHECK(test_unit.unit.speech.impulse_sound_index == 5 && test_unit.unit.speech.pre_delay_timer == 9,
			"its speech marked as the line");
		CHECK(!test_communications_started, "%d communications", test_communications_started);
		return 0;
	}

	return 2;
}
