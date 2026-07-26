/*
GAME_ENGINE.C

symbols in this file:
00096780 0020:
	_game_globals_get_weapon (0000)
000967A0 0070:
	_game_engine_get_team_score (0000)
00096810 0020:
	_linear_to_non_linear_alpha (0000)
00096830 0020:
	_game_engine_dispose (0000)
00096850 0040:
	_code_00096850 (0000)
00096890 0120:
	_code_00096890 (0000)
000969B0 0020:
	_game_engine_dispose_from_old_map (0000)
000969D0 0020:
	_game_engine_game_ending (0000)
000969F0 0020:
	_game_engine_game_starting (0000)
00096A10 0040:
	_players_in_game (0000)
00096A50 0020:
	_game_engine_statistics_append (0000)
00096A70 0020:
	_game_engine_handle_client_message (0000)
00096A90 0020:
	_game_engine_handle_server_message (0000)
00096AB0 0030:
	_sort_statistic_buffer (0000)
00096AE0 0050:
	_sort_statistic_buffer_ranking (0000)
00096B30 0010:
	_code_00096b30 (0000)
00096B40 0010:
	_code_00096b40 (0000)
00096B50 0050:
	_code_00096b50 (0000)
00096BA0 0070:
	_code_00096ba0 (0000)
00096C10 0020:
	_code_00096c10 (0000)
00096C30 0050:
	_get_postgame_hilite_colors (0000)
00096C80 0060:
	_code_00096c80 (0000)
00096CE0 0020:
	_game_engine_post_rasterize_objects (0000)
00096D00 0030:
	_code_00096d00 (0000)
00096D30 0100:
	_code_00096d30 (0000)
00096E30 00a0:
	_code_00096e30 (0000)
00096ED0 0150:
	_code_00096ed0 (0000)
00097020 00b0:
	_code_00097020 (0000)
000970D0 0050:
	_game_engine_load_stage (0000)
00097120 0030:
	_game_engine_playlist_begin (0000)
00097150 0060:
	_game_engine_get_current_stage (0000)
000971B0 0030:
	_game_engine_end_game (0000)
000971E0 0020:
	_game_engine_allow_pick_up (0000)
00097200 0050:
	_game_engine_player_damaged_player (0000)
00097250 00a0:
	_code_00097250 (0000)
000972F0 0040:
	_game_engine_player_is_out_of_lives (0000)
00097330 0100:
	_game_engine_should_spawn_player (0000)
00097430 0070:
	_game_engine_player_get_team_index (0000)
000974A0 0040:
	_game_engine_prespawn_player_update (0000)
000974E0 0010:
	_game_engine_running (0000)
000974F0 0020:
	_game_engine_can_score (0000)
00097510 0020:
	_game_engine_force_single_screen (0000)
00097530 0030:
	_get_blink_alpha (0000)
00097560 0010:
	_code_00097560 (0000)
00097570 00f0:
	_code_00097570 (0000)
00097660 0100:
	_game_engine_rasterize_message (0000)
00097760 00e0:
	_game_engine_picking_up (0000)
00097840 0080:
	_code_00097840 (0000)
000978C0 00a0:
	_game_engine_player_get_custom_motion_sensor_positions (0000)
00097960 0030:
	_get_flag_definition_index (0000)
00097990 0030:
	_get_ball_definition_index (0000)
000979C0 0040:
	_game_engine_switch_to_postgame (0000)
00097A00 0010:
	_game_engine_get_variant (0000)
00097A10 0020:
	_game_engine_get_goal_in_use (0000)
00097A30 0060:
	_game_engine_get_goal_position (0000)
00097A90 0080:
	_game_engine_set_goal_position (0000)
00097B10 0020:
	_game_engine_clear_goal_position (0000)
00097B30 00d0:
	_game_engine_render_nav_points (0000)
00097C00 0020:
	_code_00097c00 (0000)
00097C20 0030:
	_game_engine_infinite_grenades (0000)
00097C50 0020:
	_game_engine_has_teams (0000)
00097C70 0030:
	_game_engine_display_team_indicators (0000)
00097CA0 0030:
	_game_engine_has_shield (0000)
00097CD0 0060:
	_weapon_definition_index_to_list_index (0000)
00097D30 0030:
	_list_index_to_weapon_definition_index (0000)
00097D60 00c0:
	_game_engine_remap_vehicle (0000)
00097E20 0190:
	_game_engine_remap_weapon (0000)
00097FB0 0070:
	_game_engine_man_out (0000)
00098020 0030:
	_game_engine_state_message (0000)
00098050 0090:
	_game_engine_player_get_change_color (0000)
000980E0 0070:
	_game_engine_player_has_flag (0000)
00098150 0050:
	_game_engine_player_depower_active_camo (0000)
000981A0 00e0:
	_get_place_name (0000)
00098280 0170:
	_game_engine_get_place (0000)
000983F0 0040:
	_game_engine_hud_draw_motion_sensor (0000)
00098430 0020:
	_game_engine_test_flag (0000)
00098450 0020:
	_game_engine_test_trait (0000)
00098470 00a0:
	_code_00098470 (0000)
00098510 0070:
	_code_00098510 (0000)
00098580 0070:
	_game_engine_playlist_next (0000)
000985F0 0090:
	_build_game_variant_slayer (0000)
00098680 0090:
	_build_game_variant_slayer_pro (0000)
00098710 0090:
	_build_game_variant_elimination (0000)
000987A0 0090:
	_build_game_variant_phantoms (0000)
00098830 0090:
	_build_game_variant_endurance (0000)
000988C0 0090:
	_build_game_variant_rockets (0000)
00098950 0090:
	_build_game_variant_snipers (0000)
000989E0 0090:
	_build_game_variant_team_slayer (0000)
00098A70 0090:
	_build_game_variant_oddball (0000)
00098B00 0090:
	_build_game_variant_team_oddball (0000)
00098B90 0090:
	_build_game_variant_reverse_tag (0000)
00098C20 00a0:
	_build_game_variant_accumulation (0000)
00098CC0 00a0:
	_build_game_variant_juggernaut (0000)
00098D60 00a0:
	_build_game_variant_stalker (0000)
00098E00 0080:
	_build_game_variant_king (0000)
00098E80 0090:
	_build_game_variant_king_pro (0000)
00098F10 0080:
	_build_game_variant_crazy_king (0000)
00098F90 0090:
	_build_game_variant_team_king (0000)
00099020 0090:
	_build_game_variant_ctf (0000)
000990B0 0090:
	_build_game_variant_ctf_pro (0000)
00099140 0090:
	_build_game_variant_invasion (0000)
000991D0 0090:
	_build_game_variant_iron_ctf (0000)
00099260 0090:
	_build_game_variant_race (0000)
000992F0 0090:
	_build_game_variant_rally (0000)
00099380 0090:
	_build_game_variant_team_race (0000)
00099410 0090:
	_build_game_variant_team_rally (0000)
000994A0 0030:
	_game_engine_override_map_name (0000)
000994D0 0020:
	_game_engine_override_game_variant (0000)
000994F0 01a0:
	_code_000994f0 (0000)
00099690 0060:
	_game_engine_hud_draw_messages (0000)
000996F0 0080:
	_game_engine_player_has_stealth_weapon (0000)
00099770 0110:
	_game_engine_weapon_fired (0000)
00099880 0030:
	_code_00099880 (0000)
000998B0 00c0:
	_ticks_to_unicode_time_string (0000)
00099970 0060:
	_game_engine_flag_reset (0000)
000999D0 01b0:
	_game_engine_variant_cleanup (0000)
00099B80 0010:
	_game_engine_allow_pause (0000)
00099B90 0270:
	_code_00099b90 (0000)
00099E00 0020:
	_game_engine_draw_object_in_motion_sensor (0000)
00099E20 0020:
	_game_engine_allow_dynamic_lighting (0000)
00099E40 0020:
	_game_engine_allow_integrated_lights (0000)
00099E60 0030:
	_game_engine_force_autopickup (0000)
00099E90 0060:
	_game_engine_initialize (0000)
00099EF0 0100:
	_multiple_teams_alive (0000)
00099FF0 00f0:
	_team_has_players (0000)
0009A0E0 0020:
	_game_engine_should_end_game (0000)
0009A100 0080:
	_code_0009a100 (0000)
0009A180 0230:
	_populate_statistic_buffer (0000)
0009A3B0 0080:
	_code_0009a3b0 (0000)
0009A430 0060:
	_postgame_statistic_get_rating (0000)
0009A490 01f0:
	_code_0009a490 (0000)
0009A680 01c0:
	_code_0009a680 (0000)
0009A840 0100:
	_code_0009a840 (0000)
0009A940 0920:
	_code_0009a940 (0000)
0009B260 0080:
	_code_0009b260 (0000)
0009B2E0 0020:
	_code_0009b2e0 (0000)
0009B300 00a0:
	_match_game_type (0000)
0009B3A0 0150:
	_code_0009b3a0 (0000)
0009B4F0 0070:
	_code_0009b4f0 (0000)
0009B560 0060:
	_game_engine_update_player_always_invis (0000)
0009B5C0 00e0:
	_game_engine_update_non_deterministic (0000)
0009B6A0 0040:
	_code_0009b6a0 (0000)
0009B6E0 0090:
	_code_0009b6e0 (0000)
0009B770 0010:
	_code_0009b770 (0000)
0009B780 0060:
	_game_show_score_team (0000)
0009B7E0 00d0:
	_game_show_score_you_ally_enemy (0000)
0009B8B0 0080:
	_game_show_score_extended (0000)
0009B930 0020:
	_game_show_score (0000)
0009B950 0110:
	_find_netgame_flags (0000)
0009BA60 0040:
	_find_netgame_flag (0000)
0009BAA0 0130:
	_code_0009baa0 (0000)
0009BBD0 0150:
	_game_engine_postspawn_player_update (0000)
0009BD20 00d0:
	_game_engine_get_damage_multiplier (0000)
0009BDF0 03e0:
	_code_0009bdf0 (0000)
0009C1D0 0170:
	_game_engine_get_distance_rating_for_spawn (0000)
0009C340 0120:
	_code_0009c340 (0000)
0009C460 00b0:
	_code_0009c460 (0000)
0009C510 0060:
	_game_engine_get_starting_location_rating (0000)
0009C570 0220:
	_game_engine_get_variant_by_name (0000)
0009C790 0130:
	_game_engine_remap_equipment (0000)
0009C8C0 0070:
	_game_engine_remap_object_definition (0000)
0009C930 0140:
	_game_engine_get_state_message (0000)
0009CA70 00c0:
	_game_engine_did_player_win_default (0000)
0009CB30 0030:
	_game_engine_did_player_win (0000)
0009CB60 0080:
	_code_0009cb60 (0000)
0009CBE0 0040:
	_code_0009cbe0 (0000)
0009CC20 0060:
	_code_0009cc20 (0000)
0009CC80 0070:
	_code_0009cc80 (0000)
0009CCF0 0280:
	_code_0009ccf0 (0000)
0009CF70 0010:
	_game_engine_playlist_initialize (0000)
0009CF80 0080:
	_game_engine_initialize_for_new_map (0000)
0009D000 0140:
	_game_engine_player_added (0000)
0009D140 0480:
	_code_0009d140 (0000)
0009D5C0 09e0:
	_game_engine_post_rasterize_post_game (0000)
0009DFA0 02f0:
	_game_engine_update (0000)
0009E290 0340:
	_game_engine_player_killed (0000)
0009E5D0 00a0:
	_game_engine_nonplayer_post_rasterize (0000)
0009E670 0350:
	_code_0009e670 (0000)
0009E9C0 0140:
	_code_0009e9c0 (0000)
0009EB00 0060:
	_game_engine_post_rasterize (0000)
0025B190 0008:
	__real@3ffe666660000000 (0000)
0025B198 0002:
	??_C@_11LOCGONAA@?$AA?$AA@ (0000)
0025B19C 0019:
	??_C@_0BJ@IAIEOKNM@ui?2multiplayer_game_text?$AA@ (0000)
0025B1B8 001d:
	??_C@_0BN@BLHEDFPC@weapon_is_flag?$CIweapon_index?$CJ?$AA@ (0000)
0025B1D8 0022:
	??_C@_0CC@HKAICMNO@c?3?2halo?2SOURCE?2game?2game_engine?4@ (0000)
0025B1FC 003c:
	??_C@_0DM@INOFLLBD@?$CIitem?9?$DOobject?4scale?5?$DO?$DN?50?45f?$CJ?5?$CG?$CG?5@ (0000)
0025B238 0014:
	??_C@_0BE@PEGHDBAP@NULL?5?$CB?$DN?5game_engine?$AA@ (0000)
0025B24C 0014:
	??_C@_0BE@CKCLKIJA@variant?5?$CG?$CG?5map_name?$AA@ (0000)
0025B260 001a:
	??_C@_0BK@EKPIBKAB@dead_player_index?5?$CB?$DN?5NONE?$AA@ (0000)
0025B27C 000c:
	??_C@_0M@CLNODINK@game_engine?$AA@ (0000)
0025B288 0008:
	__real@3f53104b57cf969e (0000)
0025B290 0007:
	??_C@_06IEOJBDIK@object?$AA@ (0000)
0025B298 0097:
	??_C@_0JH@PNHLNBOL@?$CBallow_pick_up?5?$HM?$HM?5?$CBTEST_FLAG?$CIwea@ (0000)
0025B330 001a:
	??_C@_0BK@LOIEOAAE@global_goal?$FLindex?$FN?4in_use?$AA@ (0000)
0025B34C 0004:
	__real@3f2147ae (0000)
0025B350 0015:
	??_C@_0BF@MOJDLNAG@NONE?5?$CB?$DN?5lookup_index?$AA@ (0000)
0025B368 001d:
	??_C@_0BN@OIBHHACP@place?4place?5?$DM?5maximum_places?$AA@ (0000)
0025B388 002c:
	??_C@_0CM@IEEKPKHB@?$CI?$CBall_tied?5?$HM?$HM?5?$CItied?$CJ?$CJ?5?$HM?$HM?5?$CI1?5?$DN?$DN?5g@ (0000)
0025B3B4 001e:
	??_C@_0BO@HPNHAHOG@levels?2test?2carousel?2carousel?$AA@ (0000)
0025B3D4 000c:
	??_C@_1M@CDCAHPKB@?$AA?$CF?$AAs?$AA?3?$AA?$CF?$AAs?$AA?$AA@ (0000)
0025B3E0 0008:
	??_C@_17PMDFIHDK@?$AA0?$AA?$CF?$AAd?$AA?$AA@ (0000)
0025B3E8 0006:
	??_C@_15KNBIKKIN@?$AA?$CF?$AAd?$AA?$AA@ (0000)
0025B3F0 0004:
	??_C@_13HOIJIPNN@?$AA?5?$AA?$AA@ (0000)
0025B3F8 0046:
	??_C@_0EG@GDPJDEHO@NETGAME?5CODE?5FAILURE?3?5game_engin@ (0000)
0025B440 001b:
	??_C@_0BL@FBPHGMEB@player?9?$DOteam_index?5?$CB?$DN?5NONE?$AA@ (0000)
0025B45C 0013:
	??_C@_0BD@JDPLED@player_index?$CB?$DNNONE?$AA@ (0000)
0025B470 002b:
	??_C@_0CL@DLMKKJM@player_count?5?$DM?5MULTIPLAYER_MAXIM@ (0000)
0025B49C 0022:
	??_C@_0CC@FPLNHLNP@place?$DMMULTIPLAYER_MAXIMUM_PLAYER@ (0000)
0025B4C0 0013:
	??_C@_0BD@EGHPGJBP@found?5local?5player?$AA@ (0000)
0025B4D4 001d:
	??_C@_0BN@LMCPNAFN@player_count?$DN?$CFd?0?5maxcount?$DN?$CFd?$AA@ (0000)
0025B4F8 0008:
	__real@3fc0bf25a0000000 (0000)
0025B500 0015:
	??_C@_0BF@IHIEPAI@NONE?5?$CB?$DN?5player_index?$AA@ (0000)
0025B518 0016:
	??_C@_0BG@OJNPNGFL@failed?5to?5teleport?5?$CFd?$AA@ (0000)
0025B530 0008:
	__real@3fe3333340000000 (0000)
0025B538 000a:
	??_C@_09DOOCKEDN@team_king?$AA@ (0000)
0025B544 0005:
	??_C@_04PJOEONHN@king?$AA@ (0000)
0025B54C 0008:
	??_C@_07LIOCHKOH@ironctf?$AA@ (0000)
0025B554 0004:
	??_C@_03JHHHHEKD@ctf?$AA@ (0000)
0025B558 0008:
	??_C@_07JHGHBFJP@oddball?$AA@ (0000)
0025B560 000d:
	??_C@_0N@JBNPINGA@accumulation?$AA@ (0000)
0025B570 000d:
	??_C@_0N@CPCFMJOB@team_oddball?$AA@ (0000)
0025B580 0008:
	??_C@_07IAHNGGND@stalker?$AA@ (0000)
0025B588 000c:
	??_C@_0M@MLMHALFL@elimination?$AA@ (0000)
0025B594 000c:
	??_C@_0M@NOCMPHHF@team_slayer?$AA@ (0000)
0025B5A0 0007:
	??_C@_06CBFFIGEC@slayer?$AA@ (0000)
0025B5A8 0006:
	??_C@_05GCEFBECL@rally?$AA@ (0000)
0025B5B0 000a:
	??_C@_09KMEIIIPA@team_race?$AA@ (0000)
0025B5BC 0005:
	??_C@_04GLEOMBLA@race?$AA@ (0000)
0025B5C4 0004:
	__real@3f0ccccd (0000)
0025B5C8 003b:
	??_C@_0DL@MBPNEOKF@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B604 003b:
	??_C@_0DL@FDFHGCGI@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B640 003e:
	??_C@_0DO@GDPJBCBM@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B680 003d:
	??_C@_0DN@GLOHFEFE@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B6C0 003a:
	??_C@_0DK@COLEDOMN@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B700 0047:
	??_C@_0EH@GJGBEMDH@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B748 0047:
	??_C@_0EH@BMGHBGKH@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B790 004a:
	??_C@_0EK@DHDLIGMN@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B7E0 0049:
	??_C@_0EJ@FACIONE@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B830 004f:
	??_C@_0EP@CGHPHLAM@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B880 004f:
	??_C@_0EP@MJLNBADC@NETGAME?5MAP?5FAILURE?3?5failed?5to?5f@ (0000)
0025B8D0 0039:
	??_C@_0DJ@MPJKFIIJ@NETGAME?5MAP?5FAILURE?3?5duplicate?5r@ (0000)
0025B90C 0031:
	??_C@_0DB@KDFIELID@NETGAME?5MAP?5FAILURE?3?5missing?5rac@ (0000)
0025B940 0034:
	??_C@_0DE@KIBANADL@NETGAME?5MAP?5FAILURE?3?5missing?5odd@ (0000)
0025B974 0031:
	??_C@_0DB@IKODHNKI@NETGAME?5MAP?5FAILURE?3?5missing?5hil@ (0000)
0025B9A8 0035:
	??_C@_0DF@EHBELDKM@NETGAME?5MAP?5FAILURE?3?5ctf?5flag?5ou@ (0000)
0025B9E0 0032:
	??_C@_0DC@DCAAPLNK@NETGAME?5MAP?5FAILURE?3?5duplicate?5c@ (0000)
0025BA14 0030:
	??_C@_0DA@LKKJMHEH@NETGAME?5MAP?5FAILURE?3?5missing?5ctf@ (0000)
0025BA48 0057:
	??_C@_0FH@ODDMDCIB@failed?5to?5initialize?5custome?5gam@ (0000)
0025BAA0 000d:
	??_C@_0N@MABFIMPF@title_string?$AA@ (0000)
0025BAB0 001e:
	??_C@_1BO@NNCACCCJ@?$AA?5?$AA?7?$AA?5?$AA?7?$AA?5?$AA?7?$AA?5?$AA?7?$AA?5?$AA?7?$AA?5?$AA?7?$AA?$CF?$AAd?$AA?$AA@ (0000)
0025BAD0 001a:
	??_C@_1BK@JPAKMPFP@?$AA?5?$AA?7?$AA?5?$AA?7?$AA?5?$AA?7?$AA?5?$AA?7?$AA?5?$AA?7?$AA?$CF?$AAd?$AA?$AA@ (0000)
0025BAEC 0016:
	??_C@_1BG@EEDCCJKF@?$AA?5?$AA?7?$AA?5?$AA?7?$AA?5?$AA?7?$AA?5?$AA?7?$AA?$CF?$AAd?$AA?$AA@ (0000)
0025BB04 0012:
	??_C@_1BC@DGONEHBJ@?$AA?5?$AA?7?$AA?5?$AA?7?$AA?5?$AA?7?$AA?$CF?$AAs?$AA?$AA@ (0000)
0025BB18 000e:
	??_C@_1O@HAEHHN@?$AA?5?$AA?7?$AA?5?$AA?7?$AA?$CF?$AAs?$AA?$AA@ (0000)
0025BB28 000a:
	??_C@_19GCIMBEIF@?$AA?5?$AA?7?$AA?$CF?$AAs?$AA?$AA@ (0000)
0025BB34 0026:
	??_C@_1CG@MDCDIADO@?$AA?7?$AA?$CF?$AAs?$AA?7?$AA?$CF?$AAs?$AA?7?$AA?$CF?$AAs?$AA?7?$AA?$CF?$AAs?$AA?7?$AA?$CF?$AAs?$AA?7?$AA?$CF?$AAs?$AA?$AA@ (0000)
0025BB5C 0012:
	??_C@_0BC@FNEEDLLE@ambient_computers?$AA@ (0000)
0025BB70 0012:
	??_C@_0BC@GOFPLFEH@ambient_machinery?$AA@ (0000)
0025BB84 000f:
	??_C@_0P@PLBDAHNN@ambient_nature?$AA@ (0000)
0025BB94 0014:
	??_C@_1BE@EFJKLKFF@?$AA?7?$AA?$CF?$AAs?$AA?7?$AA?$CF?$AAs?$AA?7?$AA?$CF?$AAs?$AA?$AA@ (0000)
0025BBA8 001b:
	??_C@_0BL@NPJONDMM@NONE?5?$CB?$DN?5local_player_index?$AA@ (0000)
002DE3E0 0020:
	_debug_player_color (0000)
0043E498 047c:
	_global_goal (0000)
	_global_variant (0400)
	_game_engine (0468)
	_global_autogenerate_list (0474)
	_global_autogenerate_count (0478)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries_windows.h"
#include "game_engine.h"

#include "game_globals.h"
#include "interface/ui_widget.h"
#include "items/weapon_definitions.h"
#include "items/weapons.h"
#include "main/main.h"
#include "networking/network_game_globals.h"
#include "objects.h"
#include "players.h"
#include "scenario/scenario.h"
#include "units/units.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct game_engine_goal
{
	real_point3d position;
	boolean in_use;
	byte unused[0x13];
};

struct game_engine_globals
{
	unsigned long flags;
	byte unused4[4];
	real postgame_timer;
	byte unusedC[4];
	long postgame_state;
	real hud_message_timers[MAXIMUM_LOCAL_PLAYERS];
};

struct game_engine_stage
{
	char map_name[64];
	struct game_variant variant;
};

typedef char verify_game_engine_goal_size[sizeof(struct game_engine_goal) == 0x20 ? 1 : -1];
typedef char verify_game_engine_globals_postgame_timer_offset[
	offsetof(struct game_engine_globals, postgame_timer) == 0x8 ? 1 : -1];
typedef char verify_game_engine_globals_postgame_state_offset[
	offsetof(struct game_engine_globals, postgame_state) == 0x10 ? 1 : -1];
typedef char verify_game_engine_globals_hud_message_timers_offset[
	offsetof(struct game_engine_globals, hud_message_timers) == 0x14 ? 1 : -1];
typedef char verify_game_engine_globals_size[
	sizeof(struct game_engine_globals) == 0x24 ? 1 : -1];
typedef char verify_game_engine_stage_variant_offset[
	offsetof(struct game_engine_stage, variant) == 0x40 ? 1 : -1];
typedef char verify_game_engine_stage_size[
	sizeof(struct game_engine_stage) == 0xA8 ? 1 : -1];

/* ---------- prototypes */

void game_engine_playlist_next(
	long parameter0,
	long parameter1,
	long playlist_type);

void code_00096890(
	void);

long game_engine_did_player_win_default(
	long player_index);

void game_show_score_extended(
	long player_index,
	long score,
	long team_index);

boolean multiple_teams_alive(
	void);

struct network_game_server *global_network_game_server_get(
	void);

boolean player_ui_game_variant_specified(
	struct game_variant *variant);

/* ---------- globals */

struct game_engine *game_engine;

extern struct game_engine_goal global_goal[32];
extern struct game_variant global_variant;
extern struct game_engine_globals game_engine_globals;
extern struct game_engine_stage global_stage;
extern struct game_engine *game_engines[];

/* ---------- public code */

long game_globals_get_weapon(
	struct game_globals *game_globals,
	long weapon_list_index)
{
	struct tag_reference *weapon = TAG_BLOCK_GET_ELEMENT(
		&game_globals->weapon_list,
		weapon_list_index,
		struct tag_reference);

	return weapon->index;
}

long game_engine_get_team_score(
	long team_index)
{
	struct data_iterator iterator;
	struct player_datum *player;

	data_iterator_new(&iterator, player_data);
	player = (struct player_datum *)data_iterator_next(&iterator);
	while (player)
	{
		if (player->team_index == team_index)
			return game_engine->get_player_score(iterator.datum_index, TRUE);

		player = (struct player_datum *)data_iterator_next(&iterator);
	}

	return 0;
}

real linear_to_non_linear_alpha(
	real linear_alpha)
{
	real non_linear_alpha = (real)pow((double)linear_alpha, 1.9f);

	return non_linear_alpha;
}

long sort_statistic_buffer(
	long entry0,
	long entry1)
{
	long result = 0;
	long entry0_value;
	long entry1_value;

	entry0_value = *(long *)(entry0 + sizeof(long));
	entry1_value = *(long *)(entry1 + sizeof(long));
	if (entry1_value < entry0_value)
		return -1;
	if (entry1_value > entry0_value)
		result = 1;

	return result;
}

long players_in_game(
	void)
{
	struct data_iterator iterator;
	long player_count = 0;

	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
		player_count++;

	return player_count;
}

void code_00097560(
	void)
{
	return;
}

boolean code_00097c00(
	void)
{
	boolean infinite_grenades = FALSE;

	if (!TEST_FLAG(game_engine_globals.flags, _game_engine_disable_infinite_grenades_bit))
		infinite_grenades = TEST_FLAG(global_variant.flags, _game_variant_infinite_grenades_bit);

	return infinite_grenades;
}

boolean code_00096c80(
	long object_index,
	long const *excluded_player_index)
{
	long excluded_player = *excluded_player_index;
	boolean result = FALSE;
	struct object_datum *object = object_get(object_index);

	if (!TEST_FLAG(object->object.flags, _object_invisible_bit) &&
		TEST_FLAG(_object_mask_biped, object->object.type) &&
		!TEST_FLAG(object->object.damage_flags, _object_dead_bit) &&
		player_index_from_unit_index(object_index) != excluded_player)
	{
		result = TRUE;
	}

	return result;
}

void code_00096d30(
	void)
{
	struct object_iterator iterator;
	long cutoff_time = game_time_get() - 900;

	object_iterator_new(&iterator, _object_mask_item, 0);
	while (object_iterator_next(&iterator))
	{
		struct item_datum *item = item_get(iterator.index);

		if (item->item.last_owned_time < cutoff_time &&
			!TEST_FLAG(item->item.flags, _item_attached_to_unit_bit))
		{
			long item_index = iterator.index;
			struct weapon_datum *weapon = weapon_try_and_get(item_index);
			if (!weapon || !weapon_is_flag(item_index))
				object_delete(iterator.index);
		}
	}

	object_iterator_new(&iterator, _object_mask_biped, 0);
	while (object_iterator_next(&iterator))
	{
		struct object_datum *object;

		cutoff_time = 900;
		object = object_get(iterator.index);

		if (object->object.idle_ticks > (short)cutoff_time &&
			TEST_FLAG(object->object.damage_flags, _object_dead_bit))
		{
			object_delete(iterator.index);
		}
	}

	return;
}

void game_engine_flag_reset(
	long weapon_index,
	real_point3d const *position)
{
	if (weapon_index != NONE)
	{
		struct weapon_datum *weapon = weapon_get(weapon_index);

		object_set_position(weapon_index, position, global_forward3d, global_up3d);
		object_reset(weapon_index);
		SET_FLAG(weapon->weapon.flags, _weapon_runtime_game_engine_active_bit, FALSE);
		weapon->item.last_owned_time = game_time_get();
		weapon->item.ignore_object_index = NONE;
	}

	return;
}

void game_engine_playlist_initialize(
	void)
{
	game_engine_playlist_next(0, 0, 2);

	return;
}

void game_engine_playlist_begin(
	void)
{
	main_set_multiplayer_map_name(global_stage.map_name);
	game_set_game_variant(&global_stage.variant);

	if (!network_game_is_active())
		main_reset_map();

	return;
}

boolean game_engine_get_current_stage(
	struct game_variant *variant,
	char *map_name)
{
	match_assert("c:\\halo\\SOURCE\\game\\game_engine.c", 0x918, variant && map_name);

	csmemcpy(variant, &global_stage.variant, sizeof(*variant));
	csstrncpy(map_name, global_stage.map_name, sizeof(global_stage.map_name)-1);
	map_name[sizeof(global_stage.map_name)-1] = 0;

	return TRUE;
}

long list_index_to_weapon_definition_index(
	long weapon_list_index)
{
	long weapon_definition_index = NONE;

	if (weapon_list_index!=NONE)
	{
		struct game_globals *game_globals = scenario_get_game_globals();
		struct tag_reference *weapon = TAG_BLOCK_GET_ELEMENT(
			&game_globals->weapon_list,
			weapon_list_index,
			struct tag_reference);

		weapon_definition_index = weapon->index;
	}

	return weapon_definition_index;
}

void game_engine_state_message(
	long player_index,
	long state_message,
	long state_message_player_index)
{
	struct player_datum *player = player_get(player_index);

	player->state_message = state_message;
	player->state_message_player_index = state_message_player_index;

	return;
}

void game_engine_player_depower_active_camo(
	long player_index)
{
	if (player_index!=NONE)
	{
		struct player_datum *player = player_get(player_index);

		if (player->unit_index!=NONE)
		{
			struct unit_datum *unit = unit_get(player->unit_index);

			if (TEST_FLAG(unit->unit.flags, _unit_active_camouflaged_bit))
				unit->unit.active_camouflage = 0.5f;
		}
	}

	return;
}

float get_blink_alpha(
	void)
{
	long phase = system_milliseconds()%2700;

	return sin(phase * (3.14159265358979/2700.0));
}

long game_engine_player_get_team_index(
	long player_index)
{
	long team_index = 1;

	match_assert("c:\\halo\\SOURCE\\game\\game_engine.c", 0xC11, game_engine);

	if (!game_engine->team_index_override)
		team_index = player_get(player_index)->local_player_index%2;

	return team_index;
}

void game_engine_update_player_always_invis(
	long player_index)
{
	if (game_engine)
	{
		if ((TEST_FLAG(global_variant.flags, _game_variant_always_invisible_bit) ||
			game_engine->test_trait && game_engine->test_trait(player_index, 1)) &&
			player_get(player_index)->unit_index!=NONE)
		{
			player_handle_powerup_minor(player_index, 0, 15);
		}
	}

	return;
}

boolean game_engine_player_has_flag(
	long player_index)
{
	boolean has_flag = FALSE;

	if (player_index!=NONE)
	{
		struct player_datum *player = player_get(player_index);

		if (player->unit_index!=NONE)
		{
			struct unit_datum *unit = unit_get(player->unit_index);
			long weapon_index;

			for (weapon_index = 0; weapon_index<MAXIMUM_WEAPONS_PER_UNIT; weapon_index++)
			{
				long weapon_object_index = unit->unit.weapon_object_indices[weapon_index];

				if (weapon_object_index!=NONE && weapon_is_flag(weapon_object_index))
				{
					has_flag = TRUE;
					break;
				}
			}
		}
	}

	return has_flag;
}

void game_show_score(
	long player_index,
	long score)
{
	game_show_score_extended(player_index, score, NONE);

	return;
}

void get_postgame_hilite_colors(
	real_argb_color *winner_color,
	real_argb_color *normal_color,
	real_argb_color *hilite_color)
{
	winner_color->red = 0.45882353f;
	winner_color->green = 0.7294118f;
	winner_color->blue = 1.0f;
	winner_color->alpha = 1.0f;

	normal_color->red = 1.0f;
	normal_color->green = 1.0f;
	normal_color->blue = 0.0f;
	normal_color->alpha = 1.0f;

	hilite_color->red = 0.98f;
	hilite_color->green = 0.96f;
	hilite_color->blue = 0.96f;
	hilite_color->alpha = 1.0f;

	return;
}

boolean game_engine_running(
	void)
{
	boolean running = game_engine!=NULL;

	return running;
}

boolean game_engine_force_single_screen(
	void)
{
	boolean force_single_screen = FALSE;

	if (game_engine &&
		game_engine_globals.postgame_state>=2 &&
		game_engine_globals.postgame_state<=3)
	{
		force_single_screen = TRUE;
	}

	return force_single_screen;
}

void game_engine_dispose(
	void)
{
	if (game_engine)
	{
		if (game_engine->dispose)
			game_engine->dispose();

		game_engine = NULL;
	}

	return;
}

void game_engine_dispose_from_old_map(
	void)
{
	if (game_engine && game_engine->dispose_from_old_map)
		game_engine->dispose_from_old_map();

	return;
}

void game_engine_game_ending(
	void)
{
	if (game_engine && game_engine->game_ending)
		game_engine->game_ending();

	return;
}

void game_engine_game_starting(
	void)
{
	if (game_engine)
	{
		if (game_engine->game_starting)
			game_engine->game_starting();

		code_00096890();
	}

	return;
}

void game_engine_statistics_append(
	long statistic)
{
	if (game_engine && game_engine->statistics_append)
		game_engine->statistics_append(statistic);

	return;
}

void game_engine_handle_client_message(
	void *message)
{
	if (game_engine && game_engine->handle_client_message)
		game_engine->handle_client_message(message);

	return;
}

void game_engine_handle_server_message(
	void *message)
{
	if (game_engine && game_engine->handle_server_message)
		game_engine->handle_server_message(message);

	return;
}

void game_engine_post_rasterize_objects(
	void)
{
	if (game_engine && game_engine->post_rasterize_objects)
		game_engine->post_rasterize_objects();

	return;
}

boolean game_engine_display_team_indicators(
	void)
{
	boolean display = FALSE;

	if (game_engine)
		display = TEST_FLAG(global_variant.flags, _game_variant_unknown1_bit) && global_variant.has_teams;

	return display;
}

boolean game_engine_can_score(
	void)
{
	boolean can_score = TRUE;

	if (game_engine)
		can_score = game_engine_globals.postgame_state==0;

	return can_score;
}

boolean game_engine_infinite_grenades(
	long player_index)
{
	boolean infinite_grenades = FALSE;

	if (game_engine &&
		player_index!=NONE &&
		!TEST_FLAG(game_engine_globals.flags, _game_engine_disable_infinite_grenades_bit))
	{
		infinite_grenades = TEST_FLAG(global_variant.flags, _game_variant_infinite_grenades_bit);
	}

	return infinite_grenades;
}

boolean game_engine_has_shield(
	long player_index)
{
	boolean has_shield = TRUE;

	if (game_engine && player_index!=NONE)
		has_shield = !TEST_FLAG(global_variant.flags, _game_variant_no_shields_bit);

	return has_shield;
}

boolean game_engine_draw_object_in_motion_sensor(
	void)
{
	boolean draw_object = TRUE;

	if (game_engine)
		draw_object = TEST_FLAG(global_variant.flags, _game_variant_draw_object_in_motion_sensor_bit);

	return draw_object;
}

boolean game_engine_hud_draw_motion_sensor(
	void)
{
	boolean draw_motion_sensor = TRUE;

	if (game_engine)
	{
		boolean default_draw_motion_sensor;

		draw_motion_sensor =
			TEST_FLAG(global_variant.flags, _game_variant_draw_object_in_motion_sensor_bit);
		default_draw_motion_sensor = global_variant.unknown24 == 0;

		if (global_variant.engine_type == 2 && !global_variant.unknown4C.byte2)
			default_draw_motion_sensor = FALSE;

		draw_motion_sensor |= default_draw_motion_sensor;
	}

	return draw_motion_sensor;
}

boolean game_engine_player_has_stealth_weapon(
	long player_index)
{
	boolean has_stealth_weapon = FALSE;

	if (player_index != NONE)
	{
		struct player_datum *player = player_get(player_index);
		if (player->unit_index != NONE)
		{
			struct unit_datum *unit = unit_get(player->unit_index);
			long current_weapon_index = unit->unit.current_weapon_index;

			if (current_weapon_index != NONE)
			{
				long weapon_index =
					unit->unit.weapon_object_indices[current_weapon_index];
				if (weapon_index != NONE)
				{
					struct weapon_datum *weapon = weapon_get(weapon_index);
					if (weapon->definition_index != NONE)
					{
						struct weapon_definition *weapon_definition =
							weapon_definition_get(weapon->definition_index);
						has_stealth_weapon = TEST_FLAG(
							weapon_definition->weapon.flags,
							_weapon_does_not_depower_active_camo_bit);
					}
				}
			}
		}
	}

	return has_stealth_weapon;
}

long find_netgame_flag(
	float const *position,
	float radius,
	float height,
	short type,
	short index)
{
	long flag_index = NONE;

	find_netgame_flags(position, radius, height, type, index, 1, &flag_index);

	return flag_index;
}

void game_engine_playlist_next(
	long parameter0,
	long parameter1,
	long playlist_type)
{
	char *map_name;
	struct game_variant variant;

	csstrcpy(global_stage.map_name, "levels\\test\\carousel\\carousel");
	map_name = main_get_multiplayer_map_name();
	if (map_name && map_name[0])
	{
		csstrncpy(global_stage.map_name, map_name, sizeof(global_stage.map_name) - 1);
		global_stage.map_name[sizeof(global_stage.map_name) - 1] = 0;
	}

	if (player_ui_game_variant_specified(&variant))
		csmemcpy(&global_stage.variant, &variant, sizeof(global_stage.variant));

	return;
}

boolean game_engine_should_end_game(
	void)
{
	boolean should_end_game = FALSE;

	if (game_engine && !multiple_teams_alive())
		should_end_game = TRUE;

	return should_end_game;
}

void game_engine_clear_goal_position(
	short goal_index)
{
	csmemset(&global_goal[goal_index], 0, sizeof(struct game_engine_goal));

	return;
}

long get_flag_definition_index(
	void)
{
	struct game_globals *game_globals;
	struct game_globals_multiplayer_information *multiplayer_information;

	global_scenario_get();
	game_globals = scenario_get_game_globals();
	multiplayer_information = TAG_BLOCK_GET_ELEMENT(
		&game_globals->multiplayer_information,
		0,
		struct game_globals_multiplayer_information);

	return multiplayer_information->flag.index;
}

long get_ball_definition_index(
	void)
{
	struct game_globals *game_globals;
	struct game_globals_multiplayer_information *multiplayer_information;

	global_scenario_get();
	game_globals = scenario_get_game_globals();
	multiplayer_information = TAG_BLOCK_GET_ELEMENT(
		&game_globals->multiplayer_information,
		0,
		struct game_globals_multiplayer_information);

	return multiplayer_information->ball.index;
}

void game_engine_override_map_name(
	char const *map_name)
{
	if (map_name && map_name[0])
		csstrncpy(global_stage.map_name, map_name, sizeof(global_stage.map_name)-1);

	return;
}

void game_engine_override_game_variant(
	struct game_variant const *variant)
{
	if (variant)
		csmemcpy(&global_stage.variant, variant, sizeof(global_stage.variant));

	return;
}

void game_engine_switch_to_postgame(
	void)
{
	if (game_engine_globals.postgame_state==0)
	{
		if (global_network_game_server_get())
		{
			game_engine_globals.postgame_state = 1;
			game_engine_globals.postgame_timer = 7.0f;
		}
		else
		{
			game_engine_globals.postgame_state = 3;
		}
	}

	return;
}

void game_engine_load_stage(
	char const *map_name)
{
	if (!map_name || csstrcmp(global_stage.map_name, map_name))
		main_set_multiplayer_map_name(global_stage.map_name);

	game_set_game_variant(&global_stage.variant);
	if (!network_game_is_active())
		main_reset_map();

	return;
}

void game_engine_end_game(
	void)
{
	if (game_engine_globals.postgame_state==0)
	{
		game_engine_globals.postgame_state = 1;
		game_engine_globals.postgame_timer = 7.0f;
		game_engine_play_multiplayer_sound(1);
		ui_widgets_close_all();
	}

	return;
}

void game_engine_player_damaged_player(
	long damaging_player_index,
	long dead_player_index,
	long damage_type)
{
	match_assert("c:\\halo\\SOURCE\\game\\game_engine.c", 0xA20, dead_player_index != NONE);

	if (game_engine && game_engine->player_damaged_player)
		game_engine->player_damaged_player(damaging_player_index, dead_player_index, damage_type);

	return;
}

boolean game_engine_player_is_out_of_lives(
	long player_index)
{
	boolean out_of_lives = FALSE;

	if (global_variant.maximum_lives>0)
	{
		struct player_datum *player = player_get(player_index);

		if (player->unit_index==NONE && player->statistics.deaths>=global_variant.maximum_lives)
			out_of_lives = TRUE;
	}

	return out_of_lives;
}

boolean game_engine_hud_draw_messages(
	long player_index)
{
	boolean draw_messages = TRUE;

	if (game_engine && player_index!=NONE)
	{
		struct player_datum *player = player_get(player_index);

		if (player->local_player_index!=NONE &&
			game_engine_globals.hud_message_timers[player->local_player_index]>0.0f)
		{
			draw_messages = FALSE;
		}
	}

	return draw_messages;
}

boolean game_engine_force_autopickup(
	long unit_index,
	long weapon_index)
{
	boolean force_autopickup = FALSE;

	if (game_engine && global_variant.engine_type==1 && weapon_is_flag(weapon_index))
		force_autopickup = TRUE;

	return force_autopickup;
}

boolean game_engine_allow_pick_up(
	long unit_index,
	long weapon_index)
{
	boolean allow_pick_up = TRUE;

	if (game_engine && game_engine->allow_pick_up)
		allow_pick_up = game_engine->allow_pick_up(unit_index, weapon_index);

	return allow_pick_up;
}

boolean game_engine_test_flag(
	long flag)
{
	boolean result = FALSE;

	if (game_engine && game_engine->test_flag)
		result = game_engine->test_flag(flag);

	return result;
}

boolean game_engine_test_trait(
	long trait,
	long value)
{
	boolean result = FALSE;

	if (game_engine && game_engine->test_trait)
		result = game_engine->test_trait(trait, value);

	return result;
}

void game_engine_prespawn_player_update(
	long player_index)
{
	if (game_engine)
	{
		if (game_engine->prespawn_player_update)
			game_engine->prespawn_player_update(player_index);
		else
		{
			struct player_datum *player = player_get(player_index);
			player->team_index = player->local_player_index % 2;
		}
	}

	return;
}

long game_engine_did_player_win(
	long player_index)
{
	long result = 0;

	if (game_engine)
	{
		if (game_engine->did_player_win)
			result = game_engine->did_player_win(player_index);
		else
			result = game_engine_did_player_win_default(player_index);
	}

	return result;
}

long code_0009b2e0(
	void)
{
	long game_engine_type = NONE;

	if (game_engine)
		game_engine_type = game_engine->type;

	return game_engine_type;
}

struct game_variant *game_engine_get_variant(
	void)
{
	return &global_variant;
}

boolean game_engine_get_goal_in_use(
	short goal_index)
{
	return global_goal[goal_index].in_use;
}

/*
 * game_engine_get_goal_position (January 0x00097A30, 0x60 bytes) is deferred.
 * It reconstructs to the correct 96 bytes and 6 relocations with
 *
 *     match_assert("c:\halo\SOURCE\game\game_engine.c", 0xFE2,
 *         global_goal[index].in_use);
 *     *position = global_goal[index].position;
 *
 * (signature void(real_point3d *position, short index); the parameter is named
 * `index`, recovered from the stringified assert message string). The only
 * residual is the register schedule of the trailing real_point3d copy: January
 * reuses ESI as the copy temp with two redundant pointer materialisations
 * (mov ecx,eax / mov edx,eax -- the typed-local-alias pattern), whereas every
 * shape tried here moves the three dwords tighter through EDX. ~8 source
 * variants (direct copy, goal/out pointer locals, out alias, field-wise, block
 * scope) all match size and relocations but not the schedule. This is the same
 * class of getter tuning as get_starting_location_rating / man_out: correct
 * logic, residual instruction-schedule tie needing typed-local-alias steering.
 * Revisit before landing the goal getters.
 */

/*
 * game_engine_man_out(long player_index) is deferred: logic fully recovered,
 * residual schedule tie. It is (boolean result):
 *
 *     if (player_get(player_index)->unknown_d1)
 *         man_out = TRUE;
 *     else if (global_variant.maximum_lives > 0) {
 *         struct player_datum *player = player_get(player_index);
 *         if (player->unit_index == NONE &&
 *             player->statistics.deaths >= global_variant.maximum_lives)
 *             man_out = TRUE;
 *     }
 *     if (!man_out) man_out = code_00097250();   // fallback, private helper
 *
 * Fields: player_datum.unknown_d1 @0xd1, .unit_index @0x34, .statistics.deaths
 * (game_statistics.deaths @0x1e, i.e. player+0xaa); global_variant.maximum_lives
 * @0x38. Size, relocations (7) and the two datum_get calls all match. Residual:
 * January keeps the conditional fallback code_00097250() call expanded and
 * normalised (112 bytes) and uses EDI for player_index, whereas our CL
 * tail-merges/compacts it to 96 bytes with ESI -- the same "our compiler
 * optimises more" tie as atmospheric_fog. Needs schedule steering; revisit.
 */

boolean game_engine_has_teams(
	void)
{
	boolean has_teams = FALSE;

	if (game_engine)
		has_teams = global_variant.has_teams;

	return has_teams;
}

boolean game_engine_allow_pause(
	void)
{
	return game_engine_globals.postgame_state==0;
}

boolean game_engine_allow_dynamic_lighting(
	long object_index)
{
	boolean allow_dynamic_lighting = TRUE;

	if (game_engine)
		allow_dynamic_lighting = !TEST_FLAG(game_engine_globals.flags, _game_engine_allow_dynamic_lighting_bit);

	return allow_dynamic_lighting;
}

boolean game_engine_allow_integrated_lights(
	long object_index)
{
	boolean allow_integrated_lights = TRUE;

	if (game_engine)
		allow_integrated_lights = !TEST_FLAG(game_engine_globals.flags, _game_engine_allow_integrated_lights_bit);

	return allow_integrated_lights;
}

void game_engine_initialize(
	struct game_variant *variant)
{
	csmemset(&game_engine_globals, 0, sizeof(game_engine_globals));
	game_engine_globals.postgame_state = 0;

	if (variant && variant->engine_type)
	{
		global_variant = *variant;
		game_engine_variant_cleanup(&global_variant);
		game_engine = game_engines[variant->engine_type];
	}

	return;
}

long game_engine_remap_object_definition(
	long definition_index)
{
	short object_type;

	if (!game_engine_running() || definition_index==NONE)
	{
		return definition_index;
	}

	object_type = object_definition_get(definition_index)->object.type;
	if (object_type==_object_type_vehicle)
	{
		return game_engine_remap_vehicle(definition_index);
	}
	if (object_type==_object_type_weapon)
	{
		return game_engine_remap_weapon(definition_index);
	}
	if (object_type==_object_type_equipment)
	{
		return game_engine_remap_equipment(definition_index);
	}

	return definition_index;
}

/* ---------- private code */
