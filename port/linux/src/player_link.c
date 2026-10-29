/*
PLAYER_LINK.C

A player's Discord account linked at a master server (player_link.h).
*/

#include "platform.h"
#include "port_config.h"
#include "player_link.h"
#include "master_list.h"
#include "server_master.h"
#include "p2p.h"
#include "p2p_internal.h"
#include "posix.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef HALO_WINDOWS
#include <fcntl.h>
#include <unistd.h>
#endif

enum
{
	/* a pass asked for again when less than this is left of it */
	REFRESH_SECONDS = 48 * 60 * 60,
	/* a master that has no pass for the key (not linked there), or did not
	answer, asked again after this */
	RETRY_SECONDS = 6 * 60 * 60,
	FAILED_RETRY_SECONDS = 10 * 60,
	/* while the link page is open: asked every few seconds, for so long */
	WAIT_POLL_SECONDS = 3,
	WAIT_SECONDS = 10 * 60,
	/* the times a pass may be ahead of or behind the clock */
	CLOCK_SLACK_SECONDS = 300,
	/* joins made again with a pass, at most, for one join */
	MAXIMUM_RETRIES = PLAYER_LINK_MAXIMUM_MASTERS + 1,
};

/* ---------- the format */

static unsigned long long get_u64(const unsigned char *bytes)
{
	unsigned long long value = 0;
	int index;

	for (index = 0; index < 8; index++)
		value = value << 8 | bytes[index];
	return value;
}

int player_link_pass_read(const unsigned char *pass, int size, struct player_link_pass *fields)
{
	int index;

	if (size != PLAYER_LINK_PASS_SIZE || memcmp(pass, "OPCEPASS", 8) || pass[8] != 1)
		return 0;
	memset(fields, 0, sizeof(*fields));
	fields->flags = pass[9];
	memcpy(fields->master_key_id, pass + 10, PLAYER_LINK_KEY_ID_SIZE);
	memcpy(fields->player_key, pass + 18, PLAYER_LINK_KEY_SIZE);
	fields->discord_id = get_u64(pass + 50);
	/* (printable ASCII, up to its first zero) */
	for (index = 0; index < PLAYER_LINK_NAME_SIZE && pass[58 + index]; index++)
	{
		unsigned char character = pass[58 + index];

		fields->discord_name[index] = character >= 0x20 && character < 0x7F ? (char)character : '?';
	}
	fields->discord_name[index] = 0;
	fields->issued = get_u64(pass + 90);
	fields->expires = get_u64(pass + 98);
	return 1;
}

int player_link_pass_signed(const unsigned char *pass, const unsigned char *master_key)
{
	unsigned char message[13 + PLAYER_LINK_PASS_SIGNED_SIZE];

	memcpy(message, "opence-pass-1", 13);
	memcpy(message + 13, pass, PLAYER_LINK_PASS_SIGNED_SIZE);
	return p2p_ed25519_verify(master_key, message, (int)sizeof(message), pass + PLAYER_LINK_PASS_SIGNED_SIZE);
}

void player_link_binding(const unsigned char *secret, unsigned char *binding)
{
	if (!secret)
	{
		memset(binding, 0, PLAYER_LINK_BINDING_SIZE);
		return;
	}
	p2p_hmac_sha256(secret, P2P_SHA256_SIZE, "opence-link-binding-1", 21, binding);
}

void player_link_proof_message(const unsigned char *pass, const unsigned char *binding, unsigned char *message)
{
	memcpy(message, "opence-pass-proof-1", 19);
	p2p_sha256(pass, PLAYER_LINK_PASS_SIZE, message + 19);
	memcpy(message + 19 + 32, binding, PLAYER_LINK_BINDING_SIZE);
}

int player_link_proof_check(const unsigned char *pass, const unsigned char *binding, const unsigned char *proof)
{
	unsigned char message[PLAYER_LINK_PROOF_MESSAGE_SIZE];

	player_link_proof_message(pass, binding, message);
	return p2p_ed25519_verify(pass + 18, message, (int)sizeof(message), proof);
}

void player_link_code(const unsigned char *public_key, unsigned long long ts, char *code)
{
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
	unsigned char hashed[PLAYER_LINK_KEY_SIZE + 24];
	unsigned char digest[P2P_SHA256_SIZE];
	char text[24];
	unsigned long bits;
	int length, index;

	length = snprintf(text, sizeof(text), "%llu", ts);
	memcpy(hashed, public_key, PLAYER_LINK_KEY_SIZE);
	memcpy(hashed + PLAYER_LINK_KEY_SIZE, text, (size_t)length);
	p2p_sha256(hashed, PLAYER_LINK_KEY_SIZE + length, digest);
	/* (the first 30 bits, five to a letter of the base32's) */
	bits = (unsigned long)digest[0] << 24 | (unsigned long)digest[1] << 16 | (unsigned long)digest[2] << 8 | digest[3];
	for (index = 0; index < 6; index++)
		code[index < 3 ? index : index + 1] = alphabet[(bits >> (27 - 5 * index)) & 31];
	code[3] = '-';
	code[7] = 0;
}

void player_link_request_text(const char *prefix, const unsigned char *master_key_id,
	const unsigned char *public_key, unsigned long long ts, char *text, int size)
{
	char key_id[2 * PLAYER_LINK_KEY_ID_SIZE + 1];
	char key[2 * PLAYER_LINK_KEY_SIZE + 1];

	p2p_hex(master_key_id, PLAYER_LINK_KEY_ID_SIZE, key_id);
	p2p_hex(public_key, PLAYER_LINK_KEY_SIZE, key);
	snprintf(text, (size_t)size, "%s\n%s\n%s\n%llu", prefix, key_id, key, ts);
}

int player_link_check(const unsigned char *pass, const unsigned char *proof, const unsigned char *binding,
	const unsigned char (*master_keys)[PLAYER_LINK_KEY_SIZE], int master_count, unsigned long long now,
	struct player_link_pass *fields)
{
	int index;

	if (!player_link_pass_read(pass, PLAYER_LINK_PASS_SIZE, fields))
		return 0;
	/* (its master, by the key id it names, one of those taken) */
	for (index = 0; index < master_count; index++)
	{
		unsigned char digest[P2P_SHA256_SIZE];

		p2p_sha256(master_keys[index], PLAYER_LINK_KEY_SIZE, digest);
		if (!memcmp(digest, fields->master_key_id, PLAYER_LINK_KEY_ID_SIZE))
			break;
	}
	if (index == master_count || !player_link_pass_signed(pass, master_keys[index]))
		return 0;
	if (fields->issued > now + CLOCK_SLACK_SECONDS || fields->expires + CLOCK_SLACK_SECONDS < now)
		return 0;
	return player_link_proof_check(pass, binding, proof);
}

unsigned long long player_link_discord_created(unsigned long long discord_id)
{
	return ((discord_id >> 22) + 1420070400000ULL) / 1000;
}

/* whether the key id is one of the server's masters' */
static int listed_by(const unsigned char *key_id, const unsigned char (*server_key_ids)[PLAYER_LINK_KEY_ID_SIZE],
	int server_count)
{
	int index;

	for (index = 0; index < server_count; index++)
	{
		if (!memcmp(key_id, server_key_ids[index], PLAYER_LINK_KEY_ID_SIZE))
			return 1;
	}
	return 0;
}

int player_link_next_pass(const unsigned char (*key_ids)[PLAYER_LINK_KEY_ID_SIZE], const int *usable, int count,
	const unsigned char (*server_key_ids)[PLAYER_LINK_KEY_ID_SIZE], int server_count, int after, int *needed)
{
	int index;

	*needed = -1;
	for (index = after < 0 ? 0 : after + 1; index < count; index++)
	{
		if (usable[index] && (!server_count || listed_by(key_ids[index], server_key_ids, server_count)))
			return index;
	}
	/* (a server listed by masters this machine knows, none of which it holds
	a pass of: the one to link with) */
	for (index = 0; server_count && after < 0 && index < count; index++)
	{
		if (listed_by(key_ids[index], server_key_ids, server_count))
		{
			*needed = index;
			break;
		}
	}
	return -1;
}

/* ---------- a server */

int player_link_pass_shown(const unsigned char *pass)
{
	int index;

	for (index = 0; index < PLAYER_LINK_PASS_SIZE; index++)
	{
		if (pass[index])
			return 1;
	}
	return 0;
}

int player_link_server_check(const unsigned char *pass, const unsigned char *proof, unsigned long virtual_address,
	char *discord_id, int discord_id_size, char *discord_name, int discord_name_size, unsigned long *created,
	char *why, int why_size)
{
	unsigned char keys[SERVER_MASTER_MAXIMUM_MASTERS][PLAYER_LINK_KEY_SIZE];
	unsigned char binding[PLAYER_LINK_BINDING_SIZE];
	struct player_link_pass fields;
	int count = server_master_publishing_keys(keys, SERVER_MASTER_MAXIMUM_MASTERS);
	unsigned long long now = (unsigned long long)time(NULL);

	discord_id[0] = 0;
	discord_name[0] = 0;
	*created = 0;
	if (!virtual_address || !p2p_peer_binding(virtual_address, binding))
		player_link_binding(NULL, binding);
	if (!count)
	{
		snprintf(why, (size_t)why_size, "the server publishes to no master server, whose passes it would take");
		return 0;
	}
	if (!player_link_pass_read(pass, PLAYER_LINK_PASS_SIZE, &fields))
	{
		snprintf(why, (size_t)why_size, "the pass does not read as one");
		return 0;
	}
	if (!player_link_check(pass, proof, binding, (const unsigned char (*)[PLAYER_LINK_KEY_SIZE])keys, count, now,
		&fields))
	{
		snprintf(why, (size_t)why_size, "the pass is another master's, out of date, or not this connection's");
		return 0;
	}
	snprintf(discord_id, (size_t)discord_id_size, "%llu", fields.discord_id);
	snprintf(discord_name, (size_t)discord_name_size, "%s", fields.discord_name);
	*created = (unsigned long)player_link_discord_created(fields.discord_id);
	why[0] = 0;
	return 1;
}

/* ---------- a player's machine */

static struct
{
	pthread_mutex_t lock;
	pthread_cond_t wake;
	int started;
	int has_key;
	unsigned char seed[P2P_SEED_SIZE];
	unsigned char public_key[PLAYER_LINK_KEY_SIZE];
	/* the masters (master_list_client_masters'), and of each: its key id,
	its pass, when to ask it next, and its revocation */
	int master_count;
	char urls[PLAYER_LINK_MAXIMUM_MASTERS][MASTER_LIST_URL_SIZE];
	unsigned char keys[PLAYER_LINK_MAXIMUM_MASTERS][PLAYER_LINK_KEY_SIZE];
	unsigned char key_ids[PLAYER_LINK_MAXIMUM_MASTERS][PLAYER_LINK_KEY_ID_SIZE];
	unsigned char passes[PLAYER_LINK_MAXIMUM_MASTERS][PLAYER_LINK_PASS_SIZE];
	struct player_link_pass fields[PLAYER_LINK_MAXIMUM_MASTERS];
	int has_pass[PLAYER_LINK_MAXIMUM_MASTERS];
	unsigned long long next_ask[PLAYER_LINK_MAXIMUM_MASTERS];
	int revoked[PLAYER_LINK_MAXIMUM_MASTERS];
	/* the link page opened for a master, when, and its code */
	int waiting;
	int waiting_master;
	unsigned long long waiting_since;
	char code[PLAYER_LINK_CODE_SIZE];
	/* the join's pass: shown, which one, and how many joins were made again;
	and the master a refusing server is listed by that this machine has no
	pass of (-1: none) */
	int attach;
	int attach_index;
	int retries;
	int needed_master;
} player = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER };

/* under player.lock: waits for seconds at most, or a wake */
static void wait_seconds(int seconds)
{
	struct timespec until;

	clock_gettime(CLOCK_REALTIME, &until);
	until.tv_sec += seconds;
	pthread_cond_timedwait(&player.wake, &player.lock, &until);
}

static unsigned long long now_seconds(void)
{
	return (unsigned long long)time(NULL);
}

/* a file beside config.toml */
static void file_path(const char *name, char *path, int size)
{
	char folder[1024];

	config_folder(folder, sizeof(folder));
	snprintf(path, (size_t)size, "%s%s", folder, name);
}

/* a file written whole, its owner's alone where the system has owners: 1 */
static int write_file(const char *path, const void *data, unsigned long size)
{
#ifdef HALO_WINDOWS
	FILE *file = fopen(path, "wb");
	int written;

	if (!file)
		return 0;
	written = fwrite(data, 1, size, file) == size;
	return fclose(file) == 0 && written;
#else
	const unsigned char *bytes = data;
	unsigned long written = 0;
	int descriptor = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0600);

	if (descriptor < 0)
		return 0;
	while (written < size)
	{
		long count = (long)write(descriptor, bytes + written, (size_t)(size - written));

		if (count <= 0)
			break;
		written += (unsigned long)count;
	}
	return close(descriptor) == 0 && written == size;
#endif
}

static int hex_digit(char character)
{
	if (character >= '0' && character <= '9')
		return character - '0';
	if (character >= 'a' && character <= 'f')
		return character - 'a' + 10;
	if (character >= 'A' && character <= 'F')
		return character - 'A' + 10;
	return -1;
}

/* the player key from player.key (its first line that is not a comment: 64
hex digits); 1 */
static int read_key(void)
{
	char path[1100];
	size_t size;
	char *text;
	const char *line;
	int found = 0;

	file_path("player.key", path, sizeof(path));
	text = config_file_read(path, &size);
	if (!text)
		return 0;
	for (line = text; *line && !found;)
	{
		const char *end = line + strcspn(line, "\n");
		int index;

		while (*line == ' ' || *line == '\t')
			line++;
		if (*line != '#' && end - line >= 2 * P2P_SEED_SIZE)
		{
			for (index = 0; index < P2P_SEED_SIZE; index++)
			{
				int high = hex_digit(line[2 * index]), low = high >= 0 ? hex_digit(line[2 * index + 1]) : -1;

				if (high < 0 || low < 0)
					break;
				player.seed[index] = (unsigned char)(high << 4 | low);
			}
			found = index == P2P_SEED_SIZE;
		}
		line = *end ? end + 1 : end;
	}
	memset(text, 0, size);
	free(text);
	if (found)
		p2p_ed25519_public(player.seed, player.public_key, NULL);
	return found;
}

/* a new player key, written to player.key; 1 */
static int make_key(void)
{
	char path[1100];
	char hex[2 * P2P_SEED_SIZE + 1];
	char text[512];
	int written;

	file_path("player.key", path, sizeof(path));
	posix_random_bytes(player.seed, sizeof(player.seed));
	p2p_hex(player.seed, P2P_SEED_SIZE, hex);
	snprintf(text, sizeof(text),
		"# This machine's player key for the OpenCE master servers: it holds the Discord account\n"
		"# linked to it (Settings, Network Setup, Discord Account). Keep it secret: whoever has it\n"
		"# joins servers as that account.\n%s\n", hex);
	memset(hex, 0, sizeof(hex));
	written = write_file(path, text, (unsigned long)strlen(text));
	memset(text, 0, sizeof(text));
	if (!written)
	{
		platform_log("Discord link: cannot write %s", path);
		memset(player.seed, 0, sizeof(player.seed));
		return 0;
	}
	p2p_ed25519_public(player.seed, player.public_key, NULL);
	platform_log("Discord link: made a player key (%s)", path);
	return 1;
}

static void pass_path(int index, char *path, int size)
{
	char key_id[2 * PLAYER_LINK_KEY_ID_SIZE + 1];
	char name[64];

	p2p_hex(player.key_ids[index], PLAYER_LINK_KEY_ID_SIZE, key_id);
	snprintf(name, sizeof(name), "pass-%s.bin", key_id);
	file_path(name, path, size);
}

/* under player.lock: a pass taken for the master at the index, if it is one
of its, for this key: 1 */
static int take_pass(int index, const unsigned char *pass, int size)
{
	struct player_link_pass fields;

	if (!player_link_pass_read(pass, size, &fields) || memcmp(fields.master_key_id, player.key_ids[index],
		PLAYER_LINK_KEY_ID_SIZE) || memcmp(fields.player_key, player.public_key, PLAYER_LINK_KEY_SIZE) ||
		!player_link_pass_signed(pass, player.keys[index]))
	{
		return 0;
	}
	/* (an older one than that held is not taken) */
	if (player.has_pass[index] && fields.issued < player.fields[index].issued)
		return 0;
	memcpy(player.passes[index], pass, PLAYER_LINK_PASS_SIZE);
	player.fields[index] = fields;
	player.has_pass[index] = 1;
	player.revoked[index] = 0;
	return 1;
}

/* under player.lock: whether the master's pass is good now */
static int pass_current(int index, unsigned long long now)
{
	return player.has_pass[index] && player.fields[index].expires > now + 60 &&
		player.fields[index].issued <= now + CLOCK_SLACK_SECONDS;
}

static void read_masters_and_passes(void)
{
	char urls[PLAYER_LINK_MAXIMUM_MASTERS][MASTER_LIST_URL_SIZE];
	unsigned char keys[PLAYER_LINK_MAXIMUM_MASTERS][MASTER_LIST_KEY_SIZE];
	int count = master_list_client_masters(urls, keys);
	int index;

	pthread_mutex_lock(&player.lock);
	player.master_count = count;
	for (index = 0; index < count; index++)
	{
		unsigned char digest[P2P_SHA256_SIZE];
		char path[1100];
		size_t size;
		char *bytes;

		memcpy(player.urls[index], urls[index], MASTER_LIST_URL_SIZE);
		memcpy(player.keys[index], keys[index], PLAYER_LINK_KEY_SIZE);
		p2p_sha256(keys[index], PLAYER_LINK_KEY_SIZE, digest);
		memcpy(player.key_ids[index], digest, PLAYER_LINK_KEY_ID_SIZE);
		player.has_pass[index] = 0;
		player.next_ask[index] = 0;
		if (!player.has_key)
			continue;
		pass_path(index, path, sizeof(path));
		bytes = config_file_read(path, &size);
		if (bytes)
		{
			take_pass(index, (const unsigned char *)bytes, (int)size);
			free(bytes);
		}
	}
	pthread_mutex_unlock(&player.lock);
}

/* a master asked for its pass (GET /v1/pass, signed in its query): the
answer's status, 0 if none came; the pass taken and kept on 200 */
static int ask_pass(int index)
{
	char text[256], url[MASTER_LIST_URL_SIZE + 400];
	char key[2 * PLAYER_LINK_KEY_SIZE + 1], signature_hex[2 * P2P_SIGNATURE_SIZE + 1];
	unsigned char signature[P2P_SIGNATURE_SIZE];
	unsigned long long ts = now_seconds();
	char *body = NULL;
	unsigned long size = 0;
	int status = 0;

	pthread_mutex_lock(&player.lock);
	player_link_request_text("opence-pass-get-1", player.key_ids[index], player.public_key, ts, text, sizeof(text));
	p2p_ed25519_sign(player.seed, player.public_key, text, (int)strlen(text), signature);
	p2p_hex(player.public_key, PLAYER_LINK_KEY_SIZE, key);
	p2p_hex(signature, P2P_SIGNATURE_SIZE, signature_hex);
	snprintf(url, sizeof(url), "%s/v1/pass?key=%s&ts=%llu&sig=%s", player.urls[index], key, ts, signature_hex);
	pthread_mutex_unlock(&player.lock);
	if (!master_list_get(url, &status, &body, &size))
		return 0;
	pthread_mutex_lock(&player.lock);
	if (status == 200)
	{
		if (take_pass(index, (const unsigned char *)body, (int)size))
		{
			char path[1100];

			pass_path(index, path, sizeof(path));
			if (!write_file(path, player.passes[index], PLAYER_LINK_PASS_SIZE))
				platform_log("Discord link: cannot write %s", path);
			if (player.waiting && player.waiting_master == index)
			{
				player.waiting = 0;
				platform_log("Discord link: linked as %s at %s", player.fields[index].discord_name, player.urls[index]);
			}
		}
		else
		{
			platform_log("Discord link: %s's pass was refused (not this key's, or not signed by its key)",
				player.urls[index]);
			status = -1;
		}
	}
	else if (status == 403 && body && strstr(body, "link_revoked"))
	{
		player.revoked[index] = 1;
		player.has_pass[index] = 0;
		player.waiting = player.waiting && player.waiting_master != index;
		platform_log("Discord link: %s revoked the Discord account linked to this machine", player.urls[index]);
	}
	pthread_mutex_unlock(&player.lock);
	free(body);
	return status;
}

static void *link_thread_main(void *argument)
{
	(void)argument;
	read_masters_and_passes();
	pthread_mutex_lock(&player.lock);
	for (;;)
	{
		unsigned long long now = now_seconds();
		int index, asked = -1;

		if (player.waiting && now - player.waiting_since > WAIT_SECONDS)
			player.waiting = 0;
		for (index = 0; index < player.master_count && asked < 0 && player.has_key; index++)
		{
			int due = now >= player.next_ask[index];

			/* (the link page's master, every few seconds while it is open;
			the others' passes before they run out) */
			if (player.waiting && player.waiting_master == index)
				due = now >= player.next_ask[index];
			else if (player.has_pass[index] && player.fields[index].expires > now + REFRESH_SECONDS)
				due = 0;
			if (due && !player.revoked[index])
				asked = index;
		}
		if (asked >= 0)
		{
			int status;

			player.next_ask[asked] = now + (player.waiting && player.waiting_master == asked ? WAIT_POLL_SECONDS :
				RETRY_SECONDS);
			pthread_mutex_unlock(&player.lock);
			status = ask_pass(asked);
			pthread_mutex_lock(&player.lock);
			if (!status || status >= 500 || status == 429)
			{
				if (!(player.waiting && player.waiting_master == asked))
					player.next_ask[asked] = now_seconds() + FAILED_RETRY_SECONDS;
			}
			else if (status == 200 && player.has_pass[asked])
			{
				player.next_ask[asked] = player.fields[asked].expires > REFRESH_SECONDS ?
					player.fields[asked].expires - REFRESH_SECONDS : now_seconds() + RETRY_SECONDS;
			}
			continue;
		}
		wait_seconds(player.waiting ? 1 : 60);
	}
	return NULL;
}

void player_link_start(void)
{
	pthread_t thread;

	pthread_mutex_lock(&player.lock);
	if (player.started)
	{
		pthread_mutex_unlock(&player.lock);
		return;
	}
	player.started = 1;
	player.needed_master = -1;
	player.has_key = read_key();
	pthread_mutex_unlock(&player.lock);
	if (pthread_create(&thread, NULL, link_thread_main, NULL) == 0)
		pthread_detach(thread);
}

/* under player.lock: which of the masters' passes are good now */
static void usable_passes(int *usable, unsigned long long now)
{
	int index;

	for (index = 0; index < player.master_count; index++)
		usable[index] = player.has_key && pass_current(index, now);
}

int player_link_join_fill(unsigned long host_virtual_address, unsigned char *pass, unsigned char *proof)
{
	const char *share = config_string("network.link_share");
	unsigned long long now = now_seconds();
	unsigned char binding[PLAYER_LINK_BINDING_SIZE];
	unsigned char message[PLAYER_LINK_PROOF_MESSAGE_SIZE];
	unsigned char server_key_ids[PLAYER_LINK_MAXIMUM_MASTERS][PLAYER_LINK_KEY_ID_SIZE];
	int usable[PLAYER_LINK_MAXIMUM_MASTERS];
	int server_count, filled = 0;

	if (!strcmp(share, "never"))
		return 0;
	/* (the masters that list the game joined: read before player.lock, which
	p2p.c's lock is never taken under) */
	server_count = p2p_joined_listing_masters(server_key_ids, PLAYER_LINK_MAXIMUM_MASTERS);
	if (!host_virtual_address || !p2p_peer_binding(host_virtual_address, binding))
		player_link_binding(NULL, binding);
	pthread_mutex_lock(&player.lock);
	usable_passes(usable, now);
	/* ("always": the pass of a master that lists the game, unless one was
	refused already) */
	if (!player.attach && !strcmp(share, "always"))
	{
		int needed, index = player_link_next_pass((const unsigned char (*)[PLAYER_LINK_KEY_ID_SIZE])player.key_ids,
			usable, player.master_count, (const unsigned char (*)[PLAYER_LINK_KEY_ID_SIZE])server_key_ids,
			server_count, -1, &needed);

		if (index >= 0)
		{
			player.attach = 1;
			player.attach_index = index;
		}
	}
	if (player.attach && player.attach_index < player.master_count && usable[player.attach_index])
	{
		memcpy(pass, player.passes[player.attach_index], PLAYER_LINK_PASS_SIZE);
		player_link_proof_message(pass, binding, message);
		p2p_ed25519_sign(player.seed, player.public_key, message, (int)sizeof(message), proof);
		filled = 1;
	}
	pthread_mutex_unlock(&player.lock);
	return filled;
}

/* (network_game_protocol.h's) */
enum
{
	_refused_vpn_link_required = 8,
	_refused_vpn_link_invalid = 10,
};

int player_link_join_refused(int rejection_code)
{
	unsigned long long now = now_seconds();
	unsigned char server_key_ids[PLAYER_LINK_MAXIMUM_MASTERS][PLAYER_LINK_KEY_ID_SIZE];
	int usable[PLAYER_LINK_MAXIMUM_MASTERS];
	int server_count, again = 0, needed = -1;

	if (!strcmp(config_string("network.link_share"), "never"))
		return 0;
	server_count = p2p_joined_listing_masters(server_key_ids, PLAYER_LINK_MAXIMUM_MASTERS);
	pthread_mutex_lock(&player.lock);
	usable_passes(usable, now);
	if (((rejection_code == _refused_vpn_link_required && !player.attach) ||
		(rejection_code == _refused_vpn_link_invalid && player.attach)) && player.retries < MAXIMUM_RETRIES)
	{
		/* (the pass of the master this machine linked with, of those that
		list the game; or, after one refused, the next such) */
		int index = player_link_next_pass((const unsigned char (*)[PLAYER_LINK_KEY_ID_SIZE])player.key_ids, usable,
			player.master_count, (const unsigned char (*)[PLAYER_LINK_KEY_ID_SIZE])server_key_ids, server_count,
			rejection_code == _refused_vpn_link_invalid ? player.attach_index : -1, &needed);

		if (index >= 0)
		{
			player.attach = 1;
			player.attach_index = index;
			player.retries++;
			again = 1;
			platform_log("Discord link: the server asks for a Discord account: joining again with %s's pass",
				player.urls[index]);
		}
	}
	if (!again)
	{
		player.attach = 0;
		player.retries = 0;
		if (rejection_code == _refused_vpn_link_required)
		{
			player.needed_master = needed;
			if (needed >= 0)
				platform_log("Discord link: the server blocks VPNs; it is listed by %s, which this machine is not "
					"linked with: link Discord with it (Settings, Network Setup, Discord Account)",
					player.urls[needed]);
			else
				platform_log("Discord link: the server blocks VPNs; link a Discord account with a master server it "
					"is listed by (Settings, Network Setup, Discord Account) or turn the VPN off");
		}
	}
	pthread_mutex_unlock(&player.lock);
	return again;
}

void player_link_join_reset(void)
{
	pthread_mutex_lock(&player.lock);
	player.attach = 0;
	player.retries = 0;
	player.needed_master = -1;
	pthread_mutex_unlock(&player.lock);
}

int player_link_needed_master(void)
{
	/* (read without player.lock: p2p.c asks under its own; none before the
	start, which a dedicated server never makes) */
	if (!__atomic_load_n(&player.started, __ATOMIC_RELAXED))
		return -1;
	return __atomic_load_n(&player.needed_master, __ATOMIC_RELAXED);
}

int player_link_begin(int master_index)
{
	char text[256], url[MASTER_LIST_URL_SIZE + 400];
	char key[2 * PLAYER_LINK_KEY_SIZE + 1], signature_hex[2 * P2P_SIGNATURE_SIZE + 1];
	unsigned char signature[P2P_SIGNATURE_SIZE];
	unsigned long long ts = now_seconds();

	player_link_start();
	/* (the masters read here if the thread has not read them yet) */
	pthread_mutex_lock(&player.lock);
	if (!player.master_count)
	{
		pthread_mutex_unlock(&player.lock);
		read_masters_and_passes();
		pthread_mutex_lock(&player.lock);
	}
	if (master_index < 0 || master_index >= player.master_count)
	{
		platform_log("Discord link: no master server %d to link with (masters.txt names %d)", master_index + 1,
			player.master_count);
		pthread_mutex_unlock(&player.lock);
		return 0;
	}
	if (!player.has_key)
		player.has_key = make_key();
	if (!player.has_key)
	{
		pthread_mutex_unlock(&player.lock);
		return 0;
	}
	player_link_request_text("opence-link-1", player.key_ids[master_index], player.public_key, ts, text,
		sizeof(text));
	p2p_ed25519_sign(player.seed, player.public_key, text, (int)strlen(text), signature);
	p2p_hex(player.public_key, PLAYER_LINK_KEY_SIZE, key);
	p2p_hex(signature, P2P_SIGNATURE_SIZE, signature_hex);
	snprintf(url, sizeof(url), "%s/link?key=%s&ts=%llu&sig=%s", player.urls[master_index], key, ts, signature_hex);
	player_link_code(player.public_key, ts, player.code);
	player.waiting = 1;
	player.waiting_master = master_index;
	player.waiting_since = ts;
	player.revoked[master_index] = 0;
	player.next_ask[master_index] = ts + WAIT_POLL_SECONDS;
	pthread_cond_broadcast(&player.wake);
	pthread_mutex_unlock(&player.lock);
	platform_log("Discord link: opening %s/link in the browser (the page shows the code %s)",
		player.urls[master_index], player.code);
	platform_open_url(url);
	return 1;
}

void player_link_unlink(void)
{
	int index;

	pthread_mutex_lock(&player.lock);
	for (index = 0; index < player.master_count; index++)
	{
		char path[1100];

		pass_path(index, path, sizeof(path));
		remove(path);
		player.has_pass[index] = 0;
		player.revoked[index] = 0;
	}
	if (player.has_key)
	{
		char path[1100];

		file_path("player.key", path, sizeof(path));
		remove(path);
	}
	memset(player.seed, 0, sizeof(player.seed));
	player.has_key = 0;
	player.waiting = 0;
	player.attach = 0;
	pthread_mutex_unlock(&player.lock);
	platform_log("Discord link: unlinked (the player key and its passes deleted)");
}

int player_link_state(char *name, int name_size, char *code, int code_size, int *linked, int *masters)
{
	unsigned long long now = now_seconds();
	int index, state = _player_link_state_none;

	name[0] = 0;
	code[0] = 0;
	*linked = 0;
	pthread_mutex_lock(&player.lock);
	*masters = player.master_count;
	for (index = 0; index < player.master_count; index++)
	{
		if (player.has_key && pass_current(index, now))
		{
			if (!(*linked)++)
				snprintf(name, (size_t)name_size, "%s", player.fields[index].discord_name);
			state = _player_link_state_linked;
		}
		else if (player.revoked[index] && state != _player_link_state_linked)
		{
			state = _player_link_state_revoked;
		}
		else if (player.has_pass[index] && state == _player_link_state_none)
		{
			state = _player_link_state_expired;
			snprintf(name, (size_t)name_size, "%s", player.fields[index].discord_name);
		}
	}
	if (state != _player_link_state_linked && player.waiting)
	{
		state = _player_link_state_waiting;
		snprintf(code, (size_t)code_size, "%s", player.code);
	}
	pthread_mutex_unlock(&player.lock);
	return state;
}

int player_link_master_state(int master_index, char *host, int host_size, char *name, int name_size, char *code,
	int code_size)
{
	unsigned long long now = now_seconds();
	int state = _player_link_state_none;

	host[0] = 0;
	name[0] = 0;
	code[0] = 0;
	pthread_mutex_lock(&player.lock);
	if (master_index >= 0 && master_index < player.master_count)
	{
		/* (its address's host: after the scheme, to the port or path) */
		const char *start = strstr(player.urls[master_index], "://");

		start = start ? start + 3 : player.urls[master_index];
		snprintf(host, (size_t)host_size, "%.*s", (int)strcspn(start, ":/"), start);
		if (player.has_key && pass_current(master_index, now))
		{
			state = _player_link_state_linked;
			snprintf(name, (size_t)name_size, "%s", player.fields[master_index].discord_name);
		}
		else if (player.waiting && player.waiting_master == master_index)
		{
			state = _player_link_state_waiting;
			snprintf(code, (size_t)code_size, "%s", player.code);
		}
		else if (player.revoked[master_index])
		{
			state = _player_link_state_revoked;
		}
		else if (player.has_key && player.has_pass[master_index])
		{
			state = _player_link_state_expired;
			snprintf(name, (size_t)name_size, "%s", player.fields[master_index].discord_name);
		}
		else if (player.needed_master == master_index)
		{
			state = _player_link_state_needed;
		}
	}
	pthread_mutex_unlock(&player.lock);
	return state;
}
