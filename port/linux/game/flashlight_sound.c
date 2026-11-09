/*
FLASHLIGHT_SOUND.C

The sound a player's flashlight makes as it turns on or off, in
multiplayer as in the campaign.

A unit's flashlight plays its definition's integrated light toggle effect
as it turns on or off (units.c). The campaign's cyborg
(characters\cyborg\cyborg) names weapons\assault rifle\effects\flashlight,
a click (sound\sfx\weapons\assault rifle\flashlight); the multiplayer
cyborg (characters\cyborg_mp\cyborg_mp) names none, in every multiplayer
map, so that a multiplayer flashlight made no sound on the Xbox either.

A player's unit whose definition names no toggle effect plays the campaign
cyborg's, when the map has it (every Xbox multiplayer map does: the
campaign cyborg is in each). It is only a sound, and not a deterministic
effect: nothing the machines agree on changes.
*/

#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "effects/effect_definitions.h"
#include "objects/objects.h"
#include "units/units.h"
#include "flashlight_sound.h"

/* the campaign cyborg's flashlight toggle effect */
#define FLASHLIGHT_SOUND_EFFECT_TAG_NAME "weapons\\assault rifle\\effects\\flashlight"

long flashlight_sound_toggle_effect(
	long unit_index,
	long effect_index)
{
	struct unit_datum *unit;

	if (effect_index != NONE)
		return effect_index;
	unit = (struct unit_datum *)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
	if (!unit || unit->unit.player_index == NONE)
		return NONE;
	return tag_loaded(EFFECT_DEFINITION_TAG, FLASHLIGHT_SOUND_EFFECT_TAG_NAME);
}
