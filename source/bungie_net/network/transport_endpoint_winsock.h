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
struct transport_endpoint_set;
struct transport_address;

/* ---------- prototypes/TRANSPORT_ENDPOINT_WINSOCK.C */

struct transport_endpoint *create_transport_endpoint(
	long type);
struct transport_endpoint_set *create_endpoint_set(
	long maximum_endpoints);
long read_endpoint(
	struct transport_endpoint *endpoint,
	void *buffer,
	long size);
long read_from_endpoint(
	struct transport_endpoint *endpoint,
	void *buffer,
	long size,
	struct transport_address *source_address);
long write_endpoint(
	struct transport_endpoint *endpoint,
	void const *buffer,
	long size);
boolean endpoint_readable(
	struct transport_endpoint *endpoint,
	long timeout);
long endpoint_connected(
	struct transport_endpoint const *endpoint);
short get_endpoint_address(
	struct transport_endpoint *endpoint,
	struct transport_address *address);
short connect_endpoint(
	struct transport_endpoint *endpoint,
	struct transport_address const *address);
short connect_endpoint_async(
	struct transport_endpoint *endpoint,
	struct transport_address const *address,
	void *process_reference);
short set_endpoint_blocking(
	struct transport_endpoint *endpoint,
	boolean blocking);
short bind_endpoint(
	struct transport_endpoint *endpoint,
	struct transport_address *address);
short listen_endpoint(
	struct transport_endpoint *endpoint);
struct transport_endpoint *accept_endpoint(
	struct transport_endpoint *listening_endpoint);
short reject_endpoint(
	struct transport_endpoint *listening_endpoint);
void disconnect_endpoint(
	struct transport_endpoint *endpoint);
short poll_endpoint_set(
	struct transport_endpoint_set *set,
	long timeout);
void rewind_endpoint_set(
	struct transport_endpoint_set *set);
struct transport_endpoint *get_next_endpoint_from_set(
	struct transport_endpoint_set *set);
long count_endpoints_in_set(
	struct transport_endpoint_set *set);
short add_endpoint_to_set(
	struct transport_endpoint *endpoint,
	struct transport_endpoint_set *set);
short remove_endpoint_from_set(
	struct transport_endpoint *endpoint,
	struct transport_endpoint_set *set);
void delete_endpoint_set(
	struct transport_endpoint_set *set);
void delete_transport_endpoint(
	struct transport_endpoint *endpoint);

/* ---------- globals */

/* ---------- public code */

#endif // __TRANSPORT_ENDPOINT_WINSOCK_H
