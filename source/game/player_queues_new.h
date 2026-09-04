/*
PLAYER_QUEUES_NEW.H
*/

#ifndef __PLAYER_QUEUES_NEW_H
#define __PLAYER_QUEUES_NEW_H
#pragma once

/* ---------- structures */

struct player_action_collection;
struct player_action;
struct server_update;

/* ---------- public code */

void update_client_add_player(
	long player_index);
void update_server_add_player(
	long player_index);
void update_client_build_client_update(
	struct player_action_collection *action_collection);
void update_client_handle_server_update(
	struct server_update *update,
	unsigned long update_number);
void update_server_handle_client_update(
	long machine_index,
	struct player_action *actions);
void update_server_next_update(
	void);
void update_server_build_server_update(
	long machine_index,
	struct server_update *update,
	long *update_number);

#endif /* __PLAYER_QUEUES_NEW_H */
