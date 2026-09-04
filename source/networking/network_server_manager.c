/*
NETWORK_SERVER_MANAGER.C

symbols in this file:
0011B5D0 0030:
	_countdown_timer_update (0000)
0011B600 0060:
	_countdown_timer_get_time_remaining (0000)
0011B660 0090:
	_countdown_timer_increment (0000)
0011B6F0 0090:
	_countdown_timer_decrement (0000)
0011B780 0040:
	_countdown_timer_set_time_remaining (0000)
0011B7C0 0070:
	_network_game_server_set_game_name (0000)
0011B830 0040:
	_network_game_server_get_game_name (0000)
0011B870 0040:
	_network_game_server_get_state (0000)
0011B8B0 0050:
	_network_game_server_open_game (0000)
0011B900 0050:
	_network_game_server_close_game (0000)
0011B950 0060:
	_network_game_server_game_is_open (0000)
0011B9B0 0060:
	_network_game_server_game_is_valid (0000)
0011BA10 00d0:
	_code_0011ba10 (0000)
0011BAE0 00e0:
	_network_game_server_start_network_game (0000)
0011BBC0 00a0:
	_network_game_server_switch_to_postgame (0000)
0011BC60 00f0:
	_network_game_server_graceful_shutdown (0000)
0011BD50 0060:
	_network_game_server_client_machine_is_joined_to_game (0000)
0011BDB0 0130:
	_network_game_server_accept_client_machine_into_game (0000)
0011BEE0 0040:
	_player_name_is_unique (0000)
0011BF20 0080:
	_get_unique_random_name (0000)
0011BFA0 0090:
	_get_unique_random_color (0000)
0011C030 0140:
	_network_game_server_add_player_to_game (0000)
0011C170 00e0:
	_network_game_server_remove_player_from_game (0000)
0011C250 00a0:
	_network_game_server_adjust_machine_settings (0000)
0011C2F0 0080:
	_network_game_server_all_machines_have_loaded (0000)
0011C370 00c0:
	_network_game_server_client_machine_game_loading_complete (0000)
0011C430 0030:
	_network_game_server_client_machine_is_precached (0000)
0011C460 0150:
	_network_game_server_handle_client_update_packet (0000)
0011C5B0 0050:
	_network_game_server_switch_machine_from_postgame_to_pregame (0000)
0011C600 01b0:
	_network_game_server_update_ticks (0000)
0011C7B0 0070:
	_network_game_server_queue_player_for_addition (0000)
0011C820 0070:
	_network_game_server_begin_game_start_countdown (0000)
0011C890 0080:
	_server_needs_more_teams (0000)
0011C910 0090:
	_server_has_a_player_on_each_machine (0000)
0011C9A0 0070:
	_server_has_enough_machines (0000)
0011CA10 0050:
	_server_ok_to_countdown (0000)
0011CA60 0040:
	_network_game_server_invalidate_network_machine (0000)
0011CAA0 00a0:
	_network_game_generate_join_game_token (0000)
0011CB40 0090:
	_network_game_server_get_client_machine (0000)
0011CBD0 0030:
	_network_game_server_get_connection (0000)
0011CC00 0020:
	_network_game_server_get_client_connection (0000)
0011CC20 0080:
	_network_game_server_get_machine_connection (0000)
0011CCA0 0050:
	_network_game_server_get_client_machine_at_index (0000)
0011CCF0 00d0:
	_network_game_server_get_client_machine_at_address (0000)
0011CDC0 0040:
	_network_game_server_get_game (0000)
0011CE00 0090:
	_network_game_server_get_oldest_client_update_received (0000)
0011CE90 0050:
	_network_game_server_game_can_start (0000)
0011CEE0 0060:
	_network_game_server_pause_countdown (0000)
0011CF40 0100:
	_network_game_server_change_map_name (0000)
0011D040 0090:
	_network_game_server_change_game_variant (0000)
0011D0D0 0170:
	_code_0011d0d0 (0000)
0011D240 00a0:
	_code_0011d240 (0000)
0011D2E0 00a0:
	_code_0011d2e0 (0000)
0011D380 0030:
	_code_0011d380 (0000)
0011D3B0 0050:
	_code_0011d3b0 (0000)
0011D400 0070:
	_code_0011d400 (0000)
0011D470 00d0:
	_code_0011d470 (0000)
0011D540 0030:
	_code_0011d540 (0000)
0011D570 0100:
	_code_0011d570 (0000)
0011D670 0130:
	_code_0011d670 (0000)
0011D7A0 0140:
	_network_game_server_remove_client_machine_from_game (0000)
0011D8E0 0140:
	_network_game_server_remove_machine_from_game (0000)
0011DA20 01d0:
	_network_game_server_stalled_on_client (0000)
0011DBF0 01e0:
	_network_game_server_update_countdown (0000)
0011DDD0 01d0:
	_code_0011ddd0 (0000)
0011DFA0 02b0:
	_code_0011dfa0 (0000)
0011E250 0120:
	_network_game_server_dispose (0000)
0011E370 0180:
	_network_game_server_idle (0000)
0011E4F0 0250:
	_network_game_server_reset_to_pregame (0000)
0011E740 0150:
	_network_game_server_create (0000)
00285104 001b:
	??_C@_0BL@DEKPBLAE@timer?9?$DOtime_remaining?5?$DO?$DN?50?$AA@ (0000)
00285120 0033:
	??_C@_0DD@CFCGIJJL@c?3?2halo?2SOURCE?2networking?2networ@ (0000)
00285154 0010:
	??_C@_0BA@LLGNDOEI@adjustment?5?$DO?$DN?50?$AA@ (0000)
00285164 0007:
	??_C@_06HJHJCKIO@server?$AA@ (0000)
0028516C 000d:
	??_C@_0N@JPPODHNK@opening?5game?$AA@ (0000)
0028517C 000d:
	??_C@_0N@DMKJHMIA@closing?5game?$AA@ (0000)
0028518C 0032:
	??_C@_0DC@GCAPAPEJ@?$CITRUE?5?$DN?$DN?5game_is_open?$CJ?5?$HM?$HM?5?$CIFALSE@ (0000)
002851C0 0034:
	??_C@_0DE@CFAOOLOB@?$CITRUE?5?$DN?$DN?5game_is_valid?$CJ?5?$HM?$HM?5?$CIFALS@ (0000)
002851F8 0086:
	??_C@_0IG@MNBBLADP@network_game_server_send_message@ (0000)
00285280 0024:
	??_C@_0CE@IKCKJLGP@sending?5quit?5out?5of?5game?0?5time?5?$DN@ (0000)
002852A4 0033:
	??_C@_0DD@OJCFEAMH@_network_game_server_state_ingam@ (0000)
002852D8 0043:
	??_C@_0ED@MDKIICLM@failed?5to?5signal?5client?5machines@ (0000)
0028531C 003d:
	??_C@_0DN@JMGGBBEG@signalling?5client?5machines?5to?5be@ (0000)
0028535C 003a:
	??_C@_0DK@JICNHAFB@failed?5to?5create?5a?5_message_type@ (0000)
00285398 003b:
	??_C@_0DL@ICMNGBFO@failed?5to?5signal?5all?5client?5mach@ (0000)
002853D4 002d:
	??_C@_0CN@DJJDFOFP@server?5sent?5message_game_over?5to@ (0000)
00285408 0045:
	??_C@_0EF@JHLALKAD@server?5going?5down?0?5but?5failed?5to@ (0000)
00285450 0040:
	??_C@_0EA@EJEKFFFL@server?5closing?5down?$DL?5all?5client?5@ (0000)
00285490 003d:
	??_C@_0DN@KLHIBLCI@failed?5to?5create?5a?5message_serve@ (0000)
002854D0 003e:
	??_C@_0DO@IAODKJJI@failed?5to?5create?5a?5message_serve@ (0000)
00285510 0008:
	??_C@_07MHDNFCJE@machine?$AA@ (0000)
00285518 006e:
	??_C@_0GO@BGKBFJIE@network_game_server_accept_clien@ (0000)
00285588 005b:
	??_C@_0FL@EIIOKBIG@network_game_add_machine?$CI?$CJ?5faile@ (0000)
002855E4 003b:
	??_C@_0DL@MGNGFOBG@server?5added?5machine?5?$EA?5?$CFs?5to?5the@ (0000)
00285620 004c:
	??_C@_0EM@PJMBGAIG@client?5machine?5tried?5to?5add?5a?5pl@ (0000)
00285670 004d:
	??_C@_0EN@DNDECHNF@network_game_add_player?$CI?$CJ?5failed@ (0000)
002856C0 0049:
	??_C@_0EJ@LCGCJMBE@server?5added?5player?5from?5machine@ (0000)
00285710 004f:
	??_C@_0EP@COKGKJNJ@client?5machine?5tried?5to?5remove?5a@ (0000)
00285760 0055:
	??_C@_0FF@BKKCLGKI@network_game_remove_player?$CI?$CJ?5fai@ (0000)
002857B8 004d:
	??_C@_0EN@LDDPDGNO@server?5removed?5player?5from?5machi@ (0000)
00285808 004d:
	??_C@_0EN@KMKKCDOM@client?5machine?5tried?5to?5update?5i@ (0000)
00285858 0056:
	??_C@_0FG@IPMBAPCA@network_game_update_machine?$CI?$CJ?5fa@ (0000)
002858B0 0024:
	??_C@_0CE@NCDPADPN@server?5updated?5machine?5?$CD?$CFd?5setti@ (0000)
002858D4 0029:
	??_C@_0CJ@ECFINOOJ@server?5?$CG?$CG?5machine?5?$CG?$CG?5machine_des@ (0000)
00285900 001b:
	??_C@_0BL@PFJPBEPF@local?5game?5data?5not?5loaded?$AA@ (0000)
0028591C 0026:
	??_C@_0CG@JHLMGKLE@all?5machines?5have?5successfully?5l@ (0000)
00285944 002f:
	??_C@_0CP@JBNFJJAE@still?5waiting?5on?5machine?5?$CD?$CFd?5to?5@ (0000)
00285978 0047:
	??_C@_0EH@NKIEAGCA@client?5update?5packet?5from?5machin@ (0000)
002859C0 0040:
	??_C@_0EA@BIPOHPNI@received?5an?5outdated?5client?5upda@ (0000)
00285A00 004b:
	??_C@_0EL@GBABNBCE@client?5machine?5?$CD?$CFd?5is?5out?5of?5syn@ (0000)
00285A4C 000f:
	??_C@_0P@OKCBNOPP@message_packet?$AA@ (0000)
00285A5C 0031:
	??_C@_0DB@OKJCLNBN@machine?5?$CD?$CFd?5has?5successfully?5swi@ (0000)
00285A90 0012:
	??_C@_0BC@HJLNGPOP@server?5?$CG?$CG?5machine?$AA@ (0000)
00285AA8 0085:
	??_C@_0IF@CCOGGKBF@network_game_server_send_player_@ (0000)
00285B30 002e:
	??_C@_0CO@BNMPLHFO@server?5failed?5to?5add?5a?5network?5p@ (0000)
00285B60 005d:
	??_C@_0FN@BDPHLOBF@server?5failed?5to?5send?5game?5updat@ (0000)
00285BC0 0011:
	??_C@_0BB@JLJAPHKN@server?5?$CG?$CG?5player?$AA@ (0000)
00285BD4 0024:
	??_C@_0CE@KMCEHK@server?5game?5start?5countdown?5star@ (0000)
00285BF8 000b:
	??_C@_0L@DEEHFDNB@join_token?$AA@ (0000)
00285C04 003c:
	??_C@_0DM@GFDOGMMN@client_machine?9?$DOmachine_index?$DMMA@ (0000)
00285C40 0019:
	??_C@_0BJ@PDBLHLMI@server?5?$CG?$CG?5client_machine?$AA@ (0000)
00285C5C 002c:
	??_C@_0CM@HFLLHELF@server?5?$CG?$CG?5network_machine_is_val@ (0000)
00285C88 0030:
	??_C@_0DA@BAJANDFN@server?5?$CG?$CG?5?$CIindex?$DMMAXIMUM_NETWORK@ (0000)
00285CB8 001b:
	??_C@_0BL@OEHAOKDP@no?5machine?5found?5?$EA?5ip?5?$CD?$CFlX?$AA@ (0000)
00285CD4 0026:
	??_C@_0CG@LADAADEM@server?9?$DOclient_machines?$FLi?$FN?4conne@ (0000)
00285CFC 0015:
	??_C@_0BF@BGCFGMDN@server?5?$CG?$CG?5ip_address?$AA@ (0000)
00285D18 0056:
	??_C@_0FG@ENHBPECG@network_game_server_change_map_n@ (0000)
00285D70 0034:
	??_C@_0DE@OPKCBBPP@server?9?$DOstate?5?$DN?$DN?5_network_game_s@ (0000)
00285DA4 0022:
	??_C@_0CC@DFOMMGAI@server?5?$CG?$CG?5map_name?5?$CG?$CG?5map_name?$FL0@ (0000)
00285DC8 005a:
	??_C@_0FK@HEDIBIPF@network_game_server_change_game_@ (0000)
00285E24 0012:
	??_C@_0BC@DLOLLBOD@server?5?$CG?$CG?5variant?$AA@ (0000)
00285E38 0047:
	??_C@_0EH@GICIMKGF@network_game_server_add_new_clie@ (0000)
00285E80 0051:
	??_C@_0FB@EIEHAMPG@failed?5to?5find?5an?5available?5mach@ (0000)
00285ED8 0067:
	??_C@_0GH@CKJGDANO@network_connection_get_address?$CI?$CJ@ (0000)
00285F40 0027:
	??_C@_0CH@GMPLHAIM@new?5remote?5connection?5accepted?5f@ (0000)
00285F68 0062:
	??_C@_0GC@GMAADHDI@remote?5system?5tried?5to?5join?5our?5@ (0000)
00285FCC 0019:
	??_C@_0BJ@BMKILIN@server?5?$CG?$CG?5new_connection?$AA@ (0000)
00285FE8 005d:
	??_C@_0FN@HKDNEOAD@network_game_server_handle_datag@ (0000)
00286048 0069:
	??_C@_0GJ@NFGOFIEO@failed?5to?5create?5a?5message_serve@ (0000)
002860B8 0041:
	??_C@_0EB@ILPMAAHI@error?5sending?5rejection?5message?5@ (0000)
002860FC 0038:
	??_C@_0DI@LLMEOPEK@endpoint?5?$CG?$CG?5?$CIreason?5?$DM?5NUMBER_OF_@ (0000)
00286134 0028:
	??_C@_0CI@NMINAOIB@client?5connection?5refused?$DL?5game?5@ (0000)
00286160 0061:
	??_C@_0GB@DCECGEGP@?$CBall_machines_have_precached?5?$HM?$HM?5@ (0000)
002861C4 003e:
	??_C@_0DO@MCCLFBCA@network?5game?5setup?5failed?$DL?5proba@ (0000)
00286204 0016:
	??_C@_0BG@OLFNBGNL@setting?5up?5a?5net?5game?$AA@ (0000)
0028621C 001c:
	??_C@_0BM@JFGLKNKN@?$CFsnumber_of_games_played?5?$CFd?$AA@ (0000)
00286238 001e:
	??_C@_0BO@JOAPPGLE@?$CFsnetwork_game_random_seed?5?$CFx?$AA@ (0000)
00286258 0018:
	??_C@_0BI@BAJBJCMP@?$CFs?7player_list_index?5?$CFx?$AA@ (0000)
00286270 0011:
	??_C@_0BB@EOOHLDJC@?$CFs?7team_index?5?$CFx?$AA@ (0000)
00286284 0017:
	??_C@_0BH@LNCHICCP@?$CFs?7controller_index?5?$CFx?$AA@ (0000)
0028629C 0014:
	??_C@_0BE@HLHKAFCI@?$CFs?7machine_index?5?$CFx?$AA@ (0000)
002862B0 000c:
	??_C@_0M@ONJMLKP@?$CFsplayer?5?$CFd?$AA@ (0000)
002862BC 0012:
	??_C@_0BC@MPDEELBJ@?$CFsplayer_count?5?$CFd?$AA@ (0000)
002862D0 0011:
	??_C@_0BB@NMBMNGAB@?7?$CFsmachine?5?$CFd?5?$CFx?$AA@ (0000)
002862E4 0013:
	??_C@_0BD@FGMHAHMN@?$CFsmachine_count?5?$CFd?$AA@ (0000)
002862F8 0014:
	??_C@_0BE@JAKJKGBL@?$CFsnetwork_game_data?$AA@ (0000)
0028630C 001e:
	??_C@_0BO@KBMOCMCI@?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CKEND?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$AA@ (0000)
0028632C 002c:
	??_C@_0CM@KBELBPJF@?7time_of_first_client_loading_co@ (0000)
00286358 001c:
	??_C@_0BM@EADPCOLB@?7time_of_last_keep_alive?5?$CFd?$AA@ (0000)
00286374 0017:
	??_C@_0BH@BNHPAHB@?7next_update_number?5?$CFd?$AA@ (0000)
0028638C 000b:
	??_C@_0L@EEANCMOB@?7?7flags?5?$CFx?$AA@ (0000)
00286398 0013:
	??_C@_0BD@JKAKGNJM@?7?7machine_index?5?$CFx?$AA@ (0000)
002863AC 0016:
	??_C@_0BG@PPFEBDPA@?7?7stall_start_time?5?$CFd?$AA@ (0000)
002863C4 002a:
	??_C@_0CK@BMPEFDDJ@?7?7last_received_update_sequence_@ (0000)
002863F0 0013:
	??_C@_0BD@NOBCPADN@?7?7connection?5?$CFx?5?$CFs?$AA@ (0000)
00286404 000b:
	??_C@_0L@NIBBMIDE@?7client?5?$CFd?$AA@ (0000)
00286410 0007:
	??_C@_06NEHEIOHK@?$CIdead?$CJ?$AA@ (0000)
00286418 0009:
	??_C@_08MGFPAODM@?$CIactive?$CJ?$AA@ (0000)
00286424 000e:
	??_C@_0O@EFMNOFHN@no?5connection?$AA@ (0000)
00286434 0011:
	??_C@_0BB@HDACINMK@client_machines?3?$AA@ (0000)
00286448 0002:
	??_C@_01GPOEFGEJ@?7?$AA@ (0000)
0028644C 000a:
	??_C@_09LHENMLPO@?7flags?5?$CFx?$AA@ (0000)
00286458 000a:
	??_C@_09JEKGPEEJ@?7state?5?$CFx?$AA@ (0000)
00286464 000f:
	??_C@_0P@IGEGAEKH@?7connection?5?$CFx?$AA@ (0000)
00286474 0020:
	??_C@_0CA@JIMNGMN@?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CKBEGIN?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$CK?$AA@ (0000)
00286494 002d:
	??_C@_0CN@BOOLPEKD@server?5failed?5to?5close?5a?5client?8@ (0000)
002864C8 005b:
	??_C@_0FL@JJMLLPJG@network_game_server_remove_clien@ (0000)
00286528 0080:
	??_C@_0IA@HCKJGBDF@network_game_server_remove_clien@ (0000)
002865A8 0013:
	??_C@_0BD@OGPILLHP@machine?5index?5?$DN?5?$CFx?$AA@ (0000)
002865BC 0021:
	??_C@_0CB@BNBDJEGI@machine?5name?5?$DN?5?$DMnot?5implemented?$DO@ (0000)
002865E0 0067:
	??_C@_0GH@MMONEEJC@attempted?5to?5remove?5an?5invalid?5m@ (0000)
00286648 0069:
	??_C@_0GJ@MMCLLNAF@network_game_server_remove_machi@ (0000)
002866B8 0057:
	??_C@_0FH@BABKECJC@network_game_remove_machine?$CI?$CJ?5fa@ (0000)
00286710 0054:
	??_C@_0FE@BFPCFLEJ@network_game_server_remove_machi@ (0000)
00286768 006f:
	??_C@_0GP@EIJIPAGB@network_game_server_remove_clien@ (0000)
002867D8 0051:
	??_C@_0FB@JOJEBAMD@network_game_server_remove_machi@ (0000)
0028682C 0008:
	??_C@_07DPHHBAJ@removed?$AA@ (0000)
00286834 003c:
	??_C@_0DM@CCJHIKEB@forcibly?5removing?5client?5system?5@ (0000)
00286870 000f:
	??_C@_0P@IOAOPNJI@?$DMunknown?5name?$DO?$AA@ (0000)
00286880 0010:
	??_C@_0BA@JMALINEK@culprit?5?$CB?$DN?5NONE?$AA@ (0000)
00286890 003e:
	??_C@_0DO@BHFBBCGG@server?5?$CG?$CG?5server?9?$DOstate?5?$DN?$DN?5_netw@ (0000)
002868D0 002a:
	??_C@_0CK@CDKFKGHB@failed?5to?5remove?5client?5machine?5@ (0000)
002868FC 0021:
	??_C@_0CB@MJKIPMIF@client?5machine?5removed?5from?5game@ (0000)
00286920 0063:
	??_C@_0GD@ILCKCFLA@network_game_server_handle_clien@ (0000)
00286984 002d:
	??_C@_0CN@GAKEIIC@failed?5to?5remove?5client?5machine?5@ (0000)
002869B4 0024:
	??_C@_0CE@CPAGILJL@client?5machine?5?$CFx?5removed?5from?5g@ (0000)
002869D8 004b:
	??_C@_0EL@DJHOLFOA@forcibly?5removing?5client?5system?5@ (0000)
00286A28 0041:
	??_C@_0EB@FNLOGEMK@failed?5to?5send?5a?5message_server_@ (0000)
00286A6C 0030:
	??_C@_0DA@IPEDOIIA@network_game_server_start_networ@ (0000)
00286A9C 001f:
	??_C@_0BP@CKGEOJEP@booting?5dead?5client?5machine?5?$CFd?$AA@ (0000)
00286ABC 0018:
	??_C@_0BI@BNPOAEPH@network?5server?5disposed?$AA@ (0000)
00286AD4 0036:
	??_C@_0DG@DABJFLIB@network_game_server_memory_do_no@ (0000)
00286B10 0059:
	??_C@_0FJ@KJNIDGHK@network_game_server_handle_clien@ (0000)
00286B70 004b:
	??_C@_0EL@BMJJENN@failed?5to?5create?5a?5_message_type@ (0000)
00286BBC 0034:
	??_C@_0DE@CJDOLHKA@failed?5to?5notify?5all?5clients?5tha@ (0000)
00286BF0 002c:
	??_C@_0CM@DJIIBLPE@notified?5all?5clients?5that?5we?5are@ (0000)
00286C20 004c:
	??_C@_0EM@NAIJHAGO@failed?5to?5create?5a?5_message_type@ (0000)
00286C6C 001d:
	??_C@_0BN@CJOEAMG@the?5server?8s?5game?5is?5invalid?$AA@ (0000)
00286C8C 0021:
	??_C@_0CB@OBPDACNL@network_connection_idle?$CI?$CJ?5failed@ (0000)
00286CB0 0034:
	??_C@_0DE@MJEBDOOH@network_game_server_handle_publi@ (0000)
00286CE4 0034:
	??_C@_0DE@ECHDPAEM@network_game_server_handle_clien@ (0000)
00286D18 0015:
	??_C@_0BF@HDEDCFJD@unknown?5server?5state?$AA@ (0000)
00286D30 0030:
	??_C@_0DA@GGCMIGDD@failed?5to?5add?5new?5client?5connect@ (0000)
00286D60 0035:
	??_C@_0DF@CAHJJMAP@new?5client?5connected?5from?5ip?5?$CFs?5@ (0000)
00286D98 003a:
	??_C@_0DK@OICDNHOL@failed?5to?5signal?5all?5client?5mach@ (0000)
00286DD8 0050:
	??_C@_0FA@CGIOIDCF@the?5playlist?5has?5ended?5?9?5server?5@ (0000)
00286E28 002b:
	??_C@_0CL@GEFANPJI@the?5playlist?5has?5ended?5?9?5server?5@ (0000)
00286E54 001c:
	??_C@_0BM@DEOHMBDO@server?5resetting?5to?5pregame?$AA@ (0000)
00286E70 0027:
	??_C@_0CH@LGNFBGFC@failed?5to?5create?5the?5server?5conn@ (0000)
00286E98 002d:
	??_C@_0CN@LEAKMMND@failed?5to?5initialize?5server?5preg@ (0000)
00286EC8 0037:
	??_C@_0DH@KNNCDAKL@?$CBnetwork_game_server_memory_do_n@ (0000)
00456CF4 0008:
	_network_game_server_memory_do_not_use_directly_in_use (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "bungie_net/network/transport.h"
#include "cseries/cseries_windows.h"
#include "game/game_engine_runtime.h"
#include "game/player_queues_new.h"
#include "game/players.h"
#include "main/main.h"
#include "math/random_math.h"
#include "networking/network_client_manager.h"
#include "networking/network_connection.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_game_ui.h"
#include "networking/network_messages.h"
#include "networking/network_server_manager.h"
#include "networking/network_server_manager_internal.h"
#include "networking/network_server_message_handler.h"
#include "saved games/player_profile.h"
#include "text/unicode.h"

/* ---------- constants */

#define NETWORK_SERVER_MANAGER_FILE "c:\\halo\\SOURCE\\networking\\network_server_manager.c"

enum
{
	MAXIMUM_NETWORK_MACHINE_COUNT = 4,
	MAXIMUM_NETWORK_PLAYER_COUNT = 16,
	NETWORK_GAME_NAME_LENGTH = 16,
	NETWORK_GAME_MAP_NAME_LENGTH = 0x80,
	NETWORK_PLAYER_NAME_LENGTH = 12,
	MAXIMUM_MACHINE_NAME_LENGTH = 32,
	NUMBER_OF_MULTIPLAYER_TEAMS = 2,
	NETWORK_GAME_CLIENT_STALL_TIMEOUT = 2000,
	MAXIMUM_PLAYERS_PER_MACHINE = MAXIMUM_LOCAL_PLAYERS,
	PLAYER_UPDATE_SIZE = 0x20,
	MAXIMUM_GOOD_COLOR_ATTEMPTS = 10,
	_client_update_out_of_sync_bit = 31,
	CLIENT_UPDATE_SEQUENCE_NUMBER_MASK = 0x7FFFFFFF,
	NETWORK_GAME_COUNTDOWN_TIME = 30999,
	NETWORK_GAME_SPLITSCREEN_COUNTDOWN_TIME = 10999,
	NETWORK_GAME_COUNTDOWN_ADJUSTMENT = 5000,
	NETWORK_GAME_MINIMUM_COUNTDOWN_TIME = 999,
	_network_game_client_machine_joined_bit = 1,
	_network_game_client_machine_loaded_bit = 2,
	_network_game_client_machine_precached_bit = 3,
};

enum
{
	_network_game_server_game_open_bit = 0,
	_network_game_server_game_valid_bit,
};

enum
{
	_network_game_server_countdown_event_player_left,
	_network_game_server_countdown_event_player_joined,
	_network_game_server_countdown_event_stop,
	_network_game_server_countdown_event_start_immediately,

	NUMBER_OF_NETWORK_GAME_SERVER_COUNTDOWN_EVENTS
};

enum
{
	_network_game_server_state_pregame,
	_network_game_server_state_ingame,
	_network_game_server_state_postgame,

	NUMBER_OF_NETWORK_GAME_SERVER_STATES
};

/* ---------- macros */

#define network_machine_is_valid(machine) \
	((machine) && (machine)->machine_index >= 0 && \
	(machine)->machine_index < MAXIMUM_NETWORK_MACHINE_COUNT)

/* ---------- structures */

struct countdown_timer
{
	long time_remaining;
	unsigned long last_update_time;
};

struct message_server_game_over
{
	long unused;
};

struct message_server_graceful_game_exit_pregame
{
	long unused;
};

struct message_server_graceful_game_exit_postgame
{
	long unused;
};

struct message_client_game_update
{
	unsigned long update_number;
	short unknown04;
	short player_count;
	struct player_action actions[MAXIMUM_PLAYERS_PER_MACHINE];
};

struct server_update
{
	word player_count;
	word unknown02;
	byte player_updates[MAXIMUM_NETWORK_PLAYER_COUNT * PLAYER_UPDATE_SIZE];
};

struct message_server_game_update
{
	long update_number;
	long random_seed;
	long game_time;
	word unknown0C;
	word player_count;
	byte player_updates[MAXIMUM_NETWORK_PLAYER_COUNT * PLAYER_UPDATE_SIZE];
};

struct network_machine
{
	wchar_t name[32];
	char machine_index;
	byte padding41[3];
};

typedef char network_machine_size_assert[
	sizeof(struct network_machine) == 0x44 ? 1 : -1];

struct network_game_map
{
	long unknown0;
	char name[NETWORK_GAME_MAP_NAME_LENGTH];
};

struct network_game
{
	wchar_t name[NETWORK_GAME_NAME_LENGTH];
	struct network_game_map map;
	struct game_variant variant;
	byte opaque10C;
	char minimum_player_count;
	byte maximum_player_count;
	byte team_count;
	short difficulty;
	short machine_count;
	struct network_machine machines[MAXIMUM_NETWORK_MACHINE_COUNT];
	short player_count;
	struct network_player players[MAXIMUM_NETWORK_PLAYER_COUNT];
	byte opaque426[2];
	long random_seed;
	long number_of_games_played;
	boolean load_ui;
	byte padding431[3];
};

struct network_game_server_client_machine
{
	struct network_connection *connection;
	unsigned long last_received_update_sequence_number;
	unsigned long stall_start_time;
	short machine_index;
	word flags;
};

struct network_game_server_countdown_state
{
	struct countdown_timer timer;
	long last_countdown_message_time;
	boolean active;
	boolean paused;
	boolean complete;
	byte padding0F;
};

struct network_game_server
{
	struct network_connection *connection;
	word state;
	word flags;
	struct network_game game;
	struct network_game_server_client_machine client_machines[MAXIMUM_NETWORK_MACHINE_COUNT];
	long next_update_number;
	long time_of_last_keep_alive;
	unsigned long time_of_first_client_loading_completion;
	struct network_game_server_countdown_state countdown_state;
	struct network_player queued_player;
	boolean queued_player_valid;
	byte padding4B9[3];
};

typedef char network_game_players_offset_assert[
	offsetof(struct network_game, players) == 0x226 ? 1 : -1];
typedef char network_game_variant_has_teams_offset_assert[
	offsetof(struct network_game, variant) +
		offsetof(struct game_variant, has_teams) == 0xC0 ? 1 : -1];
typedef char network_game_size_assert[
	sizeof(struct network_game) == 0x434 ? 1 : -1];
typedef char network_game_server_client_machines_offset_assert[
	offsetof(struct network_game_server, client_machines) == 0x43C ? 1 : -1];
typedef char network_game_server_countdown_state_offset_assert[
	offsetof(struct network_game_server, countdown_state) == 0x488 ? 1 : -1];

/* ---------- prototypes */

void countdown_timer_increment(
	struct countdown_timer *timer,
	long adjustment,
	long maximum);

static long network_game_server_get_client_machine_count(
	struct network_game_server *server);
void get_unique_random_name(
	struct network_game_server *server,
	struct network_player *player);
void get_unique_random_color(
	struct network_game_server *server,
	struct network_player *player);
static boolean player_name_is_unique(
	struct network_game_server *server,
	wchar_t const *name);

/* ---------- globals */

static long network_game_server_next_team_index;

/* ---------- public code */

unsigned long countdown_timer_update(
	struct countdown_timer *timer)
{
	unsigned long update_time = system_milliseconds();
	unsigned long last_update_time = timer->last_update_time;

	timer->last_update_time = update_time;

	if ((long)update_time > (long)last_update_time)
	{
		long elapsed_time = update_time - last_update_time;

		if (elapsed_time < timer->time_remaining)
			timer->time_remaining -= elapsed_time;
		else
			timer->time_remaining = 0;
	}

	return update_time;
}

long countdown_timer_get_time_remaining(
	struct countdown_timer *timer)
{
	long time_remaining;
	unsigned long update_time = system_milliseconds();
	unsigned long last_update_time = timer->last_update_time;

	if ((long)update_time > (long)last_update_time)
	{
		long elapsed_time = update_time - last_update_time;

		if (elapsed_time < timer->time_remaining)
			timer->time_remaining -= elapsed_time;
		else
			timer->time_remaining = 0;
	}

	time_remaining = timer->time_remaining;
	timer->last_update_time = update_time;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x5F,
		timer->time_remaining >= 0);

	return time_remaining;
}

void countdown_timer_decrement(
	struct countdown_timer *timer,
	long adjustment)
{
	unsigned long update_time = system_milliseconds();
	unsigned long last_update_time = timer->last_update_time;

	if ((long)update_time > (long)last_update_time)
	{
		long elapsed_time = update_time - last_update_time;

		if (elapsed_time < timer->time_remaining)
			timer->time_remaining -= elapsed_time;
		else
			timer->time_remaining = 0;
	}

	timer->last_update_time = update_time;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x7E, adjustment >= 0);

	if (timer->time_remaining > adjustment)
		timer->time_remaining -= adjustment;
	else
		timer->time_remaining = 0;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x89,
		timer->time_remaining >= 0);

	return;
}

struct network_connection *network_game_server_get_client_connection(
	struct network_game_server_client_machine *client_machine)
{
	if (client_machine)
		return client_machine->connection;

	return NULL;
}

struct network_connection *network_game_server_get_connection(
	struct network_game_server *server)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x712, server);

	return server->connection;
}

struct network_game *network_game_server_get_game(
	struct network_game_server *server)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x769, server);

	return &server->game;
}

wchar_t *network_game_server_get_game_name(
	struct network_game_server *server)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x1E9, server);

	return server->game.name;
}

boolean network_game_server_set_game_name(
	struct network_game_server *server,
	wchar_t const *name)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x1DD, server);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x1DE, name);

	ustrncpy(server->game.name, name, NETWORK_GAME_NAME_LENGTH - 1);
	server->game.name[NETWORK_GAME_NAME_LENGTH - 1] = 0;

	return FALSE;
}

word network_game_server_get_state(
	struct network_game_server *server,
	short *substate)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x1F2, server);

	if (substate)
		*substate = 0;

	return server->state;
}

void network_game_server_open_game(
	struct network_game_server *server)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x1FC, server);

	SET_FLAG(server->flags, _network_game_server_game_open_bit, TRUE);
	network_server_allow_client_connections(server->connection, TRUE);
	network_event("opening game");

	return;
}

void network_game_server_close_game(
	struct network_game_server *server)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x208, server);

	SET_FLAG(server->flags, _network_game_server_game_open_bit, FALSE);
	network_server_allow_client_connections(server->connection, FALSE);
	network_event("closing game");

	return;
}

void network_game_server_switch_to_postgame(
	struct network_game_server *server)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x2FF, server);

	if (server->state == _network_game_server_state_ingame)
	{
		struct message_server_game_over game_over = { 0 };
		void *message;

		server->state = _network_game_server_state_postgame;

		message = create_network_game_message(
			_message_server_game_over,
			&game_over,
			sizeof(game_over));
		if (message)
		{
			if (network_game_server_send_message_to_all_machines(server, message))
				network_event("server sent message_game_over to all clients");
			else
				network_event("failed to signal all client machines to switch to postgame");
		}
		else
		{
			network_event("failed to create a _message_type_server_game_over message");
		}
	}

	return;
}

boolean network_game_server_graceful_shutdown(
	struct network_game_server *server)
{
	boolean success = FALSE;
	void *message = NULL;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x39F, server);

	switch (network_game_server_get_state(server, NULL))
	{
		case _network_game_server_state_pregame:
		{
			struct message_server_graceful_game_exit_pregame exit_pregame = { 0 };

			message = create_network_game_message(
				_message_server_graceful_game_exit_pregame,
				&exit_pregame,
				sizeof(exit_pregame));
			if (!message)
				network_event("failed to create a message_server_graceful_game_exit_pregame");
		}
		break;

		case _network_game_server_state_postgame:
		{
			struct message_server_graceful_game_exit_postgame exit_postgame = { 0 };

			message = create_network_game_message(
				_message_server_graceful_game_exit_postgame,
				&exit_postgame,
				sizeof(exit_postgame));
			if (!message)
				network_event("failed to create a message_server_graceful_game_exit_postgame");
		}
		break;
	}

	if (message)
	{
		success = network_game_server_send_message_to_all_machines(server, message);
		if (success == TRUE)
		{
			network_event(
				"server closing down; all client machines were properly informed");
		}
		else
		{
			network_event(
				"server going down, but failed to properly inform all client machines");
		}
	}

	return success;
}

void countdown_timer_set_time_remaining(
	struct countdown_timer *timer,
	long time_remaining)
{
	unsigned long update_time = system_milliseconds();

	timer->time_remaining = time_remaining;
	timer->last_update_time = update_time;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x95, timer->time_remaining >= 0);

	return;
}

void network_game_server_client_machine_is_precached(
	struct network_game_server *server,
	struct network_game_server_client_machine *client_machine,
	char const *map_name)
{
	char const *multiplayer_map_name = main_get_multiplayer_map_name();

	if (!csstrcmp(multiplayer_map_name, map_name))
		SET_FLAG(client_machine->flags, _network_game_client_machine_precached_bit, TRUE);

	return;
}

boolean network_game_server_game_is_open(
	struct network_game_server *server)
{
	boolean game_is_open;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x214, server);

	game_is_open = TEST_FLAG(server->flags, _network_game_server_game_open_bit);

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x217,
		(TRUE == game_is_open) || (FALSE == game_is_open));

	return game_is_open;
}

boolean network_game_server_game_is_valid(
	struct network_game_server *server)
{
	boolean game_is_valid;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x220, server);

	game_is_valid = TEST_FLAG(server->flags, _network_game_server_game_valid_bit);

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x223,
		(TRUE == game_is_valid) || (FALSE == game_is_valid));

	return game_is_valid;
}

boolean network_game_server_accept_client_machine_into_game(
	struct network_game_server *server,
	struct network_game_server_client_machine *machine)
{
	boolean success = FALSE;
	long machine_index;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x3DA, server);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x3DB, machine);

	for (machine_index = 0;
		machine_index < MAXIMUM_NETWORK_MACHINE_COUNT;
		machine_index++)
	{
		if (server->game.machines[machine_index].machine_index < 0 ||
			server->game.machines[machine_index].machine_index >=
				MAXIMUM_NETWORK_MACHINE_COUNT)
		{
			struct network_machine new_machine;

			csmemcpy(
				&new_machine,
				&server->game.machines[machine_index],
				sizeof(new_machine));
			new_machine.machine_index = (char)machine_index;

			success = network_game_add_machine(&server->game, &new_machine);
			if (success == TRUE)
			{
				struct transport_address address = { { { 0 } } };

				network_connection_get_address(
					machine->connection,
					&address,
					FALSE);

				network_event(
					"server added machine @ %s to the game at machine index #%d",
					transport_address_to_string(&address),
					machine_index);

				SET_FLAG(
					machine->flags,
					_network_game_client_machine_joined_bit,
					TRUE);
				machine->machine_index = (short)machine_index;
			}
			else
			{
				network_event(
					"network_game_add_machine() failed in network_game_server_accept_client_machine_into_game()");
			}

			break;
		}
	}

	if (machine_index == MAXIMUM_NETWORK_MACHINE_COUNT)
	{
		network_event(
			"network_game_server_accept_client_machine_into_game() failed to find an available opening for the new machine");
	}

	return success;
}

boolean network_game_server_client_machine_is_joined_to_game(
	struct network_game_server *server,
	struct network_game_server_client_machine *machine)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x3CD, server);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x3CE, machine);

	return TEST_FLAG(machine->flags, _network_game_client_machine_joined_bit);
}

boolean network_game_server_switch_machine_from_postgame_to_pregame(
	struct network_game_server *server,
	struct network_game_server_client_machine *machine)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x547, server && machine);

	network_event(
		"machine #%d has successfully switched to pregame",
		machine->machine_index);
	SET_FLAG(machine->flags, _network_game_client_machine_loaded_bit, FALSE);

	return TRUE;
}

void network_game_server_all_machines_have_loaded(
	struct network_game_server *server)
{
	network_event("all machines have successfully loaded");

	server->state = _network_game_server_state_ingame;
	server->time_of_first_client_loading_completion = 0;
	server->game.load_ui = global_network_game_client_get()
		? network_game_client_get_game(global_network_game_client_get())->load_ui
		: FALSE;

	match_vassert(NETWORK_SERVER_MANAGER_FILE, 0x4E0, server->game.load_ui,
		"local game data not loaded");

	return;
}

void network_game_server_client_machine_game_loading_complete(
	struct network_game_server *server,
	struct network_game_server_client_machine *machine)
{
	boolean all_machines_loaded = TRUE;
	long client_machine_index;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x4ED, server);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x4EE, machine);

	SET_FLAG(machine->flags, _network_game_client_machine_loaded_bit, TRUE);

	for (client_machine_index = 0;
		client_machine_index < MAXIMUM_NETWORK_MACHINE_COUNT;
		client_machine_index++)
	{
		struct network_game_server_client_machine *client_machine =
			&server->client_machines[client_machine_index];

		if (client_machine->machine_index >= 0 &&
			client_machine->machine_index < MAXIMUM_NETWORK_MACHINE_COUNT &&
			!TEST_FLAG(client_machine->flags, _network_game_client_machine_loaded_bit))
		{
			network_event(
				"still waiting on machine #%d to finish loading",
				client_machine->machine_index);
			all_machines_loaded = FALSE;
		}
	}

	if (all_machines_loaded == TRUE)
		network_game_server_all_machines_have_loaded(server);

	if (!server->time_of_first_client_loading_completion)
		server->time_of_first_client_loading_completion = system_milliseconds();

	return;
}

void network_game_server_handle_client_update_packet(
	struct network_game_server *server,
	struct network_game_server_client_machine *machine,
	struct message_client_game_update *message_packet)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x51E, server);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x51F, machine);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x520, message_packet);

	if (TEST_FLAG(message_packet->update_number, _client_update_out_of_sync_bit))
	{
		network_event(
			"client machine #%d is out of sync @ game tick #%ld; switching to post-game",
			machine->machine_index,
			game_time_get());

		game_engine_switch_to_postgame();
	}
	else if ((message_packet->update_number & CLIENT_UPDATE_SEQUENCE_NUMBER_MASK) <
		machine->last_received_update_sequence_number)
	{
		network_event(
			"received an outdated client update packet; ignoring (#%d / #%d)",
			message_packet->update_number & CLIENT_UPDATE_SEQUENCE_NUMBER_MASK,
			machine->last_received_update_sequence_number);
	}
	else if (message_packet->player_count < 0 ||
		message_packet->player_count > MAXIMUM_PLAYERS_PER_MACHINE)
	{
		network_event(
			"client update packet from machine #%d had a bad player count; ignoring",
			machine->machine_index);
	}
	else
	{
		struct player_action actions[MAXIMUM_PLAYERS_PER_MACHINE] = { 0 };
		long player_index;

		for (player_index = 0;
			player_index < message_packet->player_count;
			player_index++)
		{
			actions[player_index] = message_packet->actions[player_index];
		}

		update_server_handle_client_update(machine->machine_index, actions);

		machine->last_received_update_sequence_number =
			message_packet->update_number & CLIENT_UPDATE_SEQUENCE_NUMBER_MASK;
	}

	return;
}

boolean network_game_server_add_player_to_game(
	struct network_game_server *server,
	struct network_game_server_client_machine *machine,
	struct network_player *player)
{
	boolean success;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x46C, server);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x46D, machine);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x46E, player);

	if (machine->machine_index == player->machine_index)
	{
		player->team_index = (char)network_game_server_next_team_index;
		network_game_server_next_team_index =
			(network_game_server_next_team_index + 1) % NUMBER_OF_MULTIPLAYER_TEAMS;

		if (!player->name[0])
			get_unique_random_name(server, player);

		if (!player_name_is_unique(server, player->name))
			get_unique_random_name(server, player);

		if (player->primary_color_index == NONE)
			get_unique_random_color(server, player);

		success = network_game_add_player(&server->game, player);
		if (success == TRUE)
		{
			network_event(
				"server added player from machine #%d at controller index #%d to the game",
				player->machine_index,
				player->controller_index);
		}
		else
		{
			network_event(
				"network_game_add_player() failed in network_game_server_add_player_to_game()");
		}
	}
	else
	{
		network_event(
			"client machine tried to add a player with a non-matching machine identifier");
		success = FALSE;
	}

	return success;
}

void network_game_server_update_ticks(
	struct network_game_server *server,
	short tick_count)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x553, server);

	switch (network_game_server_get_state(server, NULL))
	{
		case _network_game_server_state_ingame:
		{
			struct network_game_server_client_machine *client_machine = NULL;
			long client_machine_index;
			short tick_index;

			for (tick_index = 0; tick_index < tick_count; tick_index++)
			{
				struct message_server_game_update game_update;
				struct server_update update;
				long update_number = server->next_update_number++;
				void *message;

				update_server_next_update();
				update_server_build_server_update(NONE, &update, &update_number);

				game_update.update_number = update_number;
				game_update.random_seed = get_random_seed();
				game_update.game_time = game_time_get();
				game_update.player_count = update.player_count;

				csmemcpy(
					game_update.player_updates,
					update.player_updates,
					update.player_count * PLAYER_UPDATE_SIZE);

				message = create_network_game_message(
					_message_server_game_update,
					&game_update,
					sizeof(game_update));
				if (message &&
					!network_game_server_send_message_to_all_machines(server, message))
				{
					network_event(
						"server failed to send game update message to all machines; client machine may be out of sync");
				}
			}

			if (server->queued_player_valid)
			{
				for (client_machine_index = 0;
					client_machine_index < MAXIMUM_NETWORK_MACHINE_COUNT;
					client_machine_index++)
				{
					if (server->client_machines[client_machine_index].machine_index ==
						server->queued_player.machine_index)
					{
						client_machine = &server->client_machines[client_machine_index];
						break;
					}
				}

				if (client_machine &&
					network_game_server_add_player_to_game(
						server,
						client_machine,
						&server->queued_player))
				{
					if (!network_game_server_send_player_joined_info_ingame(
						server,
						&server->queued_player))
					{
						network_event(
							"network_game_server_send_player_joined_info_ingame() failed in network_game_server_handle_message_client_add_player_request_ingame()");
					}
				}
				else
				{
					network_event("server failed to add a network player in-game");
				}

				server->queued_player_valid = FALSE;
			}
		}
		break;

		case _network_game_server_state_postgame:
			game_engine_update();
			break;
	}

	return;
}

void network_game_server_queue_player_for_addition(
	struct network_game_server *server,
	struct network_player *player)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x5DE, server && player);

	if (!server->queued_player_valid && network_player_is_valid(player))
	{
		csmemcpy(&server->queued_player, player, sizeof(server->queued_player));
		server->queued_player_valid = TRUE;
	}

	return;
}

boolean network_game_server_remove_player_from_game(
	struct network_game_server *server,
	struct network_game_server_client_machine *machine,
	struct network_player *player)
{
	boolean success;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x4A0, server);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x4A1, machine);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x4A2, player);

	if (machine->machine_index == player->machine_index)
	{
		success = network_game_remove_player(&server->game, player);
		if (success == TRUE)
		{
			network_event(
				"server removed player from machine #%d at controller index #%d from the game",
				player->machine_index,
				player->controller_index);
		}
		else
		{
			network_event(
				"network_game_remove_player() failed in network_game_server_remove_player_from_game()");
		}
	}
	else
	{
		network_event(
			"client machine tried to remove a player with a non-matching machine identifier");
		success = FALSE;
	}

	return success;
}

boolean network_game_server_adjust_machine_settings(
	struct network_game_server *server,
	struct network_game_server_client_machine *machine,
	struct network_machine *machine_description)
{
	boolean success = FALSE;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x4BF,
		server && machine && machine_description);

	if (machine->machine_index == machine_description->machine_index)
	{
		success = network_game_update_machine(&server->game, machine_description);
		if (success == TRUE)
		{
			network_event(
				"server updated machine #%d settings",
				machine_description->machine_index);
		}
		else
		{
			network_event(
				"network_game_update_machine() failed in network_game_server_adjust_machine_settings()");
		}
	}
	else
	{
		network_event(
			"client machine tried to update itself with a non-matching machine identifier");
	}

	return success;
}

void network_game_server_begin_game_start_countdown(
	struct network_game_server *server,
	long time_remaining)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x5ED, server);

	if (!server->countdown_state.active && !server->countdown_state.paused)
	{
		countdown_timer_set_time_remaining(
			&server->countdown_state.timer,
			time_remaining);
		server->countdown_state.complete = FALSE;
		server->countdown_state.active = TRUE;
		network_event("server game start countdown started");
	}

	return;
}

boolean server_needs_more_teams(
	struct network_game_server *server)
{
	boolean needs_more_teams = FALSE;

	if (server->game.variant.has_teams)
	{
		short player_count_by_team[NUMBER_OF_MULTIPLAYER_TEAMS] = { 0, 0 };
		long player_index;
		long team_index;

		for (player_index = 0;
			player_index < MAXIMUM_NETWORK_PLAYER_COUNT;
			player_index++)
		{
			struct network_player *player = &server->game.players[player_index];

			if (network_player_is_valid(player) &&
				player->team_index >= 0 &&
				player->team_index < NUMBER_OF_MULTIPLAYER_TEAMS)
			{
				player_count_by_team[player->team_index]++;
			}
		}

		for (team_index = 0;
			team_index < NUMBER_OF_MULTIPLAYER_TEAMS;
			team_index++)
		{
			if (player_count_by_team[team_index] == 0)
			{
				needs_more_teams = TRUE;
				break;
			}
		}
	}

	return needs_more_teams;
}

boolean server_has_a_player_on_each_machine(
	struct network_game_server *server)
{
	long client_machine_index;

	for (client_machine_index = 0;
		client_machine_index < MAXIMUM_NETWORK_MACHINE_COUNT;
		client_machine_index++)
	{
		struct network_game_server_client_machine *client_machine =
			&server->client_machines[client_machine_index];

		if (client_machine->machine_index >= 0 &&
			client_machine->machine_index < MAXIMUM_NETWORK_MACHINE_COUNT)
		{
			boolean has_a_player = FALSE;
			long player_index;

			for (player_index = 0;
				player_index < MAXIMUM_NETWORK_PLAYER_COUNT;
				player_index++)
			{
				if (network_player_is_valid(&server->game.players[player_index]) &&
					server->game.players[player_index].machine_index ==
						client_machine->machine_index)
				{
					has_a_player = TRUE;
				}
			}

			if (!has_a_player)
				return FALSE;
		}
	}

	return TRUE;
}

boolean server_has_enough_machines(
	struct network_game_server *server)
{
	boolean has_enough_machines;
	long minimum_machine_count =
		network_game_is_splitscreen_local() ? 1 : 2;
	long machine_count = 0;
	long client_machine_index;

	for (client_machine_index = 0;
		client_machine_index < MAXIMUM_NETWORK_MACHINE_COUNT;
		client_machine_index++)
	{
		struct network_game_server_client_machine *client_machine =
			&server->client_machines[client_machine_index];

		if (client_machine->machine_index >= 0 &&
			client_machine->machine_index < MAXIMUM_NETWORK_MACHINE_COUNT)
		{
			machine_count++;
		}
	}

	has_enough_machines = machine_count >= minimum_machine_count;

	return has_enough_machines;
}

boolean server_ok_to_countdown(
	struct network_game_server *server)
{
	if (server_has_enough_machines(server) &&
		server_has_a_player_on_each_machine(server) &&
		!server_needs_more_teams(server) &&
		server->game.player_count >= server->game.minimum_player_count)
	{
		return TRUE;
	}

	return FALSE;
}

void network_game_server_invalidate_network_machine(
	struct network_machine *machine)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x6C9, machine);

	csmemset(machine, 0, sizeof(*machine));
	machine->machine_index = NONE;

	return;
}

struct network_machine *network_game_server_get_client_machine(
	struct network_game_server *server,
	struct network_game_server_client_machine *client_machine,
	long *machine_index)
{
	struct network_machine *machine;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x701, server && client_machine);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x702,
		client_machine->machine_index<MAXIMUM_NETWORK_MACHINE_COUNT);

	if (machine_index)
		*machine_index = NONE;

	machine = &server->game.machines[client_machine->machine_index];
	if (machine_index)
		*machine_index = machine->machine_index;

	return machine;
}

struct network_connection *network_game_server_get_machine_connection(
	struct network_game_server *server,
	struct network_machine *machine)
{
	struct network_connection *connection = NULL;
	long index;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x72F,
		server && network_machine_is_valid(machine));

	for (index = 0; index < MAXIMUM_NETWORK_MACHINE_COUNT; index++)
	{
		if (server->client_machines[index].machine_index == machine->machine_index)
		{
			connection = server->client_machines[index].connection;
			break;
		}
	}

	return connection;
}

struct network_game_server_client_machine *network_game_server_get_client_machine_at_index(
	struct network_game_server *server,
	long index)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x741,
		server && (index<MAXIMUM_NETWORK_MACHINE_COUNT));

	return &server->client_machines[index];
}

struct network_game_server_client_machine *network_game_server_get_client_machine_at_address(
	struct network_game_server *server,
	unsigned long ip_address)
{
	struct network_game_server_client_machine *client_machine = NULL;
	long i;

	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x74D, server && ip_address);

	for (i = 0; i < MAXIMUM_NETWORK_MACHINE_COUNT; i++)
	{
		if (server->client_machines[i].machine_index >= 0 &&
			server->client_machines[i].machine_index <
				MAXIMUM_NETWORK_MACHINE_COUNT)
		{
			struct transport_address address;

			match_assert(NETWORK_SERVER_MANAGER_FILE, 0x755,
				server->client_machines[i].connection);

			network_connection_get_address(
				server->client_machines[i].connection,
				&address,
				FALSE);

			if (address.address.long_words[0] == ip_address)
			{
				client_machine = &server->client_machines[i];
				break;
			}
		}
	}

	if (i == MAXIMUM_NETWORK_MACHINE_COUNT)
		network_event("no machine found @ ip #%lX", ip_address);

	return client_machine;
}

long network_game_server_get_oldest_client_update_received(
	struct network_game_server *server)
{
	unsigned long oldest_update = (unsigned long)NONE;
	long index;

	for (index = 0; index < MAXIMUM_NETWORK_MACHINE_COUNT; index++)
	{
		struct network_game_server_client_machine *client_machine =
			&server->client_machines[index];

		if (client_machine->machine_index >= 0 &&
			client_machine->machine_index < MAXIMUM_NETWORK_MACHINE_COUNT)
		{
			oldest_update = MIN(
				oldest_update,
				client_machine->last_received_update_sequence_number);
		}
	}

	return oldest_update;
}

boolean network_game_server_game_can_start(
	struct network_game_server *server)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x782, server);

	return server->state == 0 &&
		server->game.player_count >= server->game.minimum_player_count;
}

void network_game_server_pause_countdown(
	struct network_game_server *server,
	boolean pause_countdown)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x78C, server);

	if (pause_countdown == TRUE)
		csmemset(&server->countdown_state, 0, sizeof(server->countdown_state));

	server->countdown_state.paused = pause_countdown;

	return;
}

void network_game_server_change_map_name(
	struct network_game_server *server,
	char const *map_name)
{
	long client_machine_index;

	match_assert(
		NETWORK_SERVER_MANAGER_FILE,
		0x79B,
		server && map_name && map_name[0]);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x79C,
		server->state == _network_game_server_state_pregame);

	for (client_machine_index = 0;
		client_machine_index < MAXIMUM_NETWORK_MACHINE_COUNT;
		client_machine_index++)
	{
		struct network_game_server_client_machine *client_machine =
			&server->client_machines[client_machine_index];

		if (client_machine->machine_index >= 0 &&
			client_machine->machine_index < MAXIMUM_NETWORK_MACHINE_COUNT)
		{
			SET_FLAG(
				client_machine->flags,
				_network_game_client_machine_precached_bit,
				FALSE);
		}
	}

	csstrncpy(
		server->game.map.name,
		map_name,
		NETWORK_GAME_MAP_NAME_LENGTH - 1);
	server->game.map.name[NETWORK_GAME_MAP_NAME_LENGTH - 1] = 0;

	if (!network_game_server_send_game_data_pregame(server))
	{
		network_event(
			"network_game_server_change_map_name() failed to send updated game settings to clients");
	}

	return;
}

void network_game_server_change_game_variant(
	struct network_game_server *server,
	struct game_variant *variant)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x7BE, server && variant);
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x7BF,
		server->state == _network_game_server_state_pregame);

	csmemcpy(&server->game.variant, variant, sizeof(server->game.variant));

	if (!network_game_server_send_game_data_pregame(server))
	{
		network_event(
			"network_game_server_change_game_variant() failed to send updated game settings to clients");
	}

	return;
}

void network_game_server_stalled_on_client(
	struct network_game_server *server,
	boolean stalled)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x59E, server);

	if (stalled)
	{
		unsigned long oldest_update = (unsigned long)NONE;
		long culprit = NONE;
		long client_machine_index;

		for (client_machine_index = 0;
			client_machine_index < MAXIMUM_NETWORK_MACHINE_COUNT;
			client_machine_index++)
		{
			if (server->client_machines[client_machine_index].machine_index >= 0 &&
				server->client_machines[client_machine_index].machine_index <
					MAXIMUM_NETWORK_MACHINE_COUNT &&
				server->client_machines[client_machine_index].last_received_update_sequence_number <
					oldest_update)
			{
				oldest_update =
					server->client_machines[client_machine_index].last_received_update_sequence_number;
				culprit = client_machine_index;
			}
		}

		match_assert(NETWORK_SERVER_MANAGER_FILE, 0x5B1, culprit != NONE);

		if (server->client_machines[culprit].stall_start_time)
		{
			if (system_milliseconds() - server->client_machines[culprit].stall_start_time >=
				NETWORK_GAME_CLIENT_STALL_TIMEOUT)
			{
				char machine_name[MAXIMUM_MACHINE_NAME_LENGTH];
				boolean removed;

				network_event(
					"forcibly removing client system '%s' due to timeout in-game",
					wide_to_ascii(
						server->game.machines[
							server->client_machines[culprit].machine_index].name,
						machine_name,
						MAXIMUM_MACHINE_NAME_LENGTH)
						? machine_name
						: "<unknown name>");

				removed = network_game_server_remove_client_machine_from_game(
					server,
					&server->client_machines[culprit]);

				match_assert(NETWORK_SERVER_MANAGER_FILE, 0x5C1, removed);
			}
		}
		else
		{
			server->client_machines[culprit].stall_start_time = system_milliseconds();
		}

		for (client_machine_index = 0;
			client_machine_index < MAXIMUM_NETWORK_MACHINE_COUNT;
			client_machine_index++)
		{
			if (client_machine_index != culprit)
				server->client_machines[client_machine_index].stall_start_time = 0;
		}
	}
	else
	{
		long client_machine_index;

		for (client_machine_index = 0;
			client_machine_index < MAXIMUM_NETWORK_MACHINE_COUNT;
			client_machine_index++)
		{
			server->client_machines[client_machine_index].stall_start_time = 0;
		}
	}

	return;
}

void network_game_server_update_countdown(
	struct network_game_server *server,
	short countdown_event)
{
	match_assert(NETWORK_SERVER_MANAGER_FILE, 0x66E,
		server && server->state == _network_game_server_state_pregame);

	if (!server->countdown_state.paused)
	{
		if (server_ok_to_countdown(server) ||
			countdown_event == _network_game_server_countdown_event_stop)
		{
			if (server->countdown_state.active == TRUE)
			{
				if (!server->countdown_state.complete)
				{
					switch (countdown_event)
					{
						case _network_game_server_countdown_event_player_left:
							server->countdown_state.complete = TRUE;
							countdown_timer_increment(
								&server->countdown_state.timer,
								NETWORK_GAME_COUNTDOWN_ADJUSTMENT,
								NETWORK_GAME_COUNTDOWN_TIME);
							break;

						case _network_game_server_countdown_event_player_joined:
							server->countdown_state.complete = TRUE;
							if (countdown_timer_get_time_remaining(&server->countdown_state.timer) >
								NETWORK_GAME_MINIMUM_COUNTDOWN_TIME)
							{
								countdown_timer_decrement(
									&server->countdown_state.timer,
									NETWORK_GAME_COUNTDOWN_ADJUSTMENT);
								if (countdown_timer_get_time_remaining(&server->countdown_state.timer) <
									NETWORK_GAME_MINIMUM_COUNTDOWN_TIME)
								{
									countdown_timer_set_time_remaining(
										&server->countdown_state.timer,
										NETWORK_GAME_MINIMUM_COUNTDOWN_TIME);
								}
							}
							break;

						case _network_game_server_countdown_event_stop:
							server->countdown_state.active = FALSE;
							server->countdown_state.complete = TRUE;
							break;

						case _network_game_server_countdown_event_start_immediately:
							server->countdown_state.complete = TRUE;
							countdown_timer_set_time_remaining(&server->countdown_state.timer, 0);
							break;
					}
				}
			}
			else
			{
				countdown_timer_set_time_remaining(&server->countdown_state.timer, 0);

				if (countdown_event == _network_game_server_countdown_event_start_immediately)
				{
					server->countdown_state.complete = FALSE;
					server->countdown_state.active = TRUE;
				}
				else if (!network_game_should_accept_remote_connections() ||
					network_game_server_get_client_machine_count(server) > 1)
				{
					server->countdown_state.active = TRUE;
					countdown_timer_set_time_remaining(
						&server->countdown_state.timer,
						network_game_is_splitscreen_local()
							? NETWORK_GAME_SPLITSCREEN_COUNTDOWN_TIME
							: NETWORK_GAME_COUNTDOWN_TIME);
					server->countdown_state.last_countdown_message_time = 0;
					server->countdown_state.complete = FALSE;
				}
			}
		}
	}

	return;
}

/* ---------- private code */

void get_unique_random_name(
	struct network_game_server *server,
	struct network_player *player)
{
	wchar_t const *name;
	long duplicate_count;

	do
	{
		long player_index;

		name = network_game_get_random_player_name();
		duplicate_count = 0;

		for (player_index = 0;
			player_index < MAXIMUM_NETWORK_PLAYER_COUNT;
			player_index++)
		{
			struct network_player *existing_player =
				&server->game.players[player_index];

			if (network_player_is_valid(existing_player) &&
				!ustrcmp(existing_player->name, name))
			{
				duplicate_count++;
			}
		}
	}
	while (duplicate_count != 0);

	ustrncpy(player->name, name, NETWORK_PLAYER_NAME_LENGTH - 1);
	player->name[NETWORK_PLAYER_NAME_LENGTH - 1] = 0;

	return;
}

void get_unique_random_color(
	struct network_game_server *server,
	struct network_player *player)
{
	long attempt_count = 0;
	long color_index;
	boolean unique;

	do
	{
		long player_index;

		color_index = attempt_count < MAXIMUM_GOOD_COLOR_ATTEMPTS
			? player_profile_get_random_good_color()
			: player_profile_get_random_color();

		unique = TRUE;

		for (player_index = 0;
			player_index < MAXIMUM_NETWORK_PLAYER_COUNT;
			player_index++)
		{
			if (network_player_is_valid(&server->game.players[player_index]) &&
				server->game.players[player_index].primary_color_index == color_index)
			{
				unique = FALSE;
				break;
			}
		}

		attempt_count++;
	}
	while (!unique);

	player->primary_color_index = (short)color_index;

	return;
}

static boolean player_name_is_unique(
	struct network_game_server *server,
	wchar_t const *name)
{
	long player_index;

	for (player_index = 0;
		player_index < MAXIMUM_NETWORK_PLAYER_COUNT;
		player_index++)
	{
		struct network_player *player = &server->game.players[player_index];

		if (network_player_is_valid(player) && !ustrcmp(player->name, name))
			return FALSE;
	}

	return TRUE;
}

static long network_game_server_get_client_machine_count(
	struct network_game_server *server)
{
	long client_machine_count = 0;
	short client_machine_index;

	for (client_machine_index = 0;
		client_machine_index < MAXIMUM_NETWORK_MACHINE_COUNT;
		client_machine_index++)
	{
		if (server->client_machines[client_machine_index].connection &&
			server->client_machines[client_machine_index].machine_index != NONE)
		{
			client_machine_count++;
		}
	}

	return client_machine_count;
}
