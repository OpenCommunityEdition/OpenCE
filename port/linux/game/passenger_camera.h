/* The local Warthog and Scorpion passenger view (passenger_camera.c). */
#ifndef PASSENGER_CAMERA_H
#define PASSENGER_CAMERA_H

#include "math/real_math.h"

struct player_control_unit_camera_info;

boolean passenger_camera_enabled(short local_player_index);
boolean passenger_camera_info(short local_player_index, struct player_control_unit_camera_info *info);
void passenger_camera_clamp_facing(short local_player_index, real_euler_angles2d *facing);

/* Scoped to crosshairs_draw: one ray query for all of a reticle's pieces. */
boolean passenger_camera_reticle_begin(short local_player_index, long weapon_index);
void passenger_camera_reticle_offset(point2d *point);
void passenger_camera_reticle_end(void);

#endif
