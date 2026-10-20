/*
HEADSHOT_PARTY.H

Grunts killed by a headshot pop with confetti and a sound
(display.headshot_party: headshot_party.c).
*/

#ifndef __HEADSHOT_PARTY_H
#define __HEADSHOT_PARTY_H

/* (damage.c's object_damage_aftermath) a unit's body was depleted by a hit
on its head: if it was a grunt and `killing_player_index` (NONE for no
player) shot it, the party begins */
void headshot_party_kill(
	long unit_index,
	long killing_player_index);

/* (hud.c's hud_draw_screen, in each view) where the parties are on the
screen of the first local player's view */
void headshot_party_project(void);

/* (interface.c's interface_draw_fullscreen_overlays) the confetti */
void headshot_party_draw(void);

#endif
