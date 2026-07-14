/*
GAME_ENGINE_MULTIPLAYER_SOUNDS.C

symbols in this file:
000A1460 0060:
	_code_000a1460 (0000)
000A14C0 0020:
	_code_000a14c0 (0000)
000A14E0 0050:
	_game_engine_update_multiplayer_sound (0000)
000A1530 0080:
	_code_000a1530 (0000)
000A15B0 0050:
	_game_engine_play_multiplayer_sound (0000)
000A1600 0030:
	_game_engine_intialize_queued_sounds (0000)
002DE530 002b:
	_data_002de530 (0000)
0043EB78 002c:
	_bss_0043eb78 (0000)
*/

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

enum multiplayer_information_sound
{
	_multiplayer_sound_play_ball,
	_multiplayer_sound_game_over,
	_multiplayer_sound_one_minute_to_win,
	_multiplayer_sound_30_seconds_to_win,
	_multiplayer_sound_red_team_minute_to_win,
	_multiplayer_sound_red_team_30_seconds_to_win,
	_multiplayer_sound_blue_team_minute_to_win,
	_multiplayer_sound_blue_team_30_seconds_to_win,
	_multiplayer_sound_blue_team_has_the_flag,
	_multiplayer_sound_blue_team_flag_returned,
	_multiplayer_sound_blue_team_score,
	_multiplayer_sound_red_team_has_the_flag,
	_multiplayer_sound_red_team_flag_returned,
	_multiplayer_sound_red_team_score,
	_multiplayer_sound_double_kill,
	_multiplayer_sound_triple_kill,
	_multiplayer_sound_killtacular,
	_multiplayer_sound_running_riot,
	_multiplayer_sound_killing_spree,
	_multiplayer_sound_oddball,
	_multiplayer_sound_race,
	_multiplayer_sound_slayer,
	_multiplayer_sound_capture_the_flag,
	_multiplayer_sound_warthog,
	_multiplayer_sound_ghost,
	_multiplayer_sound_scorpion,
	_multiplayer_sound_countdown_timer,
	_multiplayer_sound_teleporter_activate,
	_multiplayer_sound_flag_failure,
	_multiplayer_sound_countdown_for_respawn,
	_multiplayer_sound_hill_move,
	_multiplayer_sound_player_respawn,
	_multiplayer_sound_team_king_of_the_hill,
	_multiplayer_sound_team_oddball,
	_multiplayer_sound_team_race,
	_multiplayer_sound_team_slayer,
	_multiplayer_sound_king_of_the_hill,
	_multiplayer_sound_blue_team_ctf,
	_multiplayer_sound_red_team_ctf,
	_multiplayer_sound_hill_contested,
	_multiplayer_sound_hill_controlled,
	_multiplayer_sound_hill_occupied,
	_multiplayer_sound_countdown_timer_end,
	_multiplayer_sound_ting,
	NUMBER_OF_MULTIPLAYER_INFORMATION_SOUNDS
};

enum
{
	MAXIMUM_QUEUED_MULTIPLAYER_SOUNDS = 5,
	MULTIPLAYER_SOUND_QUEUE_INITIAL_DELAY_TICKS = 2 * TICKS_PER_SECOND,
};

/* ---------- macros */

/* ---------- structures */

struct queued_multiplayer_sound
{
	long sound_index;
	long delay_ticks;
};

struct multiplayer_sound_queue
{
	long count;
	struct queued_multiplayer_sound sounds[MAXIMUM_QUEUED_MULTIPLAYER_SOUNDS];
};

typedef char verify_multiplayer_sound_queue_size[sizeof(struct multiplayer_sound_queue) == 0x2C ? 1 : -1];

/* ---------- prototypes */

void game_engine_intialize_queued_sounds(void);

/* ---------- globals */

boolean data_002de530[_multiplayer_sound_ting] =
{
	TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
	TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
	TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
	TRUE, TRUE,
	FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
	TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
	TRUE, TRUE, FALSE,
};

static struct multiplayer_sound_queue bss_0043eb78;
#define multiplayer_sound_queue bss_0043eb78

/* ---------- public code */

void game_engine_intialize_queued_sounds(void)
{
	csmemset(multiplayer_sound_queue.sounds, 0, sizeof(multiplayer_sound_queue.sounds));
	multiplayer_sound_queue.count = 1;
	multiplayer_sound_queue.sounds[0].sound_index = NONE;
	multiplayer_sound_queue.sounds[0].delay_ticks = MULTIPLAYER_SOUND_QUEUE_INITIAL_DELAY_TICKS;
}

/* ---------- private code */
