| unit | symbols.json line | Jan addr / padded | current | Sept-2001 first-party | Sept body vs Jan (reloc-masked) | callees | Aug-2001 map | Jan atlas at addr |
|---|---|---|---|---|---|---|---|---|
| ai/ai_debug | 604 | 0x37890 / 0x80 | `_code_00037890` | `_ai_debug_render_path_line` | IDENT | SAME | `_ai_debug_render_path_line` | `_ai_debug_render_path_line` (exact) |
| ai/ai_debug | 610 | 0x37af0 / 0xd0 | `_code_00037af0` | `_ai_debug_render_lineoffire` | IDENT | SAME | `_ai_debug_render_lineoffire` | `_ai_debug_render_lineoffire` (exact) |
| ai/ai_debug | 611 | 0x37bc0 / 0x110 | `_code_00037bc0` | `_ai_debug_render_ballistic_lineoffire` | IDENT | SAME | `_ai_debug_render_ballistic_lineoffire` | `_ai_debug_render_ballistic_lineoffire` (exact) |
| ai/ai_debug | 614 | 0x37dd0 / 0x110 | `_code_00037dd0` | `_ai_debug_lineofsight_findpoint` | IDENT | SAME | `_ai_debug_lineofsight_findpoint` | `_ai_debug_lineofsight_findpoint` (fuzzy) |
| ai/ai_debug | 615 | 0x37ee0 / 0xc0 | `_code_00037ee0` | `_ai_debug_lineofsight_storeray` | IDENT | SAME | `_ai_debug_lineofsight_storeray` | `_ai_debug_lineofsight_storeray` (exact) |
| ai/ai_debug | 616 | 0x37fa0 / 0x130 | `_code_00037fa0` | `_ai_debug_render_lineofsight` | IDENT | SAME | `_ai_debug_render_lineofsight` | `_ai_debug_render_lineofsight` (fuzzy) |
| ai/ai_debug | 618 | 0x38280 / 0x150 | `_code_00038280` | `_ai_debug_get_this_actor` | 0.363 | assert prefix + same 7 callees | `_ai_debug_get_this_actor` | - |
| ai/ai_debug | 619 | 0x383d0 / 0x200 | `_code_000383d0` | `_ai_debug_render_aiming_validity` | 0.936 | SAME | `_ai_debug_render_aiming_validity` | `_ai_debug_render_aiming_validity` (fuzzy) |
| ai/ai_debug | 621 | 0x386a0 / 0x1f0 | `_code_000386a0` | `_ai_debug_speech_update` | IDENT | SAME | `_ai_debug_speech_update` | `_ai_debug_speech_update` (exact) |
| ai/ai_debug | 624 | 0x38ad0 / 0x1f0 | `_code_00038ad0` | `_ai_debug_communication_toggle_bits` | 0.976 | SAME | `_ai_debug_communication_toggle_bits` | `_ai_debug_communication_toggle_bits` (exact) |
| ai/ai_debug | 630 | 0x38de0 / 0x150 | `_code_00038de0` | `_ai_debug_render_idle_look` | 0.933 | SAME | `_ai_debug_render_idle_look` | `_ai_debug_render_idle_look` (fuzzy) |
| ai/ai_debug | 631 | 0x38f30 / 0x130 | `_code_00038f30` | `_ai_debug_render_spatial_effects` | IDENT | SAME | - | - |
| ai/ai_debug | 632 | 0x39060 / 0x120 | `_code_00039060` | `_ai_debug_path_storage_update` | IDENT | SAME | `_ai_debug_render_spatial_effects` | `_ai_debug_path_storage_update` (exact) |
| ai/ai_debug | 639 | 0x39990 / 0x350 | `_code_00039990` | `_ai_debug_render_path_node` | IDENT | SAME | `_ai_debug_render_path_node` | `_ai_debug_render_path_node` (exact) |
| ai/ai_debug | 642 | 0x39e10 / 0x70 | `_code_00039e10` | `_ai_debug_select_this_actor` | 0.973 | SAME | `_ai_debug_select_this_actor` | `_ai_debug_select_this_actor` (exact) |
| ai/ai_debug | 643 | 0x39e80 / 0x460 | `_code_00039e80` | `_ai_debug_render_speech` | 0.981 | SAME | `_ai_debug_render_speech` | `_ai_debug_render_speech` (fuzzy) |
| ai/ai_debug | 644 | 0x3a2e0 / 0x480 | `_code_0003a2e0` | `_ai_debug_render_vehicles_enterable` | IDENT | SAME | `_ai_debug_render_vehicles_enterable` | `_ai_debug_render_vehicles_enterable` (exact) |
| ai/ai_debug | 647 | 0x3a910 / 0x2c0 | `_code_0003a910` | `_ai_debug_render_path_nodes` | IDENT | SAME | `_ai_debug_render_path_nodes` | `_ai_debug_render_path_nodes` (fuzzy) |
| ai/ai_debug | 649 | 0x3af00 / 0x90 | `_code_0003af00` | `_ai_debug_render_path` | IDENT | SAME | `_ai_debug_render_path` | `_ai_debug_render_path` (exact) |
| ai/ai_debug | 652 | 0x41120 / 0xb0 | `_code_00041120` | `_ai_debug_render_paths_failed` | IDENT | SAME | - | `_ai_debug_render_paths_failed` (exact) |
| ai/ai_debug | 653 | 0x411d0 / 0x50 | `_code_000411d0` | `_ai_debug_render_all_actors` | IDENT | SAME | `_ai_debug_render_all_actors` | `_ai_debug_render_all_actors` (exact) |
| bungie_net/network/transport_endpoint_winsock | 1394 | 0x71300 / 0x60 | `_connection_thread_list_add` | `_add_connect_thread` | IDENT | SAME | `_add_connect_thread` | `@add_connect_thread@4` (exact) |
| bungie_net/network/transport_endpoint_winsock | 1395 | 0x71360 / 0x40 | `_connection_thread_list_mark_for_disposal` | `_mark_connection_thread_as_terminated` | IDENT | SAME | `_mark_connection_thread_as_terminated` | `_mark_connection_thread_as_terminated` (exact) |
| bungie_net/network/transport_endpoint_winsock | 1409 | 0x71fa0 / 0x130 | `_create_endpoint_socket` | `_create_socket` | IDENT | differs only in unresolved Sept lib addrs | `_create_socket` | `@create_socket@12` (exact) |
| bungie_net/network/transport_endpoint_winsock | 1415 | 0x726f0 / 0x130 | `_connect_endpoint_process@4` | `_connect_async_thread_proc@4` | IDENT | SAME | `_connect_async_thread_proc@4` | - |
| bungie_net/network/transport_endpoint_set_winsock | 1369 | 0x70430 / 0x40 | `_transport_endpoint_set_get_next_index` | `_get_next_available_set_array_index` | IDENT | SAME | `_get_next_available_set_array_index` | - |
| bungie_net/network/transport_endpoint_set_winsock | 1384 | 0x70b10 / 0x30 | `_transport_endpoint_set_compare_entries` | `_poll_ep_array_compare_proc` | IDENT | SAME | `_poll_ep_array_compare_proc` | `?poll_endpoint_array_compare_proc@@YAHPBX0@Z` (exact) |
| cseries/profile | 1618 | 0x7dd20 / 0x160 | `_profile_sections_update` | `_profile_internal_step` | IDENT | SAME | `_profile_internal_step` | `_profile_internal_step` (exact) |
| cseries/profile | 1620 | 0x7dea0 / 0x40 | `_profile_timesection_subtract` | `_profile_timesection_inherit` | IDENT | SAME | - | - |
| cseries/profile | 1627 | 0x7e1d0 / 0x610 | `_profile_frame_build_string` | `_profile_describe_frame` | IDENT | SAME | - | `_profile_describe_frame` (exact) |
| cseries/profile | 1628 | 0x7e7e0 / 0x10 | `_profile_timer_start_at` | `_profile_timesection_begin` | IDENT | SAME | - | - |
| cseries/profile | 1629 | 0x7e7f0 / 0x50 | `_profile_timer_end_at` | `_profile_timesection_end` | IDENT | SAME | - | - |
| cseries/profile | 1633 | 0x7ed40 / 0x80 | `_profile_frame_dump` | `_profile_dump_frame` | IDENT | SAME | - | `_profile_dump_frame` (exact) |
| cseries/profile | 1634 | 0x7edc0 / 0x40 | `_profile_framedump_flush` | `_profile_dump_frame_stop` | IDENT | SAME | - | - |
| cseries/profile | 1635 | 0x7ee00 / 0x30 | `_string_starts_with` | `_string_has_prefix` | IDENT | SAME | - | - |
| cseries/profile | 1636 | 0x7ee30 / 0xa0 | `_profile_sections_set_active` | `_profile_sections_activation` | IDENT | SAME | - | `_profile_sections_activation` (exact) |
| cseries/profile | 1647 | 0x7f9c0 / 0x30 | `_profile_timer_start` | `_profile_timesection_begin_now` | IDENT | SAME | - | - |
| cseries/profile | 1648 | 0x7f9f0 / 0x60 | `_profile_timer_end` | `_profile_timesection_end_now` | IDENT | SAME | - | - |
| interface/progress_bar | 3634 | 0xd1990 / 0x30 | `_progress_bar_alias_surface_texture` | `_this_is_awful` | IDENT | SAME | - | - |
| interface/progress_bar | 3635 | 0xd19c0 / 0xa0 | `_set_blur_texture_coordinates` | `_do_convoluation_coords` | IDENT | differs only in unresolved Sept lib addrs | - | `@do_convoluation_coords@8` (exact) |
| interface/progress_bar | 3636 | 0xd1a60 / 0x130 | `_draw_fullscreen_blur` | `_draw_fade_layer` | IDENT | differs only in unresolved Sept lib addrs | - | `@draw_fade_layer@12` (exact) |
| interface/progress_bar | 3648 | 0xd2460 / 0xd0 | `_progress_bar_build_glow_texture` | `_progress_bar_create_mask_texture` | IDENT | differs only in unresolved Sept lib addrs | - | `_progress_bar_create_mask_texture` (exact) |
| networking/network_connection | 4853 | 0x117f30 / 0x200 | `_network_connection_read_unreliable` | `_network_client_unreliable_connection_read` | IDENT | SAME | `_network_client_unreliable_connection_read` | `_network_client_unreliable_connection_read` (exact) |
| networking/network_connection | 4854 | 0x118130 / 0x440 | `_network_connection_notify_traffic_event` | `_network_connection_log_traffic_event` | 0.957 | SAME | `_network_connection_log_traffic_event` | `_network_connection_log_traffic_event` (exact) |
| networking/network_connection | 4859 | 0x118ac0 / 0x80 | `_network_connection_new_serverside_client` | `_network_connection_create_client_from_endpoint` | IDENT | SAME | `_network_connection_create_client_from_endpoint` | `_network_connection_create_client_from_endpoint` (exact) |
| networking/network_connection | 4860 | 0x118b40 / 0x1e0 | `_network_connection_read_reliable` | `_network_client_reliable_connection_read` | IDENT | SAME | `_network_client_reliable_connection_read` | `_network_client_reliable_connection_read` (exact) |
| saved games/saved_game_files | 7350 | 0x1b2ce0 / 0x60 | `_find_or_create_directory` | `_find_and_create_directory_if_necessary` | IDENT | SAME | - | `_find_and_create_directory_if_necessary` (exact) |
| saved games/saved_game_files | 7363 | 0x1b3520 / 0x210 | `_enumerate_default_playlist_profile_files` | `_enumerate_default_playlist_profiles` | IDENT | differs only in unresolved Sept lib addrs | - | `_enumerate_default_playlist_profiles` (exact) |
| saved games/saved_game_files | 7364 | 0x1b3730 / 0x200 | `_enumerate_default_player_profile_files` | `_enumerate_default_player_profiles` | IDENT | differs only in unresolved Sept lib addrs | - | `_enumerate_default_player_profiles` (exact) |
| saved games/saved_game_files | 7374 | 0x1b4a00 / 0x20 | `_saved_game_files_enumerate_default_files` | `_enumerate_default_profiles` | IDENT | SAME | `_enumerate_default_profiles` | - |
| rasterizer/xbox/rasterizer_xbox_profile | 5877 | 0x15ecd0 / 0x80 | `_rasterizer_profile_check` | `_profile_assert` | IDENT | SAME | `_profile_assert` | `_profile_assert` (exact) |
| rasterizer/xbox/rasterizer_xbox_profile | 5878 | 0x15ed50 / 0x110 | `_rasterizer_profile_callback` | `_callback_function` | IDENT | SAME | - | `_callback_function` (exact) |
| rasterizer/xbox/rasterizer_xbox_profile | 5879 | 0x15ee60 / 0x90 | `_rasterizer_profile_frame_callback` | `_frame_callback_function` | IDENT | SAME | - | `_frame_callback_function` (exact) |
| rasterizer/xbox/rasterizer_xbox_profile | 5880 | 0x15eef0 / 0x20 | `_rasterizer_profile_active` | `_rasterizer_profile_enabled` | IDENT | SAME | - | - |
| cache/xbox_sound_cache | 7224 | 0x1add60 / 0x40 | `_sound_cache_debug_block_name` | `_cache_block_get_sound_permutation_name` | IDENT | SAME | `_cache_block_get_sound_permutation_name` | `_cache_block_get_sound_permutation_name` (exact) |
| game/game_engine_king | 2303 | 0xa0040 / 0x20 | `_hill_points_to_2d` | `_point3d_to_point2d` | 0.929 | SAME | `_point3d_to_point2d` | - |
| game/players | 2595 | 0xa9f80 / 0x70 | `_unit_create_starting_weapon` | `_create_weapon` | IDENT | SAME | `_create_weapon` | `?unit_create_starting_weapon@@YAJJPBUstarting_weapon_info@@@Z` (exact) |
| hs/hs | 2908 | 0xb14a0 / 0x20 | `_radiosity_start_evaluate` | `_radiosity_hack_start_evaluate` | IDENT | SAME | `_radiosity_hack_start_evaluate` | - |
| hs/hs | 2909 | 0xb14c0 / 0x20 | `_radiosity_save_evaluate` | `_radiosity_hack_save_evaluate` | IDENT (xcmp best=_radiosity_hack_start_evaluate) | SAME | `_radiosity_hack_save_evaluate` | - |
| hs/hs | 2910 | 0xb14e0 / 0x20 | `_radiosity_debug_point_evaluate` | `_radiosity_hack_find_point_evaluate` | IDENT (xcmp best=_radiosity_hack_start_evaluate) | SAME | `_radiosity_hack_find_point_evaluate` | - |
| hs/hs_runtime | 3220 | 0xbace0 / 0x40 | `_hs_runtime_recompile_error` | `_script_error` | IDENT | SAME | `_script_error` | - |
| main/main | 4107 | 0xef8a0 / 0x40 | `_sort_controllers_ascending` | `_sort_desired_local_player_controllers` | IDENT | SAME | `_sort_desired_local_player_controllers` | `?sort_controllers_ascending@@YAHPBX0@Z` (exact) |
| effects/decals | 1840 | 0x86fe0 / 0x130 | `_decal_verify_neighbors` | `_decal_check` | IDENT | SAME | `_decal_check` | `_decal_check` (exact) |
| rasterizer/rasterizer_lights | 6206 | 0x170870 / 0x40 | `_lens_flare_submit_parameter_get` | `_lens_flare_parameters_get` | IDENT | SAME | `_lens_flare_parameters_get` | - |
| rasterizer/xbox/rasterizer_xbox_dynavobgeom | 5720 | 0x14f0f0 / 0x40 | `_rasterizer_screen_geometry_submit_vertex` | `_submit_screen_vertex` | IDENT | differs only in unresolved Sept lib addrs | - | - |
| rasterizer/xbox/rasterizer_xbox_environment_fog | 5786 | 0x155860 / 0x200 | `_rasterizer_environment_fog_screen_active` | `_rasterizer_environment_fog_screen_is_active` | IDENT | SAME | `_rasterizer_environment_fog_screen_is_active` | `_rasterizer_environment_fog_screen_is_active` (exact) |
| rasterizer/xbox/rasterizer_xbox_hardware_bitmaps | 5810 | 0x158120 / 0x210 | `_rasterizer_bitmap_cube_map_changed` | `_rasterizer_bitmap_cm_changed` | IDENT | differs only in unresolved Sept lib addrs | `_rasterizer_bitmap_cm_changed` | `_rasterizer_bitmap_cm_changed` (exact) |
| render/render_cameras | 6283 | 0x174fc0 / 0xa0 | `_render_camera_check_warning_condition` | `_render_camera_warn_once` | IDENT | SAME | `_render_camera_warn_once` | `_render_camera_warn_once` (exact) |
| interface/progress_bar | 3610 | 0xd1150 / 0x10 | `_progress_bar_screen_initialized` | `_progress_bar_is_stuff_ready` | IDENT | SAME | - | - |
| interface/progress_bar | 3631 | 0xd1770 / 0x160 | `_draw_gravy_layer` | `_draw_layer_int` | IDENT | differs only in unresolved Sept lib addrs | - | - |
| interface/progress_bar | 3632 | 0xd18d0 / 0x60 | `_draw_gravy_screen_layer` | `_draw_layer` | IDENT | SAME | - | - |
| interface/progress_bar | 3633 | 0xd1930 / 0x60 | `_draw_gravy_base_layer` | `_draw_initial_layer` | IDENT | SAME | - | - |
| interface/progress_bar | 3646 | 0xd1f00 / 0x4b0 | `_progress_bar_draw` | `_progress_bar_render` | IDENT | differs only in unresolved Sept lib addrs | - | - |
| interface/progress_bar | 3650 | 0xd25f0 / 0x490 | `_progress_bar_setup` | `_progress_bar_make_stuff_ready` | 0.968 | differs only in unresolved Sept lib addrs | - | - |
| interface/progress_bar | 3639 | 0xd1bb0 / 0x10 | `_IDirect3DResource8_Release@4` | `_IDirect3DTexture8_Release@4` | IDENT | - | - | - |
