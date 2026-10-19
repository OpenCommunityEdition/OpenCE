/*
LATENCY_METER.C

The latency meter of a network game (display.latency_meter, off by
default): this machine's round trip to the host, "Ping 45 ms" with four
bars, green below 80 ms, yellow below 150, red from 150, at the screen's top
right under the shields while a game is played (the frame rate counter is at
the bottom right, a map's download at the top). With the meter on, the
scoreboard's pings are drawn in the same colours, and where this machine
times a player's round trip itself (the host: every client's; a client: its
own players'), the scoreboard shows that, so it agrees with the meter; the
others are the host's measure in ticks, which every machine is sent
(game_engine.c).

What it shows is the distributed netcode's own round trip (NETCODE.md),
timed in milliseconds rather than ticks (network_distributed.c): nothing is
sent for it, so it is the same over system link, internet play's tunnel
and a browser's WebRTC data channel.

- A client: its players' input names the host's latest tick it had, and the
  host's state of its player names the client's tick the host has it at
  (its own prediction come back; driving, its vehicle's). The time since
  this machine sent that tick is the round trip: the network both ways, and
  each machine's wait for its next tick to send what it had (up to a tick,
  33 ms, each; on a LAN 15 to 35 ms in all). A client that has heard nothing
  from the host for two seconds shows "Connection problem", pulsing,
  instead.
- The host: its clients' round trips, timed the same way by when it sent
  the tick each client's input names. It shows "Max ping", its slowest
  client's (one silent for a while, as long as it has been); alone,
  nothing.

Never shown in the menus, a game over, a co-op cutscene (but a connection
problem), or a game the host plays alone.
*/

#include "cseries.h"
#include "cutscene/cinematics.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "interface/interface.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "network_distributed.h"
#include "latency_meter.h"

#include <stdio.h>

/* ---------- constants */

enum
{
	/* the colours' bounds (milliseconds): green below the first, yellow
	below the second, red from it; four bars below the first, three below
	the second, two below the third, one */
	LATENCY_GOOD_MILLISECONDS = 80,
	LATENCY_FAIR_MILLISECONDS = 150,
	LATENCY_POOR_MILLISECONDS = 250,
	LATENCY_BARS = 4,
	/* the longest shown */
	LATENCY_MAXIMUM_MILLISECONDS = 9999,
	/* the meter's right edge from the screen's, its bars' width and gap,
	and the gap between the bars and the text (the HUD's 480 lines) */
	LATENCY_RIGHT_MARGIN = 12,
	LATENCY_BAR_WIDTH = 4,
	LATENCY_BAR_GAP = 1,
	LATENCY_TEXT_GAP = 4,
	/* draw_string.c's right-justified text (its enum is its own) */
	_latency_text_right = 1,
};

/* its top: this share of the screen's height down, clear of the shields
and health at the top right */
#define LATENCY_TOP_FRACTION 0.15f

/* ---------- prototypes */

/* port_config.c's, cseries_windows.c's, cinematics.c's */
int config_boolean(const char *name);
unsigned long config_changes(void);
unsigned long system_milliseconds(void);
void draw_quad(rectangle2d *rectangle, pixel32 color);

/* ---------- private code */

static pixel32 latency_pixel(
	real_argb_color const *color)
{
	return ((pixel32)(PIN(color->alpha, 0.0f, 1.0f) * 255.0f + 0.5f) << 24) |
		((pixel32)(PIN(color->red, 0.0f, 1.0f) * 255.0f + 0.5f) << 16) |
		((pixel32)(PIN(color->green, 0.0f, 1.0f) * 255.0f + 0.5f) << 8) |
		(pixel32)(PIN(color->blue, 0.0f, 1.0f) * 255.0f + 0.5f);
}

/* ... and over the screen: not over a co-op cutscene (but a connection
problem) */
static boolean latency_meter_drawn(
	void)
{
	return latency_meter_shown() && (!cinematic_in_progress() || distributed_connection_problem());
}

/* ---------- public code */

boolean latency_meter_enabled(
	void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enabled = FALSE;

	if (changes != config_changes())
	{
		changes = config_changes();
		enabled = config_boolean("display.latency_meter") != 0;
	}
	return enabled;
}

boolean latency_meter_shown(
	void)
{
	short connection = game_connection();

	return (connection == _game_connection_network_client || connection == _game_connection_network_server) &&
		latency_meter_enabled() && game_in_progress() && game_engine_in_play();
}

short latency_meter_color(
	long milliseconds,
	real_argb_color *color)
{
	short bars;

	color->alpha = 1.0f;
	if (milliseconds < LATENCY_GOOD_MILLISECONDS)
	{
		bars = 4;
		color->red = 0.35f;
		color->green = 1.0f;
		color->blue = 0.35f;
	}
	else if (milliseconds < LATENCY_FAIR_MILLISECONDS)
	{
		bars = 3;
		color->red = 1.0f;
		color->green = 0.85f;
		color->blue = 0.25f;
	}
	else
	{
		bars = milliseconds < LATENCY_POOR_MILLISECONDS ? 2 : 1;
		color->red = 1.0f;
		color->green = 0.3f;
		color->blue = 0.3f;
	}
	return bars;
}

long latency_meter_player_ping(
	short player_index)
{
	long ping = latency_meter_shown() ? distributed_player_measured_ping(player_index) : NONE;

	return ping != NONE ? ping : distributed_player_ping(player_index);
}

void latency_meter_draw(
	void)
{
	long font_index;
	struct font_header *font;
	short line_height;
	short bar_height;
	short top;
	short right;
	short bars = 0;
	short index;
	long ping;
	char text[32];
	real_argb_color color;
	rectangle2d bounds;

	if (!latency_meter_drawn())
		return;
	font_index = interface_get_tag_index(_interface_font_terminal);
	if (font_index == NONE)
		return;
	font = font_definition_get(font_index);
	line_height = (short)(font->ascending_height + font->descending_height + font->leading_height);
	if (line_height <= 0)
		return;
	if (game_connection() == _game_connection_network_server)
	{
		/* (the host: its slowest client's; alone, nothing) */
		ping = distributed_slowest_client_ping();
		if (ping == NONE)
			return;
		bars = latency_meter_color(ping, &color);
		snprintf(text, sizeof(text), "Max ping %ld ms", MIN(ping, (long)LATENCY_MAXIMUM_MILLISECONDS));
	}
	else if (distributed_connection_problem())
	{
		/* (pulsing, the bars empty) */
		color.alpha = system_milliseconds() % 1000 < 500 ? 1.0f : 0.45f;
		color.red = 1.0f;
		color.green = 0.3f;
		color.blue = 0.3f;
		snprintf(text, sizeof(text), "Connection problem");
	}
	else
	{
		ping = distributed_own_ping();
		if (ping == NONE)
		{
			color.alpha = 1.0f;
			color.red = color.green = color.blue = 0.6f;
			snprintf(text, sizeof(text), "Ping -- ms");
		}
		else
		{
			bars = latency_meter_color(ping, &color);
			snprintf(text, sizeof(text), "Ping %ld ms", MIN(ping, (long)LATENCY_MAXIMUM_MILLISECONDS));
		}
	}

	bounds = render.camera.window_bounds;
	offset_rectangle2d(&bounds, (short)-render.camera.viewport_bounds.x0, (short)-render.camera.viewport_bounds.y0);
	top = (short)(bounds.y0 + (bounds.y1 - bounds.y0) * LATENCY_TOP_FRACTION);
	right = (short)(bounds.x1 - LATENCY_RIGHT_MARGIN);
	/* the bars, drawn quads rising left to right, at the right: lit as many
	as earned, the rest dim */
	bar_height = (short)MAX(line_height - 4, 4);
	for (index = 0; index < LATENCY_BARS; index++)
	{
		rectangle2d bar;
		real_argb_color bar_color = color;
		short height = (short)(bar_height * (index + 1) / LATENCY_BARS);

		if (index >= bars)
		{
			bar_color.red = bar_color.green = bar_color.blue = 0.25f;
			bar_color.alpha = 0.6f * color.alpha;
		}
		bar.x0 = (short)(right - (LATENCY_BARS - index) * (LATENCY_BAR_WIDTH + LATENCY_BAR_GAP) + LATENCY_BAR_GAP);
		bar.x1 = (short)(bar.x0 + LATENCY_BAR_WIDTH);
		bar.y1 = (short)(top + line_height - 2);
		bar.y0 = (short)(bar.y1 - height);
		draw_quad(&bar, latency_pixel(&bar_color));
	}
	/* the text, right-justified to their left */
	bounds.x1 = (short)(right - LATENCY_BARS * (LATENCY_BAR_WIDTH + LATENCY_BAR_GAP) - LATENCY_TEXT_GAP);
	bounds.y0 = top;
	bounds.y1 = (short)(top + line_height);
	draw_string_set_draw_mode(font_index, NONE, _latency_text_right, 0, &color);
	rasterizer_draw_string(&bounds, NULL, NULL, 0, text);
}
