/* Exposed Needler crystals illuminate the holder and nearby surfaces;
   the emitter module owns allocation, fading and frame cleanup. */
#include "cseries.h"
#include "objects/objects.h"
#include "items/weapons.h"
#include "units/units.h"
#include "interface/first_person_weapons.h"
#include "needler_lighting.h"
#include "shaders/shader_definitions.h"
#include "rasterizer/xbox/rasterizer_xbox_pixel_shader.h"
#include <string.h>

#include "lighting_config.h"

int needler_lighting_position(long index, real_point3d *position, real_vector3d *direction)
{
	struct weapon_datum *weapon;
	struct unit_datum *unit;
	struct object_marker marker;
	real_vector3d forward, up;
	const char *name;
	if (!lighting_config_boolean("display.needler_lighting"))
		return FALSE;
	weapon = weapon_try_and_get(index);
	if (!weapon || TEST_FLAG(weapon->object.flags, _object_invisible_bit))
		return FALSE;
	name = tag_get_name(weapon->definition_index);
	if (!name || strcmp(name, "weapons\\needler\\needler"))
		return FALSE;
	unit = unit_try_and_get(weapon->object.parent_object_index);
	if (!unit || TEST_FLAG(unit->object.flags, _object_invisible_bit) ||
		unit->unit.current_weapon_index < 0 || unit->unit.current_weapon_index >= MAXIMUM_WEAPONS_PER_UNIT ||
		unit_inventory_get_weapon(weapon->object.parent_object_index, unit->unit.current_weapon_index) != index)
		return FALSE;
	/* Stock FP and world smoke markers share the needle-cluster position:
	   (-.031742, 0, .080221). Sample the interpolated pose for each view. */
	if (!first_person_weapon_adjust_light(index, "smoke", position, &forward, &up))
	{
		if (weapon->object.node_matrices.size < sizeof(real_matrix4x3) ||
			!object_get_marker_by_name(index, "smoke", &marker, 1))
			return FALSE;
		*position = marker.matrix.position;
		up = marker.matrix.up;
	}
	*direction = up;
	return TRUE;
}

/* Cache only within a loaded map; the emitter lifecycle clears this pointer. */
static struct shader *needler_material;
static int needler_material_checked;
static struct shader *needler_core, *needler_world_crystals;
static int needler_crystals_checked;

void needler_lighting_forget(void)
{
	needler_material = NULL;
	needler_core = needler_world_crystals = NULL;
	needler_crystals_checked = FALSE;
	needler_material_checked = FALSE;
}

void needler_lighting_material(struct shader *shader, real_rgb_color *color)
{
	long index;
	if (!lighting_config_boolean("display.needler_lighting"))
		return;
	if (!needler_material_checked)
	{
		index = tag_loaded('soso', "weapons\\needler\\shaders\\needler dull");
		needler_material = index == NONE ? NULL : shader_definition_get(index);
		needler_material_checked = TRUE;
	}
	if (!shader || shader != needler_material)
		return;
	/* Both models share the dull needle material and its authored mask.
	   Retain the transparent crystal layers and any brighter animation. */
	color->red = MAX(color->red, 1.0f);
	color->green = MAX(color->green, 0.30f);
	color->blue = MAX(color->blue, 0.85f);
}

/* Boost only the authored crystal colour stage, before fog. Alpha, texture
   masks, blending and camouflage remain controlled by the stock shader. */
void needler_lighting_transparent(struct shader *shader, struct pixel_shader_definition *pixel, int stages)
{
	long index;
	unsigned long *output;
	if (!lighting_config_boolean("display.needler_lighting") || stages < 1 || stages > 8)
		return;
	if (!needler_crystals_checked)
	{
		index = tag_loaded('sotr', "weapons\\needler\\shaders\\needler core");
		needler_core = index == NONE ? NULL : shader_definition_get(index);
		index = tag_loaded('schi', "weapons\\needler\\shaders\\needler luminous 3p");
		needler_world_crystals = index == NONE ? NULL : shader_definition_get(index);
		needler_crystals_checked = TRUE;
	}
	if (!shader || (shader != needler_core && shader != needler_world_crystals))
		return;
	output = &pixel->rgb_outputs[stages - 1];
	/* NV2A output mapping 0x10 (bits 12+) doubles RGB, with normal clamping.
	   Leave custom biased/scaled stages alone rather than replacing them. */
	if ((*output & 0x38000UL) == 0)
		*output |= 0x10000UL;
}
