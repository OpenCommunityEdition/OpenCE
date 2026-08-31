/*
TRANSPORT_ENDPOINT_WINSOCK.H

header included in hcex build.
*/

#ifndef __TRANSPORT_ENDPOINT_WINSOCK_H
#define __TRANSPORT_ENDPOINT_WINSOCK_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct transport_endpoint;

/* ---------- prototypes/TRANSPORT_ENDPOINT_WINSOCK.C */

long write_endpoint(
	struct transport_endpoint *endpoint,
	void const *buffer,
	long size);

/* ---------- globals */

/* ---------- public code */

#endif // __TRANSPORT_ENDPOINT_WINSOCK_H
