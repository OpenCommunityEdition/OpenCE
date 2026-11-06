/* The AR display's rendered pose and optional material emission.
   The emitter module owns light allocation, fading and frame cleanup. */
#include "cseries.h"
#include <xtl.h>
#include "objects/objects.h"
#include "items/weapons.h"
#include "units/units.h"
#include "interface/first_person_weapons.h"
#include "ammo_counter_lighting.h"
#include "shaders/shader_definitions.h"
#include "rasterizer/xbox/rasterizer_xbox.h"
#include <string.h>

#include "lighting_config.h"

int ammo_counter_lighting_position(long index, real_point3d *position, real_vector3d *direction)
{
	struct weapon_datum *weapon;
	struct unit_datum *unit;
	struct object_marker marker;
	real_vector3d forward, up;
	real height;
	int pose = 1;
	const char *name;
	if (!lighting_config_boolean("display.ammo_counter_lighting"))
		return FALSE;
	weapon = weapon_try_and_get(index);
	if (!weapon || TEST_FLAG(weapon->object.flags, _object_invisible_bit))
		return FALSE;
	name = tag_get_name(weapon->definition_index);
	if (!name || strcmp(name, "weapons\\assault rifle\\assault rifle"))
		return FALSE;
	unit = unit_try_and_get(weapon->object.parent_object_index);
	if (!unit || TEST_FLAG(unit->object.flags, _object_invisible_bit) ||
		unit->unit.current_weapon_index < 0 || unit->unit.current_weapon_index >= MAXIMUM_WEAPONS_PER_UNIT ||
		unit_inventory_get_weapon(weapon->object.parent_object_index, unit->unit.current_weapon_index) != index)
		return FALSE;
	/* Both stock displays lie near x=.047, z=.069 in model space. Place the
	   light just behind/above that surface, using the identity-oriented muzzle
	   marker. The FP muzzle is .002744 units higher than the world muzzle. */
	if (first_person_weapon_adjust_light(index, "primary trigger", position, &forward, &up))
	{
		height = 0.025f;
		pose = 2;
	}
	else
	{
		if (weapon->object.node_matrices.size < sizeof(real_matrix4x3) ||
			!object_get_marker_by_name(index, "primary trigger", &marker, 1))
			return FALSE;
		*position = marker.matrix.position;
		forward = marker.matrix.forward;
		up = marker.matrix.up;
		height = 0.027744f;
	}
	/* Broad rear/up-facing emission approximates the sloped display surface.
	   Derive it from the rendered weapon basis, never world up. */
	direction->i = -0.8f * forward.i + 0.6f * up.i;
	direction->j = -0.8f * forward.j + 0.6f * up.j;
	direction->k = -0.8f * forward.k + 0.6f * up.k;
	position->x += -0.18f * forward.i + height * up.i;
	position->y += -0.18f * forward.j + height * up.j;
	position->z += -0.18f * forward.k + height * up.k;
	return pose;
}

/* Cache only within a loaded map; the emitter lifecycle clears this pointer. */
static struct shader *ammo_counter_screen;
static int ammo_counter_screen_checked;
static int ammo_counter_draw_active;
static unsigned long ammo_counter_draw_output;

void ammo_counter_lighting_forget(void)
{
	ammo_counter_screen = NULL;
	ammo_counter_screen_checked = FALSE;
	ammo_counter_draw_active = FALSE;
}

static int ammo_counter_is_screen(struct shader *shader)
{
	long index;
	if (!lighting_config_boolean("display.ammo_counter_lighting"))
		return FALSE;
	if (!ammo_counter_screen_checked)
	{
		index = tag_loaded('soso', "weapons\\assault rifle\\fp\\shaders\\display");
		ammo_counter_screen = index == NONE ? NULL : shader_definition_get(index);
		ammo_counter_screen_checked = TRUE;
	}
	return shader && shader == ammo_counter_screen;
}

void ammo_counter_lighting_screen(struct shader *shader, real_rgb_color *color)
{
	if (!ammo_counter_is_screen(shader))
		return;
	/* The authored multipurpose-map mask confines emission to the screen.
	   Both stock AR models share this shader; retain any brighter animation. */
	color->red = MAX(color->red, 0.45f);
	color->green = MAX(color->green, 1.0f);
	color->blue = MAX(color->blue, 1.0f);
}

/* Colour constants are already saturated. Boost the completed display RGB
   before fog, confined to this material in either model. Restore the exact
   output after drawing so other materials and views remain unchanged. */
void ammo_counter_lighting_draw_begin(struct shader *shader, unsigned long output)
{
	if (!ammo_counter_is_screen(shader) || (output & 0x38000UL))
		return;
	ammo_counter_draw_output = output;
	ammo_counter_draw_active = TRUE;
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_PSRGBOUTPUTS7, output | 0x10000UL);
}

void ammo_counter_lighting_draw_end(void)
{
	if (ammo_counter_draw_active)
	{
		IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_PSRGBOUTPUTS7, ammo_counter_draw_output);
		ammo_counter_draw_active = FALSE;
	}
}
