/*
SERVER_TOML.C (test)

A dedicated server's gametype files (port/linux/src/server_toml.c): the
TOML read by the vendored tomlc17 (taken whole, toml.inc) into a base, a
name and the rules of server_rotation.c's table (taken whole with no file
access, rotation.inc), each checked to be in its table and range, with the
reason and line of what is refused; and the names a file may have.
test_server_toml.py takes the code (under_test.inc).
*/

#include "harness.h"

#include <assert.h>

/* (tomlc17 is C11's; its warnings are its own) */
#ifndef static_assert
#define static_assert _Static_assert
#endif
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "toml.inc"
#pragma clang diagnostic pop

#define SERVER_ROTATION_NO_FILES
#include "rotation.inc"

#include "server_toml.h"

#include "under_test.inc"

static struct server_gametype_file file;

static int parse(char const *text)
{
	return server_gametype_file_parse(text, (int)strlen(text), &file);
}

/* refused, with a reason that says this, on that line (0: any) */
static int refused(char const *text, char const *reason, int line)
{
	return !parse(text) && file.error[0] && strstr(file.error, reason) && (!line || file.error_line == line);
}

static long rule_of(char const *key)
{
	int index;

	for (index = 0; index < file.rule_count; index++)
	{
		if (!strcmp(file.rules[index].key, key))
			return file.rules[index].value;
	}
	return NONE;
}

/* a text of count times the character */
static char const *repeated(char character, int count)
{
	static char text[256];

	memset(text, character, (size_t)count);
	text[count] = 0;
	return text;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* server_toml.h's example */
	CASE("example")
	{
		CHECK(parse(
			"base = \"team_slayer\"   # a built-in or OpenCE gametype: its rules are the defaults\n"
			"name = \"BTB Slayer\"    # 11 characters, shown in the lobby and the server browser\n"
			"[universal]            # any game engine's rules\n"
			"score_to_win = 50\n"
			"[options]              # the PC version's options\n"
			"time_limit = 15\n"
			"friendly_fire = \"off\"\n"
			"[slayer]               # the base's game engine's: ctf, slayer, king, oddball, race\n"
			"no_death_bonus = true\n"), "the example was refused: %s (line %d)", file.error, file.error_line);
		CHECK(!strcmp(file.base, "team_slayer") && !strcmp(file.name, "BTB Slayer"), "base %s, name %s", file.base,
			file.name);
		CHECK(!strcmp(file.engine_table, "slayer"), "engine table %s", file.engine_table);
		CHECK(file.rule_count == 4, "%d rules", file.rule_count);
		CHECK(rule_of("score_to_win") == 50 && rule_of("time_limit") == 15 && rule_of("friendly_fire") == 1 &&
			rule_of("no_death_bonus") == 1, "score %ld, time %ld, ff %ld, bonus %ld", rule_of("score_to_win"),
			rule_of("time_limit"), rule_of("friendly_fire"), rule_of("no_death_bonus"));
		CHECK(!file.error[0] && !file.error_line, "an error was kept: %s", file.error);
		return 0;
	}
	/* base and name: a base is needed, lowercased; a name of 11 printable
	ASCII characters at most */
	CASE("base-name")
	{
		CHECK(parse("base = \"CTF\""), "a base alone was refused: %s", file.error);
		CHECK(!strcmp(file.base, "ctf") && !file.name[0] && !file.engine_table[0] && !file.rule_count,
			"base %s, name %s", file.base, file.name);
		CHECK(parse("base = 'ctf'\nname = 'Literal'"), "literal strings were refused: %s", file.error);
		CHECK(!strcmp(file.name, "Literal"), "name %s", file.name);
		CHECK(refused("name = \"No base\"", "there is no base", 1), "a file with no base was taken: %s", file.error);
		CHECK(refused("", "there is no base", 1), "an empty file was taken");
		CHECK(refused("[universal]\nlives = 3\n", "there is no base", 1), "rules with no base were taken");
		CHECK(parse("base = \"slayer\"\nname = \"12345678901\""), "an 11 character name was refused: %s", file.error);
		CHECK(!strcmp(file.name, "12345678901"), "name %s", file.name);
		CHECK(refused("base = \"slayer\"\nname = \"123456789012\"", "name is longer than 11 characters", 2),
			"a 12 character name was taken: %s (line %d)", file.error, file.error_line);
		CHECK(parse("base = \"slayer\"\nname = \"A\\\"B\\\\C\""), "escapes were refused: %s", file.error);
		CHECK(!strcmp(file.name, "A\"B\\C"), "name %s", file.name);
		CHECK(parse("base = \"slayer\"\nname = \"\\u0041\\u0062\""), "\\u escapes were refused: %s", file.error);
		CHECK(!strcmp(file.name, "Ab"), "name %s", file.name);
		CHECK(refused("base = \"slayer\"\nname = \"Tab\\there\"", "not printable ASCII", 2), "a tab was taken: %s",
			file.error);
		CHECK(refused("base = \"slayer\"\nname = \"caf\\u00e9\"", "not printable ASCII", 2), "an accent was taken: %s",
			file.error);
		CHECK(refused("base = \"slayer\"\nname = \"x\\u007f\"", "not printable ASCII", 2), "a delete was taken");
		CHECK(refused("base = 5", "base is a string", 1), "a base that is a number was taken: %s", file.error);
		CHECK(refused("base = \"slayer\"\nname = true", "name is a string", 2), "a name that is true was taken");
		{
			char text[256];

			snprintf(text, sizeof(text), "base = \"%s\"", repeated('b', SERVER_GAMETYPE_SIZE - 1));
			CHECK(parse(text) && strlen(file.base) == SERVER_GAMETYPE_SIZE - 1, "the longest base was refused: %s",
				file.error);
			snprintf(text, sizeof(text), "base = \"%s\"", repeated('b', SERVER_GAMETYPE_SIZE));
			CHECK(!parse(text) && file.error[0], "a base longer than its room was taken");
		}
		return 0;
	}
	/* top-level keys are base and name; tables are universal, options and
	one engine's, each rule in its own */
	CASE("sections")
	{
		CHECK(parse("base = \"ctf\"\n[universal]\nlives = 3\n[options]\nhardcore = true\n[ctf]\nassault = true\n"),
			"three tables were refused: %s", file.error);
		CHECK(!strcmp(file.engine_table, "ctf") && file.rule_count == 3, "engine %s, %d rules", file.engine_table,
			file.rule_count);
		CHECK(parse("base = \"oddball\"\n[oddball]\nball_spawn_count = 2\n[universal]\n[options]\n"),
			"empty tables were refused: %s", file.error);
		CHECK(parse("base = \"ctf\"\nuniversal.lives = 2\noptions = { time_limit = 5 }\n"),
			"dotted keys and inline tables were refused: %s", file.error);
		CHECK(rule_of("lives") == 2 && rule_of("time_limit") == 5, "lives %ld, time %ld", rule_of("lives"),
			rule_of("time_limit"));
		CHECK(refused("base = \"ctf\"\nscore_to_win = 5\n", "score_to_win is not base or name", 2),
			"a rule outside a table was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[bogus]\nlives = 1\n", "[bogus] is no table of a gametype file", 0),
			"an unknown table was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[ctf]\nassault = true\n[slayer]\nkill_in_order = true\n",
			"[ctf] and [slayer]: a gametype has one game engine", 0), "two engines' tables were taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[options]\nscore_to_win = 5\n", "score_to_win goes in [universal], not [options]",
			3), "a rule in another table was taken: %s (line %d)", file.error, file.error_line);
		CHECK(refused("base = \"slayer\"\n[slayer]\nassault = true\n", "assault goes in [ctf], not [slayer]", 3),
			"an engine's rule in another engine's table was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal]\nbogus = 1\n", "no rule is named bogus (in [universal])", 3),
			"an unknown rule was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal]\nscore = 5\n", "no rule is named score", 3),
			"an alias was taken for a key: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal]\nLives = 5\n", "no rule is named Lives", 3),
			"a key in capitals was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal]\nbase = \"slayer\"\n", "no rule is named base", 3),
			"base in a table was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal.more]\nlives = 1\n", "no rule is named more", 0),
			"a table in a table was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[[universal]]\nlives = 1\n", "universal is not base or name", 0),
			"an array of tables was taken: %s", file.error);
		return 0;
	}
	/* a value as its rule takes it: numbers, booleans, words; why not */
	CASE("values")
	{
		CHECK(parse("base = \"ctf\"\n[universal]\nteams = false\nradar = 1\nweapon_set = \"Plasma\"\n"
			"vehicle_set = 2\nscore_to_win = \"50\"\nlives = 0\nhealth = 400\n"), "values were refused: %s", file.error);
		CHECK(rule_of("teams") == 0 && rule_of("radar") == 1 && rule_of("weapon_set") == 3 &&
			rule_of("vehicle_set") == 2 && rule_of("score_to_win") == 50 && rule_of("lives") == 0 &&
			rule_of("health") == 400, "teams %ld, radar %ld, weapons %ld, vehicles %ld, score %ld",
			rule_of("teams"), rule_of("radar"), rule_of("weapon_set"), rule_of("vehicle_set"), rule_of("score_to_win"));
		CHECK(parse("base = \"ctf\"\n[options]\nfriendly_fire = \"shields_only\"\ntime_limit = 0x10\n"),
			"a word and a hexadecimal number were refused: %s", file.error);
		CHECK(rule_of("friendly_fire") == 2 && rule_of("time_limit") == 16, "ff %ld, time %ld", rule_of("friendly_fire"),
			rule_of("time_limit"));
		CHECK(refused("base = \"ctf\"\n[universal]\nscore_to_win = 1000\n", "score_to_win takes 0 to 999, not 1000", 3),
			"a score out of range was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal]\nscore_to_win = -1\n", "not -1", 3), "-1 was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal]\nscore_to_win = 99999999999\n", "takes a whole number", 3),
			"a number past a long was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal]\nhealth = 24\n", "health takes 25 to 400", 3), "24 was taken");
		CHECK(refused("base = \"ctf\"\n[universal]\nteams = 2\n", "teams takes true or false", 3), "teams = 2 was taken");
		CHECK(refused("base = \"ctf\"\n[universal]\nteams = \"maybe\"\n", "teams takes true or false, not \"maybe\"", 3),
			"teams = maybe was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[options]\nfriendly_fire = \"bogus\"\n",
			"friendly_fire takes on, off, shields_only or explosives_only, not \"bogus\"", 3), "%s", file.error);
		CHECK(refused("base = \"ctf\"\n[options]\nfriendly_fire = 4\n", "friendly_fire takes", 3),
			"an index past the words was taken");
		CHECK(refused("base = \"ctf\"\n[universal]\nscore_to_win = 1.5\n",
			"score_to_win takes a number, true or false, or a word", 3), "a fraction was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal]\nlives = [1, 2]\n", "lives takes a number", 3),
			"an array was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal]\nlives = 2026-10-24\n", "lives takes a number", 3),
			"a date was taken: %s", file.error);
		CHECK(refused("base = \"ctf\"\n[universal]\nlives = { a = 1 }\n", "lives takes a number", 3),
			"a table was taken: %s", file.error);
		return 0;
	}
	/* comments, blank lines, spaces and CRLF line ends */
	CASE("comments")
	{
		CHECK(parse("# a gametype\r\n\r\n  base = \"king\"  # the base\r\n\t[king]   # its table\r\n"
			"moving_hill = true # it moves\r\n# lives = 3\r\n"), "comments were refused: %s", file.error);
		CHECK(!strcmp(file.base, "king") && file.rule_count == 1 && rule_of("moving_hill") == 1,
			"base %s, %d rules", file.base, file.rule_count);
		CHECK(parse("base = \"ctf\" # name = \"not read\"\n"), "a comment after a value was refused: %s", file.error);
		CHECK(!file.name[0], "a comment's name was read: %s", file.name);
		return 0;
	}
	/* a key or table given twice is TOML's error, with its line */
	CASE("duplicates")
	{
		CHECK(refused("base = \"ctf\"\n[universal]\nlives = 1\nlives = 2\n", "duplicate key", 4),
			"a rule given twice was taken: %s (line %d)", file.error, file.error_line);
		CHECK(refused("base = \"ctf\"\n[universal]\nlives = 1\n[universal]\nteams = true\n", "more than once", 4),
			"a table given twice was taken: %s (line %d)", file.error, file.error_line);
		CHECK(refused("base = \"ctf\"\nbase = \"slayer\"\n", "duplicate key", 2), "base given twice was taken: %s (line %d)",
			file.error, file.error_line);
		CHECK(refused("base = \"ctf\"\n[universal]\nlives = 1\n[options]\nuniversal.lives = 2\n", "no rule is named universal", 5),
			"a dotted key in another table was taken: %s (line %d)", file.error, file.error_line);
		return 0;
	}
	/* what is not TOML: refused with the line TOML names */
	CASE("malformed")
	{
		CHECK(refused("base = \"ctf\"\n[universal]\nlives =\n", "", 3), "a key with no value: %s (line %d)", file.error,
			file.error_line);
		CHECK(refused("base = \"ctf\"\n[universal\nlives = 1\n", "", 2), "an unclosed table: %s (line %d)", file.error,
			file.error_line);
		CHECK(refused("base = \"ctf\"\nname = \"unterminated\n", "", 0) && file.error_line, "an unclosed string: %s (line %d)", file.error,
			file.error_line);
		CHECK(refused("this is not toml\n", "", 1), "a sentence: %s (line %d)", file.error, file.error_line);
		CHECK(refused("base = \"ctf\"\nname = \"bad \\q escape\"\n", "", 2), "an unknown escape: %s (line %d)",
			file.error, file.error_line);
		CHECK(refused("base = \"ctf\"\n\n\nlives 3\n", "", 4), "a key with no '=': %s (line %d)", file.error,
			file.error_line);
		return 0;
	}
	/* 64 KB at most; only the length given is read */
	CASE("size")
	{
		static char text[SERVER_GAMETYPE_FILE_MAXIMUM_SIZE + 64];
		int length;

		length = snprintf(text, sizeof(text), "base = \"ctf\"\n");
		memset(text + length, '#', (size_t)(SERVER_GAMETYPE_FILE_MAXIMUM_SIZE - length - 1));
		text[SERVER_GAMETYPE_FILE_MAXIMUM_SIZE - 1] = '\n';
		text[SERVER_GAMETYPE_FILE_MAXIMUM_SIZE] = 0;
		CHECK(server_gametype_file_parse(text, SERVER_GAMETYPE_FILE_MAXIMUM_SIZE, &file),
			"a 64 KB file was refused: %s", file.error);
		text[SERVER_GAMETYPE_FILE_MAXIMUM_SIZE] = '\n';
		text[SERVER_GAMETYPE_FILE_MAXIMUM_SIZE + 1] = 0;
		CHECK(!server_gametype_file_parse(text, SERVER_GAMETYPE_FILE_MAXIMUM_SIZE + 1, &file) &&
			strstr(file.error, "the file is larger than 64 KB"), "a larger file was taken: %s", file.error);
		snprintf(text, sizeof(text), "base = \"ctf\"\n[bogus]\n");
		CHECK(server_gametype_file_parse(text, 13, &file), "read past the length: %s", file.error);
		return 0;
	}
	/* every rule of universal, options and each engine's table in one file:
	at their least and their most */
	CASE("every-rule")
	{
		static char const *const engines[] = { "ctf", "slayer", "king", "oddball", "race" };
		static char text[8192];
		int engine, most;

		for (most = 0; most <= 1; most++)
		{
			for (engine = 0; engine < (int)(sizeof(engines) / sizeof(engines[0])); engine++)
			{
				char const *tables[3] = { "universal", "options", engines[engine] };
				int table, index, expected = 0;
				int length = snprintf(text, sizeof(text), "base = \"%s\"\n", engines[engine]);

				for (table = 0; table < 3; table++)
				{
					length += snprintf(text + length, sizeof(text) - (size_t)length, "[%s]\n", tables[table]);
					for (index = 0; index < server_rule_definition_count(); index++)
					{
						const struct server_rule_definition *rule = &server_rule_definitions[index];
						int last = 0;

						if (strcmp(rule->table, tables[table]))
							continue;
						expected++;
						if (rule->type == _server_rule_integer)
						{
							length += snprintf(text + length, sizeof(text) - (size_t)length, "%s = %ld\n", rule->key,
								most ? rule->maximum : rule->minimum);
							continue;
						}
						while (rule->choices[last + 1])
							last++;
						if (rule->type == _server_rule_boolean)
						{
							length += snprintf(text + length, sizeof(text) - (size_t)length, "%s = %s\n", rule->key,
								most ? "true" : "false");
						}
						else
						{
							length += snprintf(text + length, sizeof(text) - (size_t)length, "%s = \"%s\"\n", rule->key,
								rule->choices[most ? last : 0]);
						}
					}
				}
				CHECK(server_gametype_file_parse(text, length, &file), "every %s rule was refused: %s (line %d)",
					engines[engine], file.error, file.error_line);
				CHECK(file.rule_count == expected && expected <= SERVER_RULES_MAXIMUM, "%d of %d rules", file.rule_count,
					expected);
				for (index = 0; index < file.rule_count; index++)
				{
					const struct server_rule_definition *rule = server_rule_find(file.rules[index].key);
					long want = most ? rule->maximum : rule->minimum;

					if (rule->type == _server_rule_choice)
					{
						for (want = 0; most && rule->choices[want + 1]; want++)
						{
						}
					}
					CHECK(file.rules[index].value == want, "%s is %ld, not %ld", rule->key, file.rules[index].value, want);
				}
			}
		}
		return 0;
	}
	/* a gametype file's name: letters, digits, '_' and '-', no path, and
	room for file= before it in a rotation's gametype */
	CASE("name-valid")
	{
		CHECK(server_gametype_file_name_valid("btb_slayer"), "btb_slayer was refused");
		CHECK(server_gametype_file_name_valid("Team-CTF2"), "Team-CTF2 was refused");
		CHECK(server_gametype_file_name_valid("a"), "a was refused");
		CHECK(!server_gametype_file_name_valid(""), "an empty name was taken");
		CHECK(!server_gametype_file_name_valid("../config"), "../config was taken");
		CHECK(!server_gametype_file_name_valid(".."), ".. was taken");
		CHECK(!server_gametype_file_name_valid("a/b"), "a/b was taken");
		CHECK(!server_gametype_file_name_valid("a\\b"), "a\\b was taken");
		CHECK(!server_gametype_file_name_valid("/etc/passwd"), "/etc/passwd was taken");
		CHECK(!server_gametype_file_name_valid("c:x"), "c:x was taken");
		CHECK(!server_gametype_file_name_valid("x.toml"), "x.toml was taken");
		CHECK(!server_gametype_file_name_valid("a b"), "a b was taken");
		CHECK(!server_gametype_file_name_valid("caf\xc3\xa9"), "a name past ASCII was taken");
		CHECK(!server_gametype_file_name_valid("x\n"), "a new line was taken");
		CHECK(server_gametype_file_name_valid(repeated('n', SERVER_GAMETYPE_SIZE - 6)), "the longest name was refused");
		CHECK(!server_gametype_file_name_valid(repeated('n', SERVER_GAMETYPE_SIZE - 5)), "a name too long was taken");
		CHECK(!server_gametype_file_name_valid(repeated('n', 200)), "a 200 character name was taken");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
