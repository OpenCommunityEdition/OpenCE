//! The names of the functions a widget's handlers run and of the functions
//! that give it its data, as the game's widget code names them
//! (source/interface/ui_widget.c), by index.

/// The functions an event handler runs, by index. Indices 0, 3 and 4 do
/// nothing and succeed; an index past the last fails.
pub const FUNCTION_NAMES: [&str; 102] = [
    "NULL",
    "list goto next item",
    "list goto previous item",
    "unused",
    "unused",
    "initialize sp level list solo",
    "initialize sp level list coop",
    "dispose sp level list",
    "solo level set map",
    "set difficulty",
    "start new game",
    "pause game restart at checkpoint",
    "pause game restart level",
    "pause game return to main menu",
    "clear multiplayer player joins",
    "join controller to mp game",
    "initialize net game server list",
    "start network game server",
    "dispose net game server list",
    "shutdown network game",
    "net game join from server list",
    "split screen game initialize",
    "coop game initialize",
    "main menu intialize",
    "mp type menu initialize",
    "pick play stage for quick start",
    "mp level list initialize",
    "mp level list dispose",
    "mp level select",
    "mp profiles list initialize",
    "mp profiles list dispose",
    "mp profile set for game",
    "swap player team",
    "net game join player",
    "player profile list initialize",
    "player profile list dispose",
    "3wide plyr prof set for game",
    "1wide plyr prof set for game",
    "mp profile begin editing",
    "mp profile end editing",
    "mp profile set game engine",
    "mp profile change name",
    "mp profile set ctf rules",
    "mp profile set koth rules",
    "mp profile set slayer rules",
    "mp profile set oddball rules",
    "mp profile set racing rules",
    "mp profile set player options",
    "mp profile set item options",
    "mp profile set indicator opts",
    "mp profile init game engine",
    "mp profile init name",
    "mp profile init ctf rules",
    "mp profile init koth rules",
    "mp profile init slayer rules",
    "mp profile init oddball rules",
    "mp profile init racing rules",
    "mp profile init player opts",
    "mp profile init item options",
    "mp profile init indicator opts",
    "mp profile save changes",
    "color picker menu initialize",
    "color picker menu dispose",
    "color picker select color",
    "player profile begin editing",
    "player profile end editing",
    "player profile change name",
    "player profile save changes",
    "plyr prf init cntl settings",
    "plyr prf init adv cntl set",
    "plyr prf save cntl settings",
    "plyr prf save adv cntl set",
    "mp game player quit",
    "main menu switch to solo game",
    "request del player profile",
    "request del playlist profile",
    "final del player profile",
    "final del playlist profile",
    "cancel profile delete",
    "create&edit playlist profile",
    "create&edit player profile",
    "net game speed start",
    "net game delay start",
    "net server accept conx",
    "net server defer start",
    "net server allow start",
    "disable if no xdemos",
    "run xdemos",
    "sp reset controller choices",
    "sp set p1 controller choice",
    "sp set p2 controller choice",
    "error if no network connection",
    "start server if none advertised",
    "net game unjoin player",
    "close if not editing profile",
    "exit to xbox dashboard",
    "new campaign chosen",
    "new campaign decision",
    "pop history stack once",
    "difficulty menu init",
    "begin music fade out",
    "new game if no plyr profiles",
];

/// The functions that give a widget its data each frame, before it is
/// drawn, by index.
pub const DATA_INPUT_NAMES: [&str; 41] = [
    "null",
    "settings_menu_update_extended_description",
    "null",
    "playlist_settings_menu_update_extended_description",
    "playlist_gametype_select_menu_update_extended_description",
    "multiplayer_type_menu_update_extended_description",
    "solo_level_select_list_update_displayed_items",
    "difficulty_select_menu_update_extended_description",
    "set_textbox_to_build_number",
    "server_list_menu_update",
    "network_pregame_status_screen_update",
    "splitscreen_pregame_status_screen_update",
    "netgame_prejoin_players",
    "mutliplayer_settings_select_list_update_displayed_items",
    "player_profile_3wide_list_update",
    "player_profile_edit_select_menu_update_extended_description",
    "player_profile_1wide_list_update",
    "game_options_menu_update_text_desc",
    "solo_game_objective_text",
    "player_profile_color_picker_update",
    "game_options_menu_update_pic_desc",
    "main_menu_animation_fakery",
    "mp_level_select_list_update_displayed_items",
    "get_active_player_profile_display_name",
    "get_editable_player_profile_display_name",
    "get_editable_playlist_profile_display_name",
    "get_active_player_profile_color_index",
    "multiplayer_game_set_text_box_for_map_name",
    "multiplayer_game_set_text_box_for_game_ruleset",
    "multiplayer_game_set_text_box_for_teams_noteams",
    "multiplayer_game_set_text_box_for_score_limit",
    "multiplayer_game_set_text_box_for_score_limit_type",
    "multiplayer_game_set_bitmap_for_map",
    "multiplayer_game_set_bitmap_for_ruleset",
    "multiplayer_game_set_text_box_for_number_of_players",
    "multiplayer_edit_profile_set_ruleset_textbox_string_index",
    "system_link_status_check",
    "multiplayer_game_directions",
    "teams_no_teams_mp_game_bitmap_update",
    "warn_if_difficulty_will_nuke_saved_game",
    "dim_if_no_system_link_cable",
];

/// The functions whose values replace strings in a widget's text, by index.
pub const REPLACEMENT_NAMES: [&str; 2] = ["null", "widget's controller"];

/// The function "list goto next item".
pub const LIST_GOTO_NEXT_ITEM: i16 = 1;
/// The function "list goto previous item".
pub const LIST_GOTO_PREVIOUS_ITEM: i16 = 2;

/// The name of the function `index`, or `None` past the last.
pub fn function_name(index: i16) -> Option<&'static str> {
    usize::try_from(index)
        .ok()
        .and_then(|index| FUNCTION_NAMES.get(index).copied())
}

/// The name of the data input `index`, or `None` past the last.
pub fn data_input_name(index: i16) -> Option<&'static str> {
    usize::try_from(index)
        .ok()
        .and_then(|index| DATA_INPUT_NAMES.get(index).copied())
}
