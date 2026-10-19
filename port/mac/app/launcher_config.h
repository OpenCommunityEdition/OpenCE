/* PowerSpark's startup dialog: the settings it shows and how they are read from and
   written to config.toml. Plain C89 and stdio only, so that a host test can run it
   (test_launcher_config.c). The keys are the ones the game's own Settings menus use
   (tools/port_settings.py), so the dialog and the menus keep to one file. */
#ifndef LAUNCHER_CONFIG_H
#define LAUNCHER_CONFIG_H

#include <stdio.h>
#include <stddef.h>

enum lc_id
{
	LC_MODE,		/* display.mode: "fullscreen", "borderless" or "windowed" */
	LC_RESOLUTION,		/* display.resolution: "native" or "WxH" (what fullscreen sets) */
	LC_WINDOW_SIZE,		/* display.window_size: "" or "WxH" */
	LC_SCALING,		/* display.resolution_scaling: "native" (widescreen) or "original" */
	LC_VSYNC,
	LC_MAX_FPS,
	LC_INTERPOLATION,
	LC_GAMMA,		/* display.gamma, 0.5 to 2.0 */
	LC_HIRES_HUD,
	LC_HIRES_TEXT,
	LC_ANTI_ALIASING,
	LC_SHADOW_RESOLUTION,
	LC_PER_PIXEL_LIGHTING,
	LC_DETAIL,		/* mac.detail */
	LC_TEXTURE_SKIP,	/* mac.texture_skip_levels */
	LC_RENDER,		/* mac.render_resolution: auto, native, original, or a line count */
	LC_ADAPTIVE,		/* mac.adaptive_detail: auto, off, or a target frame rate */
	LC_AUDIO,		/* audio.enabled */
	LC_VOLUME,		/* audio.volume, 0.0 to 1.0 */
	LC_REVERB,
	LC_MOVIES,		/* game.movies (the dialog sets game.intro_movies with it) */
	LC_INTRO_MOVIES,
	LC_ACHIEVEMENTS,	/* game.achievements */
	LC_PROMPT_GLYPHS,	/* input.prompt_glyphs: "auto", "xbox" or "playstation" */
	LC_GAMEPAD_LAYOUT,	/* input.gamepad_layout: "profile", "halo2", "halo3", "halo4", "halo5" or "infinite" */
	LC_UNLIMITED_SPRINT,	/* gameplay.unlimited_sprint */
	LC_COUNT
};

#define LC_VALUE 48

struct lc_model
{
	char value[LC_COUNT][LC_VALUE];		/* what the dialog shows, as written in the file (no quotes) */
	char original[LC_COUNT][LC_VALUE];	/* what it was when the dialog opened */
	char data_source[16], data_source_original[16];	/* "auto", "xbox" or "retail" (the data-source file) */
	int hide, hide_original;		/* "Don't show this at startup" (the startup-dialog file) */
};

const char *lc_key(int id);
void lc_model_defaults(struct lc_model *m);
/* the file's text (NULL: none) over the defaults; the originals are set to what is loaded */
void lc_model_load(struct lc_model *m, const char *text);
/* writes the settings that changed to path (created if missing), each into its own
   section, leaving every other byte; 0 or -1 */
int lc_model_save(const struct lc_model *m, const char *path);
/* a line per setting, for --dialog-dump */
void lc_model_dump(const struct lc_model *m, FILE *out);

/* the value of "section.name", from "name = ..." under [section] or "section.name = ..."
   at the top; quotes and a trailing comment taken off. 1 if found */
int lc_text_get(const char *text, const char *key, char *out, size_t size);
/* Moves each dotted "section.name = value" line at the top of the text into [section],
   when the file has that table further down (the old Controller Setup wrote its keys so,
   and TOML then refuses the file: "table defined before"). The moved line's value
   replaces the table's own value of that key. Returns the lines moved, or -1. */
int lc_text_repair(char **text, size_t *length);
/* A parse of the text as the game parses config.toml (launcher_toml_check.c). 1 if it is valid;
   else 0, with the parser's message (it names the line) in message. */
int lc_text_check(const char *text, size_t length, char *message, size_t size);
/* sets "section.name" to raw (TOML text: quote strings) in *text (malloc'd, grown); 0 or -1 */
int lc_text_set(char **text, size_t *length, const char *key, const char *raw);

int lc_bool(const char *value);
void lc_set_bool(struct lc_model *m, int id, int on);
/* a real as tenths (1.0 is 10), rounded and clamped to min..max */
int lc_tenths(const char *value, int min, int max);
void lc_set_tenths(struct lc_model *m, int id, int tenths);

#endif
