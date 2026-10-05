/*
MODEL_LOD.C

The models' level of detail, forced, for the console and the video settings
(port/linux, port/android, port/windows).

Every model's permutations have five geometries, from super low detail to
super high, and the game draws the one the model's size on the screen calls
for (models.c's render_model: the level grows with the pixels the model
covers). The console's rasterizer_debug_model_lod global forces one of them
for every model; this file adds the port's two other ways of forcing one:
the console's debug_lod command (hs.c), and the video settings' MODEL DETAIL
(display.model_lod, port_config.c), which forces one whenever no command
has.

A level is the index models.c selects among a permutation's geometry
indices: 0 is the super low geometry and 4 the super high, which is the
order the game's tag definition has them in (so are the detail node counts;
the detail cutoff pixels are the other way, super high first).
*/

#include "cseries.h"
#include "main/console.h"
#include "rasterizer/rasterizer_console_vars.h"

#include <stdlib.h>
#include <string.h>

/* port/linux/src/port_config.c */
const char *config_string(const char *name);
unsigned long config_changes(void);

/* ---------- constants */

/* models.c's NUMBER_OF_DETAIL_LEVELS_PER_MODEL: the geometries a model tag
has, super low first */
#define NUMBER_OF_MODEL_DETAIL_LEVELS 5

/* ---------- public code */

/* the video settings' MODEL DETAIL, as a level to force every model to, or
NONE for the game's own choice (models.c's render_model) */
long port_model_lod(
	void)
{
	static long level = NONE;
	static unsigned long read_at = (unsigned long)-1;
	const char *setting;

	if (read_at != config_changes())
	{
		read_at = config_changes();
		setting = config_string("display.model_lod");
		if (!strcmp(setting, "super_high"))
			level = 4;
		else if (!strcmp(setting, "high"))
			level = 3;
		else if (!strcmp(setting, "medium"))
			level = 2;
		else if (!strcmp(setting, "low"))
			level = 1;
		else if (!strcmp(setting, "super_low"))
			level = 0;
		else
			level = NONE;
	}
	return level;
}

/* the console's debug_lod command (hs.c, as the host's ban and kick are):
"debug_lod" says what the level is now, "debug_lod <level>" forces one, and
-1 (or "auto", "off", "none") gives the models back. TRUE when the
expression is that command, which has then answered, whatever the level
was */
boolean port_debug_lod_command(
	char const *expression)
{
	static char const *const word = "debug_lod";
	static char const *const names[] = { "super low", "low", "medium", "high", "super high" };
	static char const *const automatic[] = { "-1", "auto", "off", "none" };
	char const *text = expression;
	char argument[16];
	long index;
	long length;
	long level;

	while (*text == ' ' || *text == '\t' || *text == '(')
		text++;
	/* the word, in either case, at the expression's start */
	for (index = 0; word[index]; index++)
	{
		char character = text[index] >= 'A' && text[index] <= 'Z' ? text[index] - 'A' + 'a' : text[index];

		if (character != word[index])
			return FALSE;
	}
	text += index;
	if (*text != ' ' && *text != '\t' && *text != 0 && *text != ')')
		return FALSE;
	while (*text == ' ' || *text == '\t')
		text++;
	/* its argument, if any: up to the close or a space, in lower case */
	for (length = 0; text[length] && text[length] != ' ' && text[length] != '\t' && text[length] != ')'; length++)
		;
	if (length)
	{
		boolean automatic_argument = FALSE;

		if (length >= (long)sizeof(argument))
			length = (long)sizeof(argument) - 1;
		for (index = 0; index < length; index++)
		{
			argument[index] = text[index] >= 'A' && text[index] <= 'Z' ? text[index] - 'A' + 'a' : text[index];
		}
		argument[length] = 0;
		level = NONE;
		for (index = 0; index < (long)NUMBEROF(automatic); index++)
		{
			if (!strcmp(argument, automatic[index]))
			{
				automatic_argument = TRUE;
				break;
			}
		}
		if (!automatic_argument)
		{
			char *end;
			long number = strtol(argument, &end, 10);

			if (!*argument || *end || number < 0 || number >= NUMBER_OF_MODEL_DETAIL_LEVELS)
			{
				console_printf(FALSE, "debug_lod: the level is -1 (automatic), or 0 (super low) to %d (super high)",
					NUMBER_OF_MODEL_DETAIL_LEVELS - 1);
				return TRUE;
			}
			level = number;
		}
		rasterizer_debug_options.debug_model_lod = (short)level;
	}
	/* no argument: it says what the level is now */
	level = rasterizer_debug_options.debug_model_lod;
	if (level != NONE)
	{
		console_printf(FALSE, "debug_lod: every model at detail level %ld (%s)", level, names[level]);
	}
	else if (port_model_lod() != NONE)
	{
		level = port_model_lod();
		console_printf(FALSE, "debug_lod: every model at the video settings' MODEL DETAIL, level %ld (%s)",
			level, names[level]);
	}
	else
	{
		console_printf(FALSE, "debug_lod: the size of each model on the screen sets its level of detail");
	}
	return TRUE;
}
