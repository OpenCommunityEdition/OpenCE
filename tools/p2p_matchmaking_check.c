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

static void check_playlists(void)
{
	case_name = "playlists";
	CHECK(!mm_playlist_has_size(1u << P2P_MM_CTF, P2P_MM_FFA_6), "CTF has no free-for-all");
	CHECK(mm_playlist_has_size(1u << P2P_MM_CTF, P2P_MM_4V4) && mm_playlist_has_size(1u << P2P_MM_SLAYER, P2P_MM_FFA_6),
		"every other pair is a queue");
}

int main(void)
{
	check_playlists();
	printf(failures ? "FAILED %d\n" : "PASS\n", failures);
	return failures != 0;
}
