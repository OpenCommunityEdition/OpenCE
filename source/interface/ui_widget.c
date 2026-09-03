/*
UI_WIDGET.C

symbols in this file:
000D32C0 0020:
	_code_000d32c0 (0000)
000D32E0 0020:
	_code_000d32e0 (0000)
000D3300 0030:
	_event_controller_index_compatible_with_widget (0000)
000D3330 0030:
	_set_ui_plasma_effect_color (0000)
000D3360 00a0:
	_ui_widgets_initialize (0000)
000D3400 0010:
	_ui_widgets_safe_to_load (0000)
000D3410 0010:
	_ui_widgets_set_fade_value (0000)
000D3420 0010:
	_ui_widget_debug_show_path (0000)
000D3430 0020:
	_widget_instance_count_children (0000)
000D3450 0050:
	_widget_instance_get_nth_child (0000)
000D34A0 0030:
	_ui_widget_realloc (0000)
000D34D0 0020:
	_widget_free (0000)
000D34F0 0030:
	_ui_widgets_active (0000)
000D3520 0070:
	_ui_widgets_active_for_local_player (0000)
000D3590 0050:
	_ui_widgets_inhibit_processing (0000)
000D35E0 0020:
	_code_000d35e0 (0000)
000D3600 0490:
	_draw_bitmap_in_rect (0000)
000D3A90 0020:
	_widget_instance_get_topmost_parent (0000)
000D3AB0 0040:
	_widget_instance_get_child_index_from_parent (0000)
000D3AF0 0060:
	_widget_instance_set_visibility_recursive (0000)
000D3B50 0010:
	_main_menu_active (0000)
000D3B60 0010:
	_main_menu_is_active (0000)
000D3B70 0030:
	_main_menu_screen_is_active (0000)
000D3BA0 0050:
	_ui_set_next_level (0000)
000D3BF0 0010:
	_ui_widget_load_progress_widget (0000)
000D3C00 0010:
	_filesystem_check_thread_is_active (0000)
000D3C10 0040:
	_code_000d3c10 (0000)
000D3C50 0030:
	_display_error_when_main_menu_loaded (0000)
000D3C80 0090:
	_display_error_deferred (0000)
000D3D10 0040:
	_display_error_abort_to_dashboard_deferred (0000)
000D3D50 0070:
	_ui_start_main_menu_music (0000)
000D3DC0 0060:
	_ui_stop_main_menu_music (0000)
000D3E20 0010:
	_ui_main_menu_music_active (0000)
000D3E30 0040:
	_ui_widgets_disable_pause_game (0000)
000D3E70 0080:
	_code_000d3e70 (0000)
000D3EF0 0060:
	_code_000d3ef0 (0000)
000D3F50 0030:
	_code_000d3f50 (0000)
000D3F80 0090:
	_code_000d3f80 (0000)
000D4010 0080:
	_code_000d4010 (0000)
000D4090 0050:
	_code_000d4090 (0000)
000D40E0 0020:
	_code_000d40e0 (0000)
000D4100 0080:
	_code_000d4100 (0000)
000D4180 0040:
	_code_000d4180 (0000)
000D41C0 0040:
	_code_000d41c0 (0000)
000D4200 0050:
	_code_000d4200 (0000)
000D4250 01a0:
	_code_000d4250 (0000)
000D43F0 0070:
	_code_000d43f0 (0000)
000D4460 0060:
	_code_000d4460 (0000)
000D44C0 0060:
	_code_000d44c0 (0000)
000D4520 0130:
	_remap_sticks_for_local_player (0000)
000D4650 0030:
	_code_000d4650 (0000)
000D4680 0010:
	_code_000d4680 (0000)
000D4690 0010:
	_code_000d4690 (0000)
000D46A0 0170:
	_code_000d46a0 (0000)
000D4810 00d0:
	_code_000d4810 (0000)
000D48E0 0020:
	_code_000d48e0 (0000)
000D4900 01d0:
	_code_000d4900 (0000)
000D4AD0 0030:
	_code_000d4ad0 (0000)
000D4B00 0040:
	_code_000d4b00 (0000)
000D4B40 0020:
	_code_000d4b40 (0000)
000D4B60 0060:
	_code_000d4b60 (0000)
000D4BC0 00a0:
	_code_000d4bc0 (0000)
000D4C60 0050:
	_get_ui_rgb_white (0000)
000D4CB0 0060:
	_get_ui_argb_white (0000)
000D4D10 0050:
	_code_000d4d10 (0000)
000D4D60 0040:
	_modulate_pixel32_by_real_alpha (0000)
000D4DA0 02a0:
	_ui_widget_delete (0000)
000D5040 0050:
	_ui_widgets_close_all (0000)
000D5090 0090:
	_ui_widgets_close_all_for_local_player (0000)
000D5120 0040:
	_ui_widgets_delete_history (0000)
000D5160 0060:
	_ui_widgets_pop_stack (0000)
000D51C0 0070:
	_main_screen_shell_begin_fade (0000)
000D5230 0060:
	_ui_play_audio_feedback_sound (0000)
000D5290 02a0:
	_code_000d5290 (0000)
000D5530 0030:
	_code_000d5530 (0000)
000D5560 0360:
	_draw_string_and_hack_in_icons (0000)
000D58C0 0310:
	_code_000d58c0 (0000)
000D5BD0 0440:
	_code_000d5bd0 (0000)
000D6010 0050:
	_code_000d6010 (0000)
000D6060 00a0:
	_code_000d6060 (0000)
000D6100 00e0:
	_code_000d6100 (0000)
000D61E0 0050:
	_ui_widgets_dispose (0000)
000D6230 0200:
	_widget_event_function_list_widget_goto_next_item (0000)
000D6430 0220:
	_widget_event_function_list_widget_goto_previous_item (0000)
000D6650 04f0:
	_code_000d6650 (0000)
000D6B40 02f0:
	_code_000d6b40 (0000)
000D6E30 00b0:
	_code_000d6e30 (0000)
000D6EE0 0180:
	_render_ui_widgets_postgame (0000)
000D7060 0230:
	_render_ui_widgets (0000)
000D7290 01f0:
	_code_000d7290 (0000)
000D7480 07e0:
	_code_000d7480 (0000)
000D7C60 0220:
	_ui_widget_load_by_name_or_tag (0000)
000D7E80 0130:
	_main_screen_shell_load (0000)
000D7FB0 00e0:
	_network_game_reset_to_pregame_ui (0000)
000D8090 0380:
	_display_error (0000)
000D8410 0090:
	_display_error_abort_to_dashboard (0000)
000D84A0 0090:
	_display_error_damaged_media (0000)
000D8530 0070:
	_display_errors_deferred_until_cinematic_stop (0000)
000D85A0 0260:
	_display_scenario_help (0000)
000D8800 02a0:
	_code_000d8800 (0000)
000D8AA0 0130:
	_code_000d8aa0 (0000)
000D8BD0 03c0:
	_process_ui_widgets (0000)
00271380 0012:
	??_C@_1BC@GEHLELKL@?$AAc?$AAu?$AAs?$AAt?$AAo?$AAm?$AA?9?$AA8?$AA?$AA@ (0000)
00271394 0012:
	??_C@_1BC@DMBIFLPN@?$AAc?$AAu?$AAs?$AAt?$AAo?$AAm?$AA?9?$AA7?$AA?$AA@ (0000)
002713A8 0012:
	??_C@_1BC@IEKEDMJI@?$AAc?$AAu?$AAs?$AAt?$AAo?$AAm?$AA?9?$AA6?$AA?$AA@ (0000)
002713BC 0012:
	??_C@_1BC@JGBBJDHG@?$AAc?$AAu?$AAs?$AAt?$AAo?$AAm?$AA?9?$AA5?$AA?$AA@ (0000)
002713D0 0012:
	??_C@_1BC@COKNPEBD@?$AAc?$AAu?$AAs?$AAt?$AAo?$AAm?$AA?9?$AA4?$AA?$AA@ (0000)
002713E4 0012:
	??_C@_1BC@LDHKMMKK@?$AAc?$AAu?$AAs?$AAt?$AAo?$AAm?$AA?9?$AA3?$AA?$AA@ (0000)
002713F8 0012:
	??_C@_1BC@LMGKLMP@?$AAc?$AAu?$AAs?$AAt?$AAo?$AAm?$AA?9?$AA2?$AA?$AA@ (0000)
0027140C 0012:
	??_C@_1BC@BJHDAECB@?$AAc?$AAu?$AAs?$AAt?$AAo?$AAm?$AA?9?$AA1?$AA?$AA@ (0000)
00271420 000a:
	??_C@_19MLOAAMON@?$AAl?$AAo?$AAo?$AAk?$AA?$AA@ (0000)
0027142C 000a:
	??_C@_19OACEEDLD@?$AAm?$AAo?$AAv?$AAe?$AA?$AA@ (0000)
00271438 000a:
	??_C@_19LHGBOOBH@?$AAb?$AAa?$AAc?$AAk?$AA?$AA@ (0000)
00271444 000e:
	??_C@_1O@CNPOHNDJ@?$AAa?$AAc?$AAc?$AAe?$AAp?$AAt?$AA?$AA@ (0000)
00271454 000a:
	??_C@_19MEDLKCJO@?$AAz?$AAo?$AAo?$AAm?$AA?$AA@ (0000)
00271460 000e:
	??_C@_1O@KCCHFBLA@?$AAc?$AAr?$AAo?$AAu?$AAc?$AAh?$AA?$AA@ (0000)
00271470 0020:
	??_C@_1CA@KFDCIJKK@?$AAr?$AAo?$AAt?$AAa?$AAt?$AAe?$AA?9?$AAg?$AAr?$AAe?$AAn?$AAa?$AAd?$AAe?$AAs?$AA?$AA@ (0000)
00271490 001e:
	??_C@_1BO@POELNCJG@?$AAr?$AAo?$AAt?$AAa?$AAt?$AAe?$AA?9?$AAw?$AAe?$AAa?$AAp?$AAo?$AAn?$AAs?$AA?$AA@ (0000)
002714B0 001c:
	??_C@_1BM@IKPPIMCO@?$AAu?$AAs?$AAe?$AA?9?$AAe?$AAq?$AAu?$AAi?$AAp?$AAm?$AAe?$AAn?$AAt?$AA?$AA@ (0000)
002714CC 000a:
	??_C@_19OBFFCANM@?$AAj?$AAu?$AAm?$AAp?$AA?$AA@ (0000)
002714D8 0022:
	??_C@_1CC@OGCINJG@?$AAi?$AAn?$AAt?$AAe?$AAg?$AAr?$AAa?$AAt?$AAe?$AAd?$AA?9?$AAl?$AAi?$AAg?$AAh?$AAt?$AA?$AA@ (0000)
002714FC 0020:
	??_C@_1CA@BEHOBGA@?$AAp?$AAr?$AAi?$AAm?$AAa?$AAr?$AAy?$AA?9?$AAt?$AAr?$AAi?$AAg?$AAg?$AAe?$AAr?$AA?$AA@ (0000)
0027151C 001c:
	??_C@_1BM@HECBOACM@?$AAt?$AAh?$AAr?$AAo?$AAw?$AA?9?$AAg?$AAr?$AAe?$AAn?$AAa?$AAd?$AAe?$AA?$AA@ (0000)
00271538 000e:
	??_C@_1O@HCAHPFFC@?$AAa?$AAc?$AAt?$AAi?$AAo?$AAn?$AA?$AA@ (0000)
00271548 0018:
	??_C@_1BI@IAFIHGKJ@?$AAr?$AAi?$AAg?$AAh?$AAt?$AA?9?$AAs?$AAt?$AAi?$AAc?$AAk?$AA?$AA@ (0000)
00271560 0016:
	??_C@_1BG@GNIACJFB@?$AAl?$AAe?$AAf?$AAt?$AA?9?$AAs?$AAt?$AAi?$AAc?$AAk?$AA?$AA@ (0000)
00271578 0018:
	??_C@_1BI@KFEEPENO@?$AAr?$AAi?$AAg?$AAh?$AAt?$AA?9?$AAt?$AAh?$AAu?$AAm?$AAb?$AA?$AA@ (0000)
00271590 0016:
	??_C@_1BG@EIJMKLCG@?$AAl?$AAe?$AAf?$AAt?$AA?9?$AAt?$AAh?$AAu?$AAm?$AAb?$AA?$AA@ (0000)
002715A8 0018:
	??_C@_1BI@KIILCNEB@?$AAb?$AAa?$AAc?$AAk?$AA?9?$AAb?$AAu?$AAt?$AAt?$AAo?$AAn?$AA?$AA@ (0000)
002715C0 001a:
	??_C@_1BK@CFIHHNDP@?$AAs?$AAt?$AAa?$AAr?$AAt?$AA?9?$AAb?$AAu?$AAt?$AAt?$AAo?$AAn?$AA?$AA@ (0000)
002715DC 0016:
	??_C@_1BG@CGEEKDJO@?$AAd?$AAp?$AAa?$AAd?$AA?9?$AAr?$AAi?$AAg?$AAh?$AAt?$AA?$AA@ (0000)
002715F4 0014:
	??_C@_1BE@LFENLMOB@?$AAd?$AAp?$AAa?$AAd?$AA?9?$AAl?$AAe?$AAf?$AAt?$AA?$AA@ (0000)
00271608 0014:
	??_C@_1BE@MAHEIGBI@?$AAd?$AAp?$AAa?$AAd?$AA?9?$AAd?$AAo?$AAw?$AAn?$AA?$AA@ (0000)
0027161C 0010:
	??_C@_1BA@CNADAJOL@?$AAd?$AAp?$AAa?$AAd?$AA?9?$AAu?$AAp?$AA?$AA@ (0000)
0027162C 001c:
	??_C@_1BM@DNLFOGBJ@?$AAr?$AAi?$AAg?$AAh?$AAt?$AA?9?$AAt?$AAr?$AAi?$AAg?$AAg?$AAe?$AAr?$AA?$AA@ (0000)
00271648 001a:
	??_C@_1BK@IONNGAAO@?$AAl?$AAe?$AAf?$AAt?$AA?9?$AAt?$AAr?$AAi?$AAg?$AAg?$AAe?$AAr?$AA?$AA@ (0000)
00271664 001a:
	??_C@_1BK@DHOCGHBB@?$AAw?$AAh?$AAi?$AAt?$AAe?$AA?9?$AAb?$AAu?$AAt?$AAt?$AAo?$AAn?$AA?$AA@ (0000)
00271680 001a:
	??_C@_1BK@JINDKLEA@?$AAb?$AAl?$AAa?$AAc?$AAk?$AA?9?$AAb?$AAu?$AAt?$AAt?$AAo?$AAn?$AA?$AA@ (0000)
0027169C 0012:
	??_C@_1BC@JDEDLPGE@?$AAy?$AA?9?$AAb?$AAu?$AAt?$AAt?$AAo?$AAn?$AA?$AA@ (0000)
002716B0 0012:
	??_C@_1BC@HMBBAJIF@?$AAx?$AA?9?$AAb?$AAu?$AAt?$AAt?$AAo?$AAn?$AA?$AA@ (0000)
002716C4 0012:
	??_C@_1BC@EADHJABC@?$AAb?$AA?9?$AAb?$AAu?$AAt?$AAt?$AAo?$AAn?$AA?$AA@ (0000)
002716D8 0012:
	??_C@_1BC@KKLBENHA@?$AAa?$AA?9?$AAb?$AAu?$AAt?$AAt?$AAo?$AAn?$AA?$AA@ (0000)
002716EC 000f:
	??_C@_0P@OHNLNAJA@levels?2d40?2d40?$AA@ (0000)
002716FC 000f:
	??_C@_0P@IAIGJJBI@levels?2d20?2d20?$AA@ (0000)
0027170C 000f:
	??_C@_0P@HAMJOBDA@levels?2c40?2c40?$AA@ (0000)
0027171C 000f:
	??_C@_0P@BHJEKILI@levels?2c20?2c20?$AA@ (0000)
0027172C 000f:
	??_C@_0P@CEDKAMHM@levels?2c10?2c10?$AA@ (0000)
0027173C 000f:
	??_C@_0P@ENPIGML@levels?2b40?2b40?$AA@ (0000)
0027174C 000f:
	??_C@_0P@MEDHKOMA@levels?2b30?2b30?$AA@ (0000)
0027175C 000f:
	??_C@_0P@DPFAEPEF@levels?2a50?2a50?$AA@ (0000)
0027176C 000f:
	??_C@_0P@FIANAGMN@levels?2a30?2a30?$AA@ (0000)
0027177C 000f:
	??_C@_0P@MMBGMDIK@levels?2a10?2a10?$AA@ (0000)
0027178C 0013:
	??_C@_0BD@CKKOGMCD@widget_memory_pool?$AA@ (0000)
002717A0 0008:
	__one_over_255 (0000)
	_SECONDS_PER_MILLISECOND (0004)
002717A8 0025:
	??_C@_0CF@BFKKDAFO@c?3?2halo?2SOURCE?2interface?2ui_widg@ (0000)
002717D0 0007:
	??_C@_06KLKLDLLI@widget?$AA@ (0000)
002717D8 0024:
	??_C@_0CE@JONHIOKA@expected?5a?5valid?5local_player_in@ (0000)
002717FC 001b:
	??_C@_0BL@IGOIGKAA@widget_globals?4initialized?$AA@ (0000)
00271818 0004:
	__real@38026b7e (0000)
0027181C 0004:
	__real@43490000 (0000)
00271820 0004:
	__real@38150d23 (0000)
00271824 0004:
	__real@37e0c670 (0000)
00271828 0004:
	__real@439b8000 (0000)
0027182C 0004:
	__real@3806dd76 (0000)
00271830 000e:
	??_C@_0O@CJGPHCNG@the_main_menu?$AA@ (0000)
00271840 000e:
	??_C@_0O@LGOJGHAO@unknown?5level?$AA@ (0000)
00271850 0048:
	??_C@_0EI@DFLNAEDH@the?5old?5loading?5progress?5screen?5@ (0000)
00271898 0059:
	??_C@_0FJ@HLIPNJGE@there?5is?5already?5an?5error?5messag@ (0000)
002718F8 0051:
	??_C@_0FB@JEKLNNGC@there?5is?5already?5a?5deferred?5erro@ (0000)
0027194C 0036:
	??_C@_0DG@OLDDPPAF@?$CIindex?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIindex?$DMMAXIMUM_NUM@ (0000)
00271988 0047:
	??_C@_0EH@LLPKBLA@there?5is?5already?5a?5deferred?5dash@ (0000)
002719D0 001a:
	??_C@_0BK@LDNBEGIO@title?5music?5tag?5not?5found?$AA@ (0000)
002719EC 0019:
	??_C@_0BJ@DOPIPCPC@starting?5main?5menu?5music?$AA@ (0000)
00271A08 001a:
	??_C@_0BK@GFHNHPPO@sound?2music?2title1?2title1?$AA@ (0000)
00271A24 0019:
	??_C@_0BJ@OPCCAOJF@stopping?5main?5menu?5music?$AA@ (0000)
00271A40 0012:
	??_C@_0BC@MKLFKPBF@duration_ticks?$DO?$DN0?$AA@ (0000)
00271A54 0034:
	??_C@_0DE@HOKJOLHA@out?5of?5memory?$CB?5the?5UI?5screen?5his@ (0000)
00271A88 000c:
	??_C@_0M@EABMFPBO@top?5?$CG?$CG?5data?$AA@ (0000)
00271A94 0019:
	??_C@_0BJ@FOOJDKHD@tail_child?9?$DOnext?5?$DN?$DN?5NULL?$AA@ (0000)
00271AB0 0033:
	??_C@_0DD@MMOLGAOL@?$CIchild?9?$DOprevious?5?$DN?$DN?5NULL?$CJ?5?$CG?$CG?5?$CIch@ (0000)
00271AE4 0018:
	??_C@_0BI@LBDKLIFA@next?9?$DOprevious?5?$DN?$DN?5child?$AA@ (0000)
00271AFC 0018:
	??_C@_0BI@LBKIDJB@child?9?$DOprevious?5?$DN?$DN?5NULL?$AA@ (0000)
00271B18 0041:
	??_C@_0EB@BEJBEEHM@initial_indent?$DM0?5in?5render_state@ (0000)
00271B5C 001d:
	??_C@_0BN@EEMEPEOK@31?5?$DN?$DN?5get_icon_type?$CIL?$CClook?$CC?$CJ?$AA@ (0000)
00271B7C 0024:
	??_C@_0CE@DPIKLLNB@17?5?$DN?$DN?5get_icon_type?$CIL?$CCright?9stic@ (0000)
00271BA0 001d:
	??_C@_0BN@GOCJIAID@30?5?$DN?$DN?5get_icon_type?$CIL?$CCmove?$CC?$CJ?$AA@ (0000)
00271BC0 0023:
	??_C@_0CD@BJHDPKGJ@16?5?$DN?$DN?5get_icon_type?$CIL?$CCleft?9stick@ (0000)
00271BE8 0041:
	??_C@_0EB@PKGKIBFB@?$CD?$CD?$CDWARNING?3?5possible?5memory?5leak@ (0000)
00271C2C 0022:
	??_C@_0CC@MHEDEDIP@widget?5pause?5counter?5out?5of?5whac@ (0000)
00271C50 0025:
	??_C@_0CF@EFHCMBNF@event?5handler?5failed?5to?5spawn?5wi@ (0000)
00271C78 004b:
	??_C@_0EL@BONJAKAK@a?5?8widget?5deleted?8?5event?5handler@ (0000)
00271CC4 0025:
	??_C@_0CF@OIBDABJ@widget?5?$CG?$CG?5widget_globals?4initial@ (0000)
00271CEC 001a:
	??_C@_0BK@EFAHKEKA@sound?2sfx?2ui?2flag_failure?$AA@ (0000)
00271D08 0012:
	??_C@_0BC@LFDBPFAD@sound?2sfx?2ui?2back?$AA@ (0000)
00271D1C 0015:
	??_C@_0BF@FGMCPHMP@sound?2sfx?2ui?2forward?$AA@ (0000)
00271D34 0014:
	??_C@_0BE@EOGFPNHD@sound?2sfx?2ui?2cursor?$AA@ (0000)
00271D48 003a:
	??_C@_0DK@MJICNCDL@invalid?5controller?5index?5specifi@ (0000)
00271D88 005d:
	??_C@_0FN@LKLJLEJ@_list_items_generated_from_strin@ (0000)
00271DE8 0069:
	??_C@_0GJ@EGCLHEFN@no?5child?5widget?5references?5are?5n@ (0000)
00271E58 0063:
	??_C@_0GD@BFCMHBJK@_list_items_generated_from_strin@ (0000)
00271EBC 0026:
	??_C@_0CG@GCOFCMPN@icon_index?$DO?$DN0?5?$CG?$CG?5icon_index?$DMNUM_@ (0000)
00271EE4 0006:
	??_C@_05MAJJAKPI@FALSE?$AA@ (0000)
00271EEC 0004:
	??_C@_13EJFHHPOP@?$AA?$CF?$AA?$AA@ (0000)
00271EF0 0047:
	??_C@_0EH@BDEOCECI@failed?5to?5render?5text?5box?5widget@ (0000)
00271F38 0042:
	??_C@_0EC@OINFLBMA@failed?5to?5render?5text?5box?5widget@ (0000)
00271F7C 0020:
	??_C@_1CA@EBMNPDDP@?$AA?$DM?$AAo?$AAu?$AAt?$AA?5?$AAo?$AAf?$AA?5?$AAm?$AAe?$AAm?$AAo?$AAr?$AAy?$AA?$DO?$AA?$AA@ (0000)
00271FA0 0049:
	??_C@_0EJ@ICFFKBCK@failed?5to?5render?5spinner?5list?5it@ (0000)
00271FF0 0044:
	??_C@_0EE@GFHAOED@failed?5to?5render?5spinner?5list?5it@ (0000)
00272034 0029:
	??_C@_0CJ@CLCMGDOK@failed?5to?5find?5event?5focus?5targe@ (0000)
00272060 0046:
	??_C@_0EG@GAAKHNIE@failed?5to?5spawn?5thread?5for?5files@ (0000)
002720A8 003c:
	??_C@_0DM@DBODFMFA@begining?5filesystem?5checks?5?$CG?5sav@ (0000)
002720E4 002b:
	??_C@_0CL@DAIOFIBL@widget_globals?4initialization_th@ (0000)
00272110 003d:
	??_C@_0DN@CLOOAICL@failed?5to?5set?5focus?5to?5the?5next?5@ (0000)
00272150 006d:
	??_C@_0GN@CDIJNPDM@spinner?5lists?5with?5more?5that?51?5v@ (0000)
002721C0 0033:
	??_C@_0DD@MMCOEHPE@spinner?5lists?5must?5be?5either?51?9?5@ (0000)
002721F8 0041:
	??_C@_0EB@HAOLGJEB@failed?5to?5set?5focus?5to?5the?5?$CD?$CFd?5l@ (0000)
0027223C 0034:
	??_C@_0DE@PKDHGMBO@calling?5a?5list?5widget?5function?5o@ (0000)
00272270 0016:
	??_C@_0BG@ENJMIAKH@widget?9?$DOfocused_child?$AA@ (0000)
00272288 0048:
	??_C@_0EI@GEFPBJMP@couldn?8t?5load?5conditional?5widget@ (0000)
002722D0 0029:
	??_C@_0CJ@FPJLGLJO@condition?5handler?5failed?5to?5spaw@ (0000)
00272300 0045:
	??_C@_0EF@JLKENOEN@failed?5to?5open?5widget?5because?5th@ (0000)
00272348 0019:
	??_C@_0BJ@KIDHBPBC@previous?9?$DOnext?5?$DN?$DN?5widget?$AA@ (0000)
00272364 0019:
	??_C@_0BJ@FGBHJFEB@next?9?$DOprevious?5?$DN?$DN?5widget?$AA@ (0000)
00272380 0044:
	??_C@_0EE@NFLPKLKO@failed?5to?5close?5widget?5because?5e@ (0000)
002723C8 0045:
	??_C@_0EF@EEMBCNDP@failed?5to?5reload?5widget?5because?5@ (0000)
00272410 004e:
	??_C@_0EO@PGEAPKEN@failed?5to?5give?5focus?5to?5a?5widget@ (0000)
00272460 001e:
	??_C@_0BO@HFOFCHPP@event?5handler?5function?5failed?$AA@ (0000)
00272480 002a:
	??_C@_0CK@GOFLJEI@failed?5to?5run?5ui?5widget?5event?5sc@ (0000)
002724AC 000c:
	??_C@_0M@EJOMDMOI@ui?2small_ui?$AA@ (0000)
002724B8 0016:
	??_C@_0BG@ELECBMPG@window_bounds?5?$CB?$DN?5NULL?$AA@ (0000)
002724D0 001f:
	??_C@_0BP@JAOAINDB@failed?5to?5load?5widget?5children?$AA@ (0000)
002724F0 009a:
	??_C@_0JK@BLPAHHHK@if?5the?5_widget_pass_handled_even@ (0000)
0027258C 0021:
	??_C@_0CB@BADDKMAA@widget?9?$DOlocal_player_index?$DN?$DNNONE@ (0000)
002725B0 0037:
	??_C@_0DH@DCPBMEID@widget?5?$CG?$CG?5definition?5?$CG?$CG?5event?5?$CG?$CG@ (0000)
002725E8 002c:
	??_C@_0CM@OJPOGECC@ui_widget_definition?5tag?5?8?$CFs?8?1?$CFd@ (0000)
00272614 002c:
	??_C@_0CM@MEFFCNMB@failed?5to?5create?5new?5widget?$DL?5out@ (0000)
00272640 0035:
	??_C@_0DF@PBOMEHIM@?$CIwidget_stack?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIwidget_sta@ (0000)
00272678 0026:
	??_C@_0CG@GBPNNCLI@?$CIname?5?$CB?$DN?5NULL?$CJ?5?$HM?$HM?5?$CItag_index?5?$CB?$DN?5@ (0000)
002726A0 002a:
	??_C@_0CK@OFOHOPCB@failed?5to?5initialize?5the?5virtual@ (0000)
002726CC 0028:
	??_C@_0CI@EBHLJEJP@failed?5to?5load?5main?5screen?5shell@ (0000)
002726F4 001d:
	??_C@_0BN@PLHBGNEF@ui?2shell?2main_menu?2main_menu?$AA@ (0000)
00272714 0018:
	??_C@_0BI@HFFEIMKP@xbox?5command?5line?$DN?5?8?$CFs?8?$AA@ (0000)
0027272C 0006:
	??_C@_05GLGMCIFH@xdemo?$AA@ (0000)
00272734 002f:
	??_C@_0CP@FMNMOLIB@failed?5to?5load?5networked?5pregame@ (0000)
00272768 005b:
	??_C@_0FL@CANKNMEF@ui?2shell?2main_menu?2multiplayer_t@ (0000)
002727C4 002a:
	??_C@_0CK@BADOGGKF@failed?5to?5load?5map?5select?5postga@ (0000)
002727F0 0060:
	??_C@_0GA@GFIHBLPH@ui?2shell?2main_menu?2multiplayer_t@ (0000)
00272850 0035:
	??_C@_0DF@GJABDHAE@failed?5to?5load?5pregame?5screen?5af@ (0000)
00272888 0063:
	??_C@_0GD@HIMIFBNM@ui?2shell?2main_menu?2multiplayer_t@ (0000)
002728EC 0025:
	??_C@_0CF@GLGHHELH@widget?5pause?5counter?5is?5out?5of?5w@ (0000)
00272914 002f:
	??_C@_0CP@BGKPJLCC@expected?5a?5text?5box?5widget?5in?5th@ (0000)
00272944 0032:
	??_C@_0DC@JNAKMNPN@error?5screen?5widget?5tag?5not?5laye@ (0000)
00272978 0020:
	??_C@_0CA@PLEKCLAA@failed?5to?5display?5error?5message?$AA@ (0000)
00272998 0048:
	??_C@_0EI@MDMAJGJE@there?5is?5already?5an?5error?5messag@ (0000)
002729E0 0032:
	??_C@_0DC@JLPDPOMG@aborting?5to?5the?5main?5menu?5root?0?5@ (0000)
00272A18 0044:
	??_C@_0EE@HGHDGOBG@?$CIwidget_stack?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIwidget_sta@ (0000)
00272A5C 001b:
	??_C@_0BL@ILOCFJOB@invalid?5local?5player?5count?$AA@ (0000)
00272A78 0028:
	??_C@_0CI@NLLLFMIK@ui?2shell?2error?2error_nonmodal_qt@ (0000)
00272AA0 0025:
	??_C@_0CF@IPCJKMOA@ui?2shell?2error?2error_modal_qtrsc@ (0000)
00272AC8 0029:
	??_C@_0CJ@MDGMGIIG@ui?2shell?2error?2error_nonmodal_ha@ (0000)
00272AF4 0026:
	??_C@_0CG@GOFPHCDA@ui?2shell?2error?2error_modal_halfs@ (0000)
00272B1C 0029:
	??_C@_0CJ@COJPHONK@ui?2shell?2error?2error_nonmodal_fu@ (0000)
00272B48 0026:
	??_C@_0CG@IDKMGEGM@ui?2shell?2error?2error_modal_fulls@ (0000)
00272B70 0059:
	??_C@_0FJ@LCJINDPJ@there?5is?5already?5a?5deferred?9for?9@ (0000)
00272BCC 001b:
	??_C@_0BL@DHJKOIEB@failed?5to?5load?5?8?$CFs?8?5widget?$AA@ (0000)
00272BE8 001b:
	??_C@_0BL@PIJOAKFD@expected?5a?5text?5box?5widget?$AA@ (0000)
00272C04 003b:
	??_C@_0DL@MFGPNPO@ui?2shell?2error?2error_abort_to_da@ (0000)
00272C40 0028:
	??_C@_0CI@JFEBNJBP@ui?2shell?2error?2error_abort_to_da@ (0000)
00272C68 0017:
	??_C@_0BH@FIKKMDKG@Noooooooooooooooooo?$CB?$CB?$CB?$AA@ (0000)
00272C80 003a:
	??_C@_0DK@KGMPEFEN@can?8t?5display?5scenario?5help?5beca@ (0000)
00272CBC 003d:
	??_C@_0DN@BOLHGMPP@can?8t?5display?5scenario?5help?$DL?5unk@ (0000)
00272CFC 0023:
	??_C@_0CD@DGCLPNFD@failed?5to?5load?5in?9game?5help?5dial@ (0000)
00272D20 002f:
	??_C@_0CP@ODEPBIAI@expected?5text?5box?5widget?5in?5play@ (0000)
00272D50 0036:
	??_C@_0DG@EMGDHLEG@ui?2shell?2solo_game?2player_help?2p@ (0000)
00272D88 0004:
	??_C@_03JGNLDLAI@d40?$AA@ (0000)
00272D8C 0036:
	??_C@_0DG@EIOOAHPE@ui?2shell?2solo_game?2player_help?2p@ (0000)
00272DC4 0004:
	??_C@_03JCFGEHLK@d20?$AA@ (0000)
00272DC8 0036:
	??_C@_0DG@NBLEEDPP@ui?2shell?2solo_game?2player_help?2p@ (0000)
00272E00 0004:
	??_C@_03LAMADLB@c40?$AA@ (0000)
00272E04 0036:
	??_C@_0DG@NFDJDPEN@ui?2shell?2solo_game?2player_help?2p@ (0000)
00272E3C 0004:
	??_C@_03PIBHPAD@c20?$AA@ (0000)
00272E40 0036:
	??_C@_0DG@NHHPIBBE@ui?2shell?2solo_game?2player_help?2p@ (0000)
00272E78 0004:
	??_C@_03NMHMBFK@c10?$AA@ (0000)
00272E7C 0036:
	??_C@_0DG@GJAICEJK@ui?2shell?2solo_game?2player_help?2p@ (0000)
00272EB4 0004:
	??_C@_03LDLAGENE@b40?$AA@ (0000)
00272EB8 0036:
	??_C@_0DG@GMEHDCBP@ui?2shell?2solo_game?2player_help?2p@ (0000)
00272EF0 0004:
	??_C@_03LGPPHCFB@b30?$AA@ (0000)
00272EF4 0036:
	??_C@_0DG@HKHPOBED@ui?2shell?2solo_game?2player_help?2p@ (0000)
00272F2C 0004:
	??_C@_03KAMHKBAN@a50?$AA@ (0000)
00272F30 0036:
	??_C@_0DG@HOPCJNPB@ui?2shell?2solo_game?2player_help?2p@ (0000)
00272F68 0004:
	??_C@_03KEEKNNLP@a30?$AA@ (0000)
00272F6C 0036:
	??_C@_0DG@HNHGEJJP@ui?2shell?2solo_game?2player_help?2p@ (0000)
00272FA4 0004:
	??_C@_03KHMOAJNB@a10?$AA@ (0000)
00272FA8 0010:
	??_C@_0BA@CDPMFNHN@string_index?$DO?$DN0?$AA@ (0000)
00272FB8 002d:
	??_C@_0CN@FAHAPHCM@failed?5to?5load?5full?5screen?5pause@ (0000)
00272FE8 0029:
	??_C@_0CJ@OJHKNGAB@ui?2shell?2solo_game?2pause_game?2pa@ (0000)
00273018 0053:
	??_C@_0FD@MJAJHNPK@the?5ui?5seems?5to?5be?5confused?4?4?4?5a@ (0000)
0027306C 002e:
	??_C@_0CO@LCDPHEGN@failed?5to?5load?5split?5screen?5paus@ (0000)
0027309C 0036:
	??_C@_0DG@NEJLJPFI@ui?2shell?2solo_game?2pause_game?2pa@ (0000)
002730D4 0030:
	??_C@_0DA@JLINLCDA@invalid?5local?5player?5count?5for?5m@ (0000)
00273104 0033:
	??_C@_0DD@CLAJLOAB@ui?2shell?2multiplayer_game?2pause_@ (0000)
00273138 0033:
	??_C@_0DD@NDMLPFLH@ui?2shell?2multiplayer_game?2pause_@ (0000)
0027316C 002d:
	??_C@_0CN@BIDFDGIG@failed?5to?5load?5multiplayer?5pause@ (0000)
0027319C 0033:
	??_C@_0DD@KPKKNAGM@ui?2shell?2multiplayer_game?2pause_@ (0000)
002731D0 002a:
	??_C@_0CK@MAHDJCLA@invalid?5widget?5controller?5index?5@ (0000)
002731FC 0037:
	??_C@_0DH@EDIJCICF@waiting?5for?5?$CFd?5ticks?5before?5disp@ (0000)
00301DA0 413c:
	_data_00301da0 (0000)
	_widget_memory_pool (4034)
	_dashboard_abort_error (403c)
00454240 08b0:
	_bss_00454240 (0000)
*/

/* ---------- headers */

struct widget_instance;

#include "cseries.h"
#include "errors.h"
#include "bitmaps/bitmap_group.h"
#include "bink/bink_playback.h"
#include "bungie_net/common/thread.h"
#include "cseries/cseries_windows.h"
#include "cutscene/cinematics.h"
#include "event_manager.h"
#include "game/players.h"
#include "input/input.h"
#include "input/input_abstraction.h"
#include "interface/attract_mode.h"
#include "interface/interface.h"
#include "interface/player_ui.h"
#include "interface/ui_widget_definitions.h"
#include "interface/ui_widget_event_handler_functions.h"
#include "interface/virtual_keyboard.h"
#include "main/main.h"
#include "main/main_runtime.h"
#include "memory/stack_memory_pool.h"
#include "networking/network_connection.h"
#include "networking/network_game_globals.h"
#include "networking/network_server_manager.h"
#include "rasterizer/rasterizer.h"
#include "saved games/player_profile.h"
#include "saved games/playlist_profile.h"
#include "saved games/saved_game_files.h"
#include "shell/shell_xbox.h"
#include "sound/game_sound.h"
#include "sound/sound_definitions.h"
#include "sound/sound_manager.h"
#include "ui_widget.h"

/* ---------- constants */

enum
{
	WIDGET_MEMORY_POOL_SIZE = 0x4000
};

enum
{
	_ui_widget_type_container,
	_ui_widget_type_text_box,
	_ui_widget_type_spinner_list,
	_ui_widget_type_column_list,
	_ui_widget_type_game_model,
	_ui_widget_type_movie,
	_ui_widget_type_custom,
	NUMBER_OF_UI_WIDGET_TYPES
};

enum
{
	_ui_audio_feedback_none,
	_ui_audio_feedback_cursor,
	_ui_audio_feedback_forward,
	_ui_audio_feedback_back,
	_ui_audio_feedback_flag_failure,
	NUMBER_OF_UI_AUDIO_FEEDBACK_SOUNDS
};

enum
{
	_error_unknown,
	_error_network_generic,
	_error_network_join_game_closed,
	_error_network_join_game_generic,
	_error_network_server_shut_down,
	_error_network_connected_play_no_network,
	_error_network_connection_lost,
	_error_network_failed_to_join_game,
	_error_network_out_of_sync_alert,
	_error_network_trouble_is_brewing,
	_error_network_unused10,
	_error_controller_generic,
	_error_controller_unplugged_start_to_continue,
	_error_controller_unplugged,
	_error_controller_memory_card,
	_error_controller_saving_file_warning,
	_error_controller_creating_player_profile_warning,
	_error_controller_creating_game_settings_file_warning,
	_error_controller_coop_controller_conflict,
	_error_controller_coop_requires_two_controllers,
	_error_controller_unused10,
	_error_insert_quarter,
	_error_warning_revert_to_saved_progress_will_be_lost,
	_error_warning_restart_level_progress_will_be_lost,
	_error_warning_delete_player_profile,
	_error_warning_delete_multiplayer_profile,
	_error_cannot_delete_default_game_settings,
	_error_already_a_saved_game_file_with_that_name,
	_error_warning_are_you_sure_you_want_to_discard_changes,
	_error_cannot_create_saved_game_file_with_empty_name,
	_error_warning_saving_checkpoint,
	_error_warning_saved_game_file_damaged,
	_error_warning_unused10,
	_error_hard_drive_not_enough_free_space,
	_error_hard_drive_maximum_saved_game_files,
	_error_media_damaged,
	_error_maximum_game_files_created,
	_error_unable_to_create_player_profile,
	_error_unable_to_create_multiplayer_game_file,
	_error_saved_game_state_corrupt,
	NUMBER_OF_ERROR_CODES
};

enum
{
	_icon_a_button,
	_icon_b_button,
	_icon_x_button,
	_icon_y_button,
	_icon_black_button,
	_icon_white_button,
	_icon_left_trigger,
	_icon_right_trigger,
	_icon_dpad_up,
	_icon_dpad_down,
	_icon_dpad_left,
	_icon_dpad_right,
	_icon_start_button,
	_icon_back_button,
	_icon_left_thumb,
	_icon_right_thumb,
	_icon_left_stick,
	_icon_right_stick,
	_icon_action,
	_icon_throw_grenade,
	_icon_primary_trigger,
	_icon_integrated_light,
	_icon_jump,
	_icon_use_equipment,
	_icon_rotate_weapons,
	_icon_rotate_grenades,
	_icon_crouch,
	_icon_zoom,
	_icon_accept,
	_icon_back,
	_icon_move,
	_icon_look,
	_icon_custom_1,
	_icon_custom_2,
	_icon_custom_3,
	_icon_custom_4,
	_icon_custom_5,
	_icon_custom_6,
	_icon_custom_7,
	_icon_custom_8,
	NUMBER_OF_ICON_TYPES
};

enum
{
	_joystick_preset_standard,
	_joystick_preset_south_paw,
	_joystick_preset_legacy,
	_joystick_preset_legacy_south_paw,
	NUMBER_OF_JOYSTICK_PRESETS
};

/* ---------- macros */

/* ---------- structures */

struct stack_memory_pool_block;

struct stack_memory_pool
{
	char const *name;
	byte *base_address;
	long size;
	long maximum_block_count;
	long next_block_index;
	long bytes_used;
	long maximum_bytes_used;
	unsigned long block_count;
	unsigned long maximum_block_count_used;
	long largest_block_size;
	boolean disable_compaction;
	byte unused29[3];
	struct stack_memory_pool_block *first_block;
	struct stack_memory_pool_block *last_block;
	struct stack_memory_pool_block *blocks[1];
};

struct ui_widget_deferred_error
{
	short error_code;
	short local_player_index;
	boolean modal;
	boolean pause_game_time;
};

struct ui_widget_deferred_cinematic_error
{
	short error_code;
	boolean modal;
	boolean pause_game_time;
};

struct widget_stack_data
{
	long previous_widget_tag;
	long focused_child_parent_widget_tag;
	short focused_child_index;
	short local_player_index;
};

struct widget_stack_node
{
	struct widget_stack_data data;
	struct widget_stack_node *next;
};

struct ui_widget_runtime_globals_prefix
{
	struct widget_instance *active_widgets[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	struct widget_stack_node *widget_stack[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	long current_system_milliseconds;
	long pause_disabled_ticks;
	short main_menu_deferred_error_code;
	short pause_game_time_count;
	real fade_to_black;
	struct ui_widget_deferred_error deferred_errors[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	short deferred_dashboard_error_code;
	boolean deferred_dashboard_optional;
	byte reserved004B;
	struct ui_widget_deferred_cinematic_error deferred_cinematic_errors[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	struct thread_reference *initialization_thread;
	short filesystem_check_result;
	boolean initialized;
	boolean dont_load_children_recursive;
	boolean debug_show_path;
	boolean processing_inhibited;
	boolean main_menu_music_active;
	boolean sound_paused;
};

struct ui_widget_bss_prefix
{
	byte reserved0000[0x800];
	struct ui_widget_runtime_globals_prefix widget_globals;
	boolean we_are_at_the_main_menu;
};

typedef char verify_ui_widget_fade_to_black_offset[
	offsetof(
		struct ui_widget_runtime_globals_prefix,
		fade_to_black) == 0x2C ? 1 : -1];
typedef char verify_ui_widget_pause_disabled_ticks_offset[
	offsetof(
		struct ui_widget_runtime_globals_prefix,
		pause_disabled_ticks) == 0x24 ? 1 : -1];
typedef char verify_ui_widget_main_menu_deferred_error_code_offset[
	offsetof(
		struct ui_widget_runtime_globals_prefix,
		main_menu_deferred_error_code) == 0x28 ? 1 : -1];
typedef char verify_ui_widget_deferred_dashboard_error_code_offset[
	offsetof(
		struct ui_widget_runtime_globals_prefix,
		deferred_dashboard_error_code) == 0x48 ? 1 : -1];
typedef char verify_ui_widget_deferred_dashboard_optional_offset[
	offsetof(
		struct ui_widget_runtime_globals_prefix,
		deferred_dashboard_optional) == 0x4A ? 1 : -1];
typedef char verify_ui_widget_initialization_thread_offset[
	offsetof(
		struct ui_widget_runtime_globals_prefix,
		initialization_thread) == 0x5C ? 1 : -1];
typedef char verify_ui_widget_initialized_offset[
	offsetof(
		struct ui_widget_runtime_globals_prefix,
		initialized) == 0x62 ? 1 : -1];
typedef char verify_ui_widget_debug_show_path_offset[
	offsetof(
		struct ui_widget_runtime_globals_prefix,
		debug_show_path) == 0x64 ? 1 : -1];
typedef char verify_ui_widget_processing_inhibited_offset[
	offsetof(
		struct ui_widget_runtime_globals_prefix,
		processing_inhibited) == 0x65 ? 1 : -1];
typedef char verify_ui_widget_main_menu_music_active_offset[
	offsetof(
		struct ui_widget_runtime_globals_prefix,
		main_menu_music_active) == 0x66 ? 1 : -1];
typedef char verify_ui_widget_runtime_globals_prefix_size[
	sizeof(struct ui_widget_runtime_globals_prefix) == 0x68 ? 1 : -1];
typedef char verify_ui_widget_globals_offset[
	offsetof(
		struct ui_widget_bss_prefix,
		widget_globals) == 0x800 ? 1 : -1];
typedef char verify_ui_widget_main_menu_active_offset[
	offsetof(
		struct ui_widget_bss_prefix,
		we_are_at_the_main_menu) == 0x868 ? 1 : -1];

struct widget_animation_data
{
	short current_frame_index;
	short first_frame_index;
	short last_frame_index;
	short number_of_sprite_frames;
};

struct widget_instance
{
	long definition_tag_index;
	char const *name;
	short local_player_index;
	short horizontal_offset;
	short vertical_offset;
	short type;
	boolean visible;
	boolean render_regardless_of_controller_index;
	boolean disabled;
	boolean pause_game_time;
	boolean delete_recursion_lock;
	boolean widget_is_error_dialog;
	boolean close_if_local_player_controller_present;
	byte pad17;
	long creation_time;
	long milliseconds_to_auto_close;
	long auto_close_fade_time;
	real alpha_modifier;
	struct widget_instance *previous;
	struct widget_instance *next;
	struct widget_instance *parent;
	struct widget_instance *child;
	struct widget_instance *focused_child;
	union
	{
		struct
		{
			wchar_t *text;
			short string_list_index;
		} text_box;
		struct
		{
			short selected_index;
			short top_index;
			void *list_items;
			short number_of_items;
			struct widget_instance *extended_description;
			wchar_t *item_text;
		} list;
	} parameters;
	struct widget_animation_data animation;
};

typedef char verify_widget_instance_size[
	sizeof(struct widget_instance) == 0x58 ? 1 : -1];
typedef char verify_widget_instance_animation_offset[
	offsetof(struct widget_instance, animation) == 0x50 ? 1 : -1];

typedef char verify_widget_instance_creation_time_offset[
	offsetof(struct widget_instance, creation_time) == 0x18 ? 1 : -1];
typedef char verify_widget_instance_alpha_modifier_offset[
	offsetof(struct widget_instance, alpha_modifier) == 0x24 ? 1 : -1];
typedef char verify_widget_instance_next_offset[
	offsetof(struct widget_instance, next) == 0x2C ? 1 : -1];
typedef char verify_widget_instance_focused_child_offset[
	offsetof(struct widget_instance, focused_child) == 0x38 ? 1 : -1];
typedef char verify_widget_instance_text_box_string_list_index_offset[
	offsetof(struct widget_instance, parameters.text_box.string_list_index) == 0x40 ? 1 : -1];

/* ---------- prototypes */

static boolean transition_to_game_in_progress(
	void);
static __inline real compute_offset_coordinate(
	long time,
	real delta_per_second);
static short get_icon_type(
	wchar_t const *string);
static boolean should_flip_sticks_for_local_player(
	short local_player_index);
static unsigned long __stdcall filesystem_initialization_thread_proc(
	void *input);
static void perform_filesystem_initialization(
	void);

/* ---------- globals */

extern struct stack_memory_pool *widget_memory_pool;
extern struct ui_widget_bss_prefix ui_widget_globals_storage;

#define widget_globals ui_widget_globals_storage.widget_globals
#define we_are_at_the_main_menu ui_widget_globals_storage.we_are_at_the_main_menu
extern real_argb_color ui_plasma_effect_color;
extern real global_ui_white_red;
extern real global_ui_white_green;
extern real global_ui_white_blue;

short dashboard_abort_error = NONE;

static boolean main_screen_shell_first_load = TRUE;

static wchar_t const *icon_names[NUMBER_OF_ICON_TYPES] =
{
	L"a-button",
	L"b-button",
	L"x-button",
	L"y-button",
	L"black-button",
	L"white-button",
	L"left-trigger",
	L"right-trigger",
	L"dpad-up",
	L"dpad-down",
	L"dpad-left",
	L"dpad-right",
	L"start-button",
	L"back-button",
	L"left-thumb",
	L"right-thumb",
	L"left-stick",
	L"right-stick",
	L"action",
	L"throw-grenade",
	L"primary-trigger",
	L"integrated-light",
	L"jump",
	L"use-equipment",
	L"rotate-weapons",
	L"rotate-grenades",
	L"crouch",
	L"zoom",
	L"accept",
	L"back",
	L"move",
	L"look",
	L"custom-1",
	L"custom-2",
	L"custom-3",
	L"custom-4",
	L"custom-5",
	L"custom-6",
	L"custom-7",
	L"custom-8"
};

#define NUM_ICONS (sizeof(icon_names)/sizeof(icon_names[0]))


/* ---------- public code */

void set_ui_plasma_effect_color(
	real alpha,
	real red,
	real green,
	real blue)
{
	ui_plasma_effect_color.alpha = alpha;
	ui_plasma_effect_color.red = red;
	ui_plasma_effect_color.green = green;
	ui_plasma_effect_color.blue = blue;

	return;
}

boolean event_controller_index_compatible_with_widget(
	struct event_record const *event,
	struct widget_instance const *widget)
{
	short widget_controller_index;

	widget_controller_index = widget->local_player_index;
	return widget_controller_index == NONE ||
		widget_controller_index == event->controller_index;
}

void ui_widgets_safe_to_load(
	boolean safe)
{
	return;
}

void ui_widgets_inhibit_processing(
	boolean inhibit)
{
	match_assert(
		"c:\\halo\\SOURCE\\interface\\ui_widget.c",
		1174,
		widget_globals.initialized);
	widget_globals.processing_inhibited = inhibit;

	return;
}

static __inline real compute_offset_coordinate(
	long time,
	real delta_per_second)
{
	real scaled_delta = delta_per_second * 0.001f;

	return (real)fmod(
		scaled_delta * time,
		1.0);
}

void draw_bitmap_in_rect(
	struct bitmap_data *bitmap,
	rectangle2d *rect,
	rectangle2d *bitmap_rect,
	rectangle2d *clip_rect,
	pixel32 argb,
	struct rasterizer_dynamic_screen_geometry_parameters *multitexture_params,
	boolean no_plasma)
{
	if (bitmap && rect)
	{
		real_argb_color plasma_fade = ui_plasma_effect_color;
		real_rgb_color map_tint = { 0.9f, 0.9f, 0.9f };
		real map_fade = 0.9f;
		rectangle2d default_bitmap_rect;
		real_point2d positions[NUMBER_OF_POINTS_PER_RECTANGLE];
		struct dynamic_screen_vertex vertices[NUMBER_OF_POINTS_PER_RECTANGLE];
		struct rasterizer_dynamic_screen_geometry_parameters parameters;
		real bitmap_width;
		real bitmap_height;
		real texture_width;
		real texture_height;
		real_point2d map_offsets[2];
		short rectangle_x0;
		short rectangle_y0;
		short rectangle_width;
		short rectangle_height;
		short source_width;
		short source_height;
		short vertex_index;

		if (!bitmap_rect)
		{
			default_bitmap_rect.y0 = 0;
			default_bitmap_rect.x0 = 0;
			default_bitmap_rect.y1 = bitmap->height;
			default_bitmap_rect.x1 = bitmap->width;
			bitmap_rect = &default_bitmap_rect;
		}

		rectangle_x0 = rect->x0;
		rectangle_y0 = rect->y0;
		rectangle_width = rect->x1 - rect->x0;
		rectangle_height = rect->y1 - rect->y0;
		source_width = bitmap_rect->x1 - bitmap_rect->x0;
		source_height = bitmap_rect->y1 - bitmap_rect->y0;
		positions[0].x = (real)rectangle_x0;
		positions[0].y = (real)rectangle_y0;
		positions[1].x = (real)(rectangle_x0 + rectangle_width);
		positions[1].y = (real)rectangle_y0;
		positions[2].x = (real)(rectangle_x0 + rectangle_width);
		positions[2].y = (real)(rectangle_y0 + rectangle_height);
		positions[3].x = (real)rectangle_x0;
		positions[3].y = (real)(rectangle_y0 + rectangle_height);

		if (clip_rect)
		{
			if (clip_rect->x0 > rect->x0)
			{
				positions[0].x = (real)clip_rect->x0;
				positions[3].x = (real)clip_rect->x0;
			}
			if (clip_rect->x1 < rect->x1)
			{
				positions[1].x = (real)clip_rect->x1;
				positions[2].x = (real)clip_rect->x1;
			}
			if (clip_rect->y0 > rect->y0)
			{
				positions[0].y = (real)clip_rect->y0;
				positions[1].y = (real)clip_rect->y0;
			}
			if (clip_rect->y1 < rect->y1)
			{
				positions[2].y = (real)clip_rect->y1;
				positions[3].y = (real)clip_rect->y1;
			}
		}

		bitmap_width = MAX(1.0f, (real)bitmap->width);
		texture_width = MIN(
			(real)source_width / bitmap_width,
			1.0f);
		bitmap_height = MAX(1.0f, (real)bitmap->height);
		texture_height = MIN(
			(real)source_height / bitmap_height,
			1.0f);

		for (vertex_index = 0;
			vertex_index < NUMBER_OF_POINTS_PER_RECTANGLE;
			vertex_index++)
		{
			vertices[vertex_index].position = positions[vertex_index];
			vertices[vertex_index].texture_coordinates.x =
				(vertex_index % 3) ? texture_width : 0.0f;
			vertices[vertex_index].texture_coordinates.y =
				(vertex_index > 1) ? texture_height : 0.0f;
			vertices[vertex_index].color = argb;
		}

		csmemset(&parameters, 0, sizeof(parameters));
		if (no_plasma)
		{
			parameters.map[0] = bitmap;
			parameters.map_scale[0].i = 1.0f;
			parameters.map_scale[0].j = 1.0f;
			parameters.map_texture_scale[0].i = 1.0f;
			parameters.map_texture_scale[0].j = 1.0f;
		}
		else
		{
			struct bitmap_data *plasma_bitmap = TAG_BLOCK_GET_ELEMENT(
				&bitmap_group_get(
					interface_get_tag_index(_interface_bitmap_iface_map3))->bitmap_data,
				0,
				struct bitmap_data);
			long time = system_milliseconds();

			map_offsets[0].x =
				compute_offset_coordinate(time, 0.03215434f) * 311.0f;
			map_offsets[0].y =
				compute_offset_coordinate(time, 0.026795285f) * 311.0f;
			map_offsets[1].x =
				-compute_offset_coordinate(time, 0.035536603f);
			map_offsets[1].x *= 201.0f;
			map_offsets[1].y =
				-compute_offset_coordinate(time, 0.031094525f);
			map_offsets[1].y *= 201.0f;

			parameters.map[0] = plasma_bitmap;
			parameters.map[1] = plasma_bitmap;
			parameters.map[2] = bitmap;
			parameters.map_anchor_screen[0] = TRUE;
			parameters.map_anchor_screen[1] = TRUE;
			parameters.map_wrapped[0] = TRUE;
			parameters.map_wrapped[1] = TRUE;
			parameters.map_offset[0] = &map_offsets[0];
			parameters.map_offset[1] = &map_offsets[1];
			parameters.map_scale[0].i = 1.0f;
			parameters.map_scale[0].j = 1.0f;
			parameters.map_scale[1].i = 1.0f;
			parameters.map_scale[1].j = 1.0f;
			parameters.map_scale[2].i = 1.0f;
			parameters.map_scale[2].j = 1.0f;
			parameters.map_texture_scale[0].i = 1.0f / 311.0f;
			parameters.map_texture_scale[0].j = 1.0f / 311.0f;
			parameters.map_texture_scale[1].i = 1.0f / 201.0f;
			parameters.map_texture_scale[1].j = 1.0f / 201.0f;
			parameters.map_texture_scale[2].i = 1.0f;
			parameters.map_texture_scale[2].j = 1.0f;
			parameters.map_tint[0] = &map_tint;
			parameters.map_tint[1] = &map_tint;
			parameters.plasma_fade = plasma_fade;
			parameters.doing_plasma_effect = TRUE;
			parameters.map_fade[0] = &map_fade;
			parameters.map_fade[1] = &map_fade;
			parameters.map0_to_1_blend_function = 5;
		}

		parameters.framebuffer_blend_function = 0;
		parameters.point_sampled = FALSE;
		rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);
	}

	return;
}

void ui_widgets_set_fade_value(
	real value)
{
	widget_globals.fade_to_black = value;

	return;
}

void ui_widget_debug_show_path(
	boolean show)
{
	widget_globals.debug_show_path = show;

	return;
}

int widget_instance_count_children(
	struct widget_instance *widget)
{
	int count;
	struct widget_instance *child;

	count = 0;
	if (widget)
	{
		for (child = widget->child; child; child = child->next)
			count++;
	}
	return count;
}

struct widget_instance *widget_instance_get_nth_child(
	struct widget_instance *widget,
	int n)
{
	int i;
	struct widget_instance *result;

	match_assert(
		"c:\\halo\\SOURCE\\interface\\ui_widget.c",
		1050,
		widget);
	result = widget->child;
	for (i = 0; i < n && result; i++)
		result = result->next;

	return result;
}

void widget_instance_set_visibility_recursive(
	struct widget_instance *widget,
	boolean visible)
{
	struct widget_instance *child;

	match_assert(
		"c:\\halo\\SOURCE\\interface\\ui_widget.c",
		1859,
		widget);
	widget->visible = visible;
	for (child = widget->child; child; child = child->next)
		widget_instance_set_visibility_recursive(child, visible);

	return;
}

void *ui_widget_realloc(
	void *pointer,
	word size,
	char const *file,
	unsigned long line)
{
	return pool_resize_pointer(
		widget_memory_pool,
		pointer,
		size,
		file,
		line);
}

void widget_free(
	void *ptr)
{
	dispose_pointer(widget_memory_pool, ptr);

	return;
}

void main_menu_active(
	boolean active)
{
	we_are_at_the_main_menu = active;

	return;
}

boolean main_menu_is_active(
	void)
{
	return we_are_at_the_main_menu;
}

void ui_widget_load_progress_widget(
	void)
{
	error(
		_error_silent,
		"the old loading progress screen has been replaced with glowy halo gravy");

	return;
}

boolean filesystem_check_thread_is_active(
	void)
{
	return widget_globals.initialization_thread != NULL;
}

void display_error_when_main_menu_loaded(
	short error_code)
{
	if (widget_globals.main_menu_deferred_error_code == NONE)
	{
		widget_globals.main_menu_deferred_error_code = error_code;
		return;
	}

	error(
		_error_silent,
		"there is already an error message queued for display at the main menu; ignoring this one");
	return;
}

void display_error_abort_to_dashboard_deferred(
	short error_code,
	boolean optional)
{
	if (widget_globals.deferred_dashboard_error_code == NONE)
	{
		widget_globals.deferred_dashboard_error_code = error_code;
		widget_globals.deferred_dashboard_optional = optional;
		return;
	}

	error(
		_error_silent,
		"there is already a deferred dashbaord error queued; ignoring this one!");
	return;
}

boolean ui_main_menu_music_active(
	void)
{
	return widget_globals.main_menu_music_active;
}

void ui_widgets_disable_pause_game(
	long duration_ticks)
{
	match_assert(
		"c:\\halo\\SOURCE\\interface\\ui_widget.c",
		2519,
		duration_ticks>=0);
	widget_globals.pause_disabled_ticks = duration_ticks;

	return;
}

struct widget_instance *widget_instance_get_topmost_parent(
	struct widget_instance *widget)
{
	while (widget->parent)
		widget = widget->parent;

	return widget;
}

int widget_instance_get_child_index_from_parent(
	struct widget_instance *widget)
{
	int result = NONE;
	struct widget_instance *parent = widget->parent;

	if (parent)
	{
		int index = 0;
		struct widget_instance *child;

		for (child = parent->child; child; child = child->next)
		{
			if (child == widget)
			{
				result = index;
				break;
			}
			index++;
		}
	}

	return result;
}

pixel32 modulate_pixel32_by_real_alpha(
	pixel32 argb,
	real alpha)
{
	real modulated_alpha = (argb >> 24) * alpha;

	return (fast_ftol(modulated_alpha) << 24) | (argb & 0x00FFFFFF);
}

void ui_set_next_level(
	short level)
{
	long level_index = level;

	if (level_index != NONE)
	{
		if (level_index >= 0 && level_index <= 9)
		{
			main_set_map_name(main_get_solo_level_name(level));
			main_disallow_persistent_storage();
		}
		else
		{
			error(_error_silent, "unknown level");
			main_goto_main_menu();
		}
	}
	else
	{
		main_roll_credits();
	}

	return;
}

boolean ui_widgets_active(
	void)
{
	boolean result = FALSE;

	if (widget_globals.initialized)
	{
		long local_player_index;

		for (local_player_index = 0;
			local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
			local_player_index++)
		{
			if (widget_globals.active_widgets[local_player_index])
			{
				result = TRUE;
				break;
			}
		}
	}

	return result;
}

boolean main_menu_screen_is_active(
	void)
{
	if (we_are_at_the_main_menu == TRUE &&
		widget_globals.active_widgets[0] &&
		strcmp(widget_globals.active_widgets[0]->name, "the_main_menu") == 0)
	{
		return TRUE;
	}

	return FALSE;
}

static void *pool_alloc(
	unsigned long size)
{
	return match_malloc("c:\\halo\\SOURCE\\interface\\ui_widget.c", 117, size);
}

static void pool_free(
	void *pointer)
{
	match_free("c:\\halo\\SOURCE\\interface\\ui_widget.c", 118, pointer);

	return;
}

void ui_widgets_initialize(
	void)
{
	boolean success = TRUE;
	byte *base_address;
	long local_player_index;

	base_address = pool_alloc(WIDGET_MEMORY_POOL_SIZE);
	if (base_address)
	{
		widget_memory_pool->base_address = base_address;
		widget_memory_pool->size = WIDGET_MEMORY_POOL_SIZE;
	}
	else
	{
		success = FALSE;
	}
	stack_memory_pool_reset(widget_memory_pool);

	memset(&widget_globals, 0, sizeof(widget_globals));
	widget_globals.main_menu_deferred_error_code = NONE;
	widget_globals.deferred_dashboard_error_code = NONE;
	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		widget_globals.deferred_errors[local_player_index].error_code = NONE;
		widget_globals.deferred_cinematic_errors[local_player_index].error_code = NONE;
	}
	widget_globals.initialized = success;
	widget_globals.fade_to_black = -1.0f;

	return;
}

void ui_widgets_dispose(
	void)
{
	ui_widgets_close_all();
	if (widget_memory_pool->base_address)
		pool_free(widget_memory_pool->base_address);
	widget_memory_pool->base_address = NULL;
	widget_memory_pool->size = 0;
	memset(&widget_globals, 0, sizeof(widget_globals));

	return;
}

static void pop_widget(
	struct widget_stack_node **top,
	struct widget_stack_data *data)
{
	struct widget_stack_node *node;

	match_assert(
		"c:\\halo\\SOURCE\\interface\\ui_widget.c",
		2556,
		top && data);
	node = *top;
	*data = node->data;
	*top = node->next;
	dispose_pointer(widget_memory_pool, node);

	return;
}

static void dispose_widget_stack(
	struct widget_stack_node **top)
{
	while (*top)
	{
		struct widget_stack_node *node = *top;

		*top = node->next;
		dispose_pointer(widget_memory_pool, node);
	}

	return;
}

void ui_widgets_close_all(
	void)
{
	long local_player_index;

	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		if (widget_globals.active_widgets[local_player_index])
			ui_widget_delete(widget_globals.active_widgets[local_player_index]);
		if (widget_globals.widget_stack[local_player_index])
			dispose_widget_stack(&widget_globals.widget_stack[local_player_index]);
	}

	return;
}

void ui_widgets_close_all_for_local_player(
	short local_player_index)
{
	long widget_index;

	match_vassert(
		"c:\\halo\\SOURCE\\interface\\ui_widget.c",
		1154,
		local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS,
		"expected a valid local_player_index");
	for (widget_index = 0;
		widget_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		widget_index++)
	{
		struct widget_instance *widget = widget_globals.active_widgets[widget_index];

		if (widget && widget->local_player_index == local_player_index)
		{
			ui_widget_delete(widget);
			if (widget_globals.widget_stack[widget_index])
				dispose_widget_stack(&widget_globals.widget_stack[widget_index]);
		}
	}

	return;
}

void ui_widgets_delete_history(
	void)
{
	long local_player_index;

	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		if (widget_globals.widget_stack[local_player_index])
			dispose_widget_stack(&widget_globals.widget_stack[local_player_index]);
	}

	return;
}

void ui_widgets_pop_stack(
	short local_player_index)
{
	struct widget_stack_data data;

	if (local_player_index == NONE)
	{
		local_player_index = 0;
	}
	else
	{
		match_assert(
			"c:\\halo\\SOURCE\\interface\\ui_widget.c",
			1204,
			(local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS));
	}
	if (widget_globals.widget_stack[local_player_index])
		pop_widget(&widget_globals.widget_stack[local_player_index], &data);

	return;
}

void main_screen_shell_begin_fade(
	unsigned long fade_duration_milliseconds)
{
	long local_player_index;

	ui_stop_main_menu_music();
	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		if (widget_globals.active_widgets[local_player_index] &&
			!widget_globals.active_widgets[local_player_index]->widget_is_error_dialog)
		{
			widget_globals.active_widgets[local_player_index]->auto_close_fade_time = fade_duration_milliseconds;
			widget_globals.active_widgets[local_player_index]->milliseconds_to_auto_close =
				widget_globals.current_system_milliseconds -
				widget_globals.active_widgets[local_player_index]->creation_time + 100;
			if (widget_globals.widget_stack[local_player_index])
				dispose_widget_stack(&widget_globals.widget_stack[local_player_index]);
		}
	}

	return;
}

static void play_sound_tag(
	long sound_tag_index)
{
	if (sound_tag_index != NONE)
		unspatialized_impulse_sound_new(sound_tag_index, 1.0f);

	return;
}

void ui_play_audio_feedback_sound(
	short audio_feedback)
{
	switch (audio_feedback)
	{
	case _ui_audio_feedback_cursor:
		play_sound_tag(tag_loaded(SOUND_DEFINITION_TAG, "sound\\sfx\\ui\\cursor"));
		break;
	case _ui_audio_feedback_forward:
		play_sound_tag(tag_loaded(SOUND_DEFINITION_TAG, "sound\\sfx\\ui\\forward"));
		break;
	case _ui_audio_feedback_back:
		play_sound_tag(tag_loaded(SOUND_DEFINITION_TAG, "sound\\sfx\\ui\\back"));
		break;
	case _ui_audio_feedback_flag_failure:
		play_sound_tag(tag_loaded(SOUND_DEFINITION_TAG, "sound\\sfx\\ui\\flag_failure"));
		break;
	}

	return;
}

void ui_start_main_menu_music(
	void)
{
	if (!widget_globals.main_menu_music_active && !main_menu_fade_active())
	{
		long sound_definition_index = tag_loaded(LOOPING_SOUND_DEFINITION_TAG, "sound\\music\\title1\\title1");

		if (sound_definition_index != NONE)
		{
			error(_error_silent, "starting main menu music");
			scripted_looping_sound_start(sound_definition_index, NONE, 1.0f);
			widget_globals.main_menu_music_active = TRUE;
		}
		else
		{
			error(_error_silent, "title music tag not found");
		}
	}

	return;
}

void ui_stop_main_menu_music(
	void)
{
	if (widget_globals.main_menu_music_active == TRUE)
	{
		long sound_definition_index = tag_loaded(LOOPING_SOUND_DEFINITION_TAG, "sound\\music\\title1\\title1");

		if (sound_definition_index != NONE)
		{
			error(_error_silent, "stopping main menu music");
			scripted_looping_sound_stop(sound_definition_index);
		}
		else
		{
			error(_error_silent, "title music tag not found");
		}
		widget_globals.main_menu_music_active = FALSE;
	}

	return;
}

void display_error_deferred(
	short error_code,
	short local_player_index,
	boolean modal,
	boolean pause_game_time)
{
	long index;

	if (local_player_index == NONE)
	{
		index = 0;
	}
	else
	{
		index = local_player_index;
		match_assert(
			"c:\\halo\\SOURCE\\interface\\ui_widget.c",
			2288,
			(index>=0) && (index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS));
	}
	if (widget_globals.deferred_errors[index].error_code == NONE)
	{
		widget_globals.deferred_errors[index].error_code = error_code;
		widget_globals.deferred_errors[index].local_player_index = local_player_index;
		widget_globals.deferred_errors[index].modal = modal;
		widget_globals.deferred_errors[index].pause_game_time = pause_game_time;
	}
	else
	{
		error(
			_error_silent,
			"there is already a deferred error message for local player %d; ignoring this one",
			index);
	}

	return;
}

void display_errors_deferred_until_cinematic_stop(
	void)
{
	short local_player_index;

	match_vassert(
		"c:\\halo\\SOURCE\\interface\\ui_widget.c",
		2367,
		!cinematic_in_progress(),
		"Noooooooooooooooooo!!!");
	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		struct ui_widget_deferred_cinematic_error *deferred_error =
			&widget_globals.deferred_cinematic_errors[local_player_index];

		if (deferred_error->error_code >= 0 && deferred_error->error_code < NUMBER_OF_ERROR_CODES)
		{
			display_error(
				deferred_error->error_code,
				local_player_index,
				deferred_error->modal,
				deferred_error->pause_game_time);
		}
		deferred_error->error_code = NONE;
	}

	return;
}

boolean ui_widgets_active_for_local_player(
	short local_player_index)
{
	boolean result = FALSE;

	match_vassert(
		"c:\\halo\\SOURCE\\interface\\ui_widget.c",
		1110,
		local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS,
		"expected a valid local_player_index");
	if (widget_globals.initialized)
	{
		long widget_index;

		for (widget_index = 0;
			widget_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
			widget_index++)
		{
			if (widget_globals.active_widgets[widget_index] &&
				widget_globals.active_widgets[widget_index]->local_player_index == local_player_index)
			{
				result = TRUE;
				break;
			}
		}
	}

	return result;
}

void display_error_abort_to_dashboard(
	short error_code,
	boolean optional)
{
	char const *widget_name;
	struct widget_instance *widget;

	if (optional == TRUE)
		widget_name = "ui\\shell\\error\\error_abort_to_dashboard";
	else
		widget_name = "ui\\shell\\error\\error_abort_to_dashboard_you_have_no_choice";
	if (!optional)
		ui_widgets_close_all();
	widget = ui_widget_load_by_name_or_tag(widget_name, NONE, NULL, NONE, NONE, NONE, NONE);
	if (widget)
	{
		match_vassert(
			"c:\\halo\\SOURCE\\interface\\ui_widget.c",
			2319,
			widget->type == _ui_widget_type_text_box,
			"expected a text box widget");
		widget->parameters.text_box.string_list_index = error_code;
		widget->widget_is_error_dialog = TRUE;
		dashboard_abort_error = error_code;
	}
	else
	{
		error(_error_silent, "failed to load '%s' widget", widget_name);
	}

	return;
}

void display_error_damaged_media(
	void)
{
	display_error_abort_to_dashboard(_error_media_damaged, FALSE);
	input_frame_end();
	main_loop_of_death();

	return;
}

void network_game_reset_to_pregame_ui(
	void)
{
	ui_widgets_close_all();
	if (network_game_is_splitscreen_local())
	{
		if (network_game_is_quickstart_local())
		{
			if (!ui_widget_load_by_name_or_tag(
				"ui\\shell\\main_menu\\multiplayer_type_select\\split_screen\\pregame\\splitscreen_pregame_wrapper_normal",
				NONE, NULL, NONE, NONE, NONE, NONE))
			{
				error(_error_silent, "failed to load pregame screen after quickstart match");
			}
		}
		else
		{
			if (!ui_widget_load_by_name_or_tag(
				"ui\\shell\\main_menu\\multiplayer_type_select\\split_screen\\splitscreen_map_select_postgame_wrapper",
				NONE, NULL, NONE, NONE, NONE, NONE))
			{
				error(_error_silent, "failed to load map select postgame screen");
			}
		}
	}
	else
	{
		if (global_network_game_server_get())
		{
			network_game_server_pause_countdown(global_network_game_server_get(), TRUE);
			if (!ui_widget_load_by_name_or_tag(
				"ui\\shell\\main_menu\\multiplayer_type_select\\connected\\connected_map_select_postgame_wrapper",
				NONE, NULL, NONE, NONE, NONE, NONE))
			{
				error(_error_silent, "failed to load map select postgame screen");
			}
		}
		else
		{
			if (!ui_widget_load_by_name_or_tag(
				"ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen",
				NONE, NULL, NONE, NONE, NONE, NONE))
			{
				error(_error_silent, "failed to load networked pregame status screen");
			}
		}
	}

	return;
}

static boolean transition_to_game_in_progress(
	void)
{
	return we_are_at_the_main_menu &&
		widget_globals.fade_to_black <= 1.0f &&
		widget_globals.fade_to_black >= 0.0f;
}

static short get_icon_type(
	wchar_t const *string)
{
	short icon_index;

	for (icon_index = 0; icon_index < NUM_ICONS; icon_index++)
	{
		if (_wcsnicmp(string, icon_names[icon_index], wcslen(icon_names[icon_index])) == 0)
			break;
	}

	return (icon_index == NUM_ICONS) ? NONE : icon_index;
}

static boolean should_flip_sticks_for_local_player(
	short local_player_index)
{
	struct game_input_preferences preferences;

	if (local_player_index == NONE)
		local_player_index = local_player_get_next(NONE);
	memset(&preferences, 0, sizeof(preferences));
	if (local_player_index != NONE)
		input_abstraction_get_local_player_preferences(local_player_index, &preferences);
	switch (preferences.joystick_controls)
	{
	case _joystick_preset_south_paw:
	case _joystick_preset_legacy_south_paw:
		return TRUE;
	}

	return FALSE;
}

short remap_sticks_for_local_player(
	short icon,
	short local_player_index)
{
	long icon_index = icon;

	switch (icon_index)
	{
	case _icon_left_stick:
	case _icon_move:
		match_assert(
			"c:\\halo\\SOURCE\\interface\\ui_widget.c",
			4243,
			16 == get_icon_type(L"left-stick"));
		match_assert(
			"c:\\halo\\SOURCE\\interface\\ui_widget.c",
			4244,
			30 == get_icon_type(L"move"));
		return should_flip_sticks_for_local_player(local_player_index) ? _icon_right_stick : _icon_left_stick;
	case _icon_right_stick:
	case _icon_look:
		match_assert(
			"c:\\halo\\SOURCE\\interface\\ui_widget.c",
			4250,
			17 == get_icon_type(L"right-stick"));
		match_assert(
			"c:\\halo\\SOURCE\\interface\\ui_widget.c",
			4251,
			31 == get_icon_type(L"look"));
		return should_flip_sticks_for_local_player(local_player_index) ? _icon_left_stick : _icon_right_stick;
	}

	return icon;
}

static unsigned long __stdcall filesystem_initialization_thread_proc(
	void *input)
{
	widget_globals.filesystem_check_result = saved_game_perform_file_system_checks();
	if (!widget_globals.filesystem_check_result)
	{
		word number_of_profiles = 1;
		long profile_index;

		playlist_profiles_enumerate_available_to_local_player_index(NONE, &number_of_profiles, &profile_index);
		player_profiles_enumerate_available_to_local_player_index(NONE, &number_of_profiles, &profile_index, TRUE);
		player_ui_get_player1_last_used_profile_index();
	}

	return 0;
}

static void perform_filesystem_initialization(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\interface\\ui_widget.c",
		5439,
		widget_globals.initialization_thread==NULL);
	error(_error_silent, "begining filesystem checks & saved game file enumeration...");
	ui_widgets_inhibit_processing(TRUE);
	widget_globals.filesystem_check_result = 0;
	if (!create_thread(0, filesystem_initialization_thread_proc, NULL, &widget_globals.initialization_thread))
	{
		error(_error_silent, "failed to spawn thread for filesystem checks - running synchronously!");
		widget_globals.initialization_thread = NULL;
		filesystem_initialization_thread_proc(NULL);
		ui_widgets_inhibit_processing(FALSE);
	}

	return;
}

void main_screen_shell_load(
	void)
{
	boolean load_main_menu = TRUE;

	ui_widgets_inhibit_processing(FALSE);
	if (main_screen_shell_first_load == TRUE)
	{
		char const *command_line = shell_get_command_line();

		if (command_line && _stricmp(command_line, "xdemo") == 0)
		{
			error(_error_silent, "xbox command line= '%s'", command_line);
		}
		else
		{
			bink_playback_start(
				attract_mode_get_localized_movie_path(_bink_intro_movie),
				FLAG(_bink_playback_button_click_stops_movie_bit) |
					FLAG(_bink_playback_prevent_events_to_ui_bit) |
					FLAG(_bink_playback_return_to_main_menu_when_finished_bit) |
					FLAG(_bink_playback_dont_allow_skipping_if_filesystem_check_thread_is_active_bit) |
					FLAG(_bink_playback_eat_up_memory_like_a_goddamn_beaver_bit));
			load_main_menu = FALSE;
			if (!bink_playback_active())
				load_main_menu = TRUE;
		}
		perform_filesystem_initialization();
		input_abstraction_reset_controller_detection_timer();
	}
	if (load_main_menu)
	{
		attract_mode_reset_timer();
		ui_widgets_close_all();
		if (!ui_widget_load_by_name_or_tag("ui\\shell\\main_menu\\main_menu", NONE, NULL, NONE, NONE, NONE, NONE))
			error(_error_silent, "failed to load main screen shell window");
		if (widget_globals.main_menu_deferred_error_code != NONE)
		{
			display_error(widget_globals.main_menu_deferred_error_code, NONE, TRUE, FALSE);
			widget_globals.main_menu_deferred_error_code = NONE;
		}
		if (!widget_globals.main_menu_music_active)
			ui_start_main_menu_music();
		reset_last_player1_profile_index();
	}
	if (!virtual_keyboard_initialize())
		error(_error_silent, "failed to initialize the virtual keyboard");
	main_screen_shell_first_load = FALSE;

	return;
}

void widget_instance_reload_recursive(
	void)
{
	return;
}

void ui_widget_reload_by_tag(
	void)
{
	return;
}

real_rgb_color get_ui_rgb_white(
	void)
{
	real_rgb_color result;

	result = *global_real_rgb_white;
	result.red = global_ui_white_red;
	result.green = global_ui_white_green;
	result.blue = global_ui_white_blue;

	return result;
}

real_argb_color get_ui_argb_white(
	void)
{
	real_argb_color result;

	result = *global_real_argb_white;
	result.red = global_ui_white_red;
	result.green = global_ui_white_green;
	result.blue = global_ui_white_blue;

	return result;
}

/* ---------- private code */
