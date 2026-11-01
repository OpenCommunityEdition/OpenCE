/* Host bot controls, adapted from thelinkin3000/halo-ce-universal
d92cc933's add/drop commands, built on BirchWoodGod's OpenCE bots.
Included by network_server_manager.c because the server record and its
join/quit dispatch are private there. Clients use ordinary player messages. */
static long bots_reuse_after[BOTS_MAXIMUM];

void bots_management_reset(void)
{
	csmemset(bots_reuse_after, 0, sizeof(bots_reuse_after));
}

static boolean bots_lobby_slots_available(struct network_game_server *server)
{
	long index;
	/* Include connected machines that have not admitted a player yet. */
	for (index = 0; index < MAXIMUM_NETWORK_MACHINE_COUNT; index++)
		if (bots_machine_is_bot(server->client_machines[index].machine_index))
			return FALSE;
	for (index = 0; index < MAXIMUM_NETWORK_PLAYER_COUNT; index++)
	{
		struct network_player *player = &server->game.players[index];
		if (network_player_is_valid(player) && bots_machine_is_bot(player->machine_index) &&
			!bots_player_owned(player->machine_index))
			return FALSE;
	}
	return TRUE;
}

int bots_lobby_settings_apply(char *status, long size)
{
	struct network_game_server *server = global_network_game_server_get();
	int enabled = config_boolean("bots.network");

	/* Network policy saved outside pregame remains for the next host. Never change the
	bot policy after peers have been told to load, or reset live ownership. */
	if (!server || server->state != _network_game_server_state_pregame || server->sent_start_game_message)
		return TRUE;
	if (!network_game_is_splitscreen_local())
	{
		if (enabled && !bots_network_reserved() && !bots_lobby_slots_available(server))
		{
			snprintf(status, size, "A player connection holds a bot slot. Re-host with Network Bots enabled.");
			return FALSE;
		}
		bots_network_lobby_enable(enabled);
	}
	network_game_server_port_bots(server);
	return TRUE;
}

int bots_manage(int add, int team, char *status, long size)
{
	struct network_game_server *server = global_network_game_server_get();
	long index, slot, chosen = NONE;
	long teams[2] = { 0, 0 };
	boolean occupied[BOTS_MAXIMUM] = { FALSE };
	struct network_player player;

	if (!server)
	{
		snprintf(status, size, "Only the host can manage bots.");
		return FALSE;
	}
	if (server->state != _network_game_server_state_ingame)
	{
		snprintf(status, size, "During a match, add or remove bots here. In the lobby, set Bot Count.");
		return FALSE;
	}
	if (!bots_session_allowed())
	{
		snprintf(status, size, "Enable Network Bots in the lobby before starting the match.");
		return FALSE;
	}
	if (!bots_gametype_active(server->game.variant.game_engine_index))
	{
		snprintf(status, size, "Bots support Slayer, plus CTF/King with Enhanced Bots enabled before loading.");
		return FALSE;
	}
	if (team < NONE || team > 1 || (team != NONE && !game_engine_has_teams()))
	{
		snprintf(status, size, "Choose Any Team in a game without teams.");
		return FALSE;
	}
	for (slot = 0; slot < MAXIMUM_NETWORK_PLAYER_COUNT; slot++)
	{
		struct network_player *entry = &server->game.players[slot];
		if (!network_player_is_valid(entry)) continue;
		if (entry->team_index >= 0 && entry->team_index < 2) teams[entry->team_index]++;
		if (bots_machine_is_bot(entry->machine_index))
			occupied[entry->machine_index - BOTS_FIRST_MACHINE] = TRUE;
		if (bots_player_owned(entry->machine_index) && (team == NONE || entry->team_index == team) &&
			(chosen == NONE || entry->machine_index > server->game.players[chosen].machine_index))
			chosen = slot;
	}
	if (add)
	{
		void *message;
		for (index = 0; index < BOTS_MAXIMUM; index++)
			if (!occupied[index] && game_time_get() >= bots_reuse_after[index]) break;
		if (index == BOTS_MAXIMUM || server->game.player_count >= server->game.maximum_players)
		{
			snprintf(status, size, "No free bot slot. A departing bot needs a moment to leave.");
			return FALSE;
		}
		bots_network_player(index, &player);
		player.team_index = (char)(team != NONE ? team : game_engine_has_teams() && teams[1] < teams[0] ? 1 : 0);
		get_unique_random_color(server, &player);
		if (!network_game_add_player(&server->game, &player))
		{
			snprintf(status, size, "The game is full.");
			return FALSE;
		}
		/* Admission chooses the authoritative player slot. Encode that slot,
		then publish; failed encoding can still undo this unannounced record. */
		message = create_network_game_message(_message_server_add_player_ingame, &player, sizeof(player));
		if (!message)
		{
			network_game_remove_player(&server->game, &player);
			snprintf(status, size, "Could not prepare the bot's join.");
			return FALSE;
		}
		bots_player_ownership(player.machine_index, TRUE);
		if (!network_game_server_send_message_to_all_machines(server, message))
			network_event("bots: could not send a player's join to every machine");
		snprintf(status, size, "Bot %ld added.", index + 1);
	}
	else
	{
		struct message_server_remove_player_ingame removal;
		void *message;
		if (chosen == NONE)
		{
			snprintf(status, size, "No bot on the selected team.");
			return FALSE;
		}
		player = server->game.players[chosen];
		removal.player = player;
		removal.reason = game_time_get() + NETWORK_GAME_PLAYER_QUIT_DELAY;
		message = create_network_game_message(_message_server_remove_player_ingame, &removal, sizeof(removal));
		if (!message)
		{
			snprintf(status, size, "Could not prepare the bot's departure.");
			return FALSE;
		}
		if (!network_game_remove_player(&server->game, &player))
		{
			snprintf(status, size, "The bot could not leave.");
			return FALSE;
		}
		index = player.machine_index - BOTS_FIRST_MACHINE;
		/* Existing quit messages take effect at a future tick. Do not assign
		that identity to a replacement while peers still have its old player. */
		bots_reuse_after[index] = removal.reason + 1;
		bots_player_ownership(player.machine_index, FALSE);
		if (!network_game_server_send_message_to_all_machines(server, message))
			network_event("bots: could not send a player's departure to every machine");
		snprintf(status, size, "Bot %ld removed.", index + 1);
	}
	network_event("bots: %s", status);
	return TRUE;
}
