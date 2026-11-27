/*
SERVER_TOML.C

A dedicated server's gametype files (server_toml.h).
*/

#include "server_toml.h"
#include "server_mode.h"

#if HALO_SERVER_BUILD

#include "platform.h"
#include "port_config.h"
#include "posix.h"
#include "tomlc17.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const engine_tables[] = { "ctf", "slayer", "king", "oddball", "race" };

int server_gametype_file_name_valid(const char *name)
{
	int index;

	for (index = 0; name[index]; index++)
	{
		unsigned char character = (unsigned char)name[index];

		if (!(isalnum(character) || character == '_' || character == '-') || index >= SERVER_GAMETYPE_SIZE - 6)
			return 0;
	}
	return index > 0;
}

/* a value of the file as its rule takes it: its text */
static int gametype_value_text(toml_datum_t datum, char *text, int size)
{
	switch (datum.type)
	{
	case TOML_BOOLEAN:
		snprintf(text, (size_t)size, "%s", datum.u.boolean ? "true" : "false");
		return 1;
	case TOML_INT64:
		snprintf(text, (size_t)size, "%lld", (long long)datum.u.int64);
		return 1;
	case TOML_STRING:
		snprintf(text, (size_t)size, "%s", datum.u.s);
		return 1;
	default:
		return 0;
	}
}

static int gametype_error(struct server_gametype_file *file, int line, const char *format, const char *a,
	const char *b)
{
	snprintf(file->error, sizeof(file->error), format, a, b);
	file->error_line = line;
	return 0;
}

int server_gametype_file_parse(const char *text, int length, struct server_gametype_file *file)
{
	toml_result_t result;
	int section_index;
	int ok = 1;

	memset(file, 0, sizeof(*file));
	if (length > SERVER_GAMETYPE_FILE_MAXIMUM_SIZE)
		return gametype_error(file, 0, "the file is larger than %s%s", "64 KB", "");
	result = toml_parse(text, length);
	if (!result.ok)
	{
		int line = 0;
		const char *at = strstr(result.errmsg, "(line ");

		if (at)
			sscanf(at, "(line %d)", &line);
		gametype_error(file, line, "%s%s", result.errmsg, "");
		toml_free(result);
		return 0;
	}
	for (section_index = 0; ok && section_index < result.toptab.u.tab.size; section_index++)
	{
		const char *key = result.toptab.u.tab.key[section_index];
		toml_datum_t value = result.toptab.u.tab.value[section_index];
		int key_index, table_index;

		if (value.type != TOML_TABLE)
		{
			if (!strcmp(key, "base") && value.type == TOML_STRING && strlen(value.u.s) < sizeof(file->base))
			{
				int index;

				snprintf(file->base, sizeof(file->base), "%s", value.u.s);
				for (index = 0; file->base[index]; index++)
					file->base[index] = (char)tolower((unsigned char)file->base[index]);
			}
			else if (!strcmp(key, "name") && value.type == TOML_STRING)
			{
				size_t index;

				if (strlen(value.u.s) >= sizeof(file->name))
				{
					ok = gametype_error(file, value.lineno, "name is longer than %s characters%s", "11", "");
					break;
				}
				for (index = 0; value.u.s[index]; index++)
				{
					if ((unsigned char)value.u.s[index] < 0x20 || (unsigned char)value.u.s[index] >= 0x7F)
					{
						ok = gametype_error(file, value.lineno, "name has a character that is not printable %s%s",
							"ASCII", "");
						break;
					}
				}
				if (ok)
					snprintf(file->name, sizeof(file->name), "%s", value.u.s);
			}
			else if (!strcmp(key, "base") || !strcmp(key, "name"))
			{
				ok = gametype_error(file, value.lineno, "%s is a string of %s characters at most", key,
					!strcmp(key, "base") ? "63" : "11");
			}
			else
			{
				ok = gametype_error(file, value.lineno,
					"%s is not base or name (a rule goes in its table: [universal], [options], [%s])", key,
					"ctf] ... [race");
			}
			continue;
		}
		/* a table: universal, options or an engine's */
		for (table_index = 0; table_index < (int)(sizeof(engine_tables) / sizeof(engine_tables[0])) &&
			strcmp(key, engine_tables[table_index]); table_index++)
		{
		}
		if (strcmp(key, "universal") && strcmp(key, "options") &&
			table_index == (int)(sizeof(engine_tables) / sizeof(engine_tables[0])))
		{
			ok = gametype_error(file, value.lineno, "[%s] is no table of a gametype file%s", key, "");
			break;
		}
		if (table_index < (int)(sizeof(engine_tables) / sizeof(engine_tables[0])))
		{
			if (file->engine_table[0] && strcmp(file->engine_table, key))
			{
				ok = gametype_error(file, value.lineno, "[%s] and [%s]: a gametype has one game engine", file->engine_table,
					key);
				break;
			}
			snprintf(file->engine_table, sizeof(file->engine_table), "%s", key);
		}
		for (key_index = 0; ok && key_index < value.u.tab.size; key_index++)
		{
			const char *rule_key = value.u.tab.key[key_index];
			toml_datum_t rule_value = value.u.tab.value[key_index];
			const struct server_rule_definition *rule = server_rule_find(rule_key);
			char text_value[64];
			char reason[SERVER_REASON_SIZE];
			long number;
			int existing;

			if (!rule || strcmp(rule->key, rule_key))
			{
				ok = gametype_error(file, rule_value.lineno, "no rule is named %s (in [%s])", rule_key, key);
				break;
			}
			if (strcmp(rule->table, key))
			{
				char where[80];

				snprintf(where, sizeof(where), "[%s], not [%s]", rule->table, key);
				ok = gametype_error(file, rule_value.lineno, "%s goes in %s", rule_key, where);
				break;
			}
			if (!gametype_value_text(rule_value, text_value, sizeof(text_value)) ||
				!server_rule_value(rule, text_value, &number, reason, sizeof(reason)))
			{
				if (!gametype_value_text(rule_value, text_value, sizeof(text_value)))
					snprintf(reason, sizeof(reason), "%s takes a number, true or false, or a word", rule_key);
				ok = gametype_error(file, rule_value.lineno, "%s%s", reason, "");
				break;
			}
			for (existing = 0; existing < file->rule_count && strcmp(file->rules[existing].key, rule->key); existing++)
			{
			}
			if (existing == file->rule_count)
			{
				if (file->rule_count >= SERVER_RULES_MAXIMUM)
				{
					ok = gametype_error(file, rule_value.lineno, "more than %s rules%s", "48", "");
					break;
				}
				file->rule_count++;
			}
			snprintf(file->rules[existing].key, sizeof(file->rules[existing].key), "%s", rule->key);
			file->rules[existing].value = number;
		}
	}
	if (ok && !file->base[0])
		ok = gametype_error(file, 1, "there is no base = \"<gametype>\" (slayer, team_slayer, ctf, %s%s", "...)", "");
	toml_free(result);
	return ok;
}

void server_gametypes_folder(char *path, int size)
{
	char folder[1024];
	struct posix_file_information information;

	/* (beside config.toml when there is one there: a server's own, among
	several sharing a data root; else the data root's) */
	config_folder(folder, sizeof(folder));
	snprintf(path, (size_t)size, "%sgametypes", folder);
	if (folder[0] && posix_stat(path, &information) == 0 && (information.flags & _posix_file_is_directory))
		return;
	platform_translate_path("d:\\gametypes", path, (unsigned long)size);
}

int server_gametype_saved_read(const char *name, void *data, int size)
{
	char folder[1024];
	char path[1200];
	size_t length = 0;
	char *text;

	if (!server_gametype_file_name_valid(name) || size < 0)
		return -1;
	server_gametypes_folder(folder, sizeof(folder));
	snprintf(path, sizeof(path), "%s/%s.gametype", folder, name);
	text = config_file_read(path, &length);
	if (!text)
		return -1;
	if (length > (size_t)size)
		length = (size_t)size;
	memcpy(data, text, length);
	free(text);
	return (int)length;
}

int server_gametype_file_read(const char *name, struct server_gametype_file *file, int *absent)
{
	char folder[1024];
	char path[1200];
	size_t size = 0;
	char *text;
	int ok;

	memset(file, 0, sizeof(*file));
	*absent = 0;
	if (!server_gametype_file_name_valid(name))
	{
		*absent = 1;
		return 0;
	}
	server_gametypes_folder(folder, sizeof(folder));
	snprintf(path, sizeof(path), "%s/%s.toml", folder, name);
	text = config_file_read(path, &size);
	if (!text)
	{
		*absent = 1;
		return 0;
	}
	ok = server_gametype_file_parse(text, (int)size, file);
	free(text);
	return ok;
}

#endif
