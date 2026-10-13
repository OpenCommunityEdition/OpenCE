/*
MAP_SHA1.H

SHA-1 (FIPS 180-4), which a map's BitTorrent info hash is made with: the
index of the maps (port/assets/network/map_index.txt) names each map by it,
and a map downloaded is checked against it (map_archives.c). Not a
cryptographic use: the hashes are checked against those the index's maker
computed, and the index ships with the game.
*/

#ifndef __HALO_LINUX_MAP_SHA1_H
#define __HALO_LINUX_MAP_SHA1_H

#define SHA1_DIGEST_SIZE 20

struct sha1_state
{
	unsigned long state[5];
	unsigned long long length;
	unsigned char buffer[64];
	int buffered;
};

void sha1_start(struct sha1_state *state);
void sha1_update(struct sha1_state *state, const void *bytes, unsigned long size);
void sha1_finish(struct sha1_state *state, unsigned char digest[SHA1_DIGEST_SIZE]);
/* the digest of one buffer */
void sha1_bytes(const void *bytes, unsigned long size, unsigned char digest[SHA1_DIGEST_SIZE]);

#endif
