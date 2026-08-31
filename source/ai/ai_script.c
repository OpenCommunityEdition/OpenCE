/*
AI_SCRIPT.C

symbols in this file:
00042690 0090:
	_encounter_get_platoon (0000)
00042720 0010:
	_ai_script_initialize (0000)
00042730 0010:
	_ai_script_dispose (0000)
00042740 0010:
	_ai_script_initialize_for_new_map (0000)
00042750 0010:
	_ai_script_dispose_from_old_map (0000)
00042760 0130:
	_ai_index_from_string (0000)
00042890 00f0:
	_ai_index_to_string (0000)
00042980 0120:
	_ai_index_platoon_iterator_new (0000)
00042AA0 0070:
	_ai_index_platoon_iterator_next (0000)
00042B10 0100:
	_ai_index_squad_iterator_new (0000)
00042C10 00e0:
	_ai_index_squad_iterator_next (0000)
00042CF0 00d0:
	_ai_index_actor_iterator_new (0000)
00042DC0 0070:
	_ai_index_actor_iterator_next (0000)
00042E30 00a0:
	_object_list_from_ai_reference (0000)
00042ED0 0220:
	_ai_scripting_attach_unit (0000)
000430F0 0040:
	_ai_scripting_attach_units (0000)
00043130 0060:
	_ai_scripting_detach_unit (0000)
00043190 0090:
	_ai_scripting_detach_units (0000)
00043220 0090:
	_ai_scripting_place (0000)
000432B0 0060:
	_code_000432b0 (0000)
00043310 0060:
	_ai_scripting_kill (0000)
00043370 0060:
	_ai_scripting_kill_silent (0000)
000433D0 0090:
	_ai_scripting_erase (0000)
00043460 0030:
	_ai_scripting_erase_all (0000)
00043490 0020:
	_ai_scripting_deselect (0000)
000434B0 0040:
	_ai_scripting_select (0000)
000434F0 0110:
	_ai_scripting_spawn_actor (0000)
00043600 0080:
	_ai_scripting_set_respawn (0000)
00043680 0080:
	_ai_scripting_set_deaf (0000)
00043700 0080:
	_ai_scripting_set_blind (0000)
00043780 00d0:
	_ai_scripting_magically_see_unit (0000)
00043850 0040:
	_ai_scripting_magically_see_units (0000)
00043890 0090:
	_ai_scripting_timer_start (0000)
00043920 00a0:
	_ai_scripting_timer_expire (0000)
000439C0 02d0:
	_code_000439c0 (0000)
00043C90 0020:
	_ai_scripting_swarm_count (0000)
00043CB0 0020:
	_ai_scripting_nonswarm_count (0000)
00043CD0 0020:
	_ai_scripting_living_count (0000)
00043CF0 0040:
	_ai_scripting_living_fraction (0000)
00043D30 0030:
	_ai_scripting_strength (0000)
00043D60 0060:
	_ai_scripting_is_attacking (0000)
00043DC0 0090:
	_ai_scripting_attack (0000)
00043E50 0090:
	_ai_scripting_defend (0000)
00043EE0 0090:
	_ai_scripting_maneuver (0000)
00043F70 00a0:
	_ai_scripting_maneuver_enable (0000)
00044010 0430:
	_code_00044010 (0000)
00044440 0550:
	_code_00044440 (0000)
00044990 00a0:
	_ai_scripting_migrate (0000)
00044A30 00f0:
	_code_00044a30 (0000)
00044B20 0110:
	_ai_scripting_migrate_by_unit (0000)
00044C30 00e0:
	_ai_scripting_migrate_and_speak (0000)
00044D10 00f0:
	_ai_scripting_allegiance (0000)
00044E00 0050:
	_ai_scripting_allegiance_remove (0000)
00044E50 0050:
	_ai_scripting_allegiance_broken (0000)
00044EA0 0050:
	_code_00044ea0 (0000)
00044EF0 0060:
	_ai_scripting_going_to_vehicle (0000)
00044F50 00a0:
	_ai_scripting_exit_vehicle (0000)
00044FF0 00a0:
	_ai_scripting_braindead (0000)
00045090 0100:
	_ai_scripting_braindead_by_unit (0000)
00045190 00a0:
	_ai_scripting_ignore (0000)
00045230 00a0:
	_ai_scripting_prefer_target (0000)
000452D0 0120:
	_code_000452d0 (0000)
000453F0 0060:
	_ai_scripting_teleport_starting_location_if_unsupported (0000)
00045450 0060:
	_ai_scripting_teleport_starting_location (0000)
000454B0 0090:
	_ai_scripting_try_to_fight_nothing (0000)
00045540 00d0:
	_ai_scripting_try_to_fight (0000)
00045610 0090:
	_ai_scripting_try_to_fight_player (0000)
000456A0 00a0:
	_ai_scripting_allow_charge (0000)
00045740 00c0:
	_ai_scripting_command_list (0000)
00045800 00a0:
	_ai_scripting_command_list_by_unit (0000)
000458A0 0090:
	_ai_scripting_command_list_advance (0000)
00045930 0070:
	_ai_scripting_command_list_advance_by_unit (0000)
000459A0 0050:
	_code_000459a0 (0000)
000459F0 0250:
	_ai_scripting_command_list_status (0000)
00045C40 00d0:
	_ai_scripting_free (0000)
00045D10 00d0:
	_ai_scripting_free_units (0000)
00045DE0 00e0:
	_ai_scripting_attach_free (0000)
00045EC0 00b0:
	_ai_scripting_force_active (0000)
00045F70 00d0:
	_ai_scripting_force_active_by_unit (0000)
00046040 00d0:
	_ai_scripting_set_return_state (0000)
00046110 00a0:
	_ai_scripting_set_current_state (0000)
000461B0 0080:
	_code_000461b0 (0000)
00046230 00a0:
	_ai_scripting_status (0000)
000462D0 0010:
	_ai_scripting_reconnect (0000)
000462E0 0090:
	_ai_scripting_playfight (0000)
00046370 01f0:
	_ai_scripting_vehicle_encounter (0000)
00046560 00a0:
	_ai_scripting_find_vehicle_enterable (0000)
00046600 0040:
	_ai_scripting_vehicle_enterable_distance (0000)
00046640 0050:
	_ai_scripting_vehicle_enterable_team (0000)
00046690 0050:
	_ai_scripting_vehicle_enterable_actor_type (0000)
000466E0 00a0:
	_ai_scripting_vehicle_enterable_actors (0000)
00046780 00a0:
	_ai_scripting_vehicle_enterable_disable (0000)
00046820 0070:
	_ai_scripting_look_at_object (0000)
00046890 0050:
	_ai_scripting_stop_looking (0000)
000468E0 00a0:
	_ai_scripting_automatic_migration_target (0000)
00046980 0080:
	_ai_scripting_follow_target_disable (0000)
00046A00 0080:
	_ai_scripting_follow_target_players (0000)
00046A80 0090:
	_ai_scripting_follow_target_unit (0000)
00046B10 00b0:
	_ai_scripting_follow_target_ai (0000)
00046BC0 0080:
	_ai_scripting_follow_distance (0000)
00046C40 0070:
	_ai_scripting_conversation (0000)
00046CB0 0060:
	_ai_scripting_conversation_stop (0000)
00046D10 0060:
	_ai_scripting_conversation_advance (0000)
00046D70 0010:
	_ai_scripting_conversation_line (0000)
00046D80 0010:
	_ai_scripting_conversation_status (0000)
00046D90 00b0:
	_ai_scripting_link_activation (0000)
00046E40 0090:
	_ai_scripting_berserk (0000)
00046ED0 0070:
	_ai_scripting_set_team (0000)
00046F40 00a0:
	_ai_scripting_allow_dormant (0000)
00046FE0 00d0:
	_ai_scripting_magically_see_encounter (0000)
000470B0 00a0:
	_ai_scripting_magically_see_players (0000)
00047150 0010:
	_ai_scripting_retreat (0000)
00047160 0150:
	_code_00047160 (0000)
000472B0 0080:
	_ai_scripting_go_to_vehicle (0000)
00047330 0080:
	_ai_scripting_go_to_vehicle_override (0000)
000473B0 0170:
	_ai_scripting_renew (0000)
0024BE60 0001:
	_ai_script_squad_separator (0000)
0024BE68 004d:
	??_C@_0EN@GCPCCILO@platoon_absolute_index?$DO?$DN0?5?$CG?$CG?5pla@ (0000)
0024BEB8 006b:
	??_C@_0GL@DJKDNA@platoon_index?$DO?$DN0?5?$CG?$CG?5platoon_inde@ (0000)
0024BF24 0020:
	??_C@_0CA@NFDLKDJF@ai_string?5?$CG?$CG?5ai_index_reference?$AA@ (0000)
0024BF44 001e:
	??_C@_0BO@MOPGHLE@c?3?2halo?2SOURCE?2ai?2ai_script?4c?$AA@ (0000)
0024BF64 0009:
	??_C@_08EAMFHCBD@iterator?$AA@ (0000)
0024BF70 0042:
	??_C@_0EC@OOBEBCBB@ai_attach?3?5could?5not?5find?5a?5squa@ (0000)
0024BFB8 0058:
	??_C@_0FI@EBLCLMKD@ai_attach?3?5no?5actor?5variant?5spec@ (0000)
0024C010 001d:
	??_C@_0BN@JAFMEMLM@?$CFs?3?5ai_attach_unit?50x?$CF04X?5?$CFs?$AA@ (0000)
0024C030 001a:
	??_C@_0BK@DKGEFPIC@?$CFs?3?5ai_detach_unit?50x?$CF04X?$AA@ (0000)
0024C04C 0010:
	??_C@_0BA@GMNPJHGI@?$CFs?3?5ai_place?5?$CFs?$AA@ (0000)
0024C05C 000f:
	??_C@_0P@BLMJLDDB@?$CFs?3?5ai_kill?5?$CFs?$AA@ (0000)
0024C06C 0016:
	??_C@_0BG@OLKJPCEP@?$CFs?3?5ai_kill_silent?5?$CFs?$AA@ (0000)
0024C084 0010:
	??_C@_0BA@OAGPLBNE@?$CFs?3?5ai_erase?5?$CFs?$AA@ (0000)
0024C094 0011:
	??_C@_0BB@DAJIABOO@?$CFs?3?5ai_erase_all?$AA@ (0000)
0024C0A8 0016:
	??_C@_0BG@MJMIDNGO@?$CFs?3?5ai_spawn_actor?5?$CFs?$AA@ (0000)
0024C0C0 0019:
	??_C@_0BJ@EEKIPIPP@?$CFs?3?5ai_set_respawn?5?$CFs?5?$CFs?$AA@ (0000)
0024C0DC 0004:
	??_C@_03MCADLMAF@off?$AA@ (0000)
0024C0E0 0003:
	??_C@_02LIELOMNJ@on?$AA@ (0000)
0024C0E4 0016:
	??_C@_0BG@NGLFOKP@?$CFs?3?5ai_set_deaf?5?$CFs?5?$CFs?$AA@ (0000)
0024C0FC 0017:
	??_C@_0BH@HLGNJJOK@?$CFs?3?5ai_set_blind?5?$CFs?5?$CFs?$AA@ (0000)
0024C114 0024:
	??_C@_0CE@JALKNAAG@?$CFs?3?5ai_magically_see_unit?5?$CFs?50x?$CF@ (0000)
0024C138 0016:
	??_C@_0BG@LFPOGIIF@?$CFs?3?5ai_timer_start?5?$CFs?$AA@ (0000)
0024C150 0017:
	??_C@_0BH@JJJMMONM@?$CFs?3?5ai_timer_expire?5?$CFs?$AA@ (0000)
0024C168 003d:
	??_C@_0DN@EGOGAENI@?$CIcount_type?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIcount_type@ (0000)
0024C1A8 0011:
	??_C@_0BB@JNDCBAGE@?$CFs?3?5ai_attack?5?$CFs?$AA@ (0000)
0024C1BC 0011:
	??_C@_0BB@DELPBLFI@?$CFs?3?5ai_defend?5?$CFs?$AA@ (0000)
0024C1D0 0013:
	??_C@_0BD@CBLKAFAE@?$CFs?3?5ai_maneuver?5?$CFs?$AA@ (0000)
0024C1E4 001d:
	??_C@_0BN@CEFNBAKE@?$CFs?3?5ai_maneuver_enable?5?$CFs?5?$CFs?$AA@ (0000)
0024C204 0038:
	??_C@_0DI@LFBKAFPO@?5?5?$CFs?5?9?$DO?5?$CFs?5?$CIno?5matching?5squads?5f@ (0000)
0024C23C 0025:
	??_C@_0CF@LALAHEHA@?5?5?$CFs?5?9?$DO?5?$CFs?5?$CIno?5matching?5types?5fo@ (0000)
0024C264 001a:
	??_C@_0BK@CDOPAPNJ@?5?5?$CFs?5?9?$DO?5?$CFs?5?$CIsame?9type?5?$CFs?$CJ?$AA@ (0000)
0024C280 001b:
	??_C@_0BL@GFNCBEPH@?5?5?$CFs?5?9?$DO?5?$CFs?5?$CIsame?9actor?5?$CFs?$CJ?$AA@ (0000)
0024C2A0 005c:
	??_C@_0FM@JEMPJMEH@?$CIfound_squad_index?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIfou@ (0000)
0024C2FC 001d:
	??_C@_0BN@BJIJNCFF@?5?5?$CFs?5?9?$DO?5?$CFs?5?$CIsame?9variant?5?$CFs?$CJ?$AA@ (0000)
0024C31C 000f:
	??_C@_0P@HILOFCFK@?5?5?$CFs?5unchanged?$AA@ (0000)
0024C330 008c:
	??_C@_0IM@CIJDMBDM@?$CItarget_squad_indices?$FLcurrent_sq@ (0000)
0024C3C0 0053:
	??_C@_0FD@PAODIOCF@?$CIcurrent_squad_index?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIc@ (0000)
0024C414 0009:
	??_C@_08DLMOHGLB@squad?5?$CFs?$AA@ (0000)
0024C420 0063:
	??_C@_0GD@DHCPGCGK@?$CIsource_iterator?4squad_index?5?$DO?$DN?5@ (0000)
0024C484 0015:
	??_C@_0BF@OECKHMKF@?$CFs?3?5ai_migrate?5?$CFs?5?$CFs?$AA@ (0000)
0024C49C 0005:
	??_C@_04NPEDKLDA@unit?$AA@ (0000)
0024C4A4 0026:
	??_C@_0CG@IJGCILMG@?$CFs?3?5ai_migrate_by_unit?5?$DMsome?5guy@ (0000)
0024C4D0 0050:
	??_C@_0FA@GAKJCPBM@ai_migrate_and_speak?3?5unknown?5sp@ (0000)
0024C520 0022:
	??_C@_0CC@JKLCPGKI@?$CFs?3?5ai_migrate_and_speak?5?$CFs?5?$CFs?5?$CF@ (0000)
0024C544 0018:
	??_C@_0BI@ONAJBOCN@?$CFs?3?5ai_allegiance?5?$CFd?5?$CFd?$AA@ (0000)
0024C55C 001f:
	??_C@_0BP@ICGEEHN@?$CFs?3?5ai_allegiance_remove?5?$CFd?5?$CFd?$AA@ (0000)
0024C57C 0017:
	??_C@_0BH@HFKAOKKA@?$CFs?3?5ai_exit_vehicle?5?$CFs?$AA@ (0000)
0024C594 0017:
	??_C@_0BH@NDIAEFNI@?$CFs?3?5ai_braindead?5?$CFs?5?$CFs?$AA@ (0000)
0024C5AC 0028:
	??_C@_0CI@LMFNPANN@?$CFs?3?5ai_braindead_by_unit?5?$DMsome?5g@ (0000)
0024C5D4 0020:
	??_C@_0CA@GOECGNLE@?$CFs?3?5ai_disregard?5?$DMsome?5guys?$DO?5?$CFs?$AA@ (0000)
0024C5F4 0024:
	??_C@_0CE@HDJGLAIB@?$CFs?3?5ai_prefer_target?5?$DMsome?5guys?$DO@ (0000)
0024C618 0034:
	??_C@_0DE@FNEPLHAJ@?$CFs?3?5ai_teleport_starting_locatio@ (0000)
0024C64C 0025:
	??_C@_0CF@JAILDLKJ@?$CFs?3?5ai_teleport_starting_locatio@ (0000)
0024C674 001f:
	??_C@_0BP@MEILCKDB@?$CFs?3?5ai_try_to_fight_nothing?5?$CFs?$AA@ (0000)
0024C694 001a:
	??_C@_0BK@BDNNJAGD@?$CFs?3?5ai_try_to_fight?5?$CFs?5?$CFs?$AA@ (0000)
0024C6B0 001e:
	??_C@_0BO@OIEPGOJB@?$CFs?3?5ai_try_to_fight_player?5?$CFs?$AA@ (0000)
0024C6D0 001a:
	??_C@_0BK@MBMJKENK@?$CFs?3?5ai_allow_charge?5?$CFs?5?$CFs?$AA@ (0000)
0024C6EC 0006:
	??_C@_05LAPONLG@false?$AA@ (0000)
0024C6F4 0005:
	??_C@_04LOAJBDKD@true?$AA@ (0000)
0024C6FC 001a:
	??_C@_0BK@ECBKDHID@?$CFs?3?5ai_command_list?5?$CFs?5?$CFd?$AA@ (0000)
0024C718 0026:
	??_C@_0CG@KDJJNHNP@?$CFs?3?5ai_command_list_by_unit?5?$DMuni@ (0000)
0024C740 001f:
	??_C@_0BP@PAICMAGO@?$CFs?3?5ai_command_list_advance?5?$CFs?$AA@ (0000)
0024C760 0030:
	??_C@_0DA@BIDMGCPJ@?$CFs?3?5ai_command_list_advance_by_u@ (0000)
0024C790 000f:
	??_C@_0P@MPAKFNGN@?$CFs?3?5ai_free?5?$CFs?$AA@ (0000)
0024C7A0 001f:
	??_C@_0BP@HEGBFLAM@?$CFs?3?5ai_free_units?5?$DMsome?5units?$DO?$AA@ (0000)
0024C7C0 0036:
	??_C@_0DG@OFCIAFCL@?$CFs?3?5ai_attach_free?5?$CFs?5cannot?5be?5@ (0000)
0024C7F8 001d:
	??_C@_0BN@IEADAMKD@?$CFs?3?5ai_attach_free?50x?$CF04X?5?$CFs?$AA@ (0000)
0024C818 001a:
	??_C@_0BK@NGFIDLOP@?$CFs?3?5ai_force_active?5?$CFs?5?$CFs?$AA@ (0000)
0024C838 0063:
	??_C@_0GD@LKDLLFKG@ai_force_active_by_unit?3?5unit?5is@ (0000)
0024C89C 002b:
	??_C@_0CL@INBDPDM@?$CFs?3?5ai_force_active_by_unit?5?$DMsom@ (0000)
0024C8C8 001e:
	??_C@_0BO@GELBKNIM@?$CFs?3?5ai_set_return_state?5?$CFs?5?$CFd?$AA@ (0000)
0024C8E8 001f:
	??_C@_0BP@OMBCPLPA@?$CFs?3?5ai_set_current_state?5?$CFs?5?$CFd?$AA@ (0000)
0024C908 0011:
	??_C@_0BB@DKMLPAFC@?$CFs?3?5ai_status?5?$CFs?$AA@ (0000)
0024C91C 0017:
	??_C@_0BH@LGGEBDHB@?$CFs?3?5ai_playfight?5?$CFs?5?$CFs?$AA@ (0000)
0024C934 0028:
	??_C@_0CI@NHKIHJII@?$CFs?3?5ai_vehicle_encounter?5?$DMsome?5u@ (0000)
0024C95C 003e:
	??_C@_0DO@IHGIOMGJ@ai_vehicle_enterable?3?5too?5many?5e@ (0000)
0024C99C 0031:
	??_C@_0DB@BDJKMKHA@?$CFs?3?5ai_vehicle_enterable_distanc@ (0000)
0024C9D0 0030:
	??_C@_0DA@FDMBDBPG@?$CFs?3?5ai_vehicle_enterable_team?5?$DMs@ (0000)
0024CA00 0036:
	??_C@_0DG@LLMHMJE@?$CFs?3?5ai_vehicle_enterable_actor_t@ (0000)
0024CA38 0043:
	??_C@_0ED@KEKJMIKK@ai_vehicle_enterable_actors?3?5too@ (0000)
0024CA7C 0032:
	??_C@_0DC@OHFPMGGD@?$CFs?3?5ai_vehicle_enterable_actors?5@ (0000)
0024CAB0 0030:
	??_C@_0DA@KCDIODEC@?$CFs?3?5ai_vehicle_enterable_disable@ (0000)
0024CAE0 0030:
	??_C@_0DA@MEBODMJO@?$CFs?3?5ai_look_at_object?5?$DMsome?5unit@ (0000)
0024CB10 0020:
	??_C@_0CA@PDOKPGNA@?$CFs?3?5ai_stop_looking?5?$DMsome?5unit?$DO?$AA@ (0000)
0024CB30 0028:
	??_C@_0CI@LKLFKNDG@?$CFs?3?5ai_automatic_migration_targe@ (0000)
0024CB58 0020:
	??_C@_0CA@HJOJOKEG@?$CFs?3?5ai_follow_target_disable?5?$CFs?$AA@ (0000)
0024CB78 0020:
	??_C@_0CA@LPFMOJGF@?$CFs?3?5ai_follow_target_players?5?$CFs?$AA@ (0000)
0024CB98 0029:
	??_C@_0CJ@PMNFIMPA@?$CFs?3?5ai_follow_target_unit?5?$CFs?5?$DMso@ (0000)
0024CBC4 001e:
	??_C@_0BO@NGDADOD@?$CFs?3?5ai_follow_target_ai?5?$CFs?5?$CFs?$AA@ (0000)
0024CBE4 001f:
	??_C@_0BP@FMPPICBE@?$CFs?3?5ai_follow_distance?5?$CFs?5?$CF?41f?$AA@ (0000)
0024CC04 0017:
	??_C@_0BH@IGJNNOIC@?$CFs?3?5ai_conversation?5?$CFs?$AA@ (0000)
0024CC1C 001c:
	??_C@_0BM@HGJNKNLN@?$CFs?3?5ai_conversation_stop?5?$CFs?$AA@ (0000)
0024CC38 001f:
	??_C@_0BP@IBOEPNGC@?$CFs?3?5ai_conversation_advance?5?$CFs?$AA@ (0000)
0024CC58 006a:
	??_C@_0GK@GMJOKPPN@ai_link_activation?3?5cannot?5link?5@ (0000)
0024CCC4 001d:
	??_C@_0BN@OAHKKHJB@?$CFs?3?5ai_link_activation?5?$CFs?5?$CFs?$AA@ (0000)
0024CCE4 0012:
	??_C@_0BC@GFJPNDGO@?$CFs?3?5ai_berserk?5?$CFs?$AA@ (0000)
0024CCF8 001b:
	??_C@_0BL@MKHHLLAC@?$CFs?3?5ai_allow_dormant?5?$CFs?5?$CFs?$AA@ (0000)
0024CD14 0025:
	??_C@_0CF@EKNEFNDI@?$CFs?3?5ai_magically_see_encounter?5?$CF@ (0000)
0024CD3C 0020:
	??_C@_0CA@ENGIEHJO@?$CFs?3?5ai_magically_see_players?5?$CFs?$AA@ (0000)
0024CD5C 0022:
	??_C@_0CC@LGDPOIEJ@?$CFs?3?5ai_go_to_vehicle?5?$CFs?50x?$CF04X?5?$CF@ (0000)
0024CD80 002b:
	??_C@_0CL@HNAPEBL@?$CFs?3?5ai_go_to_vehicle_override?5?$CFs@ (0000)
0024CDAC 0010:
	??_C@_0BA@GHFHPDND@?$CFs?3?5ai_renew?5?$CFs?$AA@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "ai/actions.h"
#include "ai/actor_iterators.h"
#include "ai/actor_looking.h"
#include "ai/ai_communication.h"
#include "ai/ai_debug.h"
#include "ai/encounters.h"
#include "ai/ai_script.h"
#include "cseries/errors.h"
#include "game/game_allegiance.h"
#include "hs/hs.h"
#include "hs/object_lists.h"
#include "memory/data.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "units/units.h"
#include "units/vehicle_scripting.h"

/* ---------- constants */

enum
{
	_ai_count_living = 0,
	_ai_count_swarm,
	_ai_count_nonswarm,
	NUMBER_OF_AI_COUNT_TYPES,
};

/* ---------- macros */

/* ---------- structures */

struct ai_script_globals_prefix
{
	boolean ai_active;
	boolean ai_initialized_for_map;
};

struct actor_iterator
{
	struct data_iterator encounter_iterator;
	boolean iterated_encounterless_list;
	boolean active_only;
	byte pad[2];
	long index;
	long next_index;
};

struct ai_script_vehicle_candidate
{
	long actor_index;
	real distance_squared;
	boolean already_going_to_vehicle;
	byte pad[3];
};

struct ai_script_conversation_definition
{
	char name[32];
	byte unknown[84];
};

typedef char ai_script_actor_iterator_size_assert[
	sizeof(struct actor_iterator) == 0x1C ? 1 : -1];
typedef char ai_script_actor_reference_iterator_size_assert[
	sizeof(struct ai_script_actor_reference_iterator) == 0x18 ? 1 : -1];
typedef char ai_script_actor_reference_iterator_actor_index_offset_assert[
	offsetof(struct ai_script_actor_reference_iterator, actor_index) == 0x10 ? 1 : -1];
typedef char ai_script_vehicle_enterable_size_assert[
	sizeof(struct ai_script_vehicle_enterable) == 0xC ? 1 : -1];
typedef char ai_script_vehicle_enterable_radius_offset_assert[
	offsetof(struct ai_script_vehicle_enterable, radius) == 0x4 ? 1 : -1];
typedef char ai_script_vehicle_enterable_team_offset_assert[
	offsetof(struct ai_script_vehicle_enterable, team_bitmask) == 0x8 ? 1 : -1];
typedef char ai_script_vehicle_enterable_actor_type_offset_assert[
	offsetof(struct ai_script_vehicle_enterable, actor_type_bitmask) == 0xA ? 1 : -1];
typedef char ai_script_platoon_iterator_size_assert[
	sizeof(struct ai_script_platoon_iterator) == 0xC ? 1 : -1];
typedef char ai_script_vehicle_candidate_size_assert[
	sizeof(struct ai_script_vehicle_candidate) == 0xC ? 1 : -1];
typedef char ai_script_vehicle_candidate_distance_offset_assert[
	offsetof(struct ai_script_vehicle_candidate, distance_squared) == 0x4 ? 1 : -1];
typedef char ai_script_vehicle_candidate_state_offset_assert[
	offsetof(struct ai_script_vehicle_candidate, already_going_to_vehicle) == 0x8 ? 1 : -1];

/* ---------- prototypes */

void ai_debug_select_actor(
	long encounter_index,
	long actor_index);
void ai_scripting_maneuver(
	long ai_index);
boolean ai_conversation(
	short conversation_index,
	boolean scripted);
void actor_delete(
	long actor_index,
	boolean died);
void actor_kill(
	long actor_index,
	boolean silent,
	boolean delayed);
static void code_000432b0(
	long ai_reference,
	boolean silent);
static long code_000439c0(
	long ai_reference,
	short count_type,
	long *original_count_reference,
	real *strength_reference);
static int code_00044ea0(
	void const *candidate0,
	void const *candidate1);
static void code_00047160(
	long ai_reference,
	long unit_index,
	char const *seat_substring_name,
	boolean override_previous_vehicles);

/* ---------- globals */

extern struct ai_script_globals_prefix *ai_globals;

/* ---------- public code */

void ai_script_initialize(
	void)
{
	return;
}

void ai_script_dispose(
	void)
{
	return;
}

void ai_script_initialize_for_new_map(
	void)
{
	return;
}

void ai_script_dispose_from_old_map(
	void)
{
	return;
}

void ai_scripting_reconnect(
	void)
{
	return;
}

short ai_scripting_conversation_line(
	short conversation_index)
{
	return ai_conversation_line(conversation_index);
}

short ai_scripting_conversation_status(
	short conversation_index)
{
	return ai_conversation_status(conversation_index);
}

void ai_scripting_retreat(
	long ai_reference)
{
	ai_scripting_maneuver(ai_reference);

	return;
}

void ai_scripting_deselect(
	void)
{
	if (ai_globals->ai_initialized_for_map)
	{
		ai_debug_select_actor(NONE, NONE);
	}

	return;
}

void ai_scripting_attach_units(
	long object_list_index,
	long ai_reference)
{
	long reference_index;
	long unit_index = object_list_get_first(object_list_index, &reference_index);

	while (unit_index != NONE)
	{
		ai_scripting_attach_unit(unit_index, ai_reference);
		unit_index = object_list_get_next(object_list_index, &reference_index);
	}

	return;
}

void ai_scripting_magically_see_units(
	long ai_reference,
	long object_list_index)
{
	long reference_index;
	long unit_index = object_list_get_first(object_list_index, &reference_index);

	while (unit_index != NONE)
	{
		ai_scripting_magically_see_unit(ai_reference, unit_index);
		unit_index = object_list_get_next(object_list_index, &reference_index);
	}

	return;
}

void ai_scripting_set_respawn(
	long ai_reference,
	boolean respawn)
{
	if (ai_debug.print_scripting)
	{
		char ai_name[256];
		ai_index_to_string(
			ai_reference,
			global_scenario_get(),
			ai_name,
			sizeof(ai_name));
		error(
			_error_silent,
			"%s: ai_set_respawn %s %s",
			hs_runtime_get_executing_thread_name(),
			ai_name,
			respawn ? "on" : "off");
	}

	if (ai_reference != NONE)
		encounter_set_respawn(ai_reference & UNSIGNED_SHORT_MAX, respawn);

	return;
}

void ai_scripting_set_deaf(
	long ai_reference,
	boolean deaf)
{
	if (ai_debug.print_scripting)
	{
		char ai_name[256];
		ai_index_to_string(
			ai_reference,
			global_scenario_get(),
			ai_name,
			sizeof(ai_name));
		error(
			_error_silent,
			"%s: ai_set_deaf %s %s",
			hs_runtime_get_executing_thread_name(),
			ai_name,
			deaf ? "on" : "off");
	}

	if (ai_reference != NONE)
		encounter_set_deaf(ai_reference & UNSIGNED_SHORT_MAX, deaf);

	return;
}

void ai_scripting_set_blind(
	long ai_reference,
	boolean blind)
{
	if (ai_debug.print_scripting)
	{
		char ai_name[256];
		ai_index_to_string(
			ai_reference,
			global_scenario_get(),
			ai_name,
			sizeof(ai_name));
		error(
			_error_silent,
			"%s: ai_set_blind %s %s",
			hs_runtime_get_executing_thread_name(),
			ai_name,
			blind ? "on" : "off");
	}

	if (ai_reference != NONE)
		encounter_set_blind(ai_reference & UNSIGNED_SHORT_MAX, blind);

	return;
}

void ai_scripting_vehicle_enterable_distance(
	long unit_index,
	real distance)
{
	if (ai_debug.print_scripting)
	{
		error(
			_error_silent,
			"%s: ai_vehicle_enterable_distance <some vehicle>",
			hs_runtime_get_executing_thread_name());
	}

	if (unit_index != NONE)
	{
		struct ai_script_vehicle_enterable *vehicle_enterable =
			ai_scripting_find_vehicle_enterable(unit_index);
		if (vehicle_enterable)
			vehicle_enterable->radius = distance;
	}

	return;
}

void ai_scripting_vehicle_enterable_team(
	long unit_index,
	long team_index)
{
	short team = team_index;

	if (ai_debug.print_scripting)
	{
		error(
			_error_silent,
			"%s: ai_vehicle_enterable_team <some vehicle> %d",
			hs_runtime_get_executing_thread_name(),
			team);
	}

	if (unit_index != NONE)
	{
		struct ai_script_vehicle_enterable *vehicle_enterable =
			ai_scripting_find_vehicle_enterable(unit_index);
		if (vehicle_enterable)
			vehicle_enterable->team_bitmask |= 1 << team;
	}

	return;
}

void ai_scripting_vehicle_enterable_actor_type(
	long unit_index,
	long actor_type)
{
	short type = actor_type;

	if (ai_debug.print_scripting)
	{
		error(
			_error_silent,
			"%s: ai_vehicle_enterable_actor_type <some vehicle> %d",
			hs_runtime_get_executing_thread_name(),
			type);
	}

	if (unit_index != NONE)
	{
		struct ai_script_vehicle_enterable *vehicle_enterable =
			ai_scripting_find_vehicle_enterable(unit_index);
		if (vehicle_enterable)
			vehicle_enterable->actor_type_bitmask |= 1 << type;
	}

	return;
}

void ai_scripting_detach_unit(
	long unit_index)
{
	if (ai_debug.print_scripting)
	{
		error(
			_error_silent,
			"%s: ai_detach_unit 0x%04X",
			hs_runtime_get_executing_thread_name(),
			unit_index & UNSIGNED_SHORT_MAX);
	}

	if (unit_index != NONE)
	{
		long actor_index = unit_get(unit_index)->unit.actor_index;
		if (actor_index != NONE)
			actor_delete(actor_index, FALSE);
	}

	return;
}

void ai_scripting_kill(
	long ai_reference)
{
	if (ai_debug.print_scripting)
	{
		char ai_name[256];
		ai_index_to_string(
			ai_reference,
			global_scenario_get(),
			ai_name,
			sizeof(ai_name));
		error(
			_error_silent,
			"%s: ai_kill %s",
			hs_runtime_get_executing_thread_name(),
			ai_name);
	}

	code_000432b0(ai_reference, FALSE);

	return;
}

void ai_scripting_kill_silent(
	long ai_reference)
{
	if (ai_debug.print_scripting)
	{
		char ai_name[256];
		ai_index_to_string(
			ai_reference,
			global_scenario_get(),
			ai_name,
			sizeof(ai_name));
		error(
			_error_silent,
			"%s: ai_kill_silent %s",
			hs_runtime_get_executing_thread_name(),
			ai_name);
	}

	code_000432b0(ai_reference, TRUE);

	return;
}

short ai_scripting_swarm_count(
	long ai_reference)
{
	return code_000439c0(ai_reference, _ai_count_swarm, NULL, NULL);
}

short ai_scripting_nonswarm_count(
	long ai_reference)
{
	return code_000439c0(ai_reference, _ai_count_nonswarm, NULL, NULL);
}

short ai_scripting_living_count(
	long ai_reference)
{
	return code_000439c0(ai_reference, _ai_count_living, NULL, NULL);
}

real ai_scripting_living_fraction(
	long ai_reference)
{
	real result = 0.0f;
	long original_count;
	long count = code_000439c0(
		ai_reference,
		_ai_count_living,
		&original_count,
		NULL);

	if (original_count > 0)
		result = (real)count / (real)original_count;

	return result;
}

real ai_scripting_strength(
	long ai_reference)
{
	real strength = 0.0f;
	code_000439c0(ai_reference, _ai_count_living, NULL, &strength);
	return strength;
}

boolean ai_scripting_is_attacking(
	long ai_reference)
{
	struct ai_script_platoon_iterator iterator;
	struct platoon_datum *platoon;
	boolean attacking = FALSE;

	if (ai_reference != NONE)
	{
		ai_index_platoon_iterator_new(ai_reference, &iterator);

		platoon = ai_index_platoon_iterator_next(&iterator);
		while (platoon)
		{
			if (!platoon->defending)
			{
				attacking = TRUE;
				break;
			}

			platoon = ai_index_platoon_iterator_next(&iterator);
		}
	}

	return attacking;
}

short ai_scripting_going_to_vehicle(
	long unit_index)
{
	struct actor_iterator iterator;
	struct actor_datum *actor;
	short count = 0;

	actor_iterator_new(&iterator, TRUE);
	actor = actor_iterator_next(&iterator);
	while (actor)
	{
		if (actor->state.action == _actor_action_vehicle &&
			actor->state.action_data.vehicle.vehicle_index == unit_index)
		{
			count++;
		}

		actor = actor_iterator_next(&iterator);
	}

	return count;
}

void ai_scripting_allegiance_remove(
	short team1_index,
	short team2_index)
{
	if (ai_debug.print_scripting)
	{
		error(
			_error_silent,
			"%s: ai_allegiance_remove %d %d",
			hs_runtime_get_executing_thread_name(),
			team1_index,
			team2_index);
	}

	if (team1_index != NONE && team2_index != NONE)
		game_allegiance_remove(team1_index, team2_index);

	return;
}

boolean ai_scripting_conversation(
	long conversation_index)
{
	short conversation = conversation_index;

	if (ai_debug.print_scripting)
	{
		struct scenario *scenario;
		char const *conversation_name;

		scenario = global_scenario_get();
		conversation_name = "<error>";
		if (VALID_INDEX(conversation, scenario->ai_conversations.count))
		{
			conversation_name = TAG_BLOCK_GET_ELEMENT(
				&scenario->ai_conversations,
				conversation,
				struct ai_script_conversation_definition)->name;
		}

		error(
			_error_silent,
			"%s: ai_conversation %s",
			hs_runtime_get_executing_thread_name(),
			conversation_name);
	}

	return ai_conversation(conversation, TRUE);
}

void ai_scripting_conversation_stop(
	long conversation_index)
{
	short conversation = conversation_index;

	if (ai_debug.print_scripting)
	{
		struct scenario *scenario;
		char const *conversation_name;

		scenario = global_scenario_get();
		conversation_name = "<error>";
		if (VALID_INDEX(conversation, scenario->ai_conversations.count))
		{
			conversation_name = TAG_BLOCK_GET_ELEMENT(
				&scenario->ai_conversations,
				conversation,
				struct ai_script_conversation_definition)->name;
		}

		error(
			_error_silent,
			"%s: ai_conversation_stop %s",
			hs_runtime_get_executing_thread_name(),
			conversation_name);
	}

	ai_conversation_stop(conversation);

	return;
}

void ai_scripting_conversation_advance(
	long conversation_index)
{
	short conversation = conversation_index;

	if (ai_debug.print_scripting)
	{
		struct scenario *scenario;
		char const *conversation_name;

		scenario = global_scenario_get();
		conversation_name = "<error>";
		if (VALID_INDEX(conversation, scenario->ai_conversations.count))
		{
			conversation_name = TAG_BLOCK_GET_ELEMENT(
				&scenario->ai_conversations,
				conversation,
				struct ai_script_conversation_definition)->name;
		}

		error(
			_error_silent,
			"%s: ai_conversation_advance %s",
			hs_runtime_get_executing_thread_name(),
			conversation_name);
	}

	ai_conversation_advance(conversation);

	return;
}

void ai_scripting_stop_looking(
	long unit_index)
{
	if (ai_debug.print_scripting)
	{
		error(
			_error_silent,
			"%s: ai_stop_looking <some unit>",
			hs_runtime_get_executing_thread_name());
	}

	if (unit_index != NONE)
	{
		long actor_index = unit_get(unit_index)->unit.actor_index;
		if (actor_index != NONE)
			actor_look_secondary_stop(actor_index);
	}

	return;
}

void ai_scripting_go_to_vehicle(
	long ai_reference,
	long unit_index,
	char const *seat_substring_name)
{
	if (ai_debug.print_scripting)
	{
		char ai_name[256];
		ai_index_to_string(
			ai_reference,
			global_scenario_get(),
			ai_name,
			sizeof(ai_name));
		error(
			_error_silent,
			"%s: ai_go_to_vehicle %s 0x%04X %s",
			hs_runtime_get_executing_thread_name(),
			ai_name,
			unit_index & UNSIGNED_SHORT_MAX,
			seat_substring_name);
	}

	code_00047160(ai_reference, unit_index, seat_substring_name, FALSE);

	return;
}

void ai_scripting_go_to_vehicle_override(
	long ai_reference,
	long unit_index,
	char const *seat_substring_name)
{
	if (ai_debug.print_scripting)
	{
		char ai_name[256];
		ai_index_to_string(
			ai_reference,
			global_scenario_get(),
			ai_name,
			sizeof(ai_name));
		error(
			_error_silent,
			"%s: ai_go_to_vehicle_override %s 0x%04X %s",
			hs_runtime_get_executing_thread_name(),
			ai_name,
			unit_index & UNSIGNED_SHORT_MAX,
			seat_substring_name);
	}

	code_00047160(ai_reference, unit_index, seat_substring_name, TRUE);

	return;
}

/* ---------- private code */

static int code_00044ea0(
	void const *candidate0,
	void const *candidate1)
{
	struct ai_script_vehicle_candidate const *vehicle0 = candidate0;
	struct ai_script_vehicle_candidate const *vehicle1 = candidate1;

	if (vehicle0->already_going_to_vehicle != vehicle1->already_going_to_vehicle)
		return vehicle0->already_going_to_vehicle ? 1 : -1;

	if (vehicle0->distance_squared < vehicle1->distance_squared)
		return -1;

	if (vehicle0->distance_squared > vehicle1->distance_squared)
		return 1;

	return 0;
}

static void code_00047160(
	long ai_reference,
	long unit_index,
	char const *seat_substring_name,
	boolean override_previous_vehicles)
{
	struct unit_datum *vehicle;
	real_point3d vehicle_origin;
	short seat_indices[16];
	struct ai_script_vehicle_candidate candidates[64];
	struct ai_script_actor_reference_iterator iterator;
	struct actor_datum *actor;
	short candidate_count;
	short available_seat_count;
	short candidate_index;

	vehicle = unit_try_and_get(unit_index);
	if (ai_reference == NONE || vehicle == NULL)
		return;

	candidate_count = 0;
	object_get_origin(unit_index, &vehicle_origin);
	available_seat_count = vehicle_scripting_find_available_seats(
		unit_index,
		seat_substring_name,
		NONE,
		seat_indices,
		NUMBEROF(seat_indices));
	if (available_seat_count <= 0)
		return;

	ai_index_actor_iterator_new(ai_reference, &iterator);
	actor = ai_index_actor_iterator_next(&iterator);
	while (actor)
	{
		if ((word)candidate_count < NUMBEROF(candidates))
		{
			real dx;
			real dy;
			real dz;

			candidates[candidate_count].actor_index = iterator.actor_index;
			dx = vehicle_origin.x - actor->input.position.body_position.x;
			dy = vehicle_origin.y - actor->input.position.body_position.y;
			dz = vehicle_origin.z - actor->input.position.body_position.z;

			candidates[candidate_count].distance_squared =
				dy * dy + (dx * dx + dz * dz);
			candidates[candidate_count].already_going_to_vehicle =
				actor->state.action == _actor_action_vehicle;
			candidate_count++;
		}

		actor = ai_index_actor_iterator_next(&iterator);
	}

	qsort(candidates, candidate_count, sizeof(candidates[0]), code_00044ea0);

	for (candidate_index = 0; candidate_index < candidate_count; candidate_index++)
	{
		if (candidates[candidate_index].already_going_to_vehicle &&
			!override_previous_vehicles)
		{
			break;
		}

		actor_action_try_to_enter_vehicle(
			candidates[candidate_index].actor_index,
			unit_index,
			NULL,
			NONE,
			available_seat_count,
			seat_indices);
	}

	return;
}

static void code_000432b0(
	long ai_reference,
	boolean silent)
{
	struct ai_script_actor_reference_iterator iterator;

	if (ai_reference == NONE)
		return;

	ai_index_actor_iterator_new(ai_reference, &iterator);
	while (ai_index_actor_iterator_next(&iterator))
		actor_kill(iterator.actor_index, silent, FALSE);

	return;
}

static long code_000439c0(
	long ai_reference,
	short count_type,
	long *original_count_reference,
	real *strength_reference)
{
	long count = 0;
	long original_count = 0;
	real strength = 0.0f;

	match_assert(
		"c:\\halo\\SOURCE\\ai\\ai_script.c",
		1029,
		(count_type >= 0) && (count_type < NUMBER_OF_AI_COUNT_TYPES));

	if (ai_reference != NONE)
	{
		struct scenario *scenario = global_scenario_get();

		switch ((unsigned long)ai_reference >> 30)
		{
		case 0:
		{
			long encounter_index = ai_reference & UNSIGNED_SHORT_MAX;
			if (VALID_INDEX(encounter_index, scenario->ai_encounters.count))
			{
				struct encounter_datum *encounter = encounter_get(encounter_index);

				switch (count_type)
				{
				case _ai_count_living:
					count = encounter->current_count;
					break;
				case _ai_count_swarm:
					count = encounter->current_swarm_count;
					break;
				case _ai_count_nonswarm:
					count = FLOOR(encounter->current_count - encounter->current_swarm_count, 0);
					break;
				default:
					match_assert("c:\\halo\\SOURCE\\ai\\ai_script.c", 1060, !"unreachable");
					break;
				}

				original_count = encounter->original_count;
				strength = encounter->current_strength_fraction;
			}
			break;
		}
		case 1:
		{
			long encounter_index = ai_reference & UNSIGNED_SHORT_MAX;
			if (VALID_INDEX(encounter_index, scenario->ai_encounters.count))
			{
				struct encounter_datum *encounter = encounter_get(encounter_index);
				short platoon_index = (ai_reference >> 16) & UNSIGNED_CHAR_MAX;

				if (VALID_INDEX(platoon_index, encounter->platoon_count))
				{
					struct platoon_datum *platoon = encounter_get_platoon(encounter, platoon_index);

					switch (count_type)
					{
					case _ai_count_living:
						count = platoon->current_count;
						break;
					case _ai_count_swarm:
						count = platoon->current_swarm_count;
						break;
					case _ai_count_nonswarm:
						count = FLOOR(platoon->current_count - platoon->current_swarm_count, 0);
						break;
					default:
						match_assert("c:\\halo\\SOURCE\\ai\\ai_script.c", 1096, !"unreachable");
						break;
					}

					original_count = platoon->original_count;
					strength = platoon->current_strength_fraction;
				}
			}
			break;
		}
		case 2:
		{
			long encounter_index = ai_reference & UNSIGNED_SHORT_MAX;
			if (VALID_INDEX(encounter_index, scenario->ai_encounters.count))
			{
				struct encounter_datum *encounter = encounter_get(encounter_index);
				short squad_index = (ai_reference >> 16) & UNSIGNED_CHAR_MAX;

				if (VALID_INDEX(squad_index, encounter->squad_count))
				{
					struct squad_datum *squad = encounter_get_squad(encounter, squad_index);

					switch (count_type)
					{
					case _ai_count_living:
						count = squad->current_count;
						break;
					case _ai_count_swarm:
						count = squad->current_swarm_count;
						break;
					case _ai_count_nonswarm:
						count = FLOOR(squad->current_count - squad->current_swarm_count, 0);
						break;
					default:
						match_assert("c:\\halo\\SOURCE\\ai\\ai_script.c", 1133, !"unreachable");
						break;
					}

					original_count = squad->original_count;
					strength = squad->current_strength_fraction;
				}
			}
			break;
		}
		default:
			match_assert("c:\\halo\\SOURCE\\ai\\ai_script.c", 1143, !"unreachable");
			break;
		}
	}

	if (original_count_reference)
		*original_count_reference = original_count;
	if (strength_reference)
		*strength_reference = strength;

	return count;
}
