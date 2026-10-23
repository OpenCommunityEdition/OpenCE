/*
SUBTITLES.C

Subtitles for sound tags (display.subtitles, off by default). They are lines
of plain text, one for each sound tag. The game has its own built in
(port/assets/subtitles/subtitles.txt, embedded by tools/embed_assets.py); a
subtitles.txt next to the maps (display.subtitles_file) replaces them whole.
The subtitles are in the language display.subtitles_language names: English
is subtitles.txt, and each other language has a file of its own, such as
subtitles_french.txt, built in (port/assets/subtitles) or next to the maps
(which replaces it). A line a language lacks is shown in English. The text is
UTF-8, drawn as wide characters in the HUD font, which has Latin letters
with accents and no Chinese, Japanese or Korean characters. A line:

	"sound/dialog/a10/a10_chief_get_down" - "Chief, get down!"
	sound\dialog\a10\cortana_01 = Chief, I need you on the bridge.

A line is the sound tag's path (in quotes, or without them if it holds no
spaces; the tag folder's backslashes or slashes, any case, without the
".sound"), a "-", "=" or ":", and the text, in quotes or not. "\n" in the
text starts a new line. A path ending in "/" and a star is every tag below
that folder, unless a line names the tag itself or a longer folder. Lines
that are empty or start with "#", "//" or ";" are ignored. The file is read
again when the setting changes (and at every start).

Only on single player maps (scenario type "solo": the campaign, alone or in
co-op); the main menu and multiplayer maps have none.

A sound is shown when it is started (sound_manager.c's sound_new_impulse,
which every sound that plays at a listener goes through: the AI's speech,
scripted and cutscene dialogue, effects) and is heard: its line stays for
the sound's length. Time is the game's, so a paused game keeps its lines. At
most MAXIMUM_ACTIVE_SUBTITLES lines are shown at once, the newest lowest;
the oldest goes when another comes. In a cutscene the lines stand above the
letterbox's bar.
*/

#include "cseries.h"
#include "cutscene/cinematics.h"
#include "game/game.h"
#include "interface/interface.h"
#include "interface/ui_widget.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_files.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "subtitles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

enum
{
	MAXIMUM_SUBTITLES_FILE_SIZE = 1024 * 1024,
	MAXIMUM_SUBTITLE_PATH_LENGTH = 255,
	MAXIMUM_ACTIVE_SUBTITLES = 4,
	/* (game ticks) the least a line stays, the tail after its sound, the
	most, the fade at its end, and the longest a line may wait to start
	before it is taken for a game whose clock went back */
	MINIMUM_SUBTITLE_TICKS = 45,
	SUBTITLE_TAIL_TICKS = 15,
	MAXIMUM_SUBTITLE_TICKS = 30 * 12,
	SUBTITLE_FADE_TICKS = 6,
	MAXIMUM_SUBTITLE_WAIT_TICKS = 30 * 30,
	/* the text's width as a share of the screen's, a line's padding and the
	gap between lines (the HUD's 480 lines) */
	/* the longest line drawn, in characters */
	MAXIMUM_SUBTITLE_CHARACTERS = 1023,
	SUBTITLE_PADDING = 7,
	/* (added to the height of a line, whose glyphs are taller than the font tag's) */
	SUBTITLE_LINE_SPACING = 4,
	/* (the glyphs of the HUD font rise above the font tag's line) */
	SUBTITLE_TOP_PADDING = 7,
	SUBTITLE_GAP = 4,
	/* draw_string.c's centred text and its flags (wrap in the bounds, and
	past their bottom); its enums are its own */
	_subtitle_text_center = 2,
	_subtitle_wrap_flags = 3,
};

#define SUBTITLE_WIDTH_FRACTION 0.7f
/* the text's bottom from the screen's, as a share of the screen's height,
and in a cutscene, above the letterbox */
#define SUBTITLE_BOTTOM_FRACTION 0.07f
#define SUBTITLE_CINEMATIC_BOTTOM_FRACTION 0.15f
#define SUBTITLE_BACKGROUND_ALPHA 0.55f

/* ---------- structures */

struct subtitle_entry
{
	char *path;
	char *text;
	boolean prefix;
};

struct active_subtitle
{
	char const *text;
	long start_tick;
	long end_tick;
};

/* ---------- prototypes */

/* the subtitles built into the game (tools/embed_assets.py, from
port/assets/subtitles: subtitles.txt, subtitles_french.txt and so on), by
language (subtitle_languages); size 0 where a language has none */
extern const unsigned int *const subtitles_embedded_data[];
extern const unsigned long subtitles_embedded_sizes[];
extern const unsigned long subtitles_embedded_count;

/* port/linux/src/text_hires.c: the subtitles' font tag has no font of its
own, so its text is drawn with the HUD's, which the CJK fallback also needs */
extern void text_hires_force(int on);

/* port_config.c's, cseries_windows.c's, cinematics.c's */
int config_boolean(const char *name);
const char *config_string(const char *name);
unsigned long config_changes(void);
void draw_quad(rectangle2d *rectangle, pixel32 color);

/* ---------- globals */

static struct
{
	unsigned long changes;
	boolean enabled;
	char *buffer;
	struct subtitle_entry *entries;
	long entry_count;
	long *table;
	long table_size;
	struct active_subtitle active[MAXIMUM_ACTIVE_SUBTITLES];
	long active_count;
} subtitle_globals = { (unsigned long)-1 };

/* ---------- private code */

static pixel32 subtitle_pixel(
	real red,
	real green,
	real blue,
	real alpha)
{
	return ((pixel32)(PIN(alpha, 0.0f, 1.0f) * 255.0f + 0.5f) << 24) |
		((pixel32)(PIN(red, 0.0f, 1.0f) * 255.0f + 0.5f) << 16) |
		((pixel32)(PIN(green, 0.0f, 1.0f) * 255.0f + 0.5f) << 8) |
		(pixel32)(PIN(blue, 0.0f, 1.0f) * 255.0f + 0.5f);
}

/* a path as it is looked up: lower case, forward slashes, no leading slash
or ".sound" (in place; the new length) */
static long subtitle_normalize_path(
	char *path)
{
	char *source = path;
	char *destination = path;
	long length;

	while (*source == '/' || *source == '\\')
		source++;
	for (; *source; source++)
	{
		char character = *source;

		if (character == '\\')
			character = '/';
		else if (character >= 'A' && character <= 'Z')
			character = (char)(character - 'A' + 'a');
		*destination++ = character;
	}
	*destination = 0;
	length = (long)(destination - path);
	if (length > 6 && !strcmp(path + length - 6, ".sound"))
	{
		length -= 6;
		path[length] = 0;
	}
	return length;
}

static unsigned long subtitle_hash(
	char const *path)
{
	unsigned long hash = 2166136261UL;

	for (; *path; path++)
		hash = (hash ^ (unsigned char)*path) * 16777619UL;
	return hash;
}

static void subtitle_unload(
	void)
{
	/* (the game's memory checks refuse a free of nothing, as the C library's
	does not) */
	if (subtitle_globals.buffer)
		free(subtitle_globals.buffer);
	if (subtitle_globals.entries)
		free(subtitle_globals.entries);
	if (subtitle_globals.table)
		free(subtitle_globals.table);
	subtitle_globals.buffer = NULL;
	subtitle_globals.entries = NULL;
	subtitle_globals.table = NULL;
	subtitle_globals.entry_count = 0;
	subtitle_globals.table_size = 0;
	subtitle_globals.active_count = 0;
	return;
}

/* the exact entry for a normalized path, NONE for none */
static long subtitle_find_exact(
	char const *path)
{
	long slot;

	if (!subtitle_globals.table_size)
		return NONE;
	slot = (long)(subtitle_hash(path) & (unsigned long)(subtitle_globals.table_size - 1));
	while (subtitle_globals.table[slot] != NONE)
	{
		if (!strcmp(subtitle_globals.entries[subtitle_globals.table[slot]].path, path))
			return subtitle_globals.table[slot];
		slot = (slot + 1) & (subtitle_globals.table_size - 1);
	}
	return NONE;
}

/* an entry added to the table (the table is never full) */
static void subtitle_table_add(
	long entry_index)
{
	struct subtitle_entry *entry = subtitle_globals.entries + entry_index;
	long slot = (long)(subtitle_hash(entry->path) & (unsigned long)(subtitle_globals.table_size - 1));

	while (subtitle_globals.table[slot] != NONE)
		slot = (slot + 1) & (subtitle_globals.table_size - 1);
	subtitle_globals.table[slot] = entry_index;
	return;
}

/* the text's typographic punctuation made plain, in place (the fonts have no
characters for the UTF-8 dashes, quotes and ellipsis of text written in a
word processor, which would be drawn as three characters each) */
static void subtitle_simplify_text(
	char *text)
{
	unsigned char *source = (unsigned char *)text;
	char *destination = text;

	while (*source)
	{
		if (source[0] == 0xE2 && source[1] == 0x80 && (source[2] & 0xC0) == 0x80)
		{
			unsigned char code = source[2];
			char const *replacement = NULL;

			if (code >= 0x90 && code <= 0x95)
				replacement = "-";
			else if (code >= 0x98 && code <= 0x9A)
				replacement = "'";
			else if (code >= 0x9C && code <= 0x9E)
				replacement = "\"";
			else if (code == 0xA6)
				replacement = "...";
			if (replacement)
			{
				while (*replacement)
					*destination++ = *replacement++;
				source += 3;
				continue;
			}
		}
		*destination++ = (char)*source++;
	}
	*destination = 0;
	return;
}

/* one line of the file, in place: the entry it makes, if any */
static void subtitle_parse_line(
	char *line,
	long *entry_count,
	struct subtitle_entry *entries)
{
	char *path;
	char *text;
	char *end;
	long length;
	boolean prefix = FALSE;
	boolean separated = FALSE;

	while (*line == ' ' || *line == '\t')
		line++;
	if (!*line || *line == '#' || *line == ';' || (line[0] == '/' && line[1] == '/'))
		return;
	if (*line == '"')
	{
		path = ++line;
		while (*line && *line != '"')
			line++;
		if (!*line)
			return;
		*line++ = 0;
	}
	else
	{
		/* (the path ends at a space, "=" or ":"; a "-" is a separator only
		after a space, so a path may hold one) */
		path = line;
		while (*line && *line != ' ' && *line != '\t' && *line != '=' && *line != ':')
			line++;
		if (!*line)
			return;
		if (*line == '=' || *line == ':')
			separated = TRUE;
		*line++ = 0;
	}
	while (*line == ' ' || *line == '\t')
		line++;
	if (!separated)
	{
		if (*line != '-' && *line != '=' && *line != ':')
			return;
		line++;
	}
	while (*line == ' ' || *line == '\t')
		line++;
	text = line;
	end = text + strlen(text);
	while (end > text && (end[-1] == ' ' || end[-1] == '\t'))
		*--end = 0;
	if (end - text >= 2 && text[0] == '"' && end[-1] == '"')
	{
		text++;
		*--end = 0;
	}
	if (!*text)
		return;
	/* "\n" in the text starts a new line */
	{
		char *source = text;
		char *destination = text;

		for (; *source; source++)
		{
			if (source[0] == '\\' && source[1] == 'n')
			{
				*destination++ = '\n';
				source++;
			}
			else
			{
				*destination++ = *source;
			}
		}
		*destination = 0;
	}
	subtitle_simplify_text(text);
	length = subtitle_normalize_path(path);
	if (length > 0 && path[length - 1] == '*')
	{
		path[--length] = 0;
		prefix = TRUE;
	}
	if (!length && !prefix)
		return;
	entries[*entry_count].path = path;
	entries[*entry_count].text = text;
	entries[*entry_count].prefix = prefix;
	++*entry_count;
	return;
}

/* the subtitle languages, in the order of the setting's choices and of
tools/embed_assets.py's SUBTITLE_LANGUAGES: display.subtitles_language's word
for each, which also names its file (subtitles_french.txt; English is
subtitles.txt) */
static char const *const subtitle_languages[] =
{
	"english", "french", "german", "italian", "chinese", "chinese_simple", "japanese", "korean",
	"spanish_latam", "spanish", "portuguese",
};

/* the language display.subtitles_language names (English where it names none) */
static long subtitle_language_index(
	void)
{
	char const *word = config_string("display.subtitles_language");
	long index;

	for (index = 0; index < (long)(sizeof(subtitle_languages) / sizeof(subtitle_languages[0])); index++)
	{
		if (!strcmp(word, subtitle_languages[index]))
			return index;
	}
	return 0;
}

/* the name of a language's file: the base name for English, else with the
language's word before the extension ("subtitles.txt": "subtitles_french.txt") */
static void subtitle_language_file_name(
	char const *base_name,
	long language,
	char *name,
	long size)
{
	char const *extension = strrchr(base_name, '.');
	char const *slash = strrchr(base_name, '/');
	char const *backslash = strrchr(base_name, '\\');
	long stem;

	if (backslash && (!slash || backslash > slash))
		slash = backslash;
	if (!extension || (slash && extension < slash))
		extension = base_name + strlen(base_name);
	stem = (long)(extension - base_name);
	if (language <= 0)
		snprintf(name, (size_t)size, "%s", base_name);
	else
		snprintf(name, (size_t)size, "%.*s_%s%s", (int)stem, base_name, subtitle_languages[language], extension);
	return;
}

/* a language's subtitles read whole (a NUL after them, no byte order mark):
the player's file, else the game's own built-in text; NULL, size 0, for
neither */
static char *subtitle_read_layer(
	char const *base_name,
	long language,
	long *size)
{
	char name[512];
	FILE *file = NULL;
	char *data = NULL;

	*size = 0;
	if (base_name && *base_name)
	{
		subtitle_language_file_name(base_name, language, name, sizeof(name));
		file = fopen(name, "rb");
	}
	if (file)
	{
		/* (the player's own file, in place of the game's) */
		long length;

		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fseek(file, 0, SEEK_SET);
		if (length > 0 && length <= MAXIMUM_SUBTITLES_FILE_SIZE)
		{
			data = (char *)malloc((size_t)length + 1);
			if (data && fread(data, 1, (size_t)length, file) == (size_t)length)
			{
				*size = length;
			}
			else
			{
				if (data)
					free(data);
				data = NULL;
			}
		}
		fclose(file);
	}
	else if (language < (long)subtitles_embedded_count && subtitles_embedded_data[language] &&
		subtitles_embedded_sizes[language] > 0 && subtitles_embedded_sizes[language] <= MAXIMUM_SUBTITLES_FILE_SIZE)
	{
		/* the subtitles built into the game */
		long length = (long)subtitles_embedded_sizes[language];

		data = (char *)malloc((size_t)length + 1);
		if (data)
		{
			memcpy(data, subtitles_embedded_data[language], (size_t)length);
			*size = length;
		}
	}
	if (data)
	{
		data[*size] = 0;
		/* (a UTF-8 byte order mark) */
		if (*size >= 3 && (unsigned char)data[0] == 0xEF && (unsigned char)data[1] == 0xBB &&
			(unsigned char)data[2] == 0xBF)
		{
			memmove(data, data + 3, (size_t)(*size - 3 + 1));
			*size -= 3;
		}
	}
	return data;
}

/* the subtitles read again: English, then the language's own over it (a
line it lacks stays English), made into entries and the table */
static void subtitle_load(
	char const *name,
	long language)
{
	long size;
	long english_size = 0;
	long language_size = 0;
	long lines = 1;
	long index;
	char *english;
	char *translated = NULL;
	char *line;
	char *scan;

	subtitle_unload();
	english = subtitle_read_layer(name, 0, &english_size);
	if (language > 0)
		translated = subtitle_read_layer(name, language, &language_size);
	if (!english && !translated)
		return;
	size = english_size + language_size + 1;
	subtitle_globals.buffer = (char *)malloc((size_t)size + 1);
	if (subtitle_globals.buffer)
	{
		if (english)
			memcpy(subtitle_globals.buffer, english, (size_t)english_size);
		subtitle_globals.buffer[english_size] = '\n';
		if (translated)
			memcpy(subtitle_globals.buffer + english_size + 1, translated, (size_t)language_size);
		subtitle_globals.buffer[size] = 0;
	}
	if (english)
		free(english);
	if (translated)
		free(translated);
	if (!subtitle_globals.buffer)
		return;
	line = subtitle_globals.buffer;
	for (scan = line; *scan; scan++)
	{
		if (*scan == '\n')
			lines++;
	}
	subtitle_globals.entries = (struct subtitle_entry *)malloc((size_t)lines * sizeof(struct subtitle_entry));
	if (!subtitle_globals.entries)
	{
		subtitle_unload();
		return;
	}
	for (scan = line; scan;)
	{
		char *next = strchr(scan, '\n');

		if (next)
			*next++ = 0;
		{
			size_t length = strlen(scan);

			if (length && scan[length - 1] == '\r')
				scan[length - 1] = 0;
		}
		subtitle_parse_line(scan, &subtitle_globals.entry_count, subtitle_globals.entries);
		scan = next;
	}
	subtitle_globals.table_size = 16;
	while (subtitle_globals.table_size < 2 * (subtitle_globals.entry_count + 1))
		subtitle_globals.table_size *= 2;
	subtitle_globals.table = (long *)malloc((size_t)subtitle_globals.table_size * sizeof(long));
	if (!subtitle_globals.table)
	{
		subtitle_unload();
		return;
	}
	for (index = 0; index < subtitle_globals.table_size; index++)
		subtitle_globals.table[index] = NONE;
	/* the exact paths in the table (a path named twice keeps its last
	line), the prefixes left to the scan */
	for (index = 0; index < subtitle_globals.entry_count; index++)
	{
		struct subtitle_entry *entry = subtitle_globals.entries + index;
		long existing;

		if (entry->prefix)
			continue;
		existing = subtitle_find_exact(entry->path);
		if (existing != NONE)
			subtitle_globals.entries[existing].text = entry->text;
		else
			subtitle_table_add(index);
	}
	return;
}

/* the text for a sound tag's path (as the tag files name it), NULL for
none: its own line, else the longest prefix's */
static char const *subtitle_find(
	char const *tag_name)
{
	char path[MAXIMUM_SUBTITLE_PATH_LENGTH + 1];
	long index;
	long best = NONE;
	size_t best_length = 0;
	long exact;

	if (!tag_name)
		return NULL;
	strncpy(path, tag_name, MAXIMUM_SUBTITLE_PATH_LENGTH);
	path[MAXIMUM_SUBTITLE_PATH_LENGTH] = 0;
	subtitle_normalize_path(path);
	exact = subtitle_find_exact(path);
	if (exact != NONE)
		return subtitle_globals.entries[exact].text;
	for (index = 0; index < subtitle_globals.entry_count; index++)
	{
		struct subtitle_entry *entry = subtitle_globals.entries + index;
		size_t length;

		if (!entry->prefix)
			continue;
		length = strlen(entry->path);
		if (length >= best_length && !strncmp(path, entry->path, length))
		{
			best = index;
			best_length = length;
		}
	}
	return best != NONE ? subtitle_globals.entries[best].text : NULL;
}

/* UTF-8 text as the wide characters the game draws (what does not fit a
16-bit character, or is not UTF-8, becomes "?") */
static void subtitle_widen(
	char const *text,
	wchar_t *wide,
	long capacity)
{
	unsigned char const *source = (unsigned char const *)text;
	long count = 0;

	while (*source && count < capacity - 1)
	{
		unsigned long code;
		long extra;
		unsigned char first = *source++;

		if (first < 0x80)
		{
			code = first;
			extra = 0;
		}
		else if ((first & 0xE0) == 0xC0)
		{
			code = first & 0x1F;
			extra = 1;
		}
		else if ((first & 0xF0) == 0xE0)
		{
			code = first & 0x0F;
			extra = 2;
		}
		else if ((first & 0xF8) == 0xF0)
		{
			code = first & 0x07;
			extra = 3;
		}
		else
		{
			wide[count++] = (wchar_t)'?';
			continue;
		}
		while (extra > 0 && (*source & 0xC0) == 0x80)
		{
			code = (code << 6) | (*source & 0x3F);
			source++;
			extra--;
		}
		wide[count++] = (extra > 0 || code > 0xFFFF) ? (wchar_t)'?' : (wchar_t)code;
	}
	wide[count] = 0;
	return;
}

/* whether the map is a single player one (a campaign level, played alone or
in co-op, the Custom Edition ones too): there are no subtitles in the main
menu's or a multiplayer map */
static boolean subtitles_map_allowed(
	void)
{
	struct scenario *scenario = global_scenario_try_and_get();

	return scenario && scenario->type == _scenario_type_solo;
}

/* ---------- public code */

boolean subtitles_enabled(
	void)
{
	if (subtitle_globals.changes != config_changes())
	{
		subtitle_globals.changes = config_changes();
		subtitle_globals.enabled = config_boolean("display.subtitles") != 0;
		if (subtitle_globals.enabled)
			subtitle_load(config_string("display.subtitles_file"), subtitle_language_index());
		else
			subtitle_unload();
	}
	return subtitle_globals.enabled && subtitle_globals.entry_count > 0;
}

void subtitles_sound_started(
	long definition_index,
	long delay_milliseconds,
	long length_milliseconds)
{
	char const *text;
	long now;
	long start;
	long end;
	long index;

	if (!subtitles_enabled() || !subtitles_map_allowed())
		return;
	text = subtitle_find(tag_get_name(definition_index));
	if (!text)
		return;
	now = game_time_get();
	start = now + MAX(delay_milliseconds, 0L) * 30 / 1000;
	end = start + PIN(length_milliseconds * 30 / 1000 + SUBTITLE_TAIL_TICKS,
		(long)MINIMUM_SUBTITLE_TICKS, (long)MAXIMUM_SUBTITLE_TICKS);
	/* (the same line again while it is shown stays on: it is not shown twice) */
	for (index = 0; index < subtitle_globals.active_count; index++)
	{
		struct active_subtitle *active = subtitle_globals.active + index;

		if (active->text == text && active->end_tick > now)
		{
			active->end_tick = MAX(active->end_tick, end);
			return;
		}
	}
	if (subtitle_globals.active_count == MAXIMUM_ACTIVE_SUBTITLES)
	{
		memmove(subtitle_globals.active, subtitle_globals.active + 1,
			(MAXIMUM_ACTIVE_SUBTITLES - 1) * sizeof(struct active_subtitle));
		subtitle_globals.active_count--;
	}
	subtitle_globals.active[subtitle_globals.active_count].text = text;
	subtitle_globals.active[subtitle_globals.active_count].start_tick = start;
	subtitle_globals.active[subtitle_globals.active_count].end_tick = end;
	subtitle_globals.active_count++;
	return;
}

void subtitles_draw(
	void)
{
	long font_index;
	struct font_header *font;
	short line_height;
	long now;
	long index;
	long kept = 0;
	rectangle2d screen;
	short left;
	short right;
	short bottom;

	if (!subtitle_globals.active_count)
		return;
	if (!game_in_progress() || !subtitles_enabled() || !subtitles_map_allowed())
	{
		subtitle_globals.active_count = 0;
		return;
	}
	/* (the lines are hidden while the pause menu, or any other menu, is open
	or the game's time is stopped, and come back, with the time they have
	left, when it closes: the lines' time is the game's) */
	if (ui_widgets_active() || game_time_get_paused())
		return;
	now = game_time_get();
	/* (lines that have ended, or that wait beyond reason: the clock went back) */
	for (index = 0; index < subtitle_globals.active_count; index++)
	{
		struct active_subtitle *active = subtitle_globals.active + index;

		if (active->end_tick > now && active->start_tick <= now + MAXIMUM_SUBTITLE_WAIT_TICKS)
			subtitle_globals.active[kept++] = *active;
	}
	subtitle_globals.active_count = kept;
	if (!kept)
		return;

	font_index = interface_get_tag_index(_interface_font_terminal);
	if (font_index == NONE)
		return;
	font = font_definition_get(font_index);
	line_height = (short)(font->ascending_height + font->descending_height + font->leading_height);
	if (line_height <= 0)
		return;
	screen = render.camera.window_bounds;
	offset_rectangle2d(&screen, (short)-render.camera.viewport_bounds.x0, (short)-render.camera.viewport_bounds.y0);
	{
		short width = (short)(screen.x1 - screen.x0);

		left = (short)(screen.x0 + width * (1.0f - SUBTITLE_WIDTH_FRACTION) * 0.5f);
		right = (short)(screen.x1 - width * (1.0f - SUBTITLE_WIDTH_FRACTION) * 0.5f);
	}
	bottom = (short)(screen.y1 - (screen.y1 - screen.y0) *
		(cinematic_in_progress() ? SUBTITLE_CINEMATIC_BOTTOM_FRACTION : SUBTITLE_BOTTOM_FRACTION));

	/* the newest lowest, the older above it */
	for (index = subtitle_globals.active_count - 1; index >= 0; index--)
	{
		struct active_subtitle *active = subtitle_globals.active + index;
		real_argb_color color;
		rectangle2d measure;
		rectangle2d text_bounds;
		rectangle2d cursor_bounds;
		rectangle2d bounds;
		rectangle2d background;
		wchar_t wide[MAXIMUM_SUBTITLE_CHARACTERS + 1];
		short height;
		short margin;
		real alpha;

		if (active->start_tick > now)
			continue;
		alpha = (real)MIN(active->end_tick - now, (long)SUBTITLE_FADE_TICKS) / (real)SUBTITLE_FADE_TICKS;
		color.alpha = alpha;
		color.red = color.green = color.blue = 1.0f;

		measure.y0 = 0;
		measure.y1 = (short)(screen.y1 - screen.y0);
		draw_string_set_draw_mode(font_index, NONE, _subtitle_text_center, _subtitle_wrap_flags, &color);
		subtitle_widen(active->text, wide, MAXIMUM_SUBTITLE_CHARACTERS + 1);
		margin = 0;
		text_hires_force(1);
		measure.x0 = left;
		measure.x1 = right;
		draw_unicode_string_compute_bounds_spaced(&measure, wide, SUBTITLE_LINE_SPACING, &text_bounds, &cursor_bounds);
		text_hires_force(0);
		if (text_bounds.x1 < text_bounds.x0)
			continue;
		height = (short)(text_bounds.y1 - text_bounds.y0);
		if (height <= 0)
			height = line_height;

		bounds.x0 = (short)(left + margin);
		bounds.x1 = (short)(right - margin);
		bounds.y1 = bottom;
		bounds.y0 = (short)(bounds.y1 - height);
		background.x0 = (short)(text_bounds.x0 - SUBTITLE_PADDING);
		background.x1 = (short)(text_bounds.x1 + SUBTITLE_PADDING);
		background.y0 = (short)(bounds.y0 - SUBTITLE_TOP_PADDING);
		background.y1 = (short)(bounds.y1 + SUBTITLE_PADDING / 2);
		draw_quad(&background, subtitle_pixel(0.0f, 0.0f, 0.0f, SUBTITLE_BACKGROUND_ALPHA * alpha));

		/* a shadow, then the text */
		{
			real_argb_color shadow = color;
			rectangle2d shadow_bounds = bounds;

			shadow.red = shadow.green = shadow.blue = 0.0f;
			offset_rectangle2d(&shadow_bounds, 1, 1);
			text_hires_force(1);
			draw_string_set_draw_mode(font_index, NONE, _subtitle_text_center, _subtitle_wrap_flags, &shadow);
			rasterizer_draw_unicode_string(&shadow_bounds, NULL, NULL, SUBTITLE_LINE_SPACING, wide);
		}
		text_hires_force(1);
		draw_string_set_draw_mode(font_index, NONE, _subtitle_text_center, _subtitle_wrap_flags, &color);
		rasterizer_draw_unicode_string(&bounds, NULL, NULL, SUBTITLE_LINE_SPACING, wide);
		text_hires_force(0);

		bottom = (short)(background.y0 - SUBTITLE_GAP);
	}
	return;
}
