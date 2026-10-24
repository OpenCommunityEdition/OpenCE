/*
FRAME_RATE_DISPLAY.C

The frame rate counter of a game (display.show_fps, off by default): the
number of frames a second at the screen's bottom right, green from 30 and
red below, counted over half a second (main.c's main_framerate_render,
which the game already had: it draws when display_framerate is set, a
console variable of the Xbox version).

The counter is the game's; this is what ties it to a setting, so that the
settings screens show it (tools/port_settings.py) and that config.toml
keeps it, rather than the player typing `display_framerate 1` into the
console each time. The setting is read when it changes (config_changes),
so a menu's change shows in the next frame, and the console can still turn
the counter on and off by itself in between.

- On Android and the web the counter is the port's easiest measure of what
  the device manages, the frame rate being what its GPU or browser gives.
*/

#include "cseries.h"
#include "main/main.h"
#include "frame_rate_display.h"

/* port_config.c's */
int config_boolean(const char *name);
unsigned long config_changes(void);

void frame_rate_display_update(
	void)
{
	static unsigned long read_at = (unsigned long)-1;

	if (read_at == config_changes())
		return;
	read_at = config_changes();
	display_framerate = config_boolean("display.show_fps") != 0;
}
