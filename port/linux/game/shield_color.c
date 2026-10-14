/*
SHIELD_COLOR.C

Your energy shield's color (the pause menu's ARMOR > Shield Color): the
flare in one of the multiplayer armor colors instead of the shield's own,
whatever color the armor is. The shield's own shader is drawn, only its two
colors are replaced, keeping their brightness: no new assets.

In a network game every machine draws each player's shield in the color
that player chose, if it shows players' custom armor
(display.others_custom_armor): the color goes with the Armor menu's other
choices, to the host and from the host to every machine, in the lobby and
mid-game (armor_sync.c). A machine's own players' are its setting as it is
now.

Only the drawing is changed; the shield itself is the game's.
*/

#include "cseries.h"
#include "real_math.h"
#include "objects/objects.h"
#include "units/units.h"
#include "game/players.h"
#include "saved games/player_profile.h"

#include "shield_color.h"
#include "armor_sync.h"

#include <string.h>

/* port/linux/src/port_config.c */
const char *config_string(const char *name);
int config_boolean(const char *name);
unsigned long config_changes(void);

/* the multiplayer armor colors, in the profile's order (player_profile.c) */
static char const *const shield_color_names[] =
{
	"white", "black", "red", "blue", "gray", "yellow", "green", "pink", "purple",
	"cyan", "cobalt", "orange", "teal", "sage", "brown", "tan", "maroon", "salmon",
};

static unsigned long shield_color_changes = (unsigned long)-1;
static short shield_color_index = NONE;
static boolean shield_color_others_shown = FALSE;

short shield_color_name_index(
	char const *name)
{
	short index;

	for (index = 0; name && index < (short)NUMBEROF(shield_color_names); index++)
	{
		if (!strcmp(name, shield_color_names[index]))
			return index;
	}
	return NONE;
}

static void shield_color_update(
	void)
{
	if (shield_color_changes == config_changes())
		return;
	shield_color_changes = config_changes();
	shield_color_others_shown = config_boolean("display.others_custom_armor") != 0; /* (the Armor menu's one switch for every custom look) */
	shield_color_index = shield_color_name_index(config_string("display.shield_color"));
}

/* the shield color of a unit's player: this machine's own players' its
setting, another machine's player's the one it sent; NONE for none (the
shield's own, or no player's) */
static short shield_color_of_unit(
	long unit_index)
{
	struct unit_datum *unit = unit_try_and_get(unit_index);
	struct player_datum *player;
	short choices[NUMBER_OF_ARMOR_SYNC_CHOICES];

	if (!unit || unit->unit.player_index == NONE)
		return NONE;
	player = player_try_and_get(unit->unit.player_index);
	if (!player)
		return NONE;
	if (player->local_player_index != NONE)
		return shield_color_index;
	/* (another machine's player's, only if this machine shows them:
	display.others_custom_armor) */
	if (!shield_color_others_shown)
		return NONE;
	if (!armor_sync_of_machine(player->network_player_data.machine_index, choices))
		return NONE;
	return VALID_INDEX(choices[_armor_sync_shield], (short)NUMBEROF(shield_color_names)) ?
		choices[_armor_sync_shield] : NONE;
}

/* the colors a unit's shield is drawn with, given the unit's own: when the
unit's player has a shield color, a copy of them that shield_color_override
knows again (one for each player); otherwise the unit's own */
static real_rgb_color shield_color_marker[HALO_PORT_MAXIMUM_NETWORK_PLAYERS][4];
static short shield_color_marker_index[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];

real_rgb_color const *shield_color_colors(
	long unit_index,
	real_rgb_color const *colors)
{
	short color;
	long slot;

	if (!colors)
		return colors;
	shield_color_update();
	color = shield_color_of_unit(unit_index);
	if (color == NONE)
		return colors;
	slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_get(unit_index)->unit.player_index);
	if (!VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
		return colors;
	csmemcpy(shield_color_marker[slot], colors, sizeof(shield_color_marker[slot]));
	shield_color_marker_index[slot] = color;
	return shield_color_marker[slot];
}

/* the chosen color, when a plasma shader is drawn with colors from
shield_color_colors (FALSE: drawn as it is) */
boolean shield_color_override(
	real_rgb_color const *colors,
	real_rgb_color *chosen)
{
	long slot;

	if (!colors || colors < &shield_color_marker[0][0] ||
		colors >= &shield_color_marker[0][0] + NUMBEROF(shield_color_marker) * NUMBEROF(shield_color_marker[0]))
	{
		return FALSE;
	}
	slot = (long)((real_rgb_color const (*)[4])colors - shield_color_marker);
	if (colors != shield_color_marker[slot])
		return FALSE;
	*chosen = player_profile_get_rgb_color(shield_color_marker_index[slot]);
	/* (at full brightness: shield_color_apply) */
	{
		real peak = MAX(chosen->red, MAX(chosen->green, chosen->blue));

		if (peak > 0.f)
		{
			chosen->red /= peak;
			chosen->green /= peak;
			chosen->blue /= peak;
		}
	}
	return TRUE;
}

/* a shield color in place of one of the shader's own: the chosen color (an
armor color's at full brightness: shield_color_override) at the original's
brightness (its brightest channel) */
void shield_color_apply(
	real_rgb_color const *chosen,
	real_rgb_color const *original,
	real_rgb_color *result)
{
	real brightness = MAX(original->red, MAX(original->green, original->blue));
	real peak = MAX(chosen->red, MAX(chosen->green, chosen->blue));

	if (peak < 0.12f)
	{
		/* (black: the flare is added to what is behind it, so black would
		not show at all; a dark smoke-grey instead) */
		result->red = result->green = result->blue = brightness * 0.12f;
		return;
	}
	result->red = chosen->red * brightness;
	result->green = chosen->green * brightness;
	result->blue = chosen->blue * brightness;
}
