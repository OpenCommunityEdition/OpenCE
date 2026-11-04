#ifndef ENHANCED_BOTS_FLIGHT_H
#define ENHANCED_BOTS_FLIGHT_H
struct player_action;
void enhanced_flight_reset(long index);
void enhanced_flight_action(long index, long vehicle_index, boolean hovering, real_point3d const *goal,
	boolean land, boolean wait, struct player_action *action);
boolean enhanced_flight_landing(long index);
boolean enhanced_flight_failed(long index);
boolean enhanced_flight_can_exit(long vehicle_index);
#endif
