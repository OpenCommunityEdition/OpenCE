/*
PROFILE_TRACE.H

Expose bounded CPU trace recording and session controls.
*/

#ifndef __PROFILE_TRACE_H
#define __PROFILE_TRACE_H

#ifdef HALO_PROFILE

/* included by C89 game units: keep declarations compatible with them. */

/* ---------- constants */

enum
{
	_profile_track_game = 0,
	_profile_track_p2p,
	NUMBER_OF_PROFILE_TRACKS,

	MAXIMUM_PROFILE_TRACE_NAMES = 512,
	MAXIMUM_PROFILE_TRACE_DEPTH = 64,
	MAXIMUM_PROFILE_TRACE_AGGREGATES = 32,
	/* debug.profile_memory: both arenas together, megabytes */
	DEFAULT_PROFILE_TRACE_MEMORY = 256,
	MINIMUM_PROFILE_TRACE_MEMORY = 4,
	MAXIMUM_PROFILE_TRACE_MEMORY = 1024,
};

enum profile_trace_state
{
	_profile_trace_idle = 0,
	_profile_trace_armed,
	_profile_trace_recording,
	_profile_trace_finishing,
};

/* when an armed recording starts: at the next frame, or once a game that is
not the main menu is in progress (debug.profile_record_when = "game") */
enum profile_trace_when
{
	_profile_trace_when_now = 0,
	_profile_trace_when_game,
};

enum profile_trace_stop_reason
{
	_profile_trace_stop_none = 0,
	_profile_trace_stop_command,
	_profile_trace_stop_seconds,
	_profile_trace_stop_map_load,
	_profile_trace_stop_exit,
};

enum profile_trace_answer
{
	_profile_trace_answer_armed = 0,
	_profile_trace_answer_already_recording,
	_profile_trace_answer_still_writing,
	_profile_trace_answer_stopping,
	_profile_trace_answer_disarmed,
	_profile_trace_answer_not_recording,
};

/* ---------- structures */

/* (the recording's identity at start; players_most grows while it runs) */
struct profile_trace_session
{
	char folder[260];
	char platform[16];
	char build[48];
	/* host, client or local */
	char role[8];
	/* a client's own machine index, -1 otherwise */
	long own_machine;
	char map[64];
	char map_name[64];
	char gametype[16];
	long players;
	long players_most;
};

struct profile_trace_status
{
	int state;
	char name[64];
	long part;
};

/* ---------- prototypes/PROFILE_TRACE.C */

/* the seams: set once at start-up by the game (profile_console.c), and by
tools/profile_check.c with fakes */
void profile_trace_set_clock(unsigned long long (*now_ns)(void));
void profile_trace_set_track_lock(void (*lock)(void), void (*unlock)(void));
void profile_trace_set_allocator(void *(*allocate)(unsigned long size), void (*release)(void *block));
void profile_trace_set_log(void (*log)(const char *text));
void profile_trace_set_notify(void (*notify)(const char *text));
/* starts the writer of a new recording (profile_json_writer_start): 0 when
it cannot */
void profile_trace_set_writer(int (*start)(void));
/* whether a game that is not the main menu is in progress */
void profile_trace_set_ready(int (*ready)(void));
void profile_trace_set_session(void (*describe)(struct profile_trace_session *session));
void profile_trace_set_session_sampler(void (*sample)(struct profile_trace_session *session));

/* which track the calling thread records on */
void profile_trace_thread_register(int track);

/* a name's id (the same name, the same id, for the whole run); an
aggregate name's scopes are summed per frame rather than recorded */
int profile_trace_name(const char *name);
void profile_trace_name_aggregate(const char *name);

void profile_trace_begin(int name);
void profile_trace_end(int name);

/* the game thread, with no scope open: where a recording starts, is cut
and stops */
void profile_trace_frame_boundary(void);
/* the game thread, at each game tick's start */
void profile_trace_tick(void);

/* the console's requests (the game thread), applied at the next frame
boundary; seconds 0 records until stopped */
int profile_trace_request_start(double seconds, int when, long memory_megabytes);
int profile_trace_request_stop(void);
/* a map starts loading: a recording stops, but for the first load after
launch (the main menu, or init.txt's map) */
void profile_trace_map_loaded(void);
/* the process ends: stops a recording, cuts its last part and waits for
the writer (main_exit, and atexit for the other ways out) */
void profile_trace_shutdown(void);

/* whether the tracks record now: changed under the track lock, so a p2p
pass sees one value */
int profile_trace_recording(void);
void profile_trace_status(struct profile_trace_status *status);

#endif

#endif
