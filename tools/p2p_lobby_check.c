/*
P2P_LOBBY_CHECK.C

A check of internet play's signatures and the server browser's listings
(port/linux/src/p2p_crypto.c, port/linux/src/p2p_lobby.c), built with the
platform layer's flags by tools/test_linux_port.py: RFC 8032's Ed25519
vectors; the X25519 key a seed gives against its Ed25519 key's conversion;
bad signatures, other keys and small-order keys turned away; and a listing
from a host (this program, through stand-ins for p2p.c and p2p_signal.c) to
a browser (this program again): taken, and a tampered one, one on another's
slot, an older one, a retained one of long ago, a tombstone and a listing
older than it each handled as they must be. Prints PASS or the failures.
*/

#include "p2p_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- stand-ins for the rest of internet play */

pthread_mutex_t p2p_lock = PTHREAD_MUTEX_INITIALIZER;

static unsigned long clock_now = 1000;
static unsigned char seed[P2P_SEED_SIZE] = { 42 };
static unsigned char signing_key[P2P_KEY_SIZE], x25519_secret[P2P_KEY_SIZE], x25519_public[P2P_KEY_SIZE];
static unsigned char published[512];
static int published_size, published_closing, publish_count;
/* the game hosted (its invite's token; NULL: none) */
static const unsigned char *hosting_token;

void posix_random_bytes(void *buffer, unsigned long size) { memset(buffer, 0x5a, size); }
unsigned long p2p_now(void) { return clock_now; }
int config_boolean(const char *name) { (void)name; return 1; }
void platform_log(const char *format, ...) { (void)format; }

void p2p_hex(const unsigned char *bytes, int size, char *text)
{
	int index;

	for (index = 0; index < size; index++)
		sprintf(text + 2 * index, "%02x", bytes[index]);
	text[2 * size] = 0;
}

void p2p_key_hash(const unsigned char *key, unsigned char *hash)
{
	unsigned char digest[P2P_SHA256_SIZE];

	p2p_sha256(key, P2P_KEY_SIZE, digest);
	memcpy(hash, digest, P2P_KEY_HASH_SIZE);
}

void p2p_identifier_from_hash(const unsigned char *hash, unsigned char *identifier)
{
	memcpy(identifier, hash, P2P_IDENTIFIER_SIZE);
	identifier[0] = (unsigned char)((identifier[0] & 0xFC) | 0x02);
}

const unsigned char *p2p_public_key(void) { return x25519_public; }
const unsigned char *p2p_signing_key(void) { return signing_key; }
void p2p_sign(const void *message, int size, unsigned char *signature)
{
	p2p_ed25519_sign(seed, signing_key, message, size, signature);
}
void p2p_new_invite_if_listed(void) {}
void p2p_signal_lobby_topics(int listed, int browsing) { (void)listed; (void)browsing; }
void p2p_signal_lobby_query(void) {}
void p2p_signal_lobby_publish(const unsigned char *listing, int size, int closing)
{
	memcpy(published, listing, (size_t)size);
	published_size = size;
	published_closing = closing;
	publish_count++;
}
void p2p_signal_lobby_quit(void) {}

/* the p2p thread's calls: under p2p_lock, as there (p2p_lobby_update lets
go of it while it checks signatures) */
static void lobby_update(const unsigned char *token, int player_count, int maximum_player_count)
{
	pthread_mutex_lock(&p2p_lock);
	p2p_lobby_update(token, player_count, maximum_player_count);
	pthread_mutex_unlock(&p2p_lock);
}

/* ---------- the checks */

static int failures;

static void check(int good, const char *what)
{
	if (!good)
	{
		failures++;
		printf("FAIL: %s\n", what);
	}
}

static void unhex(const char *text, unsigned char *bytes)
{
	size_t index;

	for (index = 0; index < strlen(text) / 2; index++)
	{
		unsigned int byte;

		sscanf(text + 2 * index, "%2x", &byte);
		bytes[index] = (unsigned char)byte;
	}
}

static void rfc8032(const char *seed_hex, const char *public_hex, const char *message_hex, const char *signature_hex)
{
	unsigned char test_seed[32], expected_public[32], public_key[32], message[64], expected[64], signature[64];
	int size = (int)strlen(message_hex) / 2;

	unhex(seed_hex, test_seed);
	unhex(public_hex, expected_public);
	unhex(message_hex, message);
	unhex(signature_hex, expected);
	p2p_ed25519_public(test_seed, public_key, NULL);
	p2p_ed25519_sign(test_seed, public_key, message, size, signature);
	check(!memcmp(public_key, expected_public, 32), "RFC 8032 public key");
	check(!memcmp(signature, expected, 64), "RFC 8032 signature");
	check(p2p_ed25519_verify(public_key, message, size, signature), "RFC 8032 verification");
}

static void crypto_checks(void)
{
	unsigned char test_seed[32], ed[32], secret[32], derived[32], converted[32], signature[64], other_ed[32];
	unsigned char small[32] = { 1 };
	unsigned char forged[64] = { 1 };
	int index;

	rfc8032("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
		"d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a", "",
		"e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b");
	rfc8032("4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb",
		"3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c", "72",
		"92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00");
	/* the X25519 key from a seed is its Ed25519 key's conversion, and
	agrees on secrets */
	for (index = 0; index < 16; index++)
	{
		unsigned char other[32], other_public[32], first[32], second[32];

		memset(test_seed, index * 13 + 1, sizeof(test_seed));
		p2p_ed25519_public(test_seed, ed, secret);
		p2p_x25519(derived, secret, NULL);
		check(p2p_ed25519_to_x25519(ed, converted) && !memcmp(converted, derived, 32), "X25519 key of a seed");
		memset(other, index + 99, sizeof(other));
		p2p_x25519(other_public, other, NULL);
		p2p_x25519(first, secret, other_public);
		p2p_x25519(second, other, converted);
		check(!memcmp(first, second, 32), "secrets agree");
	}
	memset(test_seed, 3, sizeof(test_seed));
	p2p_ed25519_public(test_seed, ed, NULL);
	p2p_ed25519_sign(test_seed, ed, "listing", 7, signature);
	check(p2p_ed25519_verify(ed, "listing", 7, signature), "a good signature");
	check(!p2p_ed25519_verify(ed, "listinG", 7, signature), "another message");
	signature[5] ^= 1;
	check(!p2p_ed25519_verify(ed, "listing", 7, signature), "an altered R");
	signature[5] ^= 1;
	signature[40] ^= 1;
	check(!p2p_ed25519_verify(ed, "listing", 7, signature), "an altered S");
	signature[40] ^= 1;
	memset(test_seed, 4, sizeof(test_seed));
	p2p_ed25519_public(test_seed, other_ed, NULL);
	check(!p2p_ed25519_verify(other_ed, "listing", 7, signature), "another key");
	/* the identity, an order-8 point, and a signature anyone makes for the
	identity */
	check(!p2p_ed25519_to_x25519(small, converted), "the identity's conversion");
	unhex("c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac037a", small);
	check(!p2p_ed25519_to_x25519(small, converted), "an order-8 key's conversion");
	memset(small, 0, sizeof(small));
	small[0] = 1;
	check(!p2p_ed25519_verify(small, "anything", 8, forged), "a small-order key's signature");
}

static int games(struct p2p_listing *listing)
{
	struct p2p_listing all[4];
	int count = p2p_lobby_games(all, 4);

	if (count && listing)
		*listing = all[0];
	return count;
}

/* a payload heard on the host's slot (or another), then the browser's pass */
static void hear(const unsigned char *payload, int size, int retained, const char *slot)
{
	unsigned char hash[P2P_KEY_HASH_SIZE];
	char own[2 * P2P_KEY_HASH_SIZE + 1];

	p2p_key_hash(x25519_public, hash);
	p2p_hex(hash, P2P_KEY_HASH_SIZE, own);
	pthread_mutex_lock(&p2p_lock);
	p2p_lobby_slot_heard(slot ? slot : own, payload, size, retained);
	pthread_mutex_unlock(&p2p_lock);
	lobby_update(hosting_token, 3, 16);
}

static void lobby_checks(void)
{
	static const unsigned char token[P2P_TOKEN_SIZE] = { 7, 7, 7 };
	unsigned char first[512], tampered[512];
	int first_size, count;
	struct p2p_listing listing;
	char expected_invite[P2P_LINK_SIZE];

	p2p_ed25519_public(seed, signing_key, x25519_secret);
	p2p_x25519(x25519_public, x25519_secret, NULL);
	hosting_token = token;
	/* hosting a public game: listed */
	p2p_set_hosting_public(1);
	p2p_set_game_listing("Test game", "bloodgulch", "Slayer", 2, 1, 0, 1);
	lobby_update(token, 3, 16);
	check(publish_count == 1 && !published_closing && p2p_lobby_listed(), "the host publishes its listing");
	memcpy(first, published, (size_t)published_size);
	first_size = published_size;
	/* the browser takes it */
	p2p_lobby_browse(1);
	hear(first, first_size, 0, NULL);
	count = games(&listing);
	check(count == 1, "the browser takes the listing");
	{
		unsigned char bytes[P2P_KEY_HASH_SIZE + P2P_TOKEN_SIZE];
		char text[2 * (P2P_KEY_HASH_SIZE + P2P_TOKEN_SIZE) + 1];

		p2p_key_hash(x25519_public, bytes);
		memcpy(bytes + P2P_KEY_HASH_SIZE, token, P2P_TOKEN_SIZE);
		p2p_hex(bytes, sizeof(bytes), text);
		snprintf(expected_invite, sizeof(expected_invite), "halo://join/%s", text);
	}
	check(count == 1 && !strcmp(listing.invite, expected_invite), "the listing's invite is the host's");
	check(count == 1 && !strcmp(listing.name, "Test game") && !strcmp(listing.map, "bloodgulch") &&
		!strcmp(listing.gametype, "Slayer") && listing.player_count == 3 && listing.maximum_player_count == 16 &&
		listing.engine_type == 2 && listing.open && listing.has_teams && !listing.in_progress, "the listing's details");
	/* a change: published again after TRIGGER_INTERVAL */
	p2p_set_game_listing(NULL, NULL, NULL, 2, 1, 1, 1);
	clock_now += 1000;
	lobby_update(token, 3, 16);
	check(publish_count == 1, "no publish sooner than 5 s after the last");
	clock_now += 5000;
	lobby_update(token, 3, 16);
	check(publish_count == 2, "a change published within 5 s");
	hear(published, published_size, 0, NULL);
	check(games(&listing) == 1 && listing.in_progress, "the browser takes the newer listing");
	/* the older one again: no change */
	hear(first, first_size, 0, NULL);
	check(games(&listing) == 1 && listing.in_progress, "an older listing is ignored");
	/* tampered: a newer sequence and another name, with the old signature */
	memcpy(tampered, published, (size_t)published_size);
	tampered[9] = (unsigned char)(tampered[9] + 50);
	tampered[66] = 'X';
	hear(tampered, published_size, 0, NULL);
	check(games(&listing) == 1 && listing.name[0] == 'T', "a tampered listing is ignored");
	/* on another's slot */
	hear(published, published_size, 0, "00112233445566778899aabbccddeeff");
	check(games(NULL) == 1, "a listing on another's slot is ignored");
	/* an emptied slot is no news */
	hear(published, 0, 0, NULL);
	check(games(NULL) == 1, "an emptied slot is not a delete");
	/* expiry: 90 s unheard */
	clock_now += 91000;
	lobby_update(token, 3, 16);
	check(games(NULL) == 0, "a listing unheard for 90 s expires");
	/* the slot's retained copy, of about now: taken (the host published it
	again on its 30 s) */
	lobby_update(token, 3, 16);
	hear(published, published_size, 1, NULL);
	check(games(NULL) == 1, "a retained listing of about now is taken");
	/* the host stops: a tombstone, which removes the game */
	p2p_set_hosting_public(0);
	lobby_update(token, 3, 16);
	check(published_closing && !p2p_lobby_listed(), "the host publishes a tombstone");
	hear(published, published_size, 0, NULL);
	check(games(NULL) == 0, "a tombstone removes the game");
	/* the listing before the tombstone does not bring it back */
	hear(first, first_size, 0, NULL);
	check(games(NULL) == 0, "a listing older than the tombstone is ignored");
	/* listed again (a new sequence): shown again */
	p2p_set_hosting_public(1);
	clock_now += 10000;
	lobby_update(token, 3, 16);
	hear(published, published_size, 0, NULL);
	check(games(NULL) == 1, "a listing newer than the tombstone is taken");
}

int main(void)
{
	crypto_checks();
	lobby_checks();
	printf("%s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
	return failures != 0;
}
