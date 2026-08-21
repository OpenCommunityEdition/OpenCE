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
	_code_00031390 (0000)
00031400 0070:
	_code_00031400 (0000)
00031470 0050:
	_code_00031470 (0000)
000314C0 0090:
	_code_000314c0 (0000)
00031550 0020:
	_code_00031550 (0000)
00031570 0040:
	_code_00031570 (0000)
000315B0 00b0:
	_code_000315b0 (0000)
00031660 0040:
	_code_00031660 (0000)
000316A0 0040:
	_code_000316a0 (0000)
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
	_code_00031d50 (0000)
00031E80 01b0:
	_code_00031e80 (0000)
00032030 0290:
	_code_00032030 (0000)
000322C0 0030:
	_code_000322c0 (0000)
000322F0 00d0:
	_code_000322f0 (0000)
000323C0 00f0:
	_actor_communication_update (0000)
000324B0 0080:
	_code_000324b0 (0000)
00032530 02a0:
	_code_00032530 (0000)
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
#include "main/console.h"
#include "memory/data.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"

/* ---------- constants */

enum
{
	NUMBER_OF_COMMUNICATION_PRIORITIES = 8,
	NUMBER_OF_COMMUNICATION_TEAMS = 4,
	NUMBER_OF_COMMUNICATION_TYPES = 57,
	NUMBER_OF_COMMUNICATION_TIMER_TYPES = 5,
	NUMBER_OF_DIALOGUE_USAGES = 105,
	NUMBER_OF_REPLY_USAGES = 46,
};

/* ---------- macros */

#define ai_print_conversations (ai_debug.__unknown3C[99])

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

struct scenario_conversation_definition_view
{
	char name[32];
	byte __unknown20[0x54];
};

typedef char ai_conversation_datum_header_size_assert[
	sizeof(struct ai_conversation_datum_header) == 0x14 ? 1 : -1];
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
typedef char ai_print_conversations_offset_assert[
	offsetof(struct ai_debug_state, __unknown3C) + 99 == 0x9F ? 1 : -1];

/* ---------- prototypes */

boolean code_00031390(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
boolean code_00031400(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
boolean code_00031470(
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
boolean code_00031660(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
boolean code_000316a0(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index);
boolean actor_is_fighting(
	long actor_index);

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
		code_00031390 },
	{ 53, 2, 2, 81, -1, 2, 0, 0.0f, 0.5f, 0.7f, 60.0f,
		code_00031390 },
	{ 53, 11, 2, 82, -1, 2, 0, 0.0f, 0.6f, 0.7f, 60.0f,
		code_00031390 },
	{ 53, 10, 2, 83, -1, 2, 0, 0.0f, 0.6f, 0.7f, 60.0f,
		code_00031390 },
	{ 53, 5, 2, 84, -1, 2, 0, 0.0f, 0.8f, 0.7f, 60.0f,
		code_00031390 },
	{ 53, 3, 2, 85, -1, 2, 0, 0.5f, 0.9f, 0.7f, 60.0f,
		code_00031390 },
	{ 53, 4, 2, 86, -1, 2, 0, 0.0f, 1.0f, 0.7f, 60.0f,
		code_00031390 },
	{ 53, 6, 2, 87, -1, 2, 0, 0.0f, 1.0f, 0.7f, 60.0f,
		code_00031390 },
	{ 53, 7, 2, 88, -1, 2, 0, 0.0f, 0.9f, 0.7f, 60.0f,
		code_00031390 },
	{ 53, 12, 2, 89, -1, 2, 0, 0.0f, 0.6f, 0.7f, 60.0f,
		code_00031390 },
	{ 53, 9, 2, 90, -1, 2, 0, 0.0f, 0.8f, 0.7f, 60.0f,
		code_00031390 },
	{ 53, 8, 2, 91, -1, 2, 0, 0.0f, 1.0f, 0.7f, 60.0f,
		code_00031390 },
	{ 55, -1, 2, 56, -1, 4, 1, 1.0f, 1.0f, 0.3f, 0.0f,
		code_00031390 },
	{ 57, -1, 2, 58, -1, 2, 0, 0.8f, 0.6f, 0.5f, 30.0f,
		code_00031390 },
	{ 59, -1, 2, 60, -1, 2, 0, 0.8f, 0.6f, 0.5f, 20.0f,
		code_00031390 },
	{ 61, -1, 2, 62, -1, 2, 0, 0.8f, 0.6f, 0.5f, 20.0f,
		code_00031390 },
	{ 63, -1, 2, 64, -1, 2, 0, 0.8f, 0.6f, 0.5f, 20.0f,
		code_00031390 },
	{ 53, -1, 2, 54, -1, 2, 0, 0.6f, 0.4f, 0.5f, 30.0f,
		code_00031390 },
	{ 53, -1, 2, 80, -1, 2, 0, 0.0f, 0.4f, 0.7f, 40.0f,
		code_00031390 },
	{ 23, -1, 2, 24, -1, 1, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		NULL },
	{ 32, -1, 4, 33, -1, 1, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		NULL },
	{ 32, -1, 2, 34, -1, 1, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		NULL },
	{ 51, -1, 2, 52, -1, 4, 1, 1.0f, 0.0f, 0.3f, 0.0f,
		code_00031390 },
	{ 49, -1, 2, 50, -1, 3, 0, 0.7f, 0.0f, 0.3f, 20.0f,
		code_00031390 },
	{ 29, -1, 3, 30, -1, 2, 0, 0.7f, 0.4f, 0.5f, 20.0f,
		NULL },
	{ 108, -1, 2, 123, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		code_00031390 },
	{ 108, -1, 2, 124, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		code_00031400 },
	{ 109, -1, 2, 123, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		code_00031390 },
	{ 109, -1, 2, 124, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		code_00031400 },
	{ 110, -1, 2, 123, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		code_00031390 },
	{ 110, -1, 2, 124, -1, 3, 0, 0.8f, 0.0f, 0.7f, 20.0f,
		code_00031400 },
	{ 125, -1, 3, 126, -1, 3, 0, 0.7f, 0.0f, 0.5f, 15.0f,
		NULL },
	{ 127, -1, 2, 128, -1, 3, 0, 0.7f, 0.0f, 0.5f, 30.0f,
		code_00031660 },
	{ 129, -1, 3, 130, -1, 3, 0, 0.5f, 0.0f, 0.5f, 20.0f,
		NULL },
	{ 132, -1, 2, 133, -1, 3, 0, 1.0f, 0.0f, 0.3f, 20.0f,
		code_00031470 },
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
		code_000316a0 },
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

/* ---------- public code */

void ai_communication_dispose(
	void)
{
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

boolean code_00031550(
	long original_unit_index,
	struct ai_information_packet *communication,
	long reply_actor_index)
{
	return actor_is_fighting(reply_actor_index);
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

/* ---------- private code */
