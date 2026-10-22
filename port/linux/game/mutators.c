/*
MUTATORS.C

The mutators a game plays: effects built into the game that change its
rules (a big head on every biped), each turned on by a mutator and set by
its parameters. A mutator is only data: a mutator.toml naming the effects
it turns on and their parameters' values (port/linux/src/port_mutators.c,
docs/mutators.md). It runs nothing of its own, and a key or value this
build does not have is refused, so a mutator can do no more than the
effects built in here.

The host, and a game of one machine, plays the mutators turned on when its
map loads; a client plays the host's (network_mutators.c), never its own,
so that every machine of a network game plays the same rules. A client
plays none until the host's come with the host's first answer to it.

Each effect's key is its name ("big_head"), and each parameter's is that
and its own ("big_head.scale"): names, not numbers, so that builds of other
forks which add effects of their own never take one for another.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "game/game.h"
#include "main/console.h"
#include "networking/network_game_globals.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"

#include "mutators.h"

#include <stdio.h>
#include <string.h>

/* ---------- constants */

/* the most parameters a mutator.toml lists (port_mutators.c's, which
declares it for itself) */
#define MAXIMUM_PORT_MUTATOR_KEYS 64

/* ---------- structures */

struct mutator_parameter_definition
{
	short effect;
	char const *name;
	real minimum;
	real maximum;
	real default_value;
};

/* ---------- globals */

static char const *const mutator_effect_names[NUMBER_OF_MUTATOR_EFFECTS] =
{
	"big_head",
};

static struct mutator_parameter_definition const mutator_parameter_definitions[NUMBER_OF_MUTATOR_PARAMETERS] =
{
	/* how many times its size a biped's head is drawn and hit */
	{ _mutator_effect_big_head, "scale", 1.f, 4.f, 2.5f },
};

static struct
{
	boolean effects[NUMBER_OF_MUTATOR_EFFECTS];
	real parameters[NUMBER_OF_MUTATOR_PARAMETERS];
} mutators_globals;

/* ---------- prototypes/PORT_MUTATORS.C (port_mutators.h's) */

long port_mutators_keys(char keys[][MUTATOR_KEY_LENGTH], double *values, long maximum);

/* ---------- private code */

static void mutators_clear(void)
{
	short index;

	csmemset(mutators_globals.effects, 0, sizeof(mutators_globals.effects));
	for (index = 0; index < NUMBER_OF_MUTATOR_PARAMETERS; index++)
		mutators_globals.parameters[index] = mutator_parameter_definitions[index].default_value;
}

/* the effect `key` names ("big_head"), or NONE; *parameter its parameter
("big_head.scale"), or NONE for the effect's own key */
static short mutators_key_find(char const *key, short *parameter)
{
	char const *dot = strchr(key, '.');
	size_t length = dot ? (size_t)(dot - key) : strlen(key);
	short effect;
	short index;

	*parameter = NONE;
	for (effect = 0; effect < NUMBER_OF_MUTATOR_EFFECTS; effect++)
	{
		if (strlen(mutator_effect_names[effect]) == length && !strncmp(mutator_effect_names[effect], key, length))
			break;
	}
	if (effect == NUMBER_OF_MUTATOR_EFFECTS)
		return NONE;
	if (!dot)
		return effect;
	for (index = 0; index < NUMBER_OF_MUTATOR_PARAMETERS; index++)
	{
		if (mutator_parameter_definitions[index].effect == effect && !strcmp(mutator_parameter_definitions[index].name, dot + 1))
		{
			*parameter = index;
			return effect;
		}
	}
	return NONE;
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

static boolean mutators_value_valid(short parameter, real value)
{
	/* (not a NaN, not infinite) */
	if (!(value == value && value - value == 0.f))
		return FALSE;
	if (parameter == NONE)
		return value == 1.f;
	return value >= mutator_parameter_definitions[parameter].minimum &&
		value <= mutator_parameter_definitions[parameter].maximum;
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

		if (strchr(keys[index], '.'))
			snprintf(text + length, sizeof(text) - length, " %s=%g", strchr(keys[index], '.') + 1, values[index]);
		else
			snprintf(text + length, sizeof(text) - length, "%s %s", index ? "," : "", keys[index]);
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

	mutators_clear();
	mutator_big_head_new_game();
	/* (a client's are the host's: network_mutators.c; the main menu's map
	plays none) */
	if (game_connection() == _game_connection_network_client || global_scenario_get()->type == _scenario_type_main_menu)
		return;
	/* (port_mutators.c checked each of them with mutators_key_check) */
	count = port_mutators_keys(keys, values, MAXIMUM_PORT_MUTATOR_KEYS);
	for (index = 0; index < count; index++)
	{
		short parameter;
		short effect = mutators_key_find(keys[index], &parameter);

		if (effect == NONE || !mutators_value_valid(parameter, (real)values[index]))
			continue;
		mutators_globals.effects[effect] = TRUE;
		if (parameter != NONE)
			mutators_globals.parameters[parameter] = (real)values[index];
	}
	mutators_say("This game's");
}

boolean mutators_effect_active(
	short effect)
{
	return effect >= 0 && effect < NUMBER_OF_MUTATOR_EFFECTS && mutators_globals.effects[effect];
}

real mutators_parameter(
	short parameter)
{
	if (parameter < 0 || parameter >= NUMBER_OF_MUTATOR_PARAMETERS)
		return 0.f;
	return mutators_globals.parameters[parameter];
}

boolean mutators_active(
	void)
{
	short effect;

	for (effect = 0; effect < NUMBER_OF_MUTATOR_EFFECTS; effect++)
	{
		if (mutators_globals.effects[effect])
			return TRUE;
	}
	return FALSE;
}

short mutators_keys(
	char keys[][MUTATOR_KEY_LENGTH],
	real *values,
	short maximum)
{
	short count = 0;
	short effect;
	short parameter;

	for (effect = 0; effect < NUMBER_OF_MUTATOR_EFFECTS; effect++)
	{
		if (!mutators_globals.effects[effect] || count >= maximum)
			continue;
		snprintf(keys[count], MUTATOR_KEY_LENGTH, "%s", mutator_effect_names[effect]);
		values[count++] = 1.f;
		for (parameter = 0; parameter < NUMBER_OF_MUTATOR_PARAMETERS && count < maximum; parameter++)
		{
			if (mutator_parameter_definitions[parameter].effect != effect)
				continue;
			snprintf(keys[count], MUTATOR_KEY_LENGTH, "%s.%s", mutator_effect_names[effect],
				mutator_parameter_definitions[parameter].name);
			values[count++] = mutators_globals.parameters[parameter];
		}
	}
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
		short parameter;
		short effect;

		if (!mutators_key_text_valid(keys[index]))
			return FALSE;
		effect = mutators_key_find(keys[index], &parameter);
		if (effect != NONE && !mutators_value_valid(parameter, values[index]))
			return FALSE;
	}
	mutators_clear();
	for (index = 0; index < count; index++)
	{
		short parameter;
		short effect = mutators_key_find(keys[index], &parameter);

		if (effect == NONE)
		{
			console_warning("The host plays the mutator effect %s, which this game does not have", keys[index]);
			error(_error_log, "mutators: the host's %s is not this build's: passed over", keys[index]);
			continue;
		}
		mutators_globals.effects[effect] = TRUE;
		if (parameter != NONE)
			mutators_globals.parameters[parameter] = values[index];
	}
	mutators_say("The host's");
	return TRUE;
}

int mutators_key_check(
	char const *key,
	double value,
	char *reason,
	unsigned long size)
{
	short parameter;
	short effect;

	if (reason && size)
		reason[0] = 0;
	if (!key || !mutators_key_text_valid(key))
	{
		if (reason)
			snprintf(reason, size, "%s is not an effect's name", key ? key : "(nothing)");
		return FALSE;
	}
	effect = mutators_key_find(key, &parameter);
	if (effect == NONE)
	{
		if (reason)
			snprintf(reason, size, "this game has no %s %s", strchr(key, '.') ? "parameter" : "effect", key);
		return FALSE;
	}
	if (!mutators_value_valid(parameter, (real)value))
	{
		if (reason && parameter != NONE)
		{
			snprintf(reason, size, "%s must be from %g to %g", key, mutator_parameter_definitions[parameter].minimum,
				mutator_parameter_definitions[parameter].maximum);
		}
		else if (reason)
		{
			snprintf(reason, size, "%s is not a number", key);
		}
		return FALSE;
	}
	return TRUE;
}
