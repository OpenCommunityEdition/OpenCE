/*
P2P_MATCHMAKING_CHECK.C

Matchmaking's records, and machines searching through a stand-in broker.
*/

#include "../port/linux/src/p2p_matchmaking.h"
#include "halo_port_limits.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_OF_(x) #x
#define TEXT_OF(x) TEXT_OF_(x)
#define HALO_PORT_NETWORK_VERSION_TEXT TEXT_OF(HALO_PORT_NETWORK_VERSION)

pthread_mutex_t p2p_lock = PTHREAD_MUTEX_INITIALIZER;

static unsigned long clock_now = 1800000000UL;

void posix_random_bytes(void *buffer, unsigned long size) { memset(buffer, 0x5a, size); }
unsigned long p2p_now(void) { return clock_now; }
int config_boolean(const char *name) { (void)name; return 1; }
void platform_log(const char *format, ...) { (void)format; }

void p2p_key_hash(const unsigned char *key, unsigned char *hash)
{
	unsigned char digest[P2P_SHA256_SIZE];

	p2p_sha256(key, P2P_KEY_SIZE, digest);
	memcpy(hash, digest, P2P_KEY_HASH_SIZE);
}

static int failures;

#define CHECK(condition, ...) do { if (!(condition)) { failures++; printf("FAIL %s: ", case_name); \
	printf(__VA_ARGS__); printf("\n"); } } while (0)

static const char *case_name;

static void seed_of(int machine, unsigned char *seed)
{
	memset(seed, 0, P2P_SEED_SIZE);
	seed[0] = (unsigned char)(machine + 1);
	seed[1] = (unsigned char)((machine + 1) >> 8);
}

static struct mm_record searching(int machine, unsigned long sequence)
{
	struct mm_record record;
	unsigned char seed[P2P_SEED_SIZE];

	memset(&record, 0, sizeof(record));
	seed_of(machine, seed);
	p2p_ed25519_public(seed, record.key, NULL);
	record.kind = MM_KIND_SEARCHING;
	record.network_version = HALO_PORT_NETWORK_VERSION;
	record.sequence = sequence;
	record.interval = MM_INTERVAL;
	record.can_host = 1;
	record.playlist = P2P_MM_SLAYER;
	record.size = P2P_MM_2V2;
	return record;
}

static void check_records(void)
{
	unsigned char seed[P2P_SEED_SIZE], bytes[MM_MAXIMUM_RECORD_SIZE], message[MM_MAXIMUM_RECORD_SIZE + 9];
	struct mm_record record = searching(0, 7), read_back;
	int size;

	case_name = "records";
	seed_of(0, seed);
	size = mm_record_write(&record, seed, bytes, sizeof(bytes));
	CHECK(size == MM_HEADER_SIZE + P2P_SIGNATURE_SIZE, "a searching record is %d bytes", size);
	CHECK(mm_record_read(bytes, size, &read_back), "read back");
	CHECK(read_back.sequence == 7 && read_back.can_host &&
		read_back.playlist == P2P_MM_SLAYER && read_back.size == P2P_MM_2V2 &&
		read_back.network_version == HALO_PORT_NETWORK_VERSION, "fields kept");
	bytes[30] ^= 1;
	CHECK(!mm_record_read(bytes, size, &read_back), "E14: a tampered record is refused");
	bytes[30] ^= 1;
	CHECK(!mm_record_read(bytes, size - 1, &read_back), "cut short");
	/* (signed again, so that only the format is wrong) */
	bytes[2] = MM_FORMAT + 1;
	memcpy(message, "hceu-mm-1", 9);
	memcpy(message + 9, bytes, (size_t)(size - P2P_SIGNATURE_SIZE));
	p2p_ed25519_sign(seed, record.key, message, 9 + size - P2P_SIGNATURE_SIZE, bytes + size - P2P_SIGNATURE_SIZE);
	CHECK(!mm_record_read(bytes, size, &read_back), "another format is refused");
	record = searching(1, 7);
	size = mm_record_write(&record, seed, bytes, sizeof(bytes));
	CHECK(!mm_record_read(bytes, size, &read_back), "signed by a key not its own");
}

static void check_playlists(void)
{
	case_name = "playlists";
	CHECK(!mm_playlist_has_size(1u << P2P_MM_CTF, P2P_MM_FFA_6), "CTF has no free-for-all");
	CHECK(mm_playlist_has_size(1u << P2P_MM_CTF, P2P_MM_4V4) && mm_playlist_has_size(1u << P2P_MM_SLAYER, P2P_MM_FFA_6),
		"every other pair is a queue");
}

static void check_topics(void)
{
	char topic[P2P_MM_TOPIC_SIZE];

	case_name = "topics";
	mm_topic(P2P_MM_SLAYER, P2P_MM_2V2, topic, sizeof(topic));
	CHECK(!strcmp(topic, "hceu/3/mm/" HALO_PORT_NETWORK_VERSION_TEXT "/slayer/2v2"), "E16: %s", topic);
	mm_topic(P2P_MM_RACE, P2P_MM_FFA_6, topic, sizeof(topic));
	CHECK(!strcmp(topic, "hceu/3/mm/" HALO_PORT_NETWORK_VERSION_TEXT "/race/ffa6"), "%s", topic);
	CHECK(mm_interval(1) == 30 && mm_interval(12) == 30, "up to 12 queues: 3 s");
	CHECK(mm_interval(25) >= 62 && mm_interval(25) <= 63, "E44: 25 queues: about 6.3 s: %d", mm_interval(25));
	CHECK(mm_interval(25) * MM_RECORDS_PER_SECOND >= 25 * 10, "E44: at most 4 a second");
}

static int contains(const unsigned char *bytes, int size, const unsigned char *part, int part_size)
{
	int at;

	for (at = 0; at + part_size <= size; at++)
		if (!memcmp(bytes + at, part, (size_t)part_size))
			return 1;
	return 0;
}

static void check_hosting(void)
{
	unsigned char host_seed[P2P_SEED_SIZE], member_seed[P2P_SEED_SIZE], other_seed[P2P_SEED_SIZE];
	unsigned char bytes[MM_MAXIMUM_RECORD_SIZE], token[P2P_TOKEN_SIZE], opened[P2P_TOKEN_SIZE];
	struct mm_record record = searching(0, 1), member = searching(1, 1), read_back;
	int size;

	case_name = "hosting";
	seed_of(0, host_seed);
	seed_of(1, member_seed);
	seed_of(2, other_seed);
	memset(token, 0xa5, sizeof(token));
	size = mm_record_write(&member, member_seed, bytes, sizeof(bytes));
	CHECK(mm_record_read(bytes, size, &member), "the member's own record");
	record.kind = MM_KIND_HOSTING;
	record.players = 1;
	record.most_players = 4;
	record.held = 1;
	record.pool_count = 1;
	memcpy(record.pool[0], member.key_hash, P2P_KEY_HASH_SIZE);
	memcpy(record.token, token, P2P_TOKEN_SIZE);
	CHECK(mm_seal_token(host_seed, member.key, token, record.sealed[0]), "sealed");
	size = mm_record_write(&record, host_seed, bytes, sizeof(bytes));
	CHECK(mm_record_read(bytes, size, &read_back), "a held hosting record read back");
	CHECK(read_back.held && read_back.pool_count == 1 && read_back.most_players == 4, "held, one member, 4 most");
	CHECK(mm_open_token(member_seed, read_back.key, read_back.sealed[0], opened) &&
		!memcmp(opened, token, P2P_TOKEN_SIZE), "the member opens the token");
	CHECK(!mm_open_token(other_seed, read_back.key, read_back.sealed[0], opened), "no one else opens it");
	CHECK(!contains(bytes, size, token, P2P_TOKEN_SIZE), "a held game's token is nowhere in clear");
}

static struct mm_unit unit(int machine, int can_host)
{
	struct mm_unit result;
	unsigned char seed[P2P_SEED_SIZE], x25519[P2P_KEY_SIZE];

	memset(&result, 0, sizeof(result));
	seed_of(machine, seed);
	p2p_ed25519_public(seed, result.key, NULL);
	p2p_ed25519_to_x25519(result.key, x25519);
	p2p_key_hash(x25519, result.key_hash);
	result.players = 1;
	result.can_host = can_host;
	return result;
}

static int hash_rank(const struct mm_unit *units, int count, int index)
{
	int other, rank = 0;

	for (other = 0; other < count; other++)
		rank += memcmp(units[other].key_hash, units[index].key_hash, P2P_KEY_HASH_SIZE) < 0;
	return rank;
}

static void check_pool(void)
{
	static struct mm_unit every[MM_MAXIMUM_UNITS + 1];
	struct mm_unit units[8], shuffled[8];
	int pool[8], again[8], count, index, host, host_again, last = 0;

	case_name = "pool";
	for (index = 0; index < 6; index++)
		units[index] = unit(index, 1);
	count = mm_pool(units, 6, 4, pool);
	CHECK(count == 4, "E4: a pool of 4 from 6: %d", count);
	for (index = 0; index < count; index++)
		CHECK(hash_rank(units, 6, pool[index]) == index, "E4: the 4 lowest key hashes, in order: %d at %d",
			pool[index], index);
	for (index = 0; index < 6; index++)
		shuffled[index] = units[5 - index];
	CHECK(mm_pool(shuffled, 6, 4, again) == 4, "shuffled: a pool");
	for (index = 0; index < 4; index++)
		CHECK(!memcmp(units[pool[index]].key, shuffled[again[index]].key, P2P_KEY_SIZE), "shuffled: the same pool at %d", index);
	host = mm_elect(units, pool, count);
	host_again = mm_elect(shuffled, again, 4);
	CHECK(host >= 0 && host_again >= 0 && !memcmp(units[host].key, shuffled[host_again].key, P2P_KEY_SIZE),
		"E4: the same host on every machine");
	CHECK(mm_pool(units, 3, 4, pool) == 0, "3 of 4: no pool");
	for (index = 0; index < MM_MAXIMUM_UNITS; index++)
		every[index].players = 2;
	every[MM_MAXIMUM_UNITS].players = every[MM_MAXIMUM_UNITS].can_host = 1;
	CHECK(mm_pool(every, MM_MAXIMUM_UNITS + 1, 1, pool) == 1 && pool[0] == MM_MAXIMUM_UNITS,
		"this machine and a full peers table: every unit is walked");
	/* four strict, then one that can host with the highest key hash: it can take only the last place */
	for (index = 0; index < 5; index++)
		units[index] = unit(index, 0);
	for (index = 0; index < 5; index++)
		if (hash_rank(units, 5, index) == 4)
			last = index;
	units[last].can_host = 1;
	CHECK(mm_pool(units, 5, 4, pool) == 4 && pool[3] == last && mm_elect(units, pool, 4) == last,
		"E38: the host takes the last place and hosts");
	units[last].can_host = 0;
	CHECK(mm_pool(units, 5, 4, pool) == 0, "E38: five strict routers form no pool");
	units[last].can_host = 1;
	units[last].failed = 1;
	CHECK(mm_pool(units, 5, 4, pool) == 0, "E12: the only host failed: no pool until another arrives");
}

static void check_election(void)
{
	struct mm_unit units[4];
	int pool[4] = { 0, 1, 2, 3 }, index, host, winners = 0;

	case_name = "election";
	for (index = 0; index < 3; index++)
		units[index] = unit(index, 1);
	for (index = 0; index < 40; index++)
	{
		units[3] = unit(100 + index, 1);
		host = mm_elect(units, pool, 4);
		if (host >= 0 && host < 3)
			winners |= 1 << host;
	}
	CHECK((winners & (winners - 1)) != 0, "E14: one of the three always wins when it can: %d", winners);
}

struct machine
{
	struct mm_search search;
	int quit;
};

static struct machine machines[8];
static int machine_count;
static unsigned long now_ms, now_unix;
static int drop_percent;

struct in_flight
{
	int to;
	unsigned long at;
	struct mm_out out;
};
static struct in_flight flight[1024];
static int flight_count;

int main(void)
{
	check_pool();
	check_election();
	check_records();
	check_hosting();
	check_playlists();
	check_topics();
	printf(failures ? "FAILED %d\n" : "PASS\n", failures);
	return failures != 0;
}
