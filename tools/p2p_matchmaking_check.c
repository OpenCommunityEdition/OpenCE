/*
P2P_MATCHMAKING_CHECK.C

Matchmaking's records, and machines searching through a stand-in broker.
*/

#include "../port/linux/src/p2p_matchmaking.h"
#include "halo_port_limits.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TEXT_OF_(x) #x
#define TEXT_OF(x) TEXT_OF_(x)
#define HALO_PORT_NETWORK_VERSION_TEXT TEXT_OF(HALO_PORT_NETWORK_VERSION)

pthread_mutex_t p2p_lock = PTHREAD_MUTEX_INITIALIZER;

static unsigned long clock_now = 1800000000UL;

void posix_random_bytes(void *buffer, unsigned long size) { memset(buffer, 0x5a, size); }
unsigned long p2p_now(void) { return clock_now; }
int config_boolean(const char *name) { (void)name; return 1; }
void platform_log(const char *format, ...) { (void)format; }

void p2p_hex(const unsigned char *bytes, int size, char *text)
{
	int index;

	for (index = 0; index < size; index++)
		sprintf(text + index * 2, "%02x", bytes[index]);
}

void p2p_key_hash(const unsigned char *key, unsigned char *hash)
{
	unsigned char digest[P2P_SHA256_SIZE];

	p2p_sha256(key, P2P_KEY_SIZE, digest);
	memcpy(hash, digest, P2P_KEY_HASH_SIZE);
}

static unsigned char run_seed[P2P_SEED_SIZE] = { 0x77, 0x13 }, run_key[P2P_KEY_SIZE];

const unsigned char *p2p_signing_seed(void) { return run_seed; }
const unsigned char *p2p_signing_key(void)
{
	p2p_ed25519_public(run_seed, run_key, NULL);
	return run_key;
}
void p2p_signal_mm_topics(const char topics[][P2P_MM_TOPIC_SIZE], int count) { (void)topics; (void)count; }

int lobby_games_unlocked(struct p2p_listing *games, int maximum_count) { return 0; }
int p2p_lobby_pick(const struct p2p_listing *games, int count, const struct p2p_lobby_wanted *wanted) { return -1; }
int p2p_lobby_pick_unpinged(const struct p2p_listing *games, int count, const struct p2p_lobby_wanted *wanted) { return 0; }

static unsigned char published[MM_MAXIMUM_RECORD_SIZE];
static int published_size;

void p2p_signal_mm_publish(const char *topic, const unsigned char *payload, int size)
{
	(void)topic;
	memcpy(published, payload, (size_t)size);
	published_size = size;
}

static int failures;

#define CHECK(condition, ...) do { if (!(condition)) { failures++; printf("FAIL %s: ", case_name); \
	printf(__VA_ARGS__); printf("\n"); } } while (0)

static const char *case_name;

static void seed_of(int machine, unsigned char *seed)
{
	memset(seed, 0, P2P_SEED_SIZE);
	seed[0] = (unsigned char)(machine + 1);
	seed[1] = (unsigned char)((machine + 1) >> 8);
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

static void check_topics(void)
{
	char topic[P2P_MM_TOPIC_SIZE];

	case_name = "topics";
	mm_topic(P2P_MM_SLAYER, P2P_MM_2V2, topic, sizeof(topic));
	CHECK(!strcmp(topic, "hceu/3/mm/" HALO_PORT_NETWORK_VERSION_TEXT "/slayer/2v2"), "E16: %s", topic);
	mm_topic(P2P_MM_RACE, P2P_MM_FFA_6, topic, sizeof(topic));
	CHECK(!strcmp(topic, "hceu/3/mm/" HALO_PORT_NETWORK_VERSION_TEXT "/race/ffa6"), "%s", topic);
	CHECK(mm_interval(1) == 30 && mm_interval(12) == 30, "up to 12 queues: 3 s");
	CHECK(mm_interval(25) >= 62 && mm_interval(25) <= 63, "E44: 25 queues: about 6.3 s: %d", mm_interval(25));
	CHECK(mm_interval(25) * MM_RECORDS_PER_SECOND >= 25 * 10, "E44: at most 4 a second");
}

static void check_verify_budget(void)
{
	struct p2p_mm_choice choice = { 1u << P2P_MM_SLAYER, P2P_MM_2V2 };
	unsigned char seed[P2P_SEED_SIZE], bytes[MM_MAXIMUM_RECORD_SIZE];
	struct p2p_mm_status status;
	struct timespec before, after;
	int index, size, passes, left;
	double first_pass_ms;

	case_name = "verify_budget";
	p2p_mm_search(&choice);
	/* MAXIMUM_QUEUED (64) are kept: 21 keys of three records, and one more */
	for (index = 0; index < 300; index++)
	{
		struct mm_record record = searching(index / 3 + 10, (unsigned long)(index % 3 + 1));

		seed_of(index / 3 + 10, seed);
		size = mm_record_write(&record, seed, bytes, sizeof(bytes));
		pthread_mutex_lock(&p2p_lock);
		p2p_mm_heard(bytes, size);
		pthread_mutex_unlock(&p2p_lock);
	}
	pthread_mutex_lock(&p2p_lock);
	clock_gettime(CLOCK_MONOTONIC, &before);
	left = p2p_mm_verify_pass(clock_now);
	clock_gettime(CLOCK_MONOTONIC, &after);
	first_pass_ms = (double)(after.tv_sec - before.tv_sec) * 1000.0 + (double)(after.tv_nsec - before.tv_nsec) / 1e6;
	for (passes = 1; left && passes < 100; passes++)
		left = p2p_mm_verify_pass(clock_now);
	pthread_mutex_unlock(&p2p_lock);
	CHECK(first_pass_ms < 4.0, "one pass took %.1f ms", first_pass_ms);
	CHECK(passes > 1 || first_pass_ms < 2.5, "a pass of %.1f ms past the budget stopped early: %d passes", first_pass_ms, passes);
	CHECK(!left, "the queue drains over later passes: %d left", left);
	p2p_mm_get_status(&status);
	CHECK(status.count == 22, "64 kept, all verified into the search: %d searchers counted, 21 and this machine", status.count);
	p2p_mm_cancel();
}

static int contains(const unsigned char *bytes, int size, const unsigned char *part, int part_size)
{
	int at;

	for (at = 0; at + part_size <= size; at++)
		if (!memcmp(bytes + at, part, (size_t)part_size))
			return 1;
	return 0;
}

static void check_hosting(void)
{
	unsigned char host_seed[P2P_SEED_SIZE], member_seed[P2P_SEED_SIZE], other_seed[P2P_SEED_SIZE];
	unsigned char bytes[MM_MAXIMUM_RECORD_SIZE], token[P2P_TOKEN_SIZE], opened[P2P_TOKEN_SIZE];
	struct mm_record record = searching(0, 1), member = searching(1, 1), read_back;
	int size;

	case_name = "hosting";
	seed_of(0, host_seed);
	seed_of(1, member_seed);
	seed_of(2, other_seed);
	memset(token, 0xa5, sizeof(token));
	size = mm_record_write(&member, member_seed, bytes, sizeof(bytes));
	CHECK(mm_record_read(bytes, size, &member), "the member's own record");
	record.kind = MM_KIND_HOSTING;
	record.players = 1;
	record.most_players = 4;
	record.held = 1;
	record.pool_count = 1;
	memcpy(record.pool[0], member.key_hash, P2P_KEY_HASH_SIZE);
	memcpy(record.token, token, P2P_TOKEN_SIZE);
	CHECK(mm_seal_token(host_seed, member.key, token, record.sealed[0]), "sealed");
	size = mm_record_write(&record, host_seed, bytes, sizeof(bytes));
	CHECK(mm_record_read(bytes, size, &read_back), "a held hosting record read back");
	CHECK(read_back.held && read_back.pool_count == 1 && read_back.most_players == 4, "held, one member, 4 most");
	CHECK(mm_open_token(member_seed, read_back.key, read_back.sealed[0], opened) &&
		!memcmp(opened, token, P2P_TOKEN_SIZE), "the member opens the token");
	CHECK(!mm_open_token(other_seed, read_back.key, read_back.sealed[0], opened), "no one else opens it");
	CHECK(!contains(bytes, size, token, P2P_TOKEN_SIZE), "a held game's token is nowhere in clear");
}

static struct mm_unit unit(int machine, int can_host)
{
	struct mm_unit result;
	unsigned char seed[P2P_SEED_SIZE], x25519[P2P_KEY_SIZE];

	memset(&result, 0, sizeof(result));
	seed_of(machine, seed);
	p2p_ed25519_public(seed, result.key, NULL);
	p2p_ed25519_to_x25519(result.key, x25519);
	p2p_key_hash(x25519, result.key_hash);
	result.can_host = can_host;
	return result;
}

static int hash_rank(const struct mm_unit *units, int count, int index)
{
	int other, rank = 0;

	for (other = 0; other < count; other++)
		rank += memcmp(units[other].key_hash, units[index].key_hash, P2P_KEY_HASH_SIZE) < 0;
	return rank;
}

static void check_pool(void)
{
	static struct mm_unit every[MM_MAXIMUM_UNITS + 1];
	struct mm_unit units[8], shuffled[8];
	int pool[8], again[8], count, index, host, host_again, last = 0;

	case_name = "pool";
	for (index = 0; index < 6; index++)
		units[index] = unit(index, 1);
	count = mm_pool(units, 6, 4, pool);
	CHECK(count == 4, "E4: a pool of 4 from 6: %d", count);
	for (index = 0; index < count; index++)
		CHECK(hash_rank(units, 6, pool[index]) == index, "E4: the 4 lowest key hashes, in order: %d at %d",
			pool[index], index);
	for (index = 0; index < 6; index++)
		shuffled[index] = units[5 - index];
	CHECK(mm_pool(shuffled, 6, 4, again) == 4, "shuffled: a pool");
	for (index = 0; index < 4; index++)
		CHECK(!memcmp(units[pool[index]].key, shuffled[again[index]].key, P2P_KEY_SIZE), "shuffled: the same pool at %d", index);
	host = mm_elect(units, pool, count);
	host_again = mm_elect(shuffled, again, 4);
	CHECK(host >= 0 && host_again >= 0 && !memcmp(units[host].key, shuffled[host_again].key, P2P_KEY_SIZE),
		"E4: the same host on every machine");
	CHECK(mm_pool(units, 3, 4, pool) == 0, "3 of 4: no pool");
	for (index = 0; index < MM_MAXIMUM_UNITS; index++)
		every[index].failed = 1;
	every[MM_MAXIMUM_UNITS].can_host = 1;
	CHECK(mm_pool(every, MM_MAXIMUM_UNITS + 1, 1, pool) == 1 && pool[0] == MM_MAXIMUM_UNITS,
		"this machine and a full peers table: every unit is walked");
	/* four strict, then one that can host with the highest key hash: it can take only the last place */
	for (index = 0; index < 5; index++)
		units[index] = unit(index, 0);
	for (index = 0; index < 5; index++)
		if (hash_rank(units, 5, index) == 4)
			last = index;
	units[last].can_host = 1;
	CHECK(mm_pool(units, 5, 4, pool) == 4 && pool[3] == last && mm_elect(units, pool, 4) == last,
		"E38: the host takes the last place and hosts");
	units[last].can_host = 0;
	CHECK(mm_pool(units, 5, 4, pool) == 0, "E38: five strict routers form no pool");
	units[last].can_host = 1;
	units[last].failed = 1;
	CHECK(mm_pool(units, 5, 4, pool) == 0, "E12: the only host failed: no pool until another arrives");
}

static void check_election(void)
{
	struct mm_unit units[4];
	int pool[4] = { 0, 1, 2, 3 }, index, host, winners = 0;

	case_name = "election";
	for (index = 0; index < 3; index++)
		units[index] = unit(index, 1);
	for (index = 0; index < 40; index++)
	{
		units[3] = unit(100 + index, 1);
		host = mm_elect(units, pool, 4);
		if (host >= 0 && host < 3)
			winners |= 1 << host;
	}
	CHECK((winners & (winners - 1)) != 0, "E14: one of the three always wins when it can: %d", winners);
}

struct machine
{
	struct mm_search search;
	int in, hosting, players, most, quit;
};

static struct machine machines[8];
static int machine_count;
static unsigned long now_ms, now_unix;
static int drop_percent;
static int cancel_on_join, cancel_sent, never_joins = -1;
static int fail_join = -1, hosted_while_joining, joins_taken;
static int ignore_first_host, ignore_only = -1, ignored = -1, fail_first_host, failed_host = -1;
static int capture_from = -1, captured_count;
static struct mm_out captured[256], quit_out[MM_MAXIMUM_OUT];

struct in_flight
{
	int to;
	unsigned long at;
	struct mm_out out;
};
static struct in_flight flight[1024];
static int flight_count;

static int subscribed(const struct mm_search *search, const char *topic)
{
	char own[P2P_MM_TOPIC_SIZE];
	int index;

	for (index = 0; search->state != P2P_MM_OFF && index < search->queue_count; index++)
	{
		mm_topic(search->queues[index][0], search->queues[index][1], own, sizeof(own));
		if (!strcmp(own, topic))
			return 1;
	}
	return 0;
}

static void publish(int from, const struct mm_out *out, int count)
{
	int index, to;

	for (index = 0; index < count; index++)
	{
		if (from == capture_from && captured_count < 256)
			captured[captured_count++] = out[index];
		for (to = 0; to < machine_count; to++)
		{
			if (to == from || !subscribed(&machines[to].search, out[index].topic) || rand() % 100 < drop_percent ||
				flight_count == 1024)
			{
				continue;
			}
			flight[flight_count].to = to;
			flight[flight_count].at = now_ms + 100;
			flight[flight_count++].out = out[index];
		}
	}
}

static void deliver(void)
{
	struct mm_record record;
	int index = 0;

	while (index < flight_count)
	{
		if (flight[index].at > now_ms)
		{
			index++;
			continue;
		}
		if (!machines[flight[index].to].quit &&
			mm_record_read(flight[index].out.bytes, flight[index].out.size, &record))
		{
			mm_heard(&machines[flight[index].to].search, &record, now_ms);
		}
		flight[index] = flight[--flight_count];
	}
}

static void replay_captured(int to)
{
	int index;

	for (index = 0; index < captured_count && flight_count < 1024; index++)
	{
		flight[flight_count].to = to;
		flight[flight_count].at = now_ms + 100;
		flight[flight_count++].out = captured[index];
	}
}

static int machine_of_invite(const char *invite)
{
	char text[2 * P2P_KEY_HASH_SIZE + 1];
	int index;

	for (index = 0; index < machine_count; index++)
	{
		p2p_hex(machines[index].search.key_hash, P2P_KEY_HASH_SIZE, text);
		if (!strncmp(invite + strlen("halo://join/"), text, strlen(text)))
			return index;
	}
	return -1;
}

static void game_side(int index)
{
	struct machine *machine = &machines[index];
	struct p2p_mm_decision decision = machine->search.decision;
	unsigned char token[P2P_TOKEN_SIZE];
	int host;

	memset(token, index + 1, sizeof(token));
	machine->search.decision.kind = P2P_MM_DECIDE_NONE;
	if (machine->in >= 0 && (machines[machine->in].quit || !machines[machine->in].hosting))
	{
		machine->in = -1;
		mm_game_lost(&machine->search, now_ms);
	}
	if (decision.kind == P2P_MM_DECIDE_HOST && ignore_first_host && ignored < 0 &&
		(ignore_only < 0 || ignore_only == index))
	{
		ignored = index;
		return;
	}
	if (decision.kind == P2P_MM_DECIDE_HOST && fail_first_host && failed_host < 0)
	{
		failed_host = index;
		publish(index, quit_out, mm_cancel(&machine->search, quit_out, MM_MAXIMUM_OUT));
		return;
	}
	if (decision.kind == P2P_MM_DECIDE_HOST)
	{
		machine->hosting = 1;
		machine->players = 1;
		machine->most = p2p_mm_size_players(decision.size);
	}
	else if (decision.kind == P2P_MM_DECIDE_JOIN)
	{
		joins_taken++;
		machine->hosting = 0;
		if (fail_join == index)
		{
			mm_hosted(&machine->search, token, 1, 4, 0, now_ms);
			hosted_while_joining = machine->search.state == P2P_MM_HOSTING;
			mm_joined(&machine->search, 0, now_ms);
			return;
		}
		if (cancel_on_join && index == 1)
		{
			publish(index, quit_out, mm_cancel(&machine->search, quit_out, MM_MAXIMUM_OUT));
			cancel_sent = 1;
			return;
		}
		if (never_joins == index)
		{
			machine->quit = 1;
			return;
		}
		host = machine_of_invite(decision.invite);
		if (host < 0 || machines[host].quit || !machines[host].hosting ||
			machines[host].players >= machines[host].most)
		{
			mm_joined(&machine->search, 0, now_ms);
			return;
		}
		machine->in = host;
		machines[host].players++;
		mm_joined(&machine->search, 1, now_ms);
	}
	if (machine->hosting)
	{
		mm_hosted(&machine->search, token, machine->players, machine->most, machine->players == machine->most, now_ms);
	}
}

static void run(double seconds)
{
	unsigned long until = now_ms + (unsigned long)(seconds * 1000);
	struct mm_out out[MM_MAXIMUM_OUT];
	int index;

	for (; now_ms < until; now_ms += 100)
	{
		if (now_ms % 1000 == 0)
			now_unix++;
		deliver();
		for (index = 0; index < machine_count; index++)
		{
			if (machines[index].quit)
				continue;
			publish(index, out,
				mm_step(&machines[index].search, now_ms, out, MM_MAXIMUM_OUT));
			game_side(index);
		}
	}
}

static void world(int count)
{
	memset(machines, 0, sizeof(machines));
	machine_count = count;
	for (count = 0; count < 8; count++)
		machines[count].in = -1;
	ignore_first_host = fail_first_host = 0;
	ignored = failed_host = ignore_only = -1;
	now_ms = 1000;
	now_unix = 1800000000UL;
	drop_percent = flight_count = captured_count = cancel_on_join = cancel_sent = 0;
	never_joins = capture_from = fail_join = -1;
	hosted_while_joining = joins_taken = 0;
}

static void start_searching(int index, unsigned playlists, int size, int can_host)
{
	struct p2p_mm_choice choice;
	unsigned char seed[P2P_SEED_SIZE];

	choice.playlists = playlists;
	choice.size = size;
	seed_of(100 + index, seed);
	mm_start(&machines[index].search, seed, &choice, now_ms, now_unix);
	machines[index].search.state = P2P_MM_QUEUED;
	machines[index].search.can_host = can_host;
}

static int find_host(void)
{
	int index;

	for (index = 0; index < machine_count; index++)
		if (machines[index].hosting && !machines[index].quit)
			return index;
	return -1;
}

static int host_count(void)
{
	int index, count = 0;

	for (index = 0; index < machine_count; index++)
		count += machines[index].hosting && !machines[index].quit;
	return count;
}

static int peer_count(const struct mm_search *search)
{
	int index, count = 0;

	for (index = 0; index < MM_MAXIMUM_UNITS; index++)
		count += search->peers[index].used && mm_counted(&search->peers[index]);
	return count;
}

static void check_four(void)
{
	int index, host, in_host = 0;

	case_name = "e4_four";
	world(4);
	for (index = 0; index < 4; index++)
	{
		start_searching(index, 1u << P2P_MM_SLAYER, P2P_MM_2V2, 1);
		run(1.0);
	}
	run(30.0);
	host = find_host();
	CHECK(host >= 0 && host_count() == 1, "E4: one machine hosts: %d", host_count());
	for (index = 0; host >= 0 && index < 4; index++)
		in_host += machines[index].in == host;
	CHECK(host >= 0 && in_host == 3 && machines[host].players == 4, "E4: three join by invite: %d", in_host);
	if (host >= 0)
	{
		struct mm_out out[MM_MAXIMUM_OUT];
		struct mm_record record;
		int count = mm_step(&machines[host].search, now_ms + 60000, out, MM_MAXIMUM_OUT);

		CHECK(count == 1 && mm_record_read(out[0].bytes, out[0].size, &record) && record.under_way &&
			record.interval == 250 && machines[host].search.next_hosting_record == now_ms + 85000,
			"E4: a game under way states 25 s and sends every 25 s");
	}
}

static void check_replay(void)
{
	case_name = "e43_replay";
	world(2);
	start_searching(0, 1u << P2P_MM_SLAYER, P2P_MM_2V2, 1);
	start_searching(1, 1u << P2P_MM_SLAYER, P2P_MM_2V2, 1);
	capture_from = 0;
	run(8.0);
	capture_from = -1;
	publish(0, quit_out, mm_cancel(&machines[0].search, quit_out, MM_MAXIMUM_OUT));
	machines[0].quit = 1;
	run(1.0);
	CHECK(peer_count(&machines[1].search) == 0, "a cancel drops a searcher at once");
	replay_captured(1);
	run(7.0);
	CHECK(peer_count(&machines[1].search) == 0, "E43: a replayed searcher is never counted again");
}

static void check_return(void)
{
	unsigned long returned;
	int index, host, restarted = 1;

	case_name = "e39_return";
	world(4);
	for (index = 0; index < 4; index++)
		start_searching(index, 1u << P2P_MM_SLAYER, P2P_MM_2V2, 1);
	run(100.0);
	host = find_host();
	CHECK(host >= 0 && machines[host].players == 4, "a full match");
	if (host < 0)
		return;
	machines[host].quit = 1;
	run(0.2);
	returned = now_ms;
	for (index = 0; index < 4; index++)
		if (index != host)
			restarted &= machines[index].search.marks_from >= returned - 200;
	CHECK(restarted, "E39: marks restart from the return");
	run(125.0);
	CHECK(host_count() == 0, "E39: a player short, no lone host: %d", host_count());
	for (index = 0; index < 4; index++)
		CHECK(index == host || machines[index].search.state == P2P_MM_QUEUED,
			"E39: machine %d searches on", index);
}

int main(void)
{
	check_pool();
	check_election();
	check_records();
	check_hosting();
	check_playlists();
	check_topics();
	check_verify_budget();
	check_four();
	check_replay();
	check_return();
	printf(failures ? "FAILED %d\n" : "PASS\n", failures);
	return failures != 0;
}
