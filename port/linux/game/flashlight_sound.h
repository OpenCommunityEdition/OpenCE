/*
FLASHLIGHT_SOUND.H

The sound a player's flashlight makes as it turns on or off, in
multiplayer as in the campaign (flashlight_sound.c).
*/

#ifndef __FLASHLIGHT_SOUND_H
#define __FLASHLIGHT_SOUND_H

/* the effect a unit's flashlight plays as it turns on or off, given its
definition's: its definition's; for a player's unit with none (the
multiplayer cyborg's), the campaign cyborg's, when the map has it */
long flashlight_sound_toggle_effect(long unit_index, long effect_index);

#endif
