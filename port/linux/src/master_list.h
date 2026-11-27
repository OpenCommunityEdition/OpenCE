/*
MASTER_LIST.H

The server browser's master servers (master_list.c; docs/master_protocol.md,
"The list"): while the browser is open, each master of masters.txt is asked
for its list every 30 seconds (and on Refresh), on a thread of its own (the
official masters, built in, when masters.txt names none); a list whose
signature by the master's key (pinned in masters.txt, or built in) checks, and
that is fresh, gives its dedicated servers' host-signed listings to the
browser's games (p2p_lobby.c), each checked again there. network.listings
says whether the masters are asked at all.
*/

#ifndef __HALO_LINUX_MASTER_LIST_H
#define __HALO_LINUX_MASTER_LIST_H

enum
{
	MASTER_LIST_MAXIMUM_MASTERS = 4,
	MASTER_LIST_KEY_SIZE = 32,
	MASTER_LIST_URL_SIZE = 256,
};

/* network.listings: what the server browser lists */
enum
{
	_master_listings_both,
	_master_listings_masters,
	_master_listings_brokers,
};

/* the masters of a masters.txt (the setting's file: beside config.toml
unless a full path, else beside the executable): their URLs and Ed25519
keys, up to maximum; how many (none: no file, or one with no master) */
int master_list_read_file(const char *setting, char (*urls)[MASTER_LIST_URL_SIZE],
	unsigned char (*keys)[MASTER_LIST_KEY_SIZE], int maximum);
/* ... the official master servers (built in): how many */
int master_list_read_defaults(char (*urls)[MASTER_LIST_URL_SIZE], unsigned char (*keys)[MASTER_LIST_KEY_SIZE],
	int maximum);
/* ... masters.txt's masters, else (it names none) the official ones. The
server browser and a dedicated server's publisher ask these */
int master_list_read(const char *setting, char (*urls)[MASTER_LIST_URL_SIZE],
	unsigned char (*keys)[MASTER_LIST_KEY_SIZE], int maximum);

/* ... the same of a masters.json ({"masters":[{"url":…,"key":"ed25519:…"}]},
the web site's) */
int master_list_read_json(const char *text, unsigned long size, char (*urls)[MASTER_LIST_URL_SIZE],
	unsigned char (*keys)[MASTER_LIST_KEY_SIZE], int maximum);

/* the masters a player's machine asks (the server browser's, and a Discord
link's: player_link.c): masters.txt's; else, in the web build, its site's
masters.json (the page's list of them, which its Servers panel reads too);
else the official ones. How many */
int master_list_client_masters(char (*urls)[MASTER_LIST_URL_SIZE], unsigned char (*keys)[MASTER_LIST_KEY_SIZE]);
/* a GET of the url, as the server browser asks a master on each platform
(Mbed TLS or WinHTTP, the Android app's host, the page's fetch): 1 if an
answer came, with its status and its body (malloc'd, NUL terminated after
its size: the caller frees it; 1 MB at most). Any thread; it may block */
int master_list_get(const char *url, int *status, char **body, unsigned long *size);

/* the server browser open (asked every 30 seconds) or closed */
void master_list_browse(int on);
/* Refresh: asked again now */
void master_list_refresh(void);
/* network.listings' choice */
int master_list_policy(void);
/* the masters in masters.txt, how many answered with a list that checked
last time, and how many were asked yet (all: a round is done) */
void master_list_counts(int *count, int *answered, int *asked);

/* a signed list's checks (pure: the harness tests them): the body's
signature header's value ("ed25519=<base64>") over the body with the key,
the Signature-Key header's (the key's id: none is refused), and the body's
master and time (300 seconds old at most, and ahead of now at most); 1 if
it is a list to take */
int master_list_verify(const char *body, unsigned long size, const char *signature_header, const char *key_header,
	const unsigned char *key, unsigned long now, char *why, int why_size);

#endif
