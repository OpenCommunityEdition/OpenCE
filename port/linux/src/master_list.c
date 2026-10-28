/*
MASTER_LIST.C

The server browser's master servers (master_list.h).
*/

#include "platform.h"
#include "port_config.h"
#include "master_list.h"
#include "p2p.h"
#include "p2p_internal.h"
#include "server_json.h"

#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(HALO_ANDROID) && !defined(HALO_WEB)
#include <SDL3/SDL.h>
#include "update.h"
#endif

enum
{
	/* seconds between rounds while browsing; a list older than this is
	refused (a replayed one), and one dated further ahead of this machine's
	clock (one made to be replayed later) */
	ROUND_SECONDS = 30,
	FRESH_SECONDS = 300,
	MAXIMUM_ENTRIES = 512,
	MAXIMUM_ANSWER = 1024 * 1024,
	MAXIMUM_LISTING = 400,
};

static struct
{
	pthread_mutex_t lock;
	pthread_cond_t wake;
	/* each opening of the browser's: a thread of an earlier one (still
	asking a master when the browser was closed and opened again) ends at
	its next look, without being waited for */
	unsigned long generation;
	int browsing;
	int refresh;
	int count;
	int answered;
	int asked;
	char urls[MASTER_LIST_MAXIMUM_MASTERS][MASTER_LIST_URL_SIZE];
	unsigned char keys[MASTER_LIST_MAXIMUM_MASTERS][MASTER_LIST_KEY_SIZE];
} list = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER };

/* waits on the condition for seconds at most */
static void wait_seconds(int seconds)
{
	struct timespec until;

	clock_gettime(CLOCK_REALTIME, &until);
	until.tv_sec += seconds;
	pthread_cond_timedwait(&list.wake, &list.lock, &until);
}

static int hex_digit(char character)
{
	if (character >= '0' && character <= '9')
		return character - '0';
	if (character >= 'a' && character <= 'f')
		return character - 'a' + 10;
	if (character >= 'A' && character <= 'F')
		return character - 'A' + 10;
	return -1;
}

/* ---------- masters.txt */

/* whether a master's URL may be asked: https, or plain http to this machine
alone (a test's master): http://127.0.0.1 or http://localhost, then a port,
a path or nothing (not http://127.0.0.1.example.net) */
static int master_url_allowed(const char *url)
{
	static const char *const loopbacks[] = { "http://127.0.0.1", "http://localhost" };
	size_t index;

	if (!strncmp(url, "https://", 8))
		return 1;
	for (index = 0; index < sizeof(loopbacks) / sizeof(loopbacks[0]); index++)
	{
		size_t length = strlen(loopbacks[index]);

		if (!strncmp(url, loopbacks[index], length) && (!url[length] || url[length] == ':' || url[length] == '/'))
			return 1;
	}
	return 0;
}

static char *masters_file(const char *setting, char *path, int size)
{
	char folder[1024];
	size_t length;
	char *text;
	const char *base;

	if (setting[0] == '/' || (setting[0] && setting[1] == ':'))
	{
		snprintf(path, (size_t)size, "%s", setting);
		return config_file_read(path, &length);
	}
	config_folder(folder, sizeof(folder));
	snprintf(path, (size_t)size, "%s%s", folder, setting);
	text = config_file_read(path, &length);
#if !defined(HALO_ANDROID) && !defined(HALO_WEB)
	base = SDL_GetBasePath();
	if (!text && base)
	{
		snprintf(path, (size_t)size, "%s%s", base, setting);
		text = config_file_read(path, &length);
	}
#else
	(void)base;
#endif
	return text;
}

int master_list_read_file(const char *setting, char (*urls)[MASTER_LIST_URL_SIZE],
	unsigned char (*keys)[MASTER_LIST_KEY_SIZE], int maximum)
{
	char path[1200];
	char *text = masters_file(setting, path, sizeof(path));
	const char *line;
	int count = 0;

	if (!text)
		return 0;
	for (line = text; *line && count < maximum;)
	{
		const char *end = line + strcspn(line, "\n");
		char copy[512];
		char url[MASTER_LIST_URL_SIZE], key[96];
		int index, ok = 1;

		snprintf(copy, sizeof(copy), "%.*s", (int)(end - line), line);
		copy[strcspn(copy, "#\r")] = 0;
		line = *end ? end + 1 : end;
		if (sscanf(copy, "%255s %95s", url, key) != 2)
			continue;
		/* (https, or plain http to this machine: a test's master) */
		if (!master_url_allowed(url) ||
			strncmp(key, "ed25519:", 8) || strlen(key) != 8 + 2 * MASTER_LIST_KEY_SIZE)
		{
			continue;
		}
		for (index = 0; index < MASTER_LIST_KEY_SIZE && ok; index++)
		{
			int high = hex_digit(key[8 + 2 * index]), low = hex_digit(key[8 + 2 * index + 1]);

			ok = high >= 0 && low >= 0;
			keys[count][index] = (unsigned char)(high << 4 | low);
		}
		if (!ok)
			continue;
		while (strlen(url) > 8 && url[strlen(url) - 1] == '/')
			url[strlen(url) - 1] = 0;
		snprintf(urls[count], MASTER_LIST_URL_SIZE, "%s", url);
		count++;
	}
	free(text);
	return count;
}

/* one master (a URL and its "ed25519:<hex>" key) checked and kept: 1 */
static int master_keep(const char *url, const char *key, char (*urls)[MASTER_LIST_URL_SIZE],
	unsigned char (*keys)[MASTER_LIST_KEY_SIZE], int index)
{
	int digit;

	if (!master_url_allowed(url) ||
		strlen(url) >= MASTER_LIST_URL_SIZE || strncmp(key, "ed25519:", 8) || strlen(key) != 8 + 2 * MASTER_LIST_KEY_SIZE)
	{
		return 0;
	}
	for (digit = 0; digit < MASTER_LIST_KEY_SIZE; digit++)
	{
		int high = hex_digit(key[8 + 2 * digit]), low = hex_digit(key[8 + 2 * digit + 1]);

		if (high < 0 || low < 0)
			return 0;
		keys[index][digit] = (unsigned char)(high << 4 | low);
	}
	snprintf(urls[index], MASTER_LIST_URL_SIZE, "%s", url);
	while (strlen(urls[index]) > 8 && urls[index][strlen(urls[index]) - 1] == '/')
		urls[index][strlen(urls[index]) - 1] = 0;
	return 1;
}

/* the official master servers, asked when masters.txt names none (it is
missing, empty or has comments alone) */
int master_list_read_defaults(char (*urls)[MASTER_LIST_URL_SIZE], unsigned char (*keys)[MASTER_LIST_KEY_SIZE],
	int maximum)
{
	static const struct
	{
		const char *url;
		const char *key;
	} masters[] =
	{
		{ "https://master.opencommunityedition.com",
			"ed25519:89f4eda71229f11f3538679f49ef7fb0c2b407b380093f7f4fe02105b92225aa" },
	};
	int index, count = 0;

	for (index = 0; index < (int)(sizeof(masters) / sizeof(masters[0])) && count < maximum; index++)
		count += master_keep(masters[index].url, masters[index].key, urls, keys, count);
	return count;
}

int master_list_read(const char *setting, char (*urls)[MASTER_LIST_URL_SIZE],
	unsigned char (*keys)[MASTER_LIST_KEY_SIZE], int maximum)
{
	int count = master_list_read_file(setting, urls, keys, maximum);

	return count ? count : master_list_read_defaults(urls, keys, maximum);
}

int master_list_read_json(const char *text, unsigned long size, char (*urls)[MASTER_LIST_URL_SIZE],
	unsigned char (*keys)[MASTER_LIST_KEY_SIZE], int maximum)
{
	struct server_json_document document;
	int masters, index, count = 0;

	if (!server_json_parse(&document, text, (int)size, 8))
		return 0;
	masters = server_json_member(&document, 0, "masters");
	for (index = 0; count < maximum && server_json_element(&document, masters, index) >= 0; index++)
	{
		int entry = server_json_element(&document, masters, index);
		char url[MASTER_LIST_URL_SIZE], key[96];

		if (server_json_get_string(&document, server_json_member(&document, entry, "url"), url, sizeof(url)) &&
			server_json_get_string(&document, server_json_member(&document, entry, "key"), key, sizeof(key)))
		{
			count += master_keep(url, key, urls, keys, count);
		}
	}
	server_json_document_free(&document);
	return count;
}

int master_list_policy(void)
{
	const char *setting = config_string("network.listings");

	if (!strcmp(setting, "masters"))
		return _master_listings_masters;
	if (!strcmp(setting, "brokers"))
		return _master_listings_brokers;
	return _master_listings_both;
}

/* ---------- a list's checks */

int master_list_verify(const char *body, unsigned long size, const char *signature_header, const char *key_header,
	const unsigned char *key, unsigned long now, char *why, int why_size)
{
	unsigned char signature[P2P_SIGNATURE_SIZE + 4];
	unsigned char digest[P2P_SHA256_SIZE];
	char key_id[2 * 8 + 1];
	char text[96];
	struct server_json_document document;
	double protocol, time_value;
	int master, ok;

	p2p_sha256(key, MASTER_LIST_KEY_SIZE, digest);
	p2p_hex(digest, 8, key_id);
	if (strncmp(signature_header, "ed25519=", 8) ||
		server_base64_decode(signature_header + 8, signature, sizeof(signature)) != P2P_SIGNATURE_SIZE)
	{
		snprintf(why, (size_t)why_size, "no signature");
		return 0;
	}
	/* (the key the master says it signed with, which must be the one
	pinned: a list without it is refused too) */
	if (!key_header || strcmp(key_header, key_id))
	{
		snprintf(why, (size_t)why_size, "signed by another key (%.16s)", key_header ? key_header : "none said");
		return 0;
	}
	if (!p2p_ed25519_verify(key, body, (int)size, signature))
	{
		snprintf(why, (size_t)why_size, "its signature does not check");
		return 0;
	}
	if (!server_json_parse_tokens(&document, body, (int)size, 16, SERVER_JSON_MAXIMUM_LIST_TOKENS) ||
		document.tokens[0].type != _server_json_object)
	{
		snprintf(why, (size_t)why_size, "not a list");
		return 0;
	}
	master = server_json_member(&document, 0, "master");
	ok = server_json_get_number(&document, server_json_member(&document, 0, "protocol"), &protocol) && protocol == 1 &&
		server_json_get_string(&document, server_json_member(&document, master, "key_id"), text, sizeof(text)) &&
		!strcmp(text, key_id) &&
		server_json_get_number(&document, server_json_member(&document, master, "time"), &time_value);
	server_json_document_free(&document);
	if (!ok)
	{
		snprintf(why, (size_t)why_size, "not a list of protocol 1 by this master");
		return 0;
	}
	if ((double)now - time_value > FRESH_SECONDS)
	{
		snprintf(why, (size_t)why_size, "a list %.0f seconds old", (double)now - time_value);
		return 0;
	}
	if (time_value - (double)now > FRESH_SECONDS)
	{
		snprintf(why, (size_t)why_size, "a list dated %.0f seconds ahead (this machine's clock is wrong, or the "
			"master's)", time_value - (double)now);
		return 0;
	}
	return 1;
}

/* ---------- fetching */

struct fetched
{
	int status;
	char *body;
	unsigned long size;
	char signature[128];
	char key[32];
};

/* whether the text begins with the name, in either case */
static int begins_with_name(const char *text, const char *name, size_t length)
{
	size_t index;

	for (index = 0; index < length; index++)
	{
		if (!text[index] || tolower((unsigned char)text[index]) != tolower((unsigned char)name[index]))
			return 0;
	}
	return 1;
}

/* a response's header's value, from its "Name: value" lines */
static void header_value(const char *headers, const char *name, char *value, int size)
{
	size_t length = strlen(name);
	const char *line = headers;

	value[0] = 0;
	while (line && *line)
	{
		if (begins_with_name(line, name, length) && line[length] == ':')
		{
			const char *start = line + length + 1;
			int count;

			while (*start == ' ' || *start == '\t')
				start++;
			count = (int)strcspn(start, "\r\n");
			snprintf(value, (size_t)size, "%.*s", count, start);
			return;
		}
		line = strchr(line, '\n');
		if (line)
			line++;
	}
}

#if defined(HALO_WEB)
/* the page's fetch (port/web/src/web_library.js's web_js_fetch), which this
thread waits for: its answer in the slot. The page claims the slot for its
answer by changing its id to -id (Atomics.compareExchange), writes it, then
sets done; the thread gives up on it by changing the id to 0 the same way,
so an answer and a give-up never cross */
struct web_fetch_slot
{
	int id;
	int done;
	int status;
	char *body;
	int size;
	char signature[128];
	char key[32];
};
/* (url: a copy the page frees once read) */
void web_js_fetch(char *url, struct web_fetch_slot *slot, int id);

static int fetch(const char *url, struct fetched *fetched)
{
	/* (one slot, which the page writes into: one fetch at a time, a thread
	of an earlier opening of the browser's included) */
	static pthread_mutex_t one_at_a_time = PTHREAD_MUTEX_INITIALIZER;
	static struct web_fetch_slot slot;
	static int next_id;
	char *url_copy = strdup(url);
	int id, waited, fetched_one;

	if (!url_copy)
		return 0;
	pthread_mutex_lock(&one_at_a_time);
	/* (no answer of an earlier fetch writes to it now: it was taken, or
	given up on before the page claimed it) */
	memset(&slot, 0, sizeof(slot));
	if (++next_id <= 0)
		next_id = 1;
	id = next_id;
	__atomic_store_n(&slot.id, id, __ATOMIC_SEQ_CST);
	web_js_fetch(url_copy, &slot, id);
	for (waited = 0; !__atomic_load_n(&slot.done, __ATOMIC_ACQUIRE) && waited < 500; waited++)
	{
		struct timespec pause = { 0, 20 * 1000 * 1000 };

		nanosleep(&pause, NULL);
	}
	if (!__atomic_load_n(&slot.done, __ATOMIC_ACQUIRE))
	{
		int expected = id;

		/* (given up on: an answer coming later finds another id, and the
		page drops it) */
		if (__atomic_compare_exchange_n(&slot.id, &expected, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
			pthread_mutex_unlock(&one_at_a_time);
			return 0;
		}
		/* (the page claimed it as the time ran out: its answer is being
		written, at once) */
		while (!__atomic_load_n(&slot.done, __ATOMIC_ACQUIRE))
		{
			struct timespec pause = { 0, 1000 * 1000 };

			nanosleep(&pause, NULL);
		}
	}
	fetched->status = slot.status;
	fetched->body = slot.body;
	fetched->size = (unsigned long)slot.size;
	snprintf(fetched->signature, sizeof(fetched->signature), "%s", slot.signature);
	snprintf(fetched->key, sizeof(fetched->key), "%s", slot.key);
	fetched_one = slot.status > 0;
	pthread_mutex_unlock(&one_at_a_time);
	return fetched_one;
}
#elif defined(HALO_ANDROID)
/* the app's host (port/android/host/host_https.c) */
int host_https_get(const char *url, int *status, char *headers, int headers_size, char *body, int body_capacity,
	int *body_size, char *error, int error_size);

static int fetch(const char *url, struct fetched *fetched)
{
	enum { HEADERS_SIZE = 4096 };
	/* (each call's own: a thread of an earlier opening of the browser's may
	be fetching too) */
	char *headers = malloc(HEADERS_SIZE);
	char error[256];
	int body_size = 0;

	fetched->body = malloc(MAXIMUM_ANSWER + 1);
	if (!headers || !fetched->body || !host_https_get(url, &fetched->status, headers, HEADERS_SIZE, fetched->body,
		MAXIMUM_ANSWER, &body_size, error, sizeof(error)))
	{
		free(headers);
		free(fetched->body);
		fetched->body = NULL;
		return 0;
	}
	fetched->body[body_size] = 0;
	fetched->size = (unsigned long)body_size;
	header_value(headers, "Signature", fetched->signature, sizeof(fetched->signature));
	header_value(headers, "Signature-Key", fetched->key, sizeof(fetched->key));
	free(headers);
	return 1;
}
#else
static int fetch(const char *url, struct fetched *fetched)
{
	struct update_response response;
	char error[256];

	if (!update_https_request("GET", url, NULL, NULL, 0, MAXIMUM_ANSWER, &response, error, sizeof(error)))
	{
		platform_log("server browser: %s: %s", url, error);
		return 0;
	}
	fetched->status = response.status;
	fetched->body = response.body;
	fetched->size = response.body_size;
	header_value(response.headers, "Signature", fetched->signature, sizeof(fetched->signature));
	header_value(response.headers, "Signature-Key", fetched->key, sizeof(fetched->key));
	return 1;
}
#endif

/* the masters to ask (the list thread's): masters.txt's; else, in the web
build, its site's masters.json (the page's list of them, which its Servers
panel reads too); else the official ones */
static void read_masters(void)
{
	char urls[MASTER_LIST_MAXIMUM_MASTERS][MASTER_LIST_URL_SIZE];
	unsigned char keys[MASTER_LIST_MAXIMUM_MASTERS][MASTER_LIST_KEY_SIZE];
	int count = master_list_read_file(config_string("network.masters_file"), urls, keys, MASTER_LIST_MAXIMUM_MASTERS);

#if defined(HALO_WEB)
	if (!count)
	{
		struct fetched fetched;

		memset(&fetched, 0, sizeof(fetched));
		if (fetch("masters.json", &fetched) && fetched.status == 200)
			count = master_list_read_json(fetched.body, fetched.size, urls, keys, MASTER_LIST_MAXIMUM_MASTERS);
		free(fetched.body);
	}
#endif
	if (!count)
		count = master_list_read_defaults(urls, keys, MASTER_LIST_MAXIMUM_MASTERS);
	pthread_mutex_lock(&list.lock);
	memcpy(list.urls, urls, sizeof(urls));
	memcpy(list.keys, keys, sizeof(keys));
	list.count = count;
	pthread_mutex_unlock(&list.lock);
}

/* a list's servers given to the browser: how many were taken */
static int take_list(const char *body, unsigned long size)
{
	struct server_json_document document;
	int servers, index, taken = 0;

	if (!server_json_parse_tokens(&document, body, (int)size, 16, SERVER_JSON_MAXIMUM_LIST_TOKENS))
		return 0;
	servers = server_json_member(&document, 0, "servers");
	for (index = 0; index < MAXIMUM_ENTRIES && server_json_element(&document, servers, index) >= 0; index++)
	{
		int entry = server_json_element(&document, servers, index);
		char listing_text[600];
		unsigned char listing[MAXIMUM_LISTING];
		char publisher[64] = "", region[32] = "";
		double version = 0;
		int listing_size, character, dedicated;

		/* (an entry that is not one is skipped, not the list) */
		if (!server_json_get_string(&document, server_json_member(&document, entry, "listing"), listing_text,
			sizeof(listing_text)))
		{
			continue;
		}
		listing_size = server_base64_decode(listing_text, listing, sizeof(listing));
		if (listing_size <= 0)
			continue;
		server_json_get_string(&document, server_json_member(&document, server_json_member(&document, entry, "publisher"),
			"name"), publisher, sizeof(publisher));
		server_json_get_string(&document, server_json_member(&document, entry, "region"), region, sizeof(region));
		server_json_get_number(&document, server_json_member(&document, entry, "network_version"), &version);
		/* (a dedicated server's, as the entry says: a player's public game,
		published with a publisher key, is not) */
		dedicated = 0;
		server_json_get_boolean(&document, server_json_member(&document, entry, "dedicated"), &dedicated);
		/* (the publisher's name and the region kept to printable ASCII, cut
		to their sizes: shown in the ticker) */
		for (character = 0; publisher[character]; character++)
		{
			if ((unsigned char)publisher[character] < 0x20 || (unsigned char)publisher[character] >= 0x7F)
				publisher[character] = '?';
		}
		publisher[P2P_LISTING_PUBLISHER_SIZE] = 0;
		for (character = 0; region[character]; character++)
		{
			if (!(isalnum((unsigned char)region[character]) || region[character] == '-'))
				region[character] = '?';
		}
		region[P2P_LISTING_REGION_SIZE] = 0;
		taken += p2p_lobby_take_master_listing(listing, listing_size, publisher, region, (int)version, dedicated);
	}
	server_json_document_free(&document);
	return taken;
}

/* under list.lock: whether the thread of that opening goes on */
static int list_current(unsigned long generation)
{
	return list.browsing && list.generation == generation;
}

static void *list_thread_main(void *argument)
{
	unsigned long generation = (unsigned long)(uintptr_t)argument;
	char urls[MASTER_LIST_MAXIMUM_MASTERS][MASTER_LIST_URL_SIZE];
	unsigned char keys[MASTER_LIST_MAXIMUM_MASTERS][MASTER_LIST_KEY_SIZE];
	int count;

	read_masters();
	pthread_mutex_lock(&list.lock);
	while (list_current(generation))
	{
		int index, answered = 0;

		list.refresh = 0;
		list.asked = 0;
		/* (the masters as read, this thread's own copy) */
		memcpy(urls, list.urls, sizeof(urls));
		memcpy(keys, list.keys, sizeof(keys));
		count = list.count;
		pthread_mutex_unlock(&list.lock);
		p2p_lobby_masters_heard();
		for (index = 0; index < count; index++)
		{
			struct fetched fetched;
			char url[MASTER_LIST_URL_SIZE + 64];
			char why[128];

			memset(&fetched, 0, sizeof(fetched));
			snprintf(url, sizeof(url), "%s/v1/servers", urls[index]);
			if (!fetch(url, &fetched))
			{
				platform_log("server browser: no answer from %s", urls[index]);
			}
			else if (fetched.status != 200)
			{
				platform_log("server browser: %s answered %d", urls[index], fetched.status);
			}
			else if (!master_list_verify(fetched.body, fetched.size, fetched.signature, fetched.key, keys[index],
				(unsigned long)time(NULL), why, sizeof(why)))
			{
				platform_log("server browser: %s's list refused: %s", urls[index], why);
			}
			else
			{
				int taken = take_list(fetched.body, fetched.size);

				answered++;
				platform_log("server browser: %s lists %d server%s", urls[index], taken, taken == 1 ? "" : "s");
			}
			free(fetched.body);
			pthread_mutex_lock(&list.lock);
			list.asked++;
			list.answered = answered;
			if (!list_current(generation))
			{
				pthread_mutex_unlock(&list.lock);
				break;
			}
			pthread_mutex_unlock(&list.lock);
		}
		pthread_mutex_lock(&list.lock);
		list.answered = answered;
		if (list_current(generation) && !list.refresh)
			wait_seconds(ROUND_SECONDS);
	}
	pthread_mutex_unlock(&list.lock);
	return NULL;
}

void master_list_browse(int on)
{
	if (on && (master_list_policy() == _master_listings_brokers || !config_boolean("network.online")))
		on = 0;
	pthread_mutex_lock(&list.lock);
	if (on && !list.browsing)
	{
		pthread_t thread;

		list.answered = 0;
		list.asked = 0;
		list.browsing = 1;
		list.generation++;
		if (pthread_create(&thread, NULL, list_thread_main, (void *)(uintptr_t)list.generation) == 0)
			pthread_detach(thread);
		else
			list.browsing = 0;
		pthread_mutex_unlock(&list.lock);
		return;
	}
	if (!on && list.browsing)
	{
		list.browsing = 0;
		pthread_cond_broadcast(&list.wake);
	}
	pthread_mutex_unlock(&list.lock);
}

void master_list_refresh(void)
{
	pthread_mutex_lock(&list.lock);
	list.refresh = 1;
	pthread_cond_broadcast(&list.wake);
	pthread_mutex_unlock(&list.lock);
}

void master_list_counts(int *count, int *answered, int *asked)
{
	pthread_mutex_lock(&list.lock);
	*count = list.count;
	*answered = list.answered;
	*asked = list.asked;
	pthread_mutex_unlock(&list.lock);
}
