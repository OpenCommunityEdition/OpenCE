/*
PLASMA_PISTOL_HEAT.C

The plasma pistol's gauge showing its heat (display.plasma_pistol_heat, off
by default: the game's own gauge).

The plasma pistol's model, in first person and in the world, has a gauge
(weapons\plasma pistol\shaders\pp gauge, a meter shader) that fills from
green to red. Its value is the pistol's D function, which follows its
primary trigger's charge: it fills only as an overcharged shot charges, and
stays empty however fast the pistol fires and heats. (The plasma rifle's
gauge follows its heat.) With the setting on, the pistol's gauge shows the
larger of its charge and its heat, so that firing it fast fills the gauge
as it heats, and an overcharged shot still fills it as it charges.

Only what is drawn: the heat is the weapon's own, read as the gauge is
drawn (rasterizer_xbox_transparent_geometry.c).
*/

#include "cseries.h"
#include "objects/objects.h"
#include "items/weapons.h"
#include "tag_files/tag_groups.h"
#include "plasma_pistol_heat.h"

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
unsigned long config_changes(void);

/* the plasma pistol's weapon definition */
#define PLASMA_PISTOL_TAG_NAME "weapons\\plasma pistol\\plasma pistol"

static boolean plasma_pistol_heat_enabled(
	void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enabled = FALSE;

	if (changes != config_changes())
	{
		changes = config_changes();
		enabled = config_boolean("display.plasma_pistol_heat") != 0;
	}
	return enabled;
}

/* whether a weapon definition is the plasma pistol's (the last one asked
about, remembered) */
static boolean plasma_pistol_definition(
	long definition_index)
{
	static long last_definition_index = NONE;
	static boolean last_is_plasma_pistol = FALSE;
	char const *name;

	if (definition_index == NONE)
		return FALSE;
	if (definition_index != last_definition_index)
	{
		name = tag_get_name(definition_index);
		last_definition_index = definition_index;
		last_is_plasma_pistol = name && !csstrcmp(name, PLASMA_PISTOL_TAG_NAME);
	}
	return last_is_plasma_pistol;
}

/* the gauge's value as the larger of its own and the weapon's heat */
real plasma_pistol_heat_gauge_value(
	real value,
	real heat)
{
	if (!(heat > value))
		return value;
	return heat > 1.f ? 1.f : heat;
}

real plasma_pistol_heat_meter_value(
	long object_index,
	real value)
{
	struct weapon_datum *weapon;

	if (!plasma_pistol_heat_enabled() || object_index == NONE)
		return value;
	weapon = (struct weapon_datum *)object_try_and_get_and_verify_type(object_index, _object_mask_weapon);
	if (!weapon || !plasma_pistol_definition(weapon->definition_index))
		return value;
	return plasma_pistol_heat_gauge_value(value, weapon->weapon.heat);
}
