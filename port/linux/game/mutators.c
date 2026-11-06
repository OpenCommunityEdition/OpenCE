/*
MUTATORS.C

The mutators a game plays: its tag edits and poses (mutator_tags.c,
object_pose.c), its add-ons' objects (addons.c) and its scripts
(mutator_scripts.c), each from a mutator's files (port/linux/src/port_mutators.c,
docs/mutators.md). The game has no rules of a mutator's built in: a mutator is
data, and what it changes is general (a node of a model scaled, a field of a
tag set), so that players make mutators of their own with no change to the
game. A key or value this build does not have is refused.

The host, and a game of one machine, plays the mutators turned on when its
map loads; a client plays the host's (network_mutators.c), never its own,
so that every machine of a network game plays the same rules. A client
plays none until the host's come with the host's first answer to it.

The host's keys name its scripts ("script." and the first digits of its
mutator's digest), which each machine runs from its own copy. A key is a name,
not a number, so that builds of other forks which add keys of their own never
take one for another; one this build does not have is passed over and said.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "game/game.h"
#include "main/console.h"
#include "networking/network_game_globals.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"

#include "mutators.h"
#include "mutator_scripts.h"

#include <stdio.h>
#include <string.h>

/* ---------- constants */

/* the most keys port_mutators.c gives (port_mutators.c's, which declares it
for itself) */
#define MAXIMUM_PORT_MUTATOR_KEYS 64

/* a script's key: this, and the first digits of its mutator's digest */
#define MUTATOR_SCRIPT_KEY_PREFIX "script."

/* ---------- globals */

static struct
{
	/* the scripts played (mutator_scripts.c runs them) */
	short script_count;
	/* the tag edits played (mutator_tags.c) */
	byte tag_edits[MAXIMUM_MUTATOR_TAG_EDIT_BYTES];
	long tag_edit_size;
} mutators_globals;

/* ---------- prototypes/PORT_MUTATORS.C (port_mutators.h's) */

long port_mutators_keys(char keys[][MUTATOR_KEY_LENGTH], double *values, long maximum);
long port_mutators_tag_edits(unsigned char *edits, long maximum);

/* ---------- private code */

static boolean mutators_script_key(char const *key)
{
	return !strncmp(key, MUTATOR_SCRIPT_KEY_PREFIX, sizeof(MUTATOR_SCRIPT_KEY_PREFIX) - 1);
}

/* a key's text: lower case letters, digits and underscores, and one dot
between two names, ended within MUTATOR_KEY_LENGTH */
static boolean mutators_key_text_valid(char const *key)
{
	short length;
	short dots = 0;

	for (length = 0; length < MUTATOR_KEY_LENGTH && key[length]; length++)
	{
		char character = key[length];

		if (character == '.')
		{
			if (!length || key[length - 1] == '.' || ++dots > 1)
				return FALSE;
		}
		else if (!((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '_'))
		{
			return FALSE;
		}
	}
	return length > 0 && length < MUTATOR_KEY_LENGTH && key[length - 1] != '.';
}

static void mutators_say(char const *whose)
{
	char keys[MAXIMUM_MUTATOR_KEYS][MUTATOR_KEY_LENGTH];
	real values[MAXIMUM_MUTATOR_KEYS];
	char text[256];
	short count = mutators_keys(keys, values, MAXIMUM_MUTATOR_KEYS);
	short index;

	if (!count)
		return;
	snprintf(text, sizeof(text), "%s mutators:", whose);
	for (index = 0; index < count; index++)
	{
		size_t length = strlen(text);

		snprintf(text + length, sizeof(text) - length, " %s", keys[index]);
	}
	error(_error_log, "%s", text);
	console_printf(FALSE, "%s", text);
}

/* ---------- public code */

void mutators_new_game(
	void)
{
	char keys[MAXIMUM_PORT_MUTATOR_KEYS][MUTATOR_KEY_LENGTH];
	double values[MAXIMUM_PORT_MUTATOR_KEYS];
	long count;
	long index;

	mutators_globals.script_count = 0;
	mutator_scripts_begin();
	mutator_tags_undo();
	mutators_globals.tag_edit_size = 0;
	/* (a client's are the host's: network_mutators.c; the main menu's map
	plays none) */
	if (game_connection() == _game_connection_network_client || global_scenario_get()->type == _scenario_type_main_menu)
		return;
	/* (port_mutators.c checked each of them with mutators_tag_edit_check) */
	mutators_globals.tag_edit_size = port_mutators_tag_edits(mutators_globals.tag_edits,
		MAXIMUM_MUTATOR_TAG_EDIT_BYTES);
	if (!mutator_tags_play(mutators_globals.tag_edits, mutators_globals.tag_edit_size))
	{
		error(_error_log, "mutators: the tag edits are not this build's: none played");
		mutators_globals.tag_edit_size = 0;
	}
	/* (port_mutators.c gives only its scripts' keys) */
	count = port_mutators_keys(keys, values, MAXIMUM_PORT_MUTATOR_KEYS);
	for (index = 0; index < count; index++)
	{
		if (!mutators_script_key(keys[index]))
			continue;
		mutator_scripts_add(keys[index]);
		mutators_globals.script_count++;
	}
	mutator_scripts_commit();
	mutators_say("This game's");
}

boolean mutators_active(
	void)
{
	return mutators_globals.script_count > 0 || mutators_globals.tag_edit_size > 0;
}

long mutators_tag_edits(
	byte const **edits)
{
	*edits = mutators_globals.tag_edits;
	return mutators_globals.tag_edit_size;
}

boolean mutators_set_host_tag_edits(
	byte const *edits,
	long size)
{
	if (size < 0 || size > MAXIMUM_MUTATOR_TAG_EDIT_BYTES)
		return FALSE;
	csmemcpy(mutators_globals.tag_edits, edits, size);
	if (!mutator_tags_play(mutators_globals.tag_edits, size))
	{
		mutators_globals.tag_edit_size = 0;
		return FALSE;
	}
	mutators_globals.tag_edit_size = size;
	return TRUE;
}

short mutators_keys(
	char keys[][MUTATOR_KEY_LENGTH],
	real *values,
	short maximum)
{
	short count = 0;
	short index;

	/* (a script's key says which: the client runs its own copy of it) */
	if (maximum > 0)
		count = mutator_scripts_keys(keys, maximum);
	for (index = 0; index < count; index++)
		values[index] = 1.f;
	return count;
}

boolean mutators_set_host(
	char const keys[][MUTATOR_KEY_LENGTH],
	real const *values,
	short count)
{
	short index;

	/* (all of them checked before any is played) */
	for (index = 0; index < count; index++)
	{
		/* (not a NaN, not infinite) */
		if (!mutators_key_text_valid(keys[index]) || !(values[index] == values[index] &&
			values[index] - values[index] == 0.f))
		{
			return FALSE;
		}
	}
	mutators_globals.script_count = 0;
	mutator_scripts_begin();
	for (index = 0; index < count; index++)
	{
		if (!mutators_script_key(keys[index]))
		{
			console_warning("The host plays the mutator %s, which this game does not have", keys[index]);
			error(_error_log, "mutators: the host's %s is not this build's: passed over", keys[index]);
			continue;
		}
		mutator_scripts_add(keys[index]);
		mutators_globals.script_count++;
	}
	mutator_scripts_commit();
	mutators_say("The host's");
	return TRUE;
}

int mutators_key_check(
	char const *key,
	double value,
	char *reason,
	unsigned long size)
{
	(void)value;
	if (reason && size)
		reason[0] = 0;
	if (!key || !mutators_key_text_valid(key))
	{
		if (reason)
			snprintf(reason, size, "%s is not an effect's name", key ? key : "(nothing)");
		return FALSE;
	}
	/* (the game has no effects built in: a big head is a [[pose]]) */
	if (reason)
	{
		snprintf(reason, size, "this game has no effect %s%s", key,
			strncmp(key, "big_head", 8) ? "" : " (a big head is a [[pose]] now)");
	}
	return FALSE;
}
