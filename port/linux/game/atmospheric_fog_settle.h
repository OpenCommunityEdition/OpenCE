/*
ATMOSPHERIC_FOG_SETTLE.H

The atmospheric fog settling over time as well as over the camera's travel
(atmospheric_fog_settle.c).
*/

#ifndef __ATMOSPHERIC_FOG_SETTLE_H
#define __ATMOSPHERIC_FOG_SETTLE_H

struct atmospheric_fog_state;
struct sky_atmospheric_fog;

/* a local player's fog moved a frame's fraction of the way, by the game's
time since it was last, toward the fog where the camera is */
void atmospheric_fog_settle(short local_player_index, struct atmospheric_fog_state *state,
	struct sky_atmospheric_fog const *fog, real indoor_fog_scale);
/* (the fraction of the way a frame of so many seconds moves it, and the
move itself) */
real atmospheric_fog_settle_fraction(real seconds);
void atmospheric_fog_settle_state(struct atmospheric_fog_state *state, struct sky_atmospheric_fog const *fog,
	real indoor_fog_scale, real fraction);

#endif
