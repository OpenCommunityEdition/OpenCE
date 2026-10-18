/*
TEXT_FILTER.C

Streamer mode (Settings > Network Setup > Streamer Mode): the words of
text_filter_match.c's list hidden as stars wherever text is drawn, at the
levels set (streamer.enabled, and streamer.slurs, streamer.swearing and
streamer.insults: "off", "severe", "strong" or "mild", each that severity
and worse).

draw_string.c's draw_string and draw_unicode_string take the string's
filtered copy before they draw or measure it (draw_string_compute_bounds
and the rest measure through them), so every string the game shows is
filtered: names over heads, the kill feed, the scoreboards, the lobbies, the
server browser, map and mod names, the console's lines, and a map's or mod's
own text. A copy is the same length or shorter (a censored character is one
'*'), so a fixed buffer is never overrun. Copies are kept, so that a string
drawn each frame is looked at once.

What the player types is drawn as it is: the menus' text field being edited
(text_filter_exempt), the console's input line and the virtual keyboard's
(text_filter_suspend). The player's own words: d:\text_filter.txt, lines of
"add <word>" (hidden whenever streamer mode is on) and "allow <word>" (never
hidden), read as the settings change.

Only what this machine draws changes: nothing sent between machines.
*/

#include "text_filter.h"

#include <stdio.h>
#include <string.h>

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
const char *config_string(const char *name);
unsigned long config_changes(void);

#define USER_FILE "d:\\text_filter.txt"
#define CACHE_ENTRIES 128
#define CACHE_LENGTH 256
#define LONG_BUFFERS 4

struct text_filter_cache_entry
{
	unsigned long hash;
	short length;
	unsigned char narrow;
	unsigned char clean;
	unsigned short original[CACHE_LENGTH];
	union
	{
		unsigned short wide[CACHE_LENGTH];
		char narrow[CACHE_LENGTH];
	} filtered;
};

static struct
{
	unsigned long changes;
	int enabled;
	unsigned char levels[NUMBER_OF_TEXT_FILTER_GROUPS];
	long suspended;
	void const *exempt;
	struct text_filter_cache_entry cache[CACHE_ENTRIES];
	union
	{
		unsigned short wide[TEXT_FILTER_MAXIMUM_LENGTH + 1];
		char narrow[TEXT_FILTER_MAXIMUM_LENGTH + 1];
	} long_buffers[LONG_BUFFERS];
	long next_long_buffer;
	unsigned short units[TEXT_FILTER_MAXIMUM_LENGTH];
	unsigned char censored[TEXT_FILTER_MAXIMUM_LENGTH];
} text_filter_globals = { (unsigned long)-1 };

static unsigned char text_filter_level(
	char const *name)
{
	char const *value = config_string(name);

	if (!value || !strcmp(value, "off"))
		return _text_filter_off;
	if (!strcmp(value, "severe"))
		return _text_filter_severe;
	if (!strcmp(value, "strong"))
		return _text_filter_strong;
	return _text_filter_mild;
}

/* the list again with the player's own words (USER_FILE) */
static void text_filter_user_words(
	void)
{
	FILE *file = fopen(USER_FILE, "r");
	char line[128];

	text_filter_words_reset();
	if (!file)
		return;
	while (fgets(line, sizeof(line), file))
	{
		char *word = line;

		line[strcspn(line, "\r\n")] = 0;
		while (*word == ' ' || *word == '\t')
			word++;
		if (!strncmp(word, "add ", 4))
			text_filter_word_add(word + 4, _text_filter_own, _text_filter_severe, _text_filter_anywhere);
		else if (!strncmp(word, "allow ", 6))
			text_filter_allow_add(word + 6);
	}
	fclose(file);
	return;
}

/* whether anything is hidden: the settings, read again when they change */
static int text_filter_enabled(
	void)
{
	if (text_filter_globals.changes != config_changes())
	{
		long index;

		text_filter_globals.changes = config_changes();
		text_filter_globals.enabled = config_boolean("streamer.enabled") != 0;
		text_filter_globals.levels[_text_filter_slurs] = text_filter_level("streamer.slurs");
		text_filter_globals.levels[_text_filter_swearing] = text_filter_level("streamer.swearing");
		text_filter_globals.levels[_text_filter_insults] = text_filter_level("streamer.insults");
		text_filter_globals.levels[_text_filter_own] = _text_filter_mild;
		for (index = 0; index < CACHE_ENTRIES; index++)
			text_filter_globals.cache[index].length = -1;
		if (text_filter_globals.enabled)
			text_filter_user_words();
	}
	return text_filter_globals.enabled && !text_filter_globals.suspended;
}

/* whether the string is a copy of ours (a string measured is drawn again
through draw_string or draw_unicode_string) */
static int text_filter_ours(
	void const *string)
{
	char const *pointer = (char const *)string;
	char const *start = (char const *)&text_filter_globals;

	return pointer >= start && pointer < start + sizeof(text_filter_globals);
}

static unsigned long text_filter_hash(
	unsigned short const *units,
	long length)
{
	unsigned long hash = 2166136261UL;
	long index;

	for (index = 0; index < length; index++)
		hash = (hash ^ units[index]) * 16777619UL;
	return hash;
}

/* the units with what is hidden as '*' (a wide string's "|n" and a narrow
string's style codes kept, and a wide "|"'s hidden character dropped with
it), into wide or narrow; whether anything was */
static int text_filter_apply(
	unsigned short const *units,
	long length,
	int narrow,
	unsigned short *wide,
	char *narrow_text)
{
	unsigned char *censored = text_filter_globals.censored;
	long index;
	long written = 0;

	if (!text_filter_censor(units, length, narrow, text_filter_globals.levels, censored))
		return 0;
	for (index = 0; index < length; index++)
	{
		unsigned short unit = units[index];

		if (censored[index])
		{
			if (unit == '|' && index + 1 < length)
			{
				unsigned short next = units[index + 1];

				if (next == 'n' || (narrow && next && next < 0x80 && strchr("ibukprlctn", (char)next)))
				{
					/* a line break or a style code: kept */
					if (narrow)
					{
						narrow_text[written++] = '|';
						narrow_text[written++] = (char)next;
					}
					else
					{
						wide[written++] = '|';
						wide[written++] = next;
					}
					index++;
					continue;
				}
				if (!narrow)
				{
					/* a "|" that hides the character after it: both one '*' */
					index++;
				}
				unit = '*';
			}
			else if (unit != '\r' && unit != '\n' && unit != '\t')
			{
				unit = '*';
			}
		}
		if (narrow)
			narrow_text[written++] = (char)unit;
		else
			wide[written++] = unit;
	}
	if (narrow)
		narrow_text[written] = 0;
	else
		wide[written] = 0;
	return 1;
}

/* the string's filtered copy, or the string itself (nothing hidden, or a
string longer than TEXT_FILTER_MAXIMUM_LENGTH, which is drawn as it is) */
static void const *text_filter(
	void const *string,
	int narrow)
{
	unsigned short *units = text_filter_globals.units;
	struct text_filter_cache_entry *entry;
	unsigned long hash;
	long length = 0;

	if (!string || string == text_filter_globals.exempt || text_filter_ours(string) || !text_filter_enabled())
		return string;
	if (narrow)
	{
		unsigned char const *text = (unsigned char const *)string;

		while (text[length] && length <= TEXT_FILTER_MAXIMUM_LENGTH)
		{
			if (length < TEXT_FILTER_MAXIMUM_LENGTH)
				units[length] = text[length];
			length++;
		}
	}
	else
	{
		unsigned short const *text = (unsigned short const *)string;

		while (text[length] && length <= TEXT_FILTER_MAXIMUM_LENGTH)
		{
			if (length < TEXT_FILTER_MAXIMUM_LENGTH)
				units[length] = text[length];
			length++;
		}
	}
	if (!length || length > TEXT_FILTER_MAXIMUM_LENGTH)
		return string;
	if (length >= CACHE_LENGTH)
	{
		/* a long string: filtered each time it is drawn, into one of a few
		buffers in turn */
		long buffer = text_filter_globals.next_long_buffer;

		text_filter_globals.next_long_buffer = (buffer + 1) % LONG_BUFFERS;
		if (!text_filter_apply(units, length, narrow, text_filter_globals.long_buffers[buffer].wide,
			text_filter_globals.long_buffers[buffer].narrow))
		{
			return string;
		}
		return narrow ?
			(void const *)text_filter_globals.long_buffers[buffer].narrow :
			(void const *)text_filter_globals.long_buffers[buffer].wide;
	}
	hash = text_filter_hash(units, length) ^ (unsigned long)narrow;
	entry = &text_filter_globals.cache[hash % CACHE_ENTRIES];
	if (entry->length != length || entry->hash != hash || entry->narrow != narrow ||
		memcmp(entry->original, units, length * sizeof(*units)))
	{
		entry->hash = hash;
		entry->length = (short)length;
		entry->narrow = (unsigned char)narrow;
		memcpy(entry->original, units, length * sizeof(*units));
		entry->clean = !text_filter_apply(units, length, narrow, entry->filtered.wide, entry->filtered.narrow);
	}
	if (entry->clean)
		return string;
	return narrow ? (void const *)entry->filtered.narrow : (void const *)entry->filtered.wide;
}

/* draw_unicode_string's string (draw_string.c) */
wchar_t const *text_filter_unicode(
	wchar_t const *string)
{
	return (wchar_t const *)text_filter(string, 0);
}

/* draw_string's string (draw_string.c) */
char const *text_filter_string(
	char const *string)
{
	return (char const *)text_filter(string, 1);
}

/* what is drawn until text_filter_resume drawn as it is (what the player
types) */
void text_filter_suspend(
	void)
{
	text_filter_globals.suspended++;
	return;
}

void text_filter_resume(
	void)
{
	if (text_filter_globals.suspended > 0)
		text_filter_globals.suspended--;
	return;
}

/* the text (a widget's buffer) drawn as it is while it is being edited;
NULL: none */
void text_filter_exempt(
	void const *text)
{
	text_filter_globals.exempt = text;
	return;
}

/* text shown outside the game's drawing (the platform's message boxes),
UTF-8, filtered in place (a hidden character, of however many bytes, one
'*') */
void text_filter_utf8(
	char *text)
{
	unsigned short *units = text_filter_globals.units;
	unsigned char *censored = text_filter_globals.censored;
	long starts[TEXT_FILTER_MAXIMUM_LENGTH];
	unsigned char const *byte = (unsigned char const *)text;
	long length = 0;
	long index;
	long written = 0;
	long read = 0;
	long total;

	if (!text || !text_filter_enabled())
		return;
	while (*byte && length < TEXT_FILTER_MAXIMUM_LENGTH)
	{
		unsigned long code = *byte;
		long size = 1;

		if (code >= 0xE0 && byte[1] && byte[2])
		{
			code = ((code & 0x0F) << 12) | ((byte[1] & 0x3F) << 6) | (byte[2] & 0x3F);
			size = 3;
		}
		else if (code >= 0xC0 && byte[1])
		{
			code = ((code & 0x1F) << 6) | (byte[1] & 0x3F);
			size = 2;
		}
		starts[length] = (long)(byte - (unsigned char const *)text);
		units[length++] = (unsigned short)code;
		byte += size;
	}
	if (*byte || !text_filter_censor(units, length, 0, text_filter_globals.levels, censored))
		return;
	total = (long)(byte - (unsigned char const *)text);
	for (index = 0; index < length; index++)
	{
		long next = index + 1 < length ? starts[index + 1] : total;

		read = starts[index];
		if (censored[index])
		{
			text[written++] = '*';
		}
		else
		{
			while (read < next)
				text[written++] = text[read++];
		}
	}
	text[written] = 0;
	return;
}
