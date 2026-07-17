/*
NETWORK_CONNECTION.H

header included in hcex build.
*/

#ifndef __NETWORK_CONNECTION_H
#define __NETWORK_CONNECTION_H
#pragma once

/* ---------- constants */

enum
{
	MAXIMUM_NUMBER_OF_LOCAL_PLAYERS = 4,
};

/* ---------- macros */

/* ---------- structures */

struct network_connection;

/* ---------- prototypes/EXAMPLE.C */

void network_connection_initialize(
	void);
void network_connection_set_connection_rejection_procedure(
	struct network_connection *connection,
	void *connection_rejection_procedure);
boolean network_connection_active(
	struct network_connection *connection);
boolean network_connection_going_stale(
	struct network_connection *connection);
void network_connection_keep_alive(
	struct network_connection *connection);

/* ---------- globals */

/* ---------- public code */

#endif // __NETWORK_CONNECTION_H
