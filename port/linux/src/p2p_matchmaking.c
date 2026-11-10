/*
P2P_MATCHMAKING.C

MATCHMAKING's signed records, queues and search.
*/

#include "p2p_matchmaking.h"
#include "halo_port_limits.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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
	int all[MM_MAXIMUM_UNITS + 1], order[MM_MAXIMUM_UNITS + 1], index, taken = 0, has_host = 0;

	for (index = 0; index < count; index++)
		all[index] = index;
	indices_in_order(units, all, count, order);
	for (index = 0; index < count && taken < players; index++)
	{
		const struct mm_unit *candidate = &units[order[index]];
		int can_host = candidate->can_host;

		if (candidate->failed)
			continue;
		if (!can_host && !has_host && taken + 1 == players)
			continue;
		pool[taken++] = order[index];
		has_host |= can_host;
	}
	return taken == players && has_host ? taken : 0;
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

int p2p_mm_size_players(int size)
{
	static const int players[NUMBER_OF_P2P_MM_SIZES] = { 2, 4, 6, 8, 6 };

	return players[size];
}

/* each playlist's game engine (a listing's engine_type) */
static const int engines[NUMBER_OF_P2P_MM_PLAYLISTS] = { 2, 1, 4, 3, 5 };

void mm_wanted(const struct mm_search *search, struct p2p_lobby_wanted *wanted)
{
	int playlist;

	memset(wanted, 0, sizeof(*wanted));
	for (playlist = 0; playlist < NUMBER_OF_P2P_MM_PLAYLISTS; playlist++)
	{
		if (!(search->choice.playlists & 1u << playlist))
			continue;
		if (!mm_playlist_has_size(1u << playlist, search->choice.size))
			continue;
		wanted->engines |= 1u << engines[playlist];
	}
	wanted->players = p2p_mm_size_players(search->choice.size);
	wanted->teams = search->choice.size != P2P_MM_FFA_6;
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
	search->state = P2P_MM_LOOKING;
}

void mm_game_lost(struct mm_search *search, unsigned long now)
{
	struct p2p_mm_choice choice = search->choice;
	unsigned char seed[P2P_SEED_SIZE];
	unsigned long kept_start = search->started;
	int can_host = search->can_host;

	memcpy(seed, search->seed, sizeof(seed));
	mm_start(search, seed, &choice, now, kept_start);
	search->can_host = can_host;
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
	/* a cancelled entry stays as a tombstone, so older records of its key are dropped */
	if (record->kind == MM_KIND_CANCEL)
		peer->refreshes = -1;
	else
		peer->refreshes++;
	if (record->kind == MM_KIND_HOSTING && peer->failed == MM_FAILED_PASSED_OVER)
		peer->failed = MM_FAILED_NONE;
}

static void expire(struct mm_search *search, unsigned long now)
{
	int index;

	for (index = 0; index < MM_MAXIMUM_UNITS; index++)
	{
		struct mm_peer *peer = &search->peers[index];

		if (peer->used && now - peer->heard >= (unsigned long)MM_EXPIRY_INTERVALS * peer->record.interval * 100)
			peer->used = 0;
	}
}

static struct mm_record base_record(struct mm_search *search, int kind, int playlist, int size, int interval)
{
	struct mm_record record;

	memset(&record, 0, sizeof(record));
	record.kind = kind;
	record.network_version = HALO_PORT_NETWORK_VERSION;
	record.sequence = ++search->sequence;
	record.interval = interval;
	record.can_host = search->can_host > 0;
	record.playlist = playlist;
	record.size = size;
	memcpy(record.key, search->key, P2P_KEY_SIZE);
	return record;
}

static int emit(const struct mm_search *search, const struct mm_record *record, struct mm_out *out)
{
	mm_topic(record->playlist, record->size, out->topic, sizeof(out->topic));
	out->size = mm_record_write(record, search->seed, out->bytes, sizeof(out->bytes));
	return out->size > 0;
}

static int cancel_except(struct mm_search *search, int keep_playlist, int keep_size, struct mm_out *out, int out_size)
{
	int interval = mm_interval(search->queue_count), index, kept = 0, count = 0;

	for (index = 0; index < search->queue_count; index++)
	{
		struct mm_record record;

		if (search->queues[index][0] == keep_playlist && search->queues[index][1] == keep_size)
		{
			search->queues[kept][0] = keep_playlist;
			search->queues[kept++][1] = keep_size;
		}
		else if (count < out_size)
		{
			record = base_record(search, MM_KIND_CANCEL, search->queues[index][0], search->queues[index][1],
				interval);
			count += emit(search, &record, &out[count]);
		}
	}
	search->queue_count = kept;
	search->next_queue = 0;
	return count;
}

int mm_cancel(struct mm_search *search, struct mm_out *out, int out_size)
{
	int count = cancel_except(search, -1, -1, out, out_size);

	search->state = P2P_MM_OFF;
	return count;
}

static int make_units(const struct mm_search *search, int playlist, int size, struct mm_unit *units)
{
	int count = 1, index;

	memcpy(units[0].key, search->key, P2P_KEY_SIZE);
	memcpy(units[0].key_hash, search->key_hash, P2P_KEY_HASH_SIZE);
	units[0].can_host = search->can_host > 0;
	units[0].failed = 0;
	for (index = 0; index < MM_MAXIMUM_UNITS; index++)
	{
		const struct mm_peer *peer = &search->peers[index];

		if (!peer->used || peer->playlist != playlist || peer->size != size ||
			peer->record.kind != MM_KIND_SEARCHING || !mm_counted(peer))
		{
			continue;
		}
		memcpy(units[count].key, peer->record.key, P2P_KEY_SIZE);
		memcpy(units[count].key_hash, peer->record.key_hash, P2P_KEY_HASH_SIZE);
		units[count].can_host = peer->record.can_host;
		units[count].failed = peer->failed != MM_FAILED_NONE;
		count++;
	}
	return count;
}

static void await_host(struct mm_search *search, const struct mm_unit *host, unsigned long now)
{
	struct mm_peer *peer;

	if (memcmp(search->joining, host->key_hash, P2P_KEY_HASH_SIZE))
	{
		memcpy(search->joining, host->key_hash, P2P_KEY_HASH_SIZE);
		search->joining_since = now;
	}
	else if (now - search->joining_since >= MM_HOST_WAIT)
	{
		peer = find_peer(search, host->key_hash, search->pool_playlist, search->pool_size);
		if (peer)
			peer->failed = MM_FAILED_PASSED_OVER;
	}
}

static int settle(struct mm_search *search, unsigned long now, struct mm_out *out, int out_size)
{
	struct mm_unit units[MM_MAXIMUM_UNITS + 1];
	int pool[MM_MAXIMUM_POOL];
	int queue, member, taken = 0, host, same;

	for (queue = 0; queue < search->queue_count && !taken; queue++)
	{
		taken = mm_pool(units, make_units(search, search->queues[queue][0], search->queues[queue][1], units),
			p2p_mm_size_players(search->queues[queue][1]), pool);
		for (member = 0; member < taken && pool[member] != 0; member++)
			;
		if (member == taken)
			taken = 0;
	}
	if (!taken)
	{
		search->pool_count = 0;
		return 0;
	}
	queue--;
	same = search->pool_count == taken && search->pool_playlist == search->queues[queue][0] &&
		search->pool_size == search->queues[queue][1];
	for (member = 0; member < taken; member++)
		same = same && !memcmp(search->pool[member], units[pool[member]].key_hash, P2P_KEY_HASH_SIZE);
	if (!same)
	{
		search->pool_count = taken;
		search->pool_playlist = search->queues[queue][0];
		search->pool_size = search->queues[queue][1];
		for (member = 0; member < taken; member++)
			memcpy(search->pool[member], units[pool[member]].key_hash, P2P_KEY_HASH_SIZE);
		search->pool_since = now;
		memset(search->joining, 0, sizeof(search->joining));
		return 0;
	}
	if (now - search->pool_since >= MM_SETTLE)
	{
		host = mm_elect(units, pool, taken);
		if (host == 0)
		{
			search->decision.kind = P2P_MM_DECIDE_HOST;
			search->decision.playlist = search->hosting_playlist = search->pool_playlist;
			search->decision.size = search->hosting_size = search->pool_size;
			search->state = P2P_MM_FOUND;
			search->decided = P2P_MM_DECIDE_HOST;
			return cancel_except(search, search->pool_playlist, search->pool_size, out, out_size);
		}
		if (host > 0)
			await_host(search, &units[host], now);
	}
	return 0;
}

static int pool_index(const struct mm_record *record, const unsigned char *key_hash)
{
	int index;

	for (index = 0; index < record->pool_count; index++)
		if (!memcmp(record->pool[index], key_hash, P2P_KEY_HASH_SIZE))
			return index;
	return -1;
}

/* at equal players only the higher key hash moves, so two lobbies never swap */
static int outranks(const struct mm_search *search, const struct mm_record *other)
{
	return other->players + search->players <= other->most_players &&
		(other->players > search->players ||
			(other->players == search->players &&
				memcmp(search->key_hash, other->key_hash, P2P_KEY_HASH_SIZE) > 0));
}

static int pick_game(struct mm_search *search, unsigned long now, struct mm_out *out, int out_size)
{
	static struct p2p_listing games[MAXIMUM_GAMES];
	struct p2p_lobby_wanted wanted;
	int looking = search->state == P2P_MM_LOOKING, count, index, waiting, playlist;

	count = lobby_games_unlocked(games, MAXIMUM_GAMES);
	search->games_checked = count;
	mm_wanted(search, &wanted);
	waiting = p2p_lobby_pick_unpinged(games, count, &wanted);
	index = p2p_lobby_pick(games, count, &wanted);
	if (index < 0 || (looking && waiting && now - search->marks_from < MM_LOOK_TIME))
	{
		if (looking && index < 0 && now - search->marks_from >= MM_LOOK_TIME)
		{
			search->state = P2P_MM_QUEUED;
			search->next_record = now;
		}
		return 0;
	}
	for (playlist = 0; playlist < NUMBER_OF_P2P_MM_PLAYLISTS; playlist++)
	{
		if ((search->choice.playlists & 1u << playlist) &&
			mm_playlist_has_size(1u << playlist, search->choice.size) &&
			engines[playlist] == games[index].engine_type)
		{
			break;
		}
	}
	search->decision.kind = P2P_MM_DECIDE_JOIN;
	search->decision.playlist = playlist;
	search->decision.size = search->choice.size;
	search->decision.public_game = 1;
	snprintf(search->decision.invite, sizeof(search->decision.invite), "%s", games[index].invite);
	memset(search->joining, 0, sizeof(search->joining));
	search->joining_since = now;
	search->state = P2P_MM_FOUND;
	search->decided = P2P_MM_DECIDE_JOIN;
	if (looking)
	{
		search->queue_count = 0;
		return 0;
	}
	return cancel_except(search, -1, -1, out, out_size);
}

static int join(struct mm_search *search, unsigned long now, struct mm_out *out, int out_size)
{
	struct mm_peer *best = NULL;
	unsigned char bytes[P2P_KEY_HASH_SIZE + P2P_TOKEN_SIZE];
	char text[2 * sizeof(bytes) + 1];
	int index, member;

	for (index = 0; index < MM_MAXIMUM_UNITS; index++)
	{
		struct mm_peer *peer = &search->peers[index];

		if (!peer->used || peer->failed || peer->record.kind != MM_KIND_HOSTING ||
			!in_queue(search, peer->playlist, peer->size) ||
			peer->record.players >= peer->record.most_players ||
			(peer->record.held && pool_index(&peer->record, search->key_hash) < 0) ||
			(search->state == P2P_MM_HOSTING && !outranks(search, &peer->record)))
		{
			continue;
		}
		if (!best || peer->record.players > best->record.players ||
			(peer->record.players == best->record.players &&
				memcmp(peer->record.key_hash, best->record.key_hash, P2P_KEY_HASH_SIZE) < 0))
		{
			best = peer;
		}
	}
	if (!best)
		return 0;
	memcpy(bytes, best->record.key_hash, P2P_KEY_HASH_SIZE);
	if (best->record.held)
	{
		member = pool_index(&best->record, search->key_hash);
		if (!mm_open_token(search->seed, best->record.key, best->record.sealed[member], bytes + P2P_KEY_HASH_SIZE))
		{
			best->failed = MM_FAILED_GAME_UNJOINABLE;
			return 0;
		}
	}
	else
	{
		memcpy(bytes + P2P_KEY_HASH_SIZE, best->record.token, P2P_TOKEN_SIZE);
	}
	p2p_hex(bytes, sizeof(bytes), text);
	search->decision.kind = P2P_MM_DECIDE_JOIN;
	search->decision.playlist = best->playlist;
	search->decision.size = best->size;
	search->decision.public_game = 0;
	snprintf(search->decision.invite, sizeof(search->decision.invite), "halo://join/%s", text);
	memcpy(search->joining, best->record.key_hash, P2P_KEY_HASH_SIZE);
	search->joining_since = now;
	search->state = P2P_MM_FOUND;
	search->decided = P2P_MM_DECIDE_JOIN;
	return cancel_except(search, best->playlist, best->size, out, out_size);
}

void mm_joined(struct mm_search *search, int ok, unsigned long now)
{
	struct mm_peer *host;

	if (search->state != P2P_MM_FOUND)
		return;
	if (ok)
	{
		search->state = P2P_MM_OFF;
		return;
	}
	host = find_peer(search, search->joining, search->queues[0][0], search->queues[0][1]);
	if (host)
		host->failed = MM_FAILED_GAME_UNJOINABLE;
	set_queues(search);
	search->state = P2P_MM_QUEUED;
	search->marks_from = now;
}

void mm_hosted(struct mm_search *search, const unsigned char *token, int players, int most_players, int under_way,
	unsigned long now)
{
	if (search->state == P2P_MM_FOUND)
	{
		if (search->decided != P2P_MM_DECIDE_HOST)
			return;
		search->state = P2P_MM_HOSTING;
		search->held_until = now + MM_HOLD;
		search->next_hosting_record = now;
		memcpy(search->token, token, P2P_TOKEN_SIZE);
	}
	else if (search->state != P2P_MM_HOSTING)
	{
		return;
	}
	else if (players != search->players || under_way != search->under_way)
	{
		search->next_hosting_record = now;
	}
	search->players = players;
	search->most_players = most_players;
	search->under_way = under_way;
}

static int hosting_record(struct mm_search *search, unsigned long now, struct mm_out *out)
{
	int held = (long)(now - search->held_until) < 0, index;
	struct mm_record record = base_record(search, MM_KIND_HOSTING, search->hosting_playlist, search->hosting_size,
		search->under_way ? MM_RUNNING_INTERVAL / 100 : MM_INTERVAL);

	record.under_way = search->under_way;
	record.held = held;
	record.players = search->players;
	record.most_players = search->most_players;
	memcpy(record.token, search->token, P2P_TOKEN_SIZE);
	for (index = 0; index < search->pool_count; index++)
	{
		struct mm_peer *member = find_peer(search, search->pool[index], search->hosting_playlist, search->hosting_size);

		if (!member || (held && !mm_seal_token(search->seed, member->record.key, search->token,
			record.sealed[record.pool_count])))
		{
			continue;
		}
		memcpy(record.pool[record.pool_count++], search->pool[index], P2P_KEY_HASH_SIZE);
	}
	search->next_hosting_record = now + (search->under_way ? MM_RUNNING_INTERVAL : MM_INTERVAL * 100);
	return emit(search, &record, out);
}

static int hand_over(struct mm_search *search, unsigned long now, struct mm_out *out, int out_size)
{
	int count = join(search, now, out, out_size);

	if (search->state == P2P_MM_FOUND && count < out_size)
	{
		search->most_players = search->players;
		count += hosting_record(search, now, &out[count]);
		count += cancel_except(search, -1, -1, &out[count], out_size - count);
	}
	return count;
}

int mm_step(struct mm_search *search, unsigned long now, struct mm_out *out, int out_size)
{
	int count = 0, looked = 0;

	if (search->state == P2P_MM_OFF)
		return cancel_except(search, -1, -1, out, out_size);
	expire(search, now);
	if (search->state == P2P_MM_HOSTING)
	{
		if (!search->under_way)
			count = hand_over(search, now, out, out_size);
		if (search->state == P2P_MM_HOSTING && (long)(now - search->next_hosting_record) >= 0 && out_size > 0)
			count = hosting_record(search, now, out);
		return count;
	}
	if (search->state == P2P_MM_LOOKING)
	{
		count = pick_game(search, now, out, out_size);
		if (search->state == P2P_MM_LOOKING)
			return count;
		looked = 1;
	}
	if (search->state == P2P_MM_QUEUED)
	{
		if (!looked && search->decision.kind == P2P_MM_DECIDE_NONE)
			count = pick_game(search, now, out, out_size);
		if (search->decision.kind == P2P_MM_DECIDE_NONE)
			count = join(search, now, out, out_size);
		if (search->decision.kind == P2P_MM_DECIDE_NONE)
			count = settle(search, now, out, out_size);
	}
	if (search->queue_count && (long)(now - search->next_record) >= 0 && count < out_size)
	{
		int interval = mm_interval(search->queue_count);
		struct mm_record record = base_record(search, MM_KIND_SEARCHING, search->queues[search->next_queue][0],
			search->queues[search->next_queue][1], interval);

		search->next_queue = (search->next_queue + 1) % search->queue_count;
		search->next_record = now + interval * 100 / search->queue_count;
		count += emit(search, &record, &out[count]);
	}
	return count;
}

/* ---------- the p2p thread's search: mm is guarded by p2p_lock */

enum
{
	MAXIMUM_QUEUED = 64,
	/* milliseconds of signature checks a pass */
	VERIFY_BUDGET = 2,
};

struct queued
{
	unsigned char bytes[MM_MAXIMUM_RECORD_SIZE];
	int size;
};

static struct
{
	struct mm_search search;
	struct queued queue[MAXIMUM_QUEUED];
	int queue_count;
	struct p2p_mm_decision pending;
	int subscribed[P2P_MM_MAXIMUM_TOPICS][2], subscribed_count;
	int in_game;
	int has_token;
	unsigned char token[P2P_TOKEN_SIZE];
	int notice;
} mm;

static unsigned long monotonic_us(void)
{
	struct timespec time_now;

	clock_gettime(CLOCK_MONOTONIC, &time_now);
	return (unsigned long)time_now.tv_sec * 1000000UL + (unsigned long)time_now.tv_nsec / 1000;
}

static unsigned long elapsed_ms(unsigned long started)
{
	return (monotonic_us() - started) / 1000;
}

static int record_stale(const struct queued *queued)
{
	int index;

	if (queued->size < MM_HEADER_SIZE || (unsigned)(queued->bytes[3] << 8 | queued->bytes[4]) != HALO_PORT_NETWORK_VERSION)
		return 1;
	for (index = 0; index < MM_MAXIMUM_UNITS; index++)
	{
		const struct mm_peer *peer = &mm.search.peers[index];

		if (peer->used && peer->playlist == queued->bytes[12] && peer->size == queued->bytes[13] &&
			!memcmp(peer->record.key, queued->bytes + 14, P2P_KEY_SIZE))
		{
			return get_long(queued->bytes + 6) <= peer->record.sequence;
		}
	}
	return 0;
}

static void publish_out(const struct mm_out *out, int count)
{
	int index;

	for (index = 0; index < count; index++)
		p2p_signal_mm_publish(out[index].topic, out[index].bytes, out[index].size);
}

static int notice_after(int notice, int state)
{
	return state == P2P_MM_LOOKING || state == P2P_MM_QUEUED ? notice : P2P_MM_NOTICE_NONE;
}

void p2p_mm_heard(const unsigned char *payload, int size)
{
	if (mm.search.state == P2P_MM_OFF || mm.queue_count == MAXIMUM_QUEUED || size <= 0 ||
		size > MM_MAXIMUM_RECORD_SIZE)
	{
		return;
	}
	memcpy(mm.queue[mm.queue_count].bytes, payload, (size_t)size);
	mm.queue[mm.queue_count++].size = size;
}

int p2p_mm_verify_pass(unsigned long now)
{
	unsigned long started = monotonic_us();

	for (;;)
	{
		struct queued queued;
		struct mm_record record;
		int good;

		if (!mm.queue_count)
			break;
		queued = mm.queue[0];
		memmove(mm.queue, mm.queue + 1, sizeof(*mm.queue) * (size_t)(--mm.queue_count));
		if (record_stale(&queued))
			continue;
		pthread_mutex_unlock(&p2p_lock);
		good = mm_record_read(queued.bytes, queued.size, &record);
		pthread_mutex_lock(&p2p_lock);
		if (good)
			mm_heard(&mm.search, &record, now);
		if (elapsed_ms(started) >= VERIFY_BUDGET)
			break;
	}
	return mm.queue_count;
}

static void sync_topics(void)
{
	int count = mm.search.state == P2P_MM_OFF ? 0 : mm.search.queue_count;
	char topics[P2P_MM_MAXIMUM_TOPICS][P2P_MM_TOPIC_SIZE];
	int index;

	if (count == mm.subscribed_count && !memcmp(mm.subscribed, mm.search.queues, sizeof(int) * 2 * (size_t)count))
		return;
	for (index = 0; index < count; index++)
		mm_topic(mm.search.queues[index][0], mm.search.queues[index][1], topics[index], P2P_MM_TOPIC_SIZE);
	memcpy(mm.subscribed, mm.search.queues, sizeof(int) * 2 * (size_t)count);
	mm.subscribed_count = count;
	p2p_signal_mm_topics(topics, count);
}

int p2p_mm_active(void)
{
	return mm.search.state != P2P_MM_OFF;
}

void p2p_mm_update(unsigned long now, int can_host, const unsigned char *token)
{
	struct mm_out out[MM_MAXIMUM_OUT];
	int count;

	if (mm.search.state == P2P_MM_OFF)
	{
		/* still stepped while joined, so its queue's cancel goes out */
		publish_out(out, mm_step(&mm.search, now, out, MM_MAXIMUM_OUT));
		sync_topics();
		return;
	}
	mm.search.can_host = can_host;
	mm.has_token = token != NULL;
	if (token)
		memcpy(mm.token, token, P2P_TOKEN_SIZE);
	p2p_mm_verify_pass(now);
	/* the pass lets go of p2p_lock: the game's thread may have restarted the search after now */
	count = mm_step(&mm.search, p2p_now(), out, MM_MAXIMUM_OUT);
	publish_out(out, count);
	mm.notice = notice_after(mm.notice, mm.search.state);
	if (mm.search.decision.kind != P2P_MM_DECIDE_NONE)
	{
		mm.pending = mm.search.decision;
		mm.search.decision.kind = P2P_MM_DECIDE_NONE;
	}
	sync_topics();
}

static void cancel_search(void)
{
	struct mm_out out[MM_MAXIMUM_OUT];

	publish_out(out, mm_cancel(&mm.search, out, MM_MAXIMUM_OUT));
	mm.in_game = 0;
	mm.pending.kind = P2P_MM_DECIDE_NONE;
	mm.queue_count = 0;
}

void p2p_mm_search(const struct p2p_mm_choice *choice)
{
	pthread_mutex_lock(&p2p_lock);
	if (mm.search.state != P2P_MM_OFF)
		cancel_search();
	mm.in_game = 0;
	mm_start(&mm.search, p2p_signing_seed(), choice, p2p_now(), (unsigned long)time(NULL));
	mm.notice = P2P_MM_NOTICE_NONE;
	pthread_mutex_unlock(&p2p_lock);
}

void p2p_mm_cancel(void)
{
	pthread_mutex_lock(&p2p_lock);
	if (mm.search.state != P2P_MM_OFF)
		cancel_search();
	mm.in_game = 0;
	mm.notice = P2P_MM_NOTICE_NONE;
	pthread_mutex_unlock(&p2p_lock);
}

void p2p_mm_game(int players, int most_players, int under_way)
{
	pthread_mutex_lock(&p2p_lock);
	if (mm.search.state != P2P_MM_OFF && mm.search.decided == P2P_MM_DECIDE_HOST)
	{
		if (mm.has_token)
			mm_hosted(&mm.search, mm.token, players, most_players, under_way, p2p_now());
	}
	pthread_mutex_unlock(&p2p_lock);
}

void p2p_mm_joined(int ok)
{
	pthread_mutex_lock(&p2p_lock);
	if (mm.search.state == P2P_MM_FOUND && mm.search.decided == P2P_MM_DECIDE_JOIN)
	{
		mm_joined(&mm.search, ok, p2p_now());
		mm.in_game = ok;
		if (!ok)
			mm.notice = P2P_MM_NOTICE_HOST_UNREACHED;
	}
	pthread_mutex_unlock(&p2p_lock);
}

void p2p_mm_game_lost(void)
{
	pthread_mutex_lock(&p2p_lock);
	if (mm.search.state != P2P_MM_OFF || mm.in_game)
	{
		mm_game_lost(&mm.search, p2p_now());
		mm.in_game = 0;
		mm.notice = P2P_MM_NOTICE_HOST_LEFT;
	}
	pthread_mutex_unlock(&p2p_lock);
}

int p2p_mm_take_decision(struct p2p_mm_decision *decision)
{
	int kind;

	pthread_mutex_lock(&p2p_lock);
	*decision = mm.pending;
	kind = mm.pending.kind;
	mm.pending.kind = P2P_MM_DECIDE_NONE;
	pthread_mutex_unlock(&p2p_lock);
	return kind;
}

static int searchers_counted(int playlist, int size)
{
	int index, other, count = 1;

	for (index = 0; index < MM_MAXIMUM_UNITS; index++)
	{
		const struct mm_peer *peer = &mm.search.peers[index];
		int seen = 0;

		if (!peer->used || peer->record.kind != MM_KIND_SEARCHING || !mm_counted(peer) ||
			peer->playlist != playlist || peer->size != size)
		{
			continue;
		}
		for (other = 0; other < index && !seen; other++)
			seen = mm.search.peers[other].used && mm.search.peers[other].record.kind == MM_KIND_SEARCHING &&
				mm_counted(&mm.search.peers[other]) &&
				mm.search.peers[other].playlist == playlist && mm.search.peers[other].size == size &&
				!memcmp(mm.search.peers[other].record.key, peer->record.key, P2P_KEY_SIZE);
		count += !seen;
	}
	return count;
}

void p2p_mm_get_status(struct p2p_mm_status *status)
{
	const struct mm_search *search = &mm.search;
	int index;

	memset(status, 0, sizeof(*status));
	pthread_mutex_lock(&p2p_lock);
	status->state = search->state;
	if (search->state != P2P_MM_OFF)
	{
		status->games_checked = search->games_checked;
		status->can_host = search->can_host;
		for (index = 0; index < search->queue_count; index++)
		{
			int count = searchers_counted(search->queues[index][0], search->queues[index][1]);

			if (count > status->count)
			{
				status->count = count;
				status->playlist = search->queues[index][0];
				status->size = search->queues[index][1];
				status->needed = p2p_mm_size_players(status->size);
			}
		}
		status->notice = mm.notice = notice_after(mm.notice, search->state);
	}
	pthread_mutex_unlock(&p2p_lock);
}
