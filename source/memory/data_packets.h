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

/* ---------- globals */

/* ---------- public code */

#endif // __DATA_PACKETS_H
