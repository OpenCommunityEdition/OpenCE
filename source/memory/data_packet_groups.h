/*
DATA_PACKET_GROUPS.H
*/

#ifndef __DATA_PACKET_GROUPS_H
#define __DATA_PACKET_GROUPS_H
#pragma once

/* ---------- structures */

struct data_packet_group_definition
{
	unsigned char opaque[0x14];
};

/* ---------- prototypes/DATA_PACKET_GROUPS.C */

void data_packet_group_initialize(
	struct data_packet_group_definition *group_definition);

#endif // __DATA_PACKET_GROUPS_H
