/*
SERVER_COMMAND_WORDS.C (test)

The words of a dedicated server's commands (port/linux/game/server_commands.c):
a line split into words (a quoted one whole), the text after some of them,
a reason kept to printable ASCII, a ban's duration, a number in range, a
field of a bans.txt line, and a player found by slot or the start of their
name, with the name read from the game (server_status.c,
player_name_character_ascii of source/game/players.c) and checked against
the web page's (server_status_player_matches); and how long a role may ban
for (ban_duration_allowed). test_server_command_words.py takes the limits
(config.inc) and the code (under_test.inc).
*/

#include "harness.h"

#include <ctype.h>
#include <stdarg.h>
#include <strings.h>
#include <wchar.h>

#define csstrcasecmp strcasecmp
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define NUMBEROF(array) (sizeof(array) / sizeof(array[0]))

#include "config.inc"

struct network_player
{
	boolean valid;
	wchar_t name[12];
};

struct network_game
{
	struct network_player players[MAXIMUM_SLOTS];
};

static struct network_game game;
static boolean hosting = TRUE;

static struct network_game *server_game(void)
{
	return hosting ? &game : NULL;
}

static boolean network_player_is_valid(struct network_player *player)
{
	return player->valid;
}

/* what the commands said last */
static char said[1024];

static void say(char const *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(said, sizeof(said), format, arguments);
	va_end(arguments);
}

#include "under_test.inc"

static void add_player(long slot, wchar_t const *name)
{
	game.players[slot].valid = TRUE;
	wcsncpy(game.players[slot].name, name, NUMBEROF(game.players[slot].name));
}

static long find(char const *text)
{
	said[0] = 0;
	return find_player("kick", text);
}

static long split(char const *line, char *copy, char **words, long maximum)
{
	strcpy(copy, line);
	return split_words(copy, words, maximum);
}

static char const *printable(char const *source, long size)
{
	static char text[256];

	memset(text, '#', sizeof(text));
	printable_text(text, size, source);
	return text;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* words between spaces and tabs, a quoted one whole (no escapes); no
	more than the maximum, the rest of the line left as it was */
	CASE("split-words")
	{
		char copy[256];
		char *words[32];
		long count;

		count = split("sv_map bloodgulch ctf", copy, words, MAXIMUM_WORDS);
		CHECK(count == 3 && !strcmp(words[0], "sv_map") && !strcmp(words[1], "bloodgulch") && !strcmp(words[2], "ctf"),
			"%ld words", count);
		count = split("  \tsv_kick \t  3  ", copy, words, MAXIMUM_WORDS);
		CHECK(count == 2 && !strcmp(words[0], "sv_kick") && !strcmp(words[1], "3"), "%ld words: %s", count, words[1]);
		CHECK(split("", copy, words, MAXIMUM_WORDS) == 0, "an empty line had words");
		CHECK(split(" \t ", copy, words, MAXIMUM_WORDS) == 0, "spaces had words");
		count = split("ban \"Master Chief\" 1d being rude", copy, words, MAXIMUM_WORDS);
		CHECK(count == 5 && !strcmp(words[1], "Master Chief") && !strcmp(words[2], "1d") && !strcmp(words[4], "rude"),
			"%ld words: %s, %s", count, words[1], words[2]);
		count = split("say \"\"", copy, words, MAXIMUM_WORDS);
		CHECK(count == 2 && !words[1][0], "an empty quoted word: %ld words", count);
		count = split("kick \"Master Chief", copy, words, MAXIMUM_WORDS);
		CHECK(count == 2 && !strcmp(words[1], "Master Chief"), "an unclosed quote: %s", words[1]);
		count = split("say \"a\"b \"c\"\"d\"", copy, words, MAXIMUM_WORDS);
		CHECK(count == 5 && !strcmp(words[1], "a") && !strcmp(words[2], "b") && !strcmp(words[3], "c") &&
			!strcmp(words[4], "d"), "quotes against words: %ld words", count);
		count = split("say ab\"c d\"", copy, words, MAXIMUM_WORDS);
		CHECK(count == 3 && !strcmp(words[1], "ab\"c") && !strcmp(words[2], "d\""), "a quote inside a word: %s",
			words[1]);
		count = split("say \"a\\\"b\"", copy, words, MAXIMUM_WORDS);
		CHECK(count == 3 && !strcmp(words[1], "a\\") && !strcmp(words[2], "b\""), "a backslash escaped a quote: %s",
			words[1]);
		count = split("1 2 3 4 5 6 7 8 9 10", copy, words, MAXIMUM_WORDS);
		CHECK(count == MAXIMUM_WORDS && !strcmp(words[MAXIMUM_WORDS - 1], "8"), "%ld words, the last %s", count,
			words[count - 1]);
		CHECK(!strcmp(words[MAXIMUM_WORDS - 1] + 2, "9 10"), "the rest of the line was split: %s",
			words[MAXIMUM_WORDS - 1] + 2);
		count = split("a b c", copy, words, 2);
		CHECK(count == 2 && !strcmp(words[1], "b"), "%ld words of 2", count);
		CHECK(split("a b", copy, words, 0) == 0, "words past a maximum of none");
		return 0;
	}
	/* the text after count words: where the next word begins (its quote) */
	CASE("rest-after-words")
	{
		static char const *const lines[] =
		{
			"say hello  world ", "ban \"Master Chief\" 1d being rude", "  tell\t3   psst", "kick \"unclosed",
			"say \"a b\" c", "sv_map", "",
		};
		char copy[256];
		char *words[32];
		long line, count, index;

		CHECK(!strcmp(rest_after_words("say hello  world ", 1), "hello  world "), "%s",
			rest_after_words("say hello  world ", 1));
		CHECK(!strcmp(rest_after_words("ban \"Master Chief\" 1d being rude", 3), "being rude"), "%s",
			rest_after_words("ban \"Master Chief\" 1d being rude", 3));
		CHECK(!strcmp(rest_after_words("ban \"Master Chief\" 1d being rude", 2), "1d being rude"), "%s",
			rest_after_words("ban \"Master Chief\" 1d being rude", 2));
		CHECK(!strcmp(rest_after_words("ban joe", 3), ""), "past the words: %s", rest_after_words("ban joe", 3));
		CHECK(!strcmp(rest_after_words("say", 0), "say"), "after no words: %s", rest_after_words("say", 0));
		/* (as the words were split) */
		for (line = 0; line < (long)NUMBEROF(lines); line++)
		{
			count = split(lines[line], copy, words, 32);
			for (index = 0; index < count; index++)
			{
				char const *rest = rest_after_words(lines[line], index);
				long at = (long)(words[index] - copy);

				/* (a quoted word's: its quote) */
				if (at > 0 && lines[line][at - 1] == '"' &&
					(at == 1 || lines[line][at - 2] == ' ' || lines[line][at - 2] == '\t'))
				{
					at--;
				}
				CHECK(rest == lines[line] + at, "\"%s\": after %ld words at %ld, not %ld", lines[line], index,
					(long)(rest - lines[line]), at);
			}
			CHECK(!*rest_after_words(lines[line], count), "\"%s\": something after every word", lines[line]);
		}
		return 0;
	}
	/* a text kept to printable ASCII, its spaces after and its quotes taken off */
	CASE("printable-text")
	{
		CHECK(!strcmp(printable("being rude", 96), "being rude"), "%s", printable("being rude", 96));
		CHECK(!strcmp(printable("tab\there\x01\x7f!", 96), "tabhere!"), "%s", printable("tab\there\x01\x7f!", 96));
		CHECK(!strcmp(printable("caf\xc3\xa9", 96), "caf"), "%s", printable("caf\xc3\xa9", 96));
		CHECK(!strcmp(printable("spaces after   ", 96), "spaces after"), "\"%s\"", printable("spaces after   ", 96));
		CHECK(!strcmp(printable("  spaces before", 96), "spaces before"), "\"%s\"", printable("  spaces before", 96));
		/* (the quotes go before the text is cut to its room) */
		CHECK(!strcmp(printable("\"a long quoted reason\"", 8), "a long"), "\"%s\"",
			printable("\"a long quoted reason\"", 8));
		CHECK(!strcmp(printable("\"a quoted reason\"", 96), "a quoted reason"), "%s",
			printable("\"a quoted reason\"", 96));
		CHECK(!strcmp(printable("\"quoted\"  ", 96), "quoted"), "%s", printable("\"quoted\"  ", 96));
		CHECK(!strcmp(printable("\"\"", 96), ""), "%s", printable("\"\"", 96));
		CHECK(!strcmp(printable("\"", 96), "\""), "%s", printable("\"", 96));
		CHECK(!strcmp(printable("\"half", 96), "\"half"), "%s", printable("\"half", 96));
		CHECK(!strcmp(printable("say \"it\"", 96), "say \"it\""), "%s", printable("say \"it\"", 96));
		CHECK(!strcmp(printable("abcdefgh", 6), "abcde"), "%s", printable("abcdefgh", 6));
		CHECK(!strcmp(printable("abc", 1), ""), "a room of one held %s", printable("abc", 1));
		CHECK(!strcmp(printable(NULL, 96), ""), "no text was %s", printable(NULL, 96));
		CHECK(printable("abcdefgh", 6)[6] == '#', "written past the room");
		return 0;
	}
	/* 30m, 2h, 1d, 1w, 45s, 1y, minutes alone, forever: seconds; else -1 */
	CASE("durations")
	{
		CHECK(parse_duration("30m") == 30 * 60, "30m is %ld", parse_duration("30m"));
		CHECK(parse_duration("2h") == 2 * 60 * 60, "2h is %ld", parse_duration("2h"));
		CHECK(parse_duration("1d") == 24 * 60 * 60, "1d is %ld", parse_duration("1d"));
		CHECK(parse_duration("7d") == 7 * 24 * 60 * 60, "7d is %ld", parse_duration("7d"));
		CHECK(parse_duration("1w") == 7 * 24 * 60 * 60, "1w is %ld", parse_duration("1w"));
		CHECK(parse_duration("45s") == 45, "45s is %ld", parse_duration("45s"));
		CHECK(parse_duration("1y") == 365L * 24 * 60 * 60, "1y is %ld", parse_duration("1y"));
		CHECK(parse_duration("10") == 10 * 60, "10 is %ld", parse_duration("10"));
		CHECK(parse_duration("forever") == 0 && parse_duration("FOREVER") == 0 && parse_duration("Permanent") == 0 &&
			parse_duration("0") == 0, "forever is not 0");
		CHECK(parse_duration("10y") == 10L * 365 * 24 * 60 * 60, "10y is %ld", parse_duration("10y"));
		CHECK(parse_duration("11y") == -1, "11y is %ld", parse_duration("11y"));
		CHECK(parse_duration("3650d") == 3650L * 24 * 60 * 60 && parse_duration("3651d") == -1, "3651d is %ld",
			parse_duration("3651d"));
		CHECK(parse_duration("521w") > 0 && parse_duration("522w") == -1, "522w is %ld", parse_duration("522w"));
		CHECK(parse_duration("100000") == 100000L * 60 && parse_duration("100001") == -1, "100001 is %ld",
			parse_duration("100001"));
		CHECK(parse_duration("99999999999h") == -1, "a number past a long was %ld", parse_duration("99999999999h"));
		CHECK(parse_duration("") == -1, "nothing is %ld", parse_duration(""));
		CHECK(parse_duration("m") == -1, "m is %ld", parse_duration("m"));
		CHECK(parse_duration("-5m") == -1, "-5m is %ld", parse_duration("-5m"));
		CHECK(parse_duration("+5m") == -1, "+5m is %ld", parse_duration("+5m"));
		CHECK(parse_duration(" 5m") == -1, "\" 5m\" is %ld", parse_duration(" 5m"));
		CHECK(parse_duration("0m") == -1, "0m is %ld", parse_duration("0m"));
		CHECK(parse_duration("30x") == -1, "30x is %ld", parse_duration("30x"));
		CHECK(parse_duration("30mm") == -1, "30mm is %ld", parse_duration("30mm"));
		CHECK(parse_duration("30m5") == -1, "30m5 is %ld", parse_duration("30m5"));
		CHECK(parse_duration("1.5h") == -1, "1.5h is %ld", parse_duration("1.5h"));
		CHECK(parse_duration("spam") == -1, "a reason is %ld", parse_duration("spam"));
		CHECK(parse_duration("forevers") == -1, "forevers is %ld", parse_duration("forevers"));
		return 0;
	}
	/* a whole number within a range */
	CASE("integers")
	{
		long value = NONE;

		CHECK(parse_integer("5", 0, 9, &value) && value == 5, "5: %ld", value);
		CHECK(parse_integer("0", 0, 9, &value) && value == 0, "0: %ld", value);
		CHECK(parse_integer("9", 0, 9, &value) && value == 9, "9: %ld", value);
		value = NONE;
		CHECK(!parse_integer("10", 0, 9, &value) && value == NONE, "10 was taken");
		CHECK(!parse_integer("-1", 0, 9, &value), "-1 was taken");
		CHECK(parse_integer("-1", -5, 9, &value) && value == -1, "-1 within -5 to 9 was refused");
		CHECK(!parse_integer("5x", 0, 9, &value), "5x was taken");
		CHECK(!parse_integer("", 0, 9, &value), "nothing was taken");
		CHECK(!parse_integer(NULL, 0, 9, &value), "no text was taken");
		CHECK(!parse_integer("0", 0, -1, &value), "a number of an empty range was taken");
		CHECK(!parse_integer("99999999999", 0, 2147483646, &value), "a number past a long was taken");
		return 0;
	}
	/* a field of a bans.txt line (network_distributed.c writes them) */
	CASE("ban-fields")
	{
		static char const line[] =
			"2026-10-24 12:00:00\tip=203.0.113.7\thwid=0123abcd\tdiscord_username=none\tdiscord_id=none\t"
			"players=Chief, Bob\treason=spam (by the moderator)\tuntil=1792839600";
		char field[128];
		char small[8];

		CHECK(ban_field(line, "ip", field, sizeof(field)) && !strcmp(field, "203.0.113.7"), "ip %s", field);
		CHECK(ban_field(line, "hwid", field, sizeof(field)) && !strcmp(field, "0123abcd"), "hwid %s", field);
		CHECK(ban_field(line, "players", field, sizeof(field)) && !strcmp(field, "Chief, Bob"), "players %s", field);
		CHECK(ban_field(line, "reason", field, sizeof(field)) && !strcmp(field, "spam (by the moderator)"), "reason %s",
			field);
		CHECK(ban_field(line, "until", field, sizeof(field)) && strtoul(field, NULL, 10) == 1792839600UL, "until %s",
			field);
		CHECK(!ban_field("2026-10-24 12:00:00\tip=1.2.3.4\treason=x", "until", field, sizeof(field)),
			"a ban for ever has an end");
		CHECK(!ban_field(line, "id", field, sizeof(field)), "id was found in discord_id: %s", field);
		CHECK(!ban_field(line, "username", field, sizeof(field)), "username was found in discord_username");
		CHECK(!ban_field(line, "2026-10-24 12:00:00", field, sizeof(field)), "the date was taken for a field");
		CHECK(ban_field(line, "ip", small, sizeof(small)) && !strcmp(small, "203.0.1"), "cut to %s", small);
		CHECK(ban_field("x\tip=\treason=y", "ip", field, sizeof(field)) && !field[0], "an empty field: %s", field);
		return 0;
	}
	/* a player by slot, else by the start of their name in either case (a
	whole name first); one that is no one's, or more than one's, is refused */
	CASE("find-player")
	{
		add_player(0, L"Chief");
		add_player(1, L"Chieftain");
		add_player(2, L"bob");
		add_player(3, L"Bob");
		add_player(5, L"Ramón");
		add_player(7, L"Spartan-117");
		add_player(MAXIMUM_SLOTS - 1, L"Last");
		CHECK(find("chief") == 0, "chief found %ld: %s", find("chief"), said);
		CHECK(find("CHIEF") == 0, "CHIEF found %ld: %s", find("CHIEF"), said);
		CHECK(find("chieft") == 1, "chieft found %ld: %s", find("chieft"), said);
		CHECK(find("Chieftain") == 1, "Chieftain found %ld", find("Chieftain"));
		CHECK(find("Chieftains") == NONE && strstr(said, "no player's name begins with \"Chieftains\""),
			"more than a name found someone: %s", said);
		CHECK(find("chi") == NONE && strstr(said, "2 players' names begin with \"chi\""), "chi: %s", said);
		CHECK(find("bob") == NONE && strstr(said, "2 players are named \"bob\""), "bob: %s", said);
		CHECK(find("ramon") == 5 && find("RAM") == 5, "a name past ASCII was not found: %s", said);
		CHECK(find("spartan-1") == 7, "spartan-1 found %ld", find("spartan-1"));
		CHECK(find("last") == MAXIMUM_SLOTS - 1, "the last slot's player was not found");
		CHECK(find("3") == 3, "slot 3 found %ld", find("3"));
		CHECK(find("007") == 7, "slot 007 found %ld", find("007"));
		CHECK(find("4") == NONE && strstr(said, "no player in slot 4"), "an empty slot: %s", said);
		CHECK(find("200") == NONE && strstr(said, "no player in slot 200"), "slot 200: %s", said);
		{
			char text[16];

			snprintf(text, sizeof(text), "%d", MAXIMUM_SLOTS);
			CHECK(find(text) == NONE, "slot %s past the last found someone", text);
			snprintf(text, sizeof(text), "%d", MAXIMUM_SLOTS - 1);
			CHECK(find(text) == MAXIMUM_SLOTS - 1, "the last slot was not found");
		}
		CHECK(find("99999999999") == NONE, "a number past a long found someone");
		CHECK(find("-1") == NONE && strstr(said, "no player's name begins with \"-1\""), "-1: %s", said);
		CHECK(find("3x") == NONE && strstr(said, "no player's name begins with"), "3x: %s", said);
		CHECK(find("") == NONE && strstr(said, "give a player's slot"), "nothing: %s", said);
		CHECK(find("x") == NONE, "x found someone");
		hosting = FALSE;
		CHECK(find("chief") == NONE && strstr(said, "not hosting"), "a server not hosting found someone: %s", said);
		return 0;
	}
	/* a slot's player's name in ASCII; whether it is still the page's */
	CASE("player-matches")
	{
		char current[16];

		add_player(0, L"Chief");
		add_player(5, L"Ramón");
		add_player(6, L"中文");
		add_player(9, L"ElevenChars");
		CHECK(server_status_player_matches(0, "Chief", current, sizeof(current)) && !strcmp(current, "Chief"),
			"Chief is not slot 0's: %s", current);
		CHECK(!server_status_player_matches(0, "chief", current, sizeof(current)) && !strcmp(current, "Chief"),
			"a name in another case matched: %s", current);
		CHECK(!server_status_player_matches(0, "Chie", current, sizeof(current)), "the start of a name matched");
		CHECK(!server_status_player_matches(0, "Chiefs", current, sizeof(current)), "more than the name matched");
		CHECK(server_status_player_matches(5, "Ramon", current, sizeof(current)), "Ramón's name read %s", current);
		CHECK(server_status_player_matches(6, "??", current, sizeof(current)), "a name of no Latin letters read %s",
			current);
		CHECK(server_status_player_matches(9, "ElevenChars", current, sizeof(current)), "eleven characters read %s",
			current);
		strcpy(current, "stale");
		CHECK(!server_status_player_matches(4, "", current, sizeof(current)) && !current[0],
			"an empty slot matched, or kept \"%s\"", current);
		CHECK(!server_status_player_matches(-1, "Chief", current, sizeof(current)), "slot -1 matched");
		CHECK(!server_status_player_matches(MAXIMUM_SLOTS, "Chief", current, sizeof(current)), "a slot past the last matched");
		memset(current, '#', sizeof(current));
		CHECK(server_status_player_name(0, current, 4) && !strcmp(current, "Chi") && current[4] == '#',
			"a room of 4 held %s", current);
		CHECK(!server_status_player_matches(0, "Chief", current, 4), "a name cut to its room matched");
		hosting = FALSE;
		CHECK(!server_status_player_matches(0, "Chief", current, sizeof(current)) && !current[0],
			"a server not hosting matched");
		return 0;
	}
	/* a moderator bans for a day at most, never for ever; an admin and the
	owner for as long as they like */
	CASE("ban-limits")
	{
		CHECK(ban_duration_allowed(_server_role_moderator, 30 * 60), "a moderator's 30 minutes were refused");
		CHECK(ban_duration_allowed(_server_role_moderator, 24 * 60 * 60), "a moderator's day was refused");
		CHECK(!ban_duration_allowed(_server_role_moderator, 24 * 60 * 60 + 1), "a moderator banned for over a day");
		CHECK(!ban_duration_allowed(_server_role_moderator, 7 * 24 * 60 * 60), "a moderator banned for a week");
		CHECK(!ban_duration_allowed(_server_role_moderator, 0), "a moderator banned for ever");
		CHECK(!ban_duration_allowed(_server_role_viewer, 0), "a viewer banned for ever");
		CHECK(ban_duration_allowed(_server_role_admin, 0), "an admin's ban for ever was refused");
		CHECK(ban_duration_allowed(_server_role_admin, 7 * 24 * 60 * 60), "an admin's week was refused");
		CHECK(ban_duration_allowed(_server_role_owner, 0), "the owner's ban for ever was refused");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
