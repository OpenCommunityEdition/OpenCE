/* MCC-only extra grenade inventories. The ordinary inventory wire layout
 * and the two native grenade fields are unchanged. */
#ifndef MCC_NETWORK_H
#define MCC_NETWORK_H
#include "cseries.h"
void mcc_network_new_game(void);
void mcc_network_host_tick(void);
void mcc_network_client_tick(void);
word mcc_network_inventory_entry_size(void);
void mcc_network_handle_inventories(void const *entries, short count);
#endif
