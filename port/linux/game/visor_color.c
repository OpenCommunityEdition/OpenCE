/*
VISOR_COLOR.C

Your Spartan's visor in a multiplayer armor color (Extra Effects' SPARTAN ARMOR >
Visor Color, display.visor_color), instead of its gold.

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
players' in the colors they chose (display.others_custom_armor: Show
Players Custom Armor), in co-op too. In a campaign, every Spartan's, so the
cutscenes' Master Chief has it too.

In a network game each machine's visor color goes to the others with its
other Armor choices (armor_sync.c).
*/

#include "cseries.h"
#include "real_math.h"
#include "objects/objects.h"
#include "units/units.h"
#include "game/players.h"
#include "saved games/player_profile.h"
#include "tag_files/tag_groups.h"
#include "game/game_engine.h"
#include "armor_sync.h"

#include <string.h>

/* port/linux/src/port_config.c */
const char *config_string(const char *name);
int config_boolean(const char *name);
unsigned long config_changes(void);
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
/* others' visors shown as they chose (display.others_custom_armor) */
static boolean visor_color_others_shown = FALSE;
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
	visor_color_others_shown = config_boolean("display.others_custom_armor") != 0;
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

/* another machine's player's visor, as the host's table has it
(armor_sync.c; NONE: the gold, or not shown here) */
static short visor_color_of_other(
	long object_index)
{
	struct unit_datum *unit = unit_try_and_get(object_index);
	struct player_datum *player;
	short choices[NUMBER_OF_ARMOR_SYNC_CHOICES];

	if (!visor_color_others_shown || !unit || unit->unit.player_index == NONE)
		return NONE;
	player = player_try_and_get(unit->unit.player_index);
	if (!player || !armor_sync_of_machine(player->network_player_data.machine_index, choices))
		return NONE;
	return VALID_INDEX(choices[_armor_sync_visor], (short)NUMBEROF(visor_color_names)) ? choices[_armor_sync_visor] : NONE;
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
	if ((!game_engine_running() && !armor_sync_network_active()) || visor_color_local_unit(object_index))
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
