/* Real game_time_update, client-role predicate and round reset, with network
   clocks/queues as recorders. No assets, sockets, graphics or saved profile. */
#include "harness.h"
#include "config.inc"
#define MIN(a,b) ((a)<(b)?(a):(b))
#define csmemset memset
/* Only nonnegative frame deltas are used, so truncation is floor. */
#define floor(value) ((double)(long)(value))
static struct {
	boolean initialized, active, paused;
	long local_time, server_time;
	short last_local_time_elapsed;
	real speed, leftover_dt;
} clock_state = { TRUE, TRUE, FALSE, 0, 0, 0, 1.0f, 0.0f }, *game_time_globals = &clock_state;
struct network_game_client { boolean started; };
struct network_game { struct { boolean game_objects_loaded; long payload; } local_data; };
static struct network_game_client client;
static struct network_game game;
static boolean shell, client_exists = TRUE;
static short connection = _game_connection_local;
static long ticks, frames, local_updates, host_updates, distributed_ticks, queue_time, queue_reads;
static long shell_loads;
static boolean main_menu_is_active(void) { return shell; }
static short game_connection(void) { return connection; }
static void game_connection_set(short mode) { connection = mode; }
static struct network_game_client *global_network_game_client_get(void) { return client_exists ? &client : NULL; }
static void *global_network_game_server_get(void) { return connection == _game_connection_network_server ? &game : NULL; }
static boolean network_game_client_server_has_started_game(struct network_game_client *c) { return c->started; }
static void main_load_ui_scenario(boolean precache) { shell_loads++; shell = TRUE; clock_state.active = TRUE; }
static void update_client_local_ticks(short count) { local_updates += count; queue_time += count; }
static void network_game_server_update_ticks(void *server, short count) { host_updates += count; queue_time += count; }
static long update_client_get_maximum_possible_server_time(void) { queue_reads++; return queue_time; }
static void game_tick(void) { ticks++; }
static void render_interpolation_tick(void) { }
static void render_interpolation_frame_begin(void) { }
static void render_interpolation_frame_end(void) { }
static void network_distributed_tick(void) { distributed_ticks++; }
static void game_frame(real dt) { frames++; }

#include "under_test.inc"

static void advance(void)
{
	long frame;
	for (frame = 0; frame < 480; frame++) game_time_update(1.0f/240.0f);
}

static void check_shell(void)
{
	CHECK(clock_state.active && ticks >= 59 && ticks <= 60, "shell froze or changed speed: %ld ticks", ticks);
	CHECK(clock_state.local_time == ticks && clock_state.server_time == ticks, "scene clocks disagree");
	CHECK(frames == 480, "scene missed render frames");
	CHECK(!local_updates && !host_updates && !distributed_ticks && !queue_reads, "shell touched gameplay network queues");
	CHECK(!game_time_held() && !network_game_distributed_client(), "shell treated as another host's simulation");
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	CASE("shell-local") { shell = TRUE; advance(); check_shell(); return 0; }
	CASE("shell-browser") { shell = TRUE; connection = _game_connection_network_client; advance(); check_shell(); CHECK(connection == _game_connection_network_client, "browser lost its network role"); return 0; }
	CASE("shell-host") { shell = TRUE; connection = _game_connection_network_server; advance(); check_shell(); CHECK(connection == _game_connection_network_server, "host lost its network role"); return 0; }
	CASE("shell-joined") { shell = TRUE; connection = _game_connection_network_client; client.started = TRUE; advance(); check_shell(); return 0; }
	CASE("shell-round-reset")
	{
		shell = TRUE; connection = _game_connection_network_server; game.local_data.payload = 42;
		network_game_reset_for_next_round(&game, FALSE);
		CHECK(!game.local_data.payload && !shell_loads, "reset did not clear round state normally");
		advance(); check_shell(); return 0;
	}
	CASE("return-to-shell")
	{
		connection = _game_connection_network_client; game.local_data.game_objects_loaded = TRUE;
		network_game_reset_for_next_round(&game, TRUE);
		CHECK(shell_loads == 1 && !game.local_data.game_objects_loaded && connection == _game_connection_network_client, "round did not return to its network lobby");
		advance(); check_shell(); return 0;
	}
	CASE("game-round-reset")
	{
		network_game_reset_for_next_round(&game, FALSE);
		CHECK(!clock_state.active, "round reset no longer stops gameplay"); return 0;
	}
	CASE("game-client-wait")
	{
		connection = _game_connection_network_client; advance();
		CHECK(game_time_held() && network_game_distributed_client(), "real client no longer waits for host");
		CHECK(!ticks && !clock_state.last_local_time_elapsed && frames == 480, "client ran ahead before host started"); return 0;
	}
	CASE("game-client-ready")
	{
		connection = _game_connection_network_client; client.started = TRUE; advance();
		CHECK(ticks >= 59 && ticks <= 60 && distributed_ticks == ticks && !local_updates && !host_updates, "started client clock changed"); return 0;
	}
	CASE("game-local")
	{
		advance(); CHECK(ticks >= 59 && ticks <= 60 && local_updates == ticks && queue_reads > 0, "local gameplay queues changed"); return 0;
	}
	CASE("game-host")
	{
		connection = _game_connection_network_server; advance();
		CHECK(ticks >= 59 && ticks <= 60 && host_updates == ticks && distributed_ticks == ticks, "host/co-op clock changed"); return 0;
	}
	CASE("game-paused")
	{
		clock_state.active = FALSE; clock_state.paused = TRUE; advance();
		CHECK(!ticks && !frames && !distributed_ticks && !local_updates && !host_updates, "paused gameplay advanced"); return 0;
	}
	CHECK(FALSE, "unknown case %s", case_name);
}
