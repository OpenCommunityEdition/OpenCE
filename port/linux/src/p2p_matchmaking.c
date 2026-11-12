/*
P2P_MATCHMAKING.C

MATCHMAKING's signed records, queues and search.
*/

#include "p2p_matchmaking.h"
#include "halo_port_limits.h"

#include <stdio.h>
#include <string.h>

static const char record_label[] = "hceu-mm-1";

static const char *const playlist_names[NUMBER_OF_P2P_MM_PLAYLISTS] = { "slayer", "ctf", "king", "oddball", "race" };
static const char *const size_names[NUMBER_OF_P2P_MM_SIZES] = { "1v1", "2v2", "3v3", "4v4", "ffa6" };

void mm_topic(int playlist, int size, char *topic, int topic_size)
{
	snprintf(topic, (size_t)topic_size, "hceu/3/mm/%d/%s/%s", HALO_PORT_NETWORK_VERSION, playlist_names[playlist],
		size_names[size]);
}

int mm_topic_listed(const char *const *topics, int count, const char *topic)
{
	int index;

	for (index = 0; index < count; index++)
		if (topic[0] && !strcmp(topics[index], topic))
			return 1;
	return 0;
}

int mm_interval(int queues)
{
	if (queues <= 12)
		return MM_INTERVAL;
	return (queues * 10 + MM_RECORDS_PER_SECOND - 1) / MM_RECORDS_PER_SECOND;
}

static void put_long(unsigned char *bytes, unsigned long value)
{
	bytes[0] = (unsigned char)(value >> 24);
	bytes[1] = (unsigned char)(value >> 16);
	bytes[2] = (unsigned char)(value >> 8);
	bytes[3] = (unsigned char)value;
}

static unsigned long get_long(const unsigned char *bytes)
{
	return (unsigned long)bytes[0] << 24 | (unsigned long)bytes[1] << 16 | (unsigned long)bytes[2] << 8 | bytes[3];
}

/* 0: a small-order key, which has no X25519 form */
static int key_hash_of(const unsigned char *key, unsigned char *hash)
{
	unsigned char x25519[P2P_KEY_SIZE];

	if (!p2p_ed25519_to_x25519(key, x25519))
		return 0;
	p2p_key_hash(x25519, hash);
	return 1;
}

static int signed_message(const unsigned char *bytes, int size, unsigned char *message)
{
	memcpy(message, record_label, sizeof(record_label) - 1);
	memcpy(message + sizeof(record_label) - 1, bytes, (size_t)size);
	return (int)(sizeof(record_label) - 1) + size;
}

int mm_playlist_has_size(unsigned playlists, int size)
{
	return size != P2P_MM_FFA_6 || playlists != 1u << P2P_MM_CTF;
}

int mm_record_write(const struct mm_record *record, const unsigned char *seed, unsigned char *bytes, int size)
{
	unsigned char message[sizeof(record_label) + MM_MAXIMUM_RECORD_SIZE];
	int at = 0, index;

	if (size < MM_MAXIMUM_RECORD_SIZE || record->pool_count < 0 || record->pool_count > MM_MAXIMUM_POOL)
		return 0;
	bytes[at++] = 'H';
	bytes[at++] = 'M';
	bytes[at++] = MM_FORMAT;
	bytes[at++] = (unsigned char)(record->network_version >> 8);
	bytes[at++] = (unsigned char)record->network_version;
	bytes[at++] = (unsigned char)record->kind;
	put_long(bytes + at, record->sequence);
	at += 4;
	bytes[at++] = (unsigned char)record->interval;
	bytes[at++] = (unsigned char)((record->can_host ? MM_FLAG_CAN_HOST : 0) |
		(record->under_way ? MM_FLAG_UNDER_WAY : 0) | (record->held ? MM_FLAG_HELD : 0));
	bytes[at++] = (unsigned char)record->playlist;
	bytes[at++] = (unsigned char)record->size;
	memcpy(bytes + at, record->key, P2P_KEY_SIZE);
	at += P2P_KEY_SIZE;
	if (record->kind == MM_KIND_HOSTING)
	{
		bytes[at++] = (unsigned char)record->players;
		bytes[at++] = (unsigned char)record->most_players;
		bytes[at++] = (unsigned char)record->pool_count;
		for (index = 0; index < record->pool_count; index++)
		{
			memcpy(bytes + at, record->pool[index], P2P_KEY_HASH_SIZE);
			at += P2P_KEY_HASH_SIZE;
			if (record->held)
			{
				memcpy(bytes + at, record->sealed[index], MM_SEALED_TOKEN_SIZE);
				at += MM_SEALED_TOKEN_SIZE;
			}
		}
		if (!record->held)
		{
			memcpy(bytes + at, record->token, P2P_TOKEN_SIZE);
			at += P2P_TOKEN_SIZE;
		}
	}
	p2p_ed25519_sign(seed, record->key, message, signed_message(bytes, at, message), bytes + at);
	return at + P2P_SIGNATURE_SIZE;
}

int mm_record_read(const unsigned char *bytes, int size, struct mm_record *record)
{
	unsigned char message[sizeof(record_label) + MM_MAXIMUM_RECORD_SIZE];
	int at = MM_HEADER_SIZE, index, entry;

	if (size < MM_HEADER_SIZE + P2P_SIGNATURE_SIZE || size > MM_MAXIMUM_RECORD_SIZE ||
		bytes[0] != 'H' || bytes[1] != 'M' || bytes[2] != MM_FORMAT)
	{
		return 0;
	}
	memset(record, 0, sizeof(*record));
	record->network_version = (unsigned short)(bytes[3] << 8 | bytes[4]);
	record->kind = bytes[5];
	record->sequence = get_long(bytes + 6);
	record->interval = bytes[10];
	record->can_host = (bytes[11] & MM_FLAG_CAN_HOST) != 0;
	record->under_way = (bytes[11] & MM_FLAG_UNDER_WAY) != 0;
	record->held = (bytes[11] & MM_FLAG_HELD) != 0;
	record->playlist = bytes[12];
	record->size = bytes[13];
	memcpy(record->key, bytes + 14, P2P_KEY_SIZE);
	if (record->kind < MM_KIND_SEARCHING || record->kind > MM_KIND_CANCEL ||
		record->playlist >= NUMBER_OF_P2P_MM_PLAYLISTS || record->size >= NUMBER_OF_P2P_MM_SIZES ||
		record->interval < MM_MINIMUM_INTERVAL)
	{
		return 0;
	}
	if (record->kind == MM_KIND_HOSTING)
	{
		if (size < at + 3 + P2P_SIGNATURE_SIZE)
			return 0;
		record->players = bytes[at++];
		record->most_players = bytes[at++];
		record->pool_count = bytes[at++];
		entry = P2P_KEY_HASH_SIZE + (record->held ? MM_SEALED_TOKEN_SIZE : 0);
		if (record->pool_count > MM_MAXIMUM_POOL ||
			size < at + record->pool_count * entry + (record->held ? 0 : P2P_TOKEN_SIZE) + P2P_SIGNATURE_SIZE)
		{
			return 0;
		}
		for (index = 0; index < record->pool_count; index++)
		{
			memcpy(record->pool[index], bytes + at, P2P_KEY_HASH_SIZE);
			at += P2P_KEY_HASH_SIZE;
			if (record->held)
			{
				memcpy(record->sealed[index], bytes + at, MM_SEALED_TOKEN_SIZE);
				at += MM_SEALED_TOKEN_SIZE;
			}
		}
		if (!record->held)
		{
			memcpy(record->token, bytes + at, P2P_TOKEN_SIZE);
			at += P2P_TOKEN_SIZE;
		}
	}
	if (size != at + P2P_SIGNATURE_SIZE)
		return 0;
	if (!p2p_ed25519_verify(record->key, message, signed_message(bytes, at, message), bytes + at))
		return 0;
	return key_hash_of(record->key, record->key_hash);
}

static int token_key(const unsigned char *seed, const unsigned char *other_key, unsigned char *key)
{
	unsigned char public_key[P2P_KEY_SIZE], secret[P2P_KEY_SIZE], other[P2P_KEY_SIZE], shared[P2P_KEY_SIZE];

	p2p_ed25519_public(seed, public_key, secret);
	if (!p2p_ed25519_to_x25519(other_key, other))
		return 0;
	p2p_x25519(shared, secret, other);
	p2p_sha256(shared, P2P_KEY_SIZE, key);
	return 1;
}

int mm_seal_token(const unsigned char *seed, const unsigned char *member_key, const unsigned char *token,
	unsigned char *sealed)
{
	unsigned char key[P2P_KEY_SIZE];

	return token_key(seed, member_key, key) &&
		p2p_seal(key, token, P2P_TOKEN_SIZE, sealed) == MM_SEALED_TOKEN_SIZE;
}

int mm_open_token(const unsigned char *seed, const unsigned char *host_key, const unsigned char *sealed,
	unsigned char *token)
{
	unsigned char key[P2P_KEY_SIZE];

	return token_key(seed, host_key, key) && p2p_open(key, sealed, MM_SEALED_TOKEN_SIZE, token) == P2P_TOKEN_SIZE;
}

/* every machine must sort alike: key hash, never clock or arrival order */
static int hash_before(const struct mm_unit *units, int a, int b)
{
	return memcmp(units[a].key_hash, units[b].key_hash, P2P_KEY_HASH_SIZE) < 0;
}

static void indices_in_order(const struct mm_unit *units, const int *indices, int count, int *order)
{
	int index, other;

	for (index = 0; index < count; index++)
	{
		for (other = index; other > 0 && hash_before(units, indices[index], order[other - 1]); other--)
			order[other] = order[other - 1];
		order[other] = indices[index];
	}
}

int mm_pool(const struct mm_unit *units, int count, int players, int *pool)
{
	int all[MM_MAXIMUM_UNITS + 1], order[MM_MAXIMUM_UNITS + 1], index, taken = 0, pool_count = 0, has_host = 0;

	for (index = 0; index < count; index++)
		all[index] = index;
	indices_in_order(units, all, count, order);
	for (index = 0; index < count && taken < players; index++)
	{
		const struct mm_unit *candidate = &units[order[index]];
		int can_host = candidate->can_host && !candidate->failed;

		if (taken + candidate->players > players)
			continue;
		if (!can_host && !has_host && taken + candidate->players == players)
			continue;
		pool[pool_count++] = order[index];
		taken += candidate->players;
		has_host |= can_host;
	}
	return taken == players && has_host ? pool_count : 0;
}

int mm_elect(const struct mm_unit *units, const int *pool, int pool_count)
{
	unsigned char material[MM_MAXIMUM_POOL * P2P_KEY_HASH_SIZE + P2P_KEY_SIZE], digest[P2P_SHA256_SIZE];
	unsigned char best[P2P_SHA256_SIZE];
	int sorted[MM_MAXIMUM_POOL], index, at = 0, unit, host = -1;

	if (pool_count > MM_MAXIMUM_POOL)
		return -1;
	/* sorted so every machine hashes the same bytes */
	indices_in_order(units, pool, pool_count, sorted);
	for (index = 0; index < pool_count; index++)
	{
		memcpy(material + at, units[sorted[index]].key_hash, P2P_KEY_HASH_SIZE);
		at += P2P_KEY_HASH_SIZE;
	}
	for (index = 0; index < pool_count; index++)
	{
		unit = pool[index];
		if (!units[unit].can_host || units[unit].failed)
			continue;
		memcpy(material + at, units[unit].key, P2P_KEY_SIZE);
		p2p_sha256(material, at + P2P_KEY_SIZE, digest);
		if (host < 0 || memcmp(digest, best, sizeof(best)) < 0)
		{
			host = unit;
			memcpy(best, digest, sizeof(best));
		}
	}
	return host;
}

int mm_counted(const struct mm_peer *peer)
{
	return peer->refreshes >= MM_REFRESHES_COUNTED;
}

/* the queues of the chosen playlists at the chosen size */
static void set_queues(struct mm_search *search)
{
	int playlist;

	search->queue_count = search->next_queue = 0;
	for (playlist = 0; playlist < NUMBER_OF_P2P_MM_PLAYLISTS; playlist++)
	{
		if (!(search->choice.playlists & 1u << playlist))
			continue;
		if (!mm_playlist_has_size(1u << playlist, search->choice.size))
			continue;
		search->queues[search->queue_count][0] = playlist;
		search->queues[search->queue_count++][1] = search->choice.size;
	}
}

void mm_start(struct mm_search *search, const unsigned char *seed, const struct p2p_mm_choice *choice,
	unsigned long now, unsigned long unix_now)
{
	unsigned long sequence = search->sequence;

	memset(search, 0, sizeof(*search));
	search->sequence = sequence;
	memcpy(search->seed, seed, P2P_SEED_SIZE);
	p2p_ed25519_public(seed, search->key, NULL);
	key_hash_of(search->key, search->key_hash);
	search->choice = *choice;
	search->started = unix_now;
	search->marks_from = search->next_record = now;
	set_queues(search);
	search->state = P2P_MM_QUEUED;
}

static int in_queue(const struct mm_search *search, int playlist, int size)
{
	int index;

	for (index = 0; index < search->queue_count; index++)
		if (search->queues[index][0] == playlist && search->queues[index][1] == size)
			return 1;
	return 0;
}

static struct mm_peer *find_peer(struct mm_search *search, const unsigned char *key_hash, int playlist, int size)
{
	int index;

	for (index = 0; index < MM_MAXIMUM_UNITS; index++)
	{
		struct mm_peer *peer = &search->peers[index];

		if (peer->used && peer->playlist == playlist && peer->size == size &&
			!memcmp(peer->record.key_hash, key_hash, P2P_KEY_HASH_SIZE))
		{
			return peer;
		}
	}
	return NULL;
}

static struct mm_peer *take_slot(struct mm_search *search, unsigned long now)
{
	struct mm_peer *victim = NULL;
	int index;

	for (index = 0; index < MM_MAXIMUM_UNITS; index++)
	{
		struct mm_peer *peer = &search->peers[index];

		if (!peer->used)
			return peer;
		if (mm_counted(peer))
			continue;
		if (!victim || now - peer->heard > now - victim->heard)
			victim = peer;
	}
	return victim;
}

void mm_heard(struct mm_search *search, const struct mm_record *record, unsigned long now)
{
	struct mm_peer *peer;

	if (search->state == P2P_MM_OFF || record->network_version != HALO_PORT_NETWORK_VERSION ||
		!memcmp(record->key, search->key, P2P_KEY_SIZE) || !in_queue(search, record->playlist, record->size))
	{
		return;
	}
	peer = find_peer(search, record->key_hash, record->playlist, record->size);
	if (peer && record->sequence <= peer->record.sequence)
		return;
	if (!peer)
	{
		peer = take_slot(search, now);
		if (!peer)
			return;
		memset(peer, 0, sizeof(*peer));
		peer->used = 1;
		peer->refreshes = -1;
		peer->playlist = record->playlist;
		peer->size = record->size;
	}
	peer->record = *record;
	peer->heard = now;
	peer->refreshes++;
}

void p2p_mm_heard(const unsigned char *payload, int size)
{
	(void)payload;
	(void)size;
}
