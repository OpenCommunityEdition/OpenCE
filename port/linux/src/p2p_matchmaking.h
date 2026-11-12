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
	/* records after the first */
	MM_REFRESHES_COUNTED = 2,
	MM_SEALED_TOKEN_SIZE = P2P_TOKEN_SIZE + P2P_SEAL_OVERHEAD,
	MM_HEADER_SIZE = 2 + 1 + 2 + 1 + 4 + 1 + 1 + 1 + 1 + P2P_KEY_SIZE,
	MM_MAXIMUM_RECORD_SIZE = MM_HEADER_SIZE + 3 + MM_MAXIMUM_POOL * (P2P_KEY_HASH_SIZE + MM_SEALED_TOKEN_SIZE) +
		P2P_TOKEN_SIZE + P2P_SIGNATURE_SIZE,
	/* tenths of a second */
	MM_INTERVAL = 30,
	MM_MINIMUM_INTERVAL = 10,
	MM_RECORDS_PER_SECOND = 4,
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

void mm_topic(int playlist, int size, char *topic, int topic_size);
/* tenths of a second */
int mm_interval(int queues);

int mm_topic_listed(const char *const *topics, int count, const char *topic);

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

enum
{
	/* milliseconds */
	MM_SETTLE = 4000,
	MM_HOLD = 20000,
	MM_EXPIRY_INTERVALS = 3,
	MM_RUNNING_INTERVAL = 25000,
	MM_LOOK_TIME = 6000,
	MM_MAXIMUM_OUT = 32,
};

enum
{
	MM_FAILED_NONE,
	MM_FAILED_GAME_UNJOINABLE
};

struct mm_peer
{
	int used;
	int playlist, size;
	struct mm_record record;
	/* this machine's milliseconds, not the sender's clock */
	unsigned long heard;
	int refreshes;
	int failed;
};

struct mm_out
{
	char topic[P2P_MM_TOPIC_SIZE];
	unsigned char bytes[MM_MAXIMUM_RECORD_SIZE];
	int size;
};

struct mm_search
{
	unsigned char seed[P2P_SEED_SIZE], key[P2P_KEY_SIZE], key_hash[P2P_KEY_HASH_SIZE];
	int state;
	struct p2p_mm_choice choice;
	/* Unix seconds, never sent */
	unsigned long started;
	unsigned long marks_from;
	/* 1, 0 (strict router) or -1 (STUN has not answered yet) */
	int can_host;
	unsigned long sequence;
	int queue_count;
	int queues[P2P_MM_MAXIMUM_TOPICS][2];
	int next_queue;
	unsigned long next_record;
	struct mm_peer peers[MM_MAXIMUM_UNITS];
	unsigned char pool[MM_MAXIMUM_POOL][P2P_KEY_HASH_SIZE];
	int pool_count, pool_playlist, pool_size;
	unsigned long pool_since;
	int hosting_playlist, hosting_size, players, most_players, under_way;
	unsigned char token[P2P_TOKEN_SIZE];
	unsigned long held_until, next_hosting_record;
	unsigned char joining[P2P_KEY_HASH_SIZE];
	unsigned long joining_since;
	struct p2p_mm_decision decision;
	int decided;
};

int mm_counted(const struct mm_peer *peer);

void mm_start(struct mm_search *search, const unsigned char *seed, const struct p2p_mm_choice *choice,
	unsigned long now, unsigned long unix_now);
/* the record must already be verified (mm_record_read) */
void mm_heard(struct mm_search *search, const struct mm_record *record, unsigned long now);
int mm_step(struct mm_search *search, unsigned long now, struct mm_out *out, int out_size);
void mm_hosted(struct mm_search *search, const unsigned char *token, int players, int most_players, int under_way,
	unsigned long now);
int mm_cancel(struct mm_search *search, struct mm_out *out, int out_size);

#endif
