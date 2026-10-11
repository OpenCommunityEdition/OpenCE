#ifndef PASSENGER_CAMERA_H
#define PASSENGER_CAMERA_H

#include "cseries/cseries.h"
#include "math/real_math.h"

/* The local player in a Warthog or Scorpion passenger seat, or NONE. */
short passenger_camera_local_player(long unit_index);
boolean passenger_camera_aim(short local_player_index, real_vector3d *forward);
boolean passenger_camera_position(long unit_index, real_point3d *position);
/* While enabled in a passenger seat, including entry, weapon zoom is off. */
boolean passenger_camera_blocks_zoom(short local_player_index);
void passenger_camera_clip_pitch(short local_player_index, real_euler_angles2d *angles);

#endif
