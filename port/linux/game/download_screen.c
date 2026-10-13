/*
DOWNLOAD_SCREEN.C

The screen a map's download shows over the menus: Halo's loading screen
(source/interface/progress_bar.c), its light sweeping from left to right as
the map comes (map_downloads.h: from the community's archives), with the map's name, how
the download goes and B to stop it under it, in the menus' fonts. The menus
behind it go on, so that the join waiting for the map follows once it is
there (menu_functions.c: map_fetch_update), but
they take no presses while it is up: B stops the download, and the join
with it. maps.download_screen = false leaves the menus up instead (the
games' list's status line says how the download goes).
*/

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "input/input.h"
#include "interface/event_manager.h"
#include "interface/progress_bar.h"
#include "interface/ui_widget.h"
#include "rasterizer/rasterizer.h"
#include "tag_files/tag_groups.h"
#include "text/draw_string.h"
#include "text/font_group.h"

/* ---------- constants */

enum
{
	/* (event_manager.c's event type of a button, which it keeps to itself) */
	DOWNLOAD_EVENT_BUTTON = 3,
	/* (draw_string.c's justifications, which it keeps to itself) */
	DOWNLOAD_TEXT_LEFT = 0,
	DOWNLOAD_TEXT_CENTERED = 2,
};

/* ---------- prototypes */

/* port/linux/src (map_downloads.h, port_config.c) */
int map_downloads_progress(float *fraction, char *name, int size);
int map_downloads_fetching(char *status, int size);
int config_boolean(char const *name);
/* menu_functions.c */
void ui_widget_port_map_fetch_cancel(void);
/* ui_widget.c */
boolean main_menu_is_active(void);

/* ---------- globals */

static struct
{
	/* drawn last frame (the screen is let go of when it ends) */
	boolean shown;
	/* the share done, as last drawn: the light never goes back */
	real progress;
} download_screen;

/* ---------- private code */

/* a line of text centered at y in the menus' 640x480, in a menu font */
static void download_screen_line(
	char const *font,
	short y,
	real_argb_color const *color,
	char const *text)
{
	rectangle2d bounds;

	bounds.x0 = 0;
	bounds.x1 = 640;
	bounds.y0 = y;
	bounds.y1 = (short)(y + 40);
	draw_string_set_draw_mode(tag_loaded(FONT_GROUP_TAG, font), NONE, DOWNLOAD_TEXT_CENTERED, 0, color);
	rasterizer_draw_string(&bounds, NULL, NULL, 0, text);
}

/* ---------- public code */

/* whether the screen is up: a map's download under way, at the menus */
boolean download_screen_active(
	void)
{
	float fraction;
	char name[64];

	return main_menu_is_active() && config_boolean("maps.download_screen") &&
		map_downloads_progress(&fraction, name, sizeof(name));
}

/* each frame the menus run while the screen is up (ui_widget.c's
process_ui_widgets): the presses are the screen's, and B stops the download */
void download_screen_process(
	void)
{
	struct event_record event;

	while (get_next_event(&event, NONE))
	{
		if (event.type == DOWNLOAD_EVENT_BUTTON && event.data.button.index == _gamepad_analog_button_b)
		{
			ui_widget_port_map_fetch_cancel();
			break;
		}
	}
	/* (the menus behind take nothing) */
	event_manager_flush();
}

/* each frame, after the menus are drawn (ui_widget.c's render_ui_widgets,
the first player's) */
void download_screen_render(
	void)
{
	real_argb_color const title_color = { 1.0f, 0.9f, 0.93f, 0.99f };
	real_argb_color const status_color = { 1.0f, 0.56f, 0.65f, 0.78f };
	real_argb_color const legend_color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float fraction;
	char name[64];
	char text[256];
	real progress;
	rectangle2d bounds;
	short index;

	/* (nothing drawn without a renderer, debug.null_renderer: the menus'
	high-res button icons, which the legend draws, need OpenGL; B still
	stops the download, download_screen_process) */
	if (!download_screen_active() || config_boolean("debug.null_renderer") ||
		!map_downloads_progress(&fraction, name, sizeof(name)))
	{
		if (download_screen.shown)
		{
			download_screen.shown = FALSE;
			progress_bar_download_end();
		}
		return;
	}
	if (!download_screen.shown)
	{
		download_screen.shown = TRUE;
		download_screen.progress = 0.0f;
	}
	/* (the screen draws from a share above 0, and only forward: a share
	falls back as the next file's size is learned) */
	progress = PIN(fraction, 0.001f, 1.0f);
	if (progress < download_screen.progress)
		progress = download_screen.progress;
	download_screen.progress = progress;
	progress_bar_download_display(progress);

	snprintf(text, sizeof(text), "DOWNLOADING %s", name);
	for (index = 0; text[index]; index++)
	{
		if (text[index] >= 'a' && text[index] <= 'z')
			text[index] = (char)(text[index] - 'a' + 'A');
	}
	download_screen_line("ui\\large_ui", 300, &title_color, text);
	/* (the status line's numbers: "Downloading <name>: " is the title's) */
	if (map_downloads_fetching(text, sizeof(text)))
	{
		char const *numbers = strstr(text, ": ");

		download_screen_line("ui\\small_ui", 334, &status_color, numbers ? numbers + 2 : text);
	}
	/* (the B button's icon, as the menus' legends draw it: from the left, as
	the icon and the words after it are each placed after the one before) */
	bounds.x0 = 290;
	bounds.x1 = 640;
	bounds.y0 = 362;
	bounds.y1 = 402;
	draw_string_set_draw_mode(tag_loaded(FONT_GROUP_TAG, "ui\\small_ui"), NONE, DOWNLOAD_TEXT_LEFT, 0,
		&legend_color);
	draw_string_and_hack_in_icons(&bounds, NULL, NULL, 0, L"%b-button=STOP", FALSE);
}
