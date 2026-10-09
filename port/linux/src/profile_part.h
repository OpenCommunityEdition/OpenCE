/*
PROFILE_PART.H

Define frozen recording parts shared by the recorder and writer.
*/

#ifndef __PROFILE_PART_H
#define __PROFILE_PART_H

#ifdef HALO_PROFILE

#include "profile_trace.h"

#include <stdio.h>

/* ---------- constants */

/* records that are not scopes, by their depth (a scope's is under
MAXIMUM_PROFILE_TRACE_DEPTH): an aggregate name's time and count over a
frame (start: the frame's) */
enum
{
	PROFILE_TRACE_DEPTH_AGGREGATE_TIME = 255,
	PROFILE_TRACE_DEPTH_AGGREGATE_COUNT = 254,
};

/* ---------- structures */

/* a scope that ended: 16 bytes */
struct profile_trace_record
{
	/* nanoseconds since the recording started */
	unsigned long long start;
	/* nanoseconds, saturating (or the counter's value) */
	unsigned int duration;
	unsigned short name;
	unsigned char depth;
	unsigned char track;
};

struct profile_part
{
	char name[64];
	char folder[260];
	long number;
	int last;
	int stop_reason;
	int arena;
	struct profile_trace_record *records[NUMBER_OF_PROFILE_TRACKS];
	unsigned long record_counts[NUMBER_OF_PROFILE_TRACKS];
	unsigned long long start_ns;
	unsigned long long end_ns;
	long first_frame;
	long frames;
	long first_tick;
	long ticks;
	long name_count;
	struct profile_trace_session session;
	char start_utc[32];
	double clock_read_ns;
	unsigned long memory_used;
	unsigned long memory_limit;
	double writer_wait_ms;
	unsigned long foreign_scopes;
	unsigned long deep_scopes;
	unsigned long unbalanced_scopes;
	unsigned long dropped_scopes;
};

typedef char profile_trace_record_size_assert[sizeof(struct profile_trace_record) == 16 ? 1 : -1];

/* ---------- prototypes/PROFILE_TRACE.C */

const char *profile_trace_name_text(int name);
int profile_trace_name_is_aggregate(int name);
/* the writer: the next part (waits for one), and a part written (its
arena back; after the last, both arenas freed and the recording over) */
struct profile_part *profile_trace_writer_next(void);
/* a line in the log (profile_trace_set_log) */
void profile_trace_log(const char *format, ...);
void profile_trace_writer_done(struct profile_part *part);

/* ---------- prototypes/PROFILE_JSON.C */

/* a part as a Chrome trace with the "halo" and "cpu_summary" keys; 0 when
the file could not be written */
int profile_json_write(struct profile_part *part, FILE *file);
/* Part names keep profile_<stamp>_<role>. */
void profile_json_choose_name(const char *folder, const char *stamp, const char *role, char *name, int size);
/* the writer thread of a recording: 0 when it cannot start */
int profile_json_writer_start(void);
/* a part's file: <folder>/<name>.part<n>.json, written as .tmp and renamed */
int profile_json_write_part(struct profile_part *part);

#endif

#endif
