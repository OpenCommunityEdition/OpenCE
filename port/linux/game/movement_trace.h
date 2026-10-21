/*
MOVEMENT_TRACE.H

The reference traces of the native engine's movement (movement_trace.c).
*/

#ifndef MOVEMENT_TRACE_H
#define MOVEMENT_TRACE_H

/* debug.movement_trace's script in place of the first local player's
throttle, turning, jump and crouch (nothing without a script) */
void movement_trace_input(short local_player_index, real_vector2d *throttle,
	real_euler_angles2d *facing_delta, unsigned long *unit_control_flags);
/* the first local player's biped logged, once a tick, after the players'
update */
void movement_trace_update(void);

#endif
