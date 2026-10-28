/*
SERVER_JSON.C (test)

The real JSON of a dedicated server (port/linux/src/server_json.c): the
writer's escaping, nesting and values, the reader over the vendored jsmn
(its limits on depth and tokens, members, elements, strings unescaped) and
base64. test_server_json.py takes the whole file (under_test.inc) and its
header (server_json.h).
*/

#include "harness.h"

#include <float.h>
#include <limits.h>
#include <math.h>

#include "under_test.inc"

/* what a writer wrote: one call's text, into a static copy */
static const char *written(struct server_json *json)
{
	static char text[4096];

	snprintf(text, sizeof(text), "%s", server_json_text(json));
	server_json_free(json);
	return text;
}

/* text as ASCII, for a failure's message (the harness reads it as UTF-8) */
static const char *visible(const char *text)
{
	static char shown[4096];
	int length = 0;

	for (; *text && length < (int)sizeof(shown) - 5; text++)
	{
		if ((unsigned char)*text >= 0x20 && (unsigned char)*text < 0x7F)
			shown[length++] = *text;
		else
			length += snprintf(shown + length, 5, "\\x%02x", (unsigned char)*text);
	}
	shown[length] = 0;
	return shown;
}

static const char *escaped(const char *value)
{
	struct server_json json;

	server_json_begin(&json);
	server_json_string(&json, NULL, value);
	return written(&json);
}

static const char *escaped_length(const char *value, int length)
{
	struct server_json json;

	server_json_begin(&json);
	server_json_string_length(&json, NULL, value, length);
	return written(&json);
}

static int parse(struct server_json_document *document, const char *text, int maximum_depth)
{
	return server_json_parse(document, text, (int)strlen(text), maximum_depth);
}

/* whether text parses (at a depth of 16, the API's) */
static int parses(const char *text)
{
	struct server_json_document document;
	int ok = parse(&document, text, 16);

	server_json_document_free(&document);
	return ok;
}

/* the string at the top of a document ("..." as text), unescaped into a
buffer of size bytes: 1 if it fits */
static int unescaped(const char *text, char *value, int size)
{
	struct server_json_document document;
	char wrapped[256];
	int ok;

	snprintf(wrapped, sizeof(wrapped), "[%s]", text);
	if (!parse(&document, wrapped, 16))
		return -1;
	ok = server_json_get_string(&document, 1, value, size);
	server_json_document_free(&document);
	return ok;
}

static int same_string(const char *text, const char *expected)
{
	char value[256];

	return unescaped(text, value, sizeof(value)) == 1 && !strcmp(value, expected);
}

static int decoded(const char *text, const char *expected, int maximum)
{
	unsigned char bytes[64];
	int count = server_base64_decode(text, bytes, maximum);

	return count == (int)strlen(expected) && !memcmp(bytes, expected, (size_t)count);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* quotes, backslashes and control characters escaped; UTF-8 kept as it
	is, anything else U+FFFD */
	CASE("escaping")
	{
		CHECK(!strcmp(escaped("plain text"), "\"plain text\""), "wrote %s", visible(escaped("plain text")));
		CHECK(!strcmp(escaped("say \"hi\" \\o/"), "\"say \\\"hi\\\" \\\\o/\""), "wrote %s", visible(escaped("say \"hi\" \\o/")));
		CHECK(!strcmp(escaped("a\nb\tc\rd"), "\"a\\nb\\tc\\rd\""), "wrote %s", visible(escaped("a\nb\tc\rd")));
		CHECK(!strcmp(escaped("\x01\x1f\x08"), "\"\\u0001\\u001f\\u0008\""), "wrote %s", visible(escaped("\x01\x1f\x08")));
		CHECK(!strcmp(escaped("del\x7f"), "\"del\x7f\""), "DEL was not written as it is: %s", visible(escaped("del\x7f")));
		CHECK(!strcmp(escaped("caf\xc3\xa9 \xe2\x82\xac \xf0\x9f\x98\x80"), "\"caf\xc3\xa9 \xe2\x82\xac \xf0\x9f\x98\x80\""),
			"UTF-8 was changed: %s", visible(escaped("caf\xc3\xa9 \xe2\x82\xac \xf0\x9f\x98\x80")));
		/* (a lone continuation byte, an overlong lead, a sequence cut off,
		a lead after a lead) */
		CHECK(!strcmp(escaped("a\x80z"), "\"a\\ufffdz\""), "wrote %s", visible(escaped("a\x80z")));
		CHECK(!strcmp(escaped("\xc0\xaf"), "\"\\ufffd\\ufffd\""), "wrote %s", visible(escaped("\xc0\xaf")));
		CHECK(!strcmp(escaped("\xe2\x82"), "\"\\ufffd\\ufffd\""), "wrote %s", visible(escaped("\xe2\x82")));
		CHECK(!strcmp(escaped("\xc3\xc3\xa9"), "\"\\ufffd\xc3\xa9\""), "wrote %s", visible(escaped("\xc3\xc3\xa9")));
		CHECK(!strcmp(escaped("\xf8"), "\"\\ufffd\""), "wrote %s", visible(escaped("\xf8")));
		/* (a sequence that runs past the length given is not taken whole) */
		CHECK(!strcmp(escaped_length("\xc3\xa9", 1), "\"\\ufffd\""), "wrote %s", visible(escaped_length("\xc3\xa9", 1)));
		CHECK(!strcmp(escaped_length("abcdef", 3), "\"abc\""), "wrote %s", visible(escaped_length("abcdef", 3)));
		CHECK(!strcmp(escaped_length("ab\0cd", 5), "\"ab\""), "wrote past a NUL: %s", visible(escaped_length("ab\0cd", 5)));
		CHECK(!strcmp(escaped(NULL), "null"), "wrote %s", visible(escaped(NULL)));
		CHECK(!strcmp(escaped(""), "\"\""), "wrote %s", visible(escaped("")));
		{
			struct server_json json;

			server_json_begin(&json);
			server_json_object_begin(&json, NULL);
			server_json_string(&json, "k\"ey\n", "v");
			server_json_object_end(&json);
			CHECK(!strcmp(written(&json), "{\"k\\\"ey\\n\":\"v\"}"), "a key was not escaped");
		}
		return 0;
	}
	/* bytes that can never begin UTF-8 (0xF5 to 0xFF) are not written as
	they are */
	CASE("utf8-invalid-leads")
	{
		CHECK(!strcmp(escaped("\xf5\x80\x80\x80"), "\"\\ufffd\\ufffd\\ufffd\\ufffd\""), "wrote %s",
			visible(escaped("\xf5\x80\x80\x80")));
		CHECK(!strcmp(escaped("\xff\x80\x80"), "\"\\ufffd\\ufffd\\ufffd\""), "wrote %s", visible(escaped("\xff\x80\x80")));
		CHECK(!strcmp(escaped("\xf8\x88\x80\x80\x80"), "\"\\ufffd\\ufffd\\ufffd\\ufffd\\ufffd\""), "wrote %s",
			visible(escaped("\xf8\x88\x80\x80\x80")));
		return 0;
	}
	/* overlong forms and surrogates are not UTF-8 either */
	CASE("utf8-overlong-surrogates")
	{
		CHECK(!strcmp(escaped("\xe0\x80\xaf"), "\"\\ufffd\\ufffd\\ufffd\""), "wrote %s", visible(escaped("\xe0\x80\xaf")));
		CHECK(!strcmp(escaped("\xed\xa0\x80"), "\"\\ufffd\\ufffd\\ufffd\""), "wrote %s", visible(escaped("\xed\xa0\x80")));
		CHECK(!strcmp(escaped("\xf4\x90\x80\x80"), "\"\\ufffd\\ufffd\\ufffd\\ufffd\""), "wrote %s",
			visible(escaped("\xf4\x90\x80\x80")));
		return 0;
	}
	/* members separated by commas, keys in objects, none in arrays */
	CASE("nesting")
	{
		struct server_json json;
		const char *text;

		server_json_begin(&json);
		server_json_object_begin(&json, NULL);
		server_json_string(&json, "name", "x");
		server_json_object_begin(&json, "empty");
		server_json_object_end(&json);
		server_json_array_begin(&json, "list");
		server_json_integer(&json, NULL, 1);
		server_json_array_begin(&json, NULL);
		server_json_array_end(&json);
		server_json_object_begin(&json, NULL);
		server_json_boolean(&json, "b", 1);
		server_json_null(&json, "n");
		server_json_object_end(&json);
		server_json_string(&json, NULL, "s");
		server_json_array_end(&json);
		server_json_raw(&json, "raw", "{\"x\":[1,2]}");
		server_json_object_end(&json);
		text = written(&json);
		CHECK(!strcmp(text, "{\"name\":\"x\",\"empty\":{},\"list\":[1,[],{\"b\":true,\"n\":null},\"s\"],"
			"\"raw\":{\"x\":[1,2]}}"), "wrote %s", text);
		/* (what it writes, it reads) */
		CHECK(parses(text), "its own text did not parse: %s", text);

		server_json_begin(&json);
		server_json_array_begin(&json, NULL);
		server_json_array_end(&json);
		CHECK(!strcmp(written(&json), "[]"), "an empty array was not []");
		server_json_begin(&json);
		CHECK(!strcmp(server_json_text(&json), ""), "nothing written was not \"\"");
		server_json_free(&json);
		return 0;
	}
	/* integers, reals (no infinity nor NaN), booleans, null and raw text */
	CASE("values")
	{
		struct server_json json;
		const char *text;

		server_json_begin(&json);
		server_json_array_begin(&json, NULL);
		server_json_integer(&json, NULL, 0);
		server_json_integer(&json, NULL, -42);
		server_json_integer(&json, NULL, LLONG_MAX);
		server_json_integer(&json, NULL, LLONG_MIN);
		server_json_real(&json, NULL, 1.5);
		server_json_real(&json, NULL, -0.1);
		server_json_real(&json, NULL, 1e-7);
		server_json_real(&json, NULL, NAN);
		server_json_real(&json, NULL, INFINITY);
		server_json_real(&json, NULL, -INFINITY);
		server_json_real(&json, NULL, DBL_MAX);
		server_json_boolean(&json, NULL, 0);
		server_json_boolean(&json, NULL, 7);
		server_json_null(&json, NULL);
		server_json_raw(&json, NULL, "");
		server_json_raw(&json, NULL, NULL);
		server_json_raw(&json, NULL, "\"ready\"");
		server_json_array_end(&json);
		text = written(&json);
		CHECK(!strcmp(text, "[0,-42,9223372036854775807,-9223372036854775808,1.5,-0.1,1e-07,null,null,null,null,"
			"false,true,null,null,null,\"ready\"]"), "wrote %s", text);
		CHECK(parses(text), "its own text did not parse: %s", text);
		return 0;
	}
	/* 24 deep is written; deeper, nothing is */
	CASE("write-depth")
	{
		struct server_json json;
		int index;

		server_json_begin(&json);
		for (index = 0; index < SERVER_JSON_MAXIMUM_DEPTH; index++)
			server_json_array_begin(&json, NULL);
		for (index = 0; index < SERVER_JSON_MAXIMUM_DEPTH; index++)
			server_json_array_end(&json);
		CHECK(!json.failed && (int)strlen(server_json_text(&json)) == 2 * SERVER_JSON_MAXIMUM_DEPTH,
			"%d deep was not written", SERVER_JSON_MAXIMUM_DEPTH);
		server_json_free(&json);

		server_json_begin(&json);
		for (index = 0; index <= SERVER_JSON_MAXIMUM_DEPTH; index++)
			server_json_object_begin(&json, index ? "k" : NULL);
		for (index = 0; index <= SERVER_JSON_MAXIMUM_DEPTH; index++)
			server_json_object_end(&json);
		CHECK(json.failed && !strcmp(server_json_text(&json), ""), "%d deep was written",
			SERVER_JSON_MAXIMUM_DEPTH + 1);
		server_json_free(&json);
		return 0;
	}
	/* text that would grow past SERVER_JSON_MAXIMUM_SIZE is not written */
	CASE("write-size")
	{
		struct server_json json;
		int size = SERVER_JSON_MAXIMUM_SIZE / 2;
		char *large = malloc((size_t)size + 1);

		memset(large, 'a', (size_t)size);
		large[size] = 0;
		server_json_begin(&json);
		server_json_array_begin(&json, NULL);
		server_json_string(&json, NULL, large);
		CHECK(!json.failed && json.length == size + 3, "half the most was not written");
		server_json_string(&json, NULL, large);
		CHECK(json.failed && !strcmp(server_json_text(&json), ""), "more than the most was written");
		/* (and once failed, nothing more is) */
		server_json_array_end(&json);
		CHECK(!strcmp(server_json_text(&json), ""), "a failed text was written to");
		server_json_free(&json);
		free(large);
		return 0;
	}
	/* members, elements and values found, others refused */
	CASE("read")
	{
		static const char text[] = "{\"ab\":1,\"a\":{\"x\":[10,{\"y\":null},\"s\"]},\"t\":true,\"f\":false,"
			"\"n\":null,\"r\":-2.5e2,\"s\":\"str\"}trailing";
		struct server_json_document document;
		int a, x, element, value;
		double number;
		char string[16];

		/* (the length given ends the text: what follows is not read) */
		CHECK(server_json_parse(&document, text, (int)(strstr(text, "trailing") - text), 8), "a document was refused");
		CHECK(document.tokens[0].type == _server_json_object, "the top is not an object");
		CHECK(server_json_get_number(&document, server_json_member(&document, 0, "ab"), &number) && number == 1,
			"ab was not 1");
		a = server_json_member(&document, 0, "a");
		CHECK(a > 0 && document.tokens[a].type == _server_json_object, "a was not found after ab");
		CHECK(server_json_member(&document, 0, "") == -1 && server_json_member(&document, 0, "abc") == -1 &&
			server_json_member(&document, 0, "y") == -1, "a member that is not there (or not here) was found");
		x = server_json_member(&document, a, "x");
		CHECK(x > 0 && document.tokens[x].type == _server_json_array && document.tokens[x].size == 3, "x was not found");
		CHECK(server_json_get_number(&document, server_json_element(&document, x, 0), &number) && number == 10,
			"x[0] was not 10");
		element = server_json_element(&document, x, 1);
		CHECK(server_json_is_null(&document, server_json_member(&document, element, "y")), "x[1].y was not null");
		CHECK(server_json_get_string(&document, server_json_element(&document, x, 2), string, sizeof(string)) &&
			!strcmp(string, "s"), "x[2] was not \"s\"");
		CHECK(server_json_element(&document, x, 3) == -1 && server_json_element(&document, x, -1) == -1,
			"an element past the array was found");
		CHECK(server_json_element(&document, 0, 0) == -1 && server_json_member(&document, x, "x") == -1,
			"an object was read as an array, or an array as an object");
		CHECK(server_json_member(&document, -1, "a") == -1 && server_json_member(&document, 999, "a") == -1 &&
			server_json_element(&document, 999, 0) == -1, "a token past the document was read");
		/* (skipping a member's key skips its value with it) */
		CHECK(server_json_skip(&document, a) == server_json_member(&document, 0, "t") - 1, "a was not skipped whole");
		CHECK(server_json_skip(&document, 0) == document.count, "the document was not skipped whole");
		CHECK(server_json_get_boolean(&document, server_json_member(&document, 0, "t"), &value) && value == 1,
			"t was not true");
		CHECK(server_json_get_boolean(&document, server_json_member(&document, 0, "f"), &value) && value == 0,
			"f was not false");
		CHECK(!server_json_get_boolean(&document, server_json_member(&document, 0, "n"), &value) &&
			!server_json_get_boolean(&document, server_json_member(&document, 0, "ab"), &value),
			"null or a number was a boolean");
		CHECK(server_json_get_number(&document, server_json_member(&document, 0, "r"), &number) && number == -250,
			"r was not -250");
		CHECK(!server_json_get_number(&document, server_json_member(&document, 0, "t"), &number) &&
			!server_json_get_number(&document, server_json_member(&document, 0, "s"), &number) &&
			!server_json_get_number(&document, a, &number) && !server_json_get_number(&document, -1, &number),
			"something that is not a number was one");
		CHECK(!server_json_get_string(&document, server_json_member(&document, 0, "ab"), string, sizeof(string)) &&
			!server_json_get_string(&document, -1, string, sizeof(string)), "a number was a string");
		CHECK(!server_json_is_null(&document, server_json_member(&document, 0, "s")) &&
			!server_json_is_null(&document, -1), "a string was null");
		server_json_document_free(&document);
		CHECK(!document.tokens && !document.count, "a freed document kept its tokens");
		return 0;
	}
	/* strings unescaped, \uXXXX (and surrogate pairs) to UTF-8, into what fits */
	CASE("strings")
	{
		char value[16];

		CHECK(same_string("\"a\\nb\\tc\\rd\\be\\ff\"", "a\nb\tc\rd\be\ff"), "the short escapes were not read");
		CHECK(same_string("\"\\\"\\\\\\/\"", "\"\\/"), "a quote, backslash or slash was not read");
		CHECK(same_string("\"\\u0041\\u00e9\\u20AC\"", "A\xc3\xa9\xe2\x82\xac"), "\\u was not read to UTF-8");
		CHECK(same_string("\"\\ud83d\\ude00!\"", "\xf0\x9f\x98\x80!"), "a surrogate pair was not read as one");
		CHECK(same_string("\"\\uDBFF\\uDFFF\"", "\xf4\x8f\xbf\xbf"), "the last code point was not read");
		CHECK(same_string("\"\\ud83dx\"", "\xef\xbf\xbdx"), "a lone high surrogate was not U+FFFD");
		CHECK(same_string("\"\\ude00\"", "\xef\xbf\xbd"), "a lone low surrogate was not U+FFFD");
		CHECK(same_string("\"\\ud83d\\u0041\"", "\xef\xbf\xbd" "A"), "a high surrogate before a letter was not U+FFFD");
		CHECK(same_string("\"\\ud83d\\ud83d\\ude00\"", "\xef\xbf\xbd\xf0\x9f\x98\x80"),
			"a high surrogate before a pair was not U+FFFD and the pair");
		CHECK(same_string("\"caf\xc3\xa9\"", "caf\xc3\xa9"), "UTF-8 was changed");
		CHECK(same_string("\"\"", ""), "an empty string was not read");
		/* (exactly what fits, and a byte too many) */
		CHECK(unescaped("\"1234567\"", value, 8) == 1 && !strcmp(value, "1234567"), "a string that fits was refused");
		memset(value, '#', sizeof(value));
		CHECK(unescaped("\"12345678\"", value, 8) == 0, "a string that does not fit was taken");
		CHECK(value[8] == '#', "a string that does not fit was written past its room");
		CHECK(unescaped("\"\\u20ac\"", value, 4) == 1 && unescaped("\"\\u20ac\"", value, 3) == 0,
			"a character's bytes were split at the end of the room");
		CHECK(unescaped("\"\"", value, 0) == 0, "a string was read into no room");
		return 0;
	}
	/* text that is not JSON (as jsmn reads it strictly) is refused */
	CASE("malformed")
	{
		static const char *const refused[] = {
			"", "{", "[", "{\"a\":1", "[1,2", "{\"a\":\"b", "{\"a\" 1}", "{\"a\",1}", "{1:2}", "[1}", "{]",
			"\"open", "[\"a\\x\"]", "[\"\\u12\"]", "[\"\\u12zz\"]", "{} x",
			/* (a number, true or null at the top: jsmn's strict mode wants a
			delimiter after one) */
			"1", "true", "null",
			/* (what jsmn's strict mode takes, the grammar's check does not) */
			"[1,]", "{\"a\":1,}", "[1 2]", "{\"a\":1 \"b\":2}", "[,1]", "[\"a\":1]", "{\"a\":}", "{}{}", "[01]",
			"[\"a\x01\"]", "[\"a\"\"b\"]", "[1e]", "[.5]", "[+1]", "[True]",
			NULL,
		};
		struct server_json_document document;
		int index;

		for (index = 0; refused[index]; index++)
			CHECK(!parses(refused[index]), "%s was taken", refused[index]);
		CHECK(!server_json_parse(&document, NULL, 4, 8) && !server_json_parse(&document, "[1]", 0, 8) &&
			!server_json_parse(&document, "[1]", -1, 8), "nothing, or a length of none, was taken");
		/* (cut off anywhere, a document is refused) */
		{
			static const char whole[] = "{\"a\":[1,{\"b\":\"c\\u00e9\"}],\"d\":true}";
			int length;

			for (length = 1; length < (int)sizeof(whole) - 1; length++)
			{
				CHECK(!server_json_parse(&document, whole, length, 8), "%.*s was taken", length, whole);
				CHECK(!document.tokens, "a refused document kept tokens");
			}
			CHECK(server_json_parse(&document, whole, length, 8), "the whole document was refused");
			server_json_document_free(&document);
		}
		return 0;
	}
	/* what jsmn takes as a primitive (it reads only the first character) is
	refused unless it is a number, boolean or null */
	CASE("primitives")
	{
		static const char *const neither[] = { "[tru]", "[nul]", "[-]", "[nan]", "[truex]", "[nullx]", "[falsey]", "[1_0]",
			"[0.5.5]", NULL };
		struct server_json_document document;
		double number;
		int index, value;

		for (index = 0; neither[index]; index++)
		{
			CHECK(!parse(&document, neither[index], 8), "%s was taken", neither[index]);
		}
		CHECK(parse(&document, "[-0.5e+1,1E2]", 8), "numbers were refused");
		CHECK(server_json_get_number(&document, 1, &number) && number == -5, "-0.5e+1 was not -5");
		CHECK(server_json_get_number(&document, 2, &number) && number == 100, "1E2 was not 100");
		server_json_document_free(&document);
		return 0;
	}
	/* only JSON's numbers are numbers: not strtod's NaN, infinities or hex,
	which pass every range check a caller makes ("minutes", "time") */
	CASE("numbers-json-only")
	{
		static const char *const refused[] = { "[-nan]", "[-inf]", "[-infinity]", "[0x10]", "[0x1p3]", "[1e999]", NULL };
		struct server_json_document document;
		double number;
		int index;

		for (index = 0; refused[index]; index++)
		{
			CHECK(!parse(&document, refused[index], 8) || !server_json_get_number(&document, 1, &number),
				"%s was read as %g", refused[index], number);
			server_json_document_free(&document);
		}
		return 0;
	}
	/* nesting deeper than asked is refused (each key a level, as jsmn makes
	a key its value's parent) */
	CASE("read-depth")
	{
		struct server_json_document document;
		char deep[4096];
		int index;

		CHECK(parse(&document, "[[1]]", 2), "[[1]] was refused at a depth of 2");
		server_json_document_free(&document);
		CHECK(!parse(&document, "[[[1]]]", 2), "[[[1]]] was taken at a depth of 2");
		CHECK(parse(&document, "{\"a\":1}", 2), "{\"a\":1} was refused at a depth of 2");
		server_json_document_free(&document);
		CHECK(!parse(&document, "{\"a\":[1]}", 2), "{\"a\":[1]} was taken at a depth of 2");
		CHECK(parse(&document, "[]", 0), "[] was refused at a depth of 0");
		server_json_document_free(&document);
		CHECK(!parse(&document, "[1]", 0), "[1] was taken at a depth of 0");
		/* (a thousand open brackets) */
		for (index = 0; index < 1000; index++)
		{
			deep[index] = '[';
			deep[1000 + index] = ']';
		}
		deep[2000] = 0;
		CHECK(!parse(&document, deep, 16), "a thousand deep was taken");
		CHECK(parse(&document, deep, 1000), "a thousand deep was refused at a depth of 1000");
		server_json_document_free(&document);
		return 0;
	}
	/* a document of SERVER_JSON_MAXIMUM_TOKENS is read, one of more is not */
	CASE("tokens")
	{
		struct server_json_document document;
		int elements = SERVER_JSON_MAXIMUM_TOKENS - 1;
		char *text = malloc((size_t)(2 * elements + 8));
		int index, length = 0;

		text[length++] = '[';
		for (index = 0; index < elements; index++)
		{
			text[length++] = '0';
			text[length++] = ',';
		}
		text[length - 1] = ']';
		CHECK(server_json_parse(&document, text, length, 8) && document.count == SERVER_JSON_MAXIMUM_TOKENS,
			"%d tokens were refused", SERVER_JSON_MAXIMUM_TOKENS);
		CHECK(server_json_element(&document, 0, elements - 1) == SERVER_JSON_MAXIMUM_TOKENS - 1,
			"the last element was not found");
		server_json_document_free(&document);
		text[length - 1] = ',';
		text[length++] = '0';
		text[length++] = ']';
		CHECK(!server_json_parse(&document, text, length, 8), "%d tokens were taken", SERVER_JSON_MAXIMUM_TOKENS + 1);
		free(text);
		return 0;
	}
	/* a master's list reads up to SERVER_JSON_MAXIMUM_LIST_TOKENS, a body
	of the API still SERVER_JSON_MAXIMUM_TOKENS */
	CASE("list-tokens")
	{
		struct server_json_document document;
		int elements = SERVER_JSON_MAXIMUM_LIST_TOKENS - 1;
		char *text = malloc((size_t)(2 * elements + 8));
		int index, length = 0;

		CHECK(SERVER_JSON_MAXIMUM_LIST_TOKENS > SERVER_JSON_MAXIMUM_TOKENS, "a list reads no more than a body");
		text[length++] = '[';
		for (index = 0; index < elements; index++)
		{
			text[length++] = '0';
			text[length++] = ',';
		}
		text[length - 1] = ']';
		CHECK(server_json_parse_tokens(&document, text, length, 8, SERVER_JSON_MAXIMUM_LIST_TOKENS) &&
			document.count == SERVER_JSON_MAXIMUM_LIST_TOKENS, "a list of %d tokens was refused",
			SERVER_JSON_MAXIMUM_LIST_TOKENS);
		server_json_document_free(&document);
		CHECK(!server_json_parse(&document, text, length, 8), "a body of %d tokens was taken",
			SERVER_JSON_MAXIMUM_LIST_TOKENS);
		text[length - 1] = ',';
		text[length++] = '0';
		text[length++] = ']';
		CHECK(!server_json_parse_tokens(&document, text, length, 8, SERVER_JSON_MAXIMUM_LIST_TOKENS),
			"a list of %d tokens was taken", SERVER_JSON_MAXIMUM_LIST_TOKENS + 1);
		CHECK(!server_json_parse_tokens(&document, "[]", 2, 8, 0), "a document was read into no tokens");
		free(text);
		return 0;
	}
	/* \u0000 is refused (the text read would end there, past the checks of
	what follows it); the other control characters are read, for the callers
	to refuse */
	CASE("nul-refused")
	{
		char value[16];

		memset(value, '#', sizeof(value));
		CHECK(unescaped("\"\\u0000x\"", value, sizeof(value)) == 0 && !value[0], "\\u0000x was read as \"%s\"",
			visible(value));
		CHECK(unescaped("\"x\\u0000\"", value, sizeof(value)) == 0 && !value[0], "x\\u0000 was read as \"%s\"",
			visible(value));
		CHECK(same_string("\"\\u0001\\u001f\\u0030\"", "\x01\x1f" "0"), "\\u0001, \\u001f or \\u0030 was not read");
		return 0;
	}
	/* RFC 4648's examples, every length round trip, bad text refused, the
	most bytes kept to */
	CASE("base64")
	{
		static const char *const examples[][2] = {
			{ "", "" }, { "f", "Zg==" }, { "fo", "Zm8=" }, { "foo", "Zm9v" }, { "foob", "Zm9vYg==" },
			{ "fooba", "Zm9vYmE=" }, { "foobar", "Zm9vYmFy" },
		};
		static const char *const refused[] = {
			"Zg=", "Zm9", "Zm9vY", "Zg=a", "Z===", "====", "Zg==Zg==", "Zm9v!A==", "Zm9v YQ=", "Zm9v\nYQ=", "Zm-v",
			"Zm_v", "\xc3\xa9Zg", NULL,
		};
		unsigned char bytes[96], back[96];
		char text[160];
		int index, size;

		for (index = 0; index < (int)(sizeof(examples) / sizeof(examples[0])); index++)
		{
			server_base64_encode((const unsigned char *)examples[index][0], (int)strlen(examples[index][0]), text);
			CHECK(!strcmp(text, examples[index][1]), "%s was written %s", examples[index][0], text);
			CHECK(decoded(examples[index][1], examples[index][0], 64), "%s was not read", examples[index][1]);
		}
		for (index = 0; index < (int)sizeof(bytes); index++)
			bytes[index] = (unsigned char)(index * 151 + 7);
		for (size = 0; size <= (int)sizeof(bytes); size++)
		{
			server_base64_encode(bytes, size, text);
			CHECK((int)strlen(text) == 4 * ((size + 2) / 3), "%d bytes were written in %d characters", size,
				(int)strlen(text));
			CHECK(server_base64_decode(text, back, (int)sizeof(back)) == size && !memcmp(bytes, back, (size_t)size),
				"%d bytes did not come back", size);
		}
		server_base64_encode((const unsigned char *)"\xfb\xff\xbf", 3, text);
		CHECK(!strcmp(text, "+/+/"), "the last two characters were not + and /: %s", text);
		for (index = 0; refused[index]; index++)
		{
			CHECK(server_base64_decode(refused[index], back, (int)sizeof(back)) == -1, "%s was read",
				visible(refused[index]));
		}
		/* (the most is kept to exactly, padding counted out) */
		memset(back, '#', sizeof(back));
		CHECK(decoded("Zm9vYmFy", "foobar", 6) && server_base64_decode("Zm9vYmFy", back, 5) == -1,
			"six bytes were not read into six, or were into five");
		CHECK(decoded("Zm9vYg==", "foob", 4) && server_base64_decode("Zm9vYg==", back, 3) == -1,
			"four bytes were not read into four, or were into three");
		CHECK(decoded("Zm9vYmE=", "fooba", 5) && server_base64_decode("Zm9vYmE=", back, 4) == -1,
			"five bytes were not read into five, or were into four");
		CHECK(back[4] == '#', "bytes were written past the most");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
