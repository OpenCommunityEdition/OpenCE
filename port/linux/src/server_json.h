/*
SERVER_JSON.H

JSON for a dedicated server's API, events and log (server_json.c): a writer
that grows its text and escapes strings, and a reader over jsmn
(port/third_party/jsmn) for the few request bodies and the master servers'
lists. The game side (port/linux/game/server_*.c) writes with it too: only
pointers, ints and strings cross.
*/

#ifndef __HALO_LINUX_SERVER_JSON_H
#define __HALO_LINUX_SERVER_JSON_H

enum
{
	/* the deepest nesting written */
	SERVER_JSON_MAXIMUM_DEPTH = 24,
	/* the most text written (a reply, an event) */
	SERVER_JSON_MAXIMUM_SIZE = 4 * 1024 * 1024,
};

struct server_json
{
	char *text;
	int length;
	int capacity;
	/* text could not grow, or nesting went too deep: the text is not whole */
	int failed;
	int depth;
	/* whether each open object or array has a member yet */
	unsigned char has_member[SERVER_JSON_MAXIMUM_DEPTH];
};

void server_json_begin(struct server_json *json);
/* the text, NUL terminated ("" after a failure); the writer's to free */
const char *server_json_text(struct server_json const *json);
void server_json_free(struct server_json *json);

/* key is the member's name in an object, NULL in an array or at the top */
void server_json_object_begin(struct server_json *json, const char *key);
void server_json_object_end(struct server_json *json);
void server_json_array_begin(struct server_json *json, const char *key);
void server_json_array_end(struct server_json *json);
/* NULL writes null */
void server_json_string(struct server_json *json, const char *key, const char *value);
/* a string of at most length bytes */
void server_json_string_length(struct server_json *json, const char *key, const char *value, int length);
void server_json_integer(struct server_json *json, const char *key, long long value);
void server_json_real(struct server_json *json, const char *key, double value);
void server_json_boolean(struct server_json *json, const char *key, int value);
void server_json_null(struct server_json *json, const char *key);
/* text already JSON (another writer's), as the member's value */
void server_json_raw(struct server_json *json, const char *key, const char *value);

/* ---------- reading */

enum
{
	/* the most tokens of a document read (a request's body, a message);
	and of a master server's list (512 entries of some 50 tokens each:
	master_list.c) */
	SERVER_JSON_MAXIMUM_TOKENS = 32768,
	SERVER_JSON_MAXIMUM_LIST_TOKENS = 65536,
};

enum
{
	_server_json_undefined = 0,
	_server_json_object = 1,
	_server_json_array = 2,
	_server_json_string_token = 4,
	_server_json_primitive = 8,
};

struct server_json_token
{
	int type;
	int start;
	int end;
	int size;
};

struct server_json_document
{
	const char *text;
	int length;
	int count;
	struct server_json_token *tokens;
};

/* parses text (length bytes; not NUL terminated is fine): 1 on success,
with nesting no deeper than maximum_depth; server_json_document_free frees */
int server_json_parse(struct server_json_document *document, const char *text, int length, int maximum_depth);
/* ... with at most maximum_tokens tokens, not SERVER_JSON_MAXIMUM_TOKENS */
int server_json_parse_tokens(struct server_json_document *document, const char *text, int length, int maximum_depth,
	int maximum_tokens);
void server_json_document_free(struct server_json_document *document);
/* the index of the token after token (and all it holds) */
int server_json_skip(struct server_json_document const *document, int token);
/* the value token of the member key of the object at token, -1 if none */
int server_json_member(struct server_json_document const *document, int token, const char *key);
/* the element at index of the array at token, -1 if none */
int server_json_element(struct server_json_document const *document, int token, int index);
/* a string token's text, unescaped (\uXXXX to UTF-8), into text: 1 if it is
a string that fits (and holds no \u0000) */
int server_json_get_string(struct server_json_document const *document, int token, char *text, int size);
/* a number token's value: 1 if it is a number */
int server_json_get_number(struct server_json_document const *document, int token, double *value);
/* a true or false token's: 1 if it is one */
int server_json_get_boolean(struct server_json_document const *document, int token, int *value);
/* whether the token is null */
int server_json_is_null(struct server_json_document const *document, int token);


/* base64 (RFC 4648, with padding) of bytes, into text (4 * ((size + 2) / 3)
+ 1); and the bytes of base64 text: their count, -1 if it is not */
void server_base64_encode(const unsigned char *bytes, int size, char *text);
int server_base64_decode(const char *text, unsigned char *bytes, int maximum);

#endif
