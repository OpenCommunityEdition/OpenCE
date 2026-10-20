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

/* (network_coop.c) the host's party, at a point of the world, on a client */
void headshot_party_network(real_point3d const *world);

/* (hud.c's hud_draw_screen, in each player's view) the confetti, where the
parties are in this view */
void headshot_party_draw_view(void);

#endif
