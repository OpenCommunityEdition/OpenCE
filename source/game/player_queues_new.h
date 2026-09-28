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

boolean update_server_new(
	void);
void update_server_delete(
	void);
void update_server_start(
	void);
void update_server_add_player(
	long player_index);
void update_server_next_update(
	void);
void update_server_build_server_update(
	long machine_index,
	struct server_update *update,
	long *update_number);
void update_server_handle_client_update(
	long machine_index,
	struct player_action *actions);

boolean update_client_new(
	void);
void update_client_delete(
	void);
void update_client_start(
	void);
void update_client_add_player(
	long player_index);
void update_client_queue(
	struct player_action const *action);
void update_client_queue_push(
	void);
boolean update_client_dequeue(
	struct player_action *actions);
long update_client_get_maximum_actions(
	void);
long update_client_get_maximum_possible_server_time(
	void);
void update_client_local_ticks(
	short ticks);
void update_client_build_client_update(
	struct player_action_collection *action_collection);
void update_client_handle_server_update(
	struct server_update *update,
	long update_number);

void update_queues_reset_and_fill_with_lies(
	void);

#ifdef HALO_LINUX
/* the distributed netcode's inputs (port/linux/game/network_distributed.c):
each tick's buttons are sent again with the ticks after it, and taken once,
from whichever message brings them first. control_flags holds the buttons
of the tick and of the ones before it, newest first. */
enum
{
	/* the ticks of buttons in each message */
	DISTRIBUTED_INPUT_HISTORY = 4,
};

/* (the host) a client machine's player's action at the client's tick */
void update_server_handle_distributed_input(
	long player_index,
	long tick,
	struct player_action const *action,
	unsigned short const *control_flags,
	short count);
/* (the host) the update its last tick ran (NONE for none), and an update's
actions (NULL once it is gone) */
long update_server_ticked_update_number(
	void);
struct player_action const *update_server_update_actions(
	long update_number,
	short *count);
/* (a client) the host's action for the player at that absolute index, of
the host's update */
void update_client_handle_relayed_action(
	short player_index,
	long update_number,
	struct player_action const *action,
	unsigned short const *control_flags,
	short count);
/* (a client) a local player's last tick, its action and the buttons of the
ticks up to it; FALSE before its first */
boolean update_client_distributed_input(
	short local_player_index,
	long *tick,
	struct player_action *action,
	unsigned short *control_flags);
#endif
long player_new_queue(
	long player_index);

#endif /* __PLAYER_QUEUES_NEW_H */
