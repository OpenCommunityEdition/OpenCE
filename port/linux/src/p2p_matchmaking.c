/*
P2P_MATCHMAKING.C

MATCHMAKING's signed records, queues and search.
*/

#include "p2p_matchmaking.h"
#include "halo_port_limits.h"

#include <string.h>

static const char record_label[] = "hceu-mm-1";

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
	int at = 0;

	if (size < MM_MAXIMUM_RECORD_SIZE)
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
	p2p_ed25519_sign(seed, record->key, message, signed_message(bytes, at, message), bytes + at);
	return at + P2P_SIGNATURE_SIZE;
}

int mm_record_read(const unsigned char *bytes, int size, struct mm_record *record)
{
	unsigned char message[sizeof(record_label) + MM_MAXIMUM_RECORD_SIZE];
	int at = MM_HEADER_SIZE;

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
	if (size != at + P2P_SIGNATURE_SIZE)
		return 0;
	if (!p2p_ed25519_verify(record->key, message, signed_message(bytes, at, message), bytes + at))
		return 0;
	return key_hash_of(record->key, record->key_hash);
}
