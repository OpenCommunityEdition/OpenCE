/*
SERVER_NETLIST.C

A dedicated server's netlists (server_netlist.h).
*/

#include "platform.h"
#include "port_config.h"
#include "server_netlist.h"
#include "server_mode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- the format */

static unsigned long get_u32(const unsigned char *bytes)
{
	return (unsigned long)bytes[0] << 24 | (unsigned long)bytes[1] << 16 | (unsigned long)bytes[2] << 8 | bytes[3];
}

static unsigned long long get_u64(const unsigned char *bytes)
{
	return (unsigned long long)get_u32(bytes) << 32 | get_u32(bytes + 4);
}

int server_netlist_read(const unsigned char *body, unsigned long size, unsigned long long *generated,
	unsigned long long *serial, unsigned long *count, char *why, int why_size)
{
	unsigned long ranges, index;

	why[0] = 0;
	if (size < SERVER_NETLIST_HEADER_SIZE || memcmp(body, "OPCENET1", 8))
	{
		snprintf(why, (size_t)why_size, "not a netlist");
		return 0;
	}
	if (body[8] != 1)
	{
		snprintf(why, (size_t)why_size, "format %d, not 1", body[8]);
		return 0;
	}
	if (body[9] || body[10] || body[11])
	{
		snprintf(why, (size_t)why_size, "a reserved byte is set");
		return 0;
	}
	ranges = get_u32(body + 28);
	if (ranges > SERVER_NETLIST_MAXIMUM_RANGES || size != SERVER_NETLIST_HEADER_SIZE + 8 * (unsigned long long)ranges)
	{
		snprintf(why, (size_t)why_size, "%lu bytes for %lu ranges", size, ranges);
		return 0;
	}
	for (index = 0; index < ranges; index++)
	{
		const unsigned char *range = body + SERVER_NETLIST_HEADER_SIZE + 8 * index;
		unsigned long first = get_u32(range), last = get_u32(range + 4);

		if (first > last)
		{
			snprintf(why, (size_t)why_size, "range %lu ends before it begins", index);
			return 0;
		}
		/* (the last of the one before at least 2 below its first: neither
		overlapping nor touching) */
		if (index && (first == 0 || get_u32(range - 4) >= first - 1))
		{
			snprintf(why, (size_t)why_size, "range %lu overlaps or touches the one before it", index);
			return 0;
		}
	}
	*generated = get_u64(body + 12);
	*serial = get_u64(body + 20);
	*count = ranges;
	return 1;
}

int server_netlist_ranges_contain(const unsigned char *ranges, unsigned long count, unsigned long address)
{
	unsigned long low = 0, high = count;

	/* (the first range whose last is the address or after it) */
	while (low < high)
	{
		unsigned long middle = low + (high - low) / 2;

		if (get_u32(ranges + 8 * middle + 4) < address)
			low = middle + 1;
		else
			high = middle;
	}
	return low < count && get_u32(ranges + 8 * low) <= address;
}

/* an IPv4 address's dotted text at text: 1 with its value (host byte order),
and where it ended */
static int read_address(const char *text, unsigned long *address, const char **end)
{
	unsigned long value = 0;
	int part;

	for (part = 0; part < 4; part++)
	{
		unsigned long number = 0;
		int digits = 0;

		while (*text >= '0' && *text <= '9' && digits < 4)
		{
			number = number * 10 + (unsigned long)(*text++ - '0');
			digits++;
		}
		if (!digits || number > 255)
			return 0;
		value = value << 8 | number;
		if (part < 3 && *text++ != '.')
			return 0;
	}
	*address = value;
	*end = text;
	return 1;
}

static int compare_pairs(const void *a, const void *b)
{
	unsigned long first = *(const unsigned long *)a, second = *(const unsigned long *)b;

	return first < second ? -1 : first > second;
}

unsigned long server_netlist_parse_ranges(const char *text, unsigned long *pairs, unsigned long maximum, int *bad)
{
	unsigned long count = 0, merged = 0, index;

	*bad = 0;
	while (text && *text)
	{
		const char *line_end = text + strcspn(text, "\n");
		const char *cursor = text;
		unsigned long first, last;
		int good = 0;

		while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r')
			cursor++;
		if (cursor < line_end && *cursor != '#')
		{
			const char *after;

			if (read_address(cursor, &first, &after))
			{
				last = first;
				good = 1;
				if (*after == '/')
				{
					int bits = 0, digits = 0;

					after++;
					while (*after >= '0' && *after <= '9' && digits < 3)
					{
						bits = bits * 10 + (*after++ - '0');
						digits++;
					}
					if (!digits || bits > 32)
					{
						good = 0;
					}
					else
					{
						unsigned long mask = bits ? 0xFFFFFFFFUL << (32 - bits) & 0xFFFFFFFFUL : 0;

						first &= mask;
						last = first | (~mask & 0xFFFFFFFFUL);
					}
				}
				else if (*after == '-')
				{
					after++;
					if (!read_address(after, &last, &after) || last < first)
						good = 0;
				}
				while (good && (*after == ' ' || *after == '\t' || *after == '\r'))
					after++;
				if (good && after < line_end && *after != '#')
					good = 0;
			}
			if (good && count < maximum)
			{
				pairs[2 * count] = first;
				pairs[2 * count + 1] = last;
				count++;
			}
			else if (!good)
			{
				(*bad)++;
			}
		}
		text = *line_end ? line_end + 1 : line_end;
	}
	/* (in order of their firsts, those that overlap or touch made one) */
	qsort(pairs, count, 2 * sizeof(unsigned long), compare_pairs);
	for (index = 0; index < count; index++)
	{
		unsigned long first = pairs[2 * index], last = pairs[2 * index + 1];

		if (merged)
		{
			unsigned long *previous_last = &pairs[2 * (merged - 1) + 1];

			if (first <= *previous_last || first - 1 == *previous_last)
			{
				if (last > *previous_last)
					*previous_last = last;
				continue;
			}
		}
		pairs[2 * merged] = first;
		pairs[2 * merged + 1] = last;
		merged++;
	}
	return merged;
}

#if HALO_SERVER_BUILD

#include "p2p_internal.h"
#include "server_json.h"
#include "server_log.h"
#include "server_master.h"
#include "update.h"

#include <SDL3/SDL.h>

#include <ctype.h>
#include <time.h>

enum
{
	FETCH_SECONDS = 60 * 60,
	/* a master that did not answer, or answered wrong, asked again sooner */
	RETRY_SECONDS = 10 * 60,
	CLOCK_SLACK_SECONDS = 300,
	MAXIMUM_BODY = SERVER_NETLIST_HEADER_SIZE + 8 * SERVER_NETLIST_MAXIMUM_RANGES,
	/* the most ranges a local file gives */
	MAXIMUM_LOCAL_RANGES = 65536,
};

/* a list held: a master's, or server.vpn_list's file's */
struct held_list
{
	char url[256];
	unsigned char key[SERVER_MASTER_PUBLIC_KEY_SIZE];
	unsigned char key_id[SERVER_MASTER_KEY_ID_SIZE];
	unsigned char *body;
	unsigned long size;
	unsigned long count;
	unsigned long long generated;
	unsigned long long serial;
	/* when it was fetched (0: read from disk this run), when it is asked
	next, and why the last fetch failed */
	unsigned long long fetched;
	unsigned long long next_fetch;
	char etag[24];
	char error[160];
};

/* the refusals' reasons, in order (/api/vpn's refused{}) */
static const char *const refusal_reasons[] =
{
	"vpn_link_required", "vpn_blocked", "vpn_link_invalid", "vpn_link_too_new", "vpn_link_in_use",
};

static struct
{
	SDL_Mutex *lock;
	SDL_Condition *wake;
	SDL_Thread *thread;
	int started;
	int fetch_now;
	struct held_list masters[SERVER_MASTER_MAXIMUM_MASTERS];
	int master_count;
	struct held_list file;
	char file_name[1024];
	/* vpn_allow.txt's and vpn_block.txt's merged ranges (first, last pairs) */
	unsigned long *allowed;
	unsigned long allowed_count;
	unsigned long *blocked;
	unsigned long blocked_count;
	unsigned long refused[sizeof(refusal_reasons) / sizeof(refusal_reasons[0])];
} netlist;

static unsigned long long now_seconds(void)
{
	return (unsigned long long)time(NULL);
}

/* a file beside bans.txt (in paths.logs, else the data root) */
static void beside_bans(const char *name, char *path, int size)
{
	char bans[1024];
	char *slash;

	platform_translate_path("d:\\bans.txt", bans, sizeof(bans));
	slash = strrchr(bans, '/');
#ifdef HALO_WINDOWS
	if (!slash || strrchr(bans, '\\') > slash)
		slash = strrchr(bans, '\\');
#endif
	if (slash)
		slash[1] = 0;
	else
		bans[0] = 0;
	snprintf(path, (size_t)size, "%s%s", bans, name);
}

static void cache_path(struct held_list const *list, char *path, int size)
{
	char key_id[2 * SERVER_MASTER_KEY_ID_SIZE + 1];
	char name[64];

	p2p_hex(list->key_id, SERVER_MASTER_KEY_ID_SIZE, key_id);
	snprintf(name, sizeof(name), "netlist-%s.bin", key_id);
	beside_bans(name, path, size);
}

static void etag_of(const unsigned char *body, unsigned long size, char *etag)
{
	unsigned char digest[P2P_SHA256_SIZE];

	p2p_sha256(body, (int)size, digest);
	etag[0] = '"';
	p2p_hex(digest, 8, etag + 1);
	etag[17] = '"';
	etag[18] = 0;
}

/* under the lock: a body taken as the list (it read as one), the one held
before freed */
static void hold(struct held_list *list, unsigned char *body, unsigned long size, unsigned long long generated,
	unsigned long long serial, unsigned long count)
{
	free(list->body);
	list->body = body;
	list->size = size;
	list->generated = generated;
	list->serial = serial;
	list->count = count;
	etag_of(body, size, list->etag);
}

/* the whole file at path, or NULL */
static unsigned char *read_whole(const char *path, unsigned long *size)
{
	size_t read_size = 0;
	char *bytes = config_file_read(path, &read_size);

	*size = (unsigned long)read_size;
	return (unsigned char *)bytes;
}

/* a response's header's value, from its "Name: value" lines, in either case */
static void header_value(const char *headers, const char *name, char *value, int size)
{
	size_t length = strlen(name);
	const char *line = headers;

	value[0] = 0;
	while (line && *line)
	{
		size_t index;

		for (index = 0; index < length && line[index] &&
			tolower((unsigned char)line[index]) == tolower((unsigned char)name[index]); index++)
		{
		}
		if (index == length && line[length] == ':')
		{
			const char *start = line + length + 1;

			while (*start == ' ' || *start == '\t')
				start++;
			snprintf(value, (size_t)size, "%.*s", (int)strcspn(start, "\r\n"), start);
			return;
		}
		line = strchr(line, '\n');
		if (line)
			line++;
	}
}

/* a master's answer checked: its signature by the master's key, its key id,
the body (read, not dated ahead, its serial no lower than that held, if one
is). 1 */
static int answer_checks(struct held_list const *list, int held, const struct update_response *response,
	unsigned long long *generated, unsigned long long *serial, unsigned long *count, char *why, int why_size)
{
	char signature_text[128], key_header[32], key_id[2 * SERVER_MASTER_KEY_ID_SIZE + 1];
	unsigned char signature[P2P_SIGNATURE_SIZE + 4];

	header_value(response->headers, "Signature", signature_text, sizeof(signature_text));
	header_value(response->headers, "Signature-Key", key_header, sizeof(key_header));
	p2p_hex(list->key_id, SERVER_MASTER_KEY_ID_SIZE, key_id);
	if (strncmp(signature_text, "ed25519=", 8) ||
		server_base64_decode(signature_text + 8, signature, sizeof(signature)) != P2P_SIGNATURE_SIZE)
	{
		snprintf(why, (size_t)why_size, "no Signature header");
		return 0;
	}
	if (strcmp(key_header, key_id))
	{
		snprintf(why, (size_t)why_size, "signed by another key (%s, not %s)", key_header, key_id);
		return 0;
	}
	if (!p2p_ed25519_verify(list->key, response->body, (int)response->body_size, signature))
	{
		snprintf(why, (size_t)why_size, "its signature does not check with the master's key");
		return 0;
	}
	if (!server_netlist_read((const unsigned char *)response->body, response->body_size, generated, serial, count, why,
		why_size))
	{
		return 0;
	}
	if (*generated > now_seconds() + CLOCK_SLACK_SECONDS)
	{
		snprintf(why, (size_t)why_size, "dated ahead of this machine's clock");
		return 0;
	}
	if (held && *serial < list->serial)
	{
		snprintf(why, (size_t)why_size, "an older list (serial %llu, %llu held)", *serial, list->serial);
		return 0;
	}
	return 1;
}

/* a master asked for its list (the thread's, without the lock) */
static void fetch_one(int index)
{
	struct held_list list;
	struct update_response response;
	char url[400], headers[128] = "", error[256], why[160] = "";
	unsigned long long generated = 0, serial = 0;
	unsigned long count = 0;
	int taken = 0, unchanged = 0, held;

	SDL_LockMutex(netlist.lock);
	/* (a copy of what is held, but its body: the url, key, serial and tag) */
	list = netlist.masters[index];
	list.body = NULL;
	held = netlist.masters[index].body != NULL;
	if (held)
		snprintf(headers, sizeof(headers), "If-None-Match: %s\r\n", netlist.masters[index].etag);
	SDL_UnlockMutex(netlist.lock);
	snprintf(url, sizeof(url), "%s/v1/netlist", list.url);
	memset(&response, 0, sizeof(response));
	if (!update_https_request("GET", url, headers[0] ? headers : NULL, NULL, 0, MAXIMUM_BODY, &response, error,
		sizeof(error)))
	{
		snprintf(why, sizeof(why), "no answer: %s", error);
	}
	else if (response.status == 304)
	{
		unchanged = 1;
	}
	else if (response.status != 200)
	{
		snprintf(why, sizeof(why), "answered %d", response.status);
	}
	else if (answer_checks(&list, held, &response, &generated, &serial, &count, why, sizeof(why)))
	{
		char path[1100], temporary[1200];
		FILE *file;

		taken = 1;
		/* (kept for the next run: written aside, then put in place) */
		cache_path(&list, path, sizeof(path));
		snprintf(temporary, sizeof(temporary), "%s.part", path);
		file = fopen(temporary, "wb");
		if (file && fwrite(response.body, 1, response.body_size, file) == response.body_size && fclose(file) == 0)
		{
			remove(path);
			if (rename(temporary, path) != 0)
				server_log(_server_log_warning, "vpn", "cannot write %s", path);
		}
		else
		{
			if (file)
				fclose(file);
			remove(temporary);
			server_log(_server_log_warning, "vpn", "cannot write %s", temporary);
		}
	}
	SDL_LockMutex(netlist.lock);
	{
		struct held_list *held = &netlist.masters[index];
		unsigned long long now = now_seconds();
		int logged_before = held->error[0] != 0;

		if (taken)
		{
			int first = !held->body;

			hold(held, (unsigned char *)response.body, response.body_size, generated, serial, count);
			response.body = NULL;
			held->fetched = now;
			held->error[0] = 0;
			held->next_fetch = now + FETCH_SECONDS;
			server_log(_server_log_info, "vpn", "%s's netlist %s: serial %llu, %lu ranges", held->url,
				first ? "taken" : "updated", serial, count);
		}
		else if (unchanged)
		{
			held->fetched = now;
			held->error[0] = 0;
			held->next_fetch = now + FETCH_SECONDS;
		}
		else
		{
			snprintf(held->error, sizeof(held->error), "%s", why);
			held->next_fetch = now + RETRY_SECONDS;
			if (!logged_before)
				server_log(_server_log_warning, "vpn", "%s's netlist: %s%s", held->url, why,
					held->body ? " (the one held is kept)" : "");
		}
	}
	SDL_UnlockMutex(netlist.lock);
	free(response.body);
}

static int SDLCALL netlist_thread_main(void *unused)
{
	(void)unused;
	SDL_LockMutex(netlist.lock);
	for (;;)
	{
		unsigned long long now = now_seconds();
		int index, due = -1;

		for (index = 0; index < netlist.master_count && due < 0; index++)
		{
			if (netlist.fetch_now || now >= netlist.masters[index].next_fetch)
				due = index;
		}
		if (due < 0)
		{
			netlist.fetch_now = 0;
			SDL_WaitConditionTimeout(netlist.wake, netlist.lock, 60 * 1000);
			continue;
		}
		/* (asked now: the next one when its time comes, or at once after a
		fetch_now, which asks each once) */
		netlist.masters[due].next_fetch = now + RETRY_SECONDS;
		if (due == netlist.master_count - 1)
			netlist.fetch_now = 0;
		SDL_UnlockMutex(netlist.lock);
		fetch_one(due);
		SDL_LockMutex(netlist.lock);
	}
	return 0;
}

/* under the lock: a local file's ranges read again into pairs */
static void read_ranges_file(const char *name, unsigned long **pairs, unsigned long *count)
{
	char path[1100];
	size_t size;
	char *text;
	int bad = 0;

	free(*pairs);
	*pairs = NULL;
	*count = 0;
	beside_bans(name, path, sizeof(path));
	text = config_file_read(path, &size);
	if (!text)
		return;
	*pairs = malloc(2 * sizeof(unsigned long) * MAXIMUM_LOCAL_RANGES);
	if (*pairs)
		*count = server_netlist_parse_ranges(text, *pairs, MAXIMUM_LOCAL_RANGES, &bad);
	free(text);
	if (bad)
		server_log(_server_log_warning, "vpn", "%s: %d line%s that %s no range (a.b.c.d, a.b.c.d/n, a.b.c.d-e.f.g.h)",
			path, bad, bad == 1 ? "" : "s", bad == 1 ? "is" : "are");
	server_log(_server_log_info, "vpn", "%s: %lu range%s", path, *count, *count == 1 ? "" : "s");
}

/* under the lock: server.vpn_list's file read again (a full path, else
beside config.toml) */
static void read_list_file(void)
{
	const char *setting = config_string("server.vpn_list");
	char why[160];
	unsigned long size = 0, count = 0;
	unsigned long long generated = 0, serial = 0;
	unsigned char *body;

	free(netlist.file.body);
	memset(&netlist.file, 0, sizeof(netlist.file));
	netlist.file_name[0] = 0;
	if (!setting[0])
		return;
	if (setting[0] == '/' || (setting[0] && setting[1] == ':'))
	{
		snprintf(netlist.file_name, sizeof(netlist.file_name), "%s", setting);
	}
	else
	{
		char folder[1024];

		config_folder(folder, sizeof(folder));
		snprintf(netlist.file_name, sizeof(netlist.file_name), "%s%s", folder, setting);
	}
	snprintf(netlist.file.url, sizeof(netlist.file.url), "%s", netlist.file_name);
	body = read_whole(netlist.file_name, &size);
	if (!body)
	{
		snprintf(netlist.file.error, sizeof(netlist.file.error), "cannot read it");
		server_log(_server_log_warning, "vpn", "server.vpn_list: cannot read %s", netlist.file_name);
		return;
	}
	if (!server_netlist_read(body, size, &generated, &serial, &count, why, sizeof(why)))
	{
		snprintf(netlist.file.error, sizeof(netlist.file.error), "%s", why);
		server_log(_server_log_warning, "vpn", "server.vpn_list: %s: %s", netlist.file_name, why);
		free(body);
		return;
	}
	hold(&netlist.file, body, size, generated, serial, count);
	server_log(_server_log_info, "vpn", "server.vpn_list: %s: %lu ranges", netlist.file_name, count);
}

void server_netlist_start(void)
{
	char urls[SERVER_MASTER_MAXIMUM_MASTERS][256];
	unsigned char keys[SERVER_MASTER_MAXIMUM_MASTERS][SERVER_MASTER_PUBLIC_KEY_SIZE];
	int index, count;

	if (!halo_server_mode() || netlist.started)
		return;
	netlist.started = 1;
	netlist.lock = SDL_CreateMutex();
	netlist.wake = SDL_CreateCondition();
	count = server_master_publishing_masters(urls, keys, SERVER_MASTER_MAXIMUM_MASTERS);
	SDL_LockMutex(netlist.lock);
	netlist.master_count = count;
	for (index = 0; index < count; index++)
	{
		struct held_list *list = &netlist.masters[index];
		unsigned char digest[P2P_SHA256_SIZE];
		char path[1100], why[160];
		unsigned long size = 0, ranges = 0;
		unsigned long long generated = 0, serial = 0;
		unsigned char *body;

		memset(list, 0, sizeof(*list));
		snprintf(list->url, sizeof(list->url), "%s", urls[index]);
		memcpy(list->key, keys[index], SERVER_MASTER_PUBLIC_KEY_SIZE);
		p2p_sha256(keys[index], SERVER_MASTER_PUBLIC_KEY_SIZE, digest);
		memcpy(list->key_id, digest, SERVER_MASTER_KEY_ID_SIZE);
		/* (the list kept by the last run: taken as it was, its signature
		checked when it was fetched; a file that does not read is not) */
		cache_path(list, path, sizeof(path));
		body = read_whole(path, &size);
		if (body && server_netlist_read(body, size, &generated, &serial, &ranges, why, sizeof(why)))
		{
			hold(list, body, size, generated, serial, ranges);
			server_log(_server_log_info, "vpn", "%s's netlist held from the last run: serial %llu, %lu ranges",
				list->url, serial, ranges);
		}
		else
		{
			free(body);
		}
	}
	read_list_file();
	read_ranges_file("vpn_allow.txt", &netlist.allowed, &netlist.allowed_count);
	read_ranges_file("vpn_block.txt", &netlist.blocked, &netlist.blocked_count);
	SDL_UnlockMutex(netlist.lock);
	if (!count && !netlist.file.body)
	{
		server_log(_server_log_info, "vpn", "no netlist: the server publishes to no master server and "
			"server.vpn_list names none (server.vpn_fail is %s)", config_string("server.vpn_fail"));
	}
	if (count)
	{
		netlist.thread = SDL_CreateThread(netlist_thread_main, "server netlists", NULL);
		if (netlist.thread)
			SDL_DetachThread(netlist.thread);
	}
}

void server_netlist_reload(void)
{
	if (!netlist.started)
		return;
	SDL_LockMutex(netlist.lock);
	read_list_file();
	read_ranges_file("vpn_allow.txt", &netlist.allowed, &netlist.allowed_count);
	read_ranges_file("vpn_block.txt", &netlist.blocked, &netlist.blocked_count);
	SDL_UnlockMutex(netlist.lock);
}

void server_netlist_fetch_now(void)
{
	if (!netlist.started)
		return;
	SDL_LockMutex(netlist.lock);
	netlist.fetch_now = 1;
	SDL_BroadcastCondition(netlist.wake);
	SDL_UnlockMutex(netlist.lock);
}

/* under the lock */
static int any_held(void)
{
	int index;

	if (netlist.file.body)
		return 1;
	for (index = 0; index < netlist.master_count; index++)
	{
		if (netlist.masters[index].body)
			return 1;
	}
	return 0;
}

int server_netlist_held(void)
{
	int held;

	if (!netlist.started)
		return 0;
	SDL_LockMutex(netlist.lock);
	held = any_held();
	SDL_UnlockMutex(netlist.lock);
	return held;
}

/* whether pairs (merged firsts and lasts) hold the address */
static int pairs_contain(const unsigned long *pairs, unsigned long count, unsigned long address)
{
	unsigned long low = 0, high = count;

	while (low < high)
	{
		unsigned long middle = low + (high - low) / 2;

		if (pairs[2 * middle + 1] < address)
			low = middle + 1;
		else
			high = middle;
	}
	return low < count && pairs[2 * low] <= address;
}

int server_netlist_listed(unsigned long address, char *by, int by_size)
{
	int listed = 0, index;
	const char *source = "";

	if (!netlist.started)
	{
		by[0] = 0;
		return 0;
	}
	SDL_LockMutex(netlist.lock);
	if (pairs_contain(netlist.allowed, netlist.allowed_count, address))
	{
		source = "vpn_allow.txt";
	}
	else if (pairs_contain(netlist.blocked, netlist.blocked_count, address))
	{
		source = "vpn_block.txt";
		listed = 1;
	}
	else
	{
		if (netlist.file.body &&
			server_netlist_ranges_contain(netlist.file.body + SERVER_NETLIST_HEADER_SIZE, netlist.file.count, address))
		{
			listed = 1;
		}
		for (index = 0; index < netlist.master_count && !listed; index++)
		{
			struct held_list const *list = &netlist.masters[index];

			listed = list->body &&
				server_netlist_ranges_contain(list->body + SERVER_NETLIST_HEADER_SIZE, list->count, address);
		}
		if (listed)
			source = "netlist";
	}
	SDL_UnlockMutex(netlist.lock);
	snprintf(by, (size_t)by_size, "%s", source);
	return listed;
}

void server_netlist_count_refusal(const char *reason)
{
	int index;

	if (!netlist.started)
		return;
	SDL_LockMutex(netlist.lock);
	for (index = 0; index < (int)(sizeof(refusal_reasons) / sizeof(refusal_reasons[0])); index++)
	{
		if (!strcmp(reason, refusal_reasons[index]))
			netlist.refused[index]++;
	}
	SDL_UnlockMutex(netlist.lock);
}

static void list_status(struct server_json *json, struct held_list const *list)
{
	char key_id[2 * SERVER_MASTER_KEY_ID_SIZE + 1];

	server_json_object_begin(json, NULL);
	server_json_string(json, "master", list->url);
	p2p_hex(list->key_id, SERVER_MASTER_KEY_ID_SIZE, key_id);
	server_json_string(json, "key_id", key_id);
	if (list->body)
	{
		server_json_integer(json, "serial", (long long)list->serial);
		server_json_integer(json, "generated", (long long)list->generated);
		server_json_integer(json, "ranges", (long long)list->count);
	}
	else
	{
		server_json_null(json, "serial");
		server_json_null(json, "generated");
		server_json_integer(json, "ranges", 0);
	}
	if (list->fetched)
		server_json_integer(json, "fetched", (long long)list->fetched);
	else
		server_json_null(json, "fetched");
	server_json_string(json, "error", list->error);
	server_json_object_end(json);
}

void server_netlist_status(struct server_json *json)
{
	int index;

	server_json_string(json, "policy", config_string("server.vpn_policy"));
	server_json_string(json, "fail", config_string("server.vpn_fail"));
	if (!netlist.started)
	{
		server_json_array_begin(json, "lists");
		server_json_array_end(json);
		server_json_string(json, "file", config_string("server.vpn_list"));
		server_json_integer(json, "allow_ranges", 0);
		server_json_integer(json, "block_ranges", 0);
		server_json_boolean(json, "held", 0);
		server_json_object_begin(json, "refused");
		for (index = 0; index < (int)(sizeof(refusal_reasons) / sizeof(refusal_reasons[0])); index++)
			server_json_integer(json, refusal_reasons[index], 0);
		server_json_object_end(json);
		return;
	}
	SDL_LockMutex(netlist.lock);
	server_json_array_begin(json, "lists");
	for (index = 0; index < netlist.master_count; index++)
		list_status(json, &netlist.masters[index]);
	server_json_array_end(json);
	server_json_string(json, "file", netlist.file_name);
	if (netlist.file_name[0])
	{
		server_json_object_begin(json, "file_list");
		server_json_integer(json, "ranges", netlist.file.body ? (long long)netlist.file.count : 0);
		server_json_string(json, "error", netlist.file.error);
		server_json_object_end(json);
	}
	server_json_integer(json, "allow_ranges", (long long)netlist.allowed_count);
	server_json_integer(json, "block_ranges", (long long)netlist.blocked_count);
	server_json_boolean(json, "held", any_held());
	server_json_object_begin(json, "refused");
	for (index = 0; index < (int)(sizeof(refusal_reasons) / sizeof(refusal_reasons[0])); index++)
		server_json_integer(json, refusal_reasons[index], (long long)netlist.refused[index]);
	server_json_object_end(json);
	SDL_UnlockMutex(netlist.lock);
}

void server_netlist_describe(char *text, int size)
{
	int length, index;
	unsigned long long now = now_seconds();

	length = snprintf(text, (size_t)size, "VPN policy %s (server.vpn_fail %s)", config_string("server.vpn_policy"),
		config_string("server.vpn_fail"));
	if (!netlist.started)
		return;
	SDL_LockMutex(netlist.lock);
	for (index = 0; index < netlist.master_count && length < size; index++)
	{
		struct held_list const *list = &netlist.masters[index];

		if (list->body)
			length += snprintf(text + length, (size_t)(size - length), "\n%s: serial %llu, %lu ranges, %llu hours old%s%s",
				list->url, list->serial, list->count, now > list->generated ? (now - list->generated) / 3600 : 0,
				list->error[0] ? "; last fetch: " : "", list->error);
		else
			length += snprintf(text + length, (size_t)(size - length), "\n%s: no list%s%s", list->url,
				list->error[0] ? ": " : "", list->error);
	}
	if (netlist.file_name[0] && length < size)
		length += snprintf(text + length, (size_t)(size - length), "\nserver.vpn_list %s: %s", netlist.file_name,
			netlist.file.body ? "held" : netlist.file.error);
	if (length < size)
		length += snprintf(text + length, (size_t)(size - length),
			"\nvpn_allow.txt: %lu ranges, vpn_block.txt: %lu ranges\nrefused: %lu link required, %lu blocked, "
			"%lu link invalid, %lu account too new, %lu account in use", netlist.allowed_count, netlist.blocked_count,
			netlist.refused[0], netlist.refused[1], netlist.refused[2], netlist.refused[3], netlist.refused[4]);
	if (!any_held() && length < size)
		snprintf(text + length, (size_t)(size - length), "\nno list held: %s",
			!strcmp(config_string("server.vpn_fail"), "closed") ? "joins from the internet are refused" :
				"every machine joins");
	SDL_UnlockMutex(netlist.lock);
}

#else

void server_netlist_start(void)
{
}

void server_netlist_reload(void)
{
}

void server_netlist_fetch_now(void)
{
}

int server_netlist_held(void)
{
	return 0;
}

int server_netlist_listed(unsigned long address, char *by, int by_size)
{
	(void)address;
	if (by_size > 0)
		by[0] = 0;
	return 0;
}

void server_netlist_count_refusal(const char *reason)
{
	(void)reason;
}

void server_netlist_status(struct server_json *json)
{
	(void)json;
}

void server_netlist_describe(char *text, int size)
{
	if (size > 0)
		text[0] = 0;
}

#endif
