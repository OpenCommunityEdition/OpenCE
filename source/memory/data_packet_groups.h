/*
DATA_PACKET_GROUPS.H
*/

#ifndef __DATA_PACKET_GROUPS_H
#define __DATA_PACKET_GROUPS_H
#pragma once

/* ---------- headers */

#include "cseries.h"

/* ---------- structures */

struct data_packet_group_definition
{
	unsigned char opaque[0x14];
};

/* ---------- prototypes/DATA_PACKET_GROUPS.C */

void data_packet_group_initialize(
	struct data_packet_group_definition *group_definition);

boolean data_packet_group_decode_packet(
	struct data_packet_group_definition *group_definition,
	void *decoded_packet,
	const void *encoded_packet,
	short *encoded_packet_size,
	short *packet_type,
	short *packet_version,
	long expected_packet_class);

#endif // __DATA_PACKET_GROUPS_H
