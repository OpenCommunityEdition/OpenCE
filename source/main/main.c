/*
MAIN.C

symbols in this file:
000EF6F0 0010:
	_main_get_seconds_elapsed (0000)
000EF700 0080:
	_gamepad_button_is_down (0000)
000EF780 0010:
	_game_connection_set (0000)
000EF790 0010:
	_game_connection (0000)
000EF7A0 0010:
	_main_disallow_persistent_storage (0000)
000EF7B0 0060:
	_main_set_map_name (0000)
000EF810 0010:
	_main_defer_map_map_change (0000)
000EF820 0030:
	_main_set_multiplayer_map_name (0000)
000EF850 0010:
	_main_get_map_name (0000)
000EF860 0010:
	_main_get_multiplayer_map_name (0000)
000EF870 0020:
	_main_set_difficulty (0000)
000EF890 0010:
	_main_get_difficulty (0000)
000EF8A0 0040:
	_code_000ef8a0 (0000)
000EF8E0 01d0:
	_code_000ef8e0 (0000)
000EFAB0 0020:
	_main_reset_map (0000)
000EFAD0 0020:
	_main_revert_map (0000)
000EFAF0 0020:
	_main_skip_cinematic (0000)
000EFB10 0010:
	_main_save_map_nonsafe (0000)
000EFB20 0010:
	_main_saving_map (0000)
000EFB30 0010:
	_main_save_cancel (0000)
000EFB40 0040:
	_main_save_map_safe (0000)
000EFB80 0010:
	_main_won_map (0000)
000EFB90 0010:
	_main_lost_map (0000)
000EFBA0 0020:
	_main_respawn (0000)
000EFBC0 0020:
	_main_save_core (0000)
000EFBE0 0050:
	_main_save_core_name (0000)
000EFC30 0020:
	_main_load_core (0000)
000EFC50 0020:
	_main_load_core_at_startup (0000)
000EFC70 0050:
	_main_load_core_name (0000)
000EFCC0 0050:
	_main_load_core_name_at_startup (0000)
000EFD10 0060:
	_main_switch_structure_bsp (0000)
000EFD70 0030:
	_main_skip (0000)
000EFDA0 0040:
	_main_queue_map_name (0000)
000EFDE0 0050:
	_code_000efde0 (0000)
000EFE30 0020:
	_main_goto_main_menu (0000)
000EFE50 0050:
	_main_menu_precache_resources (0000)
000EFEA0 0020:
	_main_menu_unload (0000)
000EFEC0 0010:
	_main_menu_ensure_player_queues_exist (0000)
000EFED0 0010:
	_main_menu_fade_active (0000)
000EFEE0 0010:
	_main_menu_switch_to_single_player (0000)
000EFEF0 0010:
	_main_set_game_connection_to_film_playback (0000)
000EFF00 0170:
	_main_get_solo_level_from_name (0000)
000F0070 0010:
	_main_get_current_solo_level (0000)
000F0080 0020:
	_main_get_solo_level_name (0000)
000F00A0 0010:
	_main_run_demos (0000)
000F00B0 0070:
	_code_000f00b0 (0000)
000F0120 01f0:
	_compute_window_bounds (0000)
000F0310 0040:
	_main_get_window_count (0000)
000F0350 00d0:
	_code_000f0350 (0000)
000F0420 0180:
	_code_000f0420 (0000)
000F05A0 0020:
	_code_000f05a0 (0000)
000F05C0 0030:
	_code_000f05c0 (0000)
000F05F0 00b0:
	_code_000f05f0 (0000)
000F06A0 0020:
	_code_000f06a0 (0000)
000F06C0 00e0:
	_code_000f06c0 (0000)
000F07A0 0030:
	_code_000f07a0 (0000)
000F07D0 0040:
	_code_000f07d0 (0000)
000F0810 0040:
	_code_000f0810 (0000)
000F0850 0060:
	_code_000f0850 (0000)
000F08B0 0020:
	_code_000f08b0 (0000)
000F08D0 0020:
	_code_000f08d0 (0000)
000F08F0 0040:
	_code_000f08f0 (0000)
000F0930 0010:
	_code_000f0930 (0000)
000F0940 0220:
	_code_000f0940 (0000)
000F0B60 0050:
	_code_000f0b60 (0000)
000F0BB0 0030:
	_code_000f0bb0 (0000)
000F0BE0 0010:
	_code_000f0be0 (0000)
000F0BF0 05a0:
	_code_000f0bf0 (0000)
000F1190 0210:
	_main_rasterizer_throttle (0000)
000F13A0 0020:
	_code_000f13a0 (0000)
000F13C0 0020:
	_main_taking_screenshot (0000)
000F13E0 00b0:
	_main_movie_start (0000)
000F1490 0020:
	_main_movie_stop (0000)
000F14B0 0010:
	_main_stop_time (0000)
000F14C0 0010:
	_main_start_time (0000)
000F14D0 0010:
	_main_crash (0000)
000F14E0 0010:
	_main_print_version (0000)
000F14F0 00c0:
	_main_vertical_blank_interrupt_handler (0000)
000F15B0 0070:
	_main_save_current_solo_map (0000)
000F1620 00c0:
	_main_load_last_solo_map (0000)
000F16E0 0040:
	_main_save_map_no_timeout (0000)
000F1720 00e0:
	_main_load_ui_scenario (0000)
000F1800 0090:
	_main_menu_load (0000)
000F1890 0020:
	_main_roll_credits (0000)
000F18B0 0130:
	_main_pregame_render (0000)
000F19E0 01b0:
	_set_window_camera_values (0000)
000F1B90 0090:
	_main_present_frame (0000)
000F1C20 00c0:
	_code_000f1c20 (0000)
000F1CE0 0050:
	_code_000f1ce0 (0000)
000F1D30 01f0:
	_code_000f1d30 (0000)
000F1F20 02a0:
	_main_framerate_render (0000)
000F21C0 02a0:
	_halt_and_catch_fire (0000)
000F2460 0040:
	_main_loop_of_death (0000)
000F24A0 01c0:
	_code_000f24a0 (0000)
000F2660 0670:
	_main_loop (0000)
002795A8 003a:
	??_C@_0DK@GMPCKHNN@button_index?$DO?$DN0?5?$CG?$CG?5button_index?$DM@ (0000)
002795E4 001b:
	??_C@_0BL@EKABHJPH@c?3?2halo?2SOURCE?2main?2main?4c?$AA@ (0000)
00279600 0037:
	??_C@_0DH@DGKBOONK@?$CIgamepad_index?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIgamepad_i@ (0000)
00279638 0013:
	??_C@_0BD@OOOKAGFE@j?$DMMAXIMUM_GAMEPADS?$AA@ (0000)
00279650 0049:
	??_C@_0EJ@DIAKEACK@?$CIdesired_controllers?$FLi?$FN?$DO?$DN0?$CJ?5?$CG?$CG?5?$CI@ (0000)
0027969C 002c:
	??_C@_0CM@EOEJOPGF@game_connection?$CI?$CJ?5?$DN?$DN?5_game_conne@ (0000)
002796C8 0009:
	??_C@_08FLLKDKH@core?4bin?$AA@ (0000)
002796D4 003b:
	??_C@_0DL@LKJEADLD@warning?0?5core?5file?5name?5will?5be?5@ (0000)
00279710 002c:
	??_C@_0CM@BPJFOCNH@tried?5to?5switch?5to?5invalid?5struc@ (0000)
0027973C 002c:
	??_C@_0CM@IOHNOMHF@tried?5to?5switch?5to?5current?5struc@ (0000)
00279768 0030:
	??_C@_0DA@FGIPPFOE@cannot?5skip?5more?5than?515?5frames?5@ (0000)
00279798 0029:
	??_C@_0CJ@GNBMIDNC@scenario?9?$DOtype?$DN?$DN_scenario_type_m@ (0000)
002797C4 000e:
	??_C@_0O@OJNPMJKK@num_players?$DO0?$AA@ (0000)
002797D4 0039:
	??_C@_0DJ@LGIIGICN@horizontal_index?$DO?$DN0?5?$CG?$CG?5horizonta@ (0000)
00279810 0033:
	??_C@_0DD@NNOBFMAG@vertical_index?$DO?$DN0?5?$CG?$CG?5vertical_in@ (0000)
00279844 0019:
	??_C@_0BJ@LDIFOEOI@player_index?$DMnum_players?$AA@ (0000)
00279860 0017:
	??_C@_0BH@PGKHPIFN@main_new_map?$CI?$CJ?5failed?4?$AA@ (0000)
00279878 0014:
	??_C@_0BE@HPJEDLPC@game_load?$CI?$CJ?5failed?4?$AA@ (0000)
00279890 0041:
	??_C@_0EB@NLDLOJCN@manual?5skipping?5doesn?8t?5work?5out@ (0000)
002798D4 000c:
	??_C@_0M@NBLPJHAH@unsafe?5save?$AA@ (0000)
002798E0 0017:
	??_C@_0BH@JAFGAFKC@gave?5up?5trying?5to?5save?$AA@ (0000)
002798F8 001f:
	??_C@_0BP@JOHDPJOL@?$DLcore_load_name_at_startup?5?$CFs?6?$AA@ (0000)
00279918 0004:
	??_C@_03CCGKKFHG@a?$CLt?$AA@ (0000)
0027991C 000d:
	??_C@_0N@HMFDKLFI@map_name?5?$CFs?6?$AA@ (0000)
0027992C 0003:
	??_C@_02BKFDOEMK@wt?$AA@ (0000)
00279930 000f:
	??_C@_0P@JFNDGJMB@d?3?2?$CFs_init?4txt?$AA@ (0000)
00279940 001e:
	??_C@_0BO@EFIKKGLE@?$CFs_slow_?$CFd_?$CFd_?$CFd_?$CFd_?$CFd_?$CFd?4bin?$AA@ (0000)
00279960 0008:
	__real@3fa26e978d4fdf3c (0000)
00279968 0013:
	??_C@_0BD@BANPCAEP@?5des?5?$CFd?5targ?$CF6I64d?$AA@ (0000)
0027997C 000d:
	??_C@_0N@ONKBMNKN@?5MAINTAIN?5?$CFd?$AA@ (0000)
0027998C 000d:
	??_C@_0N@MLBLHBF@?5RESTORE?5?5?$CFd?$AA@ (0000)
0027999C 000d:
	??_C@_0N@KHJNGILI@?5FAILDOWN?5?$CFd?$AA@ (0000)
002799AC 000d:
	??_C@_0N@KAJPDIPB@?$CI?$CFs?$CF2d?1?$CF2d?$CJ?5?$AA@ (0000)
002799BC 0003:
	??_C@_02LEBOADDI@dn?$AA@ (0000)
002799C0 000c:
	??_C@_0M@FONKJGHM@?$CIok?5?5?5?$CF2d?$CJ?5?$AA@ (0000)
002799CC 0006:
	??_C@_05MPKPLMMK@ignor?$AA@ (0000)
002799D4 0009:
	??_C@_08NGBFCLJE@?$CI?$CFs?$CF2d?$CJ?5?$AA@ (0000)
002799E0 0006:
	??_C@_05KGKFECIJ@fail?5?$AA@ (0000)
002799E8 003a:
	??_C@_0DK@KGLGNIMB@last?$CF6I64d?5init?$CF6I64d?5achv?$CF6I64d@ (0000)
00279A24 0019:
	??_C@_0BJ@GBFFHJIM@?$CF6I64d?$CItarg?$CF6I64d?5?$CFs?$CF2d?$CJ?$AA@ (0000)
00279A40 0009:
	??_C@_08EEAEDAMO@LAPSED?5?5?$AA@ (0000)
00279A4C 0009:
	??_C@_08KMBEHCPI@SYNCED?5?5?$AA@ (0000)
00279A58 0009:
	??_C@_08PLAGFMEO@THROTTLE?$AA@ (0000)
00279A68 004a:
	??_C@_0EK@OBDHCGCK@stuck?5waiting?5for?5VBLANK?5callbac@ (0000)
00279AB4 0006:
	??_C@_05PAONDCEJ@movie?$AA@ (0000)
00279ABC 0019:
	??_C@_0BJ@MFOKCFOP@main_globals?4movie?$DN?$DNNULL?$AA@ (0000)
00279AD8 002a:
	??_C@_0CK@IJIKLLCA@chucky?5was?5here?$CB?5?5NULL?5belongs?5t@ (0000)
00279B04 0031:
	??_C@_0DB@HPIOCFDH@halobeta?5xbox?501?401?414?42342?5Jan?5@ (0000)
00279B38 0038:
	??_C@_0DI@IKBOMHNJ@Couldn?8t?5create?5a?5file?5to?5write?5@ (0000)
00279B70 0011:
	??_C@_0BB@HAHOOKLD@z?3?2last_solo?4txt?$AA@ (0000)
00279B84 0028:
	??_C@_0CI@KPDFMEMF@?$CBmain_globals?4main_menu_scenario@ (0000)
00279BAC 000d:
	??_C@_0N@CBIELCEF@levels?2ui?2ui?$AA@ (0000)
00279BBC 0023:
	??_C@_0CD@IMDNHNHI@congratulations?0?5you?5won?5the?5gam@ (0000)
00279BE0 0014:
	??_C@_0BE@BDDOCJAN@movie?2frame?$CF06d?4tga?$AA@ (0000)
00279BF4 0019:
	??_C@_0BJ@CKCGDFO@error?5opening?5saved?5film?$AA@ (0000)
00279C10 0015:
	??_C@_0BF@LPIHHLNO@?$CFdscreenshot?$CFd?$CFd?4tif?$AA@ (0000)
00279C28 003b:
	??_C@_0DL@LMMABMNB@halobeta?5xbox?501?401?414?42342?5buil@ (0000)
00279C64 001f:
	??_C@_0BP@HEAODFIJ@old?5tags?2internal?5system?5plain?$AA@ (0000)
00279C84 0012:
	??_C@_0BC@FLAPBDJB@end?5of?5saved?5film?$AA@ (0000)
00279C98 0018:
	??_C@_0BI@HBMJOOEK@the?5game?5host?5went?5down?$AA@ (0000)
00307818 0034:
	_global_difficulty_level (0000)
	_player_spawn_count (0004)
	_global_frame_rate_throttle (0006)
	_global_screenshot_size (0008)
00455750 0a63:
	_bss_00455750 (0000)
	_debug_force_frame_rate_update (06d0)
	_debug_no_drawing (06d1)
	_debug_game_save (06d2)
	_debug_frame_rate (06d3)
	_display_framerate (06d4)
	_display_vblank_deltas (06d5)
	_display_precache_progress (06d6)
	_global_screenshot_count (06d8)
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "cseries/profile.h"
#include "main.h"
#include "real_math.h"
#include "game.h"
#include "game_engine.h"
#include "game/cheats.h"
#include "game/player_control.h"
#include "game/player_control_runtime.h"
#include "game/players.h"
#include "game/player_queues_new.h"
#include "integer_math.h"
#include "main/main_runtime.h"
#include "input.h"
#include "shell.h"
#include "event_manager.h"
#include "telnet_console.h"
#include "ui_widget.h"
#include "editor_stubs.h"
#include "console.h"
#include "debug_keys.h"
#include "scenario.h"
#include "scenario/scenario_definitions.h"
#include "cache/cache_files.h"
#include "cache/predicted_resources.h"
#include "bitmaps/bitmaps_internal.h"
#include "interface/hud.h"
#include "interface/terminal.h"
#include "saved games/player_profile.h"
#include "saved games/game_state.h"
#include "rasterizer/rasterizer.h"
#include "bink/bink_playback.h"
#include "main/d3d_intimacy.h"
#include "networking/network_game_globals.h"
#include "camera/director.h"
#include "camera/observer.h"
#include "cutscene/cinematics.h"
#include "physics/collision_usage.h"
#include "physics/collision_debug.h"
#include "hs/hs.h"
#include "render/render.h"
#include "tag_files/files.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/*
struct _main_globals
{
	long long last_time_clocks; // 0x0
	unsigned long last_time_msec; // 0x8
	long long last_render_clocks; // 0x10
	long long last_vblank_index; // 0x18
	long long last_initial_vblank_index; // 0x20
	long long last_achievable_vblank_index; // 0x28
	long long last_present_vblank_index; // 0x30
	unsigned char did_time_overflow_occur; // 0x38
	float seconds_elapsed; // 0x3C
	short connection; // 0x40
	unsigned short screenshot_identifier; // 0x42
	bitmap_data *movie; // 0x44
	long recording_start_tick; // 0x48
	long recording_stop_tick; // 0x4C
	long recording_frame_index; // 0x50
	float recording_dt; // 0x54
	unsigned char reset_map; // 0x58
	unsigned char rename_map; // 0x59
	unsigned char revert_map; // 0x5A
	unsigned char skip_cinematic; // 0x5B
	unsigned char save_map; // 0x5C
	unsigned char save_map_safely; // 0x5D
	unsigned char save_map_timeout; // 0x5E
	unsigned char saving_map; // 0x5F
	long ticks_until_next_save_check; // 0x60
	long ticks_unable_to_save; // 0x64
	unsigned long map_change_load_timer; // 0x68
	short safe_intervals; // 0x6C
	unsigned char won_map; // 0x6E
	unsigned char lost_map; // 0x6F
	unsigned char respawn; // 0x70
	unsigned char save_core; // 0x71
	unsigned char load_core; // 0x72
	unsigned char load_core_at_startup; // 0x73
	short switch_to_structure_bsp_index; // 0x74
	unsigned char main_menu_scenario_loaded; // 0x76
	unsigned char want_to_be_at_main_menu; // 0x77
	unsigned char run_xdemos; // 0x78
	unsigned char fade_to_dashboard; // 0x79
	unsigned char exit_to_dashboard; // 0x7A
	unsigned char want_to_exit; // 0x7B
	long idle_timeout; // 0x7C
	long idle_last_interesting; // 0x80
	long idle_last_activity; // 0x84
	unsigned char playback_last_recording; // 0x88
	unsigned char halt_time_scale; // 0x89
	unsigned char restart_time; // 0x8A
	unsigned char load_last_solo_level; // 0x8B
	unsigned char cutscene_skip; // 0x8C
	short skip_ticks; // 0x8E
	short loss_timer; // 0x90
	short respawn_timer; // 0x92
	unsigned char queue_map; // 0x94
	unsigned char pad0[3]; // 0x95
	unsigned char solo_try_and_load_from_persistent_storage; // 0x98
	char soloplayer_map_name[256]; // 0x99
	char multiplayer_map_name[256]; // 0x199
	char queued_map_name[256]; // 0x299
	unsigned char want_to_connect; // 0x399
	char connect_address[32]; // 0x39A
	char connect_password[9]; // 0x3BA
	short vblank_interval_current; // 0x3C4
	short vblank_interval_minimum; // 0x3C6
	unsigned char vblank_interval_held; // 0x3C8
	short vblank_failure_count[6]; // 0x3CA
	long long vblank_last_failure_time[6]; // 0x3D8
	unsigned long *vblank_flip_counter; // 0x408
	short vblank_flip_delta_next_index; // 0x40C
	short vblank_flip_deltas[15]; // 0x40E
};
*/

struct _main_globals
{
	unsigned long frame_start_milliseconds;
	byte reservedB4[4];
	unsigned long rasterizer_frame_index;
	unsigned long rasterizer_vertical_blank_index;
	byte reservedC0[0x18];
	real seconds_elapsed;
	short connection;
	byte __unknownDE[2];
	struct bitmap_data *movie;
	long recording_start_tick;
	long recording_stop_tick;
	long recording_frame_index;
	real recording_dt;
	boolean reset_map;
	boolean defer_map_change;
	boolean revert_map;
	boolean skip_cinematic;
	boolean saving_map;
	boolean save_map_safely;
	boolean save_map_timeout;
	boolean save_map_completed;
	long ticks_until_next_save_check;
	long ticks_unable_to_save;
	unsigned long map_change_load_timer;
	short safe_intervals;
	boolean won_map;
	boolean lost_map;
	boolean respawn;
	boolean save_core;
	boolean load_core;
	boolean load_core_at_startup;
	short switch_to_structure_bsp_index;
	boolean main_menu_scenario_loaded;
	boolean want_to_be_at_main_menu;
	boolean run_xdemos;
	byte __unknown115;
	boolean halt_time_scale;
	boolean restart_time;
	boolean load_last_solo_level;
	boolean skip;
	short skip_ticks;
	short loss_timer;
	short respawn_timer;
	boolean queue_map;
	byte __unknown121[0x3];
	boolean allow_persistent_storage;
	char soloplayer_map_name[256];
	char multiplayer_map_name[256];
	char queued_map_name[256];
	char core_name[64];
	byte reserved465[0x43];
	volatile unsigned int *d3d_flip_count;
	short vblank_flip_delta_index;
	short vblank_flip_deltas[15];
	byte reserved4CC[0x204];
};

typedef char main_globals_size_assert[
	sizeof(struct _main_globals) == 0x620 ? 1 : -1];
typedef char main_globals_frame_start_milliseconds_offset_assert[
	offsetof(struct _main_globals, frame_start_milliseconds) == 0x00 ? 1 : -1];
typedef char main_globals_rasterizer_frame_index_offset_assert[
	offsetof(struct _main_globals, rasterizer_frame_index) == 0x08 ? 1 : -1];
typedef char main_globals_rasterizer_vertical_blank_index_offset_assert[
	offsetof(struct _main_globals, rasterizer_vertical_blank_index) == 0x0C ? 1 : -1];
typedef char main_globals_seconds_elapsed_offset_assert[
	offsetof(struct _main_globals, seconds_elapsed) == 0x28 ? 1 : -1];
typedef char main_globals_connection_offset_assert[
	offsetof(struct _main_globals, connection) == 0x2C ? 1 : -1];
typedef char main_globals_movie_offset_assert[
	offsetof(struct _main_globals, movie) == 0x30 ? 1 : -1];
typedef char main_globals_defer_map_change_offset_assert[
	offsetof(struct _main_globals, defer_map_change) == 0x45 ? 1 : -1];
typedef char main_globals_reset_map_offset_assert[
	offsetof(struct _main_globals, reset_map) == 0x44 ? 1 : -1];
typedef char main_globals_revert_map_offset_assert[
	offsetof(struct _main_globals, revert_map) == 0x46 ? 1 : -1];
typedef char main_globals_skip_cinematic_offset_assert[
	offsetof(struct _main_globals, skip_cinematic) == 0x47 ? 1 : -1];
typedef char main_globals_saving_map_offset_assert[
	offsetof(struct _main_globals, saving_map) == 0x48 ? 1 : -1];
typedef char main_globals_save_map_safely_offset_assert[
	offsetof(struct _main_globals, save_map_safely) == 0x49 ? 1 : -1];
typedef char main_globals_won_map_offset_assert[
	offsetof(struct _main_globals, won_map) == 0x5A ? 1 : -1];
typedef char main_globals_halt_time_scale_offset_assert[
	offsetof(struct _main_globals, halt_time_scale) == 0x66 ? 1 : -1];
typedef char main_globals_respawn_timer_offset_assert[
	offsetof(struct _main_globals, respawn_timer) == 0x6E ? 1 : -1];
typedef char main_globals_loss_timer_offset_assert[
	offsetof(struct _main_globals, loss_timer) == 0x6C ? 1 : -1];
typedef char main_globals_allow_persistent_storage_offset_assert[
	offsetof(struct _main_globals, allow_persistent_storage) == 0x74 ? 1 : -1];
typedef char main_globals_soloplayer_map_name_offset_assert[
	offsetof(struct _main_globals, soloplayer_map_name) == 0x75 ? 1 : -1];
typedef char main_globals_multiplayer_map_name_offset_assert[
	offsetof(struct _main_globals, multiplayer_map_name) == 0x175 ? 1 : -1];
typedef char main_globals_core_name_offset_assert[
	offsetof(struct _main_globals, core_name) == 0x375 ? 1 : -1];
typedef char main_globals_d3d_flip_count_offset_assert[
	offsetof(struct _main_globals, d3d_flip_count) == 0x3F8 ? 1 : -1];
typedef char main_globals_vblank_flip_delta_index_offset_assert[
	offsetof(struct _main_globals, vblank_flip_delta_index) == 0x3FC ? 1 : -1];
typedef char main_globals_vblank_flip_deltas_offset_assert[
	offsetof(struct _main_globals, vblank_flip_deltas) == 0x3FE ? 1 : -1];

struct game_options
{
	unsigned long flags;
	short code_version;
	short difficulty;
	unsigned long random_seed;
	char map_name[256];
};

typedef char game_options_size_assert[
	sizeof(struct game_options) == 0x10C ? 1 : -1];

struct _main_window_storage
{
	struct render_window window;
	byte reservedAC[4];
};

/* ---------- prototypes */

extern void create_local_players(
	void);
extern void main_setup_connection(void);
extern void main_initialize_time(void);
extern void main_change_map_name(void);
extern void main_skip_private(void);
extern void main_update_time(void);
extern void main_save_map_private(void);
extern void main_game_render(
	double time_delta_since_tick_sec);
extern void main_frame_rate_debug(void);

extern void main_new_map(
	struct game_options *options);
extern void scripted_camera_set(
	word camera_point_index0,
	word camera_point_index1,
	long transition_time);
extern struct bitmap_data *bitmap_2d_new(
	short width,
	short height,
	short mipmap_count,
	short format);
extern char const *tiff_export(
	struct file_reference *file,
	struct bitmap_data *bitmap);
extern short global_screenshot_count;

/* ---------- globals */

short global_difficulty_level = 1;
short player_spawn_count = 1;
byte global_frame_rate_throttle = 1;
short global_screenshot_size = 1;

static char const *scenario_paths[10] =
{
	"levels\\a10\\a10",
	"levels\\a30\\a30",
	"levels\\a50\\a50",
	"levels\\b30\\b30",
	"levels\\b40\\b40",
	"levels\\c10\\c10",
	"levels\\c20\\c20",
	"levels\\c40\\c40",
	"levels\\d20\\d20",
	"levels\\d40\\d40"
};

static struct _main_window_storage window_storage = { 0 };
static struct _main_globals main_globals = { 0 };
boolean debug_force_frame_rate_update = FALSE;
boolean debug_no_drawing = FALSE;
boolean debug_game_save = FALSE;
boolean debug_frame_rate = FALSE;
boolean display_framerate = FALSE;
boolean display_vblank_deltas = FALSE;
boolean display_precache_progress = FALSE;

/* ---------- public code */

real main_get_seconds_elapsed(
	void)
{
	return main_globals.seconds_elapsed;
}

boolean gamepad_button_is_down(
	short button_index)
{
	short gamepad_index;
	boolean result = FALSE;

	match_assert("c:\\halo\\SOURCE\\main\\main.c", 245,
		button_index>=0 && button_index<NUMBER_OF_GAMEPAD_BUTTONS);
	gamepad_index = 0;

	do
	{
		if (input_has_gamepad(gamepad_index))
			break;

		gamepad_index++;
	}
	while (gamepad_index < MAXIMUM_GAMEPADS);

	if (gamepad_index < MAXIMUM_GAMEPADS)
	{
		struct gamepad_state const *gamepad = input_get_gamepad_state(gamepad_index);

		result = gamepad->buttons[button_index] > 0;
	}

	return result;
}

void game_connection_set(
	short connection)
{
	main_globals.connection = connection;
	return;
}

short game_connection(
	void)
{
	return main_globals.connection;
}

void main_disallow_persistent_storage(
	void)
{
	main_globals.allow_persistent_storage = FALSE;
	return;
}

void main_defer_map_map_change(
	void)
{
	main_globals.defer_map_change = FALSE;
	return;
}

char *main_get_map_name(
	void)
{
	return main_globals.soloplayer_map_name;
}

char *main_get_multiplayer_map_name(
	void)
{
	return main_globals.multiplayer_map_name;
}

short main_get_difficulty(
	void)
{
	return global_difficulty_level;
}

boolean main_saving_map(
	void)
{
	return main_globals.saving_map;
}

void main_save_cancel(
	void)
{
	main_globals.saving_map = FALSE;
	return;
}

void main_won_map(
	void)
{
	main_globals.saving_map = FALSE;
	main_globals.won_map = TRUE;
	return;
}

void main_lost_map(
	void)
{
	main_globals.saving_map = FALSE;
	main_globals.lost_map = TRUE;
	return;
}

void main_stop_time(
	void)
{
	main_globals.halt_time_scale = FALSE;
	main_globals.restart_time = FALSE;
	return;
}

void main_start_time(
	void)
{
	main_globals.restart_time = TRUE;
	return;
}

void main_reset_map(
	void)
{
	main_globals.switch_to_structure_bsp_index = NONE;
	main_globals.saving_map = FALSE;
	main_globals.reset_map = TRUE;
	main_globals.lost_map = FALSE;
	return;
}

void main_revert_map(
	void)
{
	main_globals.switch_to_structure_bsp_index = NONE;
	main_globals.saving_map = FALSE;
	main_globals.revert_map = TRUE;
	main_globals.lost_map = FALSE;
	return;
}

void main_skip_cinematic(
	void)
{
	main_globals.switch_to_structure_bsp_index = NONE;
	main_globals.saving_map = FALSE;
	main_globals.skip_cinematic = TRUE;
	return;
}

void main_save_map_nonsafe(
	void)
{
	main_globals.saving_map = TRUE;
	main_globals.save_map_safely = FALSE;
	return;
}

void main_respawn(
	boolean in_multiplayer)
{
	main_globals.respawn = TRUE;
	if (in_multiplayer)
		main_globals.respawn_timer = 91;
	return;
}

void main_save_core(
	void)
{
	main_globals.save_core = TRUE;
	csstrcpy(main_globals.core_name, "core.bin");
	return;
}

void main_save_core_name(
	char const *core_name)
{
	match_vwarn("c:\\halo\\SOURCE\\main\\main.c", 0x3a5, csstrlen(core_name) < 64, "warning, core file name will be truncated to 63 characters");
	csstrncpy(main_globals.core_name, core_name, 63);
	main_globals.save_core = TRUE;
	return;
}

void main_load_core(
	void)
{
	main_globals.load_core = TRUE;
	csstrcpy(main_globals.core_name, "core.bin");
	return;
}

void main_load_core_at_startup(
	void)
{
	main_globals.load_core_at_startup = TRUE;
	csstrcpy(main_globals.core_name, "core.bin");
	return;
}

void main_load_core_name(
	char const *core_name)
{
	match_vwarn("c:\\halo\\SOURCE\\main\\main.c", 0x3c9, csstrlen(core_name) < 64, "warning, core file name will be truncated to 63 characters");
	csstrncpy(main_globals.core_name, core_name, 63);
	main_globals.load_core = TRUE;
	return;
}

void main_load_core_name_at_startup(
	char const *core_name)
{
	match_vwarn("c:\\halo\\SOURCE\\main\\main.c", 0x3d7, csstrlen(core_name) < 64, "warning, core file name will be truncated to 63 characters");
	csstrncpy(main_globals.core_name, core_name, 63);
	main_globals.load_core_at_startup = TRUE;
	return;
}

void main_switch_structure_bsp(
	short structure_bsp_index)
{
	struct scenario *scenario = global_scenario_get();
	if (structure_bsp_index < 0 || structure_bsp_index >= scenario->structure_bsp_references.count)
	{
		console_warning("tried to switch to invalid structure-bsp %d", structure_bsp_index);
		return;
	}
	if (structure_bsp_index == global_structure_bsp_index)
	{
		console_warning("tried to switch to current structure-bsp %d", structure_bsp_index);
		return;
	}
	main_globals.switch_to_structure_bsp_index = structure_bsp_index;
	hud_load(TRUE);
	return;
}

void main_goto_main_menu(
	void)
{
	main_globals.switch_to_structure_bsp_index = NONE;
	main_globals.saving_map = FALSE;
	main_globals.want_to_be_at_main_menu = TRUE;
	return;
}

void main_queue_map_private(
	void)
{
	real progress;
	if (cache_files_precache_in_progress() && cache_files_precache_map_status(&progress) == 1)
		cache_files_precache_map_end();
	if (!cache_files_precache_in_progress())
	{
		cache_files_precache_map_begin(main_globals.queued_map_name, FALSE);
		main_globals.queue_map = FALSE;
	}
	return;
}

void main_menu_precache_resources(
	void)
{
	struct scenario *scenario = global_scenario_get();
	if (scenario)
	{
		match_assert("c:\\halo\\SOURCE\\main\\main.c", 0x46d, scenario->type==_scenario_type_main_menu);
		predicted_resources_precache(&scenario->predicted_ui_resources);
	}
	return;
}

void main_menu_unload(
	void)
{
	ui_stop_main_menu_music();
	main_menu_active(FALSE);
	main_globals.main_menu_scenario_loaded = FALSE;
	return;
}

extern void update_server_delete(
	void);
extern void update_server_start(
	void);

void main_menu_load(
	void)
{
	if (!main_globals.main_menu_scenario_loaded)
		main_load_ui_scenario(FALSE);
	main_screen_shell_load();
	main_menu_precache_resources();
	update_server_delete();
	update_server_new();
	update_server_start();
	game_time_dispose_from_old_map();
	game_time_initialize_for_new_map();
	game_time_start();
	hs_runtime_dispose_from_old_map();
	hs_runtime_initialize_for_new_map();
	main_globals.want_to_be_at_main_menu = FALSE;

	return;
}

static void screenshot_record(
	struct bitmap_data *screen,
	struct file_reference *file)
{
	char const *error_message = tiff_export(file, screen);

	if (error_message)
		error(_error_silent, error_message);
	return;
}

void main_present_frame(
	void)
{
	struct file_reference reference;
	char path[512];

	render_frame_present(NULL, main_globals.movie);
	if (global_screenshot_count <= 0 && main_globals.movie)
	{
		_snprintf(
			path,
			NUMBEROF(path),
			"movie\\frame%06d.tga",
			main_globals.recording_frame_index++);
		file_reference_create_from_path(&reference, path, FALSE);
		screenshot_record(main_globals.movie, &reference);
	}

	return;
}

void main_menu_ensure_player_queues_exist(
	void)
{
	update_server_delete();
	update_server_new();
	update_server_start();
	return;
}

boolean main_menu_fade_active(
	void)
{
	return main_globals.map_change_load_timer != 0;
}

void main_menu_switch_to_single_player(
	void)
{
	main_globals.defer_map_change = TRUE;
	return;
}

void main_set_game_connection_to_film_playback(
	void)
{
	main_globals.__unknown115 = TRUE;
	return;
}

extern short main_get_solo_level_from_name(
	char const *name);

short main_get_current_solo_level(
	void)
{
	return main_get_solo_level_from_name(main_globals.soloplayer_map_name);
}

char const *main_get_solo_level_name(
	short level)
{
	char const *result = NULL;

	if (level >= 0 && level < NUMBEROF(scenario_paths))
		result = scenario_paths[level];

	return result;
}

void main_run_demos(
	void)
{
	main_globals.run_xdemos = TRUE;
	return;
}

static void compute_subframe_counts(
	long num_players,
	long *horizontal_count,
	long *vertical_count)
{
	long horizontal = 1;
	long vertical = 1;

	match_assert(
		"c:\\halo\\SOURCE\\main\\main.c",
		1308,
		num_players>0);
	if (num_players>1)
	{
		do
		{
			if (horizontal<vertical)
				horizontal++;
			else
			{
				horizontal = 1;
				vertical++;
			}
		}
		while (vertical*horizontal<num_players);
	}

	*horizontal_count = horizontal;
	*vertical_count = vertical;
	return;
}

void compute_window_bounds(
	long player_index,
	long num_players,
	rectangle2d *pixel_bounds,
	rectangle2d *safe_frame_bounds)
{
	long vertical_count;
	long horizontal_count;
	long horizontal_index;
	long vertical_index;
	long subframe_width;
	long subframe_height;
	long safe_frame_inset;
	boolean first_player_spans_two_columns;

	match_assert(
		"c:\\halo\\SOURCE\\main\\main.c",
		1359,
		player_index<num_players);
	safe_frame_inset = num_players>1 ? 4 : 0;
	first_player_spans_two_columns = FALSE;
	compute_subframe_counts(
		num_players,
		&horizontal_count,
		&vertical_count);

	if (horizontal_count*vertical_count>num_players)
	{
		if (!player_index)
			first_player_spans_two_columns = TRUE;
		else
			player_index++;
	}

	vertical_index = player_index/horizontal_count;
	horizontal_index = player_index-vertical_index*horizontal_count;
	match_assert(
		"c:\\halo\\SOURCE\\main\\main.c",
		1390,
		vertical_index>=0 && vertical_index<vertical_count);
	match_assert(
		"c:\\halo\\SOURCE\\main\\main.c",
		1391,
		horizontal_index>=0 && horizontal_index<horizontal_count);

	subframe_height =
		(rasterizer_globals.reserved04.frame_bounds.y1-
			rasterizer_globals.reserved04.frame_bounds.y0)/vertical_count;
	subframe_width =
		(rasterizer_globals.reserved04.frame_bounds.x1-
			rasterizer_globals.reserved04.frame_bounds.x0)/horizontal_count;
	subframe_width *= first_player_spans_two_columns ? 2 : 1;

	safe_frame_bounds->x0 =
		subframe_width*horizontal_index+
		rasterizer_globals.reserved04.frame_bounds.x0;
	safe_frame_bounds->x1 =
		subframe_width*(horizontal_index+1)+
		rasterizer_globals.reserved04.frame_bounds.x0;
	safe_frame_bounds->y0 =
		subframe_height*vertical_index+
		rasterizer_globals.reserved04.frame_bounds.y0;
	safe_frame_bounds->y1 =
		subframe_height*(vertical_index+1)+
		rasterizer_globals.reserved04.frame_bounds.y0;
	*pixel_bounds = *safe_frame_bounds;

	safe_frame_bounds->x0 += horizontal_index*safe_frame_inset;
	safe_frame_bounds->x1 -= (horizontal_index==0)*safe_frame_inset;
	safe_frame_bounds->y0 += vertical_index*safe_frame_inset;
	safe_frame_bounds->y1 -= (vertical_index==0)*safe_frame_inset;

	if (!horizontal_index)
		pixel_bounds->x0 = rasterizer_globals.reserved04.screen_bounds.x0;
	if ((first_player_spans_two_columns!=FALSE)+horizontal_index+1==horizontal_count)
		pixel_bounds->x1 = rasterizer_globals.reserved04.screen_bounds.x1;
	if (!vertical_index)
		pixel_bounds->y0 = rasterizer_globals.reserved04.screen_bounds.y0;
	if (vertical_index+1==vertical_count)
		pixel_bounds->y1 = rasterizer_globals.reserved04.screen_bounds.y1;

	return;
}

void main_set_multiplayer_map_name(
	char const *map_name)
{
	csstrncpy(main_globals.multiplayer_map_name, map_name, NUMBEROF(main_globals.multiplayer_map_name) - 1);
	main_globals.multiplayer_map_name[NUMBEROF(main_globals.multiplayer_map_name) - 1] = 0;
	cache_files_give_time_to_precache(main_globals.multiplayer_map_name);
	return;
}

void main_set_difficulty(
	short difficulty)
{
	if (difficulty >= _game_difficulty_level_easy && difficulty < NUMBER_OF_GAME_DIFFICULTY_LEVELS)
		global_difficulty_level = difficulty;
	return;
}

long sort_controllers_ascending(
	short const *a,
	short const *b)
{
	short value_a = *a;
	short value_b = *b;
	if (value_a == NONE && value_b != value_a)
		return 1;
	if (value_a != NONE && value_b == NONE)
		return -1;
	if (value_a > value_b)
		return 1;
	return (value_a >= value_b) - 1;
}

void main_save_map_safe(
	void)
{
	if (!main_globals.saving_map || main_globals.save_map_timeout)
	{
		main_globals.saving_map = TRUE;
		main_globals.save_map_safely = TRUE;
		main_globals.save_map_timeout = TRUE;
		main_globals.ticks_until_next_save_check = 0;
		main_globals.ticks_unable_to_save = 0;
		main_globals.safe_intervals = 0;
	}
	return;
}

void main_save_map_no_timeout(
	void)
{
	if (!main_globals.saving_map || main_globals.save_map_timeout)
	{
		main_globals.saving_map = TRUE;
		main_globals.save_map_safely = TRUE;
		main_globals.ticks_until_next_save_check = 0;
		main_globals.ticks_unable_to_save = 0;
		main_globals.safe_intervals = 0;
	}
	main_globals.save_map_timeout = FALSE;
	return;
}

void main_skip(
	short ticks)
{
	if (ticks <= 15)
	{
		main_globals.skip_ticks = ticks;
		main_globals.skip = TRUE;
	}
	else
	{
		error(_error_silent, "cannot skip more than 15 frames (half a second)");
	}
	return;
}

void main_queue_map_name(
	char const *map_name)
{
	if (map_name)
	{
		csstrncpy(main_globals.queued_map_name, map_name, NUMBEROF(main_globals.queued_map_name) - 1);
		main_globals.queue_map = TRUE;
	}
	else
	{
		main_globals.queued_map_name[0] = 0;
		main_globals.queue_map = FALSE;
	}
	return;
}

boolean main_taking_screenshot(
	void)
{
	return global_screenshot_count > 0 || main_globals.movie != NULL;
}

void main_movie_start(
	real frames_per_second)
{
	match_assert("c:\\halo\\SOURCE\\main\\main.c", 2715, main_globals.movie==NULL);

	main_globals.movie = bitmap_2d_new(640, 480, 0, 10);
	if (main_globals.movie)
	{
		directory_create_or_delete_contents("movie");
		main_globals.recording_frame_index = 0;
		if (frames_per_second > 0.0001f)
			main_globals.recording_dt = 1.0f / frames_per_second;
		else
			main_globals.recording_dt = 1.0f / 30.0f;
		game_time_set_speed(1.0f);
	}

	return;
}

void main_movie_stop(
	void)
{
	if (main_globals.movie)
	{
		bitmap_delete(main_globals.movie);
		main_globals.movie = NULL;
	}
	return;
}

void main_print_version(
	void)
{
	console_printf(FALSE, "halobeta xbox 01.01.14.2342 Jan 14 2002 12:49:20");
	return;
}

void main_save_current_solo_map(
	char const *map_name)
{
	FILE *file;

	if ((short)main_get_solo_level_from_name(map_name) != NONE)
	{
		file = fopen("z:\\last_solo.txt", "w");
		if (file)
		{
			fwrite(map_name, 1, csstrlen(map_name) + 1, file);
			fclose(file);
		}
		else
		{
			error(_error_silent, "Couldn't create a file to write the current solo map to");
		}
	}

	return;
}

void main_load_ui_scenario(
	boolean precache_resources)
{
	struct game_options options;

	game_precache_new_map("levels\\ui\\ui", TRUE);
	match_assert("c:\\halo\\SOURCE\\main\\main.c", 1092,
		!main_globals.main_menu_scenario_loaded);
	game_options_new(&options);
	csstrncpy(options.map_name, "levels\\ui\\ui", NUMBEROF(options.map_name) - 1);
	options.map_name[NUMBEROF(options.map_name) - 1] = 0;
	game_precache_new_map(options.map_name, TRUE);
	game_dispose_from_old_map();
	game_unload();
	game_engine_dispose();
	game_set_game_variant(NULL);
	main_globals.main_menu_scenario_loaded = TRUE;
	main_new_map(&options);
	director_script_camera(TRUE);
	scripted_camera_set(0, 0, NONE);
	main_menu_active(TRUE);
	main_globals.load_last_solo_level = TRUE;
	if (precache_resources)
		main_menu_precache_resources();

	return;
}

extern void game_end_credits_start(
	void);

void main_roll_credits(
	void)
{
	error(_error_silent, "congratulations, you won the game!");
	main_menu_load();
	game_end_credits_start();
	return;
}

void main_revert_map_private(
	void)
{
	game_state_revert();
	ui_widgets_disable_pause_game(30);
	main_globals.revert_map = FALSE;
	return;
}

void main_skip_cinematic_private(
	void)
{
	if (cinematic_can_be_skipped())
	{
		game_state_revert();
		ui_widgets_disable_pause_game(30);
		main_globals.revert_map = FALSE;
	}
	main_globals.skip_cinematic = FALSE;
	return;
}

void main_saving_map_private(
	void)
{
	game_state_save();
	hud_autosave(FALSE);
	main_globals.save_map_completed = FALSE;
	return;
}

void main_switch_to_structure_bsp_private(
	void)
{
	scenario_switch_structure_bsp(main_globals.switch_to_structure_bsp_index);
	main_globals.switch_to_structure_bsp_index = NONE;
	hud_load(0);
	return;
}

void main_lost_map_private(
	void)
{
	if (!game_time_get_paused())
	{
		if (main_globals.loss_timer++ > 90)
		{
			main_globals.lost_map = FALSE;
			main_globals.loss_timer = 0;
			game_state_revert();
		}
	}
	return;
}

void main_respawn_private(
	void)
{
	if (!game_time_get_paused() && !cinematic_in_progress())
	{
		if (main_globals.respawn_timer++ > 90 && players_respawn_coop())
		{
			main_globals.respawn = FALSE;
			main_globals.respawn_timer = 0;
		}
	}
	return;
}

void main_load_core_private(
	void)
{
	game_state_load_core(main_globals.core_name);
	main_globals.load_core = FALSE;
	return;
}

void main_save_core_private(
	void)
{
	game_state_save_core(main_globals.core_name);
	main_globals.save_core = FALSE;
	return;
}

void main_run_demos_private(
	void)
{
	main_globals.run_xdemos = FALSE;
	xbox_demos_launch();
	return;
}

void main_won_map_private(
	void)
{
	short level;
	short local_player_index;
	main_globals.want_to_be_at_main_menu = TRUE;
	main_globals.won_map = FALSE;
	level = main_get_solo_level_from_name(main_globals.soloplayer_map_name) + 1;
	if (level >= 10)
		level = NONE;
	for (local_player_index = 0; local_player_index < player_spawn_count; local_player_index++)
		player_profile_save_level_completed(local_player_index);
	ui_set_next_level(level);
	return;
}

void main_set_map_name(
	char const *map_name)
{
	main_globals.want_to_be_at_main_menu = FALSE;
	csstrncpy(main_globals.soloplayer_map_name, map_name, NUMBEROF(main_globals.soloplayer_map_name) - 1);
	main_globals.soloplayer_map_name[NUMBEROF(main_globals.soloplayer_map_name) - 1] = 0;
	main_globals.allow_persistent_storage = TRUE;
	if ((game_in_editor() || game_in_progress()) && main_globals.connection == _game_connection_local)
		main_globals.defer_map_change = TRUE;
	return;
}

void main_exit(
	void)
{
	switch (main_globals.connection)
	{
	case _game_connection_network_server:
		dispose_global_network_game_client();
		dispose_global_network_game_server();
		game_dispose_from_old_map();
		game_dispose();
		debug_keys_dispose();
		console_dispose();
		break;
	case _game_connection_network_client:
		dispose_global_network_game_client();
	default:
		game_dispose_from_old_map();
		game_dispose();
		debug_keys_dispose();
		console_dispose();
		break;
	}
	return;
}

void main_reset_time(
	void)
{
	unsigned long frame_index;
	unsigned long vertical_blank_index;
	main_globals.frame_start_milliseconds = system_milliseconds();
	frame_index = rasterizer_globals.frame_index;
	vertical_blank_index = rasterizer_globals.vertical_blank_index;
	main_globals.rasterizer_frame_index = frame_index;
	main_globals.rasterizer_vertical_blank_index = vertical_blank_index;
	return;
}

boolean code_000f0be0(
	void)
{
	return rasterizer_globals.framerate_throttle;
}

void main_initialize_time(
	void)
{
	main_globals.frame_start_milliseconds = system_milliseconds();
	main_globals.rasterizer_frame_index = 0;
	main_globals.rasterizer_vertical_blank_index = 0;
	rasterizer_set_vblank_callback(main_vertical_blank_interrupt_handler);
	main_globals.vblank_flip_delta_index = 0;
	csmemset(main_globals.vblank_flip_deltas, 0, sizeof(main_globals.vblank_flip_deltas));
	main_globals.d3d_flip_count = d3d_find_flipcount();
	return;
}

static void main_reset_map_private(
	void)
{
	if (!game_time_get_paused())
	{
		scenario_switch_structure_bsp(0);
		game_dispose_from_old_map();
		input_flush();
		game_initialize_for_new_map();
		create_local_players();
		game_time_start();
		game_initial_pulse();
		ui_widgets_disable_pause_game(30);

		main_globals.reset_map = FALSE;
	}

	return;
}

void main_loop_of_death(
	void)
{
	while (TRUE)
	{
		input_frame_begin();
		input_update();
		shell_idle();
		event_manager_update();
		telnet_console_process();
		process_ui_widgets();
		main_pregame_render();
		main_rasterizer_throttle();
		main_present_frame();
		input_frame_end();
	}

	return;
}

void main_loop(
	void)
{
	boolean render_frame;
	real precache_progress;
	long connection;

	if (!game_in_editor())
	{
		csstrncpy(main_globals.soloplayer_map_name, "levels\\b30\\b30", NUMBEROF(main_globals.soloplayer_map_name)-1);
		main_globals.soloplayer_map_name[NUMBEROF(main_globals.soloplayer_map_name)-1] = '\0';
	}

	main_globals.want_to_be_at_main_menu = !game_in_editor();
	main_globals.switch_to_structure_bsp_index = NONE;
	main_globals.halt_time_scale = TRUE;

	console_initialize();
	debug_keys_initialize();
	game_initialize();
	console_startup();
	main_setup_connection();
	main_initialize_time();

	while (TRUE)
	{
		if (!game_in_editor())
		{
			if (main_globals.switch_to_structure_bsp_index!=NONE)
			{
				scenario_switch_structure_bsp(main_globals.switch_to_structure_bsp_index);
				main_globals.switch_to_structure_bsp_index = NONE;
				hud_load(FALSE);
			}

			if (main_globals.lost_map)
			{
				if (!game_time_get_paused() && main_globals.loss_timer++>90)
				{
					main_globals.lost_map = FALSE;
					main_globals.loss_timer = 0;
					game_state_revert();
				}
			}

			if (main_globals.won_map)
			{
				main_won_map_private();
			}

			if (main_globals.respawn)
			{
				if (!game_time_get_paused() && !cinematic_in_progress() && main_globals.respawn_timer++>90 && players_respawn_coop())
				{
					main_globals.respawn = FALSE;
					main_globals.respawn_timer = 0;
				}
			}

			if (main_globals.save_map_completed)
			{
				game_state_save();
				hud_autosave(FALSE);
				main_globals.save_map_completed = FALSE;
			}

			if (main_globals.defer_map_change)
			{
				main_change_map_name();
			}

			if (main_globals.revert_map)
			{
				game_state_revert();
				ui_widgets_disable_pause_game(30);
				main_globals.revert_map = FALSE;
			}

			if (main_globals.skip_cinematic)
			{
				if (cinematic_can_be_skipped())
				{
					game_state_revert();
					ui_widgets_disable_pause_game(30);
					main_globals.revert_map = FALSE;
				}

				main_globals.skip_cinematic = FALSE;
			}

			if (main_globals.reset_map)
			{
				main_reset_map_private();
			}

			if (main_globals.save_core)
			{
				game_state_save_core(main_globals.core_name);
				main_globals.save_core = FALSE;
			}

			if (main_globals.load_core)
			{
				game_state_load_core(main_globals.core_name);
				main_globals.load_core = FALSE;
			}

			if (main_globals.want_to_be_at_main_menu)
			{
				main_menu_load();
			}

			if (main_globals.load_last_solo_level)
			{
				main_load_last_solo_map();
			}

			if (main_globals.run_xdemos)
			{
				main_globals.run_xdemos = FALSE;
				xbox_demos_launch();
			}

			if (main_globals.skip)
			{
				main_skip_private();
			}

			if (main_globals.queue_map)
			{
				if (cache_files_precache_in_progress() && cache_files_precache_map_status(&precache_progress)==1)
				{
					cache_files_precache_map_end();
				}

				if (!cache_files_precache_in_progress())
				{
					cache_files_precache_map_begin(main_globals.queued_map_name, FALSE);
					main_globals.queue_map = FALSE;
				}
			}
		}
		else if (main_globals.reset_map)
		{
			main_reset_map_private();
		}

		profile_frame_start();
		input_frame_begin();
		input_update();
		input_abstraction_update();
		shell_idle();
		event_manager_update();
		telnet_console_process();

		if (!shell_application_is_paused())
		{
			render_frame = TRUE;

			connection = main_globals.connection;
			if (connection==_game_connection_network_client)
			{
				if (!network_game_client_start_frame())
				{
					display_error_when_main_menu_loaded(6);
					error(_error_silent, "the game host went down");
					network_game_abort();
				}
			}
			else if (connection==_game_connection_network_server)
			{
				if (!network_game_client_start_frame())
				{
					display_error_when_main_menu_loaded(1);
					error(_error_silent, "the game host went down");
					network_game_abort();
				}
				else if (!network_game_server_start_frame())
				{
					display_error_when_main_menu_loaded(1);
					error(_error_silent, "the game host went down");
					network_game_abort();
				}
			}
			else if (connection==_game_connection_film_playback)
			{
				break;
			}

			main_update_time();
			process_ui_widgets();
			bink_playback_update();

			if ((!game_in_editor() && (input_key_is_down(_key_end) || input_key_is_down(_key_escape))) || editor_should_exit())
			{
				main_movie_stop();

				if (!game_engine_running())
				{
					main_reset_map();
				}
			}

			if (game_in_progress())
			{
				terminal_update();

				if (!console_update() || main_globals.connection!=_game_connection_local)
				{
					debug_keys_update();
					cheats_update();
					player_control_update((real)main_globals.halt_time_scale*main_globals.seconds_elapsed);

					connection = main_globals.connection;
					if (connection>_game_connection_local && connection<=_game_connection_network_server && !network_game_client_end_frame())
					{
						display_error_when_main_menu_loaded(1);
						network_game_abort();
					}

					game_time_update((real)main_globals.halt_time_scale*main_globals.seconds_elapsed);

					render_frame = main_globals.main_menu_scenario_loaded ||
						(main_globals.halt_time_scale &&
							(game_time_get_paused() || game_time_get_elapsed()>0 || game_time_get_speed()<1.0f));
					render_frame &= !game_engine_running() || game_time_get()>=3;

					collision_log_continue_period(1);
					director_update((real)main_globals.halt_time_scale*main_globals.seconds_elapsed);
					observer_update((real)main_globals.halt_time_scale*main_globals.seconds_elapsed);
					collision_log_end_period();
					game_engine_update_non_deterministic((real)main_globals.halt_time_scale*main_globals.seconds_elapsed);
				}

				if (main_globals.saving_map)
				{
					main_save_map_private();
				}

				if (render_frame && !debug_no_drawing)
				{
					profile_render_start();
					main_game_render((double)main_globals.seconds_elapsed);
					profile_render_end();
				}
			}
			else
			{
				profile_render_start();
				main_pregame_render();
				profile_render_end();
			}

			main_rasterizer_throttle();

			if (render_frame && !debug_no_drawing)
			{
				main_present_frame();
			}
		}

		input_frame_end();
		profile_frame_end();
		main_frame_rate_debug();

		if (main_globals.restart_time)
		{
			main_globals.restart_time = FALSE;
			main_reset_time();
			main_globals.halt_time_scale = TRUE;
		}
	}

	error(_error_silent, "end of saved film");

	switch (main_globals.connection)
	{
	case _game_connection_network_server:
		dispose_global_network_game_client();
		dispose_global_network_game_server();
		break;
	case _game_connection_network_client:
		dispose_global_network_game_client();
		break;
	}

	game_dispose_from_old_map();
	game_dispose();
	debug_keys_dispose();
	console_dispose();

	return;
}

/* ---------- private code */
