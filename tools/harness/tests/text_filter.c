/*
Streamer mode's matcher (port/linux/game/text_filter_match.c), taken whole: each line read (UTF-8; or, narrow, bytes as
the game's char strings) is written back with the code units it hides as '*'. The levels of SLURS, SWEARING and
INSULTS are the arguments (0 off, 1 mild and worse, 2 strong and worse, 3 severe); the player's own words are always
hidden. "--add word" and "--allow word" before them add a player's word and an allowed one.
*/

#include "harness.h"
#include "under_test.inc"

static long utf8_to_units(char const *text, unsigned short *units, long maximum)
{
	long count = 0;
	unsigned char const *byte = (unsigned char const *)text;

	while (*byte && count < maximum)
	{
		unsigned long code = *byte++;

		if (code >= 0xE0 && byte[0] && byte[1])
		{
			code = ((code & 0x0F) << 12) | ((byte[0] & 0x3F) << 6) | (byte[1] & 0x3F);
			byte += 2;
		}
		else if (code >= 0xC0 && byte[0])
		{
			code = ((code & 0x1F) << 6) | (byte[0] & 0x3F);
			byte += 1;
		}
		units[count++] = (unsigned short)code;
	}
	return count;
}

static void put_unit(unsigned short code)
{
	if (code < 0x80)
	{
		putchar(code);
	}
	else if (code < 0x800)
	{
		putchar(0xC0 | (code >> 6));
		putchar(0x80 | (code & 0x3F));
	}
	else
	{
		putchar(0xE0 | (code >> 12));
		putchar(0x80 | ((code >> 6) & 0x3F));
		putchar(0x80 | (code & 0x3F));
	}
}

int main(int argc, char **argv)
{
	static char line[8192];
	static unsigned short units[TEXT_FILTER_MAXIMUM_LENGTH];
	static unsigned char censored[TEXT_FILTER_MAXIMUM_LENGTH];
	unsigned char levels[NUMBER_OF_TEXT_FILTER_GROUPS] = { 1, 1, 1, 1 };
	int narrow = 0;
	int argument = 1;
	int level = 0;

	if (argc < 2 || strcmp(argv[1], "--censor"))
	{
		fprintf(stderr, "usage: text_filter --censor [--narrow] [--add word] [--allow word] slurs swearing insults\n");
		return 2;
	}
	text_filter_words_reset();
	for (argument = 2; argument < argc; argument++)
	{
		if (!strcmp(argv[argument], "--narrow"))
			narrow = 1;
		else if (!strcmp(argv[argument], "--add") && argument + 1 < argc)
			CHECK(text_filter_word_add(argv[++argument], _text_filter_own, _text_filter_severe, _text_filter_anywhere), "add");
		else if (!strcmp(argv[argument], "--allow") && argument + 1 < argc)
			CHECK(text_filter_allow_add(argv[++argument]), "allow");
		else if (level < 3)
			levels[level++] = (unsigned char)atoi(argv[argument]);
	}
	while (fgets(line, sizeof(line), stdin))
	{
		long length;
		long index;

		line[strcspn(line, "\n")] = 0;
		if (narrow)
		{
			for (length = 0; line[length] && length < TEXT_FILTER_MAXIMUM_LENGTH; length++)
				units[length] = (unsigned char)line[length];
		}
		else
		{
			length = utf8_to_units(line, units, TEXT_FILTER_MAXIMUM_LENGTH);
		}
		text_filter_censor(units, length, narrow, levels, censored);
		for (index = 0; index < length; index++)
			put_unit(censored[index] ? '*' : units[index]);
		putchar('\n');
	}
	return 0;
}
