/* CE pause-menu choices belong to the server that accepted them. A draft
map choice must never affect a normal postgame or a Restart command. The
profile is read before ending the game, so applying a confirmed request
cannot fail halfway through because a saved profile was removed. */
static struct
{
	struct network_game_server *server;
	char draft[256];
	char map[256];
	char variant_directory[256];
	struct game_variant variant;
	struct game_variant_options options;
	boolean confirmed;
	boolean restart;
} ce_new_game;

boolean custom_edition_menu_widget(struct widget_instance const *widget)
{
	return widget && custom_edition_cache_tags_loaded() && !mcc_cache_tags_loaded() &&
		!pc_menu_tag(widget->definition_tag_index);
}

void custom_edition_menu_cancel_draft(void)
{
	ce_new_game.draft[0] = 0;
}

void custom_edition_menu_server_dispose(struct network_game_server *server)
{
	if (ce_new_game.server == server)
		memset(&ce_new_game, 0, sizeof(ce_new_game));
}

boolean pc_menu_map_stash(struct widget_instance *list)
{
	char const *const *names;
	short index;
	short count;

	custom_edition_menu_cancel_draft();
	if (!custom_edition_menu_widget(list) || !global_network_game_server_get())
		return FALSE;
	map_list_update(list);
	count = ui_widget_port_multiplayer_maps(&names, NULL);
	index = map_list.chosen;
	if (index < 0 || index >= count || strlen(names[index]) >= sizeof(ce_new_game.draft))
		return FALSE;
	ce_new_game.server = global_network_game_server_get();
	strcpy(ce_new_game.draft, names[index]);
	return TRUE;
}

boolean pc_menu_new_game_begin(struct widget_instance *list, boolean *widget_deleted)
{
	long profile;
	short choice;
	char build[0x20];

	if (!custom_edition_menu_widget(list) || !ce_new_game.draft[0] ||
		!ce_new_game.server || ce_new_game.server != global_network_game_server_get() ||
		!game_engine_running())
		return FALSE;
	gametype_list_update(list);
	choice = multiplayer.gametype_chosen;
	if (choice < 0 || choice >= multiplayer.bank_count ||
		multiplayer.bank[choice] < 0 || multiplayer.bank[choice] >= multiplayer.gametype_count)
		return FALSE;
	profile = multiplayer.gametypes[multiplayer.bank[choice]];
	if (profile == NONE || !(profile & 0x80000000) ||
		!playlist_profile_get(profile, &ce_new_game.variant) ||
		!playlist_profile_get_options(profile, &ce_new_game.options))
		return FALSE;
	if (!network_game_is_splitscreen_local() && !cache_files_map_plays_multiplayer(ce_new_game.draft, build))
	{
		cache_files_show_multiplayer_unavailable(ce_new_game.draft, build);
		return FALSE;
	}
	ce_new_game.variant_directory[0] = 0;
	saved_game_file_get_path_to_enclosing_directory(profile, ce_new_game.variant_directory);
	strcpy(ce_new_game.map, ce_new_game.draft);
	ce_new_game.confirmed = TRUE;
	ce_new_game.restart = FALSE;
	custom_edition_menu_cancel_draft();
	game_engine_end_game();
	return TRUE;
}

boolean pc_menu_restart_game(boolean *widget_deleted)
{
	struct network_game_server *server = global_network_game_server_get();

	if (!custom_edition_cache_tags_loaded() || mcc_cache_tags_loaded() || !server || !game_engine_running())
		return FALSE;
	memset(&ce_new_game, 0, sizeof(ce_new_game));
	ce_new_game.server = server;
	ce_new_game.restart = TRUE;
	game_engine_end_game();
	return TRUE;
}

boolean pc_menu_new_game_apply(void)
{
	char const *const *names;
	short count;
	short index;
	boolean applied = FALSE;

	if (ce_new_game.server && ce_new_game.server == global_network_game_server_get())
	{
		if (ce_new_game.restart)
			applied = TRUE;
		else if (ce_new_game.confirmed)
		{
			count = ui_widget_port_multiplayer_maps(&names, NULL);
			for (index = 0; index < count; index++)
			{
				if (!_stricmp(names[index], ce_new_game.map))
				{
					/* Map validation happens before its first mutation. Everything
					after it uses the already-read variant, with no further I/O. */
					if (ui_widget_port_multiplayer_map_choose(index))
					{
						ui_widget_port_game_variant_set(&ce_new_game.variant, &ce_new_game.options);
						if (ce_new_game.variant_directory[0])
							saved_game_file_remember_last_used_multiplayer_variant_directory(ce_new_game.variant_directory);
						applied = TRUE;
					}
					break;
				}
			}
		}
	}
	memset(&ce_new_game, 0, sizeof(ce_new_game));
	return applied;
}
