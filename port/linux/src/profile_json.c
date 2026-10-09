/*
PROFILE_JSON.C

Write recording parts on the writer thread.
*/

#ifdef HALO_PROFILE

#include "profile_part.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

enum
{
	PROFILE_JSON_WORST_FRAMES = 10,
	PROFILE_JSON_LONGEST_CHILDREN = 3,
};

/* ---------- structures */

/* where the next item of a list goes: a comma and a line before all but
the first */
struct profile_json_list
{
	FILE *file;
	int items;
};

struct profile_json_name_totals
{
	unsigned long long count;
	unsigned long long total;
	unsigned long long maximum;
};

/* ---------- globals */

static const char *profile_json_stop_reasons[] = { "", "command", "seconds", "map_load", "exit" };

/* ---------- private code */

static void profile_json_item(
	struct profile_json_list *list)
{
	if (list->items++)
		fputs(",\n", list->file);
}

static void profile_json_escaped(
	FILE *file,
	const char *text)
{
	for (; text && *text; text++)
	{
		unsigned char character = (unsigned char)*text;

		if (character == '"' || character == '\\')
			fprintf(file, "\\%c", character);
		else if (character < 32 || character > 126)
			fprintf(file, "\\u%04x", character);
		else
			fputc(character, file);
	}
}

static void profile_json_string(
	FILE *file,
	const char *text)
{
	fputc('"', file);
	profile_json_escaped(file, text);
	fputc('"', file);
}

/* microseconds with three decimals, as the trace event format has them */
static void profile_json_time(
	FILE *file,
	unsigned long long nanoseconds)
{
	fprintf(file, "%llu.%03llu", nanoseconds / 1000ULL, nanoseconds % 1000ULL);
}

static int profile_json_compare_records(
	const void *a,
	const void *b)
{
	const struct profile_trace_record *first = a;
	const struct profile_trace_record *second = b;

	if (first->start != second->start)
		return first->start < second->start ? -1 : 1;
	return (int)first->depth - (int)second->depth;
}

static int profile_json_name_of(
	const struct profile_part *part,
	const char *text)
{
	int name;

	for (name = 0; name < part->name_count; name++)
	{
		if (strcmp(profile_trace_name_text(name), text) == 0)
			return name;
	}
	return -1;
}

static void profile_json_header(
	const struct profile_part *part,
	FILE *file)
{
	const struct profile_trace_session *session = &part->session;

	fputs("\"header\": {\"format\": 1, \"build\": ", file);
	profile_json_string(file, session->build);
	fputs(", \"platform\": ", file);
	profile_json_string(file, session->platform);
	fputs(", \"role\": ", file);
	profile_json_string(file, session->role);
	fprintf(file, ", \"own_machine\": %ld, \"map\": ", session->own_machine);
	profile_json_string(file, session->map);
	fputs(", \"map_name\": ", file);
	profile_json_string(file, session->map_name);
	fputs(", \"gametype\": ", file);
	profile_json_string(file, session->gametype);
	fprintf(file, ", \"players\": %ld, \"players_most\": %ld, \"start_utc\": ", session->players,
		session->players_most);
	profile_json_string(file, part->start_utc);
	fputs(", \"recording\": ", file);
	profile_json_string(file, part->name);
	fprintf(file, ", \"part\": %ld, \"last_part\": %s"
		", \"first_frame\": %ld, \"frames\": %ld, \"first_tick\": %ld, \"ticks\": %ld",
		part->number, part->last ? "true" : "false",
		part->first_frame, part->frames, part->first_tick, part->ticks);
	fprintf(file, ", \"start_s\": %.6f, \"duration_s\": %.6f, \"stop_reason\": ",
		(double)part->start_ns / 1e9, (double)(part->end_ns - part->start_ns) / 1e9);
	profile_json_string(file, profile_json_stop_reasons[part->stop_reason]);
	fprintf(file, ", \"clock_read_ns\": %.1f, \"memory_used\": %lu, \"memory_limit\": %lu, \"writer_wait_ms\": %.3f"
		", \"foreign_scopes\": %lu, \"deep_scopes\": %lu, \"unbalanced_scopes\": %lu, \"dropped_scopes\": %lu}",
		part->clock_read_ns, part->memory_used, part->memory_limit, part->writer_wait_ms,
		part->foreign_scopes, part->deep_scopes, part->unbalanced_scopes, part->dropped_scopes);
}

/* a worst frame's scopes: the open ones (one a depth, so no more than the
depth limit) and the three with the most time of their own so far */
struct profile_json_ranking
{
	unsigned long scope[MAXIMUM_PROFILE_TRACE_DEPTH];
	unsigned long long children[MAXIMUM_PROFILE_TRACE_DEPTH];
	int open;
	unsigned long longest[PROFILE_JSON_LONGEST_CHILDREN];
	unsigned int own[PROFILE_JSON_LONGEST_CHILDREN];
	int count;
};

/* the innermost open scope ends: its time less its children's is ranked, and
all of it is its parent's child time */
static void profile_json_close(
	struct profile_json_ranking *ranking,
	const struct profile_trace_record *game)
{
	int open = --ranking->open;
	unsigned long scope = ranking->scope[open];
	unsigned long long duration = game[scope].duration;
	unsigned int own = (unsigned int)(duration > ranking->children[open] ? duration - ranking->children[open] : 0);
	int slot;

	for (slot = ranking->count; slot > 0 && ranking->own[slot - 1] < own; slot--)
	{
		if (slot < PROFILE_JSON_LONGEST_CHILDREN)
		{
			ranking->longest[slot] = ranking->longest[slot - 1];
			ranking->own[slot] = ranking->own[slot - 1];
		}
	}
	if (slot < PROFILE_JSON_LONGEST_CHILDREN)
	{
		ranking->longest[slot] = scope;
		ranking->own[slot] = own;
		if (ranking->count < PROFILE_JSON_LONGEST_CHILDREN)
			ranking->count++;
	}
	if (open > 0)
		ranking->children[open - 1] += duration;
}

static void profile_json_cpu_summary(
	const struct profile_part *part,
	FILE *file)
{
	struct profile_json_name_totals totals[MAXIMUM_PROFILE_TRACE_NAMES];
	const struct profile_trace_record *game = part->records[_profile_track_game];
	unsigned long game_count = part->record_counts[_profile_track_game];
	unsigned long worst[PROFILE_JSON_WORST_FRAMES];
	long worst_frame_number[PROFILE_JSON_WORST_FRAMES];
	int worst_count = 0;
	struct profile_json_ranking ranking;
	int frame_name = profile_json_name_of(part, "frame");
	int tick_name = profile_json_name_of(part, "game_tick");
	struct profile_json_list list;
	unsigned long index;
	long frames = 0;
	int track, name;

	memset(totals, 0, sizeof(totals));
	for (track = 0; track < NUMBER_OF_PROFILE_TRACKS; track++)
	{
		for (index = 0; index < part->record_counts[track]; index++)
		{
			const struct profile_trace_record *record = &part->records[track][index];
			struct profile_json_name_totals *name_totals;

			if (record->name >= MAXIMUM_PROFILE_TRACE_NAMES)
				continue;
			name_totals = &totals[record->name];
			if (record->depth < MAXIMUM_PROFILE_TRACE_DEPTH || record->depth == PROFILE_TRACE_DEPTH_AGGREGATE_TIME)
			{
				name_totals->total += record->duration;
				if (record->duration > name_totals->maximum)
					name_totals->maximum = record->duration;
			}
			if (record->depth < MAXIMUM_PROFILE_TRACE_DEPTH)
				name_totals->count++;
			else if (record->depth == PROFILE_TRACE_DEPTH_AGGREGATE_COUNT)
				name_totals->count += record->duration;
		}
	}
	/* the worst frames: the game track is sorted, so a frame's children
	follow it */
	for (index = 0; index < game_count; index++)
	{
		int slot;

		if (game[index].depth >= MAXIMUM_PROFILE_TRACE_DEPTH || game[index].name != frame_name)
			continue;
		frames++;
		for (slot = worst_count; slot > 0 && game[worst[slot - 1]].duration < game[index].duration; slot--)
		{
			if (slot < PROFILE_JSON_WORST_FRAMES)
			{
				worst[slot] = worst[slot - 1];
				worst_frame_number[slot] = worst_frame_number[slot - 1];
			}
		}
		if (slot < PROFILE_JSON_WORST_FRAMES)
		{
			worst[slot] = index;
			worst_frame_number[slot] = part->first_frame + frames - 1;
			if (worst_count < PROFILE_JSON_WORST_FRAMES)
				worst_count++;
		}
	}

	fprintf(file, "{\"frames\": %ld, \"ticks\": %ld, \"columns\": [\"name\", \"count\", \"total_ms\", "
		"\"mean_ms\", \"max_ms\", \"per_frame\", \"per_tick_ms\"], \"rows\": [\n", frames, part->ticks);
	list.file = file;
	list.items = 0;
	for (name = 0; name < part->name_count && name < MAXIMUM_PROFILE_TRACE_NAMES; name++)
	{
		const char *text = profile_trace_name_text(name);
		struct profile_json_name_totals const *name_totals = &totals[name];
		int per_tick = strncmp(text, "game_tick", 9) == 0 || strncmp(text, "network_distributed_tick", 24) == 0;

		if (!name_totals->count)
			continue;
		profile_json_item(&list);
		fputc('[', file);
		profile_json_string(file, text);
		fprintf(file, ", %llu, %.3f, %.4f, %.3f, %.3f, ", name_totals->count, name_totals->total / 1e6,
			name_totals->total / 1e6 / (double)name_totals->count, name_totals->maximum / 1e6,
			frames ? (double)name_totals->count / (double)frames : 0.0);
		if (per_tick && part->ticks)
			fprintf(file, "%.4f]", name_totals->total / 1e6 / (double)part->ticks);
		else
			fputs("null]", file);
	}
	fputs(list.items ? "\n]" : "]", file);

	fputs(", \"worst_frames\": {\"columns\": [\"frame\", \"at_s\", \"frame_ms\", \"ticks\", \"longest\"], \"rows\": [\n", file);
	list.items = 0;
	for (index = 0; index < (unsigned long)worst_count; index++)
	{
		const struct profile_trace_record *frame = &game[worst[index]];
		unsigned long long end = frame->start + frame->duration;
		int ticks = 0;
		unsigned long child;
		int slot;

		/* (a tick sits under whatever the main loop wraps it in, so every
		depth of the frame counts; the scopes are ranked by the time they have
		of their own, or the parents, which hold all of it, would always come
		first. The open scopes are a stack, so nothing is allocated for it) */
		memset(&ranking, 0, sizeof(ranking));
		for (child = worst[index] + 1; child < game_count && game[child].start < end; child++)
		{
			if (game[child].depth >= MAXIMUM_PROFILE_TRACE_DEPTH || game[child].depth <= frame->depth)
				continue;
			if (game[child].name == tick_name)
				ticks++;
			while (ranking.open && game[ranking.scope[ranking.open - 1]].depth >= game[child].depth)
				profile_json_close(&ranking, game);
			ranking.scope[ranking.open] = child;
			ranking.children[ranking.open++] = 0;
		}
		while (ranking.open)
			profile_json_close(&ranking, game);
		profile_json_item(&list);
		fprintf(file, "[%ld, %.3f, %.3f, %d, [", worst_frame_number[index], frame->start / 1e9, frame->duration / 1e6,
			ticks);
		for (slot = 0; slot < ranking.count; slot++)
		{
			fputs(slot ? ", [" : "[", file);
			profile_json_string(file, profile_trace_name_text(game[ranking.longest[slot]].name));
			fprintf(file, ", %.3f]", ranking.own[slot] / 1e6);
		}
		fputs("]]", file);
	}
	fputs(list.items ? "\n]}}" : "]}}", file);
}

static void profile_json_event(
	FILE *file,
	struct profile_json_list *list,
	const struct profile_trace_record *record)
{
	const char *name = profile_trace_name_text(record->name);

	profile_json_item(list);
	switch (record->depth)
	{
	case PROFILE_TRACE_DEPTH_AGGREGATE_TIME:
	case PROFILE_TRACE_DEPTH_AGGREGATE_COUNT:
		fputs("{\"ph\":\"C\",\"name\":\"", file);
		profile_json_escaped(file, name);
		fputs(record->depth == PROFILE_TRACE_DEPTH_AGGREGATE_TIME ? "_ms\",\"pid\":1,\"tid\":1,\"ts\":" :
			"_count\",\"pid\":1,\"tid\":1,\"ts\":", file);
		profile_json_time(file, record->start);
		if (record->depth == PROFILE_TRACE_DEPTH_AGGREGATE_TIME)
			fprintf(file, ",\"args\":{\"value\":%.4f}}", record->duration / 1e6);
		else
			fprintf(file, ",\"args\":{\"value\":%u}}", record->duration);
		break;
	default:
		fputs("{\"ph\":\"X\",\"name\":", file);
		profile_json_string(file, name);
		fprintf(file, ",\"pid\":1,\"tid\":%d,\"ts\":", record->track + 1);
		profile_json_time(file, record->start);
		fputs(",\"dur\":", file);
		profile_json_time(file, record->duration);
		fputc('}', file);
		break;
	}
}

/* ---------- public code */

int profile_json_write(
	struct profile_part *part,
	FILE *file)
{
	struct profile_json_list list;
	unsigned long next[NUMBER_OF_PROFILE_TRACKS] = { 0, 0 };
	int track;

	for (track = 0; track < NUMBER_OF_PROFILE_TRACKS; track++)
	{
		if (part->record_counts[track])
		{
			qsort(part->records[track], part->record_counts[track], sizeof(struct profile_trace_record),
				profile_json_compare_records);
		}
	}

	fputs("{\"halo\": {\n", file);
	profile_json_header(part, file);
	fputs("},\n\"cpu_summary\": ", file);
	profile_json_cpu_summary(part, file);
	fputs(",\n\"traceEvents\": [\n", file);

	list.file = file;
	list.items = 0;
	profile_json_item(&list);
	fprintf(file, "{\"ph\":\"M\",\"name\":\"process_name\",\"pid\":1,\"args\":{\"name\":\"halo %s %s\"}}",
		part->session.role, part->session.platform);
	profile_json_item(&list);
	fputs("{\"ph\":\"M\",\"name\":\"thread_name\",\"pid\":1,\"tid\":1,\"args\":{\"name\":\"game\"}}", file);
	profile_json_item(&list);
	fputs("{\"ph\":\"M\",\"name\":\"thread_name\",\"pid\":1,\"tid\":2,\"args\":{\"name\":\"p2p\"}}", file);
	if (part->number == 1)
	{
		profile_json_item(&list);
		fputs("{\"ph\":\"i\",\"name\":\"recording_start\",\"s\":\"g\",\"pid\":1,\"tid\":1,\"ts\":0.000}", file);
	}
	/* the two tracks merged, in order of start and depth: each scope after
	its parents */
	for (;;)
	{
		const struct profile_trace_record *game = next[0] < part->record_counts[0] ? &part->records[0][next[0]] : NULL;
		const struct profile_trace_record *p2p = next[1] < part->record_counts[1] ? &part->records[1][next[1]] : NULL;

		if (!game && !p2p)
			break;
		if (game && (!p2p || profile_json_compare_records(game, p2p) <= 0))
		{
			profile_json_event(file, &list, game);
			next[0]++;
		}
		else
		{
			profile_json_event(file, &list, p2p);
			next[1]++;
		}
	}
	if (part->last)
	{
		profile_json_item(&list);
		fputs("{\"ph\":\"i\",\"name\":\"recording_stop\",\"s\":\"g\",\"pid\":1,\"tid\":1,\"ts\":", file);
		profile_json_time(file, part->end_ns);
		fprintf(file, ",\"args\":{\"reason\":\"%s\"}}", profile_json_stop_reasons[part->stop_reason]);
	}
	fputs("\n],\n\"displayTimeUnit\": \"ms\"}\n", file);
	return !ferror(file);
}

void profile_json_choose_name(
	const char *folder,
	const char *stamp,
	const char *role,
	char *name,
	int size)
{
	char path[512];
	int suffix;

	snprintf(name, (size_t)size, "profile_%s_%s", stamp, role);
	for (suffix = 2; suffix < 1000; suffix++)
	{
		FILE *file;
		int occupied = 0;

		snprintf(path, sizeof(path), "%s/%s.part1.json", folder, name);
		file = fopen(path, "rb");
		if (file)
		{
			fclose(file);
			occupied = 1;
		}
		if (!occupied)
			break;
		snprintf(name, (size_t)size, "profile_%s_%s_%d", stamp, role, suffix);
	}
}

int profile_json_write_part(
	struct profile_part *part)
{
	char temporary[520];
	char path[512];
	FILE *file;
	int written;

	snprintf(path, sizeof(path), "%s/%s.part%ld.json", part->folder, part->name, part->number);
	snprintf(temporary, sizeof(temporary), "%s.tmp", path);
	file = fopen(temporary, "wb");
	if (!file)
	{
		profile_trace_log("cannot write %s", temporary);
		return 0;
	}
	written = profile_json_write(part, file);
	if (fclose(file) != 0)
		written = 0;
	/* (Windows' rename does not replace a file: the recording's name was
	chosen not to be there) */
	if (written && rename(temporary, path) != 0)
		written = 0;
	if (!written)
	{
		remove(temporary);
		profile_trace_log("cannot write %s", path);
		return 0;
	}
	profile_trace_log("wrote %s", path);
	return 1;
}

static void *profile_json_writer(
	void *unused)
{
	(void)unused;
	for (;;)
	{
		struct profile_part *part = profile_trace_writer_next();
		int last = part->last;
		profile_json_write_part(part);
		profile_trace_writer_done(part);
		if (last)
			break;
	}
	return NULL;
}

int profile_json_writer_start(
	void)
{
	pthread_attr_t attributes;
	pthread_t thread;
	int started;

	pthread_attr_init(&attributes);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	started = pthread_create(&thread, &attributes, profile_json_writer, NULL) == 0;
	pthread_attr_destroy(&attributes);
	return started;
}

#endif
