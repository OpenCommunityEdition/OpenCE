/*
AI_COMMUNICATION.C

symbols in this file:
00031040 0150:
	_ai_communication_initialize (0000)
00031190 0010:
	_ai_communication_dispose (0000)
000311A0 0110:
	_ai_communication_initialize_for_new_map (0000)
000312B0 0010:
	_ai_communication_dispose_from_old_map (0000)
000312C0 0030:
	_ai_communication_get_type_name (0000)
000312F0 0040:
	_ai_communication_get_type_by_name (0000)
00031330 0060:
	_ai_communication_packet_new (0000)
00031390 0070:
	_reply_filter_close (0000)
00031400 0070:
	_reply_filter_not_close (0000)
00031470 0050:
	_reply_filter_searching (0000)
000314C0 0090:
	_code_000314c0 (0000)
00031550 0020:
	_code_00031550 (0000)
00031570 0040:
	_code_00031570 (0000)
000315B0 00b0:
	_code_000315b0 (0000)
00031660 0040:
	_reply_filter_no_certain_target (0000)
000316A0 0040:
	_reply_filter_flee_leader (0000)
000316E0 01a0:
	_code_000316e0 (0000)
00031880 0040:
	_actor_communication_team (0000)
000318C0 00b0:
	_code_000318c0 (0000)
00031970 0050:
	_code_00031970 (0000)
000319C0 0110:
	_ai_conversation_status (0000)
00031AD0 0060:
	_ai_conversation_line (0000)
00031B30 0090:
	_ai_conversation_advance (0000)
00031BC0 0190:
	_ai_conversation_finish (0000)
00031D50 0130:
	_ai_conversation_new (0000)
00031E80 01b0:
	_code_00031e80 (0000)
00032030 0290:
	_code_00032030 (0000)
000322C0 0030:
	_code_000322c0 (0000)
000322F0 00d0:
	_actor_reset_idle_vocalization_timer (0000)
000323C0 00f0:
	_actor_communication_update (0000)
000324B0 0080:
	_code_000324b0 (0000)
00032530 02a0:
	_ai_communication_update_speech_timers (0000)
000327D0 0340:
	_ai_communication_get_player_rating (0000)
00032B10 0090:
	_ai_conversation_stop (0000)
00032BA0 00d0:
	_ai_conversation_actor_deleted (0000)
00032C70 0170:
	_ai_conversation_unit_died (0000)
00032DE0 0800:
	_code_00032de0 (0000)
000335E0 02c0:
	_ai_communication_started (0000)
000338A0 0210:
	_ai_communication_notify (0000)
00033AB0 0390:
	_code_00033ab0 (0000)
00033E40 00c0:
	_code_00033e40 (0000)
00033F00 0120:
	_code_00033f00 (0000)
00034020 06d0:
	_code_00034020 (0000)
000346F0 0450:
	_code_000346f0 (0000)
00034B40 0630:
	_ai_communication_finished (0000)
00035170 0150:
	_ai_conversation (0000)
000352C0 0260:
	_ai_conversation_update (0000)
00035520 1f80:
	_ai_communication_event (0000)
00247808 18c0:
	_communication_speech_priorities (0000)
	_communication_notification_delays (0010)
	_communication_protagonist_default_look_priorities (0030)
	_communication_recipient_default_look_priorities (0040)
	_communication_player_speaking_priorities (0050)
	_communication_unit_prefer_silent_time (0060)
	_communication_timer_tolerances (0068)
	_communication_play_delays (01a8)
	_communication_player_additional_delay (01bc)
	_communication_overlap_time_modifier (01c0)
	_communication_timeout_low_priority_modifier (01c4)
	_communication_repeat_selection_time (01c8)
	_communication_player_absolute_range (01cc)
	_communication_player_ideal_range_min (01d0)
	_communication_player_ideal_range_max (01d4)
	_communication_player_ideal_fov (01d8)
	_communication_player_rating_low_priority (01dc)
	_global_dialogue_table (01e0)
	_global_reply_table (1248)
002490C8 0015:
	??_C@_0BF@PLOFNBGK@postcombat_celebrate?$AA@ (0000)
002490E0 0018:
	??_C@_0BI@BNIOPALA@postcombat_shoot_corpse?$AA@ (0000)
002490F8 0018:
	??_C@_0BI@LCABFNHO@postcombat_check_friend?$AA@ (0000)
00249110 0017:
	??_C@_0BH@JEELJBED@postcombat_check_enemy?$AA@ (0000)
00249128 0013:
	??_C@_0BD@MKOGOGNM@postcombat_triumph?$AA@ (0000)
0024913C 0014:
	??_C@_0BE@OGGIDGFC@postcombat_massacre?$AA@ (0000)
00249150 0013:
	??_C@_0BD@OKBEBJFP@postcombat_wounded?$AA@ (0000)
00249164 0015:
	??_C@_0BF@OFGIJICH@postcombat_unscathed?$AA@ (0000)
0024917C 0011:
	??_C@_0BB@LGDDKHGB@postcombat_alone?$AA@ (0000)
00249190 0005:
	??_C@_04PMNENHO@leap?$AA@ (0000)
00249198 0008:
	??_C@_07KMLHDIFF@falling?$AA@ (0000)
002491A0 0014:
	??_C@_0BE@DIJKKGAP@uncover_exclamation?$AA@ (0000)
002491B4 0005:
	??_C@_04FLLHOKOG@dive?$AA@ (0000)
002491BC 0006:
	??_C@_05IMODGEHH@melee?$AA@ (0000)
002491C4 0009:
	??_C@_08MPLMAFLL@surprise?$AA@ (0000)
002491D0 0010:
	??_C@_0BA@HBKFDHMB@vehicle_falling?$AA@ (0000)
002491E0 000f:
	??_C@_0P@EOJFLCO@vehicle_scared?$AA@ (0000)
002491F0 000f:
	??_C@_0P@OJJDOKMB@vehicle_woohoo?$AA@ (0000)
00249200 000d:
	??_C@_0N@KIHOLEBN@vehicle_exit?$AA@ (0000)
00249210 000e:
	??_C@_0O@NEANEBAA@vehicle_entry?$AA@ (0000)
00249220 0010:
	??_C@_0BA@JKBMANAN@hiding_finished?$AA@ (0000)
00249230 000f:
	??_C@_0P@BOOLPEN@attempted_flee?$AA@ (0000)
00249240 000a:
	??_C@_09LPKHDOHP@flee_idle?$AA@ (0000)
0024924C 0011:
	??_C@_0BB@GDNLPHOF@flee_leader_died?$AA@ (0000)
00249260 0011:
	??_C@_0BB@GKOPGPMO@shooting_traitor?$AA@ (0000)
00249274 000f:
	??_C@_0P@IDHEPNJE@shooting_group?$AA@ (0000)
00249284 0011:
	??_C@_0BB@LKALKDPF@shooting_berserk?$AA@ (0000)
00249298 0011:
	??_C@_0BB@GHLHMONC@shooting_vehicle?$AA@ (0000)
002492AC 0009:
	??_C@_08HFADMLAA@shooting?$AA@ (0000)
002492B8 0016:
	??_C@_0BG@IIEIONGD@sighted_friend_player?$AA@ (0000)
002492D0 0008:
	??_C@_07LADJDDJF@retreat?$AA@ (0000)
002492D8 0008:
	??_C@_07JKODCMP@advance?$AA@ (0000)
002492E0 000e:
	??_C@_0O@OAGOMGLF@uncover_start?$AA@ (0000)
002492F0 0015:
	??_C@_0BF@HNJCJGJJ@search_group_abandon?$AA@ (0000)
00249308 000f:
	??_C@_0P@GDPJMJHJ@search_abandon?$AA@ (0000)
00249318 000e:
	??_C@_0O@EKDNJHKG@search_report?$AA@ (0000)
00249328 000d:
	??_C@_0N@MDODPJCI@search_query?$AA@ (0000)
00249338 000d:
	??_C@_0N@IJIHJIIJ@search_start?$AA@ (0000)
00249348 0010:
	??_C@_0BA@KFNIENAA@alert_noncombat?$AA@ (0000)
00249358 0008:
	??_C@_07MAJPMGLJ@blocked?$AA@ (0000)
00249360 000d:
	??_C@_0N@JEODFMIK@lost_contact?$AA@ (0000)
00249370 000f:
	??_C@_0P@JELOHPAB@grenade_danger?$AA@ (0000)
00249380 0010:
	??_C@_0BA@GHAABFAJ@grenade_sighted?$AA@ (0000)
00249390 0010:
	??_C@_0BA@ILMKIOPH@grenade_startle?$AA@ (0000)
002493A0 0011:
	??_C@_0BB@NDMOOLOJ@grenade_throwing?$AA@ (0000)
002493B4 0013:
	??_C@_0BD@JHAEBNEM@allegiance_changed?$AA@ (0000)
002493C8 0012:
	??_C@_0BC@LJDIENED@found_dead_friend?$AA@ (0000)
002493DC 0011:
	??_C@_0BB@HACOIKAB@unexpected_enemy?$AA@ (0000)
002493F0 000c:
	??_C@_0M@EJKFHLEL@found_enemy?$AA@ (0000)
002493FC 000e:
	??_C@_0O@CDKKCOME@sighted_enemy?$AA@ (0000)
0024940C 0005:
	??_C@_04NAFIGEAM@hurt?$AA@ (0000)
00249414 000e:
	??_C@_0O@EGHFODNC@killing_spree?$AA@ (0000)
00249424 0006:
	??_C@_05PBMMFKGI@death?$AA@ (0000)
0024942C 0004:
	??_C@_03MADNOHBP@COV?$AA@ (0000)
00249430 0009:
	??_C@_08ONAJMEN@covenant?$AA@ (0000)
0024943C 0004:
	??_C@_03KPLLINCC@HUM?$AA@ (0000)
00249440 0006:
	??_C@_05OAIPHEDE@human?$AA@ (0000)
00249448 0008:
	??_C@_07BLDMICJO@exclaim?$AA@ (0000)
00249450 0005:
	??_C@_04EJGCJLIC@yell?$AA@ (0000)
00249458 000c:
	??_C@_0M@JHEHDIOO@communicate?$AA@ (0000)
00249464 0005:
	??_C@_04HBNBFOKD@talk?$AA@ (0000)
0024946C 0008:
	??_C@_07EFCDKMI@chatter?$AA@ (0000)
00249474 0007:
	??_C@_06NLCJMHIE@filler?$AA@ (0000)
0024947C 0012:
	??_C@_0BC@ONDNPAEP@conversation_data?$AA@ (0000)
00249490 0010:
	??_C@_0BA@HJGPPPBN@ai?5conversation?$AA@ (0000)
002494A0 0048:
	??_C@_0EI@HLDOMHAD@ai_communication_initialize?3?5una@ (0000)
002494E8 0019:
	??_C@_0BJ@DEJOBHIO@ai?5communication?5replies?$AA@ (0000)
00249508 004b:
	??_C@_0EL@IJCKEONL@ai_communication_initialize?3?5una@ (0000)
00249554 0025:
	??_C@_0CF@ENENAALK@c?3?2halo?2SOURCE?2ai?2ai_communicati@ (0000)
0024957C 001a:
	??_C@_0BK@HKKGFHIF@ai?5communication?5dialogue?$AA@ (0000)
00249598 000c:
	??_C@_0M@FJGFLLPA@information?$AA@ (0000)
002495A4 003a:
	??_C@_0DK@JAPPOFEL@?$CIplay_type?5?$DN?$DN?5_unit_play_speech_@ (0000)
002495E0 000f:
	??_C@_0P@LFGNPAFB@spk?$CFd?$DMtol?$CFd?$CL?$CFd?$AA@ (0000)
002495F0 000a:
	??_C@_09NBNIDDMC@nospch?9?$CFs?$AA@ (0000)
00249600 0040:
	??_C@_0EA@HGHJJMBM@vocalization_type?5?$CG?$CG?5sound_defin@ (0000)
00249640 0021:
	??_C@_0CB@JPNJDDBL@?$CFs?3?5told?5to?5advance?5by?5scripting@ (0000)
00249664 0012:
	??_C@_0BC@NMDPGBEB@?$CFs?3?5finished?5?$CFs?$CFs?$AA@ (0000)
00249678 000c:
	??_C@_0M@PDJPHGJL@prematurely?$AA@ (0000)
00249684 000d:
	??_C@_0N@HAEOLDAP@successfully?$AA@ (0000)
00249694 0013:
	??_C@_0BD@FOMCINHP@?5?$CIunable?5to?5begin?$CJ?$AA@ (0000)
002496A8 0048:
	??_C@_0EI@KMONEJAM@?$CFs?3?5this?5conversation?5is?5already@ (0000)
002496F0 00b5:
	??_C@_0LF@IOBNLMGM@?$CIconversation?9?$DOdialogue_indices?$FL@ (0000)
002497A8 000d:
	??_C@_0N@IDIKJELG@?$CFs?3?5speak?5?$CFs?$AA@ (0000)
002497B8 004b:
	??_C@_0EL@EAKMIEAH@?$CIreply_table_index?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIrep@ (0000)
00249808 0052:
	??_C@_0FC@DCLNMHJF@?$CIdialogue_type_index?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CId@ (0000)
0024985C 0013:
	??_C@_0BD@NKCKMGKJ@?$CFs?5?$CFs?5?$CFd?1?$CFs?3?5?$CFs?5?$CFd?$AA@ (0000)
00249870 0004:
	__real@3eb33333 (0000)
00249874 001e:
	??_C@_0BO@PGLKMLMB@?$CFs?3?5told?5to?5stop?5by?5scripting?$AA@ (0000)
00249894 0018:
	??_C@_0BI@MOODPKBH@?$CFs?3?5unit?5died?0?5aborting?$AA@ (0000)
002498AC 0008:
	??_C@_07CEJJIKE@?$CFs?$CI?$CFd?$CJ?5?$AA@ (0000)
002498B4 0012:
	??_C@_0BC@LGHGCJKF@no?9dialogue?9match?$AA@ (0000)
002498C8 0010:
	??_C@_0BA@LGNJEIF@not?9near?9player?$AA@ (0000)
002498D8 000a:
	??_C@_09DFNMPCAJ@selection?$AA@ (0000)
002498E4 0014:
	??_C@_0BE@FCFFGOIN@nowhere?9near?9player?$AA@ (0000)
002498F8 0013:
	??_C@_0BD@FMPJDKHJ@already?9conversing?$AA@ (0000)
0024990C 000b:
	??_C@_0L@FEFLJKBC@wrong?9type?$AA@ (0000)
00249918 000c:
	??_C@_0M@CFFMKJFH@?5?5reasons?3?5?$AA@ (0000)
00249924 0031:
	??_C@_0DB@PGEHKHFC@?$CFs?3?5didn?8t?5find?5?$CFd?1?$CFs?5in?5?$CFs?5?$CI?$CFd?5@ (0000)
00249958 000b:
	??_C@_0L@COMKDGN@?$DMeveryone?$DO?$AA@ (0000)
00249964 0010:
	??_C@_0BA@ELKHLFDD@?$DMspecific?5unit?$DO?$AA@ (0000)
00249978 006d:
	??_C@_0GN@FHAECEPL@?$CIactor_variant_index?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIa@ (0000)
002499E8 001c:
	??_C@_0BM@CBPILECH@found_variant_index?5?$CB?$DN?5NONE?$AA@ (0000)
00249A08 0056:
	??_C@_0FG@DGHHBBJL@change_variant_indices_count?5?$DM?5M@ (0000)
00249A60 0004:
	__real@3eaaaaab (0000)
00249A64 0004:
	__real@41a20000 (0000)
00249A68 003a:
	??_C@_0DK@CMCFHJEF@nearby_unit_count?5?$DM?5MAXIMUM_PART@ (0000)
00249AA4 0028:
	??_C@_0CI@ICHMPFMM@conversation_definition?5?$CG?$CG?5conve@ (0000)
00249ACC 0007:
	??_C@_06DJHOIPC@?$CFs?3?5?$CFs?$AA@ (0000)
00249AD4 0009:
	??_C@_08BAHEMPNM@?5?$FL?$CFd?1?$CFs?$FN?$AA@ (0000)
00249AE0 0006:
	??_C@_05DNIIFBMG@?$CFs?5?$CFs?$AA@ (0000)
00249AE8 0008:
	??_C@_07GKOCGALF@non?9voc?$AA@ (0000)
00249AF0 000c:
	??_C@_0M@DMDFMBAG@unit?5?$CF04X?3?5?$AA@ (0000)
00249AFC 000c:
	??_C@_0M@HNMGEIHE@player?5?$CFd?3?5?$AA@ (0000)
00249B08 0005:
	??_C@_04PFAOODAA@?$CFs?3?5?$AA@ (0000)
00249B10 0008:
	??_C@_07MMOCNKBD@?$CFs?1?$CFs?3?5?$AA@ (0000)
00249B18 0072:
	??_C@_0HC@PEHCJGNI@?$CIai_information?9?$DOdialogue_type_i@ (0000)
00249B90 0091:
	??_C@_0JB@OPDEEEOP@?$CIconversation?9?$DOdialogue_indices?$FL@ (0000)
00249C24 0030:
	??_C@_0DA@LIHNLMLM@?$CFs?3?5cannot?5start?0?5players?5are?5no@ (0000)
00249C54 0028:
	??_C@_0CI@LELOFLBG@?$CFs?3?5cannot?5start?0?5nobody?5can?5see@ (0000)
00249C80 0047:
	??_C@_0EH@ODDEEICK@?$CFs?3?5participants?5currently?5outsi@ (0000)
00249CC8 0025:
	??_C@_0CF@BJPALGNB@?$CFs?3?5could?5not?5find?5participant?5?$CF@ (0000)
00249CF0 0036:
	??_C@_0DG@BNDGFHIE@?$CFs?3?5found?5participant?5?$CFd?1?$CFs?5but?5@ (0000)
00249D28 0016:
	??_C@_0BG@KFPPJIJM@?$FL?$CFs?50?9player?9chance?$FN?5?$AA@ (0000)
00249D40 0014:
	??_C@_0BE@DLAFFNAP@?$FL?$CFs?5rand?$CF?42f?$DO?$CF?42f?$FN?5?$AA@ (0000)
00249D54 0012:
	??_C@_0BC@GGBDH@?$FL?$CFs?5found?9actor?$FN?5?$AA@ (0000)
00249D68 0012:
	??_C@_0BC@GOGGHGJI@?$FL?$CFs?5?$CFs?9d?9dis?1?$CFd?$FN?5?$AA@ (0000)
00249D7C 000d:
	??_C@_0N@EFEDAFNN@?$FL?$CFs?5nobody?$FN?5?$AA@ (0000)
00249D8C 0015:
	??_C@_0BF@DPPMFKIF@?$FLscripted?9override?$FN?5?$AA@ (0000)
00249DA4 0011:
	??_C@_0BB@HDBKEACA@PLAYER?9REPLY?5?$CFs?5?$AA@ (0000)
00249DB8 0007:
	??_C@_06IKJGFCFO@?$DO?$DO?$CFs?$DM?$DM?$AA@ (0000)
00249DC0 0008:
	??_C@_07GPMPHPH@nobody?5?$AA@ (0000)
00249DC8 000c:
	??_C@_0M@EMMAIOJB@playercant?5?$AA@ (0000)
00249DD4 0009:
	??_C@_08LMAMDFAK@u?9?$CFs?9?$CFs?5?$AA@ (0000)
00249DE0 0004:
	??_C@_03EKLFPLOI@n?1a?$AA@ (0000)
00249DE4 0004:
	??_C@_03CJOLJKCN@dis?$AA@ (0000)
00249DE8 0008:
	??_C@_07CPBKJGBP@filter?5?$AA@ (0000)
00249DF0 000a:
	??_C@_09PGKELFMI@0?9chance?5?$AA@ (0000)
00249DFC 000d:
	??_C@_0N@KOBKEGMJ@rand?9failed?5?$AA@ (0000)
00249E0C 0010:
	??_C@_0BA@MDFGCECJ@rand?5?$CF?42f?$DO?$CF?42f?5?$AA@ (0000)
00249E1C 0011:
	??_C@_0BB@BILJOOCN@?$CBreply_to_player?$AA@ (0000)
00249E30 0013:
	??_C@_0BD@PDIGFKID@override?9scripted?5?$AA@ (0000)
00249E44 000b:
	??_C@_0L@HPKHMDON@wrong?9dmg?5?$AA@ (0000)
00249E50 0004:
	??_C@_03DKEJHHJP@?$CFs?3?$AA@ (0000)
00249E54 000b:
	??_C@_0L@LNGLDCHG@REPLY?5?$CFs?3?5?$AA@ (0000)
00249E60 0047:
	??_C@_0EH@NNNDPBEK@?$CFs?3?5could?5not?5start?0?5and?5not?5set@ (0000)
00249EA8 0039:
	??_C@_0DJ@ONAPPANJ@?$CFs?3?5can?8t?5begin?5yet?5but?5will?5rem@ (0000)
00249EE4 0017:
	??_C@_0BH@MJIAPKFJ@?$CFs?3?5begun?5successfully?$AA@ (0000)
00249F00 0058:
	??_C@_0FI@CHGNAOEL@WARNING?3?5too?5many?5executing?5conv@ (0000)
00249F58 0027:
	??_C@_0CH@FMECNBHL@?$CFs?3?5script?5tried?5to?5start?5conver@ (0000)
00249F80 001a:
	??_C@_0BK@ELJOEOCB@?$CFs?3?5no?5more?5lines?5to?5play?$AA@ (0000)
00249F9C 0030:
	??_C@_0DA@PPIFADMI@?$CFs?3?5unable?5to?5begin?0?5and?5no?5poin@ (0000)
00249FCC 0014:
	??_C@_0BE@KPFKNBML@?$CFs?3?5trying?5to?5begin?$AA@ (0000)
00249FE0 003c:
	??_C@_0DM@GFNDBPMM@selected_possibility?9?$DOpreselecte@ (0000)
0024A01C 001e:
	??_C@_0BO@LJOGJGHI@?$FLrnd?$CF?41f?5tot?$CF?41f?5cum?$CF?41f?$EA?$CFd?$FN?5?$AA@ (0000)
0024A03C 0019:
	??_C@_0BJ@DMOBOMPI@?$FL?$CFd?1?$CF?41f?5force?5?$CFd?1?$CF?41f?$FN?5?$AA@ (0000)
0024A058 0020:
	??_C@_0CA@DKHGIOGG@total_possibility_weight?5?$DO?50?40f?$AA@ (0000)
0024A078 0059:
	??_C@_0FJ@IBKDMIEO@ai_communication_event?3?5type?5?$CFd?5@ (0000)
0024A0D8 0043:
	??_C@_0ED@GJAAIMME@?$FL?$CFs?1?$CFd?5?$CFs?5del?$CFd?5w?3?$CF?41f?$CFs?5s?$CF?41f?5p@ (0000)
0024A11C 0002:
	??_C@_01BIAFAFID@F?$AA@ (0000)
0024A120 0007:
	??_C@_06BEJGECAN@PLAYER?$AA@ (0000)
0024A128 0012:
	??_C@_0BC@FOAHPEHM@protagonist_actor?$AA@ (0000)
0024A13C 0011:
	??_C@_0BB@BOBCLBAE@?$FL?$CFs?1?$CFd?5u?9?$CFs?9?$CFs?$FN?5?$AA@ (0000)
0024A150 0030:
	??_C@_0DA@CPDLIKLO@subject_actor?9?$DOdanger_zone?4objec@ (0000)
0024A180 0015:
	??_C@_0BF@JKGAIKPI@?$FL?$CFs?1?$CFd?5?$CFs?9d?9dis?1?$CFd?$FN?5?$AA@ (0000)
0024A198 000f:
	??_C@_0P@BLNKGMGL@?$FL?$CFs?1?$CFd?5?$CFs?5?$CFs?$FN?5?$AA@ (0000)
0024A1A8 0010:
	??_C@_0BA@JGHKCAMO@shout?3?$CF?41f?$DM?$CF?41f?$AA@ (0000)
0024A1B8 000f:
	??_C@_0P@NMKONCAD@talk?3?$CF?41f?$DM?$CF?41f?$AA@ (0000)
0024A1C8 000f:
	??_C@_0P@JEDLKPOF@chat?3?$CF?41f?$DM?$CF?41f?$AA@ (0000)
0024A1D8 0039:
	??_C@_0DJ@KAPPHHBA@communication_priority?5?$DM?5_ai_com@ (0000)
0024A214 000c:
	??_C@_0M@EGBKHEDG@?$CFs?9?$CFc?$CFc?$CFc?$CFs?$AA@ (0000)
0024A220 0004:
	??_C@_03HOBIBNHG@?9hi?$AA@ (0000)
0024A224 0004:
	??_C@_03CPELBCCM@?9lo?$AA@ (0000)
0024A228 007f:
	??_C@_0HP@PLIGCELE@?$CIcommunication_priority?5?$DO?5_ai_co@ (0000)
0024A2A8 0055:
	??_C@_0FF@NKMCJBLB@?$CIcommunication_team?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIco@ (0000)
0024A300 0009:
	??_C@_08ECOGDNI@unteamed?$AA@ (0000)
0024A30C 0013:
	??_C@_0BD@NMMLILLH@?$FL?$CFs?1?$CFd?50?9playrat?$FN?5?$AA@ (0000)
0024A320 0012:
	??_C@_0BC@OMHOAJOH@?$FL?$CFs?1?$CFd?5nogrp?9?$CFc?$FN?5?$AA@ (0000)
0024A334 0014:
	??_C@_0BE@OMABELMA@?$FL?$CFs?1?$CFd?5noplyreply?$FN?5?$AA@ (0000)
0024A348 0013:
	??_C@_0BD@GACNBALB@?$FL?$CFs?1?$CFd?5nounit?9?$CFc?$FN?5?$AA@ (0000)
0024A35C 0006:
	??_C@_05LLNBKOOD@?$DMerr?$DO?$AA@ (0000)
0024A364 000f:
	??_C@_0P@OMGCNPFA@?$FL?$CFs?1?$CFd?5nodmg?$FN?5?$AA@ (0000)
0024A374 0014:
	??_C@_0BE@PGBJJNLI@?$FL?$CFs?1?$CFd?5nocausrace?$FN?5?$AA@ (0000)
0024A388 0013:
	??_C@_0BD@PHFOKNBE@?$FL?$CFs?1?$CFd?5nosubrace?$FN?5?$AA@ (0000)
0024A39C 0013:
	??_C@_0BD@NGEMBLCE@?$FL?$CFs?1?$CFd?5status?9?$CFs?$FN?5?$AA@ (0000)
0024A3B0 0004:
	??_C@_03NDPLAFDJ@vis?$AA@ (0000)
0024A3B4 0009:
	??_C@_08OLCLKDFA@nodanger?$AA@ (0000)
0024A3C0 0007:
	??_C@_06EBKKMJGD@notvis?$AA@ (0000)
0024A3C8 0005:
	??_C@_04BLLOBPAA@lost?$AA@ (0000)
0024A3D0 0005:
	??_C@_04BBBDNBOK@dead?$AA@ (0000)
0024A3D8 0011:
	??_C@_0BB@GKMIFEBC@?$FL?$CFs?1?$CFd?5host?9?$CFs?$FN?5?$AA@ (0000)
0024A3EC 0008:
	??_C@_07CEGFBHJP@traitor?$AA@ (0000)
0024A3F4 0006:
	??_C@_05ICAMJEGE@enemy?$AA@ (0000)
0024A3FC 0007:
	??_C@_06BHBNKPBA@friend?$AA@ (0000)
0024A404 0005:
	??_C@_04PJDNGLKJ@self?$AA@ (0000)
0024A410 0048:
	??_C@_0EI@GINMJEIA@?$CIdialogue_index?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIdialog@ (0000)
0024A458 0009:
	??_C@_08GLNGGDPJ@DISABLED?$AA@ (0000)
0024A464 0007:
	??_C@_06DAIKMLH@?$CFs?9?$CFc?5?$AA@ (0000)
0024A46C 002b:
	??_C@_0CL@IOMHDOAM@allegiance?5?$CFs?0?5?$CFd?5incidents?5?$CIthr@ (0000)
0024A498 000c:
	??_C@_0M@IDKAPGFO@still?5holds?$AA@ (0000)
0024A4A4 0029:
	??_C@_0CJ@GKFDOEJ@incident?5between?5teams?5?$CFs?5and?5?$CFs@ (0000)
0024A4D0 0009:
	??_C@_08PAHPFNGA@accident?$AA@ (0000)
0024A4DC 0009:
	??_C@_08JPJNIAAJ@betrayal?$AA@ (0000)
0024A4E8 000b:
	??_C@_0L@MCEFNCAK@unobserved?$AA@ (0000)
0024A4F4 0009:
	??_C@_08PJALNDK@observed?$AA@ (0000)
0024A500 0055:
	??_C@_0FF@FCIFJAOH@?$CIcommunication_type?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIco@ (0000)
002B7308 0114:
	_global_communication_priority_names (0000)
	_global_communication_team_names (0020)
	_global_communication_type_names (0030)
00319CA8 0010:
	_global_dialogue_event_count (0000)
	_global_dialogue_events (0004)
	_global_reply_event_count (0008)
	_global_reply_events (000c)
*/

/* ---------- headers */

#include "cseries.h"
#include "ai_communication.h"

#include "ai.h"
#include "ai_debug.h"
#include "actor_definitions.h"
#include "actors.h"
#include "actor_types.h"
#include "ai_scenario_definitions.h"
#include "game/game.h"
#include "main/console.h"
#include "memory/data.h"
#include "props.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_files.h"
#include "units/dialogue_definitions.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	NUMBER_OF_COMMUNICATION_PRIORITIES = 8,
	NUMBER_OF_COMMUNICATION_TEAMS = 4,
	NUMBER_OF_COMMUNICATION_TYPES = 57,
	NUMBER_OF_COMMUNICATION_TIMER_TYPES = 5,
	NUMBER_OF_DIALOGUE_USAGES = 105,
	NUMBER_OF_REPLY_USAGES = 46,
	MAXIMUM_CONVERSATION_PARTICIPANTS = 8,
	MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT = 6,
	MAXIMUM_RECENT_CONVERSATIONS = 16,
	_ai_conversation_stop_if_anyone_dies_bit = 0,
	_ai_conversation_line_addressee_look_back_bit = 0,
	_ai_conversation_line_everyone_look_at_speaker_bit = 1,
	_ai_conversation_line_everyone_look_at_addressee_bit = 2,
	_ai_conversation_line_wait_after_until_told_to_advance_bit = 3,
	_ai_conversation_line_wait_until_speaker_nearby_bit = 4,
	_ai_conversation_line_wait_until_everyone_nearby_bit = 5,
	_ai_conversation_address_player = 1,
	_ai_conversation_address_participant = 2,
	_ai_conversation_selection_radio = 6,
	_ai_conversation_selection_radio_sargeant = 7,
	_actor_mode_alert = 2,
	_actor_mode_combat = 3,
	_actor_combat_status_certain = 4,
	_ai_information_none = 0,
	_ai_information_look_unit = 1,
	_ai_information_look_object = 2,
	_ai_information_allegiance = 1,
	_ai_communication_priority_communicate = 4,
	_ai_communication_priority_yell = 6,
	_ai_sound_volume_medium = 1,
	_ai_sound_volume_loud = 2,
	_ai_sound_volume_shout = 3,
	_ai_communication_team_human = 0,
	_ai_communication_team_covenant = 1,
	NUMBER_OF_AI_COMMUNICATION_TEAMS = 2,
};

#define COMMUNICATION_CLOSE_DISTANCE 5.0f

/* ---------- macros */

#define ai_print_conversations (ai_debug.print_conversations)
#define ai_conversation_driver_get(index) \
	((struct ai_conversation_driver_datum_view *)datum_get(conversation_data, (index)))

/* ---------- structures */

struct ai_information_packet;

struct dialogue_usage
{
	short communication_type;
	short communication_priority;
	short vocalization_type;
	short animation_type;
	short protagonist_type;
	short protagonist_look_priority;
	short recipient_look_direction;
	short recipient_look_priority;
	real weight;
	real repeat_delay;
	short flags;
	short required_group;
	short required_hostility;
	short required_enemy_status;
	short required_subject_race;
	short required_cause_race;
	short required_damage;
};

struct reply_usage
{
	short original_vocalization_type;
	short original_damage_category;
	short protagonist_type;
	short vocalization_type;
	short animation_type;
	short communication_priority;
	word flags;
	real chance;
	real player_chance;
	real delay_time;
	real repeat_delay;
	boolean (*reply_filter)(
		long original_unit_index,
		struct ai_information_packet *communication,
		long reply_actor_index);
};

typedef char dialogue_usage_size_assert[
	sizeof(struct dialogue_usage) == 0x28 ? 1 : -1];
typedef char reply_usage_size_assert[
	sizeof(struct reply_usage) == 0x24 ? 1 : -1];

struct ai_conversation_line_view
{
	struct ai_conversation_datum_header header;
	byte __unknown14[0x34];
	short current_line;
};

typedef char ai_conversation_line_view_current_line_offset_assert[
	offsetof(struct ai_conversation_line_view, current_line) == 0x48 ? 1 : -1];

struct ai_conversation_driver_datum_view
{
	struct ai_conversation_datum_header header;
	unsigned long participant_bitmask;
	short dialogue_indices[MAXIMUM_CONVERSATION_PARTICIPANTS];
	long actor_indices[MAXIMUM_CONVERSATION_PARTICIPANTS];
	short current_line;
	short line_participant_index;
	short line_delay_timer;
	word line_flags;
	long line_actor_index;
	long line_unit_index;
	long line_address_unit_index;
	long line_sound_index;
	boolean line_unspatialized;
	boolean line_spoken;
	boolean line_finished;
	boolean line_advance;
};

struct ai_conversation_datum_view
{
	short identifier;
	short scenario_conversation_index;
	boolean scripted;
	boolean any_line_spoken;
	boolean begun;
	boolean finished;
	boolean waiting_to_advance;
	boolean told_to_advance;
	byte reserved0A[2];
	long creation_time;
	long triggering_player_unit_index;
	unsigned long participant_bitmask;
	short dialogue_indices[MAXIMUM_CONVERSATION_PARTICIPANTS];
	long actor_indices[MAXIMUM_CONVERSATION_PARTICIPANTS];
	short line_index;
	short line_participant_index;
	short line_delay_timer;
	word line_flags;
	long line_actor_index;
	long line_unit_index;
	long line_address_unit_index;
	long line_sound_index;
	boolean line_unspatialized;
	boolean line_spoken;
	boolean line_finished;
	boolean line_advance;
};

struct scenario_conversation_definition_view
{
	char name[32];
	word flags;
	word pad22;
	real trigger_distance;
	real run_to_player_distance;
	byte __unknown2C[0x24];
	struct tag_block participants;
	struct tag_block lines;
	struct tag_block unused;
};

struct scenario_conversation_participant_view
{
	word pad00;
	word flags;
	short selection_type;
	short actor_type;
	short preexisting_object_name_index;
	short new_attach_object_name_index;
	byte __unknown0C[0x0C];
	short dialogue_variants[MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT];
	char ai_index_name[32];
	long runtime_ai_index;
	byte __unknown48[0x0C];
};

struct scenario_conversation_line_view
{
	word flags;
	short participant_index;
	short address_type;
	short address_participant_index;
	long unknown08;
	real delay_time;
	byte __unknown10[0x0C];
	struct tag_reference dialogue[MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT];
};

struct recent_conversation_view
{
	short scenario_conversation_index;
	boolean unable_to_begin;
	boolean finished_successfully;
	long finish_time;
	byte __unknown08[8];
};

struct dialogue_event_status
{
	long last_time_spoken;
	long disable_until_time;
};

struct ai_communication_globals_view
{
	byte __unknown00[0x10];
	boolean dialogue_triggers_enabled;
	byte __unknown11[3];
	long last_chatter_time[2];
	long last_talk_time[2];
	long last_shout_time[2];
	short recent_conversation_count;
	short recent_conversation_next_index;
	struct recent_conversation_view
		recent_conversations[MAXIMUM_RECENT_CONVERSATIONS];
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

typedef char ai_conversation_datum_header_size_assert[
	sizeof(struct ai_conversation_datum_header) == 0x14 ? 1 : -1];
typedef char ai_conversation_datum_view_size_assert[
	sizeof(struct ai_conversation_datum_view) == 0x64 ? 1 : -1];
typedef char ai_conversation_datum_view_line_index_offset_assert[
	offsetof(struct ai_conversation_datum_view, line_index) == 0x48 ? 1 : -1];
typedef char ai_conversation_datum_view_line_advance_offset_assert[
	offsetof(struct ai_conversation_datum_view, line_advance) == 0x63 ? 1 : -1];
typedef char ai_conversation_datum_header_any_line_spoken_offset_assert[
	offsetof(struct ai_conversation_datum_header, any_line_spoken) == 0x5 ? 1 : -1];
typedef char ai_conversation_datum_header_begun_offset_assert[
	offsetof(struct ai_conversation_datum_header, begun) == 0x6 ? 1 : -1];
typedef char ai_conversation_datum_header_waiting_to_advance_offset_assert[
	offsetof(struct ai_conversation_datum_header, waiting_to_advance) == 0x8 ? 1 : -1];
typedef char ai_conversation_datum_header_told_to_advance_offset_assert[
	offsetof(struct ai_conversation_datum_header, told_to_advance) == 0x9 ? 1 : -1];
typedef char scenario_conversation_definition_view_size_assert[
	sizeof(struct scenario_conversation_definition_view) == 0x74 ? 1 : -1];
typedef char scenario_conversation_definition_participants_offset_assert[
	offsetof(struct scenario_conversation_definition_view, participants) == 0x50 ? 1 : -1];
typedef char scenario_conversation_definition_lines_offset_assert[
	offsetof(struct scenario_conversation_definition_view, lines) == 0x5C ? 1 : -1];
typedef char scenario_conversation_participant_view_size_assert[
	sizeof(struct scenario_conversation_participant_view) == 0x54 ? 1 : -1];
typedef char scenario_conversation_line_view_size_assert[
	sizeof(struct scenario_conversation_line_view) == 0x7C ? 1 : -1];
typedef char ai_conversation_driver_datum_view_size_assert[
	sizeof(struct ai_conversation_driver_datum_view) == 0x64 ? 1 : -1];
typedef char ai_conversation_driver_participant_bitmask_offset_assert[
	offsetof(struct ai_conversation_driver_datum_view, participant_bitmask) == 0x14 ? 1 : -1];
typedef char ai_conversation_driver_actor_indices_offset_assert[
	offsetof(struct ai_conversation_driver_datum_view, actor_indices) == 0x28 ? 1 : -1];
typedef char ai_conversation_driver_line_flags_offset_assert[
	offsetof(struct ai_conversation_driver_datum_view, line_flags) == 0x4E ? 1 : -1];
typedef char ai_conversation_driver_line_unit_index_offset_assert[
	offsetof(struct ai_conversation_driver_datum_view, line_unit_index) == 0x54 ? 1 : -1];
typedef char ai_conversation_driver_line_address_unit_index_offset_assert[
	offsetof(struct ai_conversation_driver_datum_view, line_address_unit_index) == 0x58 ? 1 : -1];
typedef char recent_conversation_view_size_assert[
	sizeof(struct recent_conversation_view) == 0x10 ? 1 : -1];
typedef char ai_print_conversations_offset_assert[
	offsetof(struct ai_debug_state, print_conversations) == 0x9F ? 1 : -1];
typedef char ai_communication_unit_speech_item_size_assert[
	sizeof(struct unit_speech_item) == 0x30 ? 1 : -1];
typedef char ai_communication_unit_speech_item_ai_offset_assert[
	offsetof(struct unit_speech_item, ai) == 0x10 ? 1 : -1];
typedef char ai_communication_actor_mode_offset_assert[
	offsetof(struct actor_datum, state.mode) == 0x6A ? 1 : -1];
typedef char ai_communication_actor_unit_index_offset_assert[
	offsetof(struct actor_datum, meta.unit_index) == 0x18 ? 1 : -1];
typedef char ai_communication_actor_idle_combat_offset_assert[
	offsetof(struct actor_datum, control.idle_vocalization_combat) == 0x6CC ? 1 : -1];
typedef char ai_communication_actor_idle_timer_offset_assert[
	offsetof(struct actor_datum, control.idle_vocalization_timer) == 0x6CE ? 1 : -1];
typedef char ai_communication_actor_iterator_size_assert[
	sizeof(struct actor_iterator) == 0x1C ? 1 : -1];
typedef char ai_communication_actor_iterator_index_offset_assert[
	offsetof(struct actor_iterator, index) == 0x14 ? 1 : -1];

/* ---------- prototypes */

static boolean reply_filter_close(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
static boolean reply_filter_not_close(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
static boolean reply_filter_searching(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
boolean code_000314c0(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
boolean code_00031550(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
boolean code_00031570(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
boolean code_000315b0(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
static boolean reply_filter_no_certain_target(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
static boolean reply_filter_flee_leader(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
static void code_000318c0(
	long actor_index,
	short type,
	short priority,
	long look_unit_index,
	long prop_index);
static void code_00031970(
	long actor_index,
	short type,
	short priority,
	long object_index);
static boolean code_00031e80(
	long conversation_index);
static boolean code_00032030(
	long conversation_index);
static struct scenario_conversation_definition_view *code_000322c0(
	long conversation_index);
boolean actor_is_fighting(
	long actor_index);
static void actor_reset_idle_vocalization_timer(
	long actor_index);
static void code_000324b0(
	long actor_index,
	long prop_index,
	struct ai_information_packet *information);
static void ai_communication_update_speech_timers(
	long unit_index,
	short priority,
	short vocalization_type,
	short dialogue_type_index,
	short reply_table_index);
short unit_test_speech(
	long unit_index,
	short priority,
	boolean allow_recursive_lookup,
	boolean allow_queue,
	long *unit_last_speech_time,
	short *vocalization_type_reference,
	long *sound_definition_index_reference);
void unit_speak(
	long unit_index,
	short play_type,
	struct unit_speech_item const *speech_item);
boolean sound_scripted_dialog_is_playing(
	void);
void scripted_sound_new(
	long definition_index,
	long source_object_index,
	real scale);
long scripted_sound_time(
	long definition_index);
void ai_handle_allegiance_broken_notification(
	short team1_index,
	short team2_index,
	boolean broken);
void actor_iterator_new(
	struct actor_iterator *iterator,
	boolean active_only);
struct actor_datum *actor_iterator_next(
	struct actor_iterator *iterator);
boolean game_team_is_enemy(
	short team_index0,
	short team_index1);
void actor_perception_find_sense_position(
	long actor_index,
	real_point3d const *position,
	long prop_index,
	struct actor_position_data *sense_position);
short actor_audibility_at_point(
	long actor_index,
	struct actor_position_data const *position,
	real_point3d const *source_position,
	struct location const *source_location,
	short source_type,
	real scale,
	short line_of_sight);
void actor_handle_communication(
	long actor_index,
	long prop_index,
	struct ai_information_packet *information);
boolean code_00034020(
	long conversation_index,
	boolean *continue_trying);

extern short global_communication_table_indices[NUMBER_OF_COMMUNICATION_TYPES];
extern struct ai_communication_globals_view *ai_globals;

/* ---------- globals */

short const communication_speech_priorities[NUMBER_OF_COMMUNICATION_PRIORITIES] =
{
	0, 3, 3, 3, 4, 5, 5, 8,
};

real const communication_notification_delays[NUMBER_OF_COMMUNICATION_PRIORITIES] =
{
	0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.3f,
};

short const communication_protagonist_default_look_priorities
	[NUMBER_OF_COMMUNICATION_PRIORITIES] =
{
	0, 4, 4, 5, 6, 6, 6, 4,
};

short const communication_recipient_default_look_priorities
	[NUMBER_OF_COMMUNICATION_PRIORITIES] =
{
	0, 3, 3, 4, 5, 5, 6, 4,
};

short const communication_player_speaking_priorities
	[NUMBER_OF_COMMUNICATION_PRIORITIES] =
{
	0, 2, 3, 4, 4, 6, 6, 7,
};

short const communication_unit_prefer_silent_time = 60;

real const communication_timer_tolerances
	[NUMBER_OF_COMMUNICATION_TIMER_TYPES][2][NUMBER_OF_COMMUNICATION_PRIORITIES] =
{
	{
		{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 4.0f, 4.5f, 2.5f, 2.0f, 0.0f, 5.0f },
	},
	{
		{ 5.0f, 3.0f, 2.5f, 0.0f, 3.0f, 3.5f, 2.0f, 1.5f },
		{ 0.0f, 4.5f, 4.5f, 2.5f, 2.0f, 0.0f, 0.3f, 1.0f },
	},
	{
		{ 1.0f, 1.0f, 0.8f, 1.0f, 2.0f, 1.5f, 1.5f, 0.3f },
		{ 0.0f, 0.5f, 0.5f, 0.5f, 1.3f, 0.2f, 1.0f, 1.0f },
	},
	{
		{ 1.0f, 0.8f, 0.0f, 0.0f, 1.0f, 1.0f, 1.5f, 0.0f },
		{ 0.0f, 1.5f, 1.5f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f },
	},
	{
		{ 1.5f, 0.0f, 0.0f, 0.5f, 0.0f, 1.5f, 0.0f, 0.0f },
		{ 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f },
	},
};

real const communication_play_delays[NUMBER_OF_COMMUNICATION_TIMER_TYPES] =
{
	0.0f, 0.5f, 0.8f, 0.5f, 0.8f,
};

short const communication_player_additional_delay = 30;
short const communication_overlap_time_modifier = 45;
short const communication_timeout_low_priority_modifier = 30;
short const communication_repeat_selection_time = 900;
real const communication_player_absolute_range = 30.0f;
real const communication_player_ideal_range_min = 3.0f;
real const communication_player_ideal_range_max = 15.0f;
real const communication_player_ideal_fov = 0.70710677f;
real const communication_player_rating_low_priority = 2.0f;

struct dialogue_usage const global_dialogue_table[NUMBER_OF_DIALOGUE_USAGES] =
{
	/* death */
	{ 0, 3, 49, -1, 1, 6, 2, 1, 10.0f, 0.0f,
		0, -1, 2, -1, -1, -1, -1 },
	{ 0, 6, 51, -1, 1, 6, 3, 6, 20.0f, 0.0f,
		66, -1, 2, -1, 1, -1, -1 },
	{ 0, 2, 53, -1, 1, 1, 2, 1, 10.0f, 0.0f,
		8, -1, 3, -1, -1, -1, -1 },
	{ 0, 6, 55, -1, 1, 6, 3, 6, 20.0f, 0.0f,
		74, -1, 3, -1, 1, -1, -1 },
	{ 0, 2, 57, -1, 1, 1, 2, 1, 10.0f, 0.0f,
		8, -1, 3, -1, 4, -1, -1 },
	{ 0, 2, 59, -1, 1, 1, 2, 1, 40.0f, 0.0f,
		8, -1, 3, -1, 8, -1, -1 },
	{ 0, 2, 61, -1, 1, 1, 2, 1, 40.0f, 0.0f,
		8, -1, 3, -1, 16, -1, -1 },
	{ 0, 2, 63, -1, 1, 1, 2, 1, 40.0f, 0.0f,
		8, -1, 3, -1, 64, -1, -1 },
	{ 0, 2, 65, -1, 1, 1, 2, 1, 10.0f, 10.0f,
		0, -1, 3, -1, -1, -1, 2 },
	{ 0, 2, 67, -1, 1, 1, 2, 1, 10.0f, 10.0f,
		0, -1, 3, -1, -1, -1, 10 },
	{ 0, 2, 66, -1, 1, 1, 2, 1, 10.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 11 },
	{ 0, 3, 68, -1, 1, 1, 2, 1, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 5 },
	{ 0, 3, 69, -1, 1, 1, 2, 1, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 3 },
	{ 0, 3, 70, -1, 1, 1, 2, 1, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 4 },
	{ 0, 3, 71, -1, 1, 1, 2, 1, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 6 },
	{ 0, 2, 72, -1, 1, 1, 2, 1, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 7 },
	{ 0, 2, 73, -1, 1, 1, 2, 1, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 12 },
	{ 0, 3, 74, -1, 1, 1, 2, 1, 30.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 9 },
	{ 0, 2, 75, -1, 1, 1, 2, 1, 30.0f, 10.0f,
		0, -1, 3, -1, -1, -1, 8 },
	{ 0, 2, 96, -1, 2, 1, 3, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	{ 0, 6, 97, -1, 2, 6, 3, 6, 15.0f, 0.0f,
		66, -1, -1, -1, 1, -1, -1 },
	{ 0, 3, 98, -1, 2, 5, 1, 1, 10.0f, 0.0f,
		0, -1, 2, -1, -1, -1, -1 },
	{ 0, 4, 99, -1, 2, 6, 3, 5, 20.0f, 0.0f,
		66, -1, 2, -1, -1, 1, -1 },
	{ 0, 2, 100, -1, 2, 1, 3, 1, 10.0f, 0.0f,
		0, -1, 3, -1, -1, -1, -1 },
	{ 0, 2, 101, -1, 2, 1, 3, 1, 10.0f, 0.0f,
		0, -1, 3, -1, -1, 1, -1 },
	{ 0, 2, 102, -1, 2, 1, 3, 1, 10.0f, 0.0f,
		0, -1, 3, -1, -1, 4, -1 },
	{ 0, 2, 103, -1, 2, 1, 3, 1, 40.0f, 0.0f,
		0, -1, 3, -1, -1, 56, -1 },
	{ 0, 2, 104, -1, 2, 1, 3, 1, 40.0f, 0.0f,
		0, -1, 3, -1, -1, 64, -1 },
	{ 0, 5, 105, -1, 2, 6, 3, 6, 30.0f, 0.0f,
		2, -1, 4, -1, -1, -1, -1 },
	/* killing_spree */
	{ 1, 4, 76, -1, 0, 1, 2, 1, 30.0f, 0.0f,
		8, -1, -1, -1, -1, -1, -1 },
	/* damage */
	{ 3, 3, 21, -1, 1, 1, 2, 1, 10.0f, 0.0f,
		0, -1, 2, -1, -1, -1, -1 },
	{ 3, 3, 22, -1, 1, 1, 2, 1, 10.0f, 0.0f,
		2, -1, 2, -1, 1, -1, -1 },
	{ 3, 3, 29, -1, 0, 5, 3, 1, 10.0f, 0.0f,
		8, -1, 2, -1, -1, -1, -1 },
	{ 3, 3, 31, -1, 0, 6, 3, 1, 10.0f, 5.0f,
		0, -1, 2, -1, -1, 1, -1 },
	/* hurt */
	{ 2, 1, 23, -1, 1, 1, 0, 0, 10.0f, 10.0f,
		0, -1, 3, -1, -1, -1, -1 },
	{ 2, 2, 32, -1, 0, 5, 0, 0, 10.0f, 10.0f,
		0, -1, 3, -1, -1, -1, -1 },
	{ 2, 2, 35, -1, 0, 5, 0, 0, 10.0f, 10.0f,
		0, -1, 3, -1, -1, -1, 2 },
	{ 2, 2, 36, -1, 0, 5, 0, 0, 10.0f, 10.0f,
		0, -1, 3, -1, -1, -1, 11 },
	{ 2, 2, 37, -1, 0, 5, 0, 0, 10.0f, 10.0f,
		0, -1, 3, -1, -1, -1, 10 },
	{ 2, 3, 38, -1, 0, 5, 0, 0, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 5 },
	{ 2, 2, 40, -1, 0, 5, 0, 0, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 4 },
	{ 2, 3, 41, -1, 0, 5, 0, 0, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 6 },
	{ 2, 2, 42, -1, 0, 5, 0, 0, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 7 },
	{ 2, 2, 43, -1, 0, 5, 0, 0, 20.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 12 },
	{ 2, 3, 44, -1, 0, 5, 0, 0, 30.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 9 },
	{ 2, 2, 45, -1, 0, 5, 0, 0, 30.0f, 0.0f,
		0, -1, 3, -1, -1, -1, 8 },
	/* sighted_enemy */
	{ 4, 5, 108, 3, 0, 6, 3, 6, 10.0f, 10.0f,
		0, -1, -1, 0, -1, -1, -1 },
	{ 4, 5, 109, 3, 0, 6, 3, 6, 10.0f, 25.0f,
		0, -1, -1, 2, -1, -1, -1 },
	/* found_enemy */
	{ 5, 5, 110, 3, 0, 6, 3, 6, 10.0f, 25.0f,
		0, -1, -1, 2, -1, -1, -1 },
	/* unexpected_enemy */
	{ 6, 3, 111, -1, 0, 6, 3, 4, 10.0f, 8.0f,
		1, -1, -1, -1, -1, -1, -1 },
	/* found_dead_friend */
	{ 7, 5, 112, -1, 0, 6, 3, 6, 10.0f, 20.0f,
		0, -1, -1, 4, -1, -1, -1 },
	/* allegiance_changed */
	{ 8, 6, 113, -1, 0, 6, 3, 6, 10.0f, 0.0f,
		68, -1, 4, -1, -1, -1, -1 },
	{ 8, 4, 114, -1, 0, 6, 3, 4, 10.0f, 0.0f,
		64, -1, 2, -1, -1, -1, -1 },
	/* sighted_friend_player */
	{ 25, 4, 148, 3, 0, 6, 3, 5, 10.0f, 15.0f,
		0, -1, -1, 4, -1, -1, -1 },
	/* lost_contact */
	{ 13, 4, 161, -1, 0, 0, 2, 1, 10.0f, 20.0f,
		0, -1, -1, 3, -1, -1, -1 },
	{ 13, 4, 127, -1, 0, 0, 2, 1, 15.0f, 20.0f,
		0, 0, -1, 3, -1, -1, -1 },
	/* alert_noncombat */
	{ 15, 4, 125, -1, 0, 4, 2, 5, 10.0f, 20.0f,
		0, -1, -1, 2, -1, -1, -1 },
	/* blocked */
	{ 14, 2, 129, 1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, 1, -1, -1, -1, -1, 0 },
	/* search_start */
	{ 16, 3, 131, -1, 0, 0, 2, 1, 10.0f, 8.0f,
		0, 0, -1, 3, -1, -1, -1 },
	/* search_query */
	{ 17, 3, 132, -1, 0, 5, 2, 1, 10.0f, 25.0f,
		0, 0, -1, 3, -1, -1, -1 },
	/* search_report */
	{ 18, 3, 134, -1, 0, 0, 2, 1, 10.0f, 25.0f,
		0, 0, -1, 3, -1, -1, -1 },
	/* search_abandon */
	{ 19, 3, 135, -1, 0, 0, 2, 1, 10.0f, 8.0f,
		0, 0, -1, 3, -1, -1, -1 },
	/* search_group_abandon */
	{ 20, 4, 136, -1, 0, 0, 2, 1, 10.0f, 0.0f,
		0, 0, -1, 3, -1, -1, -1 },
	/* uncover_start */
	{ 21, 3, 137, 2, 0, 0, 2, 1, 10.0f, 30.0f,
		0, 1, -1, 3, -1, -1, -1 },
	/* cover */
	{ 24, 2, 143, -1, 0, 0, 2, 1, 10.0f, 30.0f,
		0, 1, -1, -1, -1, -1, -1 },
	/* advance */
	{ 22, 4, 139, 2, 0, 0, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* retreat */
	{ 23, 4, 141, -1, 0, 0, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* shooting */
	{ 26, 1, 149, -1, 0, 0, 0, 0, 10.0f, 10.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* shooting_vehicle */
	{ 27, 1, 150, -1, 0, 0, 0, 0, 20.0f, 10.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* shooting_berserk */
	{ 28, 2, 151, -1, 0, 0, 0, 0, 20.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* shooting_group */
	{ 29, 2, 152, -1, 0, 0, 0, 0, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* shooting_traitor */
	{ 30, 1, 153, -1, 0, 0, 0, 0, 30.0f, 10.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* flee */
	{ 31, 3, 156, -1, 0, 0, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* flee_leader_died */
	{ 32, 4, 158, -1, 0, 0, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* flee_idle */
	{ 33, 3, 2, -1, 0, 0, 0, 0, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* attempted_flee */
	{ 34, 2, 159, -1, 0, 4, 2, 5, 10.0f, 10.0f,
		0, 1, -1, -1, -1, -1, -1 },
	/* hiding_finished */
	{ 35, 3, 162, -1, 0, 0, 2, 1, 10.0f, 30.0f,
		0, 1, -1, 3, -1, -1, -1 },
	/* vehicle_entry */
	{ 36, 3, 163, -1, 0, 0, 0, 0, 10.0f, 10.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* vehicle_exit */
	{ 37, 3, 164, -1, 0, 0, 0, 0, 10.0f, 10.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* vehicle_woohoo */
	{ 38, 2, 165, 12, 2, 0, 0, 0, 10.0f, 25.0f,
		48, -1, -1, -1, -1, -1, -1 },
	/* vehicle_scared */
	{ 39, 2, 166, 13, 2, 0, 0, 0, 10.0f, 25.0f,
		48, -1, -1, -1, -1, -1, -1 },
	/* vehicle_falling */
	{ 40, 7, 10, 13, 2, 0, 0, 0, 10.0f, 0.0f,
		48, -1, -1, -1, -1, -1, -1 },
	/* grenade_sighted */
	{ 11, 3, 117, -1, 0, 6, 4, 6, 10.0f, 4.0f,
		0, 0, 3, -1, -1, -1, -1 },
	/* grenade_startle */
	{ 10, 3, 116, -1, 0, 6, 4, 6, 10.0f, 4.0f,
		0, -1, -1, 4, -1, -1, -1 },
	/* grenade_danger */
	{ 12, 5, 118, -1, 0, 5, 4, 5, 10.0f, 4.0f,
		0, 0, 3, -1, -1, -1, -1 },
	{ 12, 5, 120, -1, 0, 5, 4, 5, 10.0f, 0.0f,
		0, 0, 2, -1, -1, -1, -1 },
	{ 12, 6, 119, -1, 0, 6, 4, 5, 10.0f, 0.0f,
		0, -1, 1, -1, -1, -1, -1 },
	/* surprise */
	{ 41, 7, 177, -1, 0, 6, 2, 4, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* berserk */
	{ 42, 7, 178, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* melee */
	{ 43, 7, 179, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* grenade_throwing */
	{ 9, 7, 115, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* dive */
	{ 44, 7, 180, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* leap */
	{ 47, 7, 182, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* falling */
	{ 46, 7, 10, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* postcombat_alone */
	{ 48, 3, 197, -1, 0, 5, 0, 0, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* postcombat_unscathed */
	{ 49, 3, 198, -1, 0, 0, 0, 0, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* postcombat_wounded */
	{ 50, 3, 199, -1, 0, 0, 0, 0, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* postcombat_massacre */
	{ 51, 3, 201, -1, 0, 0, 0, 0, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* postcombat_triumph */
	{ 52, 3, 203, -1, 0, 0, 0, 0, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* postcombat_check_enemy */
	{ 53, 3, 189, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, -1, 3, -1, -1, -1, -1 },
	/* postcombat_check_friend */
	{ 54, 3, 190, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, -1, 2, -1, -1, -1, -1 },
	/* postcombat_shoot_corpse */
	{ 55, 3, 191, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	{ 55, 6, 192, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		2, -1, -1, -1, -1, 1, -1 },
	/* postcombat_celebrate */
	{ 56, 3, 188, -1, 0, 4, 2, 1, 10.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
	/* sentinel */
	{ -1, -1, -1, -1, -1, -1, -1, -1, 0.0f, 0.0f,
		0, -1, -1, -1, -1, -1, -1 },
};

struct reply_usage const global_reply_table[NUMBER_OF_REPLY_USAGES] =
{
	{ 76, -1, 2, 92, -1, 2, 0, 0.0f, 1.0f, 0.7f, 30.0f,
		reply_filter_close },
	{ 53, 2, 2, 81, -1, 2, 0, 0.0f, 0.5f, 0.7f, 60.0f,
		reply_filter_close },
	{ 53, 11, 2, 82, -1, 2, 0, 0.0f, 0.6f, 0.7f, 60.0f,
		reply_filter_close },
	{ 53, 10, 2, 83, -1, 2, 0, 0.0f, 0.6f, 0.7f, 60.0f,
		reply_filter_close },
	{ 53, 5, 2, 84, -1, 2, 0, 0.0f, 0.8f, 0.7f, 60.0f,
		reply_filter_close },
	{ 53, 3, 2, 85, -1, 2, 0, 0.5f, 0.9f, 0.7f, 60.0f,
		reply_filter_close },
	{ 53, 4, 2, 86, -1, 2, 0, 0.0f, 1.0f, 0.7f, 60.0f,
		reply_filter_close },
	{ 53, 6, 2, 87, -1, 2, 0, 0.0f, 1.0f, 0.7f, 60.0f,
		reply_filter_close },
	{ 53, 7, 2, 88, -1, 2, 0, 0.0f, 0.9f, 0.7f, 60.0f,
		reply_filter_close },
	{ 53, 12, 2, 89, -1, 2, 0, 0.0f, 0.6f, 0.7f, 60.0f,
		reply_filter_close },
	{ 53, 9, 2, 90, -1, 2, 0, 0.0f, 0.8f, 0.7f, 60.0f,
		reply_filter_close },
	{ 53, 8, 2, 91, -1, 2, 0, 0.0f, 1.0f, 0.7f, 60.0f,
		reply_filter_close },
	{ 55, -1, 2, 56, -1, 4, 1, 1.0f, 1.0f, 0.3f, 0.0f,
		reply_filter_close },
	{ 57, -1, 2, 58, -1, 2, 0, 0.8f, 0.6f, 0.5f, 30.0f,
		reply_filter_close },
	{ 59, -1, 2, 60, -1, 2, 0, 0.8f, 0.6f, 0.5f, 20.0f,
		reply_filter_close },
	{ 61, -1, 2, 62, -1, 2, 0, 0.8f, 0.6f, 0.5f, 20.0f,
		reply_filter_close },
	{ 63, -1, 2, 64, -1, 2, 0, 0.8f, 0.6f, 0.5f, 20.0f,
		reply_filter_close },
	{ 53, -1, 2, 54, -1, 2, 0, 0.6f, 0.4f, 0.5f, 30.0f,
		reply_filter_close },
	{ 53, -1, 2, 80, -1, 2, 0, 0.0f, 0.4f, 0.7f, 40.0f,
		reply_filter_close },
	{ 23, -1, 2, 24, -1, 1, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		NULL },
	{ 32, -1, 4, 33, -1, 1, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		NULL },
	{ 32, -1, 2, 34, -1, 1, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		NULL },
	{ 51, -1, 2, 52, -1, 4, 1, 1.0f, 0.0f, 0.3f, 0.0f,
		reply_filter_close },
	{ 49, -1, 2, 50, -1, 3, 0, 0.7f, 0.0f, 0.3f, 20.0f,
		reply_filter_close },
	{ 29, -1, 3, 30, -1, 2, 0, 0.7f, 0.4f, 0.5f, 20.0f,
		NULL },
	{ 108, -1, 2, 123, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		reply_filter_close },
	{ 108, -1, 2, 124, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		reply_filter_not_close },
	{ 109, -1, 2, 123, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		reply_filter_close },
	{ 109, -1, 2, 124, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		reply_filter_not_close },
	{ 110, -1, 2, 123, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		reply_filter_close },
	{ 110, -1, 2, 124, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		reply_filter_not_close },
	{ 125, -1, 3, 126, -1, 3, 0, 0.7f, 0.0f, 0.5f, 15.0f,
		NULL },
	{ 127, -1, 2, 128, -1, 3, 0, 0.7f, 0.0f, 0.5f, 30.0f,
		reply_filter_no_certain_target },
	{ 129, -1, 3, 130, -1, 3, 0, 0.5f, 0.0f, 0.5f, 20.0f,
		NULL },
	{ 132, -1, 2, 133, -1, 3, 0, 1.0f, 0.0f, 0.3f, 20.0f,
		reply_filter_searching },
	{ 137, -1, 2, 138, -1, 3, 0, 1.0f, 0.0f, 0.3f, 20.0f,
		code_000315b0 },
	{ 139, -1, 2, 140, -1, 4, 0, 0.7f, 0.0f, 0.7f, 20.0f,
		code_000314c0 },
	{ 141, -1, 2, 142, -1, 4, 0, 0.7f, 0.0f, 0.7f, 20.0f,
		code_000314c0 },
	{ 156, -1, 2, 157, -1, 3, 0, 0.5f, 0.0f, 0.7f, 30.0f,
		code_00031570 },
	{ 156, -1, 4, 154, -1, 3, 0, 0.5f, 0.0f, 0.7f, 30.0f,
		code_00031550 },
	{ 158, -1, 4, 154, -1, 3, 0, 0.5f, 0.0f, 0.7f, 30.0f,
		code_00031550 },
	{ 159, -1, 2, 160, -1, 3, 0, 0.5f, 0.0f, 0.7f, 20.0f,
		reply_filter_flee_leader },
	{ 199, -1, 2, 200, -1, 3, 0, 0.8f, 0.0f, 0.5f, 0.0f,
		NULL },
	{ 201, -1, 2, 202, -1, 3, 0, 0.8f, 0.0f, 0.5f, 0.0f,
		NULL },
	{ 203, -1, 2, 204, -1, 3, 0, 0.8f, 0.0f, 0.5f, 0.0f,
		NULL },
	{ -1, -1, -1, -1, -1, -1, 0, 0.0f, 0.0f, 0.0f, 0.0f,
		NULL },
};

char const *global_communication_priority_names[NUMBER_OF_COMMUNICATION_PRIORITIES] =
{
	"none",
	"filler",
	"chatter",
	"talk",
	"communicate",
	"shout",
	"yell",
	"exclaim",
};

char const *global_communication_team_names[NUMBER_OF_COMMUNICATION_TEAMS] =
{
	"human",
	"HUM",
	"covenant",
	"COV",
};

char const *global_communication_type_names[NUMBER_OF_COMMUNICATION_TYPES] =
{
	"death",
	"killing_spree",
	"hurt",
	"damage",
	"sighted_enemy",
	"found_enemy",
	"unexpected_enemy",
	"found_dead_friend",
	"allegiance_changed",
	"grenade_throwing",
	"grenade_startle",
	"grenade_sighted",
	"grenade_danger",
	"lost_contact",
	"blocked",
	"alert_noncombat",
	"search_start",
	"search_query",
	"search_report",
	"search_abandon",
	"search_group_abandon",
	"uncover_start",
	"advance",
	"retreat",
	"cover",
	"sighted_friend_player",
	"shooting",
	"shooting_vehicle",
	"shooting_berserk",
	"shooting_group",
	"shooting_traitor",
	"flee",
	"flee_leader_died",
	"flee_idle",
	"attempted_flee",
	"hiding_finished",
	"vehicle_entry",
	"vehicle_exit",
	"vehicle_woohoo",
	"vehicle_scared",
	"vehicle_falling",
	"surprise",
	"berserk",
	"melee",
	"dive",
	"uncover_exclamation",
	"falling",
	"leap",
	"postcombat_alone",
	"postcombat_unscathed",
	"postcombat_wounded",
	"postcombat_massacre",
	"postcombat_triumph",
	"postcombat_check_enemy",
	"postcombat_check_friend",
	"postcombat_shoot_corpse",
	"postcombat_celebrate",
};

short global_dialogue_event_count = 0;
struct dialogue_event_status *global_dialogue_events = NULL;
short global_reply_event_count = 0;
struct dialogue_event_status *global_reply_events = NULL;

/* ---------- public code */

void ai_communication_initialize(
	void)
{
	struct dialogue_usage const *dialogue;
	struct reply_usage const *reply;
	short *communication_table_index;
	short communication_type;
	short communication_type_index;
	short dialogue_index;

	global_dialogue_event_count = 0;
	dialogue = global_dialogue_table;
	do
	{
		dialogue++;
		global_dialogue_event_count++;
	}
	while (dialogue->communication_type != NONE);

	if (global_dialogue_events == NULL)
	{
		global_dialogue_events = (struct dialogue_event_status *)game_state_malloc(
			"ai communication dialogue",
			NULL,
			16 * global_dialogue_event_count);
		match_vassert(
			"c:\\halo\\SOURCE\\ai\\ai_communication.c",
			0x286,
			global_dialogue_events,
			"ai_communication_initialize: unable to allocate comm dialogue status table");
	}

	global_reply_event_count = 0;
	reply = global_reply_table;
	do
	{
		reply++;
		global_reply_event_count++;
	}
	while (reply->original_vocalization_type != NONE);

	if (global_reply_events == NULL)
	{
		global_reply_events = (struct dialogue_event_status *)game_state_malloc(
			"ai communication replies",
			NULL,
			16 * global_reply_event_count);
		match_vassert(
			"c:\\halo\\SOURCE\\ai\\ai_communication.c",
			0x293,
			global_reply_events,
			"ai_communication_initialize: unable to allocate comm reply status table");
	}

	communication_type_index = 0;
	communication_table_index = global_communication_table_indices;
	while (communication_type_index < NUMBER_OF_COMMUNICATION_TYPES)
	{
		dialogue_index = 0;
		*communication_table_index = NONE;
		dialogue = global_dialogue_table;
		communication_type = 0;

		while (communication_type != NONE)
		{
			if (communication_type == communication_type_index)
			{
				*communication_table_index = dialogue_index;
				break;
			}

			dialogue++;
			communication_type = dialogue->communication_type;
			dialogue_index++;
		}

		communication_type_index++;
		communication_table_index++;
	}

	conversation_data = game_state_data_new("ai conversation", 8, 100);
	match_assert(
		"c:\\halo\\SOURCE\\ai\\ai_communication.c",
		0x2A8,
		conversation_data);
	return;
}

void ai_communication_dispose(
	void)
{
	return;
}

void ai_communication_initialize_for_new_map(
	void)
{
	short event_index;

	ai_globals->dialogue_triggers_enabled = TRUE;
	csmemset(ai_globals->last_chatter_time, 0, sizeof(ai_globals->last_chatter_time));
	csmemset(ai_globals->last_talk_time, 0, sizeof(ai_globals->last_talk_time));
	csmemset(ai_globals->last_shout_time, 0, sizeof(ai_globals->last_shout_time));

	event_index = 0;
	if (((long)global_dialogue_event_count << 1) > 0)
	{
		do
		{
			global_dialogue_events[event_index].disable_until_time = NONE;
			global_dialogue_events[event_index].last_time_spoken = NONE;
			event_index = (short)(event_index + 1);
		}
		while ((long)event_index < (long)global_dialogue_event_count << 1);
	}

	event_index = 0;
	if (((long)global_reply_event_count << 1) > 0)
	{
		do
		{
			global_reply_events[event_index].disable_until_time = NONE;
			global_reply_events[event_index].last_time_spoken = NONE;
			event_index = (short)(event_index + 1);
		}
		while ((long)event_index < (long)global_reply_event_count << 1);
	}

	ai_globals->recent_conversation_count = 0;
	ai_globals->recent_conversation_next_index = 0;
	csmemset(
		ai_globals->recent_conversations,
		0,
		sizeof(ai_globals->recent_conversations));
	data_make_valid(conversation_data);
	return;
}

void ai_communication_dispose_from_old_map(
	void)
{
	data_make_invalid(conversation_data);
	return;
}

void ai_communication_packet_new(
	struct ai_information_packet *information)
{
	match_assert(
		"c:\\halo\\SOURCE\\ai\\ai_communication.c",
		0x300,
		information);
	csmemset(information, 0, sizeof(*information));
	information->target_unit_index = NONE;
	information->communication_type = NONE;
	information->dialogue_type_index = NONE;
	information->damage_category = NONE;
	return;
}

char const *ai_communication_get_type_name(
	short communication_type)
{
	char const *name = "<error>";

	if (communication_type >= 0 &&
		communication_type < NUMBER_OF_COMMUNICATION_TYPES)
	{
		name = global_communication_type_names[communication_type];
	}

	return name;
}

short ai_communication_get_type_by_name(
	char const *name)
{
	short communication_type = NONE;
	short index;

	for (index = 0; index < NUMBER_OF_COMMUNICATION_TYPES; index++)
	{
		if (csstrcmp(global_communication_type_names[index], name) == 0)
		{
			communication_type = index;
		}
	}

	return communication_type;
}

static boolean reply_filter_close(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index)
{
	struct prop_datum *prop;
	long prop_index;
	boolean result;

	result = FALSE;
	if (reply_actor_index != NONE)
	{
		prop_index = prop_get_base_by_unit_index(
			reply_actor_index,
			original_unit_index,
			TRUE,
			TRUE);
		if (prop_index != NONE)
		{
			prop = prop_get(prop_index);
			if (prop->distance < COMMUNICATION_CLOSE_DISTANCE &&
				(prop->line_of_sight == _ai_line_of_sight_clear ||
				prop->line_of_sight == _ai_line_of_sight_occluded))
			{
				result = TRUE;
			}
		}
	}

	return result;
}

static boolean reply_filter_not_close(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index)
{
	struct prop_datum *prop;
	long prop_index;
	boolean result;

	result = FALSE;
	if (reply_actor_index != NONE)
	{
		prop_index = prop_get_base_by_unit_index(
			reply_actor_index,
			original_unit_index,
			TRUE,
			TRUE);
		if (prop_index != NONE)
		{
			prop = prop_get(prop_index);
			if (prop->distance > COMMUNICATION_CLOSE_DISTANCE ||
				(prop->line_of_sight != _ai_line_of_sight_clear &&
				prop->line_of_sight != _ai_line_of_sight_occluded))
			{
				result = TRUE;
			}
		}
	}

	return result;
}

static boolean reply_filter_searching(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index)
{
	struct actor_datum *reply_actor;
	boolean result;

	result = FALSE;
	if (reply_actor_index != NONE)
	{
		reply_actor = actor_get(reply_actor_index);
		switch (reply_actor->state.action)
		{
			case _actor_action_uncover:
				result = (boolean)(reply_actor->state.action_data.uncover.pursuit_location.type ==
					_pursuit_location_position);
				break;

			case _actor_action_search:
				result = TRUE;
				break;
		}
	}

	return result;
}

boolean code_000314c0(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index)
{
	struct unit_datum *original_unit;
	struct actor_datum *original_actor;
	struct actor_datum *reply_actor;
	long original_actor_index;
	boolean result;

	result = FALSE;
	if (reply_filter_close(
		original_unit_index,
		communication,
		reply_actor_index))
	{
		original_unit = unit_get(original_unit_index);
		original_actor_index = original_unit->unit.actor_index;
		if (original_actor_index != NONE && reply_actor_index != NONE)
		{
			original_actor = actor_get(original_actor_index);
			reply_actor = actor_get(reply_actor_index);
			result = original_actor->meta.encounter_index != NONE &&
				original_actor->meta.encounter_index == reply_actor->meta.encounter_index &&
				original_actor->meta.platoon_index == reply_actor->meta.platoon_index;
		}
	}

	return result;
}

boolean code_00031550(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index)
{
	return actor_is_fighting(reply_actor_index);
}

boolean code_00031570(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index)
{
	boolean result;

	result = FALSE;
	if (reply_filter_close(
		original_unit_index,
		communication,
		reply_actor_index) &&
		actor_is_fighting(reply_actor_index))
	{
		result = TRUE;
	}

	return result;
}

boolean code_000315b0(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index)
{
	struct unit_datum *original_unit;
	struct actor_datum *original_actor;
	struct actor_datum *reply_actor;
	struct prop_datum *original_prop;
	struct prop_datum *reply_prop;
	long original_actor_index;
	boolean result;

	result = FALSE;
	if (reply_filter_close(
		original_unit_index,
		communication,
		reply_actor_index))
	{
		original_unit = unit_get(original_unit_index);
		original_actor_index = original_unit->unit.actor_index;
		if (original_actor_index != NONE && reply_actor_index != NONE)
		{
			original_actor = actor_get(original_actor_index);
			reply_actor = actor_get(reply_actor_index);
			if (original_actor->target.target_prop_index != NONE &&
				reply_actor->target.target_prop_index != NONE)
			{
				original_prop = prop_get(original_actor->target.target_prop_index);
				reply_prop = prop_get(reply_actor->target.target_prop_index);
				result = original_prop->unit_index == reply_prop->unit_index;
			}
		}
	}

	return result;
}

static boolean reply_filter_no_certain_target(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index)
{
	struct actor_datum *reply_actor;
	boolean result;

	result = FALSE;
	if (reply_actor_index != NONE)
	{
		reply_actor = actor_get(reply_actor_index);
		if (reply_actor->state.mode == _actor_mode_combat &&
			reply_actor->state.combat_status < _actor_combat_status_certain)
		{
			result = TRUE;
		}
	}

	return result;
}

static boolean reply_filter_flee_leader(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index)
{
	struct actor_datum *reply_actor;
	boolean result;

	result = FALSE;
	if (actor_is_fighting(reply_actor_index))
	{
		reply_actor = actor_get(reply_actor_index);
		if (reply_actor->meta.type == _actor_elite)
		{
			result = TRUE;
		}
	}

	return result;
}

short actor_communication_team(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	short race = actor_type_get_race(actor->meta.type);
	short communication_team = NONE;

	if (TEST_FLAG(race, _race_human_bit))
	{
		communication_team = _ai_communication_team_human;
	}
	else if (TEST_FLAG(race, _race_covenant_bit))
	{
		communication_team = _ai_communication_team_covenant;
	}

	return communication_team;
}

static void code_000318c0(
	long actor_index,
	short type,
	short priority,
	long look_unit_index,
	long prop_index)
{
	struct direction_specification direction;
	short prop_state;

	if (actor_index != NONE &&
		priority > 0 &&
		look_unit_index != NONE &&
		unit_try_and_get(look_unit_index))
	{
		if (prop_index == NONE)
		{
			prop_index = prop_get_active_by_unit_index(
				actor_index,
				look_unit_index);
			if (prop_index == NONE)
			{
				goto look_at_point;
			}
		}

		prop_state = prop_get(prop_index)->state;
		if (prop_state >= _prop_state_becoming_unacknowledged &&
			prop_state <= _prop_state_acknowledged &&
			prop_index != NONE)
		{
			direction.type = _direction_specification_prop;
			direction.prop_index = prop_index;
		}
		else
		{
		look_at_point:
			direction.type = _direction_specification_point;
			unit_get_head_position(
				look_unit_index,
				&direction.point);
		}

		actor_look_secondary(
			actor_index,
			type,
			priority,
			&direction);
	}

	return;
}

static void code_00031970(
	long actor_index,
	short type,
	short priority,
	long object_index)
{
	struct direction_specification direction;

	if (actor_index != NONE &&
		priority > 0 &&
		object_index != NONE &&
		object_try_and_get(object_index))
	{
		direction.type = _direction_specification_object;
		direction.object_index = object_index;
		actor_look_secondary(
			actor_index,
			type,
			priority,
			&direction);
	}

	return;
}

short ai_conversation_status(
	short scenario_conversation_index)
{
	struct data_iterator iterator;
	struct ai_conversation_datum_header *conversation;
	short status = 0;

	data_iterator_new(&iterator, conversation_data);
	while ((conversation = (struct ai_conversation_datum_header *)
		data_iterator_next(&iterator)) != NULL)
	{
		if (conversation->scenario_conversation_index == scenario_conversation_index)
		{
			short conversation_status;

			if (!conversation->begun)
			{
				conversation_status = 1;
			}
			else if (!conversation->any_line_spoken)
			{
				conversation_status = 2;
			}
			else
			{
				conversation_status = conversation->waiting_to_advance ? 4 : 3;
			}

			status = MAX(status, conversation_status);
		}
	}

	if (!status)
	{
		struct recent_conversation_view *recent_conversation;
		long latest_finish_time = NONE;
		short latest_index = NONE;
		short index;

		for (index = 0; index < ai_globals->recent_conversation_count; index++)
		{
			if (ai_globals->recent_conversations[index].scenario_conversation_index ==
				scenario_conversation_index &&
				ai_globals->recent_conversations[index].finish_time > latest_finish_time)
			{
				latest_index = index;
				latest_finish_time = ai_globals->recent_conversations[index].finish_time;
			}
		}

		if (latest_index != NONE)
		{
			recent_conversation = &ai_globals->recent_conversations[latest_index];
			if (recent_conversation->unable_to_begin)
			{
				status = 5;
			}
			else
			{
				status = recent_conversation->finished_successfully ? 6 : 7;
			}
		}
	}

	return status;
}

static void actor_reset_idle_vocalization_timer(
	long actor_index)
{
	struct actor_datum *actor;
	struct actor_definition *definition;
	struct unit_datum *unit;
	boolean in_combat;
	short speech_offset;
	real delay;

	actor = actor_get(actor_index);
	definition = actor_definition_get(actor->meta.definition_index);
	in_combat = actor_in_combat(actor_index);
	speech_offset = 0;
	if (actor->meta.unit_index != NONE)
	{
		unit = unit_get(actor->meta.unit_index);
		if (unit->unit.speech.current.priority > 0)
		{
			speech_offset = unit->unit.speech.sound_timer;
		}
	}

	if (in_combat)
	{
		real maximum_delay;
		real minimum_delay;

		maximum_delay = definition->communication.idle_combat_time_upper_bound;
		minimum_delay = definition->communication.idle_combat_time_lower_bound;
		delay = real_seed_random_range(
			get_global_random_seed_address(),
			minimum_delay,
			maximum_delay);
	}
	else
	{
		real maximum_delay;
		real minimum_delay;

		maximum_delay = definition->communication.idle_noncombat_time_upper_bound;
		minimum_delay = definition->communication.idle_noncombat_time_lower_bound;
		delay = real_seed_random_range(
			get_global_random_seed_address(),
			minimum_delay,
			maximum_delay);
	}

	actor->control.idle_vocalization_combat = in_combat;
	actor->control.idle_vocalization_timer =
		(short)(delay * 30.0f + (real)speech_offset);
	return;
}

short ai_conversation_line(
	short scenario_conversation_index)
{
	struct data_iterator iterator;
	struct ai_conversation_line_view *conversation;
	short line = 999;

	data_iterator_new(&iterator, conversation_data);
	while ((conversation = (struct ai_conversation_line_view *)data_iterator_next(&iterator)) != NULL)
	{
		if (conversation->header.scenario_conversation_index == scenario_conversation_index)
		{
			line = conversation->current_line;
			break;
		}
	}

	return line;
}

void ai_conversation_advance(
	short scenario_conversation_index)
{
	struct data_iterator iterator;
	struct ai_conversation_datum_header *conversation;

	data_iterator_new(&iterator, conversation_data);
	while ((conversation = (struct ai_conversation_datum_header *)
		data_iterator_next(&iterator)) != NULL)
	{
		if (conversation->scenario_conversation_index == scenario_conversation_index)
		{
			if (ai_print_conversations)
			{
				struct scenario_conversation_definition_view *definition =
					TAG_BLOCK_GET_ELEMENT(
						&global_scenario_get()->ai_conversations,
						scenario_conversation_index,
						struct scenario_conversation_definition_view);

				console_printf(
					FALSE,
					"%s: told to advance by scripting",
					definition->name);
			}

			conversation->told_to_advance = TRUE;
		}
	}

	return;
}

void ai_conversation_finish(
	long conversation_index,
	boolean unable_to_begin,
	boolean success)
{
	struct ai_conversation_datum_view *conversation;
	struct ai_conversation_datum_view *recent_conversation;
	struct scenario_conversation_definition_view *definition;
	struct actor_datum *actor;
	short recent_conversation_index;
	long recent_conversation_count;
	unsigned long participant_bitmask;
	short participant_index;
	short actor_action;

	if (conversation_index != NONE)
	{
		conversation = (struct ai_conversation_datum_view *)datum_get(
			conversation_data,
			conversation_index);
		definition = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_conversations,
			conversation->scenario_conversation_index,
			struct scenario_conversation_definition_view);

		if (ai_print_conversations)
		{
			console_printf(
				FALSE,
				"%s: finished %s%s",
				definition->name,
				success ? "successfully" : "prematurely",
				unable_to_begin ? " (unable to begin)" : "");
		}

		recent_conversation = (struct ai_conversation_datum_view *)datum_get(
			conversation_data,
			conversation_index);
		recent_conversation_index =
			(word)ai_globals->recent_conversation_next_index;
		ai_globals->recent_conversation_next_index =
			(short)(recent_conversation_index + 1);
		ai_globals->recent_conversation_next_index %=
			MAXIMUM_RECENT_CONVERSATIONS;

		recent_conversation_count = ai_globals->recent_conversation_count;
		if (recent_conversation_count <= recent_conversation_index + 1)
		{
			recent_conversation_count = recent_conversation_index + 1;
		}
		ai_globals->recent_conversation_count =
			(short)recent_conversation_count;

		ai_globals->recent_conversations[recent_conversation_index]
			.scenario_conversation_index =
			recent_conversation->scenario_conversation_index;
		ai_globals->recent_conversations[recent_conversation_index]
			.unable_to_begin = unable_to_begin;
		ai_globals->recent_conversations[recent_conversation_index]
			.finished_successfully = success;
		ai_globals->recent_conversations[recent_conversation_index]
			.finish_time = game_time_get();

		participant_index = 0;
		while ((long)participant_index < definition->participants.count)
		{
			participant_bitmask = conversation->participant_bitmask;
			if ((participant_bitmask & FLAG(participant_index)) != 0 &&
				conversation->actor_indices[participant_index] != NONE)
			{
				actor = actor_get(conversation->actor_indices[participant_index]);
				actor_action = actor->state.action;
				actor->external_orders.conversation_index = NONE;
				actor->external_orders.conversation_attention_unit_index = NONE;
				if (actor_action == _actor_action_converse)
				{
					actor->state.action_data.converse.conversation_index = NONE;
				}
			}

			participant_index = (short)(participant_index + 1);
		}

		datum_delete(conversation_data, conversation_index);
	}

	return;
}

void ai_conversation_stop(
	short scenario_conversation_index)
{
	struct data_iterator iterator;
	struct ai_conversation_datum_header *conversation;

	data_iterator_new(&iterator, conversation_data);
	while ((conversation = (struct ai_conversation_datum_header *)
		data_iterator_next(&iterator)) != NULL)
	{
		if (conversation->scenario_conversation_index == scenario_conversation_index)
		{
			if (ai_print_conversations)
			{
				struct scenario_conversation_definition_view *definition =
					TAG_BLOCK_GET_ELEMENT(
						&global_scenario_get()->ai_conversations,
						scenario_conversation_index,
						struct scenario_conversation_definition_view);

				console_printf(
					FALSE,
					"%s: told to stop by scripting",
					definition->name);
			}

			ai_conversation_finish(iterator.datum_index, FALSE, FALSE);
		}
	}

	return;
}

void actor_communication_update(
	long actor_index)
{
	struct actor_datum *actor;
	boolean in_combat;

	actor = actor_get(actor_index);
	if (actor->state.mode >= _actor_mode_alert &&
		ai_globals->dialogue_triggers_enabled)
	{
		in_combat = actor_in_combat(actor_index);
		if (actor->control.idle_vocalization_timer == 0 ||
			actor->control.idle_vocalization_combat != in_combat)
		{
			actor_reset_idle_vocalization_timer(actor_index);
		}

		if (actor->control.idle_vocalization_timer > 0 &&
			--actor->control.idle_vocalization_timer == 0)
		{
			long sound_definition_index = NONE;
			long vocalization_type = in_combat != FALSE;
			short play_type = unit_test_speech(
				actor->meta.unit_index,
				_unit_speech_idle,
				TRUE,
				FALSE,
				NULL,
				(short *)&vocalization_type,
				&sound_definition_index);
			if (play_type > 0)
			{
				struct unit_speech_item speech_item;

				csmemset(&speech_item, 0, sizeof(speech_item));
				speech_item.vocalization_type = (short)vocalization_type;
				speech_item.sound_definition_index = sound_definition_index;
				speech_item.priority = _unit_speech_idle;
				ai_communication_packet_new(&speech_item.ai);
				unit_speak(actor->meta.unit_index, play_type, &speech_item);
			}
		}
	}

	return;
}

static void code_000324b0(
	long actor_index,
	long prop_index,
	struct ai_information_packet *information)
{
	struct prop_datum *prop;
	short type;

	if (information->look_type > 0)
	{
		prop = prop_get(prop_index);
		type = _secondary_look_communicated_direction;
		if (information->look_type == _ai_information_look_unit &&
			information->look_data.unit.unit_index == prop->unit_index)
		{
			type = _secondary_look_communicating_prop;
		}

		switch (information->look_type)
		{
		case _ai_information_look_unit:
			code_000318c0(
				actor_index,
				type,
				information->look_priority,
				information->look_data.unit.unit_index,
				NONE);
			break;

		case _ai_information_look_object:
			code_00031970(
				actor_index,
				type,
				information->look_priority,
				information->look_data.object.object_index);
			break;
		}
	}

	return;
}

static void ai_communication_update_speech_timers(
	long unit_index,
	short priority,
	short vocalization_type,
	short dialogue_type_index,
	short reply_table_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	struct actor_datum *actor;
	long time;
	long notification_time;

	if (unit->unit.actor_index == NONE)
	{
		actor = NULL;
	}
	else
	{
		actor = actor_get(unit->unit.actor_index);
	}

	time = game_time_get();
	notification_time = time +
		MAX(0, unit->unit.speech.sound_timer - communication_overlap_time_modifier);
	unit->unit.speech.last_speech_finished_time = notification_time;

	if (actor)
	{
		short communication_team;

		actor_reset_idle_vocalization_timer(unit->unit.actor_index);
		communication_team = actor_communication_team(unit->unit.actor_index);
		if (communication_team != NONE)
		{
			if (priority <= _unit_speech_shout)
			{
				ai_globals->last_chatter_time[communication_team] = MAX(
					ai_globals->last_chatter_time[communication_team],
					notification_time);
				if (priority >= _unit_speech_talk)
				{
					ai_globals->last_talk_time[communication_team] = MAX(
						ai_globals->last_talk_time[communication_team],
						notification_time);
				}
				if (priority >= _unit_speech_shout)
				{
					ai_globals->last_shout_time[communication_team] = MAX(
						ai_globals->last_shout_time[communication_team],
						notification_time);
				}

				if (ai_debug.print_speech_timers)
				{
					char const *team_name =
						global_communication_team_names[communication_team * 2];

					error(
						2,
						"%s %s %d/%s: %s %d",
						team_name,
						unit_get_speech_priority_name(priority),
						dialogue_type_index,
						dialogue_get_vocalization_name(vocalization_type, TRUE),
						priority >= _unit_speech_talk ? "talk" : "chatter",
						notification_time - time);
				}
			}

			if (dialogue_type_index != NONE)
			{
				struct dialogue_event_status *event;
				struct dialogue_usage const *usage;

				match_assert(
					"c:\\halo\\SOURCE\\ai\\ai_communication.c",
					0xC9C,
					(dialogue_type_index >= 0) && (dialogue_type_index < global_dialogue_event_count));
				usage = &global_dialogue_table[dialogue_type_index];
				event = &global_dialogue_events[
					dialogue_type_index * NUMBER_OF_AI_COMMUNICATION_TEAMS + communication_team];
				event->last_time_spoken = time;
				if (game_connection() ||
					!ai_debug.communication_timeout_disabled)
				{
					if (usage->repeat_delay > 0.0f)
					{
						event->disable_until_time = (long)(
							usage->repeat_delay * TICKS_PER_SECOND +
							notification_time);
					}
				}
			}

			if (reply_table_index != NONE)
			{
				struct dialogue_event_status *event;
				struct reply_usage const *usage;

				match_assert(
					"c:\\halo\\SOURCE\\ai\\ai_communication.c",
					0xCAF,
					(reply_table_index >= 0) && (reply_table_index < global_reply_event_count));
				usage = &global_reply_table[reply_table_index];
				event = &global_reply_events[
					reply_table_index * NUMBER_OF_AI_COMMUNICATION_TEAMS + communication_team];
				event->last_time_spoken = time;
				if (game_connection() ||
					!ai_debug.communication_timeout_disabled)
				{
					if (usage->repeat_delay > 0.0f)
					{
						event->disable_until_time = (long)(
							usage->repeat_delay * TICKS_PER_SECOND +
							notification_time);
					}
				}
			}
		}
	}

	return;
}

void ai_communication_started(
	long unit_index,
	short priority,
	short vocalization_type,
	struct ai_information_packet *information)
{
	switch (priority)
	{
		case _unit_speech_none:
		case _unit_speech_idle:
		case _unit_speech_pain:
		case _unit_speech_involuntary:
		case _unit_speech_death:
			break;

		default:
			if (ai_debug.print_vocalizations)
			{
				char string[1024];
				char name[256];
				struct unit_datum *unit = unit_get(unit_index);

				if (unit->unit.actor_index != NONE)
				{
					struct actor_datum *actor = actor_get(unit->unit.actor_index);

					if (actor->meta.encounter_index == NONE)
					{
						csstrcpy(name, "<no encounter>");
					}
					else
					{
						struct encounter_definition *encounter = TAG_BLOCK_GET_ELEMENT(
							&global_scenario_get()->ai_encounters,
							DATUM_INDEX_TO_ABSOLUTE_INDEX(actor->meta.encounter_index),
							struct encounter_definition);

						sprintf(
							name,
							"%s/%s",
							encounter->name,
							TAG_BLOCK_GET_ELEMENT(
								&encounter->squads,
								actor->meta.squad_index,
								struct squad_definition)->name);
					}

					sprintf(
						string,
						"%s/%s: ",
						name,
						actor_type_get_name(actor->meta.type));
				}
				else if (unit->object.name_index != NONE)
				{
					sprintf(
						string,
						"%s: ",
						TAG_BLOCK_GET_ELEMENT(
							&global_scenario_get()->object_names,
							unit->object.name_index,
							struct scenario_object_name)->name);
				}
				else if (unit->unit.player_index != NONE)
				{
					sprintf(
						string,
						"player %d: ",
						DATUM_INDEX_TO_ABSOLUTE_INDEX(unit->unit.player_index));
				}
				else
				{
					sprintf(
						string,
						"unit %04X: ",
						DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_index));
				}

				sprintf(
					temporary,
					"%s %s",
					unit_get_speech_priority_name(priority),
					vocalization_type == NONE ?
						"non-voc" :
						dialogue_get_vocalization_name(vocalization_type, FALSE));
				csstrcat(string, temporary);

				if (information &&
					information->dialogue_type_index != NONE)
				{
					sprintf(
						temporary,
						" [%d/%s]",
						information->dialogue_type_index,
						ai_communication_get_type_name(
							global_dialogue_table[information->dialogue_type_index].communication_type));
					csstrcat(string, temporary);
				}

				console_printf(FALSE, string);
			}

			if (ai_debug.print_speech)
			{
				struct unit_datum *unit = unit_get(unit_index);

				if (unit->unit.speech.current.priority > _unit_speech_none)
				{
					char actor_string[512];
					char speech_string[512];

					error(
						2,
						"%s: %s",
						ai_debug_describe_actor(
							unit->unit.actor_index,
							unit_index,
							FALSE,
							actor_string,
							sizeof(actor_string)),
						unit_describe_speech(
							unit_index,
							TRUE,
							sizeof(speech_string),
							speech_string));
				}
			}

			if (!information->updated_dialogue_timers)
			{
				ai_communication_update_speech_timers(
					unit_index,
					priority,
					vocalization_type,
					information->dialogue_type_index,
					NONE);
			}
			break;
	}

	return;
}

void ai_communication_notify(
	long unit_index,
	short priority,
	short vocalization_type,
	struct ai_information_packet *ai_information)
{
	long sound_volume;
	struct unit_datum *unit;
	struct location const *sound_location;
	short speaker_team;
	long parent_object_index;
	real_point3d speaker_head;
	struct actor_iterator actors;
	struct actor_datum *actor;
	long prop_index;
	struct prop_datum *prop;
	struct actor_position_data sense_position;

	switch (ai_information->information_type)
	{
	case _ai_information_allegiance:
		ai_handle_allegiance_broken_notification(
			ai_information->information_data.allegiance.team1_index,
			ai_information->information_data.allegiance.team2_index,
			ai_information->information_data.allegiance.broken);
		break;
	}

	if (ai_information->information_type != 0 ||
		ai_information->look_priority > 0)
	{
		unit = unit_get(unit_index);
		sound_location = &unit->object.location;
		speaker_team = unit->object.owner_team_index;
		sound_volume = _ai_sound_volume_medium;
		unit_get_head_position(unit_index, &speaker_head);

		if (ai_information->dialogue_type_index != NONE)
		{
			match_assert(
				"c:\\halo\\SOURCE\\ai\\ai_communication.c",
				0x894,
				(ai_information->dialogue_type_index >= 0) && (ai_information->dialogue_type_index < global_dialogue_event_count));
			if (global_dialogue_table[ai_information->dialogue_type_index].communication_priority >=
				_ai_communication_priority_communicate)
			{
				sound_volume = _ai_sound_volume_shout;
			}
		}

		if (unit->object.parent_object_index != NONE)
		{
			parent_object_index = object_get_ultimate_parent(unit_index);
			sound_location = &object_get(parent_object_index)->object.location;
		}

		actor_iterator_new(&actors, TRUE);
		actor = actor_iterator_next(&actors);
		while (actor)
		{
			if (actor->meta.unit_index != unit_index &&
				!game_team_is_enemy(actor->meta.team_index, speaker_team) &&
				!(distance_squared3d(
					&actor->input.position.head_position,
					&speaker_head) > 900.0f))
			{
				prop_index = prop_get_base_by_unit_index(
					actors.index,
					unit_index,
					TRUE,
					TRUE);
				if (prop_index != NONE)
				{
					prop = prop_get(prop_index);
					actor_perception_find_sense_position(
						actors.index,
						&speaker_head,
						prop_index,
						&sense_position);
					if (actor_audibility_at_point(
						actors.index,
						&sense_position,
						&speaker_head,
						sound_location,
						(short)sound_volume,
						1.0f,
						prop->line_of_sight) >= _ai_sound_volume_loud)
					{
						actor_handle_communication(
							actors.index,
							prop_index,
							ai_information);
						code_000324b0(
							actors.index,
							prop_index,
							ai_information);
					}
				}
			}

			actor = actor_iterator_next(&actors);
		}
	}

	return;
}

void ai_conversation_actor_deleted(
	long actor_index)
{
	struct data_iterator iterator;
	struct ai_conversation_datum_view *conversation;
	struct scenario_conversation_definition_view *definition;
	short participant_index;

	data_iterator_new(&iterator, conversation_data);
	conversation = (struct ai_conversation_datum_view *)
		data_iterator_next(&iterator);
	while (conversation != NULL)
	{
		definition = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_conversations,
			conversation->scenario_conversation_index,
			struct scenario_conversation_definition_view);
		participant_index = 0;
		while ((long)participant_index < definition->participants.count)
		{
			if (conversation->actor_indices[participant_index] == actor_index)
			{
				if ((definition->flags & FLAG(
					_ai_conversation_stop_if_anyone_dies_bit)) != 0)
				{
					ai_conversation_finish(
						iterator.datum_index,
						FALSE,
						FALSE);
					break;
				}

				conversation->participant_bitmask &= ~FLAG(participant_index);
				conversation->actor_indices[participant_index] = NONE;
				if (conversation->line_participant_index == participant_index)
				{
					conversation->line_advance = TRUE;
				}
			}

			participant_index = (short)(participant_index + 1);
		}

		conversation = (struct ai_conversation_datum_view *)
			data_iterator_next(&iterator);
	}

	return;
}

void ai_conversation_unit_died(
	long unit_index,
	boolean deleted)
{
	struct data_iterator iterator;
	short participant_index;
	struct ai_conversation_datum_view *conversation;
	struct scenario_conversation_definition_view *definition;
	struct actor_datum *actor;
	boolean referenced;

	data_iterator_new(&iterator, conversation_data);
	conversation = (struct ai_conversation_datum_view *)
		data_iterator_next(&iterator);
	if (conversation == NULL)
	{
		return;
	}
	while (TRUE)
	{
		definition = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_conversations,
			conversation->scenario_conversation_index,
			struct scenario_conversation_definition_view);
		referenced = FALSE;

		if (conversation->line_unit_index == unit_index)
		{
			referenced = TRUE;
			conversation->line_advance = TRUE;
			conversation->line_unit_index = NONE;
		}
		if (conversation->line_address_unit_index == unit_index)
		{
			referenced = TRUE;
			conversation->line_address_unit_index = NONE;
		}
		if (conversation->triggering_player_unit_index == unit_index)
		{
			referenced = TRUE;
			conversation->triggering_player_unit_index = NONE;
		}

		if (deleted ||
			(definition->flags & FLAG(
				_ai_conversation_stop_if_anyone_dies_bit)) != 0)
		{
			participant_index = 0;
			if (participant_index < definition->participants.count)
			{
				do
				{
					if ((conversation->participant_bitmask &
						FLAG(participant_index)) != 0 &&
						conversation->actor_indices[participant_index] != NONE)
					{
						actor = actor_get(
							conversation->actor_indices[participant_index]);
						if (actor->meta.unit_index == unit_index)
						{
							referenced = TRUE;
						}

						if (deleted)
						{
							if (actor->state.action == _actor_action_converse &&
								actor->state.action_data.converse.run_to_unit_index ==
									unit_index)
							{
								actor->state.action_data.converse.run_to_unit_index = NONE;
							}
							if (actor->external_orders.conversation_attention_unit_index ==
								unit_index)
							{
								actor->external_orders.conversation_attention_unit_index = NONE;
							}
						}
					}

					participant_index = (short)(participant_index + 1);
				}
				while (participant_index < definition->participants.count);
			}

			if (referenced)
			{
				break;
			}
		}

		conversation = (struct ai_conversation_datum_view *)
			data_iterator_next(&iterator);
		if (conversation == NULL)
		{
			return;
		}
	}

	if (ai_print_conversations)
	{
		console_printf(
			FALSE,
			"%s: unit died, aborting",
			definition->name);
	}
	ai_conversation_finish(iterator.datum_index, FALSE, FALSE);
	return;
}

long ai_conversation_new(
	short scenario_conversation_index,
	boolean scripted)
{
	struct data_iterator iterator;
	struct ai_conversation_datum_view *conversation;
	long conversation_index = datum_new(conversation_data);

	if (conversation_index == NONE && scripted)
	{
		boolean overwrite_scripted = TRUE;
		long overwrite_creation_time = LONG_MAX;
		long overwrite_conversation_index = NONE;

		data_iterator_new(&iterator, conversation_data);
		while ((conversation = (struct ai_conversation_datum_view *)
			data_iterator_next(&iterator)) != NULL)
		{
			if (conversation->scripted < overwrite_scripted ||
				conversation->creation_time < overwrite_creation_time)
			{
				overwrite_creation_time = conversation->creation_time;
				overwrite_conversation_index = iterator.datum_index;
				overwrite_scripted = conversation->scripted;
			}
		}

		if (overwrite_conversation_index != NONE)
		{
			if (ai_print_conversations)
			{
				struct scenario_conversation_definition_view *definition =
					TAG_BLOCK_GET_ELEMENT(
						&global_scenario_get()->ai_conversations,
						scenario_conversation_index,
						struct scenario_conversation_definition_view);

				console_printf(
					FALSE,
					"%s: this conversation is already running or trying to run, overwrite it",
					definition->name);
			}

			ai_conversation_finish(
				overwrite_conversation_index,
				FALSE,
				FALSE);
			conversation_index = datum_new_at_index(
				conversation_data,
				overwrite_conversation_index);
		}
	}

	if (conversation_index != NONE)
	{
		conversation = (struct ai_conversation_datum_view *)datum_get(
			conversation_data,
			conversation_index);
		conversation->scenario_conversation_index = scenario_conversation_index;
		conversation->line_index = NONE;
		conversation->scripted = scripted;
		conversation->creation_time = game_time_get();
	}

	return conversation_index;
}

static boolean code_00031e80(
	long conversation_index)
{
	struct ai_conversation_driver_datum_view *conversation;
	struct scenario_conversation_definition_view *definition;
	struct scenario_conversation_participant_view *participant;
	struct scenario_conversation_line_view *line;
	struct actor_datum *actor;
	short participant_index;
	long speaker_actor_index;
	boolean result;

	conversation = ai_conversation_driver_get(conversation_index);
	definition = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_conversations,
		conversation->header.scenario_conversation_index,
		struct scenario_conversation_definition_view);
	line = TAG_BLOCK_GET_ELEMENT(
		&definition->lines,
		conversation->current_line,
		struct scenario_conversation_line_view);
	participant_index = line->participant_index;
	result = FALSE;
	if (participant_index >= 0 &&
		participant_index < definition->participants.count &&
		(conversation->participant_bitmask & FLAG(participant_index)) != 0)
	{
		participant = TAG_BLOCK_GET_ELEMENT(
			&definition->participants,
			participant_index,
			struct scenario_conversation_participant_view);
		speaker_actor_index =
			conversation->actor_indices[line->participant_index];
		conversation->line_participant_index = line->participant_index;
		if (speaker_actor_index == NONE)
		{
			conversation->line_actor_index = NONE;
			conversation->line_unit_index = NONE;
			conversation->line_address_unit_index = NONE;
			conversation->line_unspatialized = TRUE;
		}
		else
		{
			actor = actor_get(speaker_actor_index);
			conversation->line_actor_index = speaker_actor_index;
			conversation->line_unit_index = actor->meta.unit_index;
			conversation->line_address_unit_index = NONE;

			switch (line->address_type)
			{
			case _ai_conversation_address_player:
				conversation->line_address_unit_index =
					conversation->header.triggering_player_unit_index;
				break;

			case _ai_conversation_address_participant:
				if (line->address_participant_index >= 0 &&
					line->address_participant_index < definition->participants.count &&
					conversation->actor_indices[line->address_participant_index] != NONE)
				{
					actor = actor_get(
						conversation->actor_indices[line->address_participant_index]);
					conversation->line_address_unit_index = actor->meta.unit_index;
				}
				break;
			}

			conversation->line_unspatialized =
				participant->selection_type == _ai_conversation_selection_radio ||
				participant->selection_type ==
					_ai_conversation_selection_radio_sargeant;
		}

		match_assert(
			"c:\\halo\\SOURCE\\ai\\ai_communication.c",
			0x146B,
			(conversation->dialogue_indices[line->participant_index] >= 0) &&
			(conversation->dialogue_indices[line->participant_index] <
				MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT));
		conversation->line_sound_index = line->dialogue[
			conversation->dialogue_indices[line->participant_index]].index;
		conversation->line_delay_timer = (short)(long)(line->delay_time * 30.0f);
		conversation->line_flags = line->flags;
		conversation->line_advance = FALSE;
		conversation->line_finished = FALSE;
		conversation->line_spoken = FALSE;
		result = TRUE;
	}

	return result;
}

static boolean code_00032030(
	long conversation_index)
{
	struct ai_conversation_driver_datum_view *conversation;
	struct scenario_conversation_definition_view *definition;
	struct actor_datum *actor;
	struct unit_datum *unit;
	long sound_definition_index;
	long speaking_unit_index;
	long speech_sound_definition_index;
	long participant_actor_index;
	short vocalization_type;
	short play_type;
	short participant_index;
	short flags;
	short delay_timer;
	boolean busy;

	conversation = ai_conversation_driver_get(conversation_index);
	definition = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_conversations,
		conversation->header.scenario_conversation_index,
		struct scenario_conversation_definition_view);
	if (!conversation->line_advance)
	{
		if (!conversation->line_spoken)
		{
			busy = FALSE;
			sound_definition_index = conversation->line_sound_index;
			if (sound_definition_index != NONE)
			{
				flags = conversation->line_flags;
				if ((flags &
					(FLAG(_ai_conversation_line_wait_until_speaker_nearby_bit) |
					FLAG(_ai_conversation_line_wait_until_everyone_nearby_bit))) != 0)
				{
					participant_index = 0;
					while ((long)participant_index < definition->participants.count)
					{
						participant_actor_index =
							conversation->actor_indices[participant_index];
						if (participant_actor_index != NONE)
						{
							actor = actor_get(participant_actor_index);
							flags = conversation->line_flags;
							if (((flags & FLAG(
								_ai_conversation_line_wait_until_everyone_nearby_bit)) != 0 ||
								((flags & FLAG(
									_ai_conversation_line_wait_until_speaker_nearby_bit)) != 0 &&
								participant_actor_index == conversation->line_actor_index)) &&
								actor->state.action == _actor_action_converse &&
								actor->state.action_data.converse.run_to_unit_index != NONE &&
								!actor->state.action_data.converse.in_range &&
								!actor->state.action_data.converse.failed)
							{
								busy = TRUE;
							}
						}

						participant_index = (short)(participant_index + 1);
					}
				}

				if (sound_scripted_dialog_is_playing())
				{
					busy = TRUE;
				}

				if (!busy)
				{
					speaking_unit_index = conversation->line_unit_index;
					if (speaking_unit_index == NONE ||
						conversation->line_unspatialized)
					{
						scripted_sound_new(
							conversation->line_sound_index,
							NONE,
							1.0f);
					}
					else
					{
						speech_sound_definition_index = conversation->line_sound_index;
						vocalization_type = NONE;
						play_type = unit_test_speech(
							speaking_unit_index,
							_ai_communication_priority_yell,
							FALSE,
							TRUE,
							NULL,
							&vocalization_type,
							&speech_sound_definition_index);
						if (play_type == TRUE)
						{
							busy = TRUE;
						}
						else if (play_type > 0)
						{
							struct unit_speech_item speech_item;
							long line_sound_index;

							csmemset(&speech_item, 0, sizeof(speech_item));
							line_sound_index = conversation->line_sound_index;
							speech_item.vocalization_type = NONE;
							speech_item.ai.communication_type = NONE;
							speech_item.ai.damage_category = NONE;
							speech_item.ai.dialogue_type_index = NONE;
							speech_item.ai.target_unit_index =
								conversation->line_address_unit_index;
							speech_item.priority = _ai_communication_priority_yell;
							speech_item.sound_definition_index = line_sound_index;
							speech_item.ai.look_priority = TRUE;
							speech_item.ai.look_type = _ai_information_look_unit;
							speech_item.ai.look_data.unit.unit_index =
								conversation->line_unit_index;
							speech_item.ai.information_type = _ai_information_none;

							if (ai_print_conversations)
							{
								console_printf(
									FALSE,
									"%s: speak %s",
									definition->name,
									tag_get_name(line_sound_index));
							}

							unit_speak(
								conversation->line_unit_index,
								play_type,
								&speech_item);
						}
					}
				}
			}

			if (!busy)
			{
				conversation->line_spoken = TRUE;
				conversation->header.any_line_spoken = TRUE;
			}
		}

		if (conversation->line_spoken)
		{
			if (!conversation->line_finished)
			{
				if (conversation->line_unit_index == NONE)
				{
					sound_definition_index = conversation->line_sound_index;
					conversation->line_finished =
						sound_definition_index == NONE ||
							scripted_sound_time(sound_definition_index) == 0;
				}
				else
				{
					unit = unit_get(conversation->line_unit_index);
					conversation->line_finished =
						unit->unit.speech.current.priority !=
							_ai_communication_priority_yell;
				}
			}

			if (conversation->line_finished)
			{
				delay_timer = conversation->line_delay_timer;
				if (delay_timer > 0)
				{
					conversation->line_delay_timer = (short)(delay_timer - 1);
					return conversation->line_advance;
				}

				conversation->line_advance = TRUE;
				if ((conversation->line_flags & FLAG(
					_ai_conversation_line_wait_after_until_told_to_advance_bit)) != 0)
				{
					if (!conversation->header.waiting_to_advance)
					{
						conversation->header.waiting_to_advance = TRUE;
						conversation->header.told_to_advance = FALSE;
					}

					if (conversation->header.told_to_advance)
					{
						conversation->header.waiting_to_advance = FALSE;
					}
					else
					{
						conversation->line_advance = FALSE;
					}
				}
			}
		}
	}

	return conversation->line_advance;
}

static struct scenario_conversation_definition_view *code_000322c0(
	long conversation_index)
{
	struct ai_conversation_driver_datum_view *conversation;

	conversation = ai_conversation_driver_get(conversation_index);
	return TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_conversations,
		conversation->header.scenario_conversation_index,
		struct scenario_conversation_definition_view);
}

boolean ai_conversation(
	short scenario_conversation_index,
	boolean scripted)
{
	struct scenario *scenario;
	long conversation_index;
	boolean result;

	scenario = global_scenario_get();
	result = FALSE;
	if (scenario_conversation_index >= 0 &&
		scenario_conversation_index < scenario->ai_conversations.count)
	{
		conversation_index = ai_conversation_new(
			scenario_conversation_index,
			scripted);
		if (ai_print_conversations)
		{
			console_printf(
				FALSE,
				"%s: script tried to start conversation",
				TAG_BLOCK_GET_ELEMENT(
					&global_scenario_get()->ai_conversations,
					scenario_conversation_index,
					struct scenario_conversation_definition_view)->name);
		}

		if (conversation_index == NONE)
		{
			error(
				2,
				"WARNING: too many executing conversations (ran out of MAXIMUM_CONVERSATIONS_PER_MAP %d)",
				128);
		}
		else
		{
			boolean continue_trying;

			continue_trying = FALSE;
			if (code_00034020(conversation_index, &continue_trying))
			{
				if (ai_print_conversations)
				{
					console_printf(
						FALSE,
						"%s: begun successfully",
						TAG_BLOCK_GET_ELEMENT(
							&global_scenario_get()->ai_conversations,
							scenario_conversation_index,
							struct scenario_conversation_definition_view)->name);
				}

				result = TRUE;
			}
			else if (continue_trying)
			{
				if (ai_print_conversations)
				{
					console_printf(
						FALSE,
						"%s: can't begin yet but will remember and keep trying it",
						TAG_BLOCK_GET_ELEMENT(
							&global_scenario_get()->ai_conversations,
							scenario_conversation_index,
							struct scenario_conversation_definition_view)->name);
				}

				result = TRUE;
			}
			else
			{
				if (ai_print_conversations)
				{
					console_printf(
						FALSE,
						"%s: could not start, and not set to keep trying... aborting (status 5)",
						TAG_BLOCK_GET_ELEMENT(
							&global_scenario_get()->ai_conversations,
							scenario_conversation_index,
							struct scenario_conversation_definition_view)->name);
				}

				ai_conversation_finish(conversation_index, TRUE, FALSE);
			}
		}
	}

	return result;
}

void ai_conversation_update(
	void)
{
	struct data_iterator iterator;
	struct ai_conversation_driver_datum_view *conversation;
	long time;

	time = game_time_get();
	data_iterator_new(&iterator, conversation_data);
	conversation = (struct ai_conversation_driver_datum_view *)data_iterator_next(&iterator);
	while (conversation)
	{
		struct scenario_conversation_definition_view *definition;

		definition = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_conversations,
			conversation->header.scenario_conversation_index,
			struct scenario_conversation_definition_view);
		if (!conversation->header.begun)
		{
			boolean continue_trying;

			continue_trying = TRUE;
			if ((time - conversation->header.creation_time) % 30 == 0)
			{
				if (ai_print_conversations)
				{
					console_printf(
						FALSE,
						"%s: trying to begin",
						definition->name);
				}

				code_00034020(iterator.datum_index, &continue_trying);
			}

			if (!conversation->header.begun && !continue_trying)
			{
				if (ai_print_conversations)
				{
					console_printf(
						FALSE,
						"%s: unable to begin, and no point in continuing",
						definition->name);
				}

				ai_conversation_finish(iterator.datum_index, TRUE, FALSE);
			}
		}

		if (conversation->header.begun && !conversation->header.finished)
		{
			boolean perform;

			perform = conversation->current_line >= 0 &&
				conversation->current_line < definition->lines.count;
			while (!perform || code_00032030(iterator.datum_index))
			{
				if (perform)
				{
					code_000322c0(iterator.datum_index);
				}

				conversation->current_line++;
				if (conversation->current_line < definition->lines.count)
				{
					perform = code_00031e80(iterator.datum_index);
				}
				else
				{
					if (ai_print_conversations)
					{
						console_printf(
							FALSE,
							"%s: no more lines to play",
							definition->name);
					}

					conversation->header.finished = TRUE;
					break;
				}
			}
		}

		if (conversation->header.finished)
		{
			ai_conversation_finish(iterator.datum_index, FALSE, TRUE);
		}
		else if (conversation->header.begun)
		{
			short participant_index;

			for (participant_index = 0;
				participant_index < definition->participants.count;
				participant_index++)
			{
				if (TEST_FLAG(conversation->participant_bitmask, participant_index) &&
					conversation->actor_indices[participant_index] != NONE)
				{
					struct actor_datum *actor;
					long actor_unit_index;
					long speaker_unit_index;

					actor = actor_get(conversation->actor_indices[participant_index]);
					actor->external_orders.conversation_index = iterator.datum_index;
					actor_unit_index = actor->meta.unit_index;
					actor->external_orders.conversation_attention_unit_index = NONE;
					speaker_unit_index = conversation->line_unit_index;
					if (actor_unit_index == speaker_unit_index)
					{
						actor->external_orders.conversation_attention_unit_index =
							conversation->line_address_unit_index;
					}
					else
					{
						long address_unit_index;

						address_unit_index = conversation->line_address_unit_index;
						if (actor_unit_index == address_unit_index &&
							TEST_FLAG(
								conversation->line_flags,
								_ai_conversation_line_addressee_look_back_bit))
						{
							actor->external_orders.conversation_attention_unit_index =
								speaker_unit_index;
						}
						else if (TEST_FLAG(
							conversation->line_flags,
							_ai_conversation_line_everyone_look_at_speaker_bit))
						{
							actor->external_orders.conversation_attention_unit_index =
								speaker_unit_index;
						}
						else if (TEST_FLAG(
							conversation->line_flags,
							_ai_conversation_line_everyone_look_at_addressee_bit))
						{
							actor->external_orders.conversation_attention_unit_index =
								address_unit_index;
						}
					}
				}
			}
		}

		conversation = (struct ai_conversation_driver_datum_view *)data_iterator_next(&iterator);
	}

	return;
}

/* ---------- private code */
