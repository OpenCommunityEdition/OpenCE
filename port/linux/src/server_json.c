/*
SERVER_JSON.C

JSON written and read for a dedicated server and the server browser's
master servers (server_json.h).
*/

#include "server_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSMN_STATIC
#define JSMN_PARENT_LINKS
#define JSMN_STRICT
#include "../../third_party/jsmn/jsmn.h"

/* ---------- writing */

void server_json_begin(struct server_json *json)
{
	memset(json, 0, sizeof(*json));
}

const char *server_json_text(struct server_json const *json)
{
	return json->text && !json->failed ? json->text : "";
}

void server_json_free(struct server_json *json)
{
	free(json->text);
	memset(json, 0, sizeof(*json));
}

static void json_append(struct server_json *json, const char *text, int length)
{
	if (json->failed || length <= 0)
		return;
	if (json->length + length + 1 > json->capacity)
	{
		int capacity = json->capacity ? json->capacity : 512;
		char *grown;

		while (capacity < json->length + length + 1)
			capacity *= 2;
		if (capacity > SERVER_JSON_MAXIMUM_SIZE || !(grown = realloc(json->text, (size_t)capacity)))
		{
			json->failed = 1;
			return;
		}
		json->text = grown;
		json->capacity = capacity;
	}
	memcpy(json->text + json->length, text, (size_t)length);
	json->length += length;
	json->text[json->length] = 0;
}

static void json_append_text(struct server_json *json, const char *text)
{
	json_append(json, text, (int)strlen(text));
}

/* text as a JSON string's inside: quotes, backslashes and control
characters escaped; bytes that are not UTF-8 written as U+FFFD */
static void json_append_escaped(struct server_json *json, const char *text, int length)
{
	static const char hex[] = "0123456789abcdef";
	int index = 0;

	json_append(json, "\"", 1);
	while (index < length && text[index])
	{
		unsigned char character = (unsigned char)text[index];
		int run = index;

		/* (the plain characters a run at a time) */
		while (run < length && (unsigned char)text[run] >= 0x20 && (unsigned char)text[run] < 0x80 &&
			text[run] != '"' && text[run] != '\\')
		{
			run++;
		}
		if (run > index)
		{
			json_append(json, text + index, run - index);
			index = run;
			continue;
		}
		if (character == '"' || character == '\\')
		{
			char escaped[2] = { '\\', (char)character };

			json_append(json, escaped, 2);
			index++;
		}
		else if (character < 0x20)
		{
			char escaped[6] = { '\\', 'u', '0', '0', hex[character >> 4], hex[character & 15] };

			if (character == '\n')
				json_append(json, "\\n", 2);
			else if (character == '\t')
				json_append(json, "\\t", 2);
			else if (character == '\r')
				json_append(json, "\\r", 2);
			else
				json_append(json, escaped, 6);
			index++;
		}
		else
		{
			/* a UTF-8 sequence, checked whole: no overlong form, surrogate
			or code point past U+10FFFF (RFC 3629's table) */
			int bytes = character >= 0xF0 && character < 0xF5 ? 4 : character >= 0xE0 && character < 0xF0 ? 3 :
				character >= 0xC2 && character < 0xE0 ? 2 : 0;
			int valid = bytes > 0 && index + bytes <= length;
			int byte;

			for (byte = 1; valid && byte < bytes; byte++)
				valid = ((unsigned char)text[index + byte] & 0xC0) == 0x80;
			if (valid && bytes >= 3)
			{
				unsigned char second = (unsigned char)text[index + 1];

				valid = !(character == 0xE0 && second < 0xA0) && !(character == 0xED && second > 0x9F) &&
					!(character == 0xF0 && second < 0x90) && !(character == 0xF4 && second > 0x8F);
			}
			if (valid)
			{
				json_append(json, text + index, bytes);
				index += bytes;
			}
			else
			{
				json_append(json, "\\ufffd", 6);
				index++;
			}
		}
	}
	json_append(json, "\"", 1);
}

/* the separator and key before a member */
static void json_member(struct server_json *json, const char *key)
{
	if (json->depth > 0)
	{
		if (json->has_member[json->depth - 1])
			json_append(json, ",", 1);
		json->has_member[json->depth - 1] = 1;
	}
	if (key)
	{
		json_append_escaped(json, key, (int)strlen(key));
		json_append(json, ":", 1);
	}
}

static void json_open(struct server_json *json, const char *key, const char *bracket)
{
	json_member(json, key);
	json_append_text(json, bracket);
	if (json->depth >= SERVER_JSON_MAXIMUM_DEPTH)
	{
		json->failed = 1;
		return;
	}
	json->has_member[json->depth++] = 0;
}

static void json_close(struct server_json *json, const char *bracket)
{
	if (json->depth > 0)
		json->depth--;
	json_append_text(json, bracket);
}

void server_json_object_begin(struct server_json *json, const char *key)
{
	json_open(json, key, "{");
}

void server_json_object_end(struct server_json *json)
{
	json_close(json, "}");
}

void server_json_array_begin(struct server_json *json, const char *key)
{
	json_open(json, key, "[");
}

void server_json_array_end(struct server_json *json)
{
	json_close(json, "]");
}

void server_json_string(struct server_json *json, const char *key, const char *value)
{
	json_member(json, key);
	if (value)
		json_append_escaped(json, value, (int)strlen(value));
	else
		json_append(json, "null", 4);
}

void server_json_string_length(struct server_json *json, const char *key, const char *value, int length)
{
	json_member(json, key);
	json_append_escaped(json, value, length);
}

void server_json_integer(struct server_json *json, const char *key, long long value)
{
	char text[32];

	json_member(json, key);
	snprintf(text, sizeof(text), "%lld", value);
	json_append_text(json, text);
}

void server_json_real(struct server_json *json, const char *key, double value)
{
	char text[48];

	json_member(json, key);
	/* (JSON has no infinity nor NaN) */
	if (value != value || value > 1e300 || value < -1e300)
		snprintf(text, sizeof(text), "null");
	else
		snprintf(text, sizeof(text), "%.6g", value);
	json_append_text(json, text);
}

void server_json_boolean(struct server_json *json, const char *key, int value)
{
	json_member(json, key);
	json_append_text(json, value ? "true" : "false");
}

void server_json_null(struct server_json *json, const char *key)
{
	json_member(json, key);
	json_append_text(json, "null");
}

void server_json_raw(struct server_json *json, const char *key, const char *value)
{
	json_member(json, key);
	json_append_text(json, value && value[0] ? value : "null");
}

/* ---------- reading */

/* ---------- the grammar (RFC 8259), which jsmn checks only in part: it
takes [1,], {"a":1 "b":2} and control characters in strings */

struct json_check
{
	const char *text;
	int length;
	int at;
	/* the deepest nesting followed (the parse's limit, which also counts
	keys, is checked after) */
	int maximum_depth;
};

static void json_check_space(struct json_check *check)
{
	while (check->at < check->length && (check->text[check->at] == ' ' || check->text[check->at] == '\t' ||
		check->text[check->at] == '\n' || check->text[check->at] == '\r'))
	{
		check->at++;
	}
}

static int json_check_digit(struct json_check *check)
{
	return check->at < check->length && check->text[check->at] >= '0' && check->text[check->at] <= '9';
}

static int json_check_hex(char character)
{
	return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f') ||
		(character >= 'A' && character <= 'F');
}

static int json_check_string(struct json_check *check)
{
	check->at++;
	while (check->at < check->length)
	{
		unsigned char character = (unsigned char)check->text[check->at];

		if (character == '"')
		{
			check->at++;
			return 1;
		}
		if (character < 0x20)
			return 0;
		if (character == '\\')
		{
			char escaped;

			if (++check->at >= check->length)
				return 0;
			escaped = check->text[check->at];
			if (escaped == 'u')
			{
				int index;

				for (index = 1; index <= 4; index++)
				{
					if (check->at + index >= check->length || !json_check_hex(check->text[check->at + index]))
						return 0;
				}
				check->at += 4;
			}
			else if (!strchr("\"\\/bfnrt", escaped) || !escaped)
			{
				return 0;
			}
		}
		check->at++;
	}
	return 0;
}

static int json_check_number(struct json_check *check)
{
	if (check->text[check->at] == '-')
		check->at++;
	if (check->at < check->length && check->text[check->at] == '0')
		check->at++;
	else if (json_check_digit(check))
		while (json_check_digit(check))
			check->at++;
	else
		return 0;
	if (check->at < check->length && check->text[check->at] == '.')
	{
		check->at++;
		if (!json_check_digit(check))
			return 0;
		while (json_check_digit(check))
			check->at++;
	}
	if (check->at < check->length && (check->text[check->at] == 'e' || check->text[check->at] == 'E'))
	{
		check->at++;
		if (check->at < check->length && (check->text[check->at] == '+' || check->text[check->at] == '-'))
			check->at++;
		if (!json_check_digit(check))
			return 0;
		while (json_check_digit(check))
			check->at++;
	}
	return 1;
}

static int json_check_word(struct json_check *check, const char *word)
{
	int length = (int)strlen(word);

	if (check->length - check->at < length || memcmp(check->text + check->at, word, (size_t)length))
		return 0;
	check->at += length;
	return 1;
}

static int json_check_value(struct json_check *check, int depth)
{
	char character;

	json_check_space(check);
	if (check->at >= check->length)
		return 0;
	character = check->text[check->at];
	if (character == '{' || character == '[')
	{
		char close = character == '{' ? '}' : ']';

		if (depth >= check->maximum_depth)
			return 0;
		check->at++;
		json_check_space(check);
		if (check->at < check->length && check->text[check->at] == close)
		{
			check->at++;
			return 1;
		}
		for (;;)
		{
			if (character == '{')
			{
				json_check_space(check);
				if (check->at >= check->length || check->text[check->at] != '"' || !json_check_string(check))
					return 0;
				json_check_space(check);
				if (check->at >= check->length || check->text[check->at] != ':')
					return 0;
				check->at++;
			}
			if (!json_check_value(check, depth + 1))
				return 0;
			json_check_space(check);
			if (check->at >= check->length)
				return 0;
			if (check->text[check->at] == close)
			{
				check->at++;
				return 1;
			}
			if (check->text[check->at] != ',')
				return 0;
			check->at++;
		}
	}
	if (character == '"')
		return json_check_string(check);
	if (character == '-' || (character >= '0' && character <= '9'))
		return json_check_number(check);
	return json_check_word(check, "true") || json_check_word(check, "false") || json_check_word(check, "null");
}

/* whether the text is one JSON value, and nothing but space after it */
static int json_check(const char *text, int length, int maximum_depth)
{
	struct json_check check;

	check.text = text;
	check.length = length;
	check.at = 0;
	check.maximum_depth = maximum_depth + 1;
	if (!json_check_value(&check, 0))
		return 0;
	json_check_space(&check);
	return check.at == length;
}

int server_json_parse_tokens(struct server_json_document *document, const char *text, int length, int maximum_depth,
	int maximum_tokens)
{
	jsmn_parser parser;
	jsmntok_t *tokens;
	int count, index;

	memset(document, 0, sizeof(*document));
	if (!text || length <= 0 || maximum_tokens <= 0 || !json_check(text, length, maximum_depth))
		return 0;
	tokens = malloc(sizeof(jsmntok_t) * (size_t)maximum_tokens);
	if (!tokens)
		return 0;
	jsmn_init(&parser);
	count = jsmn_parse(&parser, text, (size_t)length, tokens, (unsigned int)maximum_tokens);
	if (count <= 0)
	{
		free(tokens);
		return 0;
	}
	document->tokens = malloc(sizeof(struct server_json_token) * (size_t)count);
	if (!document->tokens)
	{
		free(tokens);
		return 0;
	}
	for (index = 0; index < count; index++)
	{
		int depth = 0, parent = tokens[index].parent;

		/* (nesting counted up the parents: a document of a thousand open
		brackets is refused before anything walks it) */
		while (parent >= 0 && depth <= maximum_depth)
		{
			depth++;
			parent = tokens[parent].parent;
		}
		if (depth > maximum_depth)
		{
			free(tokens);
			server_json_document_free(document);
			return 0;
		}
		document->tokens[index].type = tokens[index].type;
		document->tokens[index].start = tokens[index].start;
		document->tokens[index].end = tokens[index].end;
		document->tokens[index].size = tokens[index].size;
	}
	free(tokens);
	document->text = text;
	document->length = length;
	document->count = count;
	return 1;
}

int server_json_parse(struct server_json_document *document, const char *text, int length, int maximum_depth)
{
	return server_json_parse_tokens(document, text, length, maximum_depth, SERVER_JSON_MAXIMUM_TOKENS);
}

void server_json_document_free(struct server_json_document *document)
{
	free(document->tokens);
	memset(document, 0, sizeof(*document));
}

int server_json_skip(struct server_json_document const *document, int token)
{
	int remaining = 1;

	/* (each token's children follow it, size of them: an object's are its
	keys, and each key's its value) */
	while (remaining > 0 && token < document->count)
	{
		remaining += document->tokens[token].size - 1;
		token++;
	}
	return token;
}

static int json_token_equals(struct server_json_document const *document, int token, const char *text)
{
	struct server_json_token const *current = &document->tokens[token];
	int length = (int)strlen(text);

	return current->type == _server_json_string_token && current->end - current->start == length &&
		!memcmp(document->text + current->start, text, (size_t)length);
}

int server_json_member(struct server_json_document const *document, int token, const char *key)
{
	int member, child;

	if (token < 0 || token >= document->count || document->tokens[token].type != _server_json_object)
		return -1;
	child = token + 1;
	for (member = 0; member < document->tokens[token].size && child + 1 < document->count; member++)
	{
		/* (jsmn makes a key's value its child: skipping the key skips both) */
		if (json_token_equals(document, child, key))
			return child + 1;
		child = server_json_skip(document, child);
	}
	return -1;
}

int server_json_element(struct server_json_document const *document, int token, int index)
{
	int element, child;

	if (token < 0 || token >= document->count || document->tokens[token].type != _server_json_array ||
		index < 0 || index >= document->tokens[token].size)
	{
		return -1;
	}
	child = token + 1;
	for (element = 0; element < index && child < document->count; element++)
		child = server_json_skip(document, child);
	return child < document->count ? child : -1;
}

static int json_hex(const char *text, unsigned *value)
{
	int index;

	*value = 0;
	for (index = 0; index < 4; index++)
	{
		char character = text[index];

		*value <<= 4;
		if (character >= '0' && character <= '9')
			*value |= (unsigned)(character - '0');
		else if (character >= 'a' && character <= 'f')
			*value |= (unsigned)(character - 'a' + 10);
		else if (character >= 'A' && character <= 'F')
			*value |= (unsigned)(character - 'A' + 10);
		else
			return 0;
	}
	return 1;
}

static int json_get_string(struct server_json_document const *document, int token, char *text, int size)
{
	struct server_json_token const *current;
	const char *source;
	int length, index, out = 0;

	if (token < 0 || token >= document->count || size <= 0)
		return 0;
	current = &document->tokens[token];
	if (current->type != _server_json_string_token)
		return 0;
	source = document->text + current->start;
	length = current->end - current->start;
	for (index = 0; index < length; index++)
	{
		char character = source[index];
		char encoded[4];
		int encoded_length = 1;

		encoded[0] = character;
		if (character == '\\' && index + 1 < length)
		{
			index++;
			switch (source[index])
			{
			case 'n': encoded[0] = '\n'; break;
			case 't': encoded[0] = '\t'; break;
			case 'r': encoded[0] = '\r'; break;
			case 'b': encoded[0] = '\b'; break;
			case 'f': encoded[0] = '\f'; break;
			case 'u':
			{
				unsigned code;

				if (index + 4 >= length)
					return 0;
				/* (U+0000 refused: a NUL would cut the text the caller reads short,
				past the checks of what follows it) */
				if (!json_hex(source + index + 1, &code) || !code)
					return 0;
				index += 4;
				/* (a surrogate pair's two halves) */
				if (code >= 0xD800 && code < 0xDC00 && index + 6 < length && source[index + 1] == '\\' &&
					source[index + 2] == 'u')
				{
					unsigned low;

					if (json_hex(source + index + 3, &low) && low >= 0xDC00 && low < 0xE000)
					{
						code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
						index += 6;
					}
				}
				if (code >= 0xD800 && code < 0xE000)
					code = 0xFFFD;
				if (code < 0x80)
				{
					encoded[0] = (char)code;
				}
				else if (code < 0x800)
				{
					encoded[0] = (char)(0xC0 | (code >> 6));
					encoded[1] = (char)(0x80 | (code & 0x3F));
					encoded_length = 2;
				}
				else if (code < 0x10000)
				{
					encoded[0] = (char)(0xE0 | (code >> 12));
					encoded[1] = (char)(0x80 | ((code >> 6) & 0x3F));
					encoded[2] = (char)(0x80 | (code & 0x3F));
					encoded_length = 3;
				}
				else
				{
					encoded[0] = (char)(0xF0 | (code >> 18));
					encoded[1] = (char)(0x80 | ((code >> 12) & 0x3F));
					encoded[2] = (char)(0x80 | ((code >> 6) & 0x3F));
					encoded[3] = (char)(0x80 | (code & 0x3F));
					encoded_length = 4;
				}
				break;
			}
			default: encoded[0] = source[index]; break;
			}
		}
		if (out + encoded_length >= size)
			return 0;
		memcpy(text + out, encoded, (size_t)encoded_length);
		out += encoded_length;
	}
	text[out] = 0;
	return 1;
}

int server_json_get_string(struct server_json_document const *document, int token, char *text, int size)
{
	/* (the text empty, and terminated, unless the string was read whole:
	a caller that ignores the answer reads no part of it) */
	if (json_get_string(document, token, text, size))
		return 1;
	if (size > 0)
		text[0] = 0;
	return 0;
}

int server_json_get_number(struct server_json_document const *document, int token, double *value)
{
	struct server_json_token const *current;
	char text[64];
	char *end;
	int length;

	if (token < 0 || token >= document->count)
		return 0;
	current = &document->tokens[token];
	length = current->end - current->start;
	if (current->type != _server_json_primitive || length <= 0 || length >= (int)sizeof(text))
		return 0;
	memcpy(text, document->text + current->start, (size_t)length);
	text[length] = 0;
	/* (a number of JSON's grammar, json_check's, and finite: 1e999 is too) */
	if (!((text[0] >= '0' && text[0] <= '9') || text[0] == '-'))
		return 0;
	*value = strtod(text, &end);
	return *end == 0 && *value - *value == 0.0;
}

int server_json_get_boolean(struct server_json_document const *document, int token, int *value)
{
	struct server_json_token const *current;

	if (token < 0 || token >= document->count)
		return 0;
	current = &document->tokens[token];
	if (current->type != _server_json_primitive)
		return 0;
	if (current->end - current->start == 4 && !memcmp(document->text + current->start, "true", 4))
		*value = 1;
	else if (current->end - current->start == 5 && !memcmp(document->text + current->start, "false", 5))
		*value = 0;
	else
		return 0;
	return 1;
}

int server_json_is_null(struct server_json_document const *document, int token)
{
	struct server_json_token const *current;

	if (token < 0 || token >= document->count)
		return 0;
	current = &document->tokens[token];
	return current->type == _server_json_primitive && current->end - current->start == 4 &&
		!memcmp(document->text + current->start, "null", 4);
}

/* ---------- base64 */

static const char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void server_base64_encode(const unsigned char *bytes, int size, char *text)
{
	int index, out = 0;

	for (index = 0; index < size; index += 3)
	{
		unsigned long group = (unsigned long)bytes[index] << 16;

		if (index + 1 < size)
			group |= (unsigned long)bytes[index + 1] << 8;
		if (index + 2 < size)
			group |= bytes[index + 2];
		text[out++] = base64_alphabet[(group >> 18) & 63];
		text[out++] = base64_alphabet[(group >> 12) & 63];
		text[out++] = index + 1 < size ? base64_alphabet[(group >> 6) & 63] : '=';
		text[out++] = index + 2 < size ? base64_alphabet[group & 63] : '=';
	}
	text[out] = 0;
}

int server_base64_decode(const char *text, unsigned char *bytes, int maximum)
{
	int length = (int)strlen(text);
	int index, count = 0;

	if (length % 4)
		return -1;
	for (index = 0; index < length; index += 4)
	{
		unsigned long group = 0;
		int part, padding = 0;

		for (part = 0; part < 4; part++)
		{
			char character = text[index + part];
			const char *found = character ? strchr(base64_alphabet, character) : NULL;

			if (character == '=' && index + 4 == length && part >= 2)
			{
				padding++;
				group <<= 6;
				continue;
			}
			if (!found || padding)
				return -1;
			group = group << 6 | (unsigned long)(found - base64_alphabet);
		}
		if (count + 3 - padding > maximum)
			return -1;
		bytes[count++] = (unsigned char)(group >> 16);
		if (padding < 2)
			bytes[count++] = (unsigned char)(group >> 8);
		if (padding < 1)
			bytes[count++] = (unsigned char)group;
	}
	return count;
}
