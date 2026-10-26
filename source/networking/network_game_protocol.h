/*
NETWORK_GAME_PROTOCOL.H

Shared game-protocol constants for the connection and message consumers.
*/

#ifndef __NETWORK_GAME_PROTOCOL_H
#define __NETWORK_GAME_PROTOCOL_H
#pragma once

/* ---------- constants */

enum
{
	NETWORK_GAME_SERVER_PORT = 0x141E,
	NETWORK_GAME_CLIENT_PORT = 0x141F,
};

enum
{
	_rejection_code_version_too_old = 0,
	_rejection_code_version_too_new,
	_rejection_code_bad_join_token,
	_rejection_code_bad_password,
	_rejection_code_game_is_full,
	_rejection_code_game_is_closed,
	_rejection_code_blacklisted_machine,
	/* port: a player's name a dedicated server's word filter keeps out
	(server.chat_filter "block"); a build before it says the game is closed */
	_rejection_code_name_refused,
	NUMBER_OF_SERVER_REJECTION_CODES,
};

#endif // __NETWORK_GAME_PROTOCOL_H
