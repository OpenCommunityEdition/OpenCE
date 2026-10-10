#ifndef MCC_PLAYER_H
#define MCC_PLAYER_H
struct player_action;
struct scenario_starting_profile;
boolean mcc_player_choice_supported(short choice);
boolean mcc_player_action_valid(struct player_action const *action);
void mcc_player_profile_grenades(long unit, struct scenario_starting_profile const *profile, boolean reset);
void mcc_player_postspawn_grenades(long unit, boolean disabled);
#endif
