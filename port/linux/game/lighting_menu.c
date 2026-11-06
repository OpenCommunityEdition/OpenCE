/* Consolidated ON/OFF controls stage changes until the player saves. Old
   preferences are reset once by the platform configuration loader. */
#include "lighting_menu.h"
#include <string.h>
#include <stdio.h>

int config_boolean(const char *name);
int config_write(const char *name, const char *value);

static const struct lighting_menu_group
{
	const char *name;
	const char *members[5];
} groups[] = {
	{"lighting.weapon_glow", {"display.weapon_glow", "display.fuel_rod_lighting",
		"display.ammo_counter_lighting", "display.needler_lighting", NULL}},
	{"lighting.shield_emitters", {"display.jackal_shield_lighting", "display.covenant_shield_lighting", NULL}},
};

static const struct lighting_menu_group *lighting_menu_group(const char *name)
{
	unsigned int i;
	for (i = 0; i < sizeof(groups) / sizeof(groups[0]); i++)
		if (!strcmp(name, groups[i].name))
			return &groups[i];
	return NULL;
}

int lighting_menu_text(const char *name, char *text, unsigned int size, int defaults)
{
	const struct lighting_menu_group *group = lighting_menu_group(name);
	int i, any = 0;
	if (!group)
		return -1;
	if (!defaults)
		for (i = 0; group->members[i]; i++)
		{
			int value = config_boolean(group->members[i]);
			any |= value;
		}
	snprintf(text, size, "%s", defaults || !any ? "false" : "true");
	return 1;
}

int lighting_menu_write(const char *name, const char *value)
{
	const struct lighting_menu_group *group = lighting_menu_group(name);
	int i;
	if (!group)
		return -1;
	if (strcmp(value, "true") && strcmp(value, "false"))
		return 0;
	for (i = 0; group->members[i]; i++)
		if (!config_write(group->members[i], value))
			return 0;
	return 1;
}
