/*
NETWORK_MESSAGES.H

header included in hcex build.
*/

#ifndef __NETWORK_MESSAGES_H
#define __NETWORK_MESSAGES_H
#pragma once

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void initialize_network_game_packets(
	void);

void network_event(
	char *format,
	...);

boolean decode_network_game_message(
	void *message_struct,
	const void *encoded_message,
	short *encoded_message_size,
	short *packet_type,
	short *packet_version,
	long expected_packet_class);

#endif // __NETWORK_MESSAGES_H
