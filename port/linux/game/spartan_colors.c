/*
SPARTAN_COLORS.C

Your Spartan's look (the pause menus' ARMOR): its armor in another color,
its lights (the glow above the knees and wherever the armor's shader glows)
in another, and its flashlight's light in another
(display.spartan_armor_color, display.spartan_lights_color,
display.flashlight_color, each "default" for the game's own):

- Armor: any of the multiplayer armor colors, or blackout, a black deeper
  than the profile's black: the profile's black is already none (0, 0, 0),
  and what shows on it is the armor's reflection, so blackout is drawn black
  by the pixel shader (d3d8_gl_set_recolor, as the black visor is:
  visor_color.c), its reflection dimmed (rasterizer_xbox_models.c).
- Lights: any of those but blackout, drawn by the pixel shader as LEDs
  (d3d8_gl_set_glow: their cores the color at full, with a soft halo), and
  bloomed over the view where they are seen (halo_screen_light_bloom).
- Flashlight: any of those but blackout, the light it casts at its own
  brightness.
- The lights, once colored, light the armor around the knees too, in their
  color: a small, faint dynamic light on each knee, as the flashlight's is but
  shining every way (spartan_colors_glow_update, an unattached light as
  shield_glow.c's, with a light definition the map has: no new tags).

Only the drawing is changed (as the shield color's and visor's are), so
saved games and cutscenes are the game's: a cutscene's Master Chief is drawn
so too. In network games (multiplayer and co-op) every machine draws each
player's Spartan as that player chose (armor_sync.c), or as stock with Show
Players Custom Armor off (display.others_custom_armor; their flashlights
with display.others_custom_flashlights); a team game's armor keeps the
team's color.
*/

#include "cseries.h"
#include "real_math.h"
#include "objects/objects.h"
#include "units/units.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "saved games/player_profile.h"
#include "tag_files/tag_groups.h"
#include "objects/object_definitions.h"
#include "objects/object_lights.h"
#include "models/models.h"
#include "camera/observer.h"
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
void d3d8_gl_set_glow(float red, float green, float blue, float halo);
/* object_lights.c (port) */
long light_port_glow_definition_get(void);
boolean light_port_glow_set(long light_index, long object_index, real_rgb_color const *color, real radius);

/* the Spartan's model, as the campaign names it ('mode') */
#define SPARTAN_MODEL_TAG 0x6d6f6465
#define SPARTAN_MODEL "characters\\cyborg\\cyborg"
static boolean spartan_model_looked = FALSE;
static long spartan_model_index = NONE;

/* the Spartan's armor shader (a shader_model), as the campaign names it */
#define SPARTAN_SHADER_MODEL_TAG 0x736f736f /* 'soso' */
#define SPARTAN_ARMOR_SHADER "characters\\cyborg\\shaders\\armor"
/* and its first-person arms' */
#define SPARTAN_ARMS_SHADER "characters\\cyborg\\fp\\shaders\\armor hands"

/* the profile's armor colors, in its order (player_profile.c), and
blackout after them */
static char const *const spartan_color_names[] =
{
	"white", "black", "red", "blue", "gray", "yellow", "green", "pink", "purple",
	"cyan", "cobalt", "orange", "teal", "sage", "brown", "tan", "maroon", "salmon",
	"blackout",
};
enum
{
	SPARTAN_COLOR_BLACKOUT = 18,
	SPARTAN_COLOR_COUNT = 19,
};

static unsigned long spartan_colors_changes = (unsigned long)-1;
/* others' choices shown (display.others_custom_armor), and their
flashlights' (display.others_custom_flashlights) */
static boolean spartan_others_shown = FALSE;
static boolean spartan_others_flashlights = FALSE;
static short spartan_armor = NONE;
static short spartan_lights = NONE;
static short spartan_flashlight = NONE;
/* the biped kind spartan_object last looked at, and whether it is a
Spartan */
static long spartan_checked_definition = NONE;
static boolean spartan_checked_result = FALSE;
static real_rgb_color spartan_change_colors[NUMBER_OF_OBJECT_CHANGE_COLORS];
/* the map's armor shader, once looked up */
static boolean spartan_armor_shader_looked = FALSE;
static void const *spartan_armor_shader = NULL;
static void const *spartan_arms_shader = NULL;
/* a blackout draw recolored (d3d8_gl_set_recolor) until it ends */
static boolean spartan_recolored = FALSE;
/* an armor draw glowing (d3d8_gl_set_glow) until it ends */
static boolean spartan_glowing = FALSE;
/* the lights' halo, added around them: how strong */
#define SPARTAN_LIGHTS_HALO 0.35f

static short spartan_color_named(
	char const *name,
	boolean blackout)
{
	short index;

	for (index = 0; name && index < (short)(blackout ? SPARTAN_COLOR_COUNT : SPARTAN_COLOR_BLACKOUT); index++)
	{
		if (!strcmp(name, spartan_color_names[index]))
			return index;
	}
	return NONE;
}

static void spartan_colors_update(
	void)
{
	if (spartan_colors_changes == config_changes())
		return;
	spartan_colors_changes = config_changes();
	spartan_armor = spartan_color_named(config_string("display.spartan_armor_color"), TRUE);
	spartan_lights = spartan_color_named(config_string("display.spartan_lights_color"), FALSE);
	spartan_flashlight = spartan_color_named(config_string("display.flashlight_color"), FALSE);
	spartan_others_shown = config_boolean("display.others_custom_armor") != 0;
	spartan_others_flashlights = config_boolean("display.others_custom_flashlights") != 0;
}

/* whether any Spartan may be drawn other than the game's way: this machine
has a choice, or shows the others' in a network game (nothing to look up for
any object otherwise) */
static boolean spartan_colors_wanted(
	void)
{
	spartan_colors_update();
	return spartan_armor != NONE || spartan_lights != NONE || spartan_flashlight != NONE ||
		(spartan_others_shown && armor_sync_network_active());
}

static real_rgb_color spartan_color(
	short index)
{
	real_rgb_color color;

	if (index == SPARTAN_COLOR_BLACKOUT)
	{
		color.red = 0.f;
		color.green = 0.f;
		color.blue = 0.f;
		return color;
	}
	return player_profile_get_rgb_color(index);
}

/* a color at a brightness: the chosen color, as bright as the one it
replaces (its strongest channel) */
static void spartan_color_at(
	short index,
	real_rgb_color *color)
{
	real_rgb_color chosen = spartan_color(index);
	real brightness = MAX(color->red, MAX(color->green, color->blue));
	real strongest = MAX(chosen.red, MAX(chosen.green, chosen.blue));
	real scale = strongest > 0.f ? brightness / strongest : 0.f;

	color->red = PIN(chosen.red * scale, 0.f, 1.f);
	color->green = PIN(chosen.green * scale, 0.f, 1.f);
	color->blue = PIN(chosen.blue * scale, 0.f, 1.f);
}

/* whether an object is a Spartan: a biped of the Spartan's model (a
player's, a body, a cutscene's Master Chief, which is a biped of its own) */
static boolean spartan_object(
	long object_index)
{
	struct object_datum *object = object_try_and_get(object_index);
	struct object_definition *definition;

	if (!object || object->object.type != _object_type_biped)
		return FALSE;
	if (!spartan_model_looked)
	{
		spartan_model_looked = TRUE;
		spartan_model_index = tag_loaded(SPARTAN_MODEL_TAG, SPARTAN_MODEL);
	}
	if (spartan_model_index == NONE)
		return FALSE;
	/* (the last biped kind asked about, and whether it is the Spartan's) */
	if (object->definition_index != spartan_checked_definition)
	{
		definition = object_definition_get(object->definition_index);
		spartan_checked_definition = object->definition_index;
		spartan_checked_result = definition && definition->object.model.index == spartan_model_index;
	}
	return spartan_checked_result;
}

/* the player a Spartan is (or was: a body), else NULL */
static struct player_datum *spartan_player(
	long object_index)
{
	struct unit_datum *unit = unit_try_and_get(object_index);
	struct data_iterator iterator;
	struct player_datum *player;

	if (unit && unit->unit.player_index != NONE)
	{
		player = player_try_and_get(unit->unit.player_index);
		if (player)
			return player;
	}
	data_iterator_new(&iterator, player_data);
	while ((player = data_iterator_next(&iterator)) != NULL)
	{
		if (player->unit_index == object_index || player->dead_unit_index == object_index)
			return player;
	}
	return NULL;
}

/* a Spartan's choices (NONE each: the game's own), FALSE if not a Spartan:
this machine's for its own players' (and, outside network games, every
Spartan's: a cutscene's Master Chief); another machine's for its players'
as the host's table has them (armor_sync.c), unless others are shown stock
(display.others_custom_armor, display.others_custom_flashlights); never the
armor's color in a team game, whose colors are the teams' */
static boolean spartan_choices(
	long object_index,
	short choices[NUMBER_OF_ARMOR_SYNC_CHOICES])
{
	struct player_datum *player;
	short choice;

	if (!spartan_colors_wanted() || !spartan_object(object_index))
		return FALSE;
	for (choice = 0; choice < NUMBER_OF_ARMOR_SYNC_CHOICES; choice++)
		choices[choice] = NONE;
	player = spartan_player(object_index);
	if (player ? player->local_player_index != NONE : !armor_sync_network_active())
	{
		choices[_armor_sync_armor] = spartan_armor;
		choices[_armor_sync_lights] = spartan_lights;
		choices[_armor_sync_flashlight] = spartan_flashlight;
	}
	else if (player && spartan_others_shown &&
		armor_sync_of_machine(player->network_player_data.machine_index, choices))
	{
		if (!spartan_others_flashlights)
			choices[_armor_sync_flashlight] = NONE;
	}
	if (game_engine_running() && game_engine_has_teams())
		choices[_armor_sync_armor] = NONE;
	return TRUE;
}

/* whether the first-person arms drawn are a Spartan's: the first local
player's unit */
static boolean spartan_arms_drawn(
	void)
{
	long player_index = local_player_get_player_index(0);
	struct player_datum *player = player_index != NONE ? player_try_and_get(player_index) : NULL;

	return player && player->unit_index != NONE && spartan_object(player->unit_index);
}

/* the map's Spartan armor shader (NULL: none) */
static void const *spartan_armor_shader_get(
	void)
{
	if (!spartan_armor_shader_looked)
	{
		long tag_index = tag_loaded(SPARTAN_SHADER_MODEL_TAG, SPARTAN_ARMOR_SHADER);

		spartan_armor_shader_looked = TRUE;
		spartan_armor_shader = tag_index != NONE ? tag_get(SPARTAN_SHADER_MODEL_TAG, tag_index) : NULL;
		tag_index = tag_loaded(SPARTAN_SHADER_MODEL_TAG, SPARTAN_ARMS_SHADER);
		spartan_arms_shader = tag_index != NONE ? tag_get(SPARTAN_SHADER_MODEL_TAG, tag_index) : NULL;
	}
	return spartan_armor_shader;
}

/* ---------- public code */

/* (render_objects.c, first_person_weapons.c) the colors an object's model
is drawn with: a Spartan's armor in the chosen color, every change color as
multiplayer sets them (players.c), since the campaign's armor shader reads C
where multiplayer's might read any */
real_rgb_color const *spartan_colors_change_colors(
	long object_index,
	real_rgb_color const *colors)
{
	short choices[NUMBER_OF_ARMOR_SYNC_CHOICES];
	short index;

	if (!colors || !spartan_choices(object_index, choices) || choices[_armor_sync_armor] == NONE)
		return colors;
	for (index = 0; index < NUMBER_OF_OBJECT_CHANGE_COLORS; index++)
		spartan_change_colors[index] = spartan_color(choices[_armor_sync_armor]);
	return spartan_change_colors;
}

/* (rasterizer_xbox_models.c) a model's glow as it is drawn: a Spartan's in
the chosen lights' color, and on its armor the lights drawn in that color by
the port's pixel shader (d3d8_gl_set_glow: the lights the color at full,
their white slits too, with a halo), undone as the draw ends */
boolean spartan_colors_self_illumination(
	void const *shader,
	long object_index,
	real_rgb_color *color)
{
	short choices[NUMBER_OF_ARMOR_SYNC_CHOICES];
	real_rgb_color chosen;
	real peak;

	if (!color || !spartan_choices(object_index, choices) || choices[_armor_sync_lights] == NONE)
		return FALSE;
	spartan_color_at(choices[_armor_sync_lights], color);
	if (shader && shader == spartan_armor_shader_get())
	{
		/* (the armor's own glow off: the pixel shader draws its lights,
		and leaves some dark) */
		*color = *global_real_rgb_black;
		chosen = spartan_color(choices[_armor_sync_lights]);
		peak = MAX(chosen.red, MAX(chosen.green, chosen.blue));
		if (peak > 0.f)
		{
			d3d8_gl_set_glow(chosen.red / peak, chosen.green / peak, chosen.blue / peak, SPARTAN_LIGHTS_HALO);
			spartan_glowing = TRUE;
		}
	}
	return TRUE;
}

/* the reflection's darkening of blackout armor: what is left of it */
#define SPARTAN_BLACKOUT_REFLECTION 0.75f

/* (rasterizer_xbox_models.c) how bright a model's shader's reflection is
drawn, of its own: a blackout Spartan's armor dimmed to glints (not its
visor's) */
real spartan_colors_reflection(
	void const *shader,
	long object_index)
{
	short choices[NUMBER_OF_ARMOR_SYNC_CHOICES];
	long owner;

	if (!shader)
		return 1.f;
	/* (the first-person arms are drawn as the weapon, not the unit: the
	first local player's own) */
	if (shader == spartan_armor_shader_get())
		owner = object_index;
	else if (shader == spartan_arms_shader && spartan_arms_drawn())
		owner = player_get(local_player_get_player_index(0))->unit_index;
	else
		return 1.f;
	if (!spartan_choices(owner, choices) || choices[_armor_sync_armor] != SPARTAN_COLOR_BLACKOUT)
		return 1.f;
	/* (and the draw made pure black, as the black visor is: its brightness
	curved down steeply, without hue, the lights' colored glow kept:
	nv2a_psh.c's curve above 100) */
	d3d8_gl_set_recolor(1.614f, 1.614f, 1.670f, 100.f + 1.2f);
	spartan_recolored = TRUE;
	return SPARTAN_BLACKOUT_REFLECTION;
}

/* (rasterizer_xbox_models.c) a model's draw over: its recolor undone */
void spartan_colors_model_end(
	void)
{
	if (spartan_glowing)
	{
		d3d8_gl_set_glow(0.f, 0.f, 0.f, 0.f);
		spartan_glowing = FALSE;
	}
	if (!spartan_recolored)
		return;
	d3d8_gl_set_recolor(0.f, 0.f, 0.f, 0.f);
	spartan_recolored = FALSE;
}

/* ---------- the lights' glow */

enum
{
	SPARTAN_GLOW_SPARTANS = 4,
	SPARTAN_GLOW_KNEES = 2,
};
/* Spartans this near a local player's camera and nearer glow (world units) */
#define SPARTAN_GLOW_RANGE 25.f
/* how far a knee's light reaches (world units, about 35 cm: the armor
around it, not the ground), and its strength */
#define SPARTAN_GLOW_STRENGTH 0.6f
#define SPARTAN_GLOW_RADIUS 0.12f

static char const *const spartan_glow_nodes[SPARTAN_GLOW_KNEES] =
{
	"bip01 l calf", "bip01 r calf", /* (their origins the knees) */
};

struct spartan_glow
{
	long object_index; /* NONE: free */
	long light_indices[SPARTAN_GLOW_KNEES];
	unsigned long frame;
};
static struct spartan_glow spartan_glows[SPARTAN_GLOW_SPARTANS];
static boolean spartan_glows_initialized = FALSE;
static unsigned long spartan_glow_frame = 0;

/* how far a point is from the nearest local player's camera */
static real spartan_glow_camera_distance(
	real_point3d const *point)
{
	real nearest = SPARTAN_GLOW_RANGE * 2.f;
	short local_player_index;

	for (local_player_index = local_player_get_next(NONE); local_player_index != NONE;
		local_player_index = local_player_get_next(local_player_index))
	{
		struct observer_result const *camera = observer_get_camera(local_player_index);
		real distance = camera ? distance3d(point, &camera->position) : nearest;

		if (distance < nearest)
			nearest = distance;
	}
	return nearest;
}

static struct spartan_glow *spartan_glow_find(
	long object_index)
{
	struct spartan_glow *free_glow = NULL;
	short index;

	for (index = 0; index < SPARTAN_GLOW_SPARTANS; index++)
	{
		if (spartan_glows[index].object_index == object_index)
			return &spartan_glows[index];
		if (spartan_glows[index].object_index == NONE && !free_glow)
			free_glow = &spartan_glows[index];
	}
	if (free_glow)
	{
		free_glow->object_index = object_index;
		for (index = 0; index < SPARTAN_GLOW_KNEES; index++)
			free_glow->light_indices[index] = NONE;
	}
	return free_glow;
}

/* each frame, before the lights are gathered (object_lights.c's
lights_preprocess_scene): each colored Spartan's knees lit, near a camera,
seen and not camouflaged */
void spartan_colors_glow_update(
	void)
{
	short index;

	if (!spartan_glows_initialized)
	{
		for (index = 0; index < SPARTAN_GLOW_SPARTANS; index++)
			spartan_glows[index].object_index = NONE;
		spartan_glows_initialized = TRUE;
	}
	spartan_glow_frame++;
	if (spartan_colors_wanted() && (spartan_lights != NONE || spartan_others_shown) &&
		light_port_glow_definition_get() != NONE)
	{
		struct object_iterator iterator;

		object_iterator_new(&iterator, FLAG(_object_type_biped), 0);
		while (object_iterator_next(&iterator))
		{
			struct object_datum *object = object_get(iterator.index);
			struct object_definition *definition;
			struct spartan_glow *glow;
			short choices[NUMBER_OF_ARMOR_SYNC_CHOICES];
			real_rgb_color color;
			real peak;
			short knee;

			if (!spartan_choices(iterator.index, choices) || choices[_armor_sync_lights] == NONE)
				continue;
			color = spartan_color(choices[_armor_sync_lights]);
			peak = MAX(color.red, MAX(color.green, color.blue));
			/* (the color at full: a light is no brighter than full) */
			if (peak > 0.f)
			{
				color.red *= SPARTAN_GLOW_STRENGTH / peak;
				color.green *= SPARTAN_GLOW_STRENGTH / peak;
				color.blue *= SPARTAN_GLOW_STRENGTH / peak;
			}
			if (TEST_FLAG(object->object.flags, _object_invisible_bit) ||
				unit_get(iterator.index)->unit.active_camouflage > 0.f ||
				spartan_glow_camera_distance(&object->object.bounding_sphere_center) > SPARTAN_GLOW_RANGE)
			{
				continue;
			}
			glow = spartan_glow_find(iterator.index);
			if (!glow)
				continue;
			definition = object_definition_get(object->definition_index);
			for (knee = 0; knee < SPARTAN_GLOW_KNEES; knee++)
			{
				if (glow->light_indices[knee] == NONE)
				{
					short node_index = model_find_node(definition->object.model.index, spartan_glow_nodes[knee]);

					if (node_index == NONE)
						continue;
					glow->light_indices[knee] = light_new_unattached(light_port_glow_definition_get(),
						iterator.index, node_index, (real_point3d const *)global_zero_vector3d, global_forward3d, 0.f);
				}
				if (glow->light_indices[knee] != NONE &&
					!light_port_glow_set(glow->light_indices[knee], iterator.index, &color, SPARTAN_GLOW_RADIUS))
				{
					glow->light_indices[knee] = NONE;
				}
			}
			glow->frame = spartan_glow_frame;
		}
	}
	/* (a glow not kept this frame: dark, and let go; its lights go a moment
	after, as the game's own passing lights do) */
	for (index = 0; index < SPARTAN_GLOW_SPARTANS; index++)
	{
		struct spartan_glow *glow = &spartan_glows[index];
		short knee;

		if (glow->object_index == NONE || glow->frame == spartan_glow_frame)
			continue;
		for (knee = 0; knee < SPARTAN_GLOW_KNEES; knee++)
		{
			if (glow->light_indices[knee] != NONE)
				light_port_glow_set(glow->light_indices[knee], glow->object_index, global_real_rgb_black, 0.f);
		}
		glow->object_index = NONE;
	}
}

/* (object_lights.c) a light a Spartan carries, as it shines: the chosen
flashlight color */
void spartan_colors_light(
	long object_index,
	real_rgb_color *color)
{
	short choices[NUMBER_OF_ARMOR_SYNC_CHOICES];

	if (!color || !spartan_choices(object_index, choices) || choices[_armor_sync_flashlight] == NONE)
		return;
	spartan_color_at(choices[_armor_sync_flashlight], color);
}

/* the map's Spartan forgotten (objects.c, a new map) */
void spartan_colors_reset(
	void)
{
	spartan_checked_definition = NONE;
	spartan_checked_result = FALSE;
	spartan_model_looked = FALSE;
	spartan_model_index = NONE;
	spartan_armor_shader_looked = FALSE;
	spartan_armor_shader = NULL;
	spartan_arms_shader = NULL;
	/* (the old map's lights are gone with it) */
	spartan_glows_initialized = FALSE;
}
