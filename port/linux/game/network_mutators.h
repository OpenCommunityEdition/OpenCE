/*
NETWORK_MUTATORS.H

The host's mutators sent to its clients (network_mutators.c).
*/

#ifndef NETWORK_MUTATORS_H
#define NETWORK_MUTATORS_H

#include "cseries.h"

/* (network_distributed.c) the host's to a machine ready for its game */
void network_mutators_send(long machine_index);
boolean network_mutators_handles_message(word const *message, word size);
void network_mutators_handle_message(long machine_index, word const *message, word size);

#endif
