/*
PLAYER_LINK.C (test)

A Discord link's pass and a dedicated server's netlist (docs/master_protocol.md,
"The netlist", "Player keys, links and passes"), byte for byte against the
master's test vectors (vectors.inc, from its testdata/vectors.json): a pass
read, its master's signature, a connection's binding and the player key's
proof, the server's whole check of a pass at the door, the link page's
confirmation code and the texts a link and a pass request sign
(port/linux/src/player_link.c); a netlist read, the addresses in its ranges,
and the ranges of vpn_allow.txt (port/linux/src/server_netlist.c); on the real
SHA-256 and Ed25519 (port/linux/src/p2p_crypto.c, Monocypher):
test_player_link.py takes the code (under_test.inc).
*/

#include "harness.h"

#include <stdint.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang optimize off
#include "monocypher.inc"
#define hash_reduce ed25519_hash_reduce
#include "monocypher_ed25519.inc"
#undef hash_reduce
#pragma clang optimize on
#pragma clang diagnostic pop
#undef FOR
#undef COPY
#undef ZERO
#undef WIPE_CTX
#undef WIPE_BUFFER
#undef MIN

#include "p2p_crypto.inc"

void posix_random_bytes(void *buffer, posix_ulong size)
{
	memset(buffer, 0, size);
}

#include "player_link.h"
#include "server_netlist.h"

struct vector_bytes
{
	const char *name;
	const char *hex;
};

#include "vectors.inc"

#include "under_test.inc"

/* ---------- the cases */

static int from_hex(const char *hex, unsigned char *bytes, int maximum)
{
	int size = (int)strlen(hex) / 2, index;

	CHECK(size <= maximum, "a vector of %d bytes, more than %d", size, maximum);
	for (index = 0; index < size; index++)
	{
		unsigned int value;

		sscanf(hex + 2 * index, "%2x", &value);
		bytes[index] = (unsigned char)value;
	}
	return size;
}

static void key_of(const char *hex, unsigned char *key)
{
	CHECK(from_hex(hex, key, 32) == 32, "a key of 32 bytes");
}

static void vector_pass(unsigned char *pass)
{
	CHECK(from_hex(VECTOR_PASS_BYTES, pass, 400) == PLAYER_LINK_PASS_SIZE, "the vector pass is 170 bytes");
}

static void case_pass_read(void)
{
	unsigned char pass[400], player_key[32], master_key_id[8];
	struct player_link_pass fields;

	vector_pass(pass);
	key_of(VECTOR_PASS_PLAYER_PUBLIC_KEY, player_key);
	from_hex(VECTOR_MASTER_KEY_ID_BYTES, master_key_id, 8);
	CHECK(player_link_pass_read(pass, PLAYER_LINK_PASS_SIZE, &fields), "the vector pass does not read");
	CHECK(fields.flags == VECTOR_PASS_FLAGS, "flags %d", fields.flags);
	CHECK(!memcmp(fields.master_key_id, master_key_id, 8), "its master's key id");
	CHECK(!memcmp(fields.player_key, player_key, 32), "its player key");
	CHECK(fields.discord_id == strtoull(VECTOR_PASS_DISCORD_ID, NULL, 10), "its Discord id %llu", fields.discord_id);
	CHECK(!strcmp(fields.discord_name, VECTOR_PASS_DISCORD_NAME), "its name '%s'", fields.discord_name);
	CHECK(fields.issued == VECTOR_PASS_ISSUED && fields.expires == VECTOR_PASS_EXPIRES, "its times %llu %llu",
		fields.issued, fields.expires);
	CHECK(!player_link_pass_read(pass, PLAYER_LINK_PASS_SIZE - 1, &fields), "a pass cut short reads");
	/* (a name's bytes kept to printable ASCII, whatever the pass holds) */
	pass[58] = 0x07;
	pass[59] = 0xC3;
	CHECK(player_link_pass_read(pass, PLAYER_LINK_PASS_SIZE, &fields) && fields.discord_name[0] == '?' &&
		fields.discord_name[1] == '?', "a name of control bytes read as '%s'", fields.discord_name);
	vector_pass(pass);
	CHECK(player_link_pass_read(pass, PLAYER_LINK_PASS_SIZE, &fields), "the vector pass does not read again");
	CHECK(player_link_discord_created(fields.discord_id) == VECTOR_DISCORD_CREATED, "its account made at %llu",
		player_link_discord_created(fields.discord_id));
}

static void case_pass_signed(void)
{
	unsigned char pass[400], master_key[32], publisher_key[32], message[200];

	vector_pass(pass);
	key_of(VECTOR_MASTER_PUBLIC_KEY, master_key);
	key_of(VECTOR_PUBLISHER_PUBLIC_KEY, publisher_key);
	CHECK(from_hex(VECTOR_PASS_SIGNED_MESSAGE, message, sizeof(message)) == 13 + PLAYER_LINK_PASS_SIGNED_SIZE,
		"the signed message's size");
	CHECK(!memcmp(message, "opence-pass-1", 13) && !memcmp(message + 13, pass, PLAYER_LINK_PASS_SIGNED_SIZE),
		"the signed message is not the label and the pass");
	CHECK(player_link_pass_signed(pass, master_key), "the master's signature of the pass does not check");
	CHECK(!player_link_pass_signed(pass, publisher_key), "another key's checks");
}

static void case_proof(void)
{
	unsigned char pass[400], secret[32], binding[32], expected[32], message[PLAYER_LINK_PROOF_MESSAGE_SIZE];
	unsigned char expected_message[200], proof[64];

	vector_pass(pass);
	from_hex(VECTOR_PROOF_SESSION_SECRET, secret, 32);
	player_link_binding(secret, binding);
	from_hex(VECTOR_PROOF_BINDING, expected, 32);
	CHECK(!memcmp(binding, expected, 32), "the binding of the session's secret");
	player_link_proof_message(pass, binding, message);
	CHECK(from_hex(VECTOR_PROOF_MESSAGE, expected_message, sizeof(expected_message)) == sizeof(message) &&
		!memcmp(message, expected_message, sizeof(message)), "the proof's message");
	from_hex(VECTOR_PROOF_PROOF, proof, 64);
	CHECK(player_link_proof_check(pass, binding, proof), "the vector proof does not check");
	/* (a LAN's connection: no tunnel, a binding of zeros) */
	player_link_binding(NULL, binding);
	from_hex(VECTOR_PROOF_LAN_BINDING, expected, 32);
	CHECK(!memcmp(binding, expected, 32), "a LAN's binding");
	player_link_proof_message(pass, binding, message);
	CHECK(from_hex(VECTOR_PROOF_LAN_MESSAGE, expected_message, sizeof(expected_message)) == sizeof(message) &&
		!memcmp(message, expected_message, sizeof(message)), "a LAN's proof's message");
	CHECK(!player_link_proof_check(pass, binding, proof), "the tunnel's proof checks for a LAN's connection");
	from_hex(VECTOR_PROOF_LAN_PROOF, proof, 64);
	CHECK(player_link_proof_check(pass, binding, proof), "the LAN's proof does not check");
}

static void case_check(void)
{
	unsigned char pass[400], secret[32], binding[32], proof[64], keys[2][PLAYER_LINK_KEY_SIZE];
	struct player_link_pass fields;
	unsigned long long issued = VECTOR_PASS_ISSUED, expires = VECTOR_PASS_EXPIRES;

	vector_pass(pass);
	from_hex(VECTOR_PROOF_SESSION_SECRET, secret, 32);
	player_link_binding(secret, binding);
	from_hex(VECTOR_PROOF_PROOF, proof, 64);
	key_of(VECTOR_PUBLISHER_PUBLIC_KEY, keys[0]);
	key_of(VECTOR_MASTER_PUBLIC_KEY, keys[1]);
	/* (the master second of those the server publishes to) */
	CHECK(player_link_check(pass, proof, binding, (const unsigned char (*)[32])keys, 2, issued + 10, &fields),
		"the vector pass is refused");
	CHECK(fields.discord_id == strtoull(VECTOR_PASS_DISCORD_ID, NULL, 10), "its account");
	CHECK(!player_link_check(pass, proof, binding, (const unsigned char (*)[32])keys, 1, issued + 10, &fields),
		"a pass of a master the server does not publish to is taken");
	CHECK(player_link_check(pass, proof, binding, (const unsigned char (*)[32])keys, 2, issued - 300, &fields),
		"a pass 300 seconds ahead of the clock is refused");
	CHECK(!player_link_check(pass, proof, binding, (const unsigned char (*)[32])keys, 2, issued - 301, &fields),
		"a pass more than 300 seconds ahead of the clock is taken");
	CHECK(player_link_check(pass, proof, binding, (const unsigned char (*)[32])keys, 2, expires + 300, &fields),
		"a pass 300 seconds out of date is refused");
	CHECK(!player_link_check(pass, proof, binding, (const unsigned char (*)[32])keys, 2, expires + 301, &fields),
		"a pass more than 300 seconds out of date is taken");
	binding[0] ^= 1;
	CHECK(!player_link_check(pass, proof, binding, (const unsigned char (*)[32])keys, 2, issued + 10, &fields),
		"a proof for another connection is taken");
}

static void case_passes_refused(void)
{
	unsigned char keys[1][PLAYER_LINK_KEY_SIZE], binding[32], proof[64];
	struct player_link_pass fields;
	int index;

	key_of(VECTOR_MASTER_PUBLIC_KEY, keys[0]);
	for (index = 0; index < (int)(sizeof(vector_passes_refused) / sizeof(vector_passes_refused[0])); index++)
	{
		unsigned char pass[400];
		unsigned char message[PLAYER_LINK_PROOF_MESSAGE_SIZE];
		unsigned char seed[32], public_key[32];
		int size = from_hex(vector_passes_refused[index].hex, pass, sizeof(pass));

		/* (with a proof that checks, by the player's key, for each: only the
		pass is wrong) */
		memset(pass + size, 0, (size_t)(sizeof(pass) - size));
		player_link_binding(NULL, binding);
		player_link_proof_message(pass, binding, message);
		from_hex(VECTOR_PLAYER_SEED, seed, 32);
		p2p_ed25519_public(seed, public_key, NULL);
		p2p_ed25519_sign(seed, public_key, message, (int)sizeof(message), proof);
		CHECK(size != PLAYER_LINK_PASS_SIZE || !player_link_check(pass, proof, binding,
			(const unsigned char (*)[32])keys, 1, VECTOR_PASS_ISSUED + 10, &fields),
			"the refused pass '%s' is taken", vector_passes_refused[index].name);
		CHECK(!player_link_pass_read(pass, size, &fields) || size == PLAYER_LINK_PASS_SIZE,
			"the refused pass '%s' reads", vector_passes_refused[index].name);
	}
}

static void case_link_texts(void)
{
	unsigned char seed[32], public_key[32], master_key_id[8], signature[64];
	char text[300], code[PLAYER_LINK_CODE_SIZE], hex[129];

	from_hex(VECTOR_PLAYER_SEED, seed, 32);
	p2p_ed25519_public(seed, public_key, NULL);
	from_hex(VECTOR_MASTER_KEY_ID_BYTES, master_key_id, 8);
	player_link_request_text("opence-link-1", master_key_id, public_key, strtoull(VECTOR_LINK_TS, NULL, 10), text,
		sizeof(text));
	CHECK(!strcmp(text, VECTOR_LINK_SIGNED_TEXT), "the link's text '%s'", text);
	p2p_ed25519_sign(seed, public_key, text, (int)strlen(text), signature);
	p2p_hex(signature, 64, hex);
	CHECK(!strcmp(hex, VECTOR_LINK_SIG), "the link's signature %s", hex);
	player_link_code(public_key, strtoull(VECTOR_LINK_TS, NULL, 10), code);
	CHECK(!strcmp(code, VECTOR_LINK_CODE), "the link's code %s", code);
	player_link_request_text("opence-pass-get-1", master_key_id, public_key, strtoull(VECTOR_PASS_REQUEST_TS, NULL, 10),
		text, sizeof(text));
	CHECK(!strcmp(text, VECTOR_PASS_REQUEST_SIGNED_TEXT), "the pass request's text '%s'", text);
	p2p_ed25519_sign(seed, public_key, text, (int)strlen(text), signature);
	p2p_hex(signature, 64, hex);
	CHECK(!strcmp(hex, VECTOR_PASS_REQUEST_SIG), "the pass request's signature %s", hex);
}

static unsigned long dotted(const char *text)
{
	unsigned int a, b, c, d;

	CHECK(sscanf(text, "%u.%u.%u.%u", &a, &b, &c, &d) == 4, "an address %s", text);
	return (unsigned long)a << 24 | (unsigned long)b << 16 | (unsigned long)c << 8 | d;
}

static void case_netlist(void)
{
	static unsigned char body[4096];
	unsigned long long generated, serial;
	unsigned long count, size = (unsigned long)from_hex(VECTOR_NETLIST_BODY, body, sizeof(body));
	char why[160];
	int index;

	CHECK(server_netlist_read(body, size, &generated, &serial, &count, why, sizeof(why)), "the netlist: %s", why);
	CHECK(generated == VECTOR_NETLIST_GENERATED && serial == VECTOR_NETLIST_SERIAL, "its times %llu %llu",
		generated, serial);
	CHECK(count == sizeof(vector_netlist_ranges) / sizeof(vector_netlist_ranges[0]), "its %lu ranges", count);
	for (index = 0; index < (int)count; index++)
	{
		unsigned long first = dotted(vector_netlist_ranges[index][0]), last = dotted(vector_netlist_ranges[index][1]);

		CHECK(server_netlist_ranges_contain(body + 32, count, first), "%s not listed", vector_netlist_ranges[index][0]);
		CHECK(server_netlist_ranges_contain(body + 32, count, last), "%s not listed", vector_netlist_ranges[index][1]);
		CHECK(!server_netlist_ranges_contain(body + 32, count, first - 1), "the address before %s listed",
			vector_netlist_ranges[index][0]);
		CHECK(!server_netlist_ranges_contain(body + 32, count, last + 1), "the address after %s listed",
			vector_netlist_ranges[index][1]);
	}
	CHECK(!server_netlist_ranges_contain(body + 32, count, 0) &&
		!server_netlist_ranges_contain(body + 32, count, 0xFFFFFFFFUL), "the ends listed");
	CHECK(!server_netlist_ranges_contain(body + 32, 0, dotted(vector_netlist_ranges[0][0])), "an empty list holds");
}

static void case_netlists_refused(void)
{
	static unsigned char body[4096];
	unsigned long long generated, serial;
	unsigned long count;
	char why[160];
	int index;

	for (index = 0; index < (int)(sizeof(vector_netlists_refused) / sizeof(vector_netlists_refused[0])); index++)
	{
		unsigned long size = (unsigned long)from_hex(vector_netlists_refused[index].hex, body, sizeof(body));

		CHECK(!server_netlist_read(body, size, &generated, &serial, &count, why, sizeof(why)),
			"the refused netlist '%s' reads", vector_netlists_refused[index].name);
	}
}

static void case_ranges_text(void)
{
	unsigned long pairs[2 * 16];
	int bad;
	unsigned long count = server_netlist_parse_ranges(
		"# a comment\n"
		"203.0.113.0/24\n"
		"  198.51.100.7   # one address\n"
		"198.51.100.8-198.51.100.9\r\n"
		"203.0.113.128/25\n"
		"10.0.0.0/33\n"
		"not an address\n"
		"0.0.0.0/0\n", pairs, 16, &bad);

	CHECK(bad == 2, "%d lines that are no range, not 2", bad);
	/* (0.0.0.0/0 holds every other one: merged into one) */
	CHECK(count == 1 && pairs[0] == 0 && pairs[1] == 0xFFFFFFFFUL, "%lu ranges", count);
	count = server_netlist_parse_ranges("203.0.113.0/24\n198.51.100.7\n198.51.100.8-198.51.100.9\n203.0.114.0/24\n"
		"1.2.3.4/32\n", pairs, 16, &bad);
	CHECK(bad == 0, "%d bad lines", bad);
	CHECK(count == 3, "%lu ranges, not 3 (the touching ones merged)", count);
	CHECK(pairs[0] == dotted("1.2.3.4") && pairs[1] == dotted("1.2.3.4"), "the first range");
	CHECK(pairs[2] == dotted("198.51.100.7") && pairs[3] == dotted("198.51.100.9"), "the second range");
	CHECK(pairs[4] == dotted("203.0.113.0") && pairs[5] == dotted("203.0.114.255"), "the third range");
}

static void case_next_pass(void)
{
	/* (this machine's masters A, B, C; servers listed by some of them, and
	by D, which this machine does not know) */
	static const unsigned char keys[3][8] = { "masterAA", "masterBB", "masterCC" };
	static const unsigned char by_b[1][8] = { "masterBB" };
	static const unsigned char by_d_b[2][8] = { "masterDD", "masterBB" };
	static const unsigned char by_d[1][8] = { "masterDD" };
	static const unsigned char by_a_c[2][8] = { "masterAA", "masterCC" };
	int only_b[3] = { 0, 1, 0 }, only_a[3] = { 1, 0, 0 }, all[3] = { 1, 1, 1 }, none[3] = { 0, 0, 0 };
	int needed;

	/* (linked with B alone: B's pass for a server B lists, whichever else does) */
	CHECK(player_link_next_pass(keys, only_b, 3, by_b, 1, -1, &needed) == 1 && needed == -1, "B's pass for B's server");
	CHECK(player_link_next_pass(keys, only_b, 3, by_d_b, 2, -1, &needed) == 1, "B's pass for a server of D and B");
	/* (and none after it: a refusal of B's is the last) */
	CHECK(player_link_next_pass(keys, only_b, 3, by_b, 1, 1, &needed) == -1 && needed == -1, "a pass after B's");
	/* (linked with A alone, a server B lists: no pass shown, B the one to
	link with) */
	CHECK(player_link_next_pass(keys, only_a, 3, by_b, 1, -1, &needed) == -1 && needed == 1,
		"A's pass shown to B's server, or B not named");
	/* (a server listed only by masters this machine does not know: nothing) */
	CHECK(player_link_next_pass(keys, all, 3, by_d, 1, -1, &needed) == -1 && needed == -1, "a pass for D's server");
	/* (not linked at all, a server of A and C: A named) */
	CHECK(player_link_next_pass(keys, none, 3, by_a_c, 2, -1, &needed) == -1 && needed == 0, "A not named");
	/* (linked with all: the first that lists it, then the next) */
	CHECK(player_link_next_pass(keys, all, 3, by_a_c, 2, -1, &needed) == 0, "A first");
	CHECK(player_link_next_pass(keys, all, 3, by_a_c, 2, 0, &needed) == 2, "C after A, not B");
	/* (an invite's join, its masters not known: each held in turn) */
	CHECK(player_link_next_pass(keys, all, 3, NULL, 0, -1, &needed) == 0 &&
		player_link_next_pass(keys, all, 3, NULL, 0, 0, &needed) == 1 &&
		player_link_next_pass(keys, all, 3, NULL, 0, 2, &needed) == -1 && needed == -1, "an invite's turns");
	CHECK(player_link_next_pass(keys, only_b, 3, NULL, 0, -1, &needed) == 1, "the held one for an invite");
}

int main(int argc, char **argv)
{
	static const struct
	{
		const char *name;
		void (*run)(void);
	} cases[] =
	{
		{ "pass-read", case_pass_read },
		{ "pass-signed", case_pass_signed },
		{ "proof", case_proof },
		{ "check", case_check },
		{ "passes-refused", case_passes_refused },
		{ "link-texts", case_link_texts },
		{ "netlist", case_netlist },
		{ "netlists-refused", case_netlists_refused },
		{ "ranges-text", case_ranges_text },
		{ "next-pass", case_next_pass },
	};
	int index;

	for (index = 0; argc > 1 && index < (int)(sizeof(cases) / sizeof(cases[0])); index++)
	{
		if (!strcmp(argv[1], cases[index].name))
		{
			cases[index].run();
			return 0;
		}
	}
	fprintf(stderr, "no such case\n");
	return 2;
}
