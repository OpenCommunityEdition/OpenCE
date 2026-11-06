/* Host population policy. Included beside bots_management.inl because the
join queue and player message dispatch belong to the private server record.
The wire still carries ordinary player additions and departures. */
static unsigned long bots_population_wait_started;
static long bots_population_wait_machine = NONE;
static long bots_population_wait_controller = NONE;
static long bots_population_retry_tick;

static void bots_population_map_reset(void)
{
	bots_population_wait_machine = bots_population_wait_controller = NONE;
	bots_population_wait_started = 0;
	bots_population_retry_tick = 0;
}

static boolean bots_population_enabled(struct network_game_server *server)
{
	return server && !network_game_is_splitscreen_local() && bots_session_allowed() &&
		(server->state == _network_game_server_state_ingame ?
			bots_gametype_active(server->game.variant.game_engine_index) :
			bots_gametype_allowed(server->game.variant.game_engine_index));
}

static long bots_population_count(struct network_game *game)
{
	return game->player_count - bots_human_count(game);
}

int bots_population_can_join(struct network_game_server *server)
{
	return bots_population_enabled(server) && bots_population_count(&server->game) > 0 &&
		bots_human_count(&server->game) < server->game.maximum_players;
}

/* Only a validated, new player can displace a bot. Repeated requests, malformed
controllers and connections that have not joined never reserve population. */
static boolean bots_population_new_player(struct network_game_server *server, struct network_player *player)
{
	long index;
	if (!network_player_is_valid(player) || bots_player_owned(player->machine_index) ||
		!network_game_server_client_machine_is_joined_to_game(server,
			&server->client_machines[player->machine_index]))
		return FALSE;
	for (index = 0; index < MAXIMUM_NETWORK_PLAYER_COUNT; index++)
		if (network_player_is_valid(&server->game.players[index]) &&
			server->game.players[index].machine_index == player->machine_index &&
			server->game.players[index].controller_index == player->controller_index)
			return FALSE;
	return TRUE;
}

static int bots_population_team(struct network_game_server *server, boolean add)
{
	long index, teams[2] = { 0, 0 };
	if (!game_engine_has_teams()) return NONE;
	for (index = 0; index < MAXIMUM_NETWORK_PLAYER_COUNT; index++)
	{
		struct network_player *player = &server->game.players[index];
		if (!network_player_is_valid(player) || !VALID_INDEX(player->team_index, 2)) continue;
		if (bots_teams() == _bots_teams_against && !bots_player_owned(player->machine_index))
			return 1 - player->team_index;
		teams[player->team_index]++;
	}
	return add ? (teams[1] < teams[0] ? 1 : 0) : (teams[1] > teams[0] ? 1 : 0);
}

static boolean bots_population_remove(struct network_game_server *server)
{
	long index;
	char status[180];
	if (server->state == _network_game_server_state_ingame)
	{
		/* Prefer the larger team, but still yield if its players are all human. */
		if (bots_manage_change(FALSE, bots_population_team(server, FALSE), status, sizeof(status)))
			return TRUE;
		return bots_manage_change(FALSE, NONE, status, sizeof(status));
	}
	for (index = MAXIMUM_NETWORK_PLAYER_COUNT - 1; index >= 0; index--)
	{
		struct network_player player = server->game.players[index];
		if (network_player_is_valid(&player) && bots_player_owned(player.machine_index) &&
			network_game_remove_player(&server->game, &player))
		{
			bots_player_ownership(player.machine_index, FALSE);
			network_game_server_send_game_data_pregame(server);
			return TRUE;
		}
	}
	return FALSE;
}

/* Pregame additions are synchronous; a full lobby can release a bot before
the existing allocator runs. Its next fill observes the admitted human. */
static void bots_population_pregame_join(struct network_game_server *server, struct network_player *player)
{
	if (server->state == _network_game_server_state_pregame && bots_population_enabled(server) &&
		bots_population_new_player(server, player) &&
		bots_population_count(&server->game) > bots_population_after_join(&server->game))
		bots_population_remove(server);
}

/* A live departure is scheduled at a future tick and its unit may still hold
the datum. Keep this real join queued, for at most ten seconds, until an
ordinary player slot is available. The existing queue handles split screen
and concurrent requests serially; a cancelled machine is checked each time. */
static boolean bots_population_wait_for_slot(struct network_game_server *server, struct network_player *player)
{
	unsigned long now = system_milliseconds();
	boolean removed = FALSE;
	if (!bots_population_enabled(server) || !bots_population_new_player(server, player) ||
		!network_game_server_machine_may_add_player_ingame(server, player->machine_index))
		return FALSE;
	if (bots_population_wait_machine != player->machine_index ||
		bots_population_wait_controller != player->controller_index)
	{
		bots_population_wait_machine = player->machine_index;
		bots_population_wait_controller = player->controller_index;
		bots_population_wait_started = now;
	}
	if (now - bots_population_wait_started >= 10000UL) return FALSE;
	if (bots_population_count(&server->game) > bots_population_after_join(&server->game))
		removed = bots_population_remove(server);
	if (network_game_has_free_player_slot(&server->game)) return FALSE;
	/* Do not delay an ordinary full-human refusal. A prior bot departure can
still hold its slot even after ownership has been released. */
	return (removed || server->game.player_count < server->game.maximum_players) &&
		bots_human_count(&server->game) < server->game.maximum_players;
}

static void bots_population_update(struct network_game_server *server)
{
	long wanted, count, tick;
	char status[180];
	if (server->state != _network_game_server_state_ingame || !bots_population_enabled(server)) return;
	/* Pending requests take priority over restoration. Map downloaders remain
in the separate held list and do not displace bots until ready to enter. */
	if (server->queued_player_valid || server->waiting_player_count > 0) return;
	bots_population_wait_machine = bots_population_wait_controller = NONE;
	tick = game_time_get();
	if (tick < bots_population_retry_tick) return;
	bots_population_retry_tick = tick + TICKS_PER_SECOND;
	wanted = bots_population_wanted(&server->game);
	count = bots_population_count(&server->game);
	if (count > wanted)
		bots_population_remove(server);
	else if (count < wanted && network_game_has_free_player_slot(&server->game))
		bots_manage_change(TRUE, bots_population_team(server, TRUE), status, sizeof(status));
}
