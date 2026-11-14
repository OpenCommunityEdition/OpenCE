#ifndef MCC_MAIN_H
#define MCC_MAIN_H
boolean mcc_main_load_failed(char const *name);
boolean mcc_main_network_load_failed(char const *name);
boolean mcc_main_map_completed(void);
boolean mcc_main_campaign_won(void);
/* MCC's map_name: change to a map of the current package after this update. */
void mcc_main_request_map(char const *name);
#endif
