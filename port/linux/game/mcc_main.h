#ifndef MCC_MAIN_H
#define MCC_MAIN_H
boolean mcc_main_load_failed(char const *name);
boolean mcc_main_network_load_failed(char const *name);
boolean mcc_main_map_completed(void);
/* requested: the mission the level's own scripts named with map_name
 * (main_script_map_name), which needs no campaign manifest; NULL for none. */
boolean mcc_main_campaign_won(char const *requested);
#endif
