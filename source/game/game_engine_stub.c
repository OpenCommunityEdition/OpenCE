/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

enum
{
	_game_engine_type_stub = 7,
	NUMBER_OF_STUB_GAME_ENGINE_CALLBACKS = 32,
};

/* ---------- macros */

/* ---------- structures */

typedef void (*stub_game_engine_callback)(void);

struct stub_game_engine
{
	char const *name;
	long type;
	stub_game_engine_callback callbacks[NUMBER_OF_STUB_GAME_ENGINE_CALLBACKS];
};

typedef char verify_stub_game_engine_size[sizeof(struct stub_game_engine) == 0x88 ? 1 : -1];

/* ---------- prototypes */

static void stub_engine_dispose(void);
static boolean stub_engine_initialize_for_new_map(void);
static void stub_engine_dispose_from_old_map(void);
static void stub_engine_player_added(void);
static void stub_engine_game_ending(void);
static void stub_engine_game_starting(void);
static void stub_engine_statistics_append(void);
static void stub_engine_handle_client_message(void);
static void stub_engine_handle_server_message(void);
static void stub_engine_pregame_post_rasterize(void);
static void stub_engine_post_rasterize(void);
static void stub_engine_update(void);
static boolean stub_engine_allow_pick_up(void);
static void stub_engine_player_damaged_player(void);
static void stub_engine_player_killed_player(void);

/* ---------- globals */

struct stub_game_engine stub_engine =
{
	"stub",
	_game_engine_type_stub,
	{
		stub_engine_dispose,
		(stub_game_engine_callback) stub_engine_initialize_for_new_map,
		stub_engine_dispose_from_old_map,
		stub_engine_player_added,
		stub_engine_game_ending,
		stub_engine_game_starting,
		stub_engine_statistics_append,
		stub_engine_handle_client_message,
		stub_engine_handle_server_message,
		stub_engine_pregame_post_rasterize,
		stub_engine_post_rasterize,
		NULL,
		NULL,
		NULL,
		NULL,
		stub_engine_update,
		NULL,
		NULL,
		NULL,
		NULL,
		(stub_game_engine_callback) stub_engine_allow_pick_up,
		stub_engine_player_damaged_player,
		stub_engine_player_killed_player,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	},
};

/* ---------- public code */

static void stub_engine_dispose(void)
{
}

static boolean stub_engine_initialize_for_new_map(void)
{
	return TRUE;
}

static void stub_engine_dispose_from_old_map(void)
{
}

static void stub_engine_player_added(void)
{
}

static void stub_engine_game_ending(void)
{
}

static void stub_engine_game_starting(void)
{
}

static void stub_engine_statistics_append(void)
{
}

static void stub_engine_handle_client_message(void)
{
}

static void stub_engine_handle_server_message(void)
{
}

static void stub_engine_pregame_post_rasterize(void)
{
}

static void stub_engine_post_rasterize(void)
{
}

static void stub_engine_update(void)
{
}

static boolean stub_engine_allow_pick_up(void)
{
	return TRUE;
}

static void stub_engine_player_damaged_player(void)
{
}

static void stub_engine_player_killed_player(void)
{
}

/* ---------- private code */
