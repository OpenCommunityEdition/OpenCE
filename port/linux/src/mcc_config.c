/*
MCC_CONFIG.C

Older builds wrote all three discovery settings as false, even when the player
never touched them. Enable those saved defaults once; the completion key keeps
subsequent opt-outs intact. Environment overrides are applied by the caller
later and are never written into the file.
*/

#include "mcc_config.h"

#include <stddef.h>
#include <string.h>

int mcc_config_upgrade(char *text, toml_datum_t table)
{
	static const char *const settings[] =
	{
		"game.workshop_maps", "game.mcc_campaigns", "game.mcc_collections",
	};
	size_t index;
	int changed = 0;

	if (toml_seek(table, "game.mcc_discovery_defaults").type != TOML_UNKNOWN)
		return 0;
	for (index = 0; index < sizeof(settings) / sizeof(settings[0]); index++)
	{
		toml_datum_t value = toml_seek(table, settings[index]);
		char *line = text;
		int number;
		size_t length, column;

		if (value.type != TOML_BOOLEAN || value.u.boolean || value.lineno < 1 || value.colno < 1)
			continue;
		for (number = 1; number < value.lineno && line; number++)
		{
			line = strchr(line, '\n');
			if (line) line++;
		}
		if (!line) continue;
		length = strcspn(line, "\n");
		column = (size_t)value.colno - 1;
		if (column > length || length - column < 5 || strncmp(line + column, "false", 5))
			continue;
		/* Equal length keeps the remaining parser locations valid. */
		memcpy(line + column, "true ", 5);
		changed = 1;
	}
	return changed;
}
