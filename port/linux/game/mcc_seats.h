#ifndef MCC_SEATS_H
#define MCC_SEATS_H

struct unit_datum;
struct player_control_unit_camera_info;
struct unit_datum *mcc_seat_parent(long object_index);
boolean mcc_seat_nearby(long player_index, long target_index, short *seat_index);
boolean mcc_seat_action(long player_index, long target_index, short seat_index);
boolean mcc_seat_update(long unit_index);
void mcc_seat_camera(struct player_control_unit_camera_info *info);

#endif
