/*
VISOR_COLOR.C

Your Spartan's visor in a multiplayer armor color (Video Setup > FOV and
Viewmodels > Visor Color, display.visor_color), instead of its gold; and the
other players' in theirs, with Show Custom Visors (display.others_visor_colors,
off by default: their gold, as stock).

The visor's gold is not a color the game sets: it is painted into the
Spartan's own texture ("characters\cyborg\bitmaps\cyborg", the armor's),
and its shader ("characters\cyborg\shaders\visor") reflects a neutral mirror
cube map over it. So the visor is recolored as it is drawn: each of its
passes' colors made the chosen color at the pass's brightness
(d3d8_gl_set_recolor), its light and its reflections alike. Black is
darker than a color made so: the brightness curved down steeply, so that
the visor is nearly black and only bright reflections show on it, glossy,
as an ODST's.

Only the drawing is changed. In multiplayer, your own players' visors (each
local player's unit, as it is drawn: in third person, in a vehicle; and
their bodies once dead, after they have respawned too), and the other
players' in the colors they chose (display.others_visor_colors). In a
campaign, every Spartan's, so the cutscenes' Master Chief has it too.

In a network game each machine sends its visor color to the host, reliably,
whenever it is not the one the host has for it; the host keeps every
machine's and sends the whole table to every machine, reliably, when it
changes and every ten seconds, in the lobby and in the game. Builds without
it drop both kinds as unknown, as with voice chat (network_voice.c), so the
network version is unchanged. The host takes a machine's color only from its
own stream, and only an armor color's index or none.
*/

#include "cseries.h"
#include "real_math.h"
#include "objects/objects.h"
#include "units/units.h"
#include "game/players.h"
#include "saved games/player_profile.h"
#include "tag_files/tag_groups.h"
#include "bungie_net/common/message_header.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "network_distributed.h"
#include "visor_color.h"
#include "game/game_engine.h"

#include <string.h>

/* port/linux/src/port_config.c, network_distributed.c, cseries_windows.c */
const char *config_string(const char *name);
int config_boolean(const char *name);
unsigned long config_changes(void);
boolean network_distributed_client_send_reliably(void *message, word size);
boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
short network_distributed_server_machines(long *machine_indices, short maximum);
unsigned long system_milliseconds(void);
/* cache_files.c */
long tag_loaded(long group_tag, char const *name);
/* port/linux/src/d3d8_gl.c: the draws that follow recolored (curve 0: not) */
void d3d8_gl_set_recolor(float red, float green, float blue, float curve);

/* shader_model's group tag */
#define VISOR_COLOR_SHADER_MODEL_TAG 0x736f736f /* 'soso' */

/* the armor colors, in the profile's order (player_profile.c) */
static char const *const visor_color_names[] =
{
	"white", "black", "red", "blue", "gray", "yellow", "green", "pink", "purple",
	"cyan", "cobalt", "orange", "teal", "sage", "brown", "tan", "maroon", "salmon",
};
enum { VISOR_COLOR_BLACK = 1 };

/* the visor shaders of the Spartans, as the maps name them */
static char const *const visor_shader_names[] =
{
	"characters\\cyborg\\shaders\\visor",
	"characters\\cyborg_mp\\shaders\\visor",
};

static unsigned long visor_color_changes = (unsigned long)-1;
static short visor_color_index = NONE;
/* the map's visor shaders (looked up again on a new map: visor_color_reset) */
static boolean visor_color_looked = FALSE;
static void const *visor_color_shaders[NUMBEROF(visor_shader_names)];
static boolean visor_color_active = FALSE;

static void visor_color_update(
	void)
{
	char const *value;
	short index;

	if (visor_color_changes == config_changes())
		return;
	visor_color_changes = config_changes();
	visor_color_index = NONE;
	value = config_string("display.visor_color");
	for (index = 0; value && index < (short)NUMBEROF(visor_color_names); index++)
	{
		if (!strcmp(value, visor_color_names[index]))
			visor_color_index = index;
	}
}

static boolean visor_color_is_visor(
	void const *shader)
{
	short index;

	if (!visor_color_looked)
	{
		visor_color_looked = TRUE;
		for (index = 0; index < (short)NUMBEROF(visor_shader_names); index++)
		{
			long tag_index = tag_loaded(VISOR_COLOR_SHADER_MODEL_TAG, visor_shader_names[index]);

			visor_color_shaders[index] = tag_index != NONE ? tag_get(VISOR_COLOR_SHADER_MODEL_TAG, tag_index) : NULL;
		}
	}
	for (index = 0; index < (short)NUMBEROF(visor_shader_names); index++)
	{
		if (shader && shader == visor_color_shaders[index])
			return TRUE;
	}
	return FALSE;
}

/* the bodies local players have had, the latest last: a body keeps its
visor's color once its player is dead (and has another), until it is gone
(an object index's salt is not another object's) */
enum { VISOR_COLOR_BODIES = 8 };
static long visor_color_bodies[VISOR_COLOR_BODIES];
static short visor_color_body_next = 0;

static void visor_color_remember(
	long unit_index)
{
	short index;

	if (unit_index == NONE)
		return;
	for (index = 0; index < VISOR_COLOR_BODIES; index++)
	{
		if (visor_color_bodies[index] == unit_index)
			return;
	}
	visor_color_bodies[visor_color_body_next] = unit_index;
	visor_color_body_next = (short)((visor_color_body_next + 1) % VISOR_COLOR_BODIES);
}

/* a local player's unit, alive or dead, or one they had */
static boolean visor_color_local_unit(
	long object_index)
{
	short local_player;
	short index;

	for (local_player = 0; local_player < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; local_player++)
	{
		long player_index = local_player_get_player_index(local_player);

		if (player_index != NONE)
		{
			struct player_datum *player = player_get(player_index);

			visor_color_remember(player->unit_index);
			visor_color_remember(player->dead_unit_index);
		}
	}
	for (index = 0; index < VISOR_COLOR_BODIES; index++)
	{
		if (visor_color_bodies[index] == object_index)
			return TRUE;
	}
	return FALSE;
}

/* ---------- in network games */

enum
{
	VISOR_COLOR_MACHINES = HALO_PORT_MAXIMUM_NETWORK_MACHINES,
	VISOR_COLOR_SENT_NONE = 0xFF,
	/* how often a machine says its color while the host has another, and
	the host sends its table unchanged (ms) */
	VISOR_COLOR_SAY_MS = 3000,
	VISOR_COLOR_TABLE_MS = 10000,
	VISOR_COLOR_TABLE_SOONEST_MS = 500,
};

/* a machine's color, to the host */
struct distributed_visor_color
{
	byte color; /* an armor color's index, VISOR_COLOR_SENT_NONE for the gold */
	byte pad[3];
};

/* every machine's color, from the host */
struct distributed_visor_colors
{
	byte colors[VISOR_COLOR_MACHINES];
};

struct distributed_visor_color_message
{
	struct distributed_message_header header;
	struct distributed_visor_color entry;
};

struct distributed_visor_colors_message
{
	struct distributed_message_header header;
	struct distributed_visor_colors entry;
};

/* each machine's color as the host has it (heard: a client has had the
host's table) */
static byte visor_color_table[VISOR_COLOR_MACHINES];
static boolean visor_color_table_heard = FALSE;
static boolean visor_color_table_changed = FALSE;
static unsigned long visor_color_table_sent_at = 0;
static unsigned long visor_color_said_at = 0;
static boolean visor_color_in_game = FALSE;

static struct network_game *visor_color_network_game(
	void)
{
	struct network_game_client *client = global_network_game_client_get();

	return client ? network_game_client_get_game(client) : NULL;
}

static boolean visor_color_host(
	void)
{
	return global_network_game_server_get() != NULL;
}

static byte visor_color_sent(
	void)
{
	return visor_color_index == NONE ? VISOR_COLOR_SENT_NONE : (byte)visor_color_index;
}

static void visor_color_fill_header(
	struct distributed_message_header *header,
	byte type,
	word size)
{
	header->header = 0;
	build_message_header(&header->header, size, 2, 0);
	header->type = type;
	header->count = 1;
	header->game_time = game_in_progress() ? game_time_get() : 0;
}

/* another machine's player's visor, as the host's table has it (NONE: the
gold, or not shown here) */
static short visor_color_of_other(
	long object_index)
{
	struct unit_datum *unit = unit_try_and_get(object_index);
	struct player_datum *player;
	long machine_index;
	byte color;

	if (!unit || unit->unit.player_index == NONE || !visor_color_table_heard ||
		!config_boolean("display.others_visor_colors"))
	{
		return NONE;
	}
	player = player_try_and_get(unit->unit.player_index);
	if (!player)
		return NONE;
	machine_index = player->network_player_data.machine_index;
	if (machine_index < 0 || machine_index >= VISOR_COLOR_MACHINES)
		return NONE;
	color = visor_color_table[machine_index];
	return color < NUMBEROF(visor_color_names) ? color : NONE;
}

/* once a frame (interface.c): this machine's color said, and the host's
table sent */
void visor_color_network_update(
	void)
{
	unsigned long now = system_milliseconds();
	long local_machine;

	if (!visor_color_network_game())
	{
		if (visor_color_in_game)
		{
			csmemset(visor_color_table, VISOR_COLOR_SENT_NONE, sizeof(visor_color_table));
			visor_color_table_heard = FALSE;
			visor_color_in_game = FALSE;
		}
		return;
	}
	if (!visor_color_in_game)
	{
		csmemset(visor_color_table, VISOR_COLOR_SENT_NONE, sizeof(visor_color_table));
		visor_color_in_game = TRUE;
		visor_color_said_at = now - VISOR_COLOR_SAY_MS;
		visor_color_table_changed = TRUE;
	}
	visor_color_update();
	local_machine = network_game_client_get_local_machine_index();
	if (visor_color_host())
	{
		long machines[VISOR_COLOR_MACHINES];
		short count;
		short index;
		struct distributed_visor_colors_message message;

		visor_color_table_heard = TRUE;
		if (local_machine >= 0 && local_machine < VISOR_COLOR_MACHINES &&
			visor_color_table[local_machine] != visor_color_sent())
		{
			visor_color_table[local_machine] = visor_color_sent();
			visor_color_table_changed = TRUE;
		}
		if (!(visor_color_table_changed && now - visor_color_table_sent_at >= VISOR_COLOR_TABLE_SOONEST_MS) &&
			now - visor_color_table_sent_at < VISOR_COLOR_TABLE_MS)
		{
			return;
		}
		visor_color_table_changed = FALSE;
		visor_color_table_sent_at = now;
		csmemset(&message, 0, sizeof(message));
		visor_color_fill_header(&message.header, _distributed_message_visor_colors, (word)sizeof(message));
		csmemcpy(message.entry.colors, visor_color_table, sizeof(message.entry.colors));
		count = network_distributed_server_machines(machines, VISOR_COLOR_MACHINES);
		for (index = 0; index < count; index++)
		{
			if (machines[index] != local_machine)
				network_distributed_server_send_to_machine_reliably(machines[index], &message, (word)sizeof(message));
		}
		return;
	}
	/* (a client: its color, while the host has another for it) */
	if (local_machine < 0 || local_machine >= VISOR_COLOR_MACHINES ||
		(visor_color_table_heard && visor_color_table[local_machine] == visor_color_sent()) ||
		now - visor_color_said_at < VISOR_COLOR_SAY_MS)
	{
		return;
	}
	visor_color_said_at = now;
	{
		struct distributed_visor_color_message message;

		csmemset(&message, 0, sizeof(message));
		visor_color_fill_header(&message.header, _distributed_message_visor_color, (word)sizeof(message));
		message.entry.color = visor_color_sent();
		network_distributed_client_send_reliably(&message, (word)sizeof(message));
	}
}

boolean visor_color_handles_message(
	word const *message,
	word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	csmemcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_visor_color || header.type == _distributed_message_visor_colors;
}

void visor_color_handle_message(
	long machine_index,
	word const *message,
	word size,
	boolean from_stream)
{
	struct distributed_message_header header;
	byte const *entry = (byte const *)message + sizeof(header);

	if (!visor_color_handles_message(message, size))
		return;
	csmemcpy(&header, message, sizeof(header));
	if (header.type == _distributed_message_visor_color)
	{
		struct distributed_visor_color color;

		/* (the host, from that machine's own stream: an armor color or none) */
		if (machine_index == NONE || !from_stream || !visor_color_host() ||
			machine_index >= VISOR_COLOR_MACHINES || size < sizeof(header) + sizeof(color))
		{
			return;
		}
		csmemcpy(&color, entry, sizeof(color));
		if (color.color != VISOR_COLOR_SENT_NONE && color.color >= NUMBEROF(visor_color_names))
			return;
		if (visor_color_table[machine_index] != color.color)
		{
			visor_color_table[machine_index] = color.color;
			visor_color_table_changed = TRUE;
		}
	}
	else if (machine_index == NONE && !visor_color_host() &&
		size >= sizeof(header) + sizeof(struct distributed_visor_colors))
	{
		csmemcpy(visor_color_table, entry, sizeof(visor_color_table));
		visor_color_table_heard = TRUE;
	}
}

/* from _rasterizer_model_draw: a model's shader about to be drawn, of an
object (its unique identifier) */
void visor_color_model_begin(
	void const *shader,
	long object_index)
{
	real_rgb_color color;

	short index;

	visor_color_update();
	if (object_index == NONE || !visor_color_is_visor(shader))
		return;
	/* (in a campaign, every Spartan's visor: a cutscene's Master Chief is
	an object of the cutscene's own, not your player's unit; in multiplayer,
	your own, and the others' as they chose) */
	if (!game_engine_running() || visor_color_local_unit(object_index))
		index = visor_color_index;
	else
		index = visor_color_of_other(object_index);
	if (index == NONE)
		return;
	if (index == VISOR_COLOR_BLACK)
	{
		/* (deep black: the brightness curved down, only glints left) */
		d3d8_gl_set_recolor(0.35f, 0.35f, 0.38f, 2.2f);
	}
	else
	{
		/* (a color at the pass's brightness, half again: the gold's own
		brightness is middling) */
		color = player_profile_get_rgb_color(index);
		d3d8_gl_set_recolor(color.red * 1.5f, color.green * 1.5f, color.blue * 1.5f, 1.f);
	}
	visor_color_active = TRUE;
}

/* from _rasterizer_model_draw, as it returns */
void visor_color_model_end(
	void)
{
	if (!visor_color_active)
		return;
	d3d8_gl_set_recolor(0.f, 0.f, 0.f, 0.f);
	visor_color_active = FALSE;
}

/* a new map: its shaders looked up again */
void visor_color_reset(
	void)
{
	short index;

	visor_color_looked = FALSE;
	for (index = 0; index < VISOR_COLOR_BODIES; index++)
		visor_color_bodies[index] = NONE;
}
