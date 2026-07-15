/*
TRANSPORT.H

header included in hcex build.
*/

#ifndef __TRANSPORT_H
#define __TRANSPORT_H
#pragma once

/* ---------- constants */

enum
{
	IPV4_ADDRESS_LENGTH = 4,
	MAXIMUM_TRANSPORT_ADDRESS_LENGTH = 16,
};

/* ---------- macros */

/* ---------- structures */

struct transport_address_data
{
	unsigned long words[MAXIMUM_TRANSPORT_ADDRESS_LENGTH / sizeof(unsigned long)];
};

struct transport_address
{
	struct transport_address_data address;
	word address_length;
	word port;
	long address_type;
};

/* ---------- prototypes/EXAMPLE.C */

struct transport_address *create_transport_address(
	struct transport_address_data const *address,
	word address_length,
	word port);
void delete_transport_address(
	struct transport_address *address);
long transport_address_equivalent(
	struct transport_address const *a,
	struct transport_address const *b);

/* ---------- globals */

extern boolean transport_initialized;

/* ---------- public code */

#endif // __TRANSPORT_H
