/*
SERVER_MASTER.H

A dedicated server's publishing to the official master servers
(server_master.c; docs/master_protocol.md): its operator's publisher key
(publisher.key, made by halo --server --keygen), the masters of masters.txt,
the state of the game published to each every 30 seconds and at once when
it changes, signed with the key, and the masters' reachability probes
answered on the tunnel's port. It runs on a thread of its own: a slow master
never holds the game.
*/

#ifndef __HALO_LINUX_SERVER_MASTER_H
#define __HALO_LINUX_SERVER_MASTER_H

struct server_json;

enum
{
	SERVER_MASTER_PUBLIC_KEY_SIZE = 32,
	SERVER_MASTER_SEED_SIZE = 32,
	SERVER_MASTER_KEY_ID_SIZE = 8,
	/* "ed25519:" and the key in hex */
	SERVER_MASTER_PUBLIC_KEY_TEXT_SIZE = 8 + 64 + 1,
	SERVER_MASTER_MAXIMUM_MASTERS = 4,
};

/* --keygen: a new publisher key written (unless there is one), its public
key and id printed; 1 on success */
int server_master_keygen(void);
/* --sign <text>: the key's signature of the text printed in hex; 1 */
int server_master_sign_text(const char *text);

/* the publisher key (HALO_SERVER_PUBLISHER_KEY, else the file): 1 with its
public key and id (16 hex) */
int server_master_publisher(unsigned char *public_key, char *key_id);

/* the publishing thread started (server.master_publish, with masters and
a key) */
void server_master_start(void);
/* a player's host (not a dedicated server's): published as above while it
hosts a public internet game, when this machine has a publisher key and
masters.txt names masters (docs/master_protocol.md); nothing otherwise */
void server_master_start_host(void);
/* the game's state to publish (the director's, as it changes): its JSON
object's members (name, map, gametype, variant, players, ...); changed
asks for a publish within 5 seconds */
void server_master_set_state(const char *members, int changed);
/* the hosted game changed (p2p_lobby.c: public or not, its players, its
name or map): a publish within 5 seconds, as for a changed state */
void server_master_changed(void);
/* the listings withdrawn as the server stops (a tombstone to each master:
2 seconds at most) */
void server_master_stop(void);

/* a datagram the tunnel's socket received (p2p.c) from the address (IPv4,
network byte order): 1 if it was a master's probe to answer, its answer (at
most 128 bytes) then in answer, to be sent back to where it came from */
int server_master_probe(const unsigned char *data, int size, unsigned long address, unsigned char *answer,
	int *answer_size);

/* the name the masters know this server by: server.instance as published
(else a slug of server.name), and the publisher key's id (16 hex; empty
without a key), as the start read them */
void server_master_identity(char *instance, int instance_size, char *key_id, int key_id_size);

/* the masters this server publishes to (the masters of
server.masters_file, with server.master_publish and a publisher key): their
URLs (urls may be NULL) and pinned keys, up to maximum; how many (none for
a server that publishes nowhere, and a player's machine). The netlists
(server_netlist.c) and the Discord links' passes (player_link.c) are taken
from them alone */
int server_master_publishing_masters(char (*urls)[256], unsigned char (*keys)[SERVER_MASTER_PUBLIC_KEY_SIZE],
	int maximum);
#define server_master_publishing_keys(keys, maximum) server_master_publishing_masters(NULL, (keys), (maximum))

/* each master's state, for the status (masters[]) */
void server_master_status(struct server_json *json);

#endif
