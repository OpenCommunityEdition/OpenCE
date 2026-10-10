/*
PROFILE_TRACE.C

Record bounded CPU traces and hand frozen parts to the writer.
*/

#ifdef HALO_PROFILE

#include "profile_part.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* state changes happen only at the game frame boundary. The recording flag
and track regions change under the p2p lock. two arenas let the writer own
one frozen part while the game and p2p threads fill the other. */

/* ---------- constants */

enum
{
	/* a region is cut when no more than this share of it is left (a
	sixteenth: what a frame adds must still fit) */
	PROFILE_TRACE_HEADROOM_SHIFT = 4,
	PROFILE_TRACE_CLOCK_SAMPLES = 1000,
};

/* ---------- structures */

struct profile_trace_open
{
	unsigned long long start;
	unsigned short name;
	/* begun while recording: only those are written */
	unsigned char recorded;
};

struct profile_trace_track
{
	int index;
	struct profile_trace_record *records;
	unsigned long count;
	unsigned long capacity;
	struct profile_trace_open stack[MAXIMUM_PROFILE_TRACE_DEPTH];
	int depth;
	int open_aggregates;
	/* scopes begun past the stack's depth and not yet ended */
	int overflow;
	/* the track's own counts (no other thread writes them) */
	unsigned long deep;
	unsigned long unbalanced;
	unsigned long dropped;
};

struct profile_trace_aggregate
{
	int name;
	unsigned long long time;
	unsigned long count;
};

/* ---------- prototypes */

static unsigned long long profile_trace_default_clock(void);
static void profile_trace_no_lock(void);
static void *profile_trace_default_allocate(unsigned long size);
static void profile_trace_default_log(const char *text);
static void profile_trace_no_notify(const char *text);
static int profile_trace_always_ready(void);
static void profile_trace_default_session(struct profile_trace_session *session);
static void profile_trace_default_session_sampler(struct profile_trace_session *session);
static void profile_trace_sample_session(void);

/* ---------- globals */

static struct
{
	unsigned long long (*now)(void);
	void (*lock)(void);
	void (*unlock)(void);
	void *(*allocate)(unsigned long size);
	void (*release)(void *block);
	void (*emit)(const char *text);
	void (*notify)(const char *text);
	int (*start_writer)(void);
	int (*ready)(void);
	void (*describe)(struct profile_trace_session *session);
	void (*sample)(struct profile_trace_session *session);
} profile_trace_seams =
{
	profile_trace_default_clock,
	profile_trace_no_lock,
	profile_trace_no_lock,
	profile_trace_default_allocate,
	free,
	profile_trace_default_log,
	profile_trace_no_notify,
	profile_json_writer_start,
	profile_trace_always_ready,
	profile_trace_default_session,
	profile_trace_default_session_sampler,
};

/* the names, for the whole run (profile.c keeps their ids) */
static pthread_mutex_t profile_trace_names_lock = PTHREAD_MUTEX_INITIALIZER;
static const char *profile_trace_names[MAXIMUM_PROFILE_TRACE_NAMES];
static unsigned char profile_trace_aggregate_names[MAXIMUM_PROFILE_TRACE_NAMES];
static int profile_trace_name_count;
static struct profile_trace_aggregate profile_trace_aggregates[MAXIMUM_PROFILE_TRACE_AGGREGATES];
static int profile_trace_aggregate_count;
static int profile_trace_wait_name = -1;

static struct profile_trace_track profile_trace_tracks[NUMBER_OF_PROFILE_TRACKS];
static __thread struct profile_trace_track *profile_trace_thread_track;
/* scopes from threads with no track (any thread: atomically) */
static unsigned long profile_trace_foreign_scopes;

static struct
{
	int state;
	/* stored by the writer alone, once it has freed the arenas */
	int finished;
	int rearm;
	int when;
	double seconds;
	long memory_megabytes;
	int stop_reason;
	int map_loaded;
	int name_chosen;
	/* the tracks append; changed under the track lock */
	int recording;
	unsigned long long origin;
	unsigned long long frame_start;
	void *arenas[2];
	unsigned long arena_size;
	unsigned long game_bytes;
	unsigned long p2p_bytes;
	struct profile_part parts[2];
	int current;
	long part_number;
	long frame;
	long tick;
	unsigned long long next_session_sample;
	char name[64];
	struct profile_trace_session session;
	char start_utc[32];
	double clock_read_ns;
	double writer_wait_ms;
	/* the hand-off with the writer: which arenas it holds, and the parts
	queued for it */
	pthread_mutex_t writer_lock;
	pthread_cond_t writer_changed;
	int writer_holds[2];
	struct profile_part *queue[2];
	int queue_count;
} profile_trace_globals =
{
	.state = _profile_trace_idle,
	.memory_megabytes = DEFAULT_PROFILE_TRACE_MEMORY,
	.writer_lock = PTHREAD_MUTEX_INITIALIZER,
	.writer_changed = PTHREAD_COND_INITIALIZER,
};

/* ---------- private code */

static unsigned long long profile_trace_default_clock(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (unsigned long long)now.tv_sec * 1000000000ULL + (unsigned long long)now.tv_nsec;
}

static void profile_trace_no_lock(void)
{
}

static void *profile_trace_default_allocate(unsigned long size)
{
	return malloc(size);
}

static void profile_trace_default_log(const char *text)
{
	fprintf(stderr, "profile: %s\n", text);
}

static void profile_trace_no_notify(const char *text)
{
	(void)text;
}

static int profile_trace_always_ready(void)
{
	return 1;
}

static void profile_trace_default_session(struct profile_trace_session *session)
{
	memset(session, 0, sizeof(*session));
	strcpy(session->folder, ".");
	strcpy(session->role, "local");
	session->own_machine = -1;
	strcpy(session->gametype, "none");
}

static void profile_trace_default_session_sampler(struct profile_trace_session *session)
{
	(void)session;
}

static void profile_trace_sample_session(void)
{
	struct profile_trace_session current;
	unsigned long long now = profile_trace_seams.now();

	if (now < profile_trace_globals.next_session_sample)
		return;
	profile_trace_globals.next_session_sample = now + 1000000000ULL;
	memset(&current, 0, sizeof(current));
	profile_trace_seams.sample(&current);
	if (!profile_trace_globals.session.map_name[0] && current.map_name[0])
	{
		strcpy(profile_trace_globals.session.map, current.map);
		strcpy(profile_trace_globals.session.map_name, current.map_name);
		strcpy(profile_trace_globals.session.gametype, current.gametype);
	}
	if (current.players > profile_trace_globals.session.players_most)
		profile_trace_globals.session.players_most = current.players;
}

static const char *profile_trace_platform(void)
{
#if defined(HALO_ANDROID)
	return "android";
#elif defined(_WIN32)
	return "windows";
#else
	return "linux";
#endif
}

static int profile_trace_is_recording(void)
{
	return __atomic_load_n(&profile_trace_globals.recording, __ATOMIC_RELAXED);
}

static void profile_trace_append(
	struct profile_trace_track *track,
	unsigned long long start,
	unsigned long long duration,
	int name,
	int depth)
{
	struct profile_trace_record *record;

	if (!track->records || track->count >= track->capacity)
	{
		track->dropped++;
		return;
	}
	record = &track->records[track->count];
	/* (the game thread reads the p2p track's count at the frame boundary,
	without its lock) */
	__atomic_store_n(&track->count, track->count + 1, __ATOMIC_RELAXED);
	record->start = start;
	record->duration = duration > 0xFFFFFFFFULL ? 0xFFFFFFFFU : (unsigned int)duration;
	record->name = (unsigned short)name;
	record->depth = (unsigned char)depth;
	record->track = (unsigned char)track->index;
}

static int profile_trace_track_full(
	struct profile_trace_track const *track)
{
	unsigned long count = __atomic_load_n(&track->count, __ATOMIC_RELAXED);

	return count + (track->capacity >> PROFILE_TRACE_HEADROOM_SHIFT) >= track->capacity;
}

/* the arena's part n: the tracks write to its regions (under the track
lock: the p2p thread reads them) */
static void profile_trace_open_part(
	int arena,
	unsigned long long now)
{
	struct profile_part *part = &profile_trace_globals.parts[arena];
	char *base = profile_trace_globals.arenas[arena];
	struct profile_trace_track *game = &profile_trace_tracks[_profile_track_game];
	struct profile_trace_track *p2p = &profile_trace_tracks[_profile_track_p2p];

	memset(part, 0, sizeof(*part));
	part->arena = arena;
	part->number = profile_trace_globals.part_number;
	part->start_ns = now - profile_trace_globals.origin;
	part->first_frame = profile_trace_globals.frame;
	part->first_tick = profile_trace_globals.tick;
	game->records = (struct profile_trace_record *)base;
	game->capacity = profile_trace_globals.game_bytes / sizeof(struct profile_trace_record);
	game->count = 0;
	p2p->records = (struct profile_trace_record *)(base + profile_trace_globals.game_bytes);
	p2p->capacity = profile_trace_globals.p2p_bytes / sizeof(struct profile_trace_record);
	p2p->count = 0;
	part->records[_profile_track_game] = game->records;
	part->records[_profile_track_p2p] = p2p->records;
	profile_trace_globals.current = arena;
}

static void profile_trace_emit_aggregates(
	void)
{
	struct profile_trace_track *game = &profile_trace_tracks[_profile_track_game];
	unsigned long long start = profile_trace_globals.frame_start - profile_trace_globals.origin;
	int index;

	for (index = 0; index < profile_trace_aggregate_count; index++)
	{
		struct profile_trace_aggregate *aggregate = &profile_trace_aggregates[index];

		if (!aggregate->count)
			continue;
		profile_trace_append(game, start, aggregate->time, aggregate->name, PROFILE_TRACE_DEPTH_AGGREGATE_TIME);
		profile_trace_append(game, start, aggregate->count, aggregate->name, PROFILE_TRACE_DEPTH_AGGREGATE_COUNT);
		aggregate->time = 0;
		aggregate->count = 0;
	}
}

/* the current part frozen and queued for the writer: the last of the
recording, or the tracks go on in the other arena (once the writer has
given it back: the game thread waits for it here, and the wait is a
profile_wait scope in the new part) */
static void profile_trace_cut(
	int last,
	int reason)
{
	struct profile_part *part = &profile_trace_globals.parts[profile_trace_globals.current];
	int next = 1 - profile_trace_globals.current;
	unsigned long long now;
	unsigned long long wait_start = 0;
	unsigned long long waited = 0;
	int track;

	profile_trace_emit_aggregates();
	/* (a map load that stops the recording already names the next map: a
	recording that never learnt its map stays without one rather than take
	that one) */
	if (!(last && reason == _profile_trace_stop_map_load))
	{
		profile_trace_globals.next_session_sample = 0;
		profile_trace_sample_session();
	}
	if (!last)
	{
		pthread_mutex_lock(&profile_trace_globals.writer_lock);
		if (profile_trace_globals.writer_holds[next])
		{
			wait_start = profile_trace_seams.now();
			while (profile_trace_globals.writer_holds[next])
				pthread_cond_wait(&profile_trace_globals.writer_changed, &profile_trace_globals.writer_lock);
			waited = profile_trace_seams.now() - wait_start;
		}
		pthread_mutex_unlock(&profile_trace_globals.writer_lock);
		profile_trace_globals.writer_wait_ms += (double)waited / 1000000.0;
	}

	profile_trace_seams.lock();
	/* (read under the lock, after the wait for the writer: the p2p thread appends to this
	arena until here, and every record it appended ended before this time) */
	now = profile_trace_seams.now();
	for (track = 0; track < NUMBER_OF_PROFILE_TRACKS; track++)
	{
		struct profile_trace_track *source = &profile_trace_tracks[track];

		part->record_counts[track] = source->count;
		part->deep_scopes += source->deep;
		part->unbalanced_scopes += source->unbalanced;
		part->dropped_scopes += source->dropped;
		source->deep = source->unbalanced = source->dropped = 0;
	}
	if (last)
	{
		__atomic_store_n(&profile_trace_globals.recording, 0, __ATOMIC_RELAXED);
		for (track = 0; track < NUMBER_OF_PROFILE_TRACKS; track++)
		{
			profile_trace_tracks[track].records = NULL;
			profile_trace_tracks[track].capacity = 0;
			profile_trace_tracks[track].count = 0;
		}
	}
	else
	{
		profile_trace_globals.part_number++;
		profile_trace_open_part(next, now);
	}
	profile_trace_seams.unlock();

	part->last = last;
	part->stop_reason = last ? reason : _profile_trace_stop_none;
	part->end_ns = now - profile_trace_globals.origin;
	part->frames = profile_trace_globals.frame - part->first_frame;
	part->ticks = profile_trace_globals.tick - part->first_tick;
	strcpy(part->name, profile_trace_globals.name);
	strcpy(part->folder, profile_trace_globals.session.folder);
	part->session = profile_trace_globals.session;
	strcpy(part->session.platform, profile_trace_platform());
	strcpy(part->start_utc, profile_trace_globals.start_utc);
	part->clock_read_ns = profile_trace_globals.clock_read_ns;
	part->memory_used = (unsigned long)(part->record_counts[0] + part->record_counts[1]) *
		sizeof(struct profile_trace_record);
	part->memory_limit = profile_trace_globals.arena_size;
	part->writer_wait_ms = profile_trace_globals.writer_wait_ms;
	part->foreign_scopes = __atomic_exchange_n(&profile_trace_foreign_scopes, 0, __ATOMIC_RELAXED);
	pthread_mutex_lock(&profile_trace_names_lock);
	part->name_count = profile_trace_name_count;
	pthread_mutex_unlock(&profile_trace_names_lock);

	if (!last && waited)
	{
		profile_trace_append(&profile_trace_tracks[_profile_track_game], wait_start - profile_trace_globals.origin,
			now - wait_start, profile_trace_wait_name, 0);
	}

	pthread_mutex_lock(&profile_trace_globals.writer_lock);
	profile_trace_globals.writer_holds[part->arena] = 1;
	profile_trace_globals.queue[profile_trace_globals.queue_count++] = part;
	pthread_cond_broadcast(&profile_trace_globals.writer_changed);
	pthread_mutex_unlock(&profile_trace_globals.writer_lock);

	if (last)
	{
		profile_trace_globals.state = _profile_trace_finishing;
		profile_trace_log("writing %s, %ld parts", profile_trace_globals.name, profile_trace_globals.part_number);
	}
}

/* the recording's name, from the time and the machine's role: when it is
asked for (profile_record answers with it), or when a game starts */
static void profile_trace_choose_name(
	void)
{
	time_t wall = time(NULL);
	struct tm *utc = gmtime(&wall);
	char stamp[32];

	profile_trace_seams.describe(&profile_trace_globals.session);
	if (utc)
	{
		strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", utc);
		strftime(profile_trace_globals.start_utc, sizeof(profile_trace_globals.start_utc), "%Y-%m-%d %H:%M:%S", utc);
	}
	else
	{
		strcpy(stamp, "00000000-000000");
		strcpy(profile_trace_globals.start_utc, "");
	}
	profile_json_choose_name(profile_trace_globals.session.folder, stamp, profile_trace_globals.session.role,
		profile_trace_globals.name, sizeof(profile_trace_globals.name));
	profile_trace_globals.session.players_most = profile_trace_globals.session.players;
	profile_trace_globals.name_chosen = 1;
}

static void profile_trace_start_recording(
	void)
{
	unsigned long long memory = (unsigned long long)profile_trace_globals.memory_megabytes * 1024 * 1024;
	unsigned long size = (unsigned long)(memory / 2) & ~63UL;
	unsigned long scope_bytes = size;
	unsigned long long now;
	int track, sample;

	if (!profile_trace_globals.name_chosen)
		profile_trace_choose_name();
	profile_trace_globals.name_chosen = 0;
	profile_trace_globals.arenas[0] = profile_trace_seams.allocate(size);
	profile_trace_globals.arenas[1] = profile_trace_globals.arenas[0] ? profile_trace_seams.allocate(size) : NULL;
	if (!profile_trace_globals.arenas[1])
	{
		char text[96];

		if (profile_trace_globals.arenas[0])
			profile_trace_seams.release(profile_trace_globals.arenas[0]);
		profile_trace_globals.arenas[0] = NULL;
		snprintf(text, sizeof(text), "profile: no memory for %ld MB", profile_trace_globals.memory_megabytes);
		profile_trace_log("%s", text + 9);
		profile_trace_seams.notify(text);
		profile_trace_globals.state = _profile_trace_idle;
		profile_trace_globals.rearm = 0;
		return;
	}
	profile_trace_globals.arena_size = size;
	/* the scopes: seven eighths the game track's, an eighth
	the p2p track's (whole records each) */
	profile_trace_globals.game_bytes = scope_bytes / 8 * 7 / sizeof(struct profile_trace_record) *
		sizeof(struct profile_trace_record);
	profile_trace_globals.p2p_bytes = (scope_bytes - profile_trace_globals.game_bytes) /
		sizeof(struct profile_trace_record) * sizeof(struct profile_trace_record);

	__atomic_store_n(&profile_trace_foreign_scopes, 0, __ATOMIC_RELAXED);
	for (track = 0; track < profile_trace_aggregate_count; track++)
	{
		profile_trace_aggregates[track].time = 0;
		profile_trace_aggregates[track].count = 0;
	}
	profile_trace_globals.frame = 0;
	profile_trace_globals.tick = 0;
	profile_trace_globals.part_number = 1;
	profile_trace_globals.writer_wait_ms = 0.0;
	profile_trace_globals.stop_reason = _profile_trace_stop_none;
	profile_trace_globals.writer_holds[0] = profile_trace_globals.writer_holds[1] = 0;
	profile_trace_globals.queue_count = 0;
	__atomic_store_n(&profile_trace_globals.finished, 0, __ATOMIC_RELAXED);

	now = profile_trace_seams.now();
	for (sample = 0; sample < PROFILE_TRACE_CLOCK_SAMPLES; sample++)
		profile_trace_seams.now();
	profile_trace_globals.clock_read_ns = (double)(profile_trace_seams.now() - now) / PROFILE_TRACE_CLOCK_SAMPLES;

	now = profile_trace_seams.now();
	profile_trace_globals.origin = now;
	profile_trace_globals.frame_start = now;
	profile_trace_seams.lock();
	/* (under the lock: a p2p pass of the last recording may end, and count, meanwhile) */
	for (track = 0; track < NUMBER_OF_PROFILE_TRACKS; track++)
	{
		profile_trace_tracks[track].deep = 0;
		profile_trace_tracks[track].unbalanced = 0;
		profile_trace_tracks[track].dropped = 0;
	}
	profile_trace_open_part(0, now);
	profile_trace_seams.unlock();
	if (!profile_trace_seams.start_writer())
	{
		profile_trace_seams.lock();
		for (track = 0; track < NUMBER_OF_PROFILE_TRACKS; track++)
			profile_trace_tracks[track].records = NULL;
		profile_trace_seams.unlock();
		profile_trace_seams.release(profile_trace_globals.arenas[0]);
		profile_trace_seams.release(profile_trace_globals.arenas[1]);
		profile_trace_globals.arenas[0] = profile_trace_globals.arenas[1] = NULL;
		profile_trace_log("the writer thread cannot start: not recording");
		profile_trace_seams.notify("profile: the writer thread cannot start");
		profile_trace_globals.state = _profile_trace_idle;
		profile_trace_globals.rearm = 0;
		return;
	}
	profile_trace_seams.lock();
	__atomic_store_n(&profile_trace_globals.recording, 1, __ATOMIC_RELAXED);
	profile_trace_seams.unlock();
	profile_trace_globals.state = _profile_trace_recording;
	profile_trace_log("recording %s", profile_trace_globals.name);
}

/* ---------- public code */

void profile_trace_set_clock(
	unsigned long long (*now_ns)(void))
{
	profile_trace_seams.now = now_ns ? now_ns : profile_trace_default_clock;
}

void profile_trace_set_track_lock(
	void (*lock)(void),
	void (*unlock)(void))
{
	profile_trace_seams.lock = lock && unlock ? lock : profile_trace_no_lock;
	profile_trace_seams.unlock = lock && unlock ? unlock : profile_trace_no_lock;
}

void profile_trace_set_allocator(
	void *(*allocate)(unsigned long size),
	void (*release)(void *block))
{
	profile_trace_seams.allocate = allocate && release ? allocate : profile_trace_default_allocate;
	profile_trace_seams.release = allocate && release ? release : free;
}

void profile_trace_set_log(
	void (*log)(const char *text))
{
	profile_trace_seams.emit = log ? log : profile_trace_default_log;
}

void profile_trace_set_notify(
	void (*notify)(const char *text))
{
	profile_trace_seams.notify = notify ? notify : profile_trace_no_notify;
}

void profile_trace_set_writer(
	int (*start)(void))
{
	profile_trace_seams.start_writer = start ? start : profile_json_writer_start;
}

void profile_trace_set_ready(
	int (*ready)(void))
{
	profile_trace_seams.ready = ready ? ready : profile_trace_always_ready;
}

void profile_trace_set_session(
	void (*describe)(struct profile_trace_session *session))
{
	profile_trace_seams.describe = describe ? describe : profile_trace_default_session;
}

void profile_trace_set_session_sampler(
	void (*sample)(struct profile_trace_session *session))
{
	profile_trace_seams.sample = sample ? sample : profile_trace_default_session_sampler;
}

void profile_trace_log(
	const char *format,
	...)
{
	char text[512];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(text, sizeof(text), format, arguments);
	va_end(arguments);
	profile_trace_seams.emit(text);
}

void profile_trace_thread_register(
	int track)
{
	if (track < 0 || track >= NUMBER_OF_PROFILE_TRACKS)
		return;
	profile_trace_tracks[track].index = track;
	profile_trace_thread_track = &profile_trace_tracks[track];
}

int profile_trace_name(
	const char *name)
{
	int index;

	if (!name)
		return -1;
	pthread_mutex_lock(&profile_trace_names_lock);
	for (index = 0; index < profile_trace_name_count; index++)
	{
		if (strcmp(profile_trace_names[index], name) == 0)
			break;
	}
	if (index == profile_trace_name_count)
	{
		if (index < MAXIMUM_PROFILE_TRACE_NAMES)
			profile_trace_names[profile_trace_name_count++] = name;
		else
			index = -1;
	}
	pthread_mutex_unlock(&profile_trace_names_lock);
	return index;
}

void profile_trace_name_aggregate(
	const char *name)
{
	int id = profile_trace_name(name);

	if (id < 0 || profile_trace_aggregate_names[id] ||
		profile_trace_aggregate_count >= MAXIMUM_PROFILE_TRACE_AGGREGATES)
	{
		return;
	}
	profile_trace_aggregates[profile_trace_aggregate_count].name = id;
	profile_trace_aggregates[profile_trace_aggregate_count].time = 0;
	profile_trace_aggregates[profile_trace_aggregate_count].count = 0;
	profile_trace_aggregate_count++;
	profile_trace_aggregate_names[id] = 1;
}

const char *profile_trace_name_text(
	int name)
{
	const char *text = "unknown";

	pthread_mutex_lock(&profile_trace_names_lock);
	if (name >= 0 && name < profile_trace_name_count)
		text = profile_trace_names[name];
	pthread_mutex_unlock(&profile_trace_names_lock);
	return text;
}

int profile_trace_name_is_aggregate(
	int name)
{
	return name >= 0 && name < MAXIMUM_PROFILE_TRACE_NAMES && profile_trace_aggregate_names[name];
}

void profile_trace_begin(
	int name)
{
	struct profile_trace_track *track = profile_trace_thread_track;
	struct profile_trace_open *open;

	if (!track)
	{
		if (profile_trace_is_recording())
			__atomic_fetch_add(&profile_trace_foreign_scopes, 1, __ATOMIC_RELAXED);
		return;
	}
	if (name < 0)
		return;
	if (track->depth >= MAXIMUM_PROFILE_TRACE_DEPTH)
	{
		track->overflow++;
		if (profile_trace_is_recording())
			track->deep++;
		return;
	}
	open = &track->stack[track->depth++];
	open->name = (unsigned short)name;
	open->recorded = (unsigned char)profile_trace_is_recording();
	open->start = open->recorded ? profile_trace_seams.now() : 0;
	if (profile_trace_aggregate_names[name])
		track->open_aggregates++;
}

void profile_trace_end(
	int name)
{
	struct profile_trace_track *track = profile_trace_thread_track;
	struct profile_trace_open *open;
	unsigned long long duration;

	if (!track || name < 0)
		return;
	if (track->overflow)
	{
		track->overflow--;
		return;
	}
	if (!track->depth || track->stack[track->depth - 1].name != name)
	{
		if (profile_trace_is_recording())
			track->unbalanced++;
		return;
	}
	open = &track->stack[--track->depth];
	if (profile_trace_aggregate_names[name] && track->open_aggregates > 0)
		track->open_aggregates--;
	if (!open->recorded)
		return;
	/* (a scope that outlived its recording: the p2p thread lets go of its
	lock for slow work, and a stop can come meanwhile) */
	if (!profile_trace_is_recording() || !track->records || open->start < profile_trace_globals.origin)
	{
		track->dropped++;
		return;
	}
	duration = profile_trace_seams.now() - open->start;
	if (profile_trace_aggregate_names[name])
	{
		int index;

		if (track->index != _profile_track_game)
			return;
		for (index = 0; index < profile_trace_aggregate_count; index++)
		{
			if (profile_trace_aggregates[index].name == name)
			{
				profile_trace_aggregates[index].time += duration;
				profile_trace_aggregates[index].count++;
				break;
			}
		}
		return;
	}
	{
		/* (an aggregated parent leaves no record, so depth counts only the
		recorded ancestors) */
		int depth = track->depth - track->open_aggregates;
		profile_trace_append(track, open->start - profile_trace_globals.origin, duration, name, depth);
	}
}

void profile_trace_frame_boundary(
	void)
{
	struct profile_trace_track *game = &profile_trace_tracks[_profile_track_game];
	unsigned long long now;

	/* (a scope the frame left open: its end is lost, and so is it) */
	if (game->depth || game->overflow)
	{
		if (profile_trace_is_recording())
			game->unbalanced += (unsigned long)(game->depth + game->overflow);
		game->depth = 0;
		game->open_aggregates = 0;
		game->overflow = 0;
	}
	if (profile_trace_wait_name < 0)
		profile_trace_wait_name = profile_trace_name("profile_wait");
	switch (profile_trace_globals.state)
	{
	case _profile_trace_armed:
		if (profile_trace_globals.when == _profile_trace_when_now || profile_trace_seams.ready())
			profile_trace_start_recording();
		break;
	case _profile_trace_recording:
		now = profile_trace_seams.now();
		if (now >= profile_trace_globals.next_session_sample)
			profile_trace_sample_session();
		profile_trace_emit_aggregates();
		profile_trace_globals.frame++;
		profile_trace_globals.frame_start = now;
		if (!profile_trace_globals.stop_reason && profile_trace_globals.seconds > 0.0 &&
			(double)(now - profile_trace_globals.origin) >= profile_trace_globals.seconds * 1000000000.0)
		{
			profile_trace_globals.stop_reason = _profile_trace_stop_seconds;
		}
		if (profile_trace_globals.stop_reason)
		{
			profile_trace_cut(1, profile_trace_globals.stop_reason);
		}
		else if (profile_trace_track_full(&profile_trace_tracks[_profile_track_game]) ||
			profile_trace_track_full(&profile_trace_tracks[_profile_track_p2p]))
		{
			profile_trace_cut(0, _profile_trace_stop_none);
		}
		break;
	case _profile_trace_finishing:
		if (__atomic_load_n(&profile_trace_globals.finished, __ATOMIC_ACQUIRE))
		{
			__atomic_store_n(&profile_trace_globals.finished, 0, __ATOMIC_RELAXED);
			profile_trace_globals.state = profile_trace_globals.rearm ? _profile_trace_armed : _profile_trace_idle;
		}
		break;
	default:
		break;
	}
}

void profile_trace_tick(
	void)
{
	if (profile_trace_globals.state != _profile_trace_recording)
		return;
	profile_trace_globals.tick++;
}

int profile_trace_request_start(
	double seconds,
	int when,
	long memory_megabytes)
{
	switch (profile_trace_globals.state)
	{
	case _profile_trace_armed:
	case _profile_trace_recording:
		return _profile_trace_answer_already_recording;
	case _profile_trace_finishing:
		return _profile_trace_answer_still_writing;
	default:
		break;
	}
	if (memory_megabytes < MINIMUM_PROFILE_TRACE_MEMORY)
		memory_megabytes = MINIMUM_PROFILE_TRACE_MEMORY;
	if (memory_megabytes > MAXIMUM_PROFILE_TRACE_MEMORY)
		memory_megabytes = MAXIMUM_PROFILE_TRACE_MEMORY;
	profile_trace_globals.seconds = seconds > 0.0 ? seconds : 0.0;
	profile_trace_globals.when = when;
	profile_trace_globals.memory_megabytes = memory_megabytes;
	profile_trace_globals.rearm = when == _profile_trace_when_game;
	profile_trace_globals.stop_reason = _profile_trace_stop_none;
	profile_trace_globals.state = _profile_trace_armed;
	profile_trace_globals.name_chosen = 0;
	if (when == _profile_trace_when_now)
		profile_trace_choose_name();
	return _profile_trace_answer_armed;
}

int profile_trace_request_stop(
	void)
{
	switch (profile_trace_globals.state)
	{
	case _profile_trace_armed:
		profile_trace_globals.state = _profile_trace_idle;
		profile_trace_globals.rearm = 0;
		return profile_trace_globals.when == _profile_trace_when_game ? _profile_trace_answer_disarmed :
			_profile_trace_answer_not_recording;
	case _profile_trace_recording:
		profile_trace_globals.rearm = 0;
		if (!profile_trace_globals.stop_reason)
			profile_trace_globals.stop_reason = _profile_trace_stop_command;
		return _profile_trace_answer_stopping;
	case _profile_trace_finishing:
		if (profile_trace_globals.rearm)
		{
			profile_trace_globals.rearm = 0;
			return _profile_trace_answer_disarmed;
		}
		return _profile_trace_answer_not_recording;
	default:
		return _profile_trace_answer_not_recording;
	}
}

void profile_trace_map_loaded(
	void)
{
	if (!profile_trace_globals.map_loaded)
	{
		profile_trace_globals.map_loaded = 1;
		return;
	}
	if (profile_trace_globals.state == _profile_trace_recording && !profile_trace_globals.stop_reason)
		profile_trace_globals.stop_reason = _profile_trace_stop_map_load;
}

void profile_trace_shutdown(
	void)
{
	struct profile_trace_track *game = &profile_trace_tracks[_profile_track_game];

	/* (exit called on another thread: the game thread may be mid-frame,
	writing to the arena; the part being recorded is lost, as when the
	process is killed) */
	if (profile_trace_thread_track != game)
		return;
	profile_trace_globals.rearm = 0;
	if (profile_trace_globals.state == _profile_trace_armed)
		profile_trace_globals.state = _profile_trace_idle;
	if (profile_trace_globals.state == _profile_trace_recording)
	{
		game->depth = 0;
		game->open_aggregates = 0;
		game->overflow = 0;
		profile_trace_cut(1, _profile_trace_stop_exit);
	}
	if (profile_trace_globals.state == _profile_trace_finishing)
	{
		pthread_mutex_lock(&profile_trace_globals.writer_lock);
		while (!__atomic_load_n(&profile_trace_globals.finished, __ATOMIC_ACQUIRE))
			pthread_cond_wait(&profile_trace_globals.writer_changed, &profile_trace_globals.writer_lock);
		pthread_mutex_unlock(&profile_trace_globals.writer_lock);
		__atomic_store_n(&profile_trace_globals.finished, 0, __ATOMIC_RELAXED);
		profile_trace_globals.state = _profile_trace_idle;
	}
}

int profile_trace_recording(
	void)
{
	return profile_trace_is_recording();
}

void profile_trace_status(
	struct profile_trace_status *status)
{
	memset(status, 0, sizeof(*status));
	status->state = profile_trace_globals.state;
	strcpy(status->name, profile_trace_globals.name);
	if (profile_trace_globals.state != _profile_trace_recording)
		return;
	status->part = profile_trace_globals.part_number;
}

struct profile_part *profile_trace_writer_next(
	void)
{
	struct profile_part *part;

	pthread_mutex_lock(&profile_trace_globals.writer_lock);
	while (!profile_trace_globals.queue_count)
		pthread_cond_wait(&profile_trace_globals.writer_changed, &profile_trace_globals.writer_lock);
	part = profile_trace_globals.queue[0];
	profile_trace_globals.queue[0] = profile_trace_globals.queue[1];
	profile_trace_globals.queue_count--;
	pthread_mutex_unlock(&profile_trace_globals.writer_lock);
	return part;
}

void profile_trace_writer_done(
	struct profile_part *part)
{
	int last = part->last;

	pthread_mutex_lock(&profile_trace_globals.writer_lock);
	profile_trace_globals.writer_holds[part->arena] = 0;
	pthread_cond_broadcast(&profile_trace_globals.writer_changed);
	pthread_mutex_unlock(&profile_trace_globals.writer_lock);
	if (!last)
		return;
	/* (the last part: no track writes to either arena any more, and the
	writer has written the other part before this one) */
	profile_trace_seams.release(profile_trace_globals.arenas[0]);
	profile_trace_seams.release(profile_trace_globals.arenas[1]);
	profile_trace_globals.arenas[0] = profile_trace_globals.arenas[1] = NULL;
	pthread_mutex_lock(&profile_trace_globals.writer_lock);
	__atomic_store_n(&profile_trace_globals.finished, 1, __ATOMIC_RELEASE);
	pthread_cond_broadcast(&profile_trace_globals.writer_changed);
	pthread_mutex_unlock(&profile_trace_globals.writer_lock);
}

#endif
