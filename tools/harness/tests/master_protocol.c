/*
MASTER_PROTOCOL.C (test)

The game's side of the master servers' protocol (docs/master_protocol.md),
byte for byte against the master's test vectors (vectors.inc, from its
testdata/vectors.json): the server browser's check of a signed list, the
entries it takes from one and its masters.txt (port/linux/src/
master_list.c); the host's listing read and its signature checked
(port/linux/src/p2p_lobby.c); a dedicated server's publisher key, instance,
server id, signed requests, its publishes and what it makes of their
answers, and its answer to a master's probe and the probes' limits
(port/linux/src/server_master.c); on the real SHA-256 and Ed25519
(port/linux/src/p2p_crypto.c, Monocypher) and base64 and JSON
(server_json.c): test_master_protocol.py takes the code (under_test.inc).
*/

#include "harness.h"

#include <ctype.h>
#include <stdint.h>

/* ---------- the crypto, base64 and JSON, as whole files */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
/* (compiled quickly: each negative control builds it again) */
#pragma clang optimize off
#include "monocypher.inc"
/* (both files have a hash_reduce of their own) */
#define hash_reduce ed25519_hash_reduce
#include "monocypher_ed25519.inc"
#undef hash_reduce
#pragma clang optimize on
#pragma clang diagnostic pop
#undef FOR
#undef COPY
#undef ZERO
#undef WIPE_CTX
#undef WIPE_BUFFER
#undef MIN

#include "p2p_crypto.inc"
#include "server_json.inc"

/* bytes the code asks for at random: the test's */
static unsigned char random_bytes[64];

void posix_random_bytes(void *buffer, posix_ulong size)
{
	memcpy(buffer, random_bytes, size);
}

#include "master_list.h"
#include "server_master.h"

/* ---------- the fake world */

struct vector_listing
{
	const char *description;
	const char *host_seed;
	const char *host_public_key;
	const char *bytes;
	const char *base64;
	int size;
	int signed_size;
	const char *signed_message;
	const char *signature;
	int valid;
};

#include "vectors.inc"

typedef struct SDL_Mutex SDL_Mutex;
typedef struct SDL_Condition SDL_Condition;
typedef struct SDL_Thread SDL_Thread;
#define SDL_CreateMutex() NULL
#define SDL_CreateCondition() NULL
#define SDL_CreateThread(function, name, data) NULL
#define SDL_LockMutex(lock) ((void)0)
#define SDL_UnlockMutex(lock) ((void)0)
/* a dedicated server's publisher (server_mode.c) unless a case says it is
a player's host; which the game's host started as the process exits
(atexit) never is here: no thread */
static int server_mode = 1;
static int halo_server_mode(void) { return server_mode; }
void server_master_stop(void) {}
#define SDL_GetBasePath() ((const char *)NULL)
#define server_log(...) ((void)0)
static const char *server_mode_build(void) { return "build-206"; }

/* the settings and files the code reads */
static const char *setting_instance = "";
static const char *setting_name = "";
static const char *setting_region = "";
static int setting_publish_address;
static const char *file_masters = NULL;
static const char *file_key = NULL;
static unsigned long now;

static const char *config_string(const char *name)
{
	if (!strcmp(name, "server.instance"))
		return setting_instance;
	if (!strcmp(name, "server.name"))
		return setting_name;
	if (!strcmp(name, "server.region"))
		return setting_region;
	if (!strcmp(name, "server.masters_file") || !strcmp(name, "network.masters_file"))
		return "masters.txt";
	return "";
}

static int config_boolean(const char *name)
{
	return !strcmp(name, "server.master_publish_address") && setting_publish_address;
}

/* the hosted game (p2p.c, p2p_lobby.c): its listing (none: not hosting for
the internet), the tunnel's port (network byte order), its public address */
static unsigned char host_listing[400];
static int host_listing_size;
static unsigned short tunnel_port;

static int p2p_lobby_host_listing(unsigned char *bytes, int size)
{
	if (!host_listing_size || size < host_listing_size)
		return 0;
	memcpy(bytes, host_listing, (size_t)host_listing_size);
	return host_listing_size;
}

static unsigned short p2p_tunnel_local_port(void) { return tunnel_port; }

static int p2p_public_address(char *text, int size)
{
	snprintf(text, (size_t)size, "203.0.113.5:2302");
	return 1;
}

/* the events of the master's outcomes (server_events.c) */
static int events_added;

static unsigned long server_event_add(const char *group, const char *object)
{
	(void)group;
	(void)object;
	return (unsigned long)++events_added;
}

/* a master's entries handed to the server browser's games (p2p_lobby.c):
the first few kept */
static struct
{
	int size;
	char publisher[64];
	char region[64];
	int version;
	int dedicated;
	unsigned char master_key_id[8];
} entries_taken[4];
static int entries_count;

static int p2p_lobby_take_master_listing(const unsigned char *payload, int size, const char *publisher,
	const char *region, int network_version_shown, int dedicated, const unsigned char *master_key_id)
{
	(void)payload;
	if (entries_count < 4)
	{
		memcpy(entries_taken[entries_count].master_key_id, master_key_id, 8);
		entries_taken[entries_count].size = size;
		snprintf(entries_taken[entries_count].publisher, sizeof(entries_taken[entries_count].publisher), "%s", publisher);
		snprintf(entries_taken[entries_count].region, sizeof(entries_taken[entries_count].region), "%s", region);
		entries_taken[entries_count].version = network_version_shown;
		entries_taken[entries_count].dedicated = dedicated;
	}
	entries_count++;
	return 1;
}

static void config_folder(char *path, size_t size)
{
	snprintf(path, size, "/config/");
}

static char *config_file_read(const char *path, size_t *size)
{
	const char *text = !strcmp(path, "/config/masters.txt") ? file_masters :
		!strcmp(path, "/config/publisher.key") ? file_key : NULL;
	char *copy;

	if (!text)
		return NULL;
	*size = strlen(text);
	copy = malloc(*size + 1);
	memcpy(copy, text, *size + 1);
	return copy;
}

static unsigned long posix_server_unix_seconds(void)
{
	return now;
}

/* what the code signs, kept */
static unsigned char signed_message[4096];
static int signed_size;

static void test_sign(const unsigned char *seed, const unsigned char *public_key, const void *message, int size,
	unsigned char *signature)
{
	memcpy(signed_message, message, (size_t)size < sizeof(signed_message) ? (size_t)size : sizeof(signed_message));
	signed_size = size;
	p2p_ed25519_sign(seed, public_key, message, size, signature);
}
#define p2p_ed25519_sign(seed, public_key, message, size, signature) test_sign(seed, public_key, message, size, signature)

#include "config.inc"

/* the publishes made (update.h's update_https_request), and the master's
answer to them */
static struct
{
	int count;
	char method[8];
	char url[512];
	char headers[1024];
	char body[16384];
} requested;
static int answer_status = 200;
static const char *answer_headers = "";
static const char *answer_body = "{\"ok\":true,\"listed\":true,\"probe\":\"ok\"}";

static int update_https_request(const char *method, const char *url, const char *headers, const void *body,
	unsigned long body_size, unsigned long maximum, struct update_response *response, char *error, int error_size)
{
	(void)maximum;
	(void)error;
	(void)error_size;
	requested.count++;
	snprintf(requested.method, sizeof(requested.method), "%s", method);
	snprintf(requested.url, sizeof(requested.url), "%s", url);
	snprintf(requested.headers, sizeof(requested.headers), "%s", headers ? headers : "");
	snprintf(requested.body, sizeof(requested.body), "%.*s", (int)body_size, body ? (const char *)body : "");
	memset(response, 0, sizeof(*response));
	response->status = answer_status;
	snprintf(response->headers, sizeof(response->headers), "%s", answer_headers);
	response->body = strdup(answer_body);
	response->body_size = (unsigned long)strlen(answer_body);
	return 1;
}

#include "under_test.inc"

/* ---------- the test's own */

static int hex_decode(const char *text, unsigned char *bytes, int maximum)
{
	int count = 0;

	while (text[0] && text[1] && count < maximum)
	{
		unsigned int value;

		if (sscanf(text, "%2x", &value) != 1)
			return -1;
		bytes[count++] = (unsigned char)value;
		text += 2;
	}
	return count;
}

static const char *hex_of(const void *bytes, int size)
{
	static char text[2][2048];
	static int which;

	which ^= 1;
	p2p_hex(bytes, size, text[which]);
	return text[which];
}

static void key_of(const char *hex, unsigned char *key)
{
	CHECK(hex_decode(hex, key, 32) == 32, "a vector's key is not 32 bytes: %s", hex);
}

/* the publisher started as the vectors have it: its key, instance "main",
the vectors' master in masters.txt */
static void start_publisher(void)
{
	static char masters[256];

	snprintf(masters, sizeof(masters), "# the vectors' master\nhttps://master.example.org %s\n",
		VECTOR_MASTER_PUBLIC_KEY_TEXT);
	file_masters = masters;
	setenv("HALO_SERVER_PUBLISHER_KEY", VECTOR_PUBLISHER_SEED, 1);
	setting_instance = VECTOR_INSTANCE;
	memset(&master, 0, sizeof(master));
	server_master_start();
	CHECK(master.has_key, "the publisher key was not taken from HALO_SERVER_PUBLISHER_KEY");
	CHECK(master.master_count == 1, "%d masters read from masters.txt", master.master_count);
}

/* a request signed as the vectors' was: the Authorization header and the
string signed */
static void check_request(const char *master_key_id, const char *method, const char *path, const char *body,
	unsigned long size, const char *ts, const char *nonce, const char *canonical, const char *authorization)
{
	char header[512], expected[512];
	unsigned char master_key_id_bytes[8];

	start_publisher();
	now = strtoul(ts, NULL, 10);
	CHECK(hex_decode(nonce, random_bytes, 16) == 16, "the vector's nonce is not 16 bytes");
	CHECK(hex_decode(master_key_id, master_key_id_bytes, 8) == 8, "the vector's master key id is not 8 bytes");
	signed_size = -1;
	sign_request(master_key_id_bytes, method, path, body, size, header, sizeof(header));
	CHECK(signed_size == (int)strlen(canonical) && !memcmp(signed_message, canonical, strlen(canonical)),
		"signed \"%.*s\", not the vector's \"%s\"", signed_size, (const char *)signed_message, canonical);
	snprintf(expected, sizeof(expected), "Authorization: %s\r\nContent-Type: application/json\r\n", authorization);
	CHECK(!strcmp(header, expected), "the header is\n%s\nnot\n%s", header, expected);
}

static int verify_list(const char *body, unsigned long size, const char *signature_header, const char *key_header,
	const char *key_hex, unsigned long when)
{
	unsigned char key[32];
	char why[128] = "";

	key_of(key_hex, key);
	return master_list_verify(body, size, signature_header, key_header, key, when, why, sizeof(why));
}

/* a list body signed by the vectors' master: its Signature header */
static const char *master_signs(const char *body)
{
	static char header[128];
	unsigned char seed[32], public_key[32], signature[64];

	key_of(VECTOR_MASTER_SEED, seed);
	p2p_ed25519_public(seed, public_key, NULL);
	p2p_ed25519_sign(seed, public_key, body, (int)strlen(body), signature);
	memcpy(header, "ed25519=", 8);
	server_base64_encode(signature, 64, header + 8);
	return header;
}

/* a probe from an address (network byte order) answered: 1 */
static int probe_from(const unsigned char *probe, int size, unsigned long address, unsigned char *answer)
{
	int answer_size = -1;
	int answered = server_master_probe(probe, size, address, answer, &answer_size);

	CHECK(!answered || answer_size == PROBE_ANSWER_SIZE, "an answer of %d bytes", answer_size);
	return answered;
}

/* ... from the vectors' master's address */
static int probe_answered(const unsigned char *probe, int size, unsigned char *answer)
{
	return probe_from(probe, size, 0x0100A8C0UL, answer);
}

/* a listing as listing_make_token writes one (flags, the names' lengths;
a password's token sealed), signed by the vectors' host: its size */
static int make_listing(unsigned char *bytes, int format, int flags, int name_length, int map_length,
	int gametype_length)
{
	unsigned char seed[32], public_key[32], data[512];
	int lengths[3], size = 0, index, text;

	lengths[0] = name_length;
	lengths[1] = map_length;
	lengths[2] = gametype_length;
	key_of(VECTOR_HOST_SEED, seed);
	p2p_ed25519_public(seed, public_key, NULL);
	bytes[size++] = 'H';
	bytes[size++] = 'L';
	bytes[size++] = (unsigned char)format;
	bytes[size++] = 0;
	bytes[size++] = 39;
	bytes[size++] = (unsigned char)flags;
	for (index = 0; index < 4; index++)
		bytes[size++] = (unsigned char)(index == 3 ? 9 : 0);
	for (index = 0; index < 4; index++)
		bytes[size++] = (unsigned char)(1760900000UL >> (24 - 8 * index));
	memcpy(bytes + size, public_key, 32);
	size += 32;
	for (index = 0; index < ((flags & 16) ? P2P_SEALED_TOKEN_SIZE : P2P_TOKEN_SIZE); index++)
		bytes[size++] = (unsigned char)(0xB0 + index);
	bytes[size++] = 3;
	bytes[size++] = 16;
	bytes[size++] = 2;
	for (text = 0; text < 3; text++)
	{
		bytes[size++] = (unsigned char)lengths[text];
		for (index = 0; index < lengths[text]; index++)
			bytes[size++] = (unsigned char)('a' + text);
	}
	memset(bytes + size, 0, 8);
	bytes[size] = 3;
	size += 8;
	memcpy(data, "hceu-lobby-1", 12);
	memcpy(data + 12, bytes, (size_t)size);
	p2p_ed25519_sign(seed, public_key, data, 12 + size, bytes + size);
	return size + 64;
}

/* whether listing_read takes the bytes, and listing_signed their
signature */
static int listing_good(const unsigned char *bytes, int size)
{
	struct listing listing;

	return listing_read(bytes, size, &listing) && listing_signed(bytes, &listing);
}

/* the vectors' first listing as the hosted game's */
static void host_vector_listing(void)
{
	host_listing_size = hex_decode(vector_listings[0].bytes, host_listing, sizeof(host_listing));
	CHECK(host_listing_size == vector_listings[0].size, "the vectors' first listing is %d bytes", host_listing_size);
}

/* a JSON body's member, as text: its string's value, or its number or
boolean as written */
static const char *body_member(const char *body, const char *name)
{
	static char value[1024];
	struct server_json_document document;
	int token;

	value[0] = 0;
	if (!server_json_parse(&document, body, (int)strlen(body), 8))
		return "(not JSON)";
	token = server_json_member(&document, 0, name);
	if (token < 0)
		snprintf(value, sizeof(value), "(none)");
	else if (document.tokens[token].type == _server_json_string_token)
		server_json_get_string(&document, token, value, sizeof(value));
	else
		snprintf(value, sizeof(value), "%.*s", document.tokens[token].end - document.tokens[token].start,
			body + document.tokens[token].start);
	server_json_document_free(&document);
	return value;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* the vectors' keys, ids and hashes from the seeds; a small-order key
	refused */
	CASE("crypto")
	{
		const char *seeds[] = { VECTOR_PUBLISHER_SEED, VECTOR_MASTER_SEED, VECTOR_HOST_SEED };
		const char *keys[] = { VECTOR_PUBLISHER_PUBLIC_KEY, VECTOR_MASTER_PUBLIC_KEY, VECTOR_HOST_PUBLIC_KEY };
		const char *ids[] = { VECTOR_PUBLISHER_KEY_ID, VECTOR_MASTER_KEY_ID, VECTOR_HOST_KEY_ID };
		unsigned char seed[32], public_key[32], digest[32], x25519[32], signature[64];
		int index;

		for (index = 0; index < 3; index++)
		{
			key_of(seeds[index], seed);
			p2p_ed25519_public(seed, public_key, NULL);
			CHECK(!strcmp(hex_of(public_key, 32), keys[index]), "the key of seed %d is %s", index,
				hex_of(public_key, 32));
			p2p_sha256(public_key, 32, digest);
			CHECK(!strcmp(hex_of(digest, 8), ids[index]), "key id %s, not %s", hex_of(digest, 8), ids[index]);
		}
		CHECK(!strcmp(VECTOR_MASTER_KEY_ID_BYTES, VECTOR_MASTER_KEY_ID), "the master's key id bytes are its id");
		p2p_sha256("", 0, digest);
		CHECK(!strcmp(hex_of(digest, 32), VECTOR_ME_BODY_SHA256), "SHA-256 of nothing is %s", hex_of(digest, 32));
		p2p_sha256(VECTOR_PUBLISH_BODY, (int)strlen(VECTOR_PUBLISH_BODY), digest);
		CHECK(!strcmp(hex_of(digest, 32), VECTOR_PUBLISH_BODY_SHA256), "the body's SHA-256 is %s",
			hex_of(digest, 32));
		p2p_sha256(VECTOR_SERVER_ID_INPUT, (int)strlen(VECTOR_SERVER_ID_INPUT), digest);
		CHECK(!strcmp(hex_of(digest, 8), VECTOR_SERVER_ID), "the server id input hashes to %s", hex_of(digest, 8));
		/* (the vectors' signatures are the keys' over the vectors' strings) */
		key_of(VECTOR_PUBLISHER_PUBLIC_KEY, public_key);
		CHECK(server_base64_decode(VECTOR_PUBLISH_SIGNATURE, signature, sizeof(signature)) == 64, "not 64 bytes");
		CHECK(p2p_ed25519_verify(public_key, VECTOR_PUBLISH_CANONICAL, (int)strlen(VECTOR_PUBLISH_CANONICAL),
			signature), "the publish's signature does not verify");
		/* (a key of small order: refused, whatever it signs) */
		key_of(VECTOR_SMALL_ORDER_KEY, public_key);
		CHECK(!p2p_ed25519_to_x25519(public_key, x25519), "a key of small order was taken");
		memset(signature, 0, sizeof(signature));
		CHECK(!p2p_ed25519_verify(public_key, "", 0, signature), "a small-order key's signature verified");
		signature[0] = 1;
		CHECK(!p2p_ed25519_verify(public_key, "", 0, signature), "a small-order key's signature verified");
		return 0;
	}
	/* the listings' bytes from their base64, their host's key, and their
	signatures over "hceu-lobby-1" and their signed bytes */
	CASE("listing-signatures")
	{
		int index;

		for (index = 0; index < (int)(sizeof(vector_listings) / sizeof(vector_listings[0])); index++)
		{
			struct vector_listing const *vector = &vector_listings[index];
			unsigned char bytes[400], decoded[400], seed[32], public_key[32], message[512], signature[64];
			int size = hex_decode(vector->bytes, bytes, sizeof(bytes));

			CHECK(size == vector->size, "%s: %d bytes", vector->description, size);
			CHECK(server_base64_decode(vector->base64, decoded, sizeof(decoded)) == size &&
				!memcmp(decoded, bytes, (size_t)size), "%s: its base64 is other bytes", vector->description);
			key_of(vector->host_seed, seed);
			p2p_ed25519_public(seed, public_key, NULL);
			CHECK(!strcmp(hex_of(public_key, 32), vector->host_public_key) && !memcmp(bytes + 14, public_key, 32),
				"%s: another host key", vector->description);
			CHECK(!strcmp(hex_of(bytes + size - 64, 64), vector->signature), "%s: the signature is not its last 64 "
				"bytes", vector->description);
			CHECK(vector->signed_size == size - 64,
				"%s: signed %d of %d bytes", vector->description, vector->signed_size, size);
			memcpy(message, "hceu-lobby-1", 12);
			memcpy(message + 12, bytes, (size_t)vector->signed_size);
			CHECK((strcmp(hex_of(message, 12 + vector->signed_size), vector->signed_message) == 0) == vector->valid,
				"%s: the signed message %s the vector's", vector->description, vector->valid ? "is not" : "is");
			CHECK(p2p_ed25519_verify(public_key, message, 12 + vector->signed_size, bytes + size - 64) ==
				vector->valid, "%s: its signature %s", vector->description, vector->valid ? "fails" : "verifies");
			if (vector->valid)
			{
				p2p_ed25519_sign(seed, public_key, message, 12 + vector->signed_size, signature);
				CHECK(!memcmp(signature, bytes + size - 64, 64), "%s: signed otherwise", vector->description);
			}
		}
		return 0;
	}
	/* --sign: the publisher key's signature of an application's nonce */
	CASE("proof")
	{
		unsigned char signature[64];

		setenv("HALO_SERVER_PUBLISHER_KEY", VECTOR_PUBLISHER_SEED, 1);
		CHECK(publisher_sign(VECTOR_PROOF_NONCE, (int)strlen(VECTOR_PROOF_NONCE), signature), "no publisher key");
		CHECK(!strcmp(hex_of(signature, 64), VECTOR_PROOF_SIGNATURE), "the proof is %s", hex_of(signature, 64));
		return 0;
	}
	/* the server's key id and server id, and its master's key id */
	CASE("server-id")
	{
		unsigned char master_id[8];

		start_publisher();
		CHECK(!strcmp(master.key_id, VECTOR_PUBLISHER_KEY_ID), "key id %s", master.key_id);
		CHECK(!strcmp(master.instance, VECTOR_INSTANCE), "instance %s", master.instance);
		CHECK(!strcmp(master.server_id, VECTOR_SERVER_ID), "server id %s, not %s", master.server_id, VECTOR_SERVER_ID);
		CHECK(hex_decode(VECTOR_MASTER_KEY_ID_BYTES, master_id, 8) == 8 &&
			!memcmp(master.masters[0].key_id, master_id, 8), "the master's key id is %s",
			hex_of(master.masters[0].key_id, 8));
		CHECK(!strcmp(master.masters[0].url, "https://master.example.org"), "url %s", master.masters[0].url);
		return 0;
	}
	/* publisher.key: one line of 64 hex digits, comments and blank lines
	skipped; HALO_SERVER_PUBLISHER_KEY first */
	CASE("key-file")
	{
		unsigned char public_key[32];
		char key_id[17], text[256], upper[65];
		int index;

		unsetenv("HALO_SERVER_PUBLISHER_KEY");
		file_key = NULL;
		CHECK(!server_master_publisher(public_key, key_id), "a key without a file");
		snprintf(text, sizeof(text), "# the operator's key\n\n   %s  # mine\n", VECTOR_PUBLISHER_SEED);
		file_key = text;
		CHECK(server_master_publisher(public_key, key_id), "the key file was refused");
		CHECK(!strcmp(hex_of(public_key, 32), VECTOR_PUBLISHER_PUBLIC_KEY) && !strcmp(key_id, VECTOR_PUBLISHER_KEY_ID),
			"the file's key is %s (%s)", hex_of(public_key, 32), key_id);
		snprintf(text, sizeof(text), "%s\r\n", VECTOR_PUBLISHER_SEED);
		CHECK(server_master_publisher(public_key, key_id), "a key file with CRLF was refused");
		for (index = 0; index < 64; index++)
			upper[index] = (char)toupper((unsigned char)VECTOR_PUBLISHER_SEED[index]);
		upper[64] = 0;
		snprintf(text, sizeof(text), "%s\n", upper);
		CHECK(server_master_publisher(public_key, key_id) && !strcmp(key_id, VECTOR_PUBLISHER_KEY_ID),
			"a key in capitals was read otherwise");
		snprintf(text, sizeof(text), "%.63s\n", VECTOR_PUBLISHER_SEED);
		CHECK(!server_master_publisher(public_key, key_id), "63 digits were taken");
		snprintf(text, sizeof(text), "%s0\n", VECTOR_PUBLISHER_SEED);
		CHECK(!server_master_publisher(public_key, key_id), "65 digits were taken");
		snprintf(text, sizeof(text), "%.62sxy\n", VECTOR_PUBLISHER_SEED);
		CHECK(!server_master_publisher(public_key, key_id), "a key that is not hex was taken");
		snprintf(text, sizeof(text), "# %s\n", VECTOR_PUBLISHER_SEED);
		CHECK(!server_master_publisher(public_key, key_id), "a commented key was taken");
		/* (the environment's over the file's) */
		snprintf(text, sizeof(text), "%s\n", VECTOR_MASTER_SEED);
		setenv("HALO_SERVER_PUBLISHER_KEY", VECTOR_PUBLISHER_SEED, 1);
		CHECK(server_master_publisher(public_key, key_id) && !strcmp(key_id, VECTOR_PUBLISHER_KEY_ID),
			"the file's key was taken over the environment's");
		setenv("HALO_SERVER_PUBLISHER_KEY", "not a key", 1);
		CHECK(!server_master_publisher(public_key, key_id), "a bad HALO_SERVER_PUBLISHER_KEY was taken");
		return 0;
	}
	/* the instance: server.instance, else a slug of the server's name; 1 to
	32 of a-z 0-9 - _, the first a letter or digit */
	CASE("instance")
	{
		static const struct instance_vector { const char *instance, *name, *expected; } names[] =
		{
			{ "main", "Anything", "main" },
			{ "Main", "", "main" },
			{ "", "OpenCE Dedicated", "opence-dedicated" },
			{ "", "  --My.Server__ ", "my-server__" },
			{ "", "EU #1 CTF / Slayer!", "eu-1-ctf-slayer" },
			{ "", "", "main" },
			{ "", "!!! ???", "main" },
			{ "", "_-_", "main" },
			{ "", "\xc3\x9c" "ber Server", "ber-server" },
			{ "", "abcdefghijklmnopqrstuvwxyz0123456789", "abcdefghijklmnopqrstuvwxyz012345" },
			{ "", "a                                       b", "a-b" },
		};
		int index;

		for (index = 0; index < (int)(sizeof(names) / sizeof(names[0])); index++)
		{
			char instance[33];
			int character;

			setting_instance = names[index].instance;
			setting_name = names[index].name;
			memset(instance, 'z', sizeof(instance));
			instance_name(instance, sizeof(instance));
			CHECK(!strcmp(instance, names[index].expected), "\"%s\" / \"%s\" made %s, not %s", names[index].instance,
				names[index].name, instance, names[index].expected);
			/* (docs/master_protocol.md's form) */
			CHECK(strlen(instance) >= 1 && strlen(instance) <= 32 && isalnum((unsigned char)instance[0]),
				"%s is no instance", instance);
			for (character = 0; instance[character]; character++)
			{
				CHECK(islower((unsigned char)instance[character]) || isdigit((unsigned char)instance[character]) ||
					instance[character] == '-' || instance[character] == '_', "%s is no instance", instance);
			}
		}
		return 0;
	}
	/* the vectors' signed publish */
	CASE("publish-request")
	{
		CHECK(!strcmp(&VECTOR_PUBLISH_PATH[strlen("/v1/servers/")], VECTOR_SERVER_ID), "the path is another server's");
		check_request(VECTOR_PUBLISH_MASTER_KEY_ID, VECTOR_PUBLISH_METHOD, VECTOR_PUBLISH_PATH, VECTOR_PUBLISH_BODY, strlen(VECTOR_PUBLISH_BODY),
			VECTOR_PUBLISH_TS, VECTOR_PUBLISH_NONCE, VECTOR_PUBLISH_CANONICAL, VECTOR_PUBLISH_AUTHORIZATION);
		return 0;
	}
	/* the vectors' signed GET /v1/me: an empty body ("" or none) */
	CASE("me-request")
	{
		check_request(VECTOR_ME_MASTER_KEY_ID, VECTOR_ME_METHOD, VECTOR_ME_PATH, VECTOR_ME_BODY, 0, VECTOR_ME_TS, VECTOR_ME_NONCE,
			VECTOR_ME_CANONICAL, VECTOR_ME_AUTHORIZATION);
		check_request(VECTOR_ME_MASTER_KEY_ID, VECTOR_ME_METHOD, VECTOR_ME_PATH, NULL, 0, VECTOR_ME_TS, VECTOR_ME_NONCE, VECTOR_ME_CANONICAL,
			VECTOR_ME_AUTHORIZATION);
		return 0;
	}
	/* the vectors' probe answered as they have it */
	CASE("probe")
	{
		unsigned char probe[64], expected[128], answer[128], message[64], public_key[32];
		int size, expected_size, message_size;

		start_publisher();
		now = 1760900000;
		size = hex_decode(VECTOR_PROBE_PACKET, probe, sizeof(probe));
		expected_size = hex_decode(VECTOR_PROBE_ANSWER, expected, sizeof(expected));
		message_size = hex_decode(VECTOR_PROBE_SIGNED_MESSAGE, message, sizeof(message));
		CHECK(size == 35 && expected_size == 103 && message_size == 29, "the vectors' probe is %d, %d, %d bytes", size,
			expected_size, message_size);
		CHECK(!strcmp(hex_of(probe + 11, 8), VECTOR_PROBE_MASTER_KEY_ID) && !strcmp(hex_of(probe + 19, 16),
			VECTOR_PROBE_NONCE), "the vectors' probe holds another key id or nonce");
		signed_size = -1;
		CHECK(probe_answered(probe, size, answer), "the vectors' probe was not answered");
		CHECK(signed_size == message_size && !memcmp(signed_message, message, (size_t)message_size),
			"signed %s, not the vector's %s", hex_of(signed_message, signed_size > 0 ? signed_size : 0),
			VECTOR_PROBE_SIGNED_MESSAGE);
		CHECK(!memcmp(answer, expected, 39), "answered %s", hex_of(answer, 39));
		CHECK(!memcmp(answer + 39, expected + 39, 64), "signed %s", hex_of(answer + 39, 64));
		key_of(VECTOR_PUBLISHER_PUBLIC_KEY, public_key);
		CHECK(p2p_ed25519_verify(public_key, message, message_size, answer + 39),
			"the answer's signature is not the publisher's");
		return 0;
	}
	/* a probe answered only as it is, from a master of masters.txt, with a
	publisher key, a nonce once a second */
	CASE("probe-refusals")
	{
		unsigned char probe[64], other[64], answer[128];
		int size, index;

		start_publisher();
		now = 1760900000;
		size = hex_decode(VECTOR_PROBE_PACKET, probe, sizeof(probe));
		memcpy(other, probe, (size_t)size);
		CHECK(!probe_answered(probe, size - 1, answer), "a probe of 34 bytes was answered");
		CHECK(!probe_answered(probe, size + 1, answer), "a probe of 36 bytes was answered");
		other[10] = 2;
		CHECK(!probe_answered(other, size, answer), "a probe of version 2 was answered");
		other[10] = 1;
		other[0] = 'o';
		CHECK(!probe_answered(other, size, answer), "a probe with another magic was answered");
		other[0] = probe[0];
		other[11] ^= 1;
		CHECK(!probe_answered(other, size, answer), "a probe of a master not in masters.txt was answered");
		other[11] = probe[11];
		CHECK(probe_answered(probe, size, answer), "the vectors' probe was not answered");
		CHECK(!probe_answered(probe, size, answer), "a nonce was answered twice in a second");
		/* (a master's retry, two seconds later, with the same nonce) */
		now += 2;
		CHECK(probe_answered(probe, size, answer), "the master's retry was not answered");
		(void)index;
		/* (no publisher key: nothing answered) */
		now += 1;
		master.has_key = 0;
		other[34] = 0xEF;
		CHECK(!probe_answered(other, size, answer), "a probe was answered without a key");
		return 0;
	}
	/* the vectors' signed list, taken while it is fresh */
	CASE("list-verify")
	{
		unsigned long time_value = strtoul(VECTOR_LIST_TIME, NULL, 10);
		unsigned long size = (unsigned long)strlen(VECTOR_LIST_BODY);

		CHECK(verify_list(VECTOR_LIST_BODY, size, VECTOR_LIST_SIGNATURE_HEADER, VECTOR_LIST_KEY_HEADER,
			VECTOR_MASTER_PUBLIC_KEY, time_value), "the vectors' list was refused");
		CHECK(verify_list(VECTOR_LIST_BODY, size, VECTOR_LIST_SIGNATURE_HEADER, VECTOR_LIST_KEY_HEADER,
			VECTOR_MASTER_PUBLIC_KEY, time_value + 300), "a list 300 seconds old was refused");
		CHECK(!verify_list(VECTOR_LIST_BODY, size, VECTOR_LIST_SIGNATURE_HEADER, VECTOR_LIST_KEY_HEADER,
			VECTOR_MASTER_PUBLIC_KEY, time_value + 301), "a list 301 seconds old was taken");
		CHECK(verify_list(VECTOR_LIST_BODY, size, VECTOR_LIST_SIGNATURE_HEADER, VECTOR_LIST_KEY_HEADER,
			VECTOR_MASTER_PUBLIC_KEY, time_value - 60), "a list of a master's clock ahead was refused");
		/* (dated ahead of this machine's clock: 300 seconds at most) */
		CHECK(verify_list(VECTOR_LIST_BODY, size, VECTOR_LIST_SIGNATURE_HEADER, VECTOR_LIST_KEY_HEADER,
			VECTOR_MASTER_PUBLIC_KEY, time_value - 300), "a list 300 seconds ahead was refused");
		CHECK(!verify_list(VECTOR_LIST_BODY, size, VECTOR_LIST_SIGNATURE_HEADER, VECTOR_LIST_KEY_HEADER,
			VECTOR_MASTER_PUBLIC_KEY, time_value - 301), "a list 301 seconds ahead was taken");
		return 0;
	}
	/* a list altered, signed by another key, or with a malformed header */
	CASE("list-refusals")
	{
		unsigned long time_value = strtoul(VECTOR_LIST_TIME, NULL, 10);
		unsigned long size = (unsigned long)strlen(VECTOR_LIST_BODY);
		const char *signature = VECTOR_LIST_SIGNATURE_HEADER;
		char body[512], header[256];
		unsigned long offsets[3];
		int index;

		offsets[0] = 0;
		offsets[1] = size / 2;
		offsets[2] = size - 1;
		for (index = 0; index < 3; index++)
		{
			snprintf(body, sizeof(body), "%s", VECTOR_LIST_BODY);
			body[offsets[index]] ^= 1;
			CHECK(!verify_list(body, size, signature, VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY, time_value),
				"a list with byte %lu changed was taken", offsets[index]);
		}
		CHECK(!verify_list(VECTOR_LIST_BODY, size - 1, signature, VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY,
			time_value), "a list without its last byte was taken");
		CHECK(!verify_list(VECTOR_LIST_BODY, size, signature, NULL, VECTOR_PUBLISHER_PUBLIC_KEY, time_value),
			"a list was taken with another master's key");
		CHECK(!verify_list(VECTOR_LIST_BODY, size, signature, VECTOR_LIST_KEY_HEADER, VECTOR_PUBLISHER_PUBLIC_KEY,
			time_value), "a list was taken with another master's key");
		CHECK(!verify_list(VECTOR_LIST_BODY, size, signature, VECTOR_PUBLISHER_KEY_ID, VECTOR_MASTER_PUBLIC_KEY,
			time_value), "a list was taken whose Signature-Key is another key's");
		CHECK(!verify_list(VECTOR_LIST_BODY, size, signature, "39f713d0a644253", VECTOR_MASTER_PUBLIC_KEY,
			time_value), "a list was taken whose Signature-Key is cut short");
		CHECK(!verify_list(VECTOR_LIST_BODY, size, signature, NULL, VECTOR_MASTER_PUBLIC_KEY, time_value) &&
			!verify_list(VECTOR_LIST_BODY, size, signature, "", VECTOR_MASTER_PUBLIC_KEY, time_value),
			"a list was taken without its Signature-Key");
		/* (the Signature header malformed) */
		CHECK(!verify_list(VECTOR_LIST_BODY, size, "", VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY, time_value),
			"a list without a signature was taken");
		CHECK(!verify_list(VECTOR_LIST_BODY, size, "ed25519=", VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY,
			time_value), "an empty signature was taken");
		CHECK(!verify_list(VECTOR_LIST_BODY, size, signature + 8, VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY,
			time_value), "a signature without ed25519= was taken");
		snprintf(header, sizeof(header), "ed25519:%s", signature + 8);
		CHECK(!verify_list(VECTOR_LIST_BODY, size, header, VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY,
			time_value), "ed25519: was taken for ed25519=");
		snprintf(header, sizeof(header), "Ed25519=%s", signature + 8);
		CHECK(!verify_list(VECTOR_LIST_BODY, size, header, VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY,
			time_value), "Ed25519= was taken for ed25519=");
		snprintf(header, sizeof(header), "%.*s", (int)strlen(signature) - 4, signature);
		CHECK(!verify_list(VECTOR_LIST_BODY, size, header, VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY,
			time_value), "a signature cut short was taken");
		snprintf(header, sizeof(header), "%sAAAA", signature);
		CHECK(!verify_list(VECTOR_LIST_BODY, size, header, VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY,
			time_value), "a signature too long was taken");
		snprintf(header, sizeof(header), "%s", signature);
		header[10] = header[10] == 'A' ? 'B' : 'A';
		CHECK(!verify_list(VECTOR_LIST_BODY, size, header, VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY,
			time_value), "a signature changed was taken");
		snprintf(header, sizeof(header), "ed25519=%s", "!!!!");
		CHECK(!verify_list(VECTOR_LIST_BODY, size, header, VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY,
			time_value), "a signature that is not base64 was taken");
		return 0;
	}
	/* a body the master signed, but not a list of protocol 1 by it */
	CASE("list-body")
	{
		static const char *refused[] =
		{
			"{\"protocol\":2,\"master\":{\"name\":\"m\",\"key_id\":\"39f713d0a644253f\",\"time\":1760900000},\"servers\":[]}",
			"{\"protocol\":\"1\",\"master\":{\"name\":\"m\",\"key_id\":\"39f713d0a644253f\",\"time\":1760900000},\"servers\":[]}",
			"{\"master\":{\"name\":\"m\",\"key_id\":\"39f713d0a644253f\",\"time\":1760900000},\"servers\":[]}",
			"{\"protocol\":1,\"master\":{\"name\":\"m\",\"key_id\":\"21fe31dfa154a261\",\"time\":1760900000},\"servers\":[]}",
			"{\"protocol\":1,\"master\":{\"name\":\"m\",\"key_id\":\"39f713d0a644253f\"},\"servers\":[]}",
			"{\"protocol\":1,\"master\":{\"name\":\"m\",\"key_id\":\"39f713d0a644253f\",\"time\":\"1760900000\"}}",
			"{\"protocol\":1,\"servers\":[]}",
			"[{\"protocol\":1,\"master\":{\"name\":\"m\",\"key_id\":\"39f713d0a644253f\",\"time\":1760900000}}]",
			"{\"protocol\":1,\"master\":{\"name\":\"m\",\"key_id\":\"39f713d0a644253f\",\"time\":1760900000}",
			"not json",
		};
		static const char *taken[] =
		{
			"{\"protocol\":1,\"master\":{\"name\":\"m\",\"key_id\":\"39f713d0a644253f\",\"time\":1760900000},\"servers\":[],"
				"\"later\":{\"field\":[1,2,3]}}",
			"{\"servers\":[{\"id\":\"4c5f15237c866f6f\"}],\"master\":{\"time\":1760900000,\"key_id\":\"39f713d0a644253f\"},"
				"\"protocol\":1}",
		};
		int index;

		for (index = 0; index < (int)(sizeof(refused) / sizeof(refused[0])); index++)
		{
			CHECK(!verify_list(refused[index], strlen(refused[index]), master_signs(refused[index]),
				VECTOR_LIST_KEY_HEADER, VECTOR_MASTER_PUBLIC_KEY, 1760900000), "taken: %s", refused[index]);
		}
		for (index = 0; index < (int)(sizeof(taken) / sizeof(taken[0])); index++)
		{
			CHECK(verify_list(taken[index], strlen(taken[index]), master_signs(taken[index]), VECTOR_LIST_KEY_HEADER,
				VECTOR_MASTER_PUBLIC_KEY, 1760900000), "refused: %s", taken[index]);
		}
		return 0;
	}
	/* masters.txt: a URL and its key a line, comments, https or this
	machine's http, the first four */
	CASE("masters-file")
	{
		char urls[MASTER_LIST_MAXIMUM_MASTERS + 2][MASTER_LIST_URL_SIZE];
		unsigned char keys[MASTER_LIST_MAXIMUM_MASTERS + 2][MASTER_LIST_KEY_SIZE];
		unsigned char key[32];
		char text[4096];
		int count;

		key_of(VECTOR_MASTER_PUBLIC_KEY, key);
		snprintf(text, sizeof(text),
			"# OpenCE's masters\n"
			"\n"
			"https://master1.example.org/ %s   # the first\r\n"
			"http://master2.example.org %s\n"
			"https://master3.example.org ed25519:%.62s\n"
			"https://master4.example.org ed25519:%.62sxy\n"
			"https://master5.example.org %s0\n"
			"https://master6.example.org\n"
			"#https://master7.example.org %s\n"
			"http://127.0.0.1.example.net %s\n"
			"  http://127.0.0.1:8080 %s\n"
			"https://master9.example.org/opence// %s\n"
			"http://localhost:9000 %s\n"
			"https://master11.example.org %s\n",
			VECTOR_MASTER_PUBLIC_KEY_TEXT, VECTOR_MASTER_PUBLIC_KEY_TEXT, VECTOR_MASTER_PUBLIC_KEY,
			VECTOR_MASTER_PUBLIC_KEY, VECTOR_MASTER_PUBLIC_KEY_TEXT, VECTOR_MASTER_PUBLIC_KEY_TEXT,
			VECTOR_MASTER_PUBLIC_KEY_TEXT, VECTOR_MASTER_PUBLIC_KEY_TEXT, VECTOR_MASTER_PUBLIC_KEY_TEXT,
			VECTOR_MASTER_PUBLIC_KEY_TEXT, VECTOR_MASTER_PUBLIC_KEY_TEXT);
		file_masters = text;
		memset(keys, 0, sizeof(keys));
		count = master_list_read_file("masters.txt", urls, keys, MASTER_LIST_MAXIMUM_MASTERS);
		CHECK(count == MASTER_LIST_MAXIMUM_MASTERS, "%d masters read", count);
		CHECK(!strcmp(urls[0], "https://master1.example.org"), "the first is %s", urls[0]);
		CHECK(!strcmp(urls[1], "http://127.0.0.1:8080"), "the second is %s", urls[1]);
		CHECK(!strcmp(urls[2], "https://master9.example.org/opence"), "the third is %s", urls[2]);
		CHECK(!strcmp(urls[3], "http://localhost:9000"), "the fourth is %s", urls[3]);
		for (count = 0; count < MASTER_LIST_MAXIMUM_MASTERS; count++)
			CHECK(!memcmp(keys[count], key, 32), "master %d's key is %s", count, hex_of(keys[count], 32));
		CHECK(master_list_read_file("masters.txt", urls, keys, 1) == 1, "more than the most were read");
		file_masters = NULL;
		CHECK(master_list_read_file("masters.txt", urls, keys, MASTER_LIST_MAXIMUM_MASTERS) == 0,
			"masters read from no file");
		return 0;
	}
	/* the official master, built in: asked when masters.txt names none */
	CASE("masters-default")
	{
		char urls[MASTER_LIST_MAXIMUM_MASTERS][MASTER_LIST_URL_SIZE];
		unsigned char keys[MASTER_LIST_MAXIMUM_MASTERS][MASTER_LIST_KEY_SIZE];
		unsigned char official[32];
		static const char *const none[] = { NULL, "", "# comments alone\n\n#https://master1.example.org\n",
			"https://master1.example.org ed25519:abc\n" };
		static char named[256];
		int index;

		key_of("89f4eda71229f11f3538679f49ef7fb0c2b407b380093f7f4fe02105b92225aa", official);
		for (index = 0; index < (int)(sizeof(none) / sizeof(none[0])); index++)
		{
			file_masters = none[index];
			memset(urls, 0, sizeof(urls));
			CHECK(master_list_read("masters.txt", urls, keys, MASTER_LIST_MAXIMUM_MASTERS) == 1 &&
				!strcmp(urls[0], "https://master.opencommunityedition.com"),
				"masters.txt %d naming none: %s", index, urls[0]);
			CHECK(!memcmp(keys[0], official, 32), "the official master's key is %s", hex_of(keys[0], 32));
		}
		snprintf(named, sizeof(named), "https://master1.example.org %s\n", VECTOR_MASTER_PUBLIC_KEY_TEXT);
		file_masters = named;
		CHECK(master_list_read("masters.txt", urls, keys, MASTER_LIST_MAXIMUM_MASTERS) == 1 &&
			!strcmp(urls[0], "https://master1.example.org"), "masters.txt's master was not alone: %s", urls[0]);
		file_masters = NULL;
		return 0;
	}
	/* the web site's masters.json */
	CASE("masters-json")
	{
		/* (room past the most: a fault that reads more is seen, not a crash) */
		char urls[MASTER_LIST_MAXIMUM_MASTERS + 4][MASTER_LIST_URL_SIZE];
		unsigned char keys[MASTER_LIST_MAXIMUM_MASTERS + 4][MASTER_LIST_KEY_SIZE];
		unsigned char key[32];
		char text[2048];
		int count, index, length;

		key_of(VECTOR_MASTER_PUBLIC_KEY, key);
		snprintf(text, sizeof(text),
			"{\"masters\":[{\"url\":\"https://master1.example.org/\",\"key\":\"%s\"},"
			"{\"url\":\"http://master2.example.org\",\"key\":\"%s\"},"
			"{\"url\":\"https://master3.example.org\",\"key\":\"ed25519:%.62s\"},"
			"{\"url\":\"https://master4.example.org\"},"
			"{\"url\":1,\"key\":\"%s\"},"
			"\"https://master6.example.org\","
			"{\"key\":\"%s\",\"url\":\"https://master7.example.org\",\"name\":\"later\"}]}",
			VECTOR_MASTER_PUBLIC_KEY_TEXT, VECTOR_MASTER_PUBLIC_KEY_TEXT, VECTOR_MASTER_PUBLIC_KEY,
			VECTOR_MASTER_PUBLIC_KEY_TEXT, VECTOR_MASTER_PUBLIC_KEY_TEXT);
		count = master_list_read_json(text, strlen(text), urls, keys, MASTER_LIST_MAXIMUM_MASTERS);
		CHECK(count == 2, "%d masters read", count);
		CHECK(!strcmp(urls[0], "https://master1.example.org") && !strcmp(urls[1], "https://master7.example.org"),
			"read %s and %s", urls[0], urls[1]);
		CHECK(!memcmp(keys[0], key, 32) && !memcmp(keys[1], key, 32), "another key was read");
		CHECK(master_list_read_json("{}", 2, urls, keys, MASTER_LIST_MAXIMUM_MASTERS) == 0, "masters read from {}");
		CHECK(master_list_read_json("not json", 8, urls, keys, MASTER_LIST_MAXIMUM_MASTERS) == 0,
			"masters read from what is not JSON");
		/* (six good ones: the first four) */
		length = snprintf(text, sizeof(text), "{\"masters\":[");
		for (index = 0; index < MASTER_LIST_MAXIMUM_MASTERS + 2; index++)
		{
			length += snprintf(text + length, sizeof(text) - (size_t)length, "%s{\"url\":\"https://m%d.example.org\","
				"\"key\":\"%s\"}", index ? "," : "", index, VECTOR_MASTER_PUBLIC_KEY_TEXT);
		}
		length += snprintf(text + length, sizeof(text) - (size_t)length, "]}");
		count = master_list_read_json(text, (unsigned long)length, urls, keys, MASTER_LIST_MAXIMUM_MASTERS);
		CHECK(count == MASTER_LIST_MAXIMUM_MASTERS && !strcmp(urls[3], "https://m3.example.org"),
			"%d masters read of %d", count, MASTER_LIST_MAXIMUM_MASTERS + 2);
		return 0;
	}

	/* the vectors' listings as p2p_lobby.c reads them: each field, and the
	signature over "hceu-lobby-1" and the signed bytes */
	CASE("listing-read")
	{
		unsigned char bytes[400], host_key[32];
		struct listing listing;
		int index, size;

		key_of(VECTOR_HOST_PUBLIC_KEY, host_key);
		for (index = 0; index < (int)(sizeof(vector_listings) / sizeof(vector_listings[0])); index++)
		{
			struct vector_listing const *vector = &vector_listings[index];

			size = hex_decode(vector->bytes, bytes, sizeof(bytes));
			memset(&listing, 0, sizeof(listing));
			CHECK(listing_read(bytes, size, &listing), "%s: not read", vector->description);
			CHECK(listing.signed_size == vector->signed_size && !memcmp(listing.key, host_key, 32),
				"%s: signed %d bytes, by %s", vector->description, listing.signed_size, hex_of(listing.key, 32));
			CHECK(listing_signed(bytes, &listing) == vector->valid, "%s: its signature %s", vector->description,
				vector->valid ? "fails" : "checks");
		}
		/* (the first: an open team game of a dedicated server) */
		size = hex_decode(vector_listings[0].bytes, bytes, sizeof(bytes));
		listing_read(bytes, size, &listing);
		CHECK(listing.version == 38 && listing.flags == (_listing_open | _listing_has_teams | _listing_dedicated) &&
			listing.sequence == 412 && listing.time == 1760900000UL, "version %d, flags %d, sequence %lu, time %lu",
			listing.version, listing.flags, listing.sequence, listing.time);
		CHECK(!strcmp(hex_of(listing.token, 16), "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"), "token %s", hex_of(listing.token, 16));
		CHECK(listing.player_count == 5 && listing.maximum_player_count == 16 && listing.engine_type == 1 &&
			listing.kind == 0, "%d of %d, engine %d, kind %d", listing.player_count, listing.maximum_player_count,
			listing.engine_type, listing.kind);
		CHECK(!strcmp(listing.name, "OpenCE Dedicated") && !strcmp(listing.map, "bloodgulch") &&
			!strcmp(listing.gametype, "CTF"), "named %s, %s, %s", listing.name, listing.map, listing.gametype);
		/* (the second: a password's game in progress, its token sealed, SWAT
		with Hardcore) */
		size = hex_decode(vector_listings[1].bytes, bytes, sizeof(bytes));
		listing_read(bytes, size, &listing);
		CHECK(listing.flags == (_listing_open | _listing_in_progress | _listing_password) && listing.kind == 0x81 &&
			!memcmp(listing.sealed_token, bytes + 46, P2P_SEALED_TOKEN_SIZE) &&
			listing.player_count == bytes[46 + P2P_SEALED_TOKEN_SIZE], "the password's game was read as flags %d, kind %d",
			listing.flags, listing.kind);
		/* (the third: a tombstone) */
		size = hex_decode(vector_listings[2].bytes, bytes, sizeof(bytes));
		listing_read(bytes, size, &listing);
		CHECK((listing.flags & _listing_closed) && listing.sequence == 413, "the tombstone was read as flags %d, "
			"sequence %lu", listing.flags, listing.sequence);
		/* (a byte changed anywhere in the signed part, or the signature) */
		size = hex_decode(vector_listings[0].bytes, bytes, sizeof(bytes));
		for (index = 0; index < size; index += 7)
		{
			bytes[index] ^= 0x20;
			CHECK(!listing_good(bytes, size), "the first listing with byte %d changed was taken", index);
			bytes[index] ^= 0x20;
		}
		CHECK(listing_good(bytes, size), "the first listing was refused");
		return 0;
	}
	/* a listing is 140 to 268 bytes, of format 1, its names within their
	fields and its stamp and signature ending it exactly */
	CASE("listing-bounds")
	{
		unsigned char bytes[600];
		struct listing listing;
		int size;

		CHECK(MINIMUM_LISTING_SIZE == 140 && MAXIMUM_LISTING_SIZE == 268, "listings of %d to %d bytes",
			MINIMUM_LISTING_SIZE, MAXIMUM_LISTING_SIZE);
		size = make_listing(bytes, 1, _listing_open, 0, 0, 0);
		CHECK(size == 140 && listing_good(bytes, size), "the shortest listing (%d bytes) was refused", size);
		size = make_listing(bytes, 1, _listing_open | _listing_password, P2P_LISTING_NAME_SIZE, P2P_LISTING_MAP_SIZE,
			P2P_LISTING_GAMETYPE_SIZE);
		CHECK(size == 268 && listing_good(bytes, size), "the longest listing (%d bytes) was refused", size);
		listing_read(bytes, size, &listing);
		CHECK(strlen(listing.name) == 32 && strlen(listing.map) == 32 && strlen(listing.gametype) == 24 &&
			listing.kind == 3 && listing.engine_type == 2, "the longest listing was read as %s / %s / %s",
			listing.name, listing.map, listing.gametype);
		CHECK(!listing_read(bytes, size - 1, &listing) && !listing_read(bytes, 139, &listing),
			"a listing cut short was read");
		size = make_listing(bytes, 1, _listing_open, 4, 4, 4);
		bytes[size] = 0;
		CHECK(!listing_read(bytes, size + 1, &listing), "a listing with a byte after its signature was read");
		size = make_listing(bytes, 2, _listing_open, 4, 4, 4);
		CHECK(!listing_read(bytes, size, &listing), "a listing of format 2 was read");
		size = make_listing(bytes, 1, _listing_open, 4, 4, 4);
		bytes[0] = 'h';
		CHECK(!listing_read(bytes, size, &listing), "a listing without HL was read");
		/* (a name past its field, which ends the listing exactly) */
		size = make_listing(bytes, 1, _listing_open, P2P_LISTING_NAME_SIZE + 1, 0, 0);
		CHECK(!listing_read(bytes, size, &listing), "a name of 33 was read");
		size = make_listing(bytes, 1, _listing_open, 0, 0, P2P_LISTING_GAMETYPE_SIZE + 1);
		CHECK(!listing_read(bytes, size, &listing), "a gametype of 25 was read");
		/* (a length past the end) */
		size = make_listing(bytes, 1, _listing_open, 0, 0, 0);
		bytes[65] = 200;
		CHECK(!listing_read(bytes, size, &listing), "a name's length past the end was read");
		/* (a name's bytes kept to printable ASCII, a zero ending it) */
		size = make_listing(bytes, 1, _listing_open, 6, 0, 0);
		bytes[66] = 0x01;
		bytes[67] = 0xC3;
		bytes[69] = 0;
		CHECK(listing_read(bytes, size, &listing) && !strcmp(listing.name, "??a"), "a name was read as %s", listing.name);
		return 0;
	}
	/* a publish: its body (the listing, the tunnel's port, dedicated or not,
	closed or not), its URL and the path signed under a master's base path;
	a listing withdrawn once */
	CASE("publish-body")
	{
		static const struct
		{
			unsigned short network_order;
			const char *port;
		} ports[] = { { 0xFE08, "2302" }, { 0x0100, "1" }, { 0xFFFF, "65535" }, { 0x0001, "256" } };
		char base64[400];
		int index;

		start_publisher();
		now = 1760900000;
		host_vector_listing();
		server_base64_encode(host_listing, host_listing_size, base64);
		snprintf(master.masters[0].url, sizeof(master.masters[0].url), "https://m.example.org/opence");
		tunnel_port = 0xFE08;
		setting_region = "eu";
		memset(&requested, 0, sizeof(requested));
		publish_to(&master.masters[0], 0);
		CHECK(requested.count == 1 && !strcmp(requested.method, "PUT") &&
			!strcmp(requested.url, "https://m.example.org/opence/v1/servers/" VECTOR_SERVER_ID), "published %s %s",
			requested.method, requested.url);
		CHECK(strstr((const char *)signed_message, "\nPUT\n/opence/v1/servers/" VECTOR_SERVER_ID "\n") != NULL,
			"signed %.*s", signed_size, (const char *)signed_message);
		CHECK(!strncmp(requested.headers, "Authorization: OpenCE-Ed25519 keyid=" VECTOR_PUBLISHER_KEY_ID ", ", 52),
			"the headers were %s", requested.headers);
		CHECK(!strcmp(body_member(requested.body, "protocol"), "1") && !strcmp(body_member(requested.body, "instance"),
			"main") && !strcmp(body_member(requested.body, "listing"), base64), "the body was %s", requested.body);
		CHECK(!strcmp(body_member(requested.body, "tunnel_port"), "2302") &&
			!strcmp(body_member(requested.body, "dedicated"), "true") &&
			!strcmp(body_member(requested.body, "closed"), "false") &&
			!strcmp(body_member(requested.body, "region"), "eu") &&
			!strcmp(body_member(requested.body, "build"), "build-206") &&
			!strcmp(body_member(requested.body, "address"), "(none)"), "the body was %s", requested.body);
		CHECK(master.masters[0].published && master.masters[0].listed, "the master's listing was not kept");
		/* (a master at its origin signs the path alone) */
		snprintf(master.masters[0].url, sizeof(master.masters[0].url), "https://m.example.org");
		publish_to(&master.masters[0], 0);
		CHECK(!strcmp(requested.url, "https://m.example.org/v1/servers/" VECTOR_SERVER_ID) &&
			strstr((const char *)signed_message, "\nPUT\n/v1/servers/" VECTOR_SERVER_ID "\n") != NULL,
			"published %s, signed %.*s", requested.url, signed_size, (const char *)signed_message);
		/* (the port as the socket has it, in network byte order) */
		for (index = 0; index < (int)(sizeof(ports) / sizeof(ports[0])); index++)
		{
			tunnel_port = ports[index].network_order;
			publish_to(&master.masters[0], 0);
			CHECK(!strcmp(body_member(requested.body, "tunnel_port"), ports[index].port), "port %04x was sent as %s",
				ports[index].network_order, body_member(requested.body, "tunnel_port"));
		}
		/* (a player's host's, and its public address when asked) */
		server_mode = 0;
		setting_publish_address = 1;
		publish_to(&master.masters[0], 0);
		CHECK(!strcmp(body_member(requested.body, "dedicated"), "false") &&
			!strcmp(body_member(requested.body, "address"), "203.0.113.5:2302"), "the body was %s", requested.body);
		server_mode = 1;
		setting_publish_address = 0;
		/* (a tombstone) */
		publish_to(&master.masters[0], 1);
		CHECK(!strcmp(body_member(requested.body, "closed"), "true") && !master.masters[0].published,
			"a tombstone was %s", requested.body);
		/* (no listing: nothing, unless one was published, which is withdrawn
		once) */
		host_listing_size = 0;
		requested.count = 0;
		publish_to(&master.masters[0], 0);
		CHECK(!requested.count, "a publish was made with no listing to withdraw");
		master.masters[0].published = 1;
		publish_to(&master.masters[0], 0);
		CHECK(requested.count == 1 && !strcmp(body_member(requested.body, "closed"), "true") &&
			!strcmp(body_member(requested.body, "listing"), "(none)") && !master.masters[0].published,
			"a listing was not withdrawn (%d: %s)", requested.count, requested.body);
		publish_to(&master.masters[0], 0);
		CHECK(requested.count == 1, "a listing was withdrawn twice");
		return 0;
	}
	/* what a master's answers make the publisher do: wait as Retry-After
	says (its name in any case), stop after a revocation, say how far off
	the clock is */
	CASE("publish-answers")
	{
		start_publisher();
		now = 1760900000;
		host_vector_listing();
		tunnel_port = 0xFE08;
		answer_status = 429;
		answer_headers = "Content-Type: application/json\r\nretry-after: 120\r\n";
		answer_body = "{\"ok\":false,\"error\":{\"code\":\"rate_limited\",\"message\":\"slow down\"}}";
		memset(&requested, 0, sizeof(requested));
		publish_to(&master.masters[0], 0);
		CHECK(master.masters[0].retry_at == now + 120, "Retry-After: 120 waited until %lu", master.masters[0].retry_at);
		now += 119;
		publish_to(&master.masters[0], 0);
		CHECK(requested.count == 1, "a publish was made before Retry-After");
		now += 1;
		answer_headers = "RETRY-AFTER: 999999999\r\n";
		publish_to(&master.masters[0], 0);
		CHECK(requested.count == 2 && master.masters[0].retry_at == now + 86400, "Retry-After past a day waited until %lu",
			master.masters[0].retry_at);
		now += 86400;
		answer_headers = "";
		publish_to(&master.masters[0], 0);
		CHECK(master.masters[0].retry_at == now + 30, "no Retry-After waited until %lu", master.masters[0].retry_at);
		now += 30;
		/* (the clock: the difference, from the answer's master_time: this
		machine's 450 seconds behind) */
		answer_status = 401;
		answer_body = "{\"ok\":false,\"error\":{\"code\":\"clock\",\"message\":\"ts\"},\"master_time\":1760987000}";
		publish_to(&master.masters[0], 0);
		CHECK(!strcmp(master.masters[0].code, "clock") && strstr(master.masters[0].message, "seconds behind") &&
			strstr(master.masters[0].message, " 450 "), "the clock's answer was kept as %s: %s", master.masters[0].code,
			master.masters[0].message);
		/* (a revoked key: not published there again, nor withdrawn) */
		answer_status = 403;
		answer_body = "{\"ok\":false,\"error\":{\"code\":\"key_revoked\",\"message\":\"revoked\"}}";
		master.masters[0].retry_at = 0;
		publish_to(&master.masters[0], 0);
		CHECK(master.masters[0].revoked && !master.masters[0].published, "a revoked key was not stopped");
		requested.count = 0;
		now += 3600;
		publish_to(&master.masters[0], 0);
		publish_to(&master.masters[0], 1);
		CHECK(!requested.count, "a revoked key published again (%d)", requested.count);
		answer_status = 200;
		answer_headers = "";
		answer_body = "{\"ok\":true,\"listed\":true,\"probe\":\"ok\"}";
		return 0;
	}
	/* a list's entries: one that is not an entry is skipped, not the list;
	the publisher's name and region kept to what the ticker shows; the
	entry's word on a dedicated server passed on */
	CASE("take-list")
	{
		char base64[400], body[8192];
		unsigned char bytes[400], large[600];
		char large_base64[900];
		int size;

		size = hex_decode(vector_listings[0].bytes, bytes, sizeof(bytes));
		server_base64_encode(bytes, size, base64);
		memset(large, 'x', sizeof(large));
		server_base64_encode(large, (int)sizeof(large) - 150, large_base64);
		snprintf(body, sizeof(body), "{\"protocol\":1,\"servers\":["
			"{\"id\":\"a\",\"listing\":\"%s\",\"publisher\":{\"name\":\"Mil\\u0001enko's \\u00e9 servers, a name past "
			"thirty-two\"},\"region\":\"eu west!\",\"network_version\":38,\"dedicated\":true},"
			"{\"id\":\"b\",\"listing\":7},"
			"{\"id\":\"c\"},"
			"\"not an entry\","
			"{\"id\":\"d\",\"listing\":\"not base64!\"},"
			"{\"id\":\"e\",\"listing\":\"%s\"},"
			"{\"id\":\"f\",\"listing\":\"%s\",\"region\":\"us-east-1\",\"dedicated\":\"yes\"}]}",
			base64, large_base64, base64);
		entries_count = 0;
		CHECK(take_list(body, (unsigned long)strlen(body), (const unsigned char *)"listedby") == 2 && entries_count == 2,
			"%d entries taken of the 2 good ones", entries_count);
		CHECK(entries_taken[0].size == size && entries_taken[0].dedicated == 1 && entries_taken[0].version == 38,
			"the first entry was taken as %d bytes, dedicated %d, version %d", entries_taken[0].size,
			entries_taken[0].dedicated, entries_taken[0].version);
		CHECK(!strcmp(entries_taken[0].publisher, "Mil?enko's ?? servers, a name pa") &&
			!strcmp(entries_taken[0].region, "eu?west?"), "the publisher %s, the region %s", entries_taken[0].publisher,
			entries_taken[0].region);
		CHECK(!memcmp(entries_taken[0].master_key_id, "listedby", 8) && !memcmp(entries_taken[1].master_key_id, "listedby", 8),
			"the entries were not taken as the master's");
		CHECK(entries_taken[1].dedicated == 0 && !strcmp(entries_taken[1].region, "us-east-1") &&
			!entries_taken[1].publisher[0], "the last entry was taken as dedicated %d, region %s",
			entries_taken[1].dedicated, entries_taken[1].region);
		entries_count = 0;
		CHECK(!take_list("{\"servers\":7}", 13, (const unsigned char *)"listedby") && !take_list("not json", 8, (const unsigned char *)"listedby") && !entries_count,
			"entries taken from what is no list");
		return 0;
	}
	/* a list of 512 entries and more JSON tokens than a body of the API
	may have: checked and taken, its first 512 entries */
	CASE("list-large")
	{
		static char body[600 * 1024];
		char base64[400];
		unsigned char bytes[400];
		int size, index, member, length;

		size = hex_decode(vector_listings[0].bytes, bytes, sizeof(bytes));
		server_base64_encode(bytes, size, base64);
		length = snprintf(body, sizeof(body), "{\"protocol\":1,\"master\":{\"name\":\"m\",\"key_id\":\"%s\",\"time\":1760900000},"
			"\"servers\":[", VECTOR_MASTER_KEY_ID);
		for (index = 0; index < MAXIMUM_ENTRIES + 3; index++)
		{
			length += snprintf(body + length, sizeof(body) - (size_t)length, "%s{\"id\":\"s%d\",\"listing\":\"%s\"",
				index ? "," : "", index, base64);
			for (member = 0; member < 34; member++)
				length += snprintf(body + length, sizeof(body) - (size_t)length, ",\"f%d\":%d", member, member);
			length += snprintf(body + length, sizeof(body) - (size_t)length, "}");
		}
		length += snprintf(body + length, sizeof(body) - (size_t)length, "]}");
		CHECK(length < (int)sizeof(body) - 1, "the test's list does not fit");
		CHECK(verify_list(body, (unsigned long)length, master_signs(body), VECTOR_LIST_KEY_HEADER,
			VECTOR_MASTER_PUBLIC_KEY, 1760900000), "a list of %d entries was refused", MAXIMUM_ENTRIES + 3);
		entries_count = 0;
		CHECK(take_list(body, (unsigned long)length, (const unsigned char *)"listedby") == MAXIMUM_ENTRIES && entries_count == MAXIMUM_ENTRIES,
			"%d entries were taken of %d", entries_count, MAXIMUM_ENTRIES + 3);
		return 0;
	}
	/* a nonce answered once a second, whatever came between: A, B, A in one
	second answers A once; the remembered nonces are a ring */
	CASE("probe-nonces")
	{
		unsigned char a[64], b[64], answer[128];
		int size, index;

		start_publisher();
		now = 1760900100;
		size = hex_decode(VECTOR_PROBE_PACKET, a, sizeof(a));
		memcpy(b, a, (size_t)size);
		b[34] ^= 0x55;
		CHECK(probe_from(a, size, 0x01000001UL, answer), "nonce A was not answered");
		CHECK(probe_from(b, size, 0x01000002UL, answer), "nonce B was not answered");
		CHECK(!probe_from(a, size, 0x01000003UL, answer), "nonce A was answered again after B in one second");
		now++;
		CHECK(probe_from(a, size, 0x01000003UL, answer), "nonce A was not answered the next second");
		/* (the ring: the oldest of REMEMBERED_NONCES + 1 is forgotten, the
		newest are not) */
		now++;
		for (index = 0; index < REMEMBERED_NONCES; index++)
		{
			memcpy(b, a, (size_t)size);
			b[19] = (unsigned char)index;
			b[20] = 0xEE;
			master.probe_answers = 0;
			memset(master.source_seconds, 0, sizeof(master.source_seconds));
			CHECK(probe_from(b, size, 0x02000000UL + (unsigned long)index, answer), "nonce %d was not answered", index);
		}
		b[19] = (unsigned char)(REMEMBERED_NONCES - 1);
		master.probe_answers = 0;
		CHECK(!probe_from(b, size, 0x03000000UL, answer), "the newest nonce was answered twice");
		return 0;
	}
	/* two answers a second to an address, ten in all: probes forged from
	one address do not use up the answers a master's probe needs; a nonce
	resent counts against neither */
	CASE("probe-sources")
	{
		unsigned char probe[64], answer[128];
		int size, index, sources;

		start_publisher();
		now = 1760900200;
		size = hex_decode(VECTOR_PROBE_PACKET, probe, sizeof(probe));
		/* (one address flooding) */
		for (index = 0; index < 20; index++)
		{
			probe[34] = (unsigned char)index;
			CHECK(probe_from(probe, size, 0x0A0A0A0AUL, answer) == (index < PROBE_ANSWERS_PER_ADDRESS),
				"probe %d from one address was %s", index + 1, index < PROBE_ANSWERS_PER_ADDRESS ? "refused" : "answered");
		}
		probe[34] = 0xF0;
		CHECK(probe_from(probe, size, 0x0B0B0B0BUL, answer), "a master's probe was refused after another's flood");
		CHECK(!probe_from(probe, size, 0x0C0C0C0CUL, answer), "a nonce resent was answered");
		probe[34] = 0xF1;
		CHECK(probe_from(probe, size, 0x0B0B0B0BUL, answer), "a nonce resent counted against its address");
		/* (ten in all a second, whatever the addresses) */
		now++;
		for (index = 0, sources = 0; index < 20; index++)
		{
			probe[34] = (unsigned char)(0x40 + index);
			sources += probe_from(probe, size, 0x0D000000UL + (unsigned long)index, answer);
		}
		CHECK(sources == PROBE_ANSWERS_PER_SECOND, "%d probes of 20 addresses were answered in a second", sources);
		/* (the next second, an address's allowance again) */
		now++;
		probe[34] = 0x7F;
		CHECK(probe_from(probe, size, 0x0A0A0A0AUL, answer), "a flooding address was not answered the next second");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
