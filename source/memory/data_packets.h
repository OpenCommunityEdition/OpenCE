/*
DATA_PACKETS.H

header included in hcex build.
*/

#ifndef __DATA_PACKETS_H
#define __DATA_PACKETS_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct data_packet_definition;

/* ---------- prototypes/DATA_PACKETS.C */

void data_packet_verify(
	struct data_packet_definition *packet_definition);

boolean data_packet_encode(
	struct data_packet_definition *packet_definition,
	long packet_version,
	void const *decoded_packet,
	void *encoded_packet,
	short *encoded_packet_size,
	short maximum_buffer_size);

/* ---------- globals */

/* ---------- public code */

#endif // __DATA_PACKETS_H
