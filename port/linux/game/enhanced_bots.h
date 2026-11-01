/* Optional destinations and tactics; combat and path ownership stay in bots.c. */
#ifndef HALO_ENHANCED_BOTS_H
#define HALO_ENHANCED_BOTS_H
struct unit_datum;
struct player_datum;
void enhanced_bots_initialize_for_new_map(void);
void enhanced_bots_dispose_from_old_map(void);
void enhanced_bots_update(long count, real_point3d const *points);
void enhanced_bots_reset(long index, long player_index);
boolean enhanced_bots_goal(long index, long player_index, real_point3d const *position,
	boolean urgent_only, real_point3d *goal, long *surface);
boolean enhanced_bots_advancing(long index, long unit_index);
boolean enhanced_bots_holding_hill(long unit_index);
boolean enhanced_bots_retreat(struct unit_datum *unit, short skill, long hurt_tick);
boolean enhanced_bots_carrying(long index);
real enhanced_bots_target_priority(struct player_datum *player, long target_index);
void enhanced_bots_aim(long index, short skill, long target_index, long weapon_index,
	real_point3d const *eye, real_point3d *point);
real enhanced_bots_powerup_score(struct unit_datum *unit, long definition_index, real distance,
	short skill, boolean fighting, boolean holding);
#endif
