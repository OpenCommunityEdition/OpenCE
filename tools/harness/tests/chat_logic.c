/*
CHAT_LOGIC.C (test)

The rules of port/linux/game/network_chat.c's text chat: what a line's text
may hold (another machine's is untrusted), how many lines the host takes
from a machine, who hears a team's line, and how long a line shows.
test_chat_logic.py takes the constants and structures (config.inc) and the
code (under_test.inc).
*/

#include "harness.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define csstrlen strlen

#include "config.inc"
#include "under_test.inc"

/* the text cleaned from a source of size bytes */
static short clean(
	char *text,
	char const *source,
	short size)
{
	return chat_text_clean(text, source, size);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	char text[CHAT_TEXT_SIZE];
	char source[256];

	/* printable ASCII kept as it is, the spaces at its ends dropped */
	CASE("text-printable")
	{
		CHECK(clean(text, "  eat my dedi  ", 64) == 11, "length %d", (int)strlen(text));
		CHECK(!strcmp(text, "eat my dedi"), "'%s'", text);
		CHECK(clean(text, "a~!{}", 64) == 5 && !strcmp(text, "a~!{}"), "'%s'", text);
	}

	/* control characters, DEL and bytes past ASCII never reach the screen */
	CASE("text-control-characters")
	{
		CHECK(clean(text, "a\tb\x1b" "c\x7f" "d\xe9", 64) == 8, "length %d", (int)strlen(text));
		CHECK(!strcmp(text, "a?b?c?d?"), "'%s'", text);
	}

	/* a message cut at CHAT_TEXT_SIZE - 1 characters, and never read past
	its buffer when another machine sent it unended */
	CASE("text-bounds")
	{
		memset(source, 'x', sizeof(source));
		CHECK(clean(text, source, sizeof(source)) == CHAT_TEXT_SIZE - 1, "length %d", (int)strlen(text));
		CHECK(text[CHAT_TEXT_SIZE - 1] == 0, "not ended");
		memset(source, 'y', sizeof(source));
		source[10] = 'z';
		CHECK(clean(text, source, 5) == 5 && !strcmp(text, "yyyyy"), "read past its size: '%s'", text);
	}

	/* nothing but spaces is no line */
	CASE("text-empty")
	{
		CHECK(clean(text, "     ", 64) == 0 && !text[0], "'%s'", text);
		CHECK(clean(text, "", 64) == 0, "'%s'", text);
	}

	/* typing and pasting: printable ASCII only, no further than a line holds */
	CASE("append")
	{
		char line[CHAT_TEXT_SIZE] = "hi";

		chat_text_append(line, " there\n\x01!");
		CHECK(!strcmp(line, "hi there!"), "'%s'", line);
		memset(source, 'w', 200);
		source[200] = 0;
		chat_text_append(line, source);
		CHECK(strlen(line) == CHAT_TEXT_SIZE - 1, "length %d", (int)strlen(line));
	}

	/* CHAT_BURST lines at once, then one a period */
	CASE("rate-burst")
	{
		struct chat_rate rate;
		unsigned long now = 1000;
		int index;

		memset(&rate, 0, sizeof(rate));
		for (index = 0; index < CHAT_BURST; index++)
			CHECK(chat_rate_take(&rate, now), "line %d of the burst refused", index);
		CHECK(!chat_rate_take(&rate, now), "a line past the burst taken");
		CHECK(!chat_rate_take(&rate, now + CHAT_REFILL_MILLISECONDS - 1), "a line taken before its period");
		CHECK(chat_rate_take(&rate, now + CHAT_REFILL_MILLISECONDS), "the line a period later refused");
		CHECK(!chat_rate_take(&rate, now + CHAT_REFILL_MILLISECONDS), "two lines in one period");
	}

	/* a machine quiet a long while has its burst again, and no more */
	CASE("rate-refill")
	{
		struct chat_rate rate;
		unsigned long now = 5000;
		int index;
		int taken = 0;

		memset(&rate, 0, sizeof(rate));
		for (index = 0; index < CHAT_BURST; index++)
			chat_rate_take(&rate, now);
		now += 100 * CHAT_REFILL_MILLISECONDS;
		for (index = 0; index < 4 * CHAT_BURST; index++)
			taken += chat_rate_take(&rate, now);
		CHECK(taken == CHAT_BURST, "%d lines taken at once after a long quiet", taken);
	}

	/* the clock wrapping round (system_milliseconds) changes nothing */
	CASE("rate-wrap")
	{
		struct chat_rate rate;
		unsigned long now = 0xFFFFFFFFUL - 200;
		int index;

		memset(&rate, 0, sizeof(rate));
		for (index = 0; index < CHAT_BURST; index++)
			CHECK(chat_rate_take(&rate, now), "line %d refused", index);
		CHECK(!chat_rate_take(&rate, now + 300), "a line taken past the burst across the wrap");
		CHECK(chat_rate_take(&rate, now + CHAT_REFILL_MILLISECONDS), "the line a period later refused across the wrap");
	}

	/* a team's line by that team alone, or by everyone without teams */
	CASE("hears")
	{
		CHECK(chat_hears(_chat_channel_everyone, TRUE, 0, 1, NONE, 7), "a line to everyone not heard by the other team");
		CHECK(chat_hears(_chat_channel_team, TRUE, 1, 1, NONE, NONE), "a team's line not heard by the team");
		CHECK(!chat_hears(_chat_channel_team, TRUE, 0, 1, 7, 7), "a team's line heard by the other team");
		CHECK(chat_hears(_chat_channel_team, FALSE, 0, 1, NONE, NONE), "a team's line not heard without teams");
	}

	/* a vehicle's line by those riding in the sender's vehicle alone, of any
	team; by no one when the sender rides in none */
	CASE("hears-vehicle")
	{
		CHECK(chat_hears(_chat_channel_vehicle, TRUE, 0, 1, 7, 7), "not heard in the same vehicle (another team)");
		CHECK(!chat_hears(_chat_channel_vehicle, TRUE, 0, 0, 7, 8), "heard in another vehicle");
		CHECK(!chat_hears(_chat_channel_vehicle, TRUE, 0, 0, 7, NONE), "heard on foot");
		CHECK(!chat_hears(_chat_channel_vehicle, FALSE, 0, 0, NONE, NONE), "heard on foot from a sender on foot");
	}

	/* a line's channel through its flags, as sent */
	CASE("flags")
	{
		short channel;

		for (channel = 0; channel < NUMBER_OF_CHAT_CHANNELS; channel++)
			CHECK(chat_channel_from_flags(chat_channel_flags(channel)) == channel, "channel %d came back as %d",
				channel, chat_channel_from_flags(chat_channel_flags(channel)));
		CHECK(chat_channel_from_flags(0) == _chat_channel_everyone, "no flags is not everyone");
	}

	/* shown whole, then fading, then gone; whole while a line is typed */
	CASE("alpha")
	{
		CHECK(chat_line_alpha(0, FALSE) == 1.0f, "a new line faded");
		CHECK(chat_line_alpha(CHAT_LINE_MILLISECONDS - 1, FALSE) == 1.0f, "faded early");
		CHECK(chat_line_alpha(CHAT_LINE_MILLISECONDS + CHAT_FADE_MILLISECONDS / 2, FALSE) > 0.4f &&
			chat_line_alpha(CHAT_LINE_MILLISECONDS + CHAT_FADE_MILLISECONDS / 2, FALSE) < 0.6f, "not half way");
		CHECK(chat_line_alpha(CHAT_LINE_MILLISECONDS + CHAT_FADE_MILLISECONDS, FALSE) == 0.0f, "still shown");
		CHECK(chat_line_alpha(10 * CHAT_LINE_MILLISECONDS, TRUE) == 1.0f, "faded while typing");
	}

	return 0;
}
