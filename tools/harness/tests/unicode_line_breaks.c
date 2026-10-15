/*
UNICODE_LINE_BREAKS.C (test)

The real parse_unicode_string (draw_string.c) over strings with a '|': "|n"
ends a line, any other '|' is a character and the one after it is read, and
a string ending in '|' ends at its terminator, not past it (a player's name,
a server's message). test_unicode_line_breaks.py takes the parser's state,
its results (config.inc) and the code (under_test.inc).
*/

#include <wchar.h>
#include "harness.h"

typedef unsigned short word;
typedef unsigned long pixel32;
struct font_header;

#include "config.inc"

#include "under_test.inc"

/* the string parsed from its start, the result each character gave */
static struct parse_string_state state;
static void parse_new(wchar_t const *string)
{
	memset(&state, 0, sizeof(state));
	state.string = (byte *)string;
	/* (a result left from a string parsed before) */
	state.result = _parsed_end_of_line;
}

/* the next character: its result, character and the index after it */
static void expect(short result, word character, short string_index)
{
	parse_unicode_string(&state);
	CHECK(state.result == result && state.character == character && state.string_index == string_index,
		"parsed result %d, character %#x, index %d (wanted %d, %#x, %d)",
		state.result, state.character, state.string_index, result, character, string_index);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	/* "|n" ends the line and is taken whole */
	CASE("bar-n-ends-a-line")
	{
		static wchar_t const string[] = L"a|nb";

		parse_new(string);
		expect(_parsed_character, 'a', 1);
		expect(_parsed_end_of_line, '\r', 3);
		expect(_parsed_character, 'b', 4);
		expect(_parsed_end_of_string, 0, 5);
		return 0;
	}
	/* any other '|' is a character, and the one after it is read */
	CASE("bar-is-a-character")
	{
		static wchar_t const string[] = L"|b|";

		parse_new(string);
		expect(_parsed_character, '|', 1);
		expect(_parsed_character, 'b', 2);
		return 0;
	}
	/* a string ending in '|' ends at its terminator: what lies past it is
	not read */
	CASE("bar-at-the-end")
	{
		static wchar_t const string[] = L"abc|\0XY";

		parse_new(string);
		expect(_parsed_character, 'a', 1);
		expect(_parsed_character, 'b', 2);
		expect(_parsed_character, 'c', 3);
		expect(_parsed_character, '|', 4);
		expect(_parsed_end_of_string, 0, 5);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
