/*
MESSAGE_HEADER.H

header included in hcex build.
*/

#ifndef __MESSAGE_HEADER_H
#define __MESSAGE_HEADER_H
#pragma once

/* ---------- constants */

enum
{
	MAXIMUM_MESSAGE_SIZE = 0xFFF,
	NUMBER_OF_MESSAGE_TYPES = 4,
	MESSAGE_FLAG_BITS_MASK = 3,
};

enum message_header_byte_order
{
	_message_header_byte_order_network = 0,
	_message_header_byte_order_host,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/MESSAGE_HEADER.C */

void build_message_header(word *msg, word length, byte type, byte flags);
void byte_swap_message_header(word *header, enum message_header_byte_order byte_order);

/* ---------- globals */

/* ---------- public code */

#endif // __MESSAGE_HEADER_H
