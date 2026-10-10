/*
SHIELD_GLOW.C

The energy shield's glow (Video Setup > Graphics > Shield Glow,
display.shield_glow): as a unit's shield flares (hit, charging, falling),
it lights what is around it, as a plasma bolt does, in the shield's own
color: a Spartan's gold, an Elite's blue. display.shield_glow_intensity
"light_show" makes it three times as bright and as far.

The flare's strength is the shield shader's own: the intensity its plasma is
drawn with (its intensity source, an object function of the unit, to its
exponent), so the glow rises and falls with the flare seen on the body. Its
color is the shader's edge color at full brightness. The light is an
ordinary dynamic light of the game's (object_lights.c's light_port_glow_*),
drawn with a light definition the map already has: no new assets.

Only the drawing changes: nothing the game simulates, nothing sent between
machines.
*/

#include "cseries.h"
#include "real_math.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "render/render.h"
#include "rasterizer/rasterizer.h"
#include "shaders/shader_definitions.h"
#include "shaders/shaders.h"
#include "units/units.h"

#include <math.h>
#include <string.h>

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
const char *config_string(const char *name);
unsigned long config_changes(void);

/* object_lights.c (port) */
long light_port_glow_new(long object_index, short node_index);
boolean light_port_glow_set(long light_index, long object_index, real_rgb_color const *color, real radius);

enum
{
	SHIELD_GLOW_MAXIMUM = 8,
	_shader_type_transparent_plasma_glow = 10, /* (shader_get_and_verify_type's plasma) */
};

/* units this far from the camera and nearer glow (world units) */
#define SHIELD_GLOW_RANGE 25.f
/* how far the glow reaches, world units: at the flare's least and its most */
#define SHIELD_GLOW_RADIUS_MINIMUM 0.5f
#define SHIELD_GLOW_RADIUS_MAXIMUM 1.4f
/* a flare fainter than this lights nothing */
#define SHIELD_GLOW_THRESHOLD 0.03f

/* the plasma shader's fields after its header (rasterizer_xbox_plasma_energy.c's) */
struct shield_glow_plasma
{
	byte reserved00[4];
	short intensity_exponent_source;
	short pad06;
	real intensity_exponent;
	byte reserved0C[0x2C];
	real perpendicular_alpha;
	real_rgb_color perpendicular_color;
	real parallel_alpha;
	real_rgb_color parallel_color;
};

struct shield_glow
{
	long unit_index; /* NONE: free */
	long light_index;
	unsigned long frame;
};

static struct shield_glow shield_glows[SHIELD_GLOW_MAXIMUM];
static unsigned long shield_glow_frame = 0;

static real shield_glow_scale = 1.f;

/* display.shield_glow, and its intensity (display.shield_glow_intensity):
"default", or "light_show", three times as bright and as far */
static boolean shield_glow_enabled(
	void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enabled = FALSE;

	if (changes != config_changes())
	{
		char const *intensity = config_string("display.shield_glow_intensity");

		changes = config_changes();
		enabled = config_boolean("display.shield_glow") != 0;
		shield_glow_scale = intensity && !strcmp(intensity, "light_show") ? 3.f : 1.f;
	}
	return enabled;
}

/* a unit's flare now: its strength (0: none) and color */
static real shield_glow_flare(
	long unit_index,
	real_rgb_color *color)
{
	struct object_datum *object = object_try_and_get(unit_index);
	struct object_definition *definition;
	struct shader *shader;
	struct shield_glow_plasma const *plasma;
	short source;
	real value;
	real peak;

	if (!object || TEST_FLAG(object->object.flags, _object_invisible_bit))
		return 0.f;
	definition = object_definition_get(object->definition_index);
	if (!definition || definition->object.modifier_shader.index == NONE)
		return 0.f;
	shader = shader_definition_get(definition->object.modifier_shader.index);
	if (!shader || shader->base.type != _shader_type_transparent_plasma_glow)
		return 0.f;
	plasma = (struct shield_glow_plasma const *)((byte const *)shader + sizeof(struct shader));
	source = plasma->intensity_exponent_source;
	if (source < 1 || source > 4)
		return 0.f;
	value = object->object.outgoing_function_values[source - 1];
	if (!(value > 0.f))
		return 0.f;
	value = (real)pow((double)value, (double)plasma->intensity_exponent);

	*color = plasma->perpendicular_color;
	peak = MAX(color->red, MAX(color->green, color->blue));
	if (!(peak > 0.f))
		return 0.f;
	color->red /= peak;
	color->green /= peak;
	color->blue /= peak;
	if (unit_get(unit_index)->unit.active_camouflage > 0.f)
		value *= 1.f - unit_get(unit_index)->unit.active_camouflage;
	return PIN(value, 0.f, 1.f);
}

static struct shield_glow *shield_glow_find(
	long unit_index,
	boolean make)
{
	struct shield_glow *free_glow = NULL;
	short index;

	for (index = 0; index < SHIELD_GLOW_MAXIMUM; index++)
	{
		if (shield_glows[index].unit_index == unit_index)
			return &shield_glows[index];
		if (shield_glows[index].unit_index == NONE && !free_glow)
			free_glow = &shield_glows[index];
	}
	if (!make || !free_glow)
		return NULL;
	free_glow->unit_index = unit_index;
	free_glow->light_index = NONE;
	return free_glow;
}

/* each frame, before the lights are gathered (lights_preprocess_scene) */
void shield_glow_update(
	void)
{
	static boolean initialized = FALSE;
	struct object_iterator iterator;
	short index;

	if (!initialized)
	{
		for (index = 0; index < SHIELD_GLOW_MAXIMUM; index++)
			shield_glows[index].unit_index = NONE;
		initialized = TRUE;
	}
	shield_glow_frame++;
	if (shield_glow_enabled())
	{
		object_iterator_new(&iterator, _object_mask_unit, 0);
		while (object_iterator_next(&iterator))
		{
			struct object_datum *object = object_get(iterator.index);
			real_rgb_color color;
			real flare;
			struct shield_glow *glow;

			if (distance3d(&object->object.bounding_sphere_center, &global_window_parameters.camera.position) >
				SHIELD_GLOW_RANGE)
			{
				continue;
			}
			flare = shield_glow_flare(iterator.index, &color);
			glow = shield_glow_find(iterator.index, flare > SHIELD_GLOW_THRESHOLD);
			if (!glow)
				continue;
			if (flare <= SHIELD_GLOW_THRESHOLD)
				continue; /* (let go below) */
			if (glow->light_index == NONE)
				glow->light_index = light_port_glow_new(iterator.index, 0);
			color.red *= flare * shield_glow_scale;
			color.green *= flare * shield_glow_scale;
			color.blue *= flare * shield_glow_scale;
			if (glow->light_index != NONE &&
				light_port_glow_set(glow->light_index, iterator.index, &color,
					(SHIELD_GLOW_RADIUS_MINIMUM + (SHIELD_GLOW_RADIUS_MAXIMUM - SHIELD_GLOW_RADIUS_MINIMUM) * flare) *
						shield_glow_scale))
			{
				glow->frame = shield_glow_frame;
			}
			else
			{
				glow->light_index = NONE;
			}
		}
	}
	/* (a glow not kept this frame: dark, and let go; its light goes a moment
	after, as the game's own passing lights do) */
	for (index = 0; index < SHIELD_GLOW_MAXIMUM; index++)
	{
		struct shield_glow *glow = &shield_glows[index];

		if (glow->unit_index == NONE || glow->frame == shield_glow_frame)
			continue;
		if (glow->light_index != NONE)
			light_port_glow_set(glow->light_index, glow->unit_index, global_real_rgb_black, 0.f);
		glow->unit_index = NONE;
		glow->light_index = NONE;
	}
}
