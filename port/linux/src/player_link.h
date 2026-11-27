/*
PLAYER_LINK.H

A player's Discord account linked at a master server (player_link.c;
docs/master_protocol.md, "Player keys, links and passes"): the player key
(player.key beside config.toml, made the first time it is needed), the
master's link page opened in the browser, the passes the masters give for
the key (pass-<master key id>.bin beside it), asked for again on a thread of
their own before they run out, and shown to a server that asks for one (a
join refused with _rejection_code_vpn_link_required: the client joins again
at once with it), with a proof that binds it to the connection. And a
server's checks of a pass shown to it.

Only integers and strings cross here: the game's side (the netcode, the
menus) calls it.
*/

#ifndef __HALO_LINUX_PLAYER_LINK_H
#define __HALO_LINUX_PLAYER_LINK_H

enum
{
	PLAYER_LINK_PASS_SIZE = 170,
	PLAYER_LINK_PROOF_SIZE = 64,
	PLAYER_LINK_BINDING_SIZE = 32,
	PLAYER_LINK_KEY_SIZE = 32,
	PLAYER_LINK_KEY_ID_SIZE = 8,
	PLAYER_LINK_NAME_SIZE = 32,
	/* the bytes the pass's signature covers, and where it is */
	PLAYER_LINK_PASS_SIGNED_SIZE = 106,
	PLAYER_LINK_MAXIMUM_MASTERS = 4,
	/* a Discord id as decimal text, with its end */
	PLAYER_LINK_DISCORD_ID_TEXT_SIZE = 24,
	/* the link page's confirmation code, "ABC-DEF", with its end */
	PLAYER_LINK_CODE_SIZE = 8,
};

/* a pass's fields (player_link_pass_read) */
struct player_link_pass
{
	int flags;
	unsigned char master_key_id[PLAYER_LINK_KEY_ID_SIZE];
	unsigned char player_key[PLAYER_LINK_KEY_SIZE];
	unsigned long long discord_id;
	/* printable ASCII, ended */
	char discord_name[PLAYER_LINK_NAME_SIZE + 1];
	unsigned long long issued;
	unsigned long long expires;
};

/* ---------- the format (pure: the harness tests them against the master's
vectors) */

/* the pass's fields, if it reads as one (its magic, format, size): 1 */
int player_link_pass_read(const unsigned char *pass, int size, struct player_link_pass *fields);
/* whether the pass's signature is the master key's */
int player_link_pass_signed(const unsigned char *pass, const unsigned char *master_key);
/* a connection's binding: HMAC-SHA-256 of "opence-link-binding-1" keyed
with the internet play tunnel's session secret; 32 zero bytes without one
(secret NULL: a LAN's connection) */
void player_link_binding(const unsigned char *secret, unsigned char *binding);
/* the proof's message: "opence-pass-proof-1", SHA-256 of the pass, the
binding (19 + 32 + 32 bytes) */
enum { PLAYER_LINK_PROOF_MESSAGE_SIZE = 19 + 32 + 32 };
void player_link_proof_message(const unsigned char *pass, const unsigned char *binding, unsigned char *message);
/* whether the proof is the pass's player key's, for the binding */
int player_link_proof_check(const unsigned char *pass, const unsigned char *binding, const unsigned char *proof);
/* the link page's confirmation code for the public key and time ("ABC-DEF") */
void player_link_code(const unsigned char *public_key, unsigned long long ts, char *code);
/* the text a link (prefix "opence-link-1") or a pass request
("opence-pass-get-1") signs, for the master's key id */
void player_link_request_text(const char *prefix, const unsigned char *master_key_id,
	const unsigned char *public_key, unsigned long long ts, char *text, int size);
/* a server's whole check of a pass shown to it (docs/master_protocol.md, "The
pass at the door"): its master one of the keys, its signature theirs, its
times within 300 seconds of now, the proof its player key's for the
binding. 1 with its fields */
int player_link_check(const unsigned char *pass, const unsigned char *proof, const unsigned char *binding,
	const unsigned char (*master_keys)[PLAYER_LINK_KEY_SIZE], int master_count, unsigned long long now,
	struct player_link_pass *fields);
/* the Discord account's creation, Unix seconds, from its id */
unsigned long long player_link_discord_created(unsigned long long discord_id);
/* which pass a join shows next: of this machine's masters (their key ids,
and whether each holds a good pass), the first after the one shown last
(after; -1: none yet) of a master the server's game is listed by (its
masters' key ids; none known: any master, as for an invite's join). -1 if
none; then needed is the first of this machine's masters the server is
listed by of which it holds no good pass (the one to link with), -1 if none */
int player_link_next_pass(const unsigned char (*key_ids)[PLAYER_LINK_KEY_ID_SIZE], const int *usable, int count,
	const unsigned char (*server_key_ids)[PLAYER_LINK_KEY_ID_SIZE], int server_count, int after, int *needed);

/* ---------- a server (the game's side: network_link.c) */

/* whether a pass is shown (any of its bytes not zero) */
int player_link_pass_shown(const unsigned char *pass);
/* the pass shown checked for this server: against the masters it publishes
to (server_master_publishing_keys), now, with the binding of the
connection's tunnel (its peer's virtual address, network byte order; 0: a
LAN's). 1 if it checks, with its account's id as decimal text, its
username and its creation (Unix seconds); 0 if not, why in why */
int player_link_server_check(const unsigned char *pass, const unsigned char *proof, unsigned long virtual_address,
	char *discord_id, int discord_id_size, char *discord_name, int discord_name_size, unsigned long *created,
	char *why, int why_size);

/* ---------- a player's machine */

/* the passes read, and asked for again before they run out (at the start;
a machine with no player key does nothing until one is made) */
void player_link_start(void);
/* the join's pass to show, if it shows one (player_link_join_refused said
so, or network.link_share is "always"): the pass of the master this machine
linked with, of those that list the game joined from the server browser
(p2p_joined_listing_masters; any held, in turn, for an invite's), and its
proof for the connection to the host (its virtual address, network byte
order; 0: a LAN's); 1 if it fills them */
int player_link_join_fill(unsigned long host_virtual_address, unsigned char *pass, unsigned char *proof);
/* a join refused (its rejection code): 1 if it should be made again at
once, with a pass (the next held, after one refused as invalid) */
int player_link_join_refused(int rejection_code);
/* a new join, or one accepted: no pass to show unless asked again */
void player_link_join_reset(void);
/* the master (its index among this machine's: player_link_master_state) a
server that refused the join for its VPN is listed by, of which this machine
holds no pass: the one to link with; -1 if none (until the next join) */
int player_link_needed_master(void);

/* Network Setup's DISCORD ACCOUNT (menu_functions.c): the link page opened
in the browser (the player key made if there is none) for this machine's
master at the index; 1 if it was */
int player_link_begin(int master_index);
/* ... every pass and the player key forgotten (the master's record of the
key is then unused, and deleted in time; /link/manage deletes it at once) */
void player_link_unlink(void);
/* ... its state, for its row */
enum
{
	_player_link_state_none,
	/* the link page opened: waiting for the master's pass (code: the page's) */
	_player_link_state_waiting,
	_player_link_state_linked,
	/* the master revoked the account */
	_player_link_state_revoked,
	/* linked, but the passes ran out and no master answers */
	_player_link_state_expired,
	/* not linked, and a server that refused a join for its VPN is listed by
	it (player_link_needed_master) */
	_player_link_state_needed,
};
/* ... the whole: linked (to any master) as the account, waiting for a link
page, revoked, expired, or none; how many of this machine's masters it is
linked with, of how many */
int player_link_state(char *name, int name_size, char *code, int code_size, int *linked, int *masters);
/* ... a master's (the index among this machine's: masters.txt's order): its
address's host and its state (none past the last master, with host empty) */
int player_link_master_state(int master_index, char *host, int host_size, char *name, int name_size, char *code,
	int code_size);

#endif
