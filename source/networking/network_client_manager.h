/*
NETWORK_CLIENT_MANAGER.H

header included in hcex build.
*/

#ifndef __NETWORK_CLIENT_MANAGER_H
#define __NETWORK_CLIENT_MANAGER_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct network_game_client;
struct network_player;
struct transport_address;
struct message_server_game_advertise;
struct message_server_machine_accepted;
struct message_server_game_settings_update;
struct message_server_game_update;

/* ---------- prototypes/NETWORK_CLIENT_MANAGER.C */

boolean transport_network_available(
	void);
short network_game_client_get_state(
	struct network_game_client *client,
	short *state_data);
short network_game_client_get_seconds_to_game_start(
	struct network_game_client *client);
boolean network_game_client_add_player(
	struct network_game_client *client,
	short controller_index);
boolean network_game_client_address_matches_server(
	struct network_game_client *client,
	struct transport_address *source_address);
void network_game_client_new_advertised_game(
	struct network_game_client *client,
	struct message_server_game_advertise *advertisement);
void network_game_client_ponged(
	struct network_game_client *client,
	struct transport_address *source_address,
	long timestamp);
void network_game_client_accepted_into_game(
	struct network_game_client *client,
	struct transport_address *source_address,
	struct message_server_machine_accepted *acceptance);
void network_game_client_rejected_by_game(
	struct network_game_client *client,
	struct transport_address *source_address,
	long reason);
boolean network_game_client_game_settings_updated(
	struct network_game_client *client,
	struct message_server_game_settings_update *game_settings);
void network_game_client_countdown_timer_update(
	struct network_game_client *client,
	long seconds_remaining);
boolean network_game_client_game_has_started(
	struct network_game_client *client);
void network_game_client_game_shutdown(
	struct network_game_client *client);
boolean network_game_client_handle_game_update(
	struct network_game_client *client,
	struct message_server_game_update *game_update);
void network_game_client_game_out_of_sync(
	struct network_game_client *client);
boolean network_game_client_add_player_to_game(
	struct network_game_client *client,
	struct network_player *player);
boolean network_game_client_remove_player(
	struct network_game_client *client,
	struct network_player *player,
	long reason);
void network_game_client_switch_to_postgame(
	struct network_game_client *client);
boolean network_game_client_switch_to_pregame(
	struct network_game_client *client);

/* ---------- globals */

/* ---------- public code */

#endif // __NETWORK_CLIENT_MANAGER_H
