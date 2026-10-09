/*
PROFILE_CHECK.C

Exercise the CPU recorder and writer with standalone fakes.
*/

#include "profile_part.h"
#include "profile_console_gametype.h"

#include <pthread.h>
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* ---------- fakes */

static int failures;
static int session_sample_calls;
static unsigned long long fake_now = 1000000000ULL;
static pthread_mutex_t fake_track_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t fake_allocator_lock = PTHREAD_MUTEX_INITIALIZER;
static long allocations_outstanding;
static long allocations_made;
static int allocations_refused;
/* allocations that succeed before the rest are refused (-1: no such limit) */
static int allocations_allowed = -1;
static int fake_ready;
static const char *fake_role = "host";
static long fake_players = 2;
static long fake_own_machine = -1;
static const char *fake_map = "levels\\a30\\a30";
static const char *fake_map_name = "a30";
static const char *fake_folder = ".";
static char last_notice[128];
static char last_log[600];
static int name_aggregate_child;

static void check(int good, const char *what)
{
	if (!good)
	{
		failures++;
		printf("FAIL: %s\n", what);
	}
}

static void gametype_checks(void)
{
	static const char *const expected[] = { "none", "ctf", "slayer", "oddball", "king", "race" };
	long engine;
	for (engine = 0; engine <= 5; engine++)
		check(strcmp(profile_console_engine_gametype(1, engine), expected[engine]) == 0,
			"engine index maps to its engine gametype");
	check(strcmp(profile_console_engine_gametype(0, 2), "campaign") == 0,
		"campaign does not reuse a stale slayer variant");
	check(strcmp(profile_console_engine_gametype(1, 6), "none") == 0 &&
		strcmp(profile_console_engine_gametype(1, 7), "none") == 0, "an engine past race is none");
	check(strcmp(profile_console_scenario_map("levels\\b30\\b30", "ui"), "levels\\b30\\b30") == 0,
		"the pending scenario takes precedence over the previous map");
}

static unsigned long long fake_clock(void)
{
	return __atomic_load_n(&fake_now, __ATOMIC_RELAXED);
}

static void advance(unsigned long long nanoseconds)
{
	__atomic_fetch_add(&fake_now, nanoseconds, __ATOMIC_RELAXED);
}

static void fake_lock(void)
{
	pthread_mutex_lock(&fake_track_lock);
}

static void fake_unlock(void)
{
	pthread_mutex_unlock(&fake_track_lock);
}

static void *fake_allocate(unsigned long size)
{
	void *block;

	pthread_mutex_lock(&fake_allocator_lock);
	block = allocations_refused || allocations_allowed == 0 ? NULL : malloc(size);
	if (allocations_allowed > 0)
		allocations_allowed--;
	if (block)
	{
		allocations_outstanding++;
		allocations_made++;
	}
	pthread_mutex_unlock(&fake_allocator_lock);
	return block;
}

static void fake_release(void *block)
{
	if (!block)
		return;
	pthread_mutex_lock(&fake_allocator_lock);
	allocations_outstanding--;
	pthread_mutex_unlock(&fake_allocator_lock);
	free(block);
}

static long outstanding(void)
{
	long count;

	pthread_mutex_lock(&fake_allocator_lock);
	count = allocations_outstanding;
	pthread_mutex_unlock(&fake_allocator_lock);
	return count;
}

static void quiet_log(const char *text)
{
	if (strstr(text, "cannot write"))
		snprintf(last_log, sizeof(last_log), "%s", text);
}

static void fake_notify(const char *text)
{
	snprintf(last_notice, sizeof(last_notice), "%s", text);
}

static int fake_ready_test(void)
{
	return fake_ready;
}

static void fake_session(struct profile_trace_session *session)
{
	memset(session, 0, sizeof(*session));
	snprintf(session->folder, sizeof(session->folder), "%s", fake_folder);
	strcpy(session->build, "check");
	strncpy(session->role, fake_role, sizeof(session->role) - 1);
	session->own_machine = fake_own_machine;
	strncpy(session->map, fake_map, sizeof(session->map) - 1);
	strncpy(session->map_name, fake_map_name, sizeof(session->map_name) - 1);
	strcpy(session->gametype, "campaign");
	session->players = fake_players;
	session->players_most = fake_players;
}

static void fake_session_sample(struct profile_trace_session *session)
{
	session_sample_calls++;
	fake_session(session);
}

/* ---------- the fake writer: what each part held */

enum
{
	MAXIMUM_PARTS = 64,
};

static struct
{
	pthread_mutex_t lock;
	int parts;
	int finished;
	unsigned long counts[MAXIMUM_PROFILE_TRACE_NAMES];
	unsigned long counter_records;
	unsigned long long aggregate_time[MAXIMUM_PROFILE_TRACE_NAMES];
	unsigned long aggregate_count[MAXIMUM_PROFILE_TRACE_NAMES];
	long first_frame[MAXIMUM_PARTS];
	long frames[MAXIMUM_PARTS];
	unsigned long long start_ns[MAXIMUM_PARTS];
	unsigned long long end_ns[MAXIMUM_PARTS];
	int last[MAXIMUM_PARTS];
	int stop_reason[MAXIMUM_PARTS];
	char map_name[MAXIMUM_PARTS][64];
	double writer_wait_ms;
	unsigned long unbalanced;
	unsigned long deep;
	unsigned long foreign;
	unsigned long dropped;
	int records_in_span;
	int aggregate_child_depth;
	int delay_milliseconds;
} written = { .lock = PTHREAD_MUTEX_INITIALIZER };

static void *fake_writer(void *unused)
{
	(void)unused;
	for (;;)
	{
		struct profile_part *part = profile_trace_writer_next();
		int last = part->last;
		unsigned long index;
		int track;

		/* (a slow first part: the game thread's wait for it is on the fake
		clock too) */
		if (written.delay_milliseconds && part->number == 1)
		{
			usleep((useconds_t)written.delay_milliseconds * 1000);
			advance((unsigned long long)written.delay_milliseconds * 1000000ULL);
		}
		pthread_mutex_lock(&written.lock);
		if (written.parts < MAXIMUM_PARTS)
		{
			written.first_frame[written.parts] = part->first_frame;
			written.frames[written.parts] = part->frames;
			written.start_ns[written.parts] = part->start_ns;
			written.end_ns[written.parts] = part->end_ns;
			written.last[written.parts] = part->last;
			written.stop_reason[written.parts] = part->stop_reason;
			strcpy(written.map_name[written.parts], part->session.map_name);
		}
		written.parts++;
		written.writer_wait_ms = part->writer_wait_ms;
		written.unbalanced += part->unbalanced_scopes;
		written.deep += part->deep_scopes;
		written.foreign += part->foreign_scopes;
		written.dropped += part->dropped_scopes;
		for (track = 0; track < NUMBER_OF_PROFILE_TRACKS; track++)
		{
			for (index = 0; index < part->record_counts[track]; index++)
			{
				const struct profile_trace_record *record = &part->records[track][index];

				if (record->depth < MAXIMUM_PROFILE_TRACE_DEPTH)
				{
					written.counts[record->name]++;
					if (record->name == name_aggregate_child)
						written.aggregate_child_depth = record->depth;
					/* (a scope that began before the part, in the p2p pass a
					cut came in, is in the part it ended in) */
					if (record->start + record->duration > part->end_ns)
						written.records_in_span = 0;
				}
				else
				{
					written.counter_records++;
					if (record->depth == PROFILE_TRACE_DEPTH_AGGREGATE_TIME)
						written.aggregate_time[record->name] += record->duration;
					else if (record->depth == PROFILE_TRACE_DEPTH_AGGREGATE_COUNT)
						written.aggregate_count[record->name] += record->duration;
				}
			}
		}
		pthread_mutex_unlock(&written.lock);
		if (last)
		{
			pthread_mutex_lock(&written.lock);
			written.finished++;
			pthread_mutex_unlock(&written.lock);
		}
		/* Publish the fake writer's last observation before done makes the
		recorder idle; finish() may read the observations as soon as it does. */
		profile_trace_writer_done(part);
		if (last)
			break;
	}
	return NULL;
}

static int fake_writer_start(void)
{
	pthread_t thread;

	return pthread_create(&thread, NULL, fake_writer, NULL) == 0 && pthread_detach(thread) == 0;
}

static void forget_written(void)
{
	pthread_mutex_lock(&written.lock);
	written.parts = 0;
	written.finished = 0;
	memset(written.counts, 0, sizeof(written.counts));
	written.counter_records = 0;
	written.unbalanced = written.deep = written.foreign = written.dropped = 0;
	written.records_in_span = 1;
	pthread_mutex_unlock(&written.lock);
}

/* whether the writer has finished a recording, read through its lock */
static int writer_finished(void)
{
	int finished;

	pthread_mutex_lock(&written.lock);
	finished = written.finished;
	pthread_mutex_unlock(&written.lock);
	return finished;
}

/* frame boundaries until the recording is written and the state idle */
static void finish(void)
{
	struct profile_trace_status status;
	int frames;

	for (frames = 0; frames < 100000; frames++)
	{
		profile_trace_frame_boundary();
		profile_trace_status(&status);
		if (status.state == _profile_trace_idle || status.state == _profile_trace_armed)
		{
			/* (what the writer noted, seen through its lock) */
			pthread_mutex_lock(&written.lock);
			pthread_mutex_unlock(&written.lock);
			return;
		}
		usleep(100);
	}
	check(0, "the recording finishes");
}

/* ---------- checks */

static int name_frame, name_tick, name_a, name_b, name_c, name_texture, name_pass, name_inner_aggregate;

static void frame(int scopes)
{
	int index;

	profile_trace_frame_boundary();
	profile_trace_begin(name_frame);
	for (index = 0; index < scopes; index++)
	{
		profile_trace_begin(name_a);
		advance(1000);
		profile_trace_begin(name_b);
		advance(500);
		profile_trace_end(name_b);
		profile_trace_end(name_a);
	}
	advance(1000);
	profile_trace_end(name_frame);
}

static void scope_checks(void)
{
	struct profile_trace_status status;
	int index;

	forget_written();
	check(profile_trace_request_start(0.0, _profile_trace_when_now, 4) == _profile_trace_answer_armed,
		"profile_record arms");
	profile_trace_status(&status);
	check(status.state == _profile_trace_armed, "armed until the next frame boundary");
	check(strncmp(status.name, "profile_", 8) == 0 && strstr(status.name, "_host"), "named when asked for");
	check(!profile_trace_recording(), "not recording before the boundary");
	frame(3);
	check(profile_trace_recording(), "recording from the boundary");
	/* the first map load after launch (the main menu) does not stop it */
	profile_trace_map_loaded();
	profile_trace_frame_boundary();
	check(profile_trace_recording(), "the first map load does not stop a recording");
	check(profile_trace_request_start(0.0, _profile_trace_when_now, 4) == _profile_trace_answer_already_recording,
		"a second profile_record is refused");

	/* nesting deeper than the stack: not recorded, counted, and the rest
	still balances */
	profile_trace_frame_boundary();
	for (index = 0; index < MAXIMUM_PROFILE_TRACE_DEPTH + 3; index++)
		profile_trace_begin(name_c);
	for (index = 0; index < MAXIMUM_PROFILE_TRACE_DEPTH + 3; index++)
		profile_trace_end(name_c);
	/* an end that is not the top's, and one with nothing open */
	profile_trace_begin(name_a);
	profile_trace_end(name_b);
	profile_trace_end(name_a);
	profile_trace_end(name_b);
	/* aggregate scopes: summed for the frame, no records */
	profile_trace_begin(name_texture);
	profile_trace_begin(name_inner_aggregate);
	advance(500);
	profile_trace_begin(name_aggregate_child);
	advance(1000);
	profile_trace_end(name_aggregate_child);
	profile_trace_end(name_inner_aggregate);
	advance(500);
	profile_trace_end(name_texture);
	for (index = 0; index < 5; index++)
	{
		profile_trace_begin(name_texture);
		advance(2000);
		profile_trace_end(name_texture);
	}
	/* a scope left open is dropped at the boundary, and counted */
	profile_trace_begin(name_a);
	frame(1);
	check(profile_trace_request_stop() == _profile_trace_answer_stopping, "profile_stop stops");
	check(profile_trace_recording(), "a stop applies at the next boundary");
	profile_trace_frame_boundary();
	check(!profile_trace_recording(), "stopped at the boundary");
	check(profile_trace_request_start(0.0, _profile_trace_when_now, 4) == _profile_trace_answer_still_writing ||
		writer_finished(), "profile_record while writing is refused");
	finish();
	check(written.finished == 1 && written.parts == 1, "one part written");
	check(written.counts[name_frame] == 2, "two frames recorded");
	check(written.counts[name_a] == 5 && written.counts[name_b] == 4, "every scope recorded once");
	check(written.counts[name_c] == MAXIMUM_PROFILE_TRACE_DEPTH, "scopes within the stack recorded");
	check(written.deep == 3, "scopes past the stack counted");
	check(written.unbalanced == 3, "unbalanced ends and the open scope counted");
	check(written.counts[name_texture] == 0 && written.counter_records >= 2, "aggregate names give counters");
	check(written.aggregate_child_depth == 0, "ordinary scopes inside an aggregate keep visible depth");
	check(written.aggregate_count[name_texture] == 6 && written.aggregate_time[name_texture] == 12000,
		"outer aggregate totals include calls nested with another aggregate");
	check(written.aggregate_count[name_inner_aggregate] == 1 && written.aggregate_time[name_inner_aggregate] == 1500,
		"nested aggregate keeps its own cpu summary count and time");
	check(written.counts[name_aggregate_child] == 1, "ordinary nested scope contributes one cpu summary count");
	check(written.stop_reason[0] == _profile_trace_stop_command, "stopped by the command");
	check(outstanding() == 0, "both arenas freed after the recording");
}

static void state_checks(void)
{
	struct profile_trace_status status;

	/* game: armed until a game is in progress, stopped by a map load, then
	armed again */
	forget_written();
	fake_ready = 0;
	fake_map = "";
	fake_map_name = "";
	check(profile_trace_request_start(0.0, _profile_trace_when_game, 4) == _profile_trace_answer_armed, "armed for a game");
	frame(1);
	frame(1);
	profile_trace_status(&status);
	check(status.state == _profile_trace_armed, "waits for a game");
	fake_ready = 1;
	frame(1);
	check(profile_trace_recording(), "records once a game is in progress");
	frame(1);
	fake_map = "levels\\next\\next";
	fake_map_name = "next";
	profile_trace_map_loaded();
	frame(1);
	check(!profile_trace_recording(), "a map load stops it");
	finish();
	fake_map = "levels\\a30\\a30";
	fake_map_name = "a30";
	profile_trace_status(&status);
	check(status.state == _profile_trace_armed, "armed again for the next game");
	check(written.stop_reason[0] == _profile_trace_stop_map_load, "stopped by the map load");
	check(strcmp(written.map_name[0], "next") != 0, "the map that stops a recording is not sampled into it");
	frame(1);
	check(profile_trace_recording(), "the next game is recorded");
	check(profile_trace_request_stop() == _profile_trace_answer_stopping, "stopped by hand");
	frame(1);
	finish();
	profile_trace_status(&status);
	check(status.state == _profile_trace_idle, "a stop by hand does not arm again");
	check(outstanding() == 0, "nothing left allocated");

	/* the seconds of profile_record */
	forget_written();
	profile_trace_request_start(0.5, _profile_trace_when_now, 4);
	frame(1);
	advance(400000000ULL);
	frame(1);
	check(profile_trace_recording(), "still recording before its seconds");
	advance(200000000ULL);
	frame(1);
	check(!profile_trace_recording(), "stopped after its seconds");
	finish();
	check(written.stop_reason[0] == _profile_trace_stop_seconds, "stopped by its seconds");

	/* no memory: not recording, said so, nothing kept */
	allocations_refused = 1;
	profile_trace_request_start(0.0, _profile_trace_when_now, 4);
	frame(1);
	allocations_refused = 0;
	profile_trace_status(&status);
	check(status.state == _profile_trace_idle && strstr(last_notice, "no memory for 4 MB"), "no memory is reported");
	check(outstanding() == 0, "a failed allocation keeps nothing");
}

static volatile int p2p_running;
static unsigned long p2p_recorded;

static void *p2p_thread(void *unused)
{
	(void)unused;
	profile_trace_thread_register(_profile_track_p2p);
	while (__atomic_load_n(&p2p_running, __ATOMIC_RELAXED))
	{
		fake_lock();
		if (profile_trace_recording())
			p2p_recorded++;
		profile_trace_begin(name_pass);
		profile_trace_end(name_pass);
		fake_unlock();
		/* (a pass every 50 microseconds: the p2p region holds an eighth of
		the scopes, as the game's p2p thread has far fewer) */
		usleep(50);
	}
	return NULL;
}

/* what profile_stop answers while a recording is only armed: a game is waited for
only by "game" mode, which stop ends too */
static void stop_answer_checks(void)
{
	struct profile_trace_status status;

	check(profile_trace_request_start(0.0, _profile_trace_when_now, 4) == _profile_trace_answer_armed, "armed for the next frame");
	check(profile_trace_request_stop() == _profile_trace_answer_not_recording,
		"stopped before it began, a recording armed for the next frame was waiting for no game");
	profile_trace_status(&status);
	check(status.state == _profile_trace_idle, "stopped while armed, it does not start");
	check(profile_trace_request_start(0.0, _profile_trace_when_game, 4) == _profile_trace_answer_armed, "armed for a game");
	check(profile_trace_request_stop() == _profile_trace_answer_disarmed, "a recording armed for a game is not waited for any more");
	profile_trace_status(&status);
	check(status.state == _profile_trace_idle, "stopped while armed for a game, it does not start");
}

static void part_checks(void)
{
	pthread_t thread;
	unsigned long frames_recorded = 0;
	int frame_index, part;
	int continuous = 1;

	forget_written();
	p2p_recorded = 0;
	p2p_running = 1;
	pthread_create(&thread, NULL, p2p_thread, NULL);
	profile_trace_request_start(0.0, _profile_trace_when_now, 4);
	/* (4 MB, CPU-only: about 114 000 game records a part) */
	for (frame_index = 0; frame_index < 3000; frame_index++)
	{
		frame(50);
		if (profile_trace_recording())
			frames_recorded++;
		if (frame_index % 30 == 0)
			profile_trace_tick();
		advance(1000000);
	}
	profile_trace_request_stop();
	profile_trace_frame_boundary();
	__atomic_store_n(&p2p_running, 0, __ATOMIC_RELAXED);
	pthread_join(thread, NULL);
	finish();
	fake_role = "host";
	fake_players = 2;
	fake_own_machine = -1;
	check(written.parts >= 3, "a long recording is cut into parts");
	check(written.counts[name_frame] == frames_recorded, "every frame in exactly one part");
	check(written.counts[name_a] == frames_recorded * 50, "every scope in exactly one part");
	check(written.counts[name_pass] == p2p_recorded, "every recorded p2p pass in exactly one part");
	check(written.dropped == 0, "nothing dropped");
	for (part = 1; part < written.parts && part < MAXIMUM_PARTS; part++)
	{
		continuous &= written.first_frame[part] == written.first_frame[part - 1] + written.frames[part - 1];
		continuous &= written.start_ns[part] == written.end_ns[part - 1];
		continuous &= !written.last[part - 1];
	}
	check(continuous, "frames and time continue across parts");
	check(written.last[written.parts - 1], "only the last part is the last");
	check(written.records_in_span, "each record within its part");
	check(outstanding() == 0, "both arenas freed after a long recording");

	/* a cut while the writer holds the other arena waits for it */
	forget_written();
	written.delay_milliseconds = 1000;
	p2p_recorded = 0;
	p2p_running = 1;
	pthread_create(&thread, NULL, p2p_thread, NULL);
	profile_trace_request_start(0.0, _profile_trace_when_now, 4);
	frames_recorded = 0;
	for (frame_index = 0; frame_index < 3000; frame_index++)
	{
		frame(50);
		if (profile_trace_recording())
			frames_recorded++;
	}
	profile_trace_request_stop();
	profile_trace_frame_boundary();
	__atomic_store_n(&p2p_running, 0, __ATOMIC_RELAXED);
	pthread_join(thread, NULL);
	finish();
	written.delay_milliseconds = 0;
	check(written.records_in_span, "each record within its part while the writer is slow");
	check(written.counts[name_pass] == p2p_recorded, "a slow writer loses no p2p pass");
	check(written.parts >= 3, "parts while the writer is slow");
	check(written.writer_wait_ms > 0.0, "the wait for the writer is counted");
	check(written.counts[name_a] == frames_recorded * 50, "a slow writer loses nothing");
	check(written.counts[profile_trace_name("profile_wait")] >= 1, "the wait is a scope");
}

static int straddle_begun;
static int straddle_released;

/* a p2p pass that lets go of its lock for slow work (p2p_resolve's DNS) */
static void *straddling_pass(void *unused)
{
	(void)unused;
	profile_trace_thread_register(_profile_track_p2p);
	fake_lock();
	profile_trace_begin(name_pass);
	fake_unlock();
	__atomic_store_n(&straddle_begun, 1, __ATOMIC_RELEASE);
	while (!__atomic_load_n(&straddle_released, __ATOMIC_ACQUIRE))
		usleep(100);
	fake_lock();
	profile_trace_end(name_pass);
	fake_unlock();
	return NULL;
}

static void straddle(int cut)
{
	pthread_t thread;
	struct profile_trace_status status;

	forget_written();
	straddle_begun = straddle_released = 0;
	profile_trace_request_start(0.0, _profile_trace_when_now, 4);
	frame(1);
	pthread_create(&thread, NULL, straddling_pass, NULL);
	while (!__atomic_load_n(&straddle_begun, __ATOMIC_ACQUIRE))
		usleep(100);
	if (cut)
	{
		/* frames until a cut comes, while the pass is out of its lock */
		do
		{
			frame(50);
			profile_trace_status(&status);
		}
		while (status.part < 2);
	}
	else
	{
		profile_trace_request_stop();
		profile_trace_frame_boundary();
	}
	__atomic_store_n(&straddle_released, 1, __ATOMIC_RELEASE);
	pthread_join(thread, NULL);
	if (cut)
	{
		profile_trace_request_stop();
		profile_trace_frame_boundary();
	}
	finish();
}

/* a pass begun in one recording and ended in the next (a stop, a re-arm and
a start between): its start is before the new origin, so it is dropped, and
counted in the new recording. concurrent: it ends while the start is made */
static void straddle_restart(int concurrent)
{
	pthread_t thread;

	forget_written();
	straddle_begun = straddle_released = 0;
	profile_trace_request_start(0.0, _profile_trace_when_now, 4);
	frame(1);
	pthread_create(&thread, NULL, straddling_pass, NULL);
	while (!__atomic_load_n(&straddle_begun, __ATOMIC_ACQUIRE))
		usleep(100);
	profile_trace_request_stop();
	profile_trace_frame_boundary();
	finish();
	forget_written();
	advance(1000000);
	profile_trace_request_start(0.0, _profile_trace_when_now, 4);
	if (concurrent)
	{
		__atomic_store_n(&straddle_released, 1, __ATOMIC_RELEASE);
		frame(1);
	}
	else
	{
		frame(1);
		__atomic_store_n(&straddle_released, 1, __ATOMIC_RELEASE);
	}
	pthread_join(thread, NULL);
	frame(1);
	profile_trace_request_stop();
	profile_trace_frame_boundary();
	finish();
}

static void straddle_checks(void)
{
	straddle(1);
	check(written.counts[name_pass] == 1, "a pass across a cut is in the part it ended in, once");
	straddle(0);
	check(written.counts[name_pass] == 0, "a pass that outlives its recording is not written");
	check(outstanding() == 0, "nothing left after a pass across a stop");
	straddle_restart(0);
	check(written.counts[name_pass] == 0, "a pass across a restart is dropped, not given a wrapped start");
	check(written.dropped == 1, "and counted in the new recording");
	straddle_restart(1);
	check(written.counts[name_pass] == 0, "a pass ending as the next recording starts is dropped");
	check(outstanding() == 0, "nothing left after a pass across a restart");
}

static void lifetime_checks(void)
{
	int cycle;

	/* many recordings, cut and not, stopped every way, with nothing left */
	for (cycle = 0; cycle < 40; cycle++)
	{
		int frames = cycle % 4 == 0 ? 2500 : 3;
		int index;

		forget_written();
		profile_trace_request_start(0.0, _profile_trace_when_now, 4);
		for (index = 0; index < frames; index++)
			frame(50);
		if (cycle % 3 == 0)
		{
			profile_trace_map_loaded();
			profile_trace_frame_boundary();
		}
		else if (cycle % 3 == 1)
		{
			profile_trace_request_stop();
			profile_trace_frame_boundary();
		}
		else
		{
			profile_trace_shutdown();
		}
		finish();
		if (outstanding() != 0)
		{
			check(0, "a recording leaves nothing allocated");
			break;
		}
	}
	check(allocations_made > 80, "the cycles allocated");
	/* exit while armed, while writing, and twice */
	profile_trace_request_start(0.0, _profile_trace_when_game, 4);
	fake_ready = 0;
	profile_trace_shutdown();
	profile_trace_shutdown();
	check(outstanding() == 0, "shutdown while armed keeps nothing");
	fake_ready = 1;
}

/* a real recording through the real writer, into the folder */
static void file_checks(const char *folder)
{
	struct profile_trace_status status;
	int frame_index;
	int quoted;

	fake_folder = folder;
	profile_trace_set_writer(NULL);
	profile_trace_set_session_sampler(fake_session_sample);
	quoted = profile_trace_name("quote\"back\\slash");
	fake_map = "";
	fake_map_name = "";
	profile_trace_request_start(0.0, _profile_trace_when_now, 4);
	fake_role = "client";
	fake_players = 5;
	fake_own_machine = 1;
	fake_map = "levels\\a30\\a30";
	fake_map_name = "a30";
	for (frame_index = 0; frame_index < 2400; frame_index++)
	{
		if (frame_index == 2)
		{
			fake_map = "levels\\b30\\b30";
			fake_map_name = "b30";
		}
		frame(40);
		profile_trace_begin(name_texture);
		profile_trace_begin(name_inner_aggregate);
		advance(100);
		profile_trace_end(name_inner_aggregate);
		profile_trace_begin(name_aggregate_child);
		advance(100);
		profile_trace_end(name_aggregate_child);
		profile_trace_end(name_texture);
		profile_trace_begin(quoted);
		advance(10);
		profile_trace_end(quoted);
		if (frame_index % 2 == 0)
		{
			profile_trace_begin(name_tick);
			profile_trace_tick();
			advance(100);
			profile_trace_end(name_tick);
		}
		advance(16000000);
	}
	profile_trace_shutdown();
	check(session_sample_calls > 0 && session_sample_calls < 2400,
		"session sampling is periodic and not per frame");
	fake_role = "host";
	fake_players = 2;
	fake_own_machine = -1;
	fake_map = "levels\\a30\\a30";
	fake_map_name = "a30";
	check(outstanding() == 0, "the real writer frees both arenas");
	profile_trace_status(&status);
	{
		char name[64];
		char path[512];
		FILE *file;

		profile_json_choose_name(folder, "20261005-142233", "host", name, sizeof(name));
		check(strcmp(name, "profile_20261005-142233_host") == 0, "a free name is taken as it is");
		snprintf(path, sizeof(path), "%s/%s.part1.json", folder, name);
		file = fopen(path, "wb");
		if (file)
			fclose(file);
		profile_json_choose_name(folder, "20261005-142233", "host", name, sizeof(name));
		check(strcmp(name, "profile_20261005-142233_host_2") == 0, "a name taken gets _2");
		remove(path);
	}
}

/* a folder that cannot be written: each part is logged and given up, its
.tmp gone, the arenas freed, and the recording still finishes */
static void unwritable_checks(const char *folder)
{
	char missing[512];
	int frame_index;

	snprintf(missing, sizeof(missing), "%s/missing/deeper", folder);
	fake_folder = missing;
	last_log[0] = 0;
	profile_trace_request_start(0.0, _profile_trace_when_now, 4);
	for (frame_index = 0; frame_index < 2400; frame_index++)
		frame(40);
	profile_trace_request_stop();
	profile_trace_frame_boundary();
	finish();
	check(strstr(last_log, "cannot write") && strstr(last_log, "missing/deeper"), "a part that cannot be written is logged");
	check(outstanding() == 0, "a part that cannot be written frees its arena");
	fake_folder = folder;
}

static int refusing_writer_start(void)
{
	return 0;
}

/* a start that fails after it has allocated: nothing is kept, it says so, and
a recording of "game" mode is not armed again to fail at every game */
static void release_path_checks(void)
{
	static const char *const what[] = { "the second arena refused", "the writer thread cannot start" };
	int fail, game;

	for (fail = 0; fail < 2; fail++)
	{
		for (game = 0; game < 2; game++)
		{
			struct profile_trace_status status;
			char text[96];

			snprintf(text, sizeof(text), "%s, %s mode", what[fail], game ? "game" : "now");
			forget_written();
			fake_ready = 1;
			last_notice[0] = 0;
			if (fail == 0)
				allocations_allowed = 1;
			else
				profile_trace_set_writer(refusing_writer_start);
			profile_trace_request_start(0.0, game ? _profile_trace_when_game : _profile_trace_when_now, 4);
			frame(1);
			allocations_allowed = -1;
			profile_trace_set_writer(fake_writer_start);
			profile_trace_status(&status);
			check(outstanding() == 0, text);
			check(status.state == _profile_trace_idle, text);
			check(strstr(last_notice, fail == 0 ? "no memory for 4 MB" : "the writer thread cannot start") != NULL, text);
			check(!profile_trace_recording(), text);
			frame(1);
			profile_trace_status(&status);
			check(status.state == _profile_trace_idle && outstanding() == 0, text);
		}
	}
}

/* a worst frame's ticks and longest scopes come from any depth of the frame:
game_tick sits under game_time_update, and the time a scope has of its own is
what ranks it (its parents would always come first otherwise) */
static void worst_frame_checks(void)
{
	struct profile_trace_record game[16];
	struct profile_part *part = calloc(1, sizeof(*part));
	int frame_id = profile_trace_name("frame");
	int update = profile_trace_name("game_time_update");
	int tick = profile_trace_name("game_tick");
	int objects = profile_trace_name("game_tick.objects");
	int ai = profile_trace_name("game_tick.ai");
	int render = profile_trace_name("render");
	int bsp = profile_trace_name("render.bsp");
	int texture = profile_trace_name("texture");
	char text[65536];
	unsigned long count = 0, length;
	FILE *file = tmpfile();
	const char *row;

	/* (milliseconds, as nanoseconds below) */
#define WORST_RECORD(start_ms, duration_ms, id, level) \
	do { game[count].start = (start_ms) * 1000000ULL; game[count].duration = (duration_ms) * 1000000U; \
		game[count].name = (unsigned short)(id); game[count].depth = (level); game[count].track = 0; count++; } while (0)
	WORST_RECORD(0, 100, frame_id, 0);
	WORST_RECORD(0, 60, update, 1);
	WORST_RECORD(0, 30, tick, 2);
	WORST_RECORD(0, 20, objects, 3);
	WORST_RECORD(30, 25, tick, 2);
	WORST_RECORD(30, 22, ai, 3);
	WORST_RECORD(60, 35, render, 1);
	WORST_RECORD(60, 10, bsp, 2);
	/* (an aggregate's time is a counter record, not a scope) */
	WORST_RECORD(5, 99, texture, PROFILE_TRACE_DEPTH_AGGREGATE_TIME);
	WORST_RECORD(100, 5, frame_id, 0);
#undef WORST_RECORD
	part->records[_profile_track_game] = game;
	part->record_counts[_profile_track_game] = count;
	part->name_count = profile_trace_name("worst_frame_last") + 1;
	part->ticks = 2;
	check(file != NULL, "a scratch file for the worst frames");
	if (!file)
	{
		free(part);
		return;
	}
	check(profile_json_write(part, file), "a part with nested scopes is written");
	rewind(file);
	length = fread(text, 1, sizeof(text) - 1, file);
	text[length] = 0;
	fclose(file);
	row = strstr(text, "\"worst_frames\"");
	check(row && strstr(row, "[0, 0.000, 100.000, 2, [[\"render\", 25.000], [\"game_tick.ai\", 22.000], "
		"[\"game_tick.objects\", 20.000]]]"),
		"the worst frame runs its two ticks under game_time_update and names the scopes with the most time of their own");
	check(row && strstr(row, "[1, 0.100, 5.000, 0, []]"), "a frame with no scopes in it has no ticks and no longest");
	free(part);
}

/* the reset race: a p2p pass begun in one recording ends, from inside
the next recording's allocator seam, before that start resets the tracks'
counts. Only the track lock orders its bump before the reset, which
ThreadSanitizer checks (the reset moved out of the lock is a race) */
static int race_armed;
static int race_ended;

static void *race_pass(void *unused)
{
	(void)unused;
	profile_trace_thread_register(_profile_track_p2p);
	fake_lock();
	profile_trace_begin(name_pass);
	fake_unlock();
	__atomic_store_n(&straddle_begun, 1, __ATOMIC_RELEASE);
	/* (relaxed: no happens-before from the game thread's release) */
	while (!__atomic_load_n(&straddle_released, __ATOMIC_RELAXED))
		usleep(100);
	fake_lock();
	profile_trace_end(name_pass);
	fake_unlock();
	__atomic_store_n(&race_ended, 1, __ATOMIC_RELAXED);
	return NULL;
}

static void *race_allocate(unsigned long size)
{
	if (__atomic_load_n(&race_armed, __ATOMIC_RELAXED))
	{
		__atomic_store_n(&race_armed, 0, __ATOMIC_RELAXED);
		__atomic_store_n(&straddle_released, 1, __ATOMIC_RELAXED);
		while (!__atomic_load_n(&race_ended, __ATOMIC_RELAXED))
			usleep(100);
	}
	return fake_allocate(size);
}

static void reset_race_checks(void)
{
	pthread_t thread;

	profile_trace_set_allocator(race_allocate, fake_release);
	forget_written();
	straddle_begun = straddle_released = 0;
	profile_trace_request_start(0.0, _profile_trace_when_now, 4);
	frame(1);
	pthread_create(&thread, NULL, race_pass, NULL);
	while (!__atomic_load_n(&straddle_begun, __ATOMIC_ACQUIRE))
		usleep(100);
	profile_trace_request_stop();
	profile_trace_frame_boundary();
	finish();

	forget_written();
	advance(1000000);
	__atomic_store_n(&race_armed, 1, __ATOMIC_RELAXED);
	profile_trace_request_start(0.0, _profile_trace_when_now, 4);
	frame(1);
	pthread_join(thread, NULL);
	frame(1);
	profile_trace_request_stop();
	profile_trace_frame_boundary();
	finish();
	check(written.counts[name_pass] == 0, "the pass of the last recording is not written");
	check(written.dropped == 0, "a pass ended before the start is not counted in the new recording");
	check(outstanding() == 0, "the race recordings leave nothing allocated");
	profile_trace_set_allocator(fake_allocate, fake_release);
}

int main(int argc, char **argv)
{
	profile_trace_set_clock(fake_clock);
	profile_trace_set_track_lock(fake_lock, fake_unlock);
	profile_trace_set_allocator(fake_allocate, fake_release);
	profile_trace_set_log(quiet_log);
	profile_trace_set_notify(fake_notify);
	profile_trace_set_writer(fake_writer_start);
	profile_trace_set_ready(fake_ready_test);
	profile_trace_set_session(fake_session);
	profile_trace_set_session_sampler(fake_session_sample);
	gametype_checks();
	profile_trace_thread_register(_profile_track_game);
	name_frame = profile_trace_name("frame");
	name_tick = profile_trace_name("game_tick");
	name_a = profile_trace_name("game_tick.objects");
	name_b = profile_trace_name("objects_update");
	name_c = profile_trace_name("deep");
	name_pass = profile_trace_name("p2p.pass");
	name_aggregate_child = profile_trace_name("aggregate_child");
	profile_trace_name_aggregate("texture");
	profile_trace_name_aggregate("render_model");
	name_texture = profile_trace_name("texture");
	name_inner_aggregate = profile_trace_name("render_model");
	fake_ready = 1;
	written.records_in_span = 1;

	scope_checks();
	state_checks();
	stop_answer_checks();
	part_checks();
	straddle_checks();
	lifetime_checks();
	worst_frame_checks();
	release_path_checks();
	reset_race_checks();
	if (argc > 1)
	{
		file_checks(argv[1]);
		unwritable_checks(argv[1]);
	}
	printf("%s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
	return failures != 0;
}
