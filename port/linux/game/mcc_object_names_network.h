#ifndef MCC_OBJECT_NAMES_NETWORK_H
#define MCC_OBJECT_NAMES_NETWORK_H
#include "cseries.h"
void mcc_object_names_network_reset(void);
void mcc_object_names_host_tick(void);
word mcc_object_names_entry_size(void);
void mcc_object_names_receive(void const *entries, short count, long host_time);
#endif
