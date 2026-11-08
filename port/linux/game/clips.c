/* Records a campaign level or a multiplayer game, one tick at a time, and
plays that tape back from the clips screen. The file is the
players' actions and, for whoever sat at this machine, the pad, the keys
and the mouse. A checkpoint every ten seconds is a hash of where the
players stood. Left and right step one tick, and a hold steps faster
the longer it is held. Backward loads the level again and runs up to
that tick. A hash that does not match stops on the last tick that did, when the
clip was recorded on this build. A clip from another build plays on. */

#include "cseries.h"
#include "clips.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "game/local_players.h"
#include "game/player_queues_new.h"
#include "input/input.h"
#include "main/main.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "camera/director.h"
#include "cutscene/cinematics.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "tag_files/tag_groups.h"
#include "text/draw_string.h"
#include "text/unicode.h"
#include "interface/player_ui.h"
#include "interface/hud.h"
#include "items/item_definitions.h"
#include "networking/network_game_globals.h"
#include "halo_keyboard.h"
#include "units/units.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef HALO_BUILD_NUMBER
#define HALO_BUILD_NUMBER 0
#endif

#define CLIPS_PLAYERS 16
#define CLIPS_VERSION 1
#define CLIPS_VISIBLE 8
#define CLIPS_LIST 64
#define CLIPS_KEEP_DEFAULT 20
#define CLIPS_KEEP_MIN 1
#define CLIPS_KEEP_MAX CLIPS_LIST
#define CLIPS_DISK_CAP (256L * 1024L * 1024L)
#define CLIPS_CHECKPOINT 300

extern int __stdcall CreateDirectoryA(const char *path, void *security);
extern void halo_linux_mouse_sample(short *x, short *y);
extern void halo_linux_pad_sample(short port, short *lx, short *ly, short *rx, short *ry,
	unsigned char *left_trigger, unsigned char *right_trigger, unsigned short *buttons,
	unsigned char *face);
/* space, left, right, [, ], V, escape, C, up, down:
bits 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 */
extern unsigned long halo_linux_theatre_keys(void);
extern unsigned long system_milliseconds(void);
extern int halo_linux_mouse_is_aim_device(short gamepad_index);

void game_loaded_setup(char *map_name, long map_name_size, short *difficulty, unsigned long *random_seed);

#pragma pack(push, 1)
struct clip_player_disk
{
	unsigned short name[12];
	short color;
	short team;
	short local_seat;
	long profile;
	unsigned char kind;
	unsigned short absolute;
	unsigned char pad;
};

struct clip_header_disk
{
	char magic[4];
	long version;
	long build;
	long kind;
	long difficulty;
	unsigned long seed;
	char map[128];
	char title[64];
	long ticks;
	long player_count;
	long recorded_at;
	struct clip_player_disk players[CLIPS_PLAYERS];
	unsigned char variant[0x68];
	unsigned char options[0x1C];
};

struct clip_sample_disk
{
	short slot;
	struct player_action action;
	unsigned char kind;
	unsigned long keys;
	short lx, ly, rx, ry;
	unsigned char lt, rt;
	unsigned short buttons;
	unsigned char face;
	unsigned char pad0;
	short mouse_x, mouse_y;
};
#pragma pack(pop)

struct clip_row
{
	char file[64];
	char map[64];
	char title[48];
	long ticks;
	long when;
	long kind;
};

static struct
{
	int recording;
	int replaying;
	int return_menu;
	int restore;
	int spawned;
	int stopped;
	int ended;
	int show_inputs;
	/* the free camera's stream of every player's text, on the picture control */
	int show_feed;
	int flying;
	int camera_mode;
	int watch;
	int hold_camera;
	int arm;
	int seeking;
	long seek_tick;
	float resume_speed;
	int rewinding;
	long rewind_to;
	int speed_index;
	int scrubbing;
	int scrub_dir;
	/* the scrub started while the clip was still running, so the hold
	steps at once instead of waiting out a tap */
	int scrub_from_play;
	/* backward hold: the target walks back from the tick it started at */
	int scrub_aimed;
	int scrub_wait;
	int scrub_limit;
	int scrub_exact;
	long scrub_allow;
	/* the sample on screen, and 1 when the next sample is still that tick */
	long play_tick;
	int hold_clock;
	long scrub_target;
	unsigned long scrub_mark;
	unsigned long scrub_clock;
	float scrub_debt;
	long profile;
	char path[260];
	char pending[260];
	char file_name[64];
	FILE *file;
	struct clip_header_disk header;
	int header_ok;
	long slot_datum[CLIPS_PLAYERS];
	int slot_count;
	struct clip_sample_disk last[CLIPS_PLAYERS];
	int last_count;
	short live_mouse_x;
	short live_mouse_y;
	short trail_x[CLIPS_PLAYERS][8];
	short trail_y[CLIPS_PLAYERS][8];
	int trail_i[CLIPS_PLAYERS];
	int trail_n[CLIPS_PLAYERS];
	int input_seen[CLIPS_PLAYERS];
	int menu_ready;
	int menu_count;
	int menu_offset;
	int menu_focus;
	struct clip_row rows[CLIPS_LIST];
	char message[160];
} clips;

static float const clips_speeds[5] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };

static void clips_dirs(void)
{
	CreateDirectoryA("z:\\saved", NULL);
	CreateDirectoryA("z:\\saved\\clips", NULL);
	CreateDirectoryA("z:\\saved\\clips\\files", NULL);
	CreateDirectoryA("z:\\saved\\clips\\profiles", NULL);
}

static void clips_copy(char *dst, long n, char const *src)
{
	long i;

	if (!dst || n < 1)
		return;
	if (!src)
		src = "";
	for (i = 0; i < n - 1 && src[i]; i++)
		dst[i] = src[i];
	dst[i] = 0;
}

static void clips_widen(wchar_t *dst, long n, char const *src)
{
	long i;

	if (!dst || n < 1)
		return;
	if (!src)
		src = "";
	for (i = 0; i < n - 1 && src[i]; i++)
		dst[i] = (wchar_t)(unsigned char)src[i];
	dst[i] = 0;
}

static int clips_write(void const *data, long size)
{
	if (!clips.file || size < 0)
		return 0;
	return fwrite(data, 1, (size_t)size, clips.file) == (size_t)size;
}

static int clips_read(void *data, long size)
{
	if (!clips.file || size < 0)
		return 0;
	return fread(data, 1, (size_t)size, clips.file) == (size_t)size;
}

static int clips_read_header(char const *path, struct clip_header_disk *header)
{
	FILE *file;
	int ok;

	csmemset(header, 0, sizeof(*header));
	file = fopen(path, "rb");
	if (!file)
		return 0;
	ok = fread(header, 1, sizeof(*header), file) == sizeof(*header);
	fclose(file);
	if (!ok || header->magic[0] != 'C' || header->magic[1] != 'L' ||
		header->magic[2] != 'I' || header->magic[3] != 'P' ||
		header->version != CLIPS_VERSION)
		return 0;
	if (header->player_count < 0 || header->player_count > CLIPS_PLAYERS)
		return 0;
	header->map[sizeof(header->map) - 1] = 0;
	header->title[sizeof(header->title) - 1] = 0;
	return 1;
}

static void clips_close_file(void)
{
	if (clips.file)
		fclose(clips.file);
	clips.file = NULL;
}

static unsigned long clips_mix(unsigned long hash, unsigned long value)
{
	hash ^= value;
	hash *= 16777619UL;
	return hash;
}

static unsigned long clips_hash(long tick)
{
	unsigned long hash = 2166136261UL;
	int slot;

	/* the seat, then where that seat stood. playback's players are not
	created in the order the match listed them */
	hash = clips_mix(hash, (unsigned long)tick);
	hash = clips_mix(hash, (unsigned long)clips.slot_count);
	for (slot = 0; slot < clips.slot_count && slot < CLIPS_PLAYERS; slot++)
	{
		struct player_datum *player;
		real_point3d origin;
		long x, y, z;
		long datum;

		datum = clips.slot_datum[slot];
		player = datum == NONE ? NULL : player_try_and_get(datum);
		hash = clips_mix(hash, (unsigned long)slot);
		if (!player || player->unit_index == NONE)
		{
			hash = clips_mix(hash, 0);
			continue;
		}
		object_get_origin(player->unit_index, &origin);
		x = (long)(origin.x * 100.0f);
		y = (long)(origin.y * 100.0f);
		z = (long)(origin.z * 100.0f);
		hash = clips_mix(hash, (unsigned long)x);
		hash = clips_mix(hash, (unsigned long)y);
		hash = clips_mix(hash, (unsigned long)z);
	}
	return hash;
}

static void clips_title(char *title, long n, int multiplayer)
{
	struct game_variant *variant;
	long i;

	title[0] = 0;
	if (!multiplayer)
	{
		char const *name = "";

		switch (clips.header.difficulty)
		{
		case 0: name = "Easy"; break;
		case 1: name = "Normal"; break;
		case 2: name = "Heroic"; break;
		case 3: name = "Legendary"; break;
		}
		clips_copy(title, n, name);
		return;
	}
	variant = game_engine_get_variant();
	if (!variant)
		return;
	for (i = 0; i < n - 1 && i < 12 && variant->human_readable_game_description[i]; i++)
		title[i] = (char)(variant->human_readable_game_description[i] & 0x7f);
	title[i] = 0;
}

static void clips_safe_name(char *dst, long n, char const *src)
{
	long i;

	if (n < 2)
		return;
	if (!src || !src[0])
		src = "clip";
	for (i = 0; i < n - 1 && src[i]; i++)
	{
		char c = src[i];

		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9'))
			dst[i] = c;
		else
			dst[i] = '_';
	}
	dst[i] = 0;
}

static int clips_profile_ok(long profile)
{
	return profile != NONE;
}

static void clips_registry_add(unsigned long profile)
{
	FILE *file;
	char line[64];
	char current[64];
	int found = 0;

	snprintf(current, sizeof(current), "%08lx", profile);
	file = fopen("z:\\saved\\clips\\profiles\\list.txt", "r");
	if (file)
	{
		while (fgets(line, sizeof(line), file))
		{
			if (!strncmp(line, current, 8))
				found = 1;
		}
		fclose(file);
	}
	if (found)
		return;
	file = fopen("z:\\saved\\clips\\profiles\\list.txt", "a");
	if (!file)
		return;
	fprintf(file, "%s\n", current);
	fclose(file);
}

static void clips_keep_enforce(unsigned long profile);

static void clips_index_add(unsigned long profile, char const *file_name)
{
	char path[260];
	char map[64];
	char title[48];
	char line[256];
	FILE *file;
	long size;
	char *body;

	snprintf(path, sizeof(path), "z:\\saved\\clips\\profiles\\%08lx.txt", profile);
	clips_copy(map, sizeof(map), clips.header.map);
	clips_copy(title, sizeof(title), clips.header.title[0] ? clips.header.title : clips.header.map);
	{
		char *bar;

		for (bar = map; *bar; bar++)
			if (*bar == '|')
				*bar = ' ';
		for (bar = title; *bar; bar++)
			if (*bar == '|')
				*bar = ' ';
	}
	snprintf(line, sizeof(line), "%s|%ld|%ld|%ld|%s|%s\n",
		file_name, clips.header.ticks, clips.header.kind, clips.header.recorded_at, map, title);
	body = NULL;
	size = 0;
	file = fopen(path, "rb");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		if (size < 0)
			size = 0;
		if (size > 256 * 1024)
			size = 256 * 1024;
		fseek(file, 0, SEEK_SET);
		body = (char *)malloc((size_t)size + 1);
		if (body && size)
			fread(body, 1, (size_t)size, file);
		if (body)
			body[size] = 0;
		fclose(file);
	}
	file = fopen(path, "wb");
	if (file)
	{
		fputs(line, file);
		if (body)
			fwrite(body, 1, (size_t)size, file);
		fclose(file);
	}
	if (body)
		free(body);
	clips_registry_add(profile);
	clips_keep_enforce(profile);
	clips.menu_ready = 0;
}

static int clips_file_listed(char const *file_name)
{
	FILE *registry;
	char line[64];

	registry = fopen("z:\\saved\\clips\\profiles\\list.txt", "r");
	if (!registry)
		return 0;
	while (fgets(line, sizeof(line), registry))
	{
		char path[260];
		FILE *index;
		char row[256];
		unsigned long profile = 0;

		if (sscanf(line, "%lx", &profile) != 1)
			continue;
		snprintf(path, sizeof(path), "z:\\saved\\clips\\profiles\\%08lx.txt", profile);
		index = fopen(path, "r");
		if (!index)
			continue;
		while (fgets(row, sizeof(row), index))
		{
			char *bar = strchr(row, '|');
			int n;

			if (bar)
				*bar = 0;
			n = (int)strlen(row);
			while (n > 0 && (row[n - 1] == '\n' || row[n - 1] == '\r'))
				row[--n] = 0;
			if (!strcmp(row, file_name))
			{
				fclose(index);
				fclose(registry);
				return 1;
			}
		}
		fclose(index);
	}
	fclose(registry);
	return 0;
}

static void clips_catalog_add(long bytes)
{
	FILE *file;

	file = fopen("z:\\saved\\clips\\catalog.txt", "a");
	if (!file)
		return;
	fprintf(file, "%ld %s\n", bytes, clips.file_name);
	fclose(file);
}

static void clips_catalog_remove(char const *file_name)
{
	FILE *file;
	char line[256];
	char *body;
	long kept = 0;

	file = fopen("z:\\saved\\clips\\catalog.txt", "r");
	if (!file)
		return;
	body = (char *)malloc(64 * 1024);
	if (!body)
	{
		fclose(file);
		return;
	}
	body[0] = 0;
	while (fgets(line, sizeof(line), file) && kept < 60 * 1024)
	{
		char name[64];
		long bytes = 0;

		if (sscanf(line, "%ld %63s", &bytes, name) != 2)
			continue;
		if (!strcmp(name, file_name))
			continue;
		kept += (long)strlen(line);
		if (kept < 60 * 1024)
			strcat(body, line);
	}
	fclose(file);
	file = fopen("z:\\saved\\clips\\catalog.txt", "w");
	if (file)
	{
		fputs(body, file);
		fclose(file);
	}
	free(body);
}

static void clips_delete_file(char const *file_name)
{
	char path[260];

	if (clips_file_listed(file_name))
		return;
	snprintf(path, sizeof(path), "z:\\saved\\clips\\files\\%s", file_name);
	remove(path);
	clips_catalog_remove(file_name);
}

/* the number is stored beside the profile's clip list. a missing file is
the default, so a profile that has never set it still has a limit */
static int clips_keep_count(unsigned long profile)
{
	char path[260];
	FILE *file;
	char line[32];
	int count;

	count = CLIPS_KEEP_DEFAULT;
	snprintf(path, sizeof(path), "z:\\saved\\clips\\profiles\\%08lx.keep", profile);
	file = fopen(path, "r");
	if (!file)
		return count;
	if (fgets(line, sizeof(line), file))
	{
		if (sscanf(line, "%d", &count) != 1)
			count = CLIPS_KEEP_DEFAULT;
	}
	fclose(file);
	if (count < CLIPS_KEEP_MIN)
		count = CLIPS_KEEP_MIN;
	if (count > CLIPS_KEEP_MAX)
		count = CLIPS_KEEP_MAX;
	return count;
}

static void clips_keep_write(unsigned long profile, int count)
{
	char path[260];
	FILE *file;

	clips_dirs();
	snprintf(path, sizeof(path), "z:\\saved\\clips\\profiles\\%08lx.keep", profile);
	file = fopen(path, "wb");
	if (!file)
		return;
	fprintf(file, "%d\n", count);
	fclose(file);
}

/* newest lines are first. keep that many and drop the rest. a file goes
away only when no profile still lists it */
static void clips_keep_enforce(unsigned long profile)
{
	char path[260];
	FILE *file;
	char line[256];
	char *kept;
	char *drop;
	int keep;
	int seen;
	long kept_n;
	long drop_n;

	keep = clips_keep_count(profile);
	snprintf(path, sizeof(path), "z:\\saved\\clips\\profiles\\%08lx.txt", profile);
	file = fopen(path, "r");
	if (!file)
		return;
	kept = (char *)malloc(256 * 1024);
	drop = (char *)malloc(256 * 1024);
	if (!kept || !drop)
	{
		if (kept)
			free(kept);
		if (drop)
			free(drop);
		fclose(file);
		return;
	}
	kept[0] = 0;
	drop[0] = 0;
	seen = 0;
	kept_n = 0;
	drop_n = 0;
	while (fgets(line, sizeof(line), file))
	{
		char name[64];
		char *bar;
		int n;
		long len;

		if (line[0] == '\n' || line[0] == '\r')
			continue;
		len = (long)strlen(line);
		if (seen < keep)
		{
			if (kept_n + len < 250 * 1024)
			{
				strcat(kept, line);
				kept_n += len;
			}
			seen++;
			continue;
		}
		clips_copy(name, sizeof(name), line);
		bar = strchr(name, '|');
		if (bar)
			*bar = 0;
		n = (int)strlen(name);
		while (n > 0 && (name[n - 1] == '\n' || name[n - 1] == '\r'))
			name[--n] = 0;
		if (name[0] && drop_n + n + 2 < 250 * 1024)
		{
			strcat(drop, name);
			strcat(drop, "\n");
			drop_n += n + 1;
		}
		seen++;
	}
	fclose(file);
	if (seen > keep)
	{
		char *cursor;

		file = fopen(path, "wb");
		if (file)
		{
			fputs(kept, file);
			fclose(file);
		}
		cursor = drop;
		while (cursor[0])
		{
			char name[64];
			char *nl;
			int n;

			nl = strchr(cursor, '\n');
			if (nl)
				*nl = 0;
			clips_copy(name, sizeof(name), cursor);
			n = (int)strlen(name);
			while (n > 0 && name[n - 1] == '\r')
				name[--n] = 0;
			if (name[0])
				clips_delete_file(name);
			if (!nl)
				break;
			cursor = nl + 1;
		}
		clips.menu_ready = 0;
	}
	free(kept);
	free(drop);
}

int clips_menu_keep(void)
{
	if (!clips_profile_ok(clips.profile))
		return CLIPS_KEEP_DEFAULT;
	return clips_keep_count((unsigned long)clips.profile);
}

void clips_menu_keep_adjust(int delta)
{
	unsigned long profile;
	int count;
	int next;

	if (!clips_profile_ok(clips.profile))
		return;
	profile = (unsigned long)clips.profile;
	count = clips_keep_count(profile);
	next = count + delta;
	if (next < CLIPS_KEEP_MIN)
		next = CLIPS_KEEP_MIN;
	if (next > CLIPS_KEEP_MAX)
		next = CLIPS_KEEP_MAX;
	if (next == count)
		return;
	clips_keep_write(profile, next);
	clips_keep_enforce(profile);
	clips.menu_ready = 0;
}

static void clips_unlist_everywhere(char const *file_name);

static void clips_enforce_cap(void)
{
	FILE *file;
	char line[256];
	long total = 0;
	int guard;

	for (guard = 0; guard < 64; guard++)
	{
		char name[64];
		long bytes;
		char *body;
		int dropped = 0;

		total = 0;
		file = fopen("z:\\saved\\clips\\catalog.txt", "r");
		if (!file)
			return;
		while (fgets(line, sizeof(line), file))
		{
			long size = 0;

			if (sscanf(line, "%ld", &size) == 1 && size > 0)
				total += size;
		}
		fclose(file);
		if (total <= CLIPS_DISK_CAP)
			return;
		file = fopen("z:\\saved\\clips\\catalog.txt", "r");
		if (!file)
			return;
		name[0] = 0;
		bytes = 0;
		if (fgets(line, sizeof(line), file))
			sscanf(line, "%ld %63s", &bytes, name);
		body = (char *)malloc(64 * 1024);
		if (!body)
		{
			fclose(file);
			return;
		}
		body[0] = 0;
		while (fgets(line, sizeof(line), file))
		{
			if (strlen(body) + strlen(line) < 60 * 1024)
				strcat(body, line);
		}
		fclose(file);
		file = fopen("z:\\saved\\clips\\catalog.txt", "w");
		if (file)
		{
			fputs(body, file);
			fclose(file);
		}
		free(body);
		if (name[0])
		{
			char path[260];

			clips_unlist_everywhere(name);
			snprintf(path, sizeof(path), "z:\\saved\\clips\\files\\%s", name);
			remove(path);
			dropped = 1;
		}
		if (!dropped)
			return;
	}
}

static void clips_finish_recording(void)
{
	int slot;
	int indexed = 0;
	long bytes;
	unsigned long seen[CLIPS_PLAYERS];
	int seen_count = 0;
	int i;

	if (!clips.file)
		return;
	clips.header.player_count = clips.slot_count;
	clips.header.ticks = clips.header.ticks < 0 ? 0 : clips.header.ticks;
	fseek(clips.file, 0, SEEK_SET);
	clips_write(&clips.header, sizeof(clips.header));
	fflush(clips.file);
	bytes = ftell(clips.file);
	clips_close_file();
	clips.recording = 0;
	if (clips.header.ticks < 1 || bytes < (long)sizeof(clips.header))
	{
		char path[260];

		snprintf(path, sizeof(path), "z:\\saved\\clips\\files\\%s", clips.file_name);
		remove(path);
		return;
	}
	for (slot = 0; slot < clips.slot_count && slot < CLIPS_PLAYERS; slot++)
	{
		unsigned long profile = (unsigned long)clips.header.players[slot].profile;
		int already = 0;

		if (clips.header.players[slot].local_seat < 0)
			continue;
		if (!clips_profile_ok(clips.header.players[slot].profile))
			continue;
		for (i = 0; i < seen_count; i++)
			if (seen[i] == profile)
				already = 1;
		if (already)
			continue;
		if (seen_count < CLIPS_PLAYERS)
			seen[seen_count++] = profile;
		clips_index_add(profile, clips.file_name);
		indexed = 1;
	}
	if (!indexed && clips_profile_ok(clips.profile))
		clips_index_add((unsigned long)clips.profile, clips.file_name);
	clips_catalog_add(bytes);
	clips_enforce_cap();
}

static void clips_reset_slots(void)
{
	int i;

	for (i = 0; i < CLIPS_PLAYERS; i++)
		clips.slot_datum[i] = NONE;
	clips.slot_count = 0;
	clips.spawned = 0;
	clips.stopped = 0;
	clips.ended = 0;
	clips.play_tick = 0;
	clips.hold_clock = 0;
	clips.last_count = 0;
	csmemset(clips.trail_x, 0, sizeof(clips.trail_x));
	csmemset(clips.trail_y, 0, sizeof(clips.trail_y));
	csmemset(clips.trail_i, 0, sizeof(clips.trail_i));
	csmemset(clips.trail_n, 0, sizeof(clips.trail_n));
	csmemset(clips.input_seen, 0, sizeof(clips.input_seen));
	clips.message[0] = 0;
}

static void clips_begin_recording(void)
{
	struct scenario *scenario;
	char map[128];
	char safe[32];
	char path[260];
	int multiplayer;
	struct game_variant *variant;
	struct game_variant_options const *options;

	scenario = global_scenario_get();
	if (!scenario || scenario->type == _scenario_type_main_menu)
		return;
	if (network_game_distributed_client())
		return;
	{
		short difficulty = 0;
		unsigned long seed = 0;

		map[0] = 0;
		game_loaded_setup(map, sizeof(map), &difficulty, &seed);
		if (!map[0])
			return;
		/* filed under whoever sits down for this run */
		clips.profile = NONE;
		clips_dirs();
		csmemset(&clips.header, 0, sizeof(clips.header));
		clips_copy(clips.header.map, sizeof(clips.header.map), map);
		clips.header.difficulty = difficulty;
		clips.header.seed = seed;
	}
	clips.header.magic[0] = 'C';
	clips.header.magic[1] = 'L';
	clips.header.magic[2] = 'I';
	clips.header.magic[3] = 'P';
	clips.header.version = CLIPS_VERSION;
	clips.header.build = HALO_BUILD_NUMBER;
	multiplayer = scenario->type == _scenario_type_multiplayer;
	clips.header.kind = multiplayer ? 1 : 0;
	clips_title(clips.header.title, sizeof(clips.header.title), multiplayer);
	clips.header.recorded_at = (long)time(NULL);
	if (multiplayer)
	{
		variant = game_engine_get_variant();
		options = game_variant_options_get();
		if (variant)
			csmemcpy(clips.header.variant, variant, sizeof(clips.header.variant));
		if (options)
			csmemcpy(clips.header.options, options, sizeof(clips.header.options));
	}
	clips_safe_name(safe, sizeof(safe), clips.header.map);
	snprintf(clips.file_name, sizeof(clips.file_name), "%08lx_%s.clip",
		(unsigned long)clips.header.recorded_at, safe);
	snprintf(path, sizeof(path), "z:\\saved\\clips\\files\\%s", clips.file_name);
	clips.file = fopen(path, "wb+");
	if (!clips.file)
		return;
	if (!clips_write(&clips.header, sizeof(clips.header)))
	{
		clips_close_file();
		remove(path);
		return;
	}
	clips_copy(clips.path, sizeof(clips.path), path);
	clips_reset_slots();
	clips.recording = 1;
	clips.replaying = 0;
}

static int clips_watched_slot(void)
{
	int slot;
	int fallback = 0;

	for (slot = 0; slot < clips.header.player_count && slot < CLIPS_PLAYERS; slot++)
	{
		if (clips.header.players[slot].local_seat >= 0 && fallback == 0)
			fallback = slot;
		if (clips.header.players[slot].profile == clips.profile &&
			clips.header.players[slot].local_seat >= 0)
			return slot;
	}
	return fallback;
}

static long clips_place_datum(unsigned short absolute)
{
	unsigned short salt;

	if (!player_data)
		return NONE;
	salt = (unsigned short)player_data->next_identifier;
	if (!salt)
		salt = 0x8000;
	return ((long)salt << 16) | (long)absolute;
}

static void clips_spawn_roster(void)
{
	long local;
	int watched;
	int slot;
	long kept;

	if (clips.spawned || !clips.replaying || !clips.header_ok)
		return;
	local = local_player_get_player_index(0);
	if (local == NONE)
		return;
	if (clips.header.player_count < 1)
	{
		clips.spawned = 1;
		return;
	}
	watched = clips_watched_slot();
	/* the game updates players in datum order. each seat comes back at the
	index it had, or the positions diverge and the checkpoint stops the clip */
	kept = NONE;
	if ((unsigned short)(local & 0xFFFF) == clips.header.players[watched].absolute)
		kept = local;
	else
	{
		local_player_set_player_index(0, NONE);
		machine_remove_player(local);
		datum_delete(player_data, local);
	}
	for (slot = 0; slot < clips.header.player_count && slot < CLIPS_PLAYERS; slot++)
	{
		struct network_player network;
		struct player_datum *player;
		long datum;
		long placed;
		int i;

		if (slot == watched && kept != NONE)
		{
			clips.slot_datum[slot] = kept;
			continue;
		}
		csmemset(&network, 0, sizeof(network));
		for (i = 0; i < 12; i++)
			network.name[i] = (wchar_t)clips.header.players[slot].name[i];
		network.primary_color_index = clips.header.players[slot].color;
		network.team_index = (char)clips.header.players[slot].team;
		/* one machine holds four players. a kept seat is already on
		machine 0, so each other seat takes a machine of its own */
		placed = clips_place_datum(clips.header.players[slot].absolute);
		datum = player_new(slot + 1, placed, (short)(slot == watched ? 0 : NONE), &network);
		clips.slot_datum[slot] = datum;
		if (datum == NONE)
			continue;
		if (slot == watched)
			local_player_set_player_index(0, datum);
		player = player_get(datum);
		if (player)
			player->team_index = clips.header.players[slot].team;
		game_engine_player_added(datum);
	}
	/* the engine assigns teams as players are added. the clip's teams are
	the ones the match was played with, including the player already here */
	for (slot = 0; slot < clips.header.player_count && slot < CLIPS_PLAYERS; slot++)
	{
		long datum = clips.slot_datum[slot];
		struct player_datum *recorded;
		int i;

		if (datum == NONE)
			continue;
		recorded = player_get(datum);
		if (!recorded)
			continue;
		for (i = 0; i < 12; i++)
		{
			recorded->name[i] = (wchar_t)clips.header.players[slot].name[i];
			recorded->network_player_data.name[i] =
				(wchar_t)clips.header.players[slot].name[i];
		}
		recorded->team_index = clips.header.players[slot].team;
		recorded->network_player_data.team_index = (char)clips.header.players[slot].team;
		recorded->network_player_data.primary_color_index = clips.header.players[slot].color;
		/* the queues were built for whoever was here before these seats */
		update_server_add_player(datum);
		update_client_add_player(datum);
	}
	clips.spawned = 1;
}

#define CLIPS_FEED_LINES 12
#define CLIPS_FEED_CHARS 96

static wchar_t clips_feed_text[CLIPS_FEED_LINES][CLIPS_FEED_CHARS];
static long clips_feed_when[CLIPS_FEED_LINES];
static int clips_feed_count;
static int clips_feed_next;

static void clips_feed_clear(void)
{
	clips_feed_count = 0;
	clips_feed_next = 0;
}

void clips_feed_note(wchar_t const *text)
{
	int last;
	long now;

	if (!clips.replaying || !text || !text[0])
		return;
	now = game_time_get();
	if (clips_feed_count > 0)
	{
		last = clips_feed_next - 1;
		if (last < 0)
			last = CLIPS_FEED_LINES - 1;
		if (clips_feed_when[last] == now && ustrcmp(clips_feed_text[last], text) == 0)
			return;
	}
	ustrncpy_terminated(clips_feed_text[clips_feed_next], text, CLIPS_FEED_CHARS);
	clips_feed_when[clips_feed_next] = now;
	clips_feed_next++;
	if (clips_feed_next >= CLIPS_FEED_LINES)
		clips_feed_next = 0;
	if (clips_feed_count < CLIPS_FEED_LINES)
		clips_feed_count++;
}

void clips_feed_item(long player_index, long definition_index, short quantity, int message_offset)
{
	struct player_datum *player;
	struct item_definition *item;
	wchar_t const *item_text;
	wchar_t body[CLIPS_FEED_CHARS];
	wchar_t line[CLIPS_FEED_CHARS];
	short string_index;
	short value_scale;
	char used;

	if (!clips.replaying || player_index == NONE || definition_index == NONE)
		return;
	player = player_try_and_get(player_index);
	if (!player)
		return;
	item = item_definition_get(definition_index);
	if (!item || item->item.hud_message_index == NONE)
		return;
	if (message_offset < 0)
		used = quantity > 1;
	else
		used = (char)message_offset;
	string_index = (short)(item->item.hud_message_index + used);
	item_text = hud_get_item_string(string_index);
	if (!item_text || !item_text[0])
		return;
	if ((message_offset < 0 && used) || quantity)
	{
		value_scale = item->item.hud_message_value_scale;
		if (value_scale < 1)
			value_scale = 1;
		if (ustring_format_takes(item_text, "d"))
			usnprintf(body, CLIPS_FEED_CHARS, item_text, quantity / value_scale);
		else
			ustrncpy_terminated(body, item_text, CLIPS_FEED_CHARS);
	}
	else
		ustrncpy_terminated(body, item_text, CLIPS_FEED_CHARS);
	body[CLIPS_FEED_CHARS - 1] = 0;
	usnprintf(line, CLIPS_FEED_CHARS, L"%s  %s", player->name, body);
	line[CLIPS_FEED_CHARS - 1] = 0;
	clips_feed_note(line);
}

static void clips_begin_playback(void)
{
	if (!clips.header_ok && !clips_read_header(clips.pending, &clips.header))
	{
		clips.pending[0] = 0;
		clips.replaying = 0;
		return;
	}
	clips.header_ok = 1;
	clips_copy(clips.path, sizeof(clips.path), clips.pending);
	clips.file = fopen(clips.path, "rb");
	clips.pending[0] = 0;
	if (!clips.file)
	{
		clips.replaying = 0;
		return;
	}
	fseek(clips.file, (long)sizeof(clips.header), SEEK_SET);
	clips_reset_slots();
	if (clips.header.player_count > 0 && clips.header.player_count <= CLIPS_PLAYERS)
		clips.slot_count = clips.header.player_count;
	clips.replaying = 1;
	clips.recording = 0;
	clips_feed_clear();
	if (!clips.rewinding)
	{
		clips.flying = 0;
		clips.show_feed = 0;
	}
	clips.camera_mode = 0;
	clips.watch = clips_watched_slot();
	clips.seeking = 0;
	clips.resume_speed = 1.0f;
	clips.scrub_limit = 0;
	clips.scrub_exact = 0;
	if (clips.rewinding)
	{
		clips.seek_tick = clips.rewind_to;
		clips.resume_speed = 0.0f;
		clips.scrubbing = 1;
		if (clips.rewind_to > 0)
		{
			clips.seeking = 1;
			clips.scrub_allow = clips.rewind_to > 64 ? 64 : clips.rewind_to;
			clips.scrub_limit = 1;
			clips.scrub_exact = 1;
			game_time_set_speed(16.0f);
		}
		else
			game_time_set_speed(0.0f);
	}
	else
	{
		clips.show_inputs = 1;
		clips.show_feed = 0;
		clips.speed_index = 2;
		clips.arm = 30;
		clips.scrubbing = 0;
		clips.scrub_dir = 0;
		clips.scrub_from_play = 0;
		clips.scrub_aimed = 0;
		clips.scrub_wait = 0;
		clips.scrub_debt = 0.0f;
		clips.scrub_clock = 0;
		game_time_set_speed(1.0f);
	}
	clips.rewinding = 0;
	clips.rewind_to = 0;
	clips_spawn_roster();
}

static int clips_slot_for(long datum, struct player_datum *player)
{
	int slot;
	int i;

	for (slot = 0; slot < clips.slot_count; slot++)
		if (clips.slot_datum[slot] == datum)
			return slot;
	if (clips.slot_count >= CLIPS_PLAYERS)
		return -1;
	slot = clips.slot_count++;
	clips.slot_datum[slot] = datum;
	csmemset(&clips.header.players[slot], 0, sizeof(clips.header.players[slot]));
	clips.header.players[slot].local_seat = -1;
	clips.header.players[slot].profile = NONE;
	clips.header.players[slot].team = (short)player->team_index;
	clips.header.players[slot].color = player->network_player_data.primary_color_index;
	clips.header.players[slot].absolute = (unsigned short)(datum & 0xFFFF);
	for (i = 0; i < 12; i++)
		clips.header.players[slot].name[i] = (unsigned short)player->name[i];
	if (player->local_player_index >= 0 && player->local_player_index < 4)
	{
		clips.header.players[slot].local_seat = player->local_player_index;
		clips.header.players[slot].profile =
			player_ui_get_active_player_profile_index(player->local_player_index);
		if (clips.profile == NONE)
			clips.profile = clips.header.players[slot].profile;
	}
	return slot;
}

static void clips_sample_local(short seat, struct clip_sample_disk *sample)
{
	short lx = 0, ly = 0, rx = 0, ry = 0;
	unsigned char lt = 0, rt = 0, face = 0;
	unsigned short buttons = 0;

	sample->keys = 0;
	sample->mouse_x = 0;
	sample->mouse_y = 0;
	sample->lx = sample->ly = sample->rx = sample->ry = 0;
	sample->lt = sample->rt = 0;
	sample->buttons = 0;
	sample->face = 0;
	sample->kind = 3;
	if (seat < 0)
		return;
	halo_linux_pad_sample(seat, &lx, &ly, &rx, &ry, &lt, &rt, &buttons, &face);
	sample->lx = lx;
	sample->ly = ly;
	sample->rx = rx;
	sample->ry = ry;
	sample->lt = lt;
	sample->rt = rt;
	sample->buttons = buttons;
	sample->face = face;
	if (seat == 0)
	{
		sample->keys = halo_keyboard_actions(0);
		sample->mouse_x = clips.live_mouse_x;
		sample->mouse_y = clips.live_mouse_y;
		sample->kind = halo_linux_mouse_is_aim_device(0) ? 1 : 0;
	}
	else
		sample->kind = 0;
}

static void clips_sample_remote(struct player_action const *action, struct clip_sample_disk *sample)
{
	sample->kind = 3;
	sample->keys = 0;
	sample->mouse_x = 0;
	sample->mouse_y = 0;
	sample->buttons = 0;
	sample->face = 0;
	sample->lx = (short)(action->throttle.i * 32767.0f);
	sample->ly = (short)(action->throttle.j * 32767.0f);
	sample->rx = 0;
	sample->ry = 0;
	sample->lt = 0;
	sample->rt = (unsigned char)(action->primary_trigger * 255.0f);
}

static void clips_record_tick(struct player_action *actions, int capacity)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct clip_sample_disk samples[CLIPS_PLAYERS];
	short count = 0;
	unsigned char type;
	long tick;
	unsigned long hash;

	tick = game_time_get();
	halo_linux_mouse_sample(&clips.live_mouse_x, &clips.live_mouse_y);
	if (tick > 0 && (tick % CLIPS_CHECKPOINT) == 0)
	{
		type = 2;
		hash = clips_hash(tick);
		if (!clips_write(&type, 1) || !clips_write(&tick, sizeof(tick)) ||
			!clips_write(&hash, sizeof(hash)))
		{
			clips_close_file();
			clips.recording = 0;
			return;
		}
	}
	data_iterator_new(&iterator, player_data);
	player = data_iterator_next(&iterator);
	while (player && count < CLIPS_PLAYERS)
	{
		int absolute = (int)(iterator.datum_index & 0xFFFF);
		int slot;

		if (absolute >= 0 && absolute < capacity)
		{
			slot = clips_slot_for(iterator.datum_index, player);
			if (slot >= 0)
			{
				csmemset(&samples[count], 0, sizeof(samples[count]));
				samples[count].slot = (short)slot;
				samples[count].action = actions[absolute];
				if (player->local_player_index >= 0)
					clips_sample_local(player->local_player_index, &samples[count]);
				else
					clips_sample_remote(&samples[count].action, &samples[count]);
				clips.last[slot] = samples[count];
				count++;
			}
		}
		player = data_iterator_next(&iterator);
	}
	clips.last_count = clips.slot_count;
	type = 1;
	if (!clips_write(&type, 1) || !clips_write(&tick, sizeof(tick)) ||
		!clips_write(&count, sizeof(count)) ||
		(count && !clips_write(samples, (long)(count * sizeof(samples[0])))))
	{
		clips_close_file();
		clips.recording = 0;
		return;
	}
	clips.header.ticks = tick + 1;
}

static void clips_idle_actions(struct player_action *actions, int capacity)
{
	int i;

	for (i = 0; i < capacity; i++)
	{
		csmemset(&actions[i], 0, sizeof(actions[i]));
		actions[i].desired_weapon_index = NONE;
		actions[i].desired_grenade_index = NONE;
		actions[i].desired_zoom_level = NONE;
	}
}

static void clips_stop_mismatch(struct player_action *actions, int capacity)
{
	clips.stopped = 1;
	clips.hold_clock = 0;
	clips_copy(clips.message, sizeof(clips.message),
		"This build no longer plays this clip through.");
	game_time_set_speed(0.0f);
	clips.seeking = 0;
	clips_idle_actions(actions, capacity);
}

static short clips_stick_from_turn(float turn)
{
	float scaled;

	/* a respawn snaps the facing. a stick cannot turn that far in one tick.
	a still view jitters by less than a pixel, and that stays centered */
	if (turn > 0.45f || turn < -0.45f)
		return 0;
	if (turn > -0.004f && turn < 0.004f)
		return 0;
	scaled = turn / 0.06f * 32767.0f;
	if (scaled > 32767.0f)
		scaled = 32767.0f;
	if (scaled < -32767.0f)
		scaled = -32767.0f;
	return (short)scaled;
}

static short clips_axis(float unit)
{
	float scaled;

	scaled = unit * 32767.0f;
	if (scaled > 32767.0f)
		scaled = 32767.0f;
	if (scaled < -32767.0f)
		scaled = -32767.0f;
	return (short)scaled;
}

static void clips_remember_sample(struct clip_sample_disk const *sample)
{
	struct clip_sample_disk kept;
	int slot;

	if (sample->slot < 0 || sample->slot >= CLIPS_PLAYERS)
		return;
	slot = sample->slot;
	kept = *sample;
	/* the look stick is how far the facing moved. the saved right stick is
	often resting while that facing is turning */
	if (clips.input_seen[slot])
	{
		float dyaw;
		float dpitch;
		short look_x;
		short look_y;

		dyaw = kept.action.desired_facing.yaw - clips.last[slot].action.desired_facing.yaw;
		dpitch = kept.action.desired_facing.pitch - clips.last[slot].action.desired_facing.pitch;
		while (dyaw > 3.14159265f)
			dyaw -= 6.2831853f;
		while (dyaw < -3.14159265f)
			dyaw += 6.2831853f;
		/* stick right turns the view left, so the dot goes the other way */
		look_x = clips_stick_from_turn(-dyaw);
		look_y = clips_stick_from_turn(dpitch);
		if (kept.rx > -2500 && kept.rx < 2500 && kept.ry > -2500 && kept.ry < 2500)
		{
			kept.rx = look_x;
			kept.ry = look_y;
		}
	}
	clips.last[slot] = kept;
	clips.input_seen[slot] = 1;
	clips.trail_x[slot][clips.trail_i[slot]] = kept.mouse_x;
	clips.trail_y[slot][clips.trail_i[slot]] = kept.mouse_y;
	clips.trail_i[slot] = (clips.trail_i[slot] + 1) % 8;
	if (clips.trail_n[slot] < 8)
		clips.trail_n[slot]++;
}

/* several samples share a tick while the match clock is waiting. the last
one of that tick is the sample the clock moves on from. */
static int clips_same_tick_follows(long tick)
{
	long mark;
	unsigned char type;
	long next;
	short count;
	unsigned long hash;
	int same;

	same = 0;
	if (!clips.file)
		return 0;
	mark = ftell(clips.file);
	if (mark < 0)
		return 0;
	for (;;)
	{
		if (!clips_read(&type, 1))
			break;
		if (type == 2)
		{
			if (!clips_read(&next, sizeof(next)) || !clips_read(&hash, sizeof(hash)))
				break;
			continue;
		}
		if (type != 1 || !clips_read(&next, sizeof(next)) || !clips_read(&count, sizeof(count)))
			break;
		same = next == tick;
		break;
	}
	fseek(clips.file, mark, SEEK_SET);
	return same;
}

static void clips_playback_tick(struct player_action *actions, int capacity)
{
	unsigned char type;
	long tick;
	unsigned long hash;
	unsigned long live;
	short count;
	int n;

	if (clips.stopped || clips.ended)
	{
		clips.hold_clock = 0;
		clips_idle_actions(actions, capacity);
		game_time_set_speed(0.0f);
		return;
	}
	clips_spawn_roster();
	for (;;)
	{
		if (!clips_read(&type, 1))
		{
			clips.ended = 1;
			clips.hold_clock = 0;
			game_time_set_speed(0.0f);
			clips.seeking = 0;
			clips_idle_actions(actions, capacity);
			return;
		}
		if (type == 2)
		{
			if (!clips_read(&tick, sizeof(tick)) || !clips_read(&hash, sizeof(hash)))
			{
				clips.ended = 1;
				clips.hold_clock = 0;
				game_time_set_speed(0.0f);
				clips_idle_actions(actions, capacity);
				return;
			}
			live = clips_hash(tick);
			/* a newer build can stand a player a step away from where an
			older one did. the recorded actions still run to the end.
			a clip saved before the build number was written has build 0 */
			if (live != hash && clips.header.build != 0 &&
				clips.header.build == (long)HALO_BUILD_NUMBER)
			{
				clips_stop_mismatch(actions, capacity);
				return;
			}
			continue;
		}
		break;
	}
	if (type != 1 || !clips_read(&tick, sizeof(tick)) || !clips_read(&count, sizeof(count)))
	{
		clips.ended = 1;
		clips.hold_clock = 0;
		game_time_set_speed(0.0f);
		clips_idle_actions(actions, capacity);
		return;
	}
	clips.play_tick = tick;
	clips.hold_clock = 0;
	if (count < 0 || count > CLIPS_PLAYERS)
	{
		clips_stop_mismatch(actions, capacity);
		return;
	}
	clips_idle_actions(actions, capacity);
	for (n = 0; n < count; n++)
	{
		struct clip_sample_disk sample;
		int absolute;
		long datum;

		if (!clips_read(&sample, sizeof(sample)))
		{
			clips.ended = 1;
			clips.hold_clock = 0;
			game_time_set_speed(0.0f);
			return;
		}
		clips_remember_sample(&sample);
		if (sample.slot < 0 || sample.slot >= CLIPS_PLAYERS)
			continue;
		datum = clips.slot_datum[sample.slot];
		if (datum == NONE)
			continue;
		absolute = (int)(datum & 0xFFFF);
		if (absolute < 0 || absolute >= capacity)
			continue;
		actions[absolute] = sample.action;
	}
	clips.hold_clock = clips_same_tick_follows(tick);
}

void clips_on_actions(struct player_action *actions, int capacity)
{
	if (!actions || capacity < 1)
		return;
	if (clips.replaying)
		clips_playback_tick(actions, capacity);
	else if (clips.recording)
		clips_record_tick(actions, capacity);
}

int clips_fill_options(char *map_name, long map_name_size, short *difficulty, unsigned long *random_seed)
{
	if (!clips.pending[0])
		return 0;
	if (!clips_read_header(clips.pending, &clips.header))
	{
		clips.pending[0] = 0;
		clips.replaying = 0;
		clips.header_ok = 0;
		return 0;
	}
	clips.header_ok = 1;
	clips.replaying = 1;
	if (map_name && map_name_size > 0)
		clips_copy(map_name, map_name_size, clips.header.map);
	if (difficulty)
		*difficulty = (short)clips.header.difficulty;
	if (random_seed)
		*random_seed = clips.header.seed;
	if (clips.header.kind == 1)
	{
		struct game_variant variant;
		struct game_variant_options options;

		csmemcpy(&variant, clips.header.variant, sizeof(variant));
		game_set_game_variant(&variant);
		csmemset(&options, 0, sizeof(options));
		csmemcpy(&options, clips.header.options, sizeof(clips.header.options));
		game_set_game_variant_options(&options);
	}
	return 1;
}

void clips_map_started(void)
{
	if (clips.pending[0])
	{
		clips_begin_playback();
		return;
	}
	if (clips.replaying || clips.recording)
		return;
	clips_begin_recording();
}

void clips_map_ended(void)
{
	int playing = clips.file && clips.replaying;

	if (clips.recording)
		clips_finish_recording();
	else
		clips_close_file();
	if (playing && !clips.rewinding)
	{
		clips.return_menu = 1;
		clips.restore = 1;
	}
	if (playing && !clips.rewinding)
		clips.pending[0] = 0;
	clips.recording = 0;
	clips.scrub_limit = 0;
	clips.scrub_exact = 0;
	clips.hold_clock = 0;
	clips.replaying = clips.pending[0] ? 1 : 0;
	clips.spawned = 0;
	if (!clips.replaying)
		clips.header_ok = clips.pending[0] ? clips.header_ok : 0;
	if (clips.hold_camera)
	{
		director_clips_hold(FALSE);
		clips.hold_camera = 0;
	}
}

int clips_replaying(void)
{
	return clips.replaying;
}

int clips_flying(void)
{
	return clips.replaying && clips.flying;
}

int clips_catching_up(void)
{
	return clips.replaying && clips.seeking;
}

int clips_clock_held(void)
{
	return clips.replaying && clips.hold_clock;
}

long clips_tick_allowance(int *exact)
{
	if (exact)
		*exact = 0;
	if (!clips.replaying || !game_in_progress() || !clips.scrub_limit)
		return -1;
	if (exact)
		*exact = clips.scrub_exact ? 1 : 0;
	if (clips.scrub_allow < 0)
		return 0;
	return clips.scrub_allow;
}

int clips_return_pending(void)
{
	return clips.return_menu;
}

int clips_take_return(void)
{
	int pending = clips.return_menu;

	clips.return_menu = 0;
	return pending;
}

static void clips_launch_again(void)
{
	if (!clips.path[0] || !clips.header_ok)
		return;
	clips_copy(clips.pending, sizeof(clips.pending), clips.path);
	clips.header_ok = 1;
	clips.rewinding = 1;
	main_set_map_name(clips.header.map);
	main_set_difficulty((short)clips.header.difficulty);
	game_connection_set(0);
	main_menu_switch_to_single_player();
}

static int clips_edge(int down, int *was)
{
	int edge = down && !*was;

	*was = down ? 1 : 0;
	return edge;
}

/* one tick on the press. nothing more until the hold is past a tap,
then twice as fast every 400ms. forward tops out at 240 ticks a second.
backward tops out at the pace of the match, so a hold stays on the tick
it has reached. */
static long clips_scrub_step(unsigned long held_ms, unsigned long dt_ms, float *debt, int first, int back)
{
	long step;
	int rate;
	int cap;
	unsigned long span;

	if (first)
	{
		*debt = 0.0f;
		return 0;
	}
	if (held_ms < 600)
		return 0;
	span = held_ms - 600;
	rate = back ? 8 : 4;
	cap = back ? 30 : 240;
	while (span >= 400 && rate < cap)
	{
		span -= 400;
		rate *= 2;
	}
	if (rate > cap)
		rate = cap;
	*debt += (float)rate * (float)dt_ms / 1000.0f;
	step = (long)*debt;
	if (step > 7)
		step = 7;
	*debt -= (float)step;
	if (*debt < 0.0f)
		*debt = 0.0f;
	return step;
}

static void clips_scrub_forward(long step)
{
	clips.seeking = 0;
	if (clips.stopped || clips.ended || step <= 0)
	{
		clips.scrub_limit = 0;
		clips.scrub_exact = 0;
		game_time_set_speed(0.0f);
		return;
	}
	clips.scrub_allow = step;
	clips.scrub_limit = 1;
	clips.scrub_exact = 1;
	game_time_set_speed(16.0f);
}

static long clips_scrub_shown(long tick)
{
	if (clips.seeking)
		return clips.seek_tick;
	if (clips.rewinding)
		return clips.scrub_target;
	/* a backward hold keeps the tick it has walked to */
	if (clips.scrub_aimed && clips.scrub_dir < 0)
		return clips.scrub_target;
	return tick;
}

static void clips_scrub_move(long tick, int dir, long count)
{
	long shown;
	long next;

	if (dir == 0 || count <= 0)
		return;
	shown = clips_scrub_shown(tick);
	clips.resume_speed = 0.0f;
	if (dir < 0)
	{
		next = shown - count;
		if (next < 0)
			next = 0;
		clips.scrub_target = next;
		if (clips.seeking && tick < next)
		{
			clips.seek_tick = next;
			return;
		}
		if (clips.rewinding)
		{
			clips.rewind_to = next;
			return;
		}
		/* the picture is already here. letting go reloads to the tick */
		clips.seeking = 0;
		clips.scrub_limit = 0;
		clips.scrub_exact = 0;
		clips.scrub_target = next;
		clips.scrub_aimed = 1;
		game_time_set_speed(0.0f);
		return;
	}
	next = shown + count;
	if (clips.seeking && tick < next)
	{
		clips.seek_tick = next;
		clips.scrub_target = next;
		return;
	}
	if (clips.rewinding && tick < next)
	{
		clips.rewind_to = next;
		clips.scrub_target = next;
		return;
	}
	clips.seeking = 0;
	clips_scrub_forward(count);
}

void clips_frame(void)
{
	static int was_pause, was_slow, was_fast;
	static int was_hide, was_back, was_camera;
	short lx, ly, rx, ry;
	unsigned char lt, rt, face;
	unsigned short buttons;
	int back_down;
	int speed_down;
	int pause_edge;
	int dir;
	int first;
	long tick;
	long step;
	long room;
	unsigned long keys;
	unsigned long now;
	unsigned long dt;

	if (!clips.replaying || !game_in_progress())
		return;
	if (clips.arm > 0)
		clips.arm--;
	tick = game_time_get();
	clips.scrub_limit = 0;
	clips.scrub_exact = 0;
	/* the file tick, not the clock: the landing is the sample on screen */
	if (clips.seeking && clips.play_tick >= clips.seek_tick)
	{
		clips.seeking = 0;
		game_time_set_speed(clips.resume_speed);
		if (clips.scrub_dir == 0)
			clips.scrubbing = 0;
	}
	if (clips.seeking)
	{
		room = clips.seek_tick - clips.play_tick;
		if (room < 0)
			room = 0;
		if (room > 64)
			room = 64;
		clips.scrub_allow = room;
		clips.scrub_limit = 1;
		clips.scrub_exact = 1;
	}
	lx = ly = rx = ry = 0;
	lt = rt = face = 0;
	buttons = 0;
	halo_linux_pad_sample(0, &lx, &ly, &rx, &ry, &lt, &rt, &buttons, &face);
	keys = halo_linux_theatre_keys();
	dir = 0;
	if ((keys & 2UL) != 0 || (buttons & 0x0004) != 0)
		dir = -1;
	if ((keys & 4UL) != 0 || (buttons & 0x0008) != 0)
		dir = dir < 0 ? 0 : 1;
	if (dir == 0)
	{
		clips.scrub_wait = 0;
		clips.scrub_from_play = 0;
		clips.scrub_aimed = 0;
	}
	pause_edge = clips_edge((keys & 1UL) != 0 || (buttons & 0x0010) != 0, &was_pause);
	if (pause_edge)
	{
		if (clips.scrub_dir != 0 || clips.scrubbing || clips.seeking)
		{
			clips.scrub_dir = 0;
			clips.scrub_from_play = 0;
			clips.scrub_aimed = 0;
			clips.scrub_debt = 0.0f;
			clips.scrubbing = 0;
			clips.seeking = 0;
			clips.scrub_limit = 0;
			clips.scrub_exact = 0;
			clips.scrub_wait = 1;
			game_time_set_speed(0.0f);
		}
		else if (game_time_get_speed() == 0.0f && !clips.stopped && !clips.ended)
			game_time_set_speed(clips_speeds[clips.speed_index]);
		else
			game_time_set_speed(0.0f);
		input_suppress();
	}
	else if (!clips.scrub_wait && dir != 0 && !clips.seeking && !clips.rewinding)
	{
		long base;

		now = system_milliseconds();
		if (clips.scrub_clock == 0 || now < clips.scrub_clock)
			dt = 16;
		else
			dt = now - clips.scrub_clock;
		if (clips.scrub_dir != 0 && dt > 250)
			clips.scrub_mark += dt;
		if (dt > 100)
			dt = 100;
		clips.scrub_clock = now;
		first = dir != clips.scrub_dir;
		if (first)
		{
			clips.scrub_dir = dir;
			clips.scrub_mark = now;
			clips.scrub_debt = 0.0f;
			clips.scrub_aimed = 0;
			/* still running: the hold moves at once. a paused tap stays one tick */
			clips.scrub_from_play = game_time_get_speed() > 0.0f ? 1 : 0;
		}
		step = clips_scrub_step(now - clips.scrub_mark, dt, &clips.scrub_debt, first, dir < 0);
		clips.scrubbing = 1;
		/* backward steps from the tick on the bar. forward follows the clock */
		base = dir < 0 ? clips.play_tick : tick;
		if (first)
			clips_scrub_move(base, dir, 1);
		else if (step > 0)
			clips_scrub_move(base, dir, step);
		else if (dir > 0 && clips.scrub_from_play && !clips.seeking && !clips.rewinding)
			clips_scrub_move(base, dir, 1);
		else if (dir > 0 && !clips.seeking && !clips.rewinding)
			clips_scrub_forward(0);
		input_suppress();
	}
	else if (dir == 0 && clips.scrub_dir != 0)
	{
		int back;
		long target;

		back = clips.scrub_dir < 0;
		target = clips.scrub_target;
		clips.scrub_dir = 0;
		clips.scrub_from_play = 0;
		clips.scrub_aimed = 0;
		clips.scrub_debt = 0.0f;
		clips.scrub_exact = 0;
		if (back && target < clips.play_tick)
		{
			clips.scrubbing = 1;
			clips.resume_speed = 0.0f;
			clips.rewind_to = target;
			if (!clips.rewinding)
			{
				clips.scrub_allow = 0;
				clips.scrub_limit = 1;
				game_time_set_speed(0.0f);
				clips_launch_again();
			}
		}
		else
		{
			clips.scrubbing = 0;
			clips.scrub_limit = 0;
			game_time_set_speed(0.0f);
		}
	}
	speed_down = 0;
	/* free camera: the old speed controls rise and drop. up and down set the speed */
	if (clips.flying)
	{
		if (clips_edge((keys & 512UL) != 0 || (buttons & 0x0002) != 0, &was_slow))
			speed_down = -1;
		if (clips_edge((keys & 256UL) != 0 || (buttons & 0x0001) != 0, &was_fast))
			speed_down = 1;
	}
	else
	{
		if (clips_edge((keys & 8UL) != 0 || lt >= 128, &was_slow))
			speed_down = -1;
		if (clips_edge((keys & 16UL) != 0 || rt >= 128, &was_fast))
			speed_down = 1;
	}
	if (speed_down && !clips.stopped && !clips.ended)
	{
		clips.speed_index += speed_down;
		if (clips.speed_index < 0)
			clips.speed_index = 0;
		if (clips.speed_index > 4)
			clips.speed_index = 4;
		if (!clips.seeking && !clips.scrubbing && game_time_get_speed() != 0.0f)
			game_time_set_speed(clips_speeds[clips.speed_index]);
		input_suppress();
	}
	if (clips_edge((keys & 32UL) != 0 || (face & 0x10) != 0, &was_hide))
	{
		/* in the free camera this is the text stream. the input pictures stay as they were */
		if (clips.flying)
			clips.show_feed = !clips.show_feed;
		else
			clips.show_inputs = !clips.show_inputs;
		input_suppress();
	}
	/* C and Y swap the player panes for one free camera, or back */
	if (clips_edge((keys & 128UL) != 0 || (face & 0x08) != 0, &was_camera))
	{
		clips.flying = !clips.flying;
		input_suppress();
	}
	back_down = (keys & 64UL) != 0 || (face & 0x02) != 0 || (buttons & 0x0020) != 0;
	if (clips_edge(back_down, &was_back) && clips.arm == 0)
	{
		input_suppress();
		clips.return_menu = 1;
		clips.restore = 1;
		main_goto_main_menu();
	}
	/* the recorded cameras stay. a clip does not take them over */
	if (clips.hold_camera)
	{
		director_clips_hold(FALSE);
		clips.hold_camera = 0;
	}
}

void clips_after_director(void)
{
	/* each player keeps the camera the recording left them with */
	(void)0;
}

static void clips_quad(short x0, short y0, short x1, short y1, unsigned long color)
{
	rectangle2d rect;

	rect.x0 = x0;
	rect.y0 = y0;
	rect.x1 = x1;
	rect.y1 = y1;
	draw_quad(&rect, color);
}

static void clips_text(long font, rectangle2d *bounds, wchar_t const *text)
{
	real_argb_color color = { 1.0f, 1.0f, 1.0f, 1.0f };

	if (font == NONE || !text || !text[0])
		return;
	draw_string_set_draw_mode(font, NONE, 0, 0, &color);
	rasterizer_draw_unicode_string(bounds, NULL, NULL, 0, text);
}

/* a length from the 480-line layout, in this pane's own pixels */
static short clips_px(real scale, short design)
{
	short value = (short)(design * scale + 0.5f);

	return value < 1 ? 1 : value;
}

static int clips_pane_count(void)
{
	int window_count;

	if (game_engine_force_single_screen() || cinematic_in_progress())
		return 1;
	window_count = local_player_count();
	if (window_count < 1)
		window_count = 1;
	if (window_count > 4)
		window_count = 4;
	return window_count;
}

static long clips_pane_player(int window_index)
{
	if (window_index < 0 || window_index >= clips_pane_count())
		return NONE;
	if (game_engine_force_single_screen() || cinematic_in_progress())
		return NONE;
	return local_player_get_player_index((short)window_index);
}

static int clips_slot_of(long datum)
{
	int slot;

	if (datum == NONE)
		return -1;
	for (slot = 0; slot < CLIPS_PLAYERS; slot++)
		if (clips.slot_datum[slot] == datum)
			return slot;
	return -1;
}

static void clips_stick_box(short x, short y, real scale, short sx, short sy, int invert_y)
{
	short box = clips_px(scale, 34);
	short stick = clips_px(scale, 5);
	short reach = clips_px(scale, 13);
	short kx, ky;

	clips_quad(x, y, (short)(x + box), (short)(y + box), 0xC0101828);
	/* the dot's middle sits on the box middle when the stick is at rest */
	kx = (short)(x + (box - stick) / 2 + (sx * reach) / 32767);
	ky = (short)(y + (box - stick) / 2 + ((invert_y ? -sy : sy) * reach) / 32767);
	if (kx < x)
		kx = x;
	if (ky < y)
		ky = y;
	if (kx > x + box - stick)
		kx = (short)(x + box - stick);
	if (ky > y + box - stick)
		ky = (short)(y + box - stick);
	clips_quad(kx, ky, (short)(kx + stick), (short)(ky + stick), 0xF0FFFFFF);
}

static void clips_pip(long font, short x, short y, real scale, int lit, char const *letter)
{
	short pip_w = clips_px(scale, 22);
	short pip_h = clips_px(scale, 16);
	wchar_t wide[4];
	rectangle2d bounds;

	clips_quad(x, y, (short)(x + pip_w), (short)(y + pip_h), lit ? 0xF05682DC : 0x90283848);
	if (font == NONE || !letter || !letter[0])
		return;
	clips_widen(wide, 4, letter);
	bounds.x0 = (short)(x + clips_px(scale, 2));
	bounds.x1 = (short)(bounds.x0 + 36);
	bounds.y0 = (short)(y + clips_px(scale, 1));
	bounds.y1 = (short)(bounds.y0 + 18);
	rasterizer_text_set_scale(scale * 0.52f, bounds.x0, bounds.y0);
	clips_text(font, &bounds, wide);
}

/* one player's recorded controls. the sticks follow the move and the
look the clip stored, and the buttons use the controller's names */
static void clips_draw_inputs(short x, short y, real scale, long font,
	struct clip_sample_disk const *sample, int slot)
{
	short lx, ly, rx, ry;
	unsigned char lt, rt;
	int jump, fire, grenade, melee;
	short box;
	short gap;
	short thick;
	short pip_w;
	short pip_h;
	short row;
	unsigned long flags;
	unsigned long keys;

	if (!sample || slot < 0 || slot >= CLIPS_PLAYERS)
		return;
	/* forward is the stick's vertical, and strafe is stored with the
	opposite sign from the stick's horizontal */
	lx = clips_axis(-sample->action.throttle.j);
	ly = clips_axis(sample->action.throttle.i);
	if (lx == 0 && ly == 0)
	{
		lx = sample->lx;
		ly = sample->ly;
	}
	rx = sample->rx;
	ry = sample->ry;
	lt = sample->lt;
	rt = sample->rt;
	keys = sample->keys;
	flags = sample->action.control_flags;
	jump = (sample->face & 1) != 0 || (keys & (1UL << HALO_KEYBOARD_JUMP)) != 0 ||
		TEST_FLAG(flags, _unit_control_jump_bit);
	fire = rt >= 128 || sample->action.primary_trigger > 0.2f ||
		(keys & (1UL << HALO_KEYBOARD_FIRE)) != 0 ||
		TEST_FLAG(flags, _unit_control_weapon_primary_trigger_bit);
	grenade = lt >= 128 || (keys & (1UL << HALO_KEYBOARD_THROW_GRENADE)) != 0 ||
		TEST_FLAG(flags, _unit_control_throw_grenade_bit);
	melee = (sample->face & 2) != 0 || (keys & (1UL << HALO_KEYBOARD_MELEE)) != 0 ||
		TEST_FLAG(flags, _unit_control_use_equipment_bit);
	if (fire && rt < 128)
		rt = 255;
	if (grenade && lt < 128)
		lt = 255;
	box = clips_px(scale, 34);
	gap = clips_px(scale, 6);
	thick = clips_px(scale, 5);
	pip_w = clips_px(scale, 22);
	pip_h = clips_px(scale, 16);
	clips_stick_box(x, y, scale, lx, ly, 1);
	clips_stick_box((short)(x + box + gap), y, scale, rx, ry, 1);
	row = (short)(y + box + gap);
	clips_pip(font, x, row, scale, jump, "A");
	clips_pip(font, (short)(x + pip_w + gap), row, scale, melee, "B");
	clips_pip(font, (short)(x + (pip_w + gap) * 2), row, scale, grenade, "LT");
	clips_pip(font, (short)(x + (pip_w + gap) * 3), row, scale, fire, "RT");
	row = (short)(row + pip_h + gap);
	clips_quad(x, row, (short)(x + box), (short)(row + thick), 0x90283848);
	clips_quad((short)(x + box + gap), row, (short)(x + box + gap + box), (short)(row + thick), 0x90283848);
	if (lt)
		clips_quad(x, row, (short)(x + (lt * box) / 255), (short)(row + thick), 0xF05682DC);
	if (rt)
		clips_quad((short)(x + box + gap), row,
			(short)(x + box + gap + (rt * box) / 255), (short)(row + thick), 0xF05682DC);
	rasterizer_text_set_scale(1.0f, 0.0f, 0.0f);
}

static void clips_feed_draw(long font, real scale, short margin, short top, short pane_w)
{
	int shown;
	int index;
	int row;
	short line_h;
	short y;
	rectangle2d bounds;

	if (!clips.flying || !clips.show_feed || clips_feed_count < 1 || font == NONE)
		return;
	shown = clips_feed_count < 8 ? clips_feed_count : 8;
	line_h = clips_px(scale, 19);
	y = (short)(top - clips_px(scale, 6) - shown * line_h);
	if (y < clips_px(scale, 4))
		y = clips_px(scale, 4);
	index = clips_feed_next - shown;
	if (index < 0)
		index += CLIPS_FEED_LINES;
	for (row = 0; row < shown; row++)
	{
		bounds.x0 = margin;
		bounds.x1 = (short)(pane_w - margin);
		bounds.y0 = y;
		bounds.y1 = (short)(y + 18);
		rasterizer_text_set_scale(scale, bounds.x0, bounds.y0);
		clips_text(font, &bounds, clips_feed_text[index]);
		index++;
		if (index >= CLIPS_FEED_LINES)
			index = 0;
		y = (short)(y + line_h);
	}
	rasterizer_text_set_scale(1.0f, 0.0f, 0.0f);
}

void clips_draw(short local_player_index, union rectangle2d const *window)
{
	rectangle2d bounds;
	rectangle2d view;
	wchar_t line[160];
	wchar_t keys_help[160];
	wchar_t pad_help[160];
	long font;
	long tick;
	long shown;
	long total;
	long seconds;
	long length;
	long filled;
	char text[160];
	char const *rate;
	short pane_w;
	short pane_h;
	short bar_h;
	short top;
	short margin;
	short track_x0;
	short track_x1;
	short track_y;
	short track_h;
	short head;
	real scale;

	(void)window;
	if (!clips.replaying || local_player_index != 0)
		return;
	view = render.camera.viewport_bounds;
	pane_w = (short)(view.x1 - view.x0);
	pane_h = (short)(view.y1 - view.y0);
	if (pane_w < 8 || pane_h < 8)
		return;
	/* this draw is the full screen, after the panes. the bar uses the
	   screen's own height, and it sits on the bottom of that screen */
	scale = (real)pane_h / 480.0f;
	if (pane_w < 640)
	{
		real across = (real)pane_w / 640.0f;

		if (across < scale)
			scale = across;
	}
	/* the 480-line layout at full screen size covered the view */
	scale *= 0.55f;
	if (scale < 0.32f)
		scale = 0.32f;
	bar_h = clips_px(scale, 70);
	if (bar_h > pane_h)
		bar_h = pane_h;
	top = (short)(pane_h - bar_h);
	margin = clips_px(scale, 10);
	if (margin * 2 >= pane_w)
		margin = pane_w > 4 ? 2 : 0;
	font = tag_loaded('font', "ui\\small_ui");
	if (font == NONE)
	{
		extern long hud_get_font_index(void);

		font = hud_get_font_index();
	}
	/* the sample on screen, not the clock. the opening holds several
	samples on one tick, and a scrub used to follow the landing tick */
	tick = clips.play_tick;
	/* the backward hold, and the catch-up after it, show the tick they will land on */
	if (clips.seeking)
		tick = clips.seek_tick;
	else if ((clips.scrub_dir < 0 && clips.scrub_aimed) || clips.rewinding)
		tick = clips.scrub_target;
	if (tick < 0)
		tick = 0;
	shown = tick;
	total = clips.header.ticks;
	if (total < shown)
		total = shown;
	seconds = shown / 30;
	length = total / 30;
	rate = (clips.scrubbing || clips.seeking) ? "SCRUB" :
		game_time_get_speed() == 0.0f ? "PAUSE" :
		clips.speed_index == 0 ? "1/4" :
		clips.speed_index == 1 ? "1/2" :
		clips.speed_index == 3 ? "2" :
		clips.speed_index == 4 ? "4" : "1";
	if (clips.message[0])
		snprintf(text, sizeof(text), "%s", clips.message);
	else
		snprintf(text, sizeof(text), "%ld / %ld   %ld:%02ld / %ld:%02ld   %s%s%s",
			shown, total, seconds / 60, seconds % 60, length / 60, length % 60, rate,
			clips.flying ? "   FREE" : "",
			clips.ended ? "   END" : "");
	clips_widen(line, 160, text);
	clips_widen(keys_help, 160, clips.flying ?
		"KEYS   SPACE pause    LEFT RIGHT scrub    UP DOWN speed    [ ] height    V text    C camera    B ESC leave" :
		"KEYS   SPACE pause    ARROWS scrub    [ ] speed    V picture    C camera    B ESC leave");
	clips_widen(pad_help, 160, clips.flying ?
		"PAD    START pause    LEFT RIGHT scrub    UP DOWN speed    LT RT height    RB text    Y camera    B / BACK leave" :
		"PAD    START pause    DPAD scrub    LT RT speed    RB picture    Y camera    B / BACK leave");
	clips_feed_draw(font, scale, margin, top, pane_w);
	clips_quad(0, top, pane_w, pane_h, 0xF0101824);
	track_h = clips_px(scale, 4);
	track_y = (short)(top + clips_px(scale, 4));
	if (track_y + track_h > pane_h)
		track_y = (short)(pane_h - track_h);
	track_x0 = margin;
	track_x1 = (short)(pane_w - margin);
	if (track_x1 < track_x0 + 4)
		track_x1 = (short)(track_x0 + 4);
	clips_quad(track_x0, track_y, track_x1, (short)(track_y + track_h), 0x90283848);
	filled = 0;
	if (total > 0)
	{
		if (shown >= total)
			filled = track_x1 - track_x0;
		else
			filled = (long)((double)(track_x1 - track_x0) * (double)shown / (double)total);
	}
	head = (short)(track_x0 + filled);
	if (head < track_x0)
		head = track_x0;
	if (head > track_x1)
		head = track_x1;
	if (head > track_x0)
		clips_quad(track_x0, track_y, head, (short)(track_y + track_h), 0xF02995FF);
	{
		short mark0 = (short)(head - clips_px(scale, 1));
		short mark1 = (short)(head + clips_px(scale, 2));

		if (mark0 < track_x0)
			mark0 = track_x0;
		if (mark1 > track_x1)
			mark1 = track_x1;
		clips_quad(mark0, (short)(track_y - clips_px(scale, 2)),
			mark1, (short)(track_y + track_h + clips_px(scale, 2)), 0xF0FFFFFF);
	}
	/* the line box is the font's own height. the scale then sizes it.
	   a box already shrunk to the pane clips the letters away */
	bounds.x0 = margin;
	bounds.x1 = (short)(pane_w - margin);
	bounds.y0 = (short)(track_y + track_h + clips_px(scale, 3));
	bounds.y1 = (short)(bounds.y0 + 18);
	rasterizer_text_set_scale(scale, bounds.x0, bounds.y0);
	clips_text(font, &bounds, line);
	bounds.y0 = (short)(bounds.y0 + clips_px(scale, 19));
	bounds.y1 = (short)(bounds.y0 + 18);
	rasterizer_text_set_scale(scale, bounds.x0, bounds.y0);
	clips_text(font, &bounds, keys_help);
	bounds.y0 = (short)(bounds.y0 + clips_px(scale, 19));
	bounds.y1 = (short)(bounds.y0 + 18);
	rasterizer_text_set_scale(scale, bounds.x0, bounds.y0);
	clips_text(font, &bounds, pad_help);
	rasterizer_text_set_scale(1.0f, 0.0f, 0.0f);
	if (clips.show_inputs && !clips.flying)
	{
		int panes;
		int pane;

		panes = clips_pane_count();
		for (pane = 0; pane < panes; pane++)
		{
			rectangle2d pixel;
			rectangle2d safe;
			long player;
			int slot;
			short left, right, pane_top, pane_bot, region_bot;
			short widget_h, x, y;
			real pscale;

			player = clips_pane_player(pane);
			slot = clips_slot_of(player);
			if (slot < 0)
				continue;
			compute_window_bounds(pane, panes, &pixel, &safe);
			left = (short)(pixel.x0 - view.x0);
			right = (short)(pixel.x1 - view.x0);
			pane_top = (short)(pixel.y0 - view.y0);
			pane_bot = (short)(pixel.y1 - view.y0);
			if (right - left < 8 || pane_bot - pane_top < 8)
				continue;
			pscale = (real)(pane_bot - pane_top) / 480.0f * 0.9f;
			if (pscale > 1.4f)
				pscale = 1.4f;
			if (pscale < 0.75f)
				pscale = 0.75f;
			widget_h = clips_px(pscale, 68);
			x = (short)(left + clips_px(pscale, 8));
			if (x < left)
				x = left;
			region_bot = pane_bot;
			if (region_bot > top)
				region_bot = top;
			if (region_bot - pane_top < widget_h + clips_px(pscale, 8))
				region_bot = pane_bot;
			y = (short)(pane_top + (region_bot - pane_top - widget_h) / 2);
			if (y < pane_top)
				y = pane_top;
			clips_draw_inputs(x, y, pscale, font, &clips.last[slot], slot);
		}
	}
}

void clips_menu_set_profile(long profile_index)
{
	clips.profile = profile_index;
	clips.menu_ready = 0;
	clips.menu_offset = 0;
	clips.menu_focus = 0;
	clips.restore = 1;
}

long clips_menu_profile(void)
{
	return clips.profile;
}

int clips_menu_take_restore(long *profile_index)
{
	if (!clips.restore || !profile_index)
		return 0;
	clips.restore = 0;
	*profile_index = clips.profile;
	return 1;
}

static int clips_split(char *line, char *file, char *map, char *title, long *ticks, long *kind, long *when)
{
	char *fields[6];
	int i;
	char *cursor;

	fields[0] = line;
	cursor = line;
	for (i = 1; i < 6; i++)
	{
		cursor = strchr(cursor, '|');
		if (!cursor)
			return 0;
		*cursor++ = 0;
		fields[i] = cursor;
	}
	{
		char *end = fields[5];

		while (*end && *end != '\n' && *end != '\r')
			end++;
		*end = 0;
	}
	clips_copy(file, 64, fields[0]);
	*ticks = atol(fields[1]);
	*kind = atol(fields[2]);
	*when = atol(fields[3]);
	clips_copy(map, 64, fields[4]);
	clips_copy(title, 48, fields[5]);
	return file[0] != 0;
}

void clips_menu_load(void)
{
	char path[260];
	FILE *file;
	char line[256];

	if (clips.menu_ready)
		return;
	if (clips_profile_ok(clips.profile))
		clips_keep_enforce((unsigned long)clips.profile);
	clips.menu_count = 0;
	clips.menu_ready = 1;
	if (!clips_profile_ok(clips.profile))
		return;
	snprintf(path, sizeof(path), "z:\\saved\\clips\\profiles\\%08lx.txt", (unsigned long)clips.profile);
	file = fopen(path, "r");
	if (!file)
		return;
	while (clips.menu_count < CLIPS_LIST && fgets(line, sizeof(line), file))
	{
		struct clip_row *row = &clips.rows[clips.menu_count];

		if (clips_split(line, row->file, row->map, row->title, &row->ticks, &row->kind, &row->when))
			clips.menu_count++;
	}
	fclose(file);
	if (clips.menu_offset > clips.menu_count)
		clips.menu_offset = 0;
	if (clips.menu_offset < 0)
		clips.menu_offset = 0;
}

int clips_menu_count(void)
{
	clips_menu_load();
	return clips.menu_count;
}

void clips_menu_scroll(int delta)
{
	int max;

	clips_menu_load();
	clips.menu_offset += delta;
	if (clips.menu_offset < 0)
		clips.menu_offset = 0;
	max = clips.menu_count - CLIPS_VISIBLE;
	if (max < 0)
		max = 0;
	if (clips.menu_offset > max)
		clips.menu_offset = max;
}

void clips_menu_note_focus(int visible_row)
{
	if (visible_row >= 0)
		clips.menu_focus = visible_row;
}

int clips_menu_focused_row(void)
{
	if (clips.menu_count <= 0 || clips.menu_focus < 0)
		return -1;
	return clips.menu_focus;
}

void clips_menu_line(int visible_row, wchar_t *text, int length)
{
	int index;
	struct clip_row *row;
	wchar_t map[64];
	wchar_t title[48];
	long seconds;
	time_t when;
	struct tm *tm;
	char date[16];
	wchar_t wdate[16];

	if (!text || length < 2)
		return;
	text[0] = 0;
	clips_menu_load();
	index = clips.menu_offset + visible_row;
	if (index < 0 || index >= clips.menu_count)
		return;
	row = &clips.rows[index];
	seconds = row->ticks / 30;
	when = (time_t)row->when;
	tm = localtime(&when);
	if (tm)
		snprintf(date, sizeof(date), "%d/%d", tm->tm_mon + 1, tm->tm_mday);
	else
		date[0] = 0;
	clips_widen(map, 64, row->map);
	clips_widen(title, 48, row->title[0] ? row->title : row->map);
	clips_widen(wdate, 16, date);
	usnprintf(text, (unsigned long)length, L"%s   %s   %d:%02d   %s",
		map, title, (int)(seconds / 60), (int)(seconds % 60), wdate);
	text[length - 1] = 0;
}

int clips_menu_play(int visible_row, char *map_name, long map_name_size, short *difficulty)
{
	int index;
	char path[260];

	clips_menu_load();
	/* the VIEW button names no row. it plays the highlighted one */
	if (visible_row < 0)
		visible_row = clips.menu_focus;
	index = clips.menu_offset + visible_row;
	if (index < 0 || index >= clips.menu_count || !map_name || map_name_size < 2)
		return 0;
	snprintf(path, sizeof(path), "z:\\saved\\clips\\files\\%s", clips.rows[index].file);
	if (!clips_read_header(path, &clips.header))
		return 0;
	clips.header_ok = 1;
	clips_copy(clips.pending, sizeof(clips.pending), path);
	clips_copy(clips.file_name, sizeof(clips.file_name), clips.rows[index].file);
	clips_copy(map_name, map_name_size, clips.header.map);
	if (difficulty)
		*difficulty = (short)clips.header.difficulty;
	clips.rewinding = 0;
	clips.return_menu = 0;
	return 1;
}

static void clips_index_remove(unsigned long profile, char const *file_name)
{
	char path[260];
	FILE *file;
	char line[256];
	char *body;
	long kept;

	snprintf(path, sizeof(path), "z:\\saved\\clips\\profiles\\%08lx.txt", profile);
	file = fopen(path, "r");
	if (!file)
		return;
	body = (char *)malloc(256 * 1024);
	if (!body)
	{
		fclose(file);
		return;
	}
	body[0] = 0;
	kept = 0;
	while (fgets(line, sizeof(line), file))
	{
		char name[64];
		char *bar;
		int n;

		clips_copy(name, sizeof(name), line);
		bar = strchr(name, '|');
		if (bar)
			*bar = 0;
		n = (int)strlen(name);
		while (n > 0 && (name[n - 1] == '\n' || name[n - 1] == '\r'))
			name[--n] = 0;
		if (!strcmp(name, file_name))
			continue;
		if (kept + (long)strlen(line) < 250 * 1024)
		{
			strcat(body, line);
			kept += (long)strlen(line);
		}
	}
	fclose(file);
	file = fopen(path, "w");
	if (file)
	{
		fputs(body, file);
		fclose(file);
	}
	free(body);
}

int clips_menu_delete(int visible_row)
{
	int index;
	char file_name[64];
	int row;

	clips_menu_load();
	if (visible_row < 0)
		visible_row = clips.menu_focus;
	index = clips.menu_offset + visible_row;
	if (index < 0 || index >= clips.menu_count)
		return 0;
	clips_copy(file_name, sizeof(file_name), clips.rows[index].file);
	clips_index_remove((unsigned long)clips.profile, file_name);
	clips_delete_file(file_name);
	clips.menu_ready = 0;
	clips_menu_load();
	if (clips.menu_offset >= clips.menu_count)
		clips.menu_offset = clips.menu_count > 0 ? clips.menu_count - 1 : 0;
	row = clips.menu_offset;
	if (row < 0)
		row = 0;
	return 1;
}

static void clips_unlist_everywhere(char const *file_name)
{
	FILE *registry;
	char line[64];

	if (!file_name || !file_name[0])
		return;
	registry = fopen("z:\\saved\\clips\\profiles\\list.txt", "r");
	if (!registry)
		return;
	while (fgets(line, sizeof(line), registry))
	{
		unsigned long profile = 0;

		if (sscanf(line, "%lx", &profile) == 1)
			clips_index_remove(profile, file_name);
	}
	fclose(registry);
}
