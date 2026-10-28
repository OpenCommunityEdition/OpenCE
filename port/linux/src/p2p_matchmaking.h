/*
P2P_MATCHMAKING.H

MATCHMAKING's signed records, queues and search (p2p_matchmaking.c). A
search's state lives in struct mm_search, never in globals:
tools/p2p_matchmaking_check.c runs several machines in one program.
*/

#ifndef __HALO_LINUX_P2P_MATCHMAKING_H
#define __HALO_LINUX_P2P_MATCHMAKING_H

#include "p2p_internal.h"
#include "p2p.h"

enum
{
	MM_FORMAT = 1,
	MM_KIND_SEARCHING = 1,
	MM_KIND_HOSTING,
	MM_KIND_CANCEL,
	MM_FLAG_CAN_HOST = 1,
	MM_FLAG_UNDER_WAY = 2,
	MM_FLAG_HELD = 4,
	MM_MAXIMUM_POOL = 8,
	MM_MAXIMUM_UNITS = 256,
	/* a searcher counts once heard on this many records */
	MM_REFRESHES_COUNTED = 2,
	MM_SEALED_TOKEN_SIZE = P2P_TOKEN_SIZE + P2P_SEAL_OVERHEAD,
	MM_HEADER_SIZE = 2 + 1 + 2 + 1 + 4 + 1 + 1 + 1 + 1 + P2P_KEY_SIZE,
	MM_MAXIMUM_RECORD_SIZE = MM_HEADER_SIZE + 3 + MM_MAXIMUM_POOL * (P2P_KEY_HASH_SIZE + MM_SEALED_TOKEN_SIZE) +
		P2P_TOKEN_SIZE + P2P_SIGNATURE_SIZE,
	/* tenths of a second */
	MM_INTERVAL = 30,
	MM_MINIMUM_INTERVAL = 10,
};

struct mm_record
{
	int kind;
	unsigned short network_version;
	/* rises with each record of a run's key */
	unsigned long sequence;
	/* tenths of a second until the next */
	int interval;
	int can_host, under_way, held;
	int playlist, size;
	unsigned char key[P2P_KEY_SIZE];
	/* the hash of the key's X25519 form: an invite's host part */
	unsigned char key_hash[P2P_KEY_HASH_SIZE];
	int players, most_players;
	int pool_count;
	unsigned char pool[MM_MAXIMUM_POOL][P2P_KEY_HASH_SIZE];
	unsigned char sealed[MM_MAXIMUM_POOL][MM_SEALED_TOKEN_SIZE];
	unsigned char token[P2P_TOKEN_SIZE];
};

int mm_record_write(const struct mm_record *record, const unsigned char *seed, unsigned char *bytes, int size);
/* checks the signature: 1 only if whole and signed by its key */
int mm_record_read(const unsigned char *bytes, int size, struct mm_record *record);
int mm_seal_token(const unsigned char *seed, const unsigned char *member_key, const unsigned char *token,
	unsigned char *sealed);
int mm_open_token(const unsigned char *seed, const unsigned char *host_key, const unsigned char *sealed,
	unsigned char *token);

struct mm_unit
{
	unsigned char key[P2P_KEY_SIZE];
	unsigned char key_hash[P2P_KEY_HASH_SIZE];
	int players;
	int can_host;
	int failed;
};

int mm_pool(const struct mm_unit *units, int count, int players, int *pool);
int mm_elect(const struct mm_unit *units, const int *pool, int pool_count);

#endif
