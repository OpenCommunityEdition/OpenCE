/* Read-only engine queries needed by the optional enhanced bots.
The engine owns the flag and moving-hill state; bots never mutate it. */
#ifndef HALO_BOTS_OBJECTIVES_H
#define HALO_BOTS_OBJECTIVES_H

boolean game_engine_ctf_flag_state(long team_index, real_point3d *position,
	long *carrier_player_index, boolean *at_home, real_point3d *home);
boolean game_engine_king_hill(real_point3d *points, long maximum, long *point_count,
	real *bottom, real *top, real_point3d *center, long *hill_id);
boolean game_engine_king_point_on_hill(real_point3d const *position);

#endif
