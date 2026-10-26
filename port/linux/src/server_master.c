/*
SERVER_MASTER.C

A dedicated server's publishing to the official master servers
(server_master.h; docs/master_protocol.md).
*/

#include "platform.h"
#include "port_config.h"
#include "server_master.h"
#include "server_mode.h"

#if HALO_SERVER_BUILD

#include "p2p.h"
#include "p2p_internal.h"
#include "posix.h"
#include "posix_server.h"
#include "server_events.h"
#include "server_json.h"
#include "server_log.h"
#include "update.h"
#include "master_list.h"

#include <SDL3/SDL.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- the publisher key */

static int hex_value(char character)
{
	if (character >= '0' && character <= '9')
		return character - '0';
	if (character >= 'a' && character <= 'f')
		return character - 'a' + 10;
	if (character >= 'A' && character <= 'F')
		return character - 'A' + 10;
	return -1;
}

/* a seed's 64 hex digits (spaces and a comment after them allowed): 1 */
static int seed_from_text(const char *text, unsigned char *seed)
{
	int index;

	while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r')
		text++;
	for (index = 0; index < SERVER_MASTER_SEED_SIZE; index++)
	{
		int high = hex_value(text[2 * index]), low = high >= 0 ? hex_value(text[2 * index + 1]) : -1;

		if (high < 0 || low < 0)
			return 0;
		seed[index] = (unsigned char)(high << 4 | low);
	}
	text += 2 * SERVER_MASTER_SEED_SIZE;
	return !*text || *text == ' ' || *text == '\t' || *text == '\r' || *text == '\n' || *text == '#';
}

/* the key file's path: beside config.toml unless a full path */
static void key_file_path(char *path, int size)
{
	const char *setting = config_string("server.publisher_key_file");
	char folder[1024];

	if (setting[0] == '/' || (setting[0] && setting[1] == ':'))
	{
		snprintf(path, (size_t)size, "%s", setting);
		return;
	}
	config_folder(folder, sizeof(folder));
	snprintf(path, (size_t)size, "%s%s", folder, setting[0] ? setting : "publisher.key");
}

/* the seed: HALO_SERVER_PUBLISHER_KEY's, else the file's first line that is
not a comment; 1 */
static int publisher_seed(unsigned char *seed)
{
	const char *environment = getenv("HALO_SERVER_PUBLISHER_KEY");
	char path[1100];
	char *text;
	size_t size;
	const char *line;
	int found = 0;

	if (environment && environment[0])
		return seed_from_text(environment, seed);
	key_file_path(path, sizeof(path));
	text = config_file_read(path, &size);
	if (!text)
		return 0;
	for (line = text; *line && !found;)
	{
		const char *end = line + strcspn(line, "\n");

		while (*line == ' ' || *line == '\t')
			line++;
		if (*line != '#' && line < end)
			found = seed_from_text(line, seed);
		line = *end ? end + 1 : end;
	}
	memset(text, 0, size);
	free(text);
	return found;
}

static void key_id_of(const unsigned char *public_key, char *key_id)
{
	unsigned char digest[P2P_SHA256_SIZE];

	p2p_sha256(public_key, SERVER_MASTER_PUBLIC_KEY_SIZE, digest);
	p2p_hex(digest, SERVER_MASTER_KEY_ID_SIZE, key_id);
}

int server_master_publisher(unsigned char *public_key, char *key_id)
{
	unsigned char seed[SERVER_MASTER_SEED_SIZE];

	if (!publisher_seed(seed))
		return 0;
	p2p_ed25519_public(seed, public_key, NULL);
	memset(seed, 0, sizeof(seed));
	key_id_of(public_key, key_id);
	return 1;
}

/* a signature by the publisher key: 1 */
static int publisher_sign(const void *message, int size, unsigned char *signature)
{
	unsigned char seed[SERVER_MASTER_SEED_SIZE];
	unsigned char public_key[SERVER_MASTER_PUBLIC_KEY_SIZE];

	if (!publisher_seed(seed))
		return 0;
	p2p_ed25519_public(seed, public_key, NULL);
	p2p_ed25519_sign(seed, public_key, message, size, signature);
	memset(seed, 0, sizeof(seed));
	return 1;
}

int server_master_keygen(void)
{
	unsigned char seed[SERVER_MASTER_SEED_SIZE];
	unsigned char public_key[SERVER_MASTER_PUBLIC_KEY_SIZE];
	char key_id[2 * SERVER_MASTER_KEY_ID_SIZE + 1];
	char hex[2 * SERVER_MASTER_SEED_SIZE + 1];
	char path[1100];
	char text[512];
	FILE *file;
	int written;

	key_file_path(path, sizeof(path));
	if (getenv("HALO_SERVER_PUBLISHER_KEY") && getenv("HALO_SERVER_PUBLISHER_KEY")[0])
	{
		if (!publisher_seed(seed))
		{
			fprintf(stderr, "HALO_SERVER_PUBLISHER_KEY is not 64 hexadecimal digits\n");
			return 0;
		}
	}
	else if (!publisher_seed(seed))
	{
		file = fopen(path, "rb");
		if (file)
		{
			fclose(file);
			fprintf(stderr, "%s is there but holds no key (64 hexadecimal digits): move it aside to make a new one\n",
				path);
			return 0;
		}
		posix_random_bytes(seed, sizeof(seed));
		p2p_hex(seed, SERVER_MASTER_SEED_SIZE, hex);
		snprintf(text, sizeof(text),
			"# The publisher key of this server's operator for the OpenCE master servers (halo --server\n"
			"# --keygen). Keep it secret: whoever has it publishes as you. Its public key is in the\n"
			"# application you make to a master.\n%s\n", hex);
		memset(hex, 0, sizeof(hex));
		/* (only its owner may read it, from its creation: no moment when the
		key is in a file others can read) */
		written = posix_server_write_private_file(path, text, (unsigned long)strlen(text));
		memset(text, 0, sizeof(text));
		if (!written)
		{
			fprintf(stderr, "cannot write %s\n", path);
			return 0;
		}
		fprintf(stdout, "wrote a new publisher key to %s\n", path);
	}
	else
	{
		fprintf(stdout, "the publisher key in %s:\n", path);
	}
	p2p_ed25519_public(seed, public_key, NULL);
	memset(seed, 0, sizeof(seed));
	key_id_of(public_key, key_id);
	p2p_hex(public_key, SERVER_MASTER_PUBLIC_KEY_SIZE, hex);
	fprintf(stdout, "public key: ed25519:%s\nkey id: %s\n", hex, key_id);
	return 1;
}

int server_master_sign_text(const char *text)
{
	unsigned char signature[P2P_SIGNATURE_SIZE];
	char hex[2 * P2P_SIGNATURE_SIZE + 1];

	if (!publisher_sign(text, (int)strlen(text), signature))
	{
		fprintf(stderr, "there is no publisher key (halo --server --keygen makes one)\n");
		return 0;
	}
	p2p_hex(signature, P2P_SIGNATURE_SIZE, hex);
	fprintf(stdout, "%s\n", hex);
	return 1;
}

/* ---------- the masters */

struct master
{
	char url[256];
	unsigned char key[SERVER_MASTER_PUBLIC_KEY_SIZE];
	unsigned char key_id[SERVER_MASTER_KEY_ID_SIZE];
	/* the publisher thread's, read under the lock by server_master_status */
	int listed;
	char probe[16];
	char code[32];
	char message[160];
	unsigned long updated;
	/* a refusal's: when it may be asked again */
	unsigned long retry_at;
	/* whether the last publish listed the game (a player's host that stops
	hosting is withdrawn, and a server gone private: its tombstone is sent) */
	int published;
	/* the master revoked the key or its publisher (key_revoked,
	publisher_revoked): not asked again this run */
	int revoked;
	/* the outcome last logged, when the failed probe's hint was, and when
	an outcome logged once a minute was */
	char logged[200];
	unsigned long hint_logged;
	unsigned long repeat_logged;
};

enum
{
	PUBLISH_SECONDS = 30,
	CHANGE_SECONDS = 5,
	STOP_MILLISECONDS = 2000,
	/* a probe's answers a second at most, in all and to one address, the
	nonces remembered, and the addresses counted in a second */
	PROBE_ANSWERS_PER_SECOND = 10,
	PROBE_ANSWERS_PER_ADDRESS = 2,
	REMEMBERED_NONCES = 64,
	REMEMBERED_SOURCES = 32,
	PROBE_SIZE = 35,
	PROBE_ANSWER_SIZE = 103,
	MAXIMUM_BODY = 16 * 1024,
	MAXIMUM_ANSWER = 64 * 1024,
};

static struct
{
	SDL_Mutex *lock;
	SDL_Condition *wake;
	SDL_Thread *thread;
	int started;
	int stopping;
	int stopped;
	struct master masters[SERVER_MASTER_MAXIMUM_MASTERS];
	int master_count;
	/* the publisher key, held for the run (the probes' answers need it on the
	internet play thread) */
	int has_key;
	unsigned char seed[SERVER_MASTER_SEED_SIZE];
	unsigned char public_key[SERVER_MASTER_PUBLIC_KEY_SIZE];
	char key_id[2 * SERVER_MASTER_KEY_ID_SIZE + 1];
	char instance[33];
	char server_id[17];
	/* the game's state (server_master_set_state), and a change waiting */
	char state[1024];
	int changed;
	unsigned long serial;
	/* the probes answered: when, how many this second, the nonces, and
	how many to each address this second */
	unsigned long probe_second;
	int probe_answers;
	unsigned char nonces[REMEMBERED_NONCES][16];
	unsigned long nonce_seconds[REMEMBERED_NONCES];
	int next_nonce;
	unsigned long source_addresses[REMEMBERED_SOURCES];
	unsigned long source_seconds[REMEMBERED_SOURCES];
	int source_answers[REMEMBERED_SOURCES];
} master;

/* the instance: server.instance, else a slug of the server's name */
static void instance_name(char *instance, int size)
{
	const char *setting = config_string("server.instance");
	/* (a player's host: "host", unless server.instance names it) */
	const char *source = setting[0] ? setting : halo_server_mode() ? config_string("server.name") : "host";
	int length = 0, index;

	for (index = 0; source[index] && length < size - 1 && length < 32; index++)
	{
		char character = (char)tolower((unsigned char)source[index]);

		if ((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '_' ||
			character == '-')
		{
			instance[length++] = character;
		}
		else if ((character == ' ' || character == '.') && length && instance[length - 1] != '-')
		{
			instance[length++] = '-';
		}
	}
	while (length && instance[length - 1] == '-')
		length--;
	/* (the first a letter or a digit) */
	while (length && (instance[0] == '-' || instance[0] == '_'))
	{
		memmove(instance, instance + 1, (size_t)length - 1);
		length--;
	}
	instance[length] = 0;
	if (!length)
		snprintf(instance, (size_t)size, "main");
}

static void hex_text(const unsigned char *bytes, int size, char *text)
{
	p2p_hex(bytes, size, text);
}

/* ---------- a publish */

/* the Authorization header of a signed request to the master of that key
id (its list key's, pinned in masters.txt: a request signed for one master
is no good to another) */
static void sign_request(const unsigned char *master_key_id, const char *method, const char *path, const char *body,
	unsigned long size, char *header, int header_size)
{
	char master_key_id_text[2 * SERVER_MASTER_KEY_ID_SIZE + 1];
	unsigned char nonce_bytes[16];
	char nonce[33];
	unsigned char digest[P2P_SHA256_SIZE];
	char digest_text[2 * P2P_SHA256_SIZE + 1];
	char text[2600];
	unsigned char signature[P2P_SIGNATURE_SIZE];
	char signature_text[100];
	unsigned long now = posix_server_unix_seconds();

	posix_random_bytes(nonce_bytes, sizeof(nonce_bytes));
	hex_text(nonce_bytes, 16, nonce);
	p2p_sha256(body ? body : "", (int)size, digest);
	hex_text(digest, P2P_SHA256_SIZE, digest_text);
	hex_text(master_key_id, SERVER_MASTER_KEY_ID_SIZE, master_key_id_text);
	snprintf(text, sizeof(text), "OpenCE-Master-v1\n%s\n%s\n%s\n%lu\n%s\n%s", master_key_id_text, method, path, now,
		nonce, digest_text);
	p2p_ed25519_sign(master.seed, master.public_key, text, (int)strlen(text), signature);
	server_base64_encode(signature, P2P_SIGNATURE_SIZE, signature_text);
	snprintf(header, (size_t)header_size,
		"Authorization: OpenCE-Ed25519 keyid=%s, ts=%lu, nonce=%s, sig=%s\r\nContent-Type: application/json\r\n",
		master.key_id, now, nonce, signature_text);
}

/* the publish's body: the listing, what it lacks, closed for a tombstone */
static int publish_body(struct server_json *json, int closed, int *has_listing)
{
	unsigned char listing[400];
	char listing_text[600];
	int size = p2p_lobby_host_listing(listing, sizeof(listing));
	struct server_json_document document;
	char public_address[64];

	*has_listing = size > 0;
	server_json_begin(json);
	server_json_object_begin(json, NULL);
	server_json_integer(json, "protocol", 1);
	server_json_string(json, "instance", master.instance);
	if (size > 0)
	{
		server_base64_encode(listing, size, listing_text);
		server_json_string(json, "listing", listing_text);
	}
	/* (the port the tunnel's socket has: a player's host binds any free one,
	network.tunnel_port 0; it is in network byte order) */
	{
		unsigned short port = p2p_tunnel_local_port();

		server_json_integer(json, "tunnel_port", (long long)(unsigned short)(port << 8 | port >> 8));
	}
	server_json_boolean(json, "dedicated", halo_server_mode() != 0);
	server_json_string(json, "build", server_mode_build());
	if (config_string("server.region")[0])
		server_json_string(json, "region", config_string("server.region"));
	/* (network.public_address as internet play looked it up, an IPv4
	address and port: a host name, or IPv6, the master would refuse) */
	if (config_boolean("server.master_publish_address") && p2p_public_address(public_address, sizeof(public_address)))
		server_json_string(json, "address", public_address);
	/* (what the game's state adds: the variant's name) */
	SDL_LockMutex(master.lock);
	if (master.state[0] && server_json_parse(&document, master.state, (int)strlen(master.state), 8))
	{
		char variant[64];

		if (server_json_get_string(&document, server_json_member(&document, 0, "variant"), variant, sizeof(variant)) &&
			variant[0])
		{
			server_json_string(json, "variant", variant);
		}
		server_json_document_free(&document);
	}
	server_json_integer(json, "serial", (long long)master.serial);
	SDL_UnlockMutex(master.lock);
	server_json_boolean(json, "closed", closed);
	server_json_object_end(json);
	return 1;
}

/* a response's header's value as a number (its name in any case): 0 if it
is not there */
static unsigned long header_number(const char *headers, const char *name)
{
	size_t length = strlen(name);
	const char *line = headers;

	while (line && *line)
	{
		size_t index;

		for (index = 0; index < length && line[index] &&
			tolower((unsigned char)line[index]) == tolower((unsigned char)name[index]); index++)
		{
		}
		if (index == length && line[length] == ':')
			return strtoul(line + length + 1, NULL, 10);
		line = strchr(line, '\n');
		if (line)
			line++;
	}
	return 0;
}

/* whether a master's refusal is logged again once a minute while it lasts
(docs/master_protocol.md, "Errors") */
static int logged_each_minute(const char *code)
{
	return !strcmp(code, "unknown_key") || !strcmp(code, "publisher_pending") || !strcmp(code, "application_denied") ||
		!strcmp(code, "listing_taken");
}

/* a master's answer taken: its state, logged when it changes */
static void publish_answer(struct master *each, struct update_response *response, const char *error, int closed)
{
	struct server_json_document document;
	char code[32] = "", message[160] = "", probe[16] = "", reason[160] = "", outcome[200];
	int listed = 0, value;
	unsigned long retry = 0, now = posix_server_unix_seconds();
	double master_time = 0;
	int has_master_time = 0;

	if (error)
	{
		snprintf(code, sizeof(code), "unreachable");
		snprintf(message, sizeof(message), "%s", error);
	}
	else if (server_json_parse(&document, response->body, (int)response->body_size, 8))
	{
		int failure = server_json_member(&document, 0, "error");

		if (server_json_get_boolean(&document, server_json_member(&document, 0, "listed"), &value))
			listed = value;
		server_json_get_string(&document, server_json_member(&document, 0, "probe"), probe, sizeof(probe));
		server_json_get_string(&document, server_json_member(&document, 0, "reason"), reason, sizeof(reason));
		if (failure >= 0)
		{
			server_json_get_string(&document, server_json_member(&document, failure, "code"), code, sizeof(code));
			server_json_get_string(&document, server_json_member(&document, failure, "message"), message,
				sizeof(message));
		}
		has_master_time = server_json_get_number(&document, server_json_member(&document, 0, "master_time"),
			&master_time);
		server_json_document_free(&document);
	}
	else
	{
		snprintf(code, sizeof(code), "status_%d", response->status);
		snprintf(message, sizeof(message), "the master answered %d", response->status);
	}
	if (response && response->status == 429)
	{
		/* (a day at most: a master's word, but not a run's end) */
		retry = header_number(response->headers, "Retry-After");
		each->retry_at = now + (retry ? (retry < 86400 ? retry : 86400) : 30);
	}
	/* (revoked for good: not asked again; a suspended publisher, an unknown
	key: asked less often) */
	else if (!strcmp(code, "key_revoked") || !strcmp(code, "publisher_revoked"))
	{
		each->revoked = 1;
		each->published = 0;
	}
	else if (!strcmp(code, "publisher_suspended") || !strcmp(code, "unknown_key"))
	{
		each->retry_at = now + 60;
	}
	/* (the machine's clock off by more than the master takes: by how much,
	if the master said its time) */
	if (!strcmp(code, "clock") && has_master_time)
	{
		snprintf(message, sizeof(message), "this machine's clock is %.0f seconds %s the master's: set it right",
			master_time > (double)now ? master_time - (double)now : (double)now - master_time,
			master_time > (double)now ? "behind" : "ahead of");
	}
	SDL_LockMutex(master.lock);
	each->listed = listed;
	snprintf(each->probe, sizeof(each->probe), "%s", probe);
	snprintf(each->code, sizeof(each->code), "%s", code);
	snprintf(each->message, sizeof(each->message), "%s", message[0] ? message : reason);
	each->updated = posix_server_unix_seconds();
	SDL_UnlockMutex(master.lock);
	snprintf(outcome, sizeof(outcome), "%d %s %s", listed, probe, code);
	if (strcmp(outcome, each->logged) || (logged_each_minute(code) && now - each->repeat_logged >= 60))
	{
		int changed = strcmp(outcome, each->logged) != 0;
		struct server_json json;

		snprintf(each->logged, sizeof(each->logged), "%s", outcome);
		each->repeat_logged = now;
		if (closed && !code[0])
			server_log(_server_log_info, "master", "%s: withdrawn", each->url);
		else if (listed)
			server_log(_server_log_info, "master", "%s: listed (probe %s)", each->url, probe[0] ? probe : "-");
		else if (!strcmp(code, "publisher_pending"))
			server_log(_server_log_warning, "master", "%s: publisher pending: apply at %s/apply with the public key "
				"sv_publisher shows", each->url, each->url);
		else if (!strcmp(code, "unknown_key"))
			server_log(_server_log_warning, "master", "%s: unknown key: apply with key %s (%s/apply; sv_publisher "
				"shows its public key)", each->url, master.key_id, each->url);
		else if (each->revoked)
			server_log(_server_log_warning, "master", "%s: %s (%s): not published there again", each->url, code,
				message);
		else if (code[0])
			server_log(_server_log_warning, "master", "%s: not listed: %s (%s)", each->url, code, message);
		else
			server_log(_server_log_info, "master", "%s: not listed yet (probe %s)", each->url, probe[0] ? probe : "-");
		/* (the event at a change alone) */
		if (changed)
		{
			server_json_begin(&json);
			server_json_object_begin(&json, NULL);
			server_json_string(&json, "event", "master");
			server_json_string(&json, "url", each->url);
			server_json_boolean(&json, "listed", listed);
			server_json_string(&json, "probe", probe);
			server_json_string(&json, "code", code);
			server_json_string(&json, "message", message[0] ? message : reason);
			server_json_object_end(&json);
			server_event_add("master", server_json_text(&json));
			server_json_free(&json);
		}
	}
	/* (the failed probe's hint, once a minute until it passes) */
	if (!strcmp(probe, "failed") && posix_server_unix_seconds() - each->hint_logged >= 60)
	{
		each->hint_logged = posix_server_unix_seconds();
		server_log(_server_log_warning, "master", "%s: %s", each->url, reason[0] ? reason :
			"the master cannot reach this server: let its UDP port through your firewall or NAT, or set "
			"network.public_address");
	}
}

static void publish_to(struct master *each, int closed)
{
	struct server_json json;
	struct update_response response;
	char header[512], url[400], path[64], error[256];
	int has_listing;

	if (each->revoked || (!closed && each->retry_at > posix_server_unix_seconds()))
		return;
	publish_body(&json, closed, &has_listing);
	/* (no listing: the game is not hosting for the internet, or no longer,
	when the listing published is withdrawn) */
	if (!has_listing && !closed)
	{
		server_json_free(&json);
		if (!each->published)
			return;
		closed = 1;
		publish_body(&json, closed, &has_listing);
	}
	each->published = !closed;
	snprintf(path, sizeof(path), "/v1/servers/%s", master.server_id);
	snprintf(url, sizeof(url), "%s%s", each->url, path);
	{
		const char *body = server_json_text(&json);
		unsigned long size = (unsigned long)strlen(body);
		/* (the path after the scheme and host: https:// or a test's http://) */
		const char *host = strstr(each->url, "://");
		const char *base_path = strchr(host ? host + 3 : each->url, '/');
		char signed_path[400];

		/* (a master served under a path signs the whole path) */
		snprintf(signed_path, sizeof(signed_path), "%s%s", base_path ? base_path : "", path);
		sign_request(each->key_id, "PUT", signed_path, body, size, header, sizeof(header));
		if (update_https_request("PUT", url, header, body, size, MAXIMUM_ANSWER, &response, error, sizeof(error)))
		{
			publish_answer(each, &response, NULL, closed);
			free(response.body);
		}
		else
		{
			publish_answer(each, NULL, error, closed);
		}
	}
	server_json_free(&json);
}

static int SDLCALL master_thread_main(void *unused)
{
	Uint64 last_publish = 0;
	int index;

	(void)unused;
	SDL_LockMutex(master.lock);
	while (!master.stopping)
	{
		Uint64 now = SDL_GetTicksNS();
		int due = !last_publish || now - last_publish >= (Uint64)PUBLISH_SECONDS * 1000000000ULL ||
			(master.changed && now - last_publish >= (Uint64)CHANGE_SECONDS * 1000000000ULL);
		int withdrawing = 0;

		/* (a listing published to a master: one more publish, which withdraws
		it, once the server is private or off the internet) */
		for (index = 0; index < master.master_count; index++)
			withdrawing |= master.masters[index].published;
		/* (a player's host: its PUBLIC, which the listing says) */
		if (!due || (!withdrawing && ((halo_server_mode() && !config_boolean("server.public")) ||
			!config_boolean("network.online"))))
		{
			SDL_WaitConditionTimeout(master.wake, master.lock, 1000);
			continue;
		}
		master.changed = 0;
		master.serial++;
		last_publish = now;
		SDL_UnlockMutex(master.lock);
		for (index = 0; index < master.master_count; index++)
			publish_to(&master.masters[index], 0);
		SDL_LockMutex(master.lock);
	}
	SDL_UnlockMutex(master.lock);
	/* the tombstone, to each (a stop waits for it 2 seconds at most): a
	player's host's only where its game is listed */
	for (index = 0; index < master.master_count; index++)
	{
		if (halo_server_mode() || master.masters[index].published)
			publish_to(&master.masters[index], 1);
	}
	SDL_LockMutex(master.lock);
	master.stopped = 1;
	SDL_BroadcastCondition(master.wake);
	SDL_UnlockMutex(master.lock);
	return 0;
}

static void master_start(int quiet);

void server_master_start(void)
{
	master_start(0);
}

void server_master_start_host(void)
{
	if (!halo_server_mode())
		master_start(1);
}

/* the masters and the key read, and the thread started if there are both
(quiet: a player's host, which says nothing of what it lacks) */
static void master_start(int quiet)
{
	char urls[SERVER_MASTER_MAXIMUM_MASTERS][256];
	unsigned char keys[SERVER_MASTER_MAXIMUM_MASTERS][SERVER_MASTER_PUBLIC_KEY_SIZE];
	int index;

	master.lock = SDL_CreateMutex();
	master.wake = SDL_CreateCondition();
	master.master_count = master_list_read(config_string("server.masters_file"), urls, keys,
		SERVER_MASTER_MAXIMUM_MASTERS);
	for (index = 0; index < master.master_count; index++)
	{
		unsigned char digest[P2P_SHA256_SIZE];

		snprintf(master.masters[index].url, sizeof(master.masters[index].url), "%s", urls[index]);
		memcpy(master.masters[index].key, keys[index], SERVER_MASTER_PUBLIC_KEY_SIZE);
		p2p_sha256(keys[index], SERVER_MASTER_PUBLIC_KEY_SIZE, digest);
		memcpy(master.masters[index].key_id, digest, SERVER_MASTER_KEY_ID_SIZE);
	}
	master.has_key = publisher_seed(master.seed);
	if (master.has_key)
	{
		char text[64];
		unsigned char digest[P2P_SHA256_SIZE];

		p2p_ed25519_public(master.seed, master.public_key, NULL);
		key_id_of(master.public_key, master.key_id);
		instance_name(master.instance, sizeof(master.instance));
		snprintf(text, sizeof(text), "%s%s", master.key_id, master.instance);
		p2p_sha256(text, (int)strlen(text), digest);
		hex_text(digest, 8, master.server_id);
	}
	if (!config_boolean("server.master_publish"))
		return;
	if (quiet && (!master.master_count || !master.has_key))
		return;
	if (!master.master_count)
	{
		server_log(_server_log_info, "master", "no master servers (%s): the server is joined by its invite and on the LAN",
			config_string("server.masters_file"));
		return;
	}
	if (!master.has_key)
	{
		server_log(_server_log_warning, "master", "no publisher key (halo --server --keygen makes one, "
			"HALO_SERVER_PUBLISHER_KEY holds one): not published to the %d master server%s", master.master_count,
			master.master_count == 1 ? "" : "s");
		return;
	}
	server_log(_server_log_info, "master", "publishing to %d master server%s as %s (key id %s, server id %s)",
		master.master_count, master.master_count == 1 ? "" : "s", master.instance, master.key_id, master.server_id);
	master.thread = SDL_CreateThread(master_thread_main, "server masters", NULL);
	master.started = master.thread != NULL;
	/* (a player's host withdrawn as the game exits: the director stops a
	dedicated server's itself) */
	if (quiet && master.started)
		atexit(server_master_stop);
}

void server_master_set_state(const char *members, int changed)
{
	if (!master.started)
		return;
	SDL_LockMutex(master.lock);
	snprintf(master.state, sizeof(master.state), "%s", members ? members : "");
	if (changed)
	{
		master.changed = 1;
		SDL_SignalCondition(master.wake);
	}
	SDL_UnlockMutex(master.lock);
}

void server_master_changed(void)
{
	if (!master.started)
		return;
	SDL_LockMutex(master.lock);
	master.changed = 1;
	SDL_SignalCondition(master.wake);
	SDL_UnlockMutex(master.lock);
}

void server_master_stop(void)
{
	Uint64 deadline;

	if (!master.started)
		return;
	deadline = SDL_GetTicksNS() + (Uint64)STOP_MILLISECONDS * 1000000ULL;
	SDL_LockMutex(master.lock);
	master.stopping = 1;
	SDL_SignalCondition(master.wake);
	while (!master.stopped && SDL_GetTicksNS() < deadline)
		SDL_WaitConditionTimeout(master.wake, master.lock, 100);
	SDL_UnlockMutex(master.lock);
}

/* whether a probe from the address may be answered this second: two to an
address at most, counted (the addresses of this second only: more than the
table holds is past the ten a second in all) */
static int probe_source_allowed(unsigned long address, unsigned long second)
{
	int index, free_index = -1;

	for (index = 0; index < REMEMBERED_SOURCES; index++)
	{
		if (master.source_seconds[index] != second)
		{
			if (free_index < 0)
				free_index = index;
		}
		else if (master.source_addresses[index] == address)
		{
			return ++master.source_answers[index] <= PROBE_ANSWERS_PER_ADDRESS;
		}
	}
	if (free_index < 0)
		return 0;
	master.source_addresses[free_index] = address;
	master.source_seconds[free_index] = second;
	master.source_answers[free_index] = 1;
	return 1;
}

int server_master_probe(const unsigned char *data, int size, unsigned long address, unsigned char *answer,
	int *answer_size)
{
	unsigned char message[5 + SERVER_MASTER_KEY_ID_SIZE + 16];
	unsigned long second;
	int index, known = 0;

	if (size != PROBE_SIZE || memcmp(data, "OPCE-PROBE\x01", 11) || !master.has_key)
		return 0;
	for (index = 0; index < master.master_count; index++)
		known |= !memcmp(master.masters[index].key_id, data + 11, SERVER_MASTER_KEY_ID_SIZE);
	if (!known)
		return 0;
	second = posix_server_unix_seconds();
	if (second != master.probe_second)
	{
		master.probe_second = second;
		master.probe_answers = 0;
	}
	/* (a nonce answered once a second at most, as docs/master_protocol.md
	says: a master's probe round sends one nonce for its attempts, a second
	or more apart, and a new one each round; a longer window than a second
	would refuse a round's retries. A nonce resent counts against nothing) */
	for (index = 0; index < REMEMBERED_NONCES; index++)
	{
		if (master.nonce_seconds[index] == second && !memcmp(master.nonces[index], data + 19, 16))
			return 0;
	}
	/* (two a second to an address, and ten in all: a probe reflects
	nothing, and probes forged from one address do not use up the answers a
	master's would have) */
	if (!probe_source_allowed(address, second) || ++master.probe_answers > PROBE_ANSWERS_PER_SECOND)
		return 0;
	memcpy(master.nonces[master.next_nonce], data + 19, 16);
	master.nonce_seconds[master.next_nonce] = second;
	master.next_nonce = (master.next_nonce + 1) % REMEMBERED_NONCES;
	memcpy(message, "probe", 5);
	memcpy(message + 5, data + 11, SERVER_MASTER_KEY_ID_SIZE + 16);
	memcpy(answer, "OPCE-PROBE-ACK\x01", 15);
	memcpy(answer + 15, data + 11, SERVER_MASTER_KEY_ID_SIZE + 16);
	p2p_ed25519_sign(master.seed, master.public_key, message, (int)sizeof(message), answer + 39);
	*answer_size = PROBE_ANSWER_SIZE;
	return 1;
}

void server_master_identity(char *instance, int instance_size, char *key_id, int key_id_size)
{
	instance_name(instance, instance_size);
	snprintf(key_id, (size_t)key_id_size, "%s", master.has_key ? master.key_id : "");
}

void server_master_status(struct server_json *json)
{
	int index;

	server_json_array_begin(json, "masters");
	if (master.lock)
		SDL_LockMutex(master.lock);
	for (index = 0; index < master.master_count; index++)
	{
		struct master const *each = &master.masters[index];

		server_json_object_begin(json, NULL);
		server_json_string(json, "url", each->url);
		server_json_boolean(json, "listed", each->listed);
		server_json_string(json, "probe", each->probe[0] ? each->probe : (master.started ? "pending" : "off"));
		server_json_string(json, "code", master.started ? each->code : !master.has_key ? "no_key" : "off");
		server_json_string(json, "message", each->message);
		server_json_integer(json, "updated", each->updated);
		server_json_object_end(json);
	}
	if (master.lock)
		SDL_UnlockMutex(master.lock);
	server_json_array_end(json);
}

#else

int server_master_keygen(void)
{
	return 0;
}

int server_master_sign_text(const char *text)
{
	(void)text;
	return 0;
}

int server_master_publisher(unsigned char *public_key, char *key_id)
{
	(void)public_key;
	(void)key_id;
	return 0;
}

void server_master_start(void)
{
}

void server_master_start_host(void)
{
}

void server_master_set_state(const char *members, int changed)
{
	(void)members;
	(void)changed;
}

void server_master_stop(void)
{
}

void server_master_changed(void)
{
}

int server_master_probe(const unsigned char *data, int size, unsigned long address, unsigned char *answer,
	int *answer_size)
{
	(void)data;
	(void)size;
	(void)address;
	(void)answer;
	(void)answer_size;
	return 0;
}

void server_master_identity(char *instance, int instance_size, char *key_id, int key_id_size)
{
	if (instance_size > 0)
		instance[0] = 0;
	if (key_id_size > 0)
		key_id[0] = 0;
}

void server_master_status(struct server_json *json)
{
	(void)json;
}

#endif
