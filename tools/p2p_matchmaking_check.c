/*
P2P_MATCHMAKING_CHECK.C

Matchmaking's records, and machines searching through a stand-in broker.
*/

#include "../port/linux/src/p2p_matchmaking.h"
#include "halo_port_limits.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(void)
{
	check_records();
	check_playlists();
	printf(failures ? "FAILED %d\n" : "PASS\n", failures);
	return failures != 0;
}
