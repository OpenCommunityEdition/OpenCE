/* Compile the real production state machine against a deterministic engine
boundary. These tests exercise lifecycle, deadlines, cancellation and player
confirmation; multiplayer transport is covered by the separate relay tests. */
#define HALO_WEB 1
#define HALO_LINUX 1
#define HALO_QUICK_PLAY_TEST 1
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../port/linux/game/quick_play.c"

static struct network_game_client mock_client;
static struct network_game_server mock_server;
static struct network_game_client *client;
static struct network_game_server *server;
static unsigned long clock_ms, last_target;
static const char *mode, *target;
static short connection, join_result;
static int cancel, created, aborts, menus, add_requests, starts, searches, reports, maps;
static int quick_policy, policy_requests, countdown;
static char last_phase[32];

unsigned long system_milliseconds(void) { return clock_ms; }
const char *config_string(const char *name) { return !strcmp(name, "network.quick_play") ? mode : target; }
int web_quick_play_take_cancel(void) { int result = cancel; cancel = 0; return result; }
void web_quick_play_report(const char *phase, const char *message)
{ assert(message && *message); reports++; snprintf(last_phase, sizeof(last_phase), "%s", phase); }
struct network_game_client *global_network_game_client_get(void) { return client; }
struct network_game_server *global_network_game_server_get(void) { return server; }
void dispose_global_network_game_client(void) { client = NULL; }
void dispose_global_network_game_server(void) { server = NULL; }
boolean create_global_network_game_client(void) { created++; client = &mock_client; return TRUE; }
void player_ui_fast_setup_network_server(void)
{ created++; client = &mock_client; server = &mock_server; connection = _game_connection_network_server; }
void main_goto_main_menu(void) { menus++; }
void game_connection_set(short value) { connection = value; }
short game_connection(void) { return connection; }
void network_game_abort(void) { aborts++; }
void network_game_client_request_immediate_start(void) { starts++; }
void network_game_server_change_map_name(struct network_game_server *value, const char *path)
{ assert(value == server); assert(!strcmp(path, "levels\\test\\bloodgulch\\bloodgulch")); maps++; }
void network_game_server_change_game_variant(struct network_game_server *value, struct game_variant *variant)
{ assert(value == server && variant); }
boolean network_game_server_enable_quick_play(struct network_game_server *value)
{ assert(value == server); policy_requests++; return quick_policy; }
void network_game_server_pause_countdown(struct network_game_server *value, boolean paused)
{ assert(value == server && !paused); }
struct game_variant *game_engine_get_variant_by_name(struct game_variant *variant, const char *name)
{ assert(!strcmp(name, "slayer")); variant->unused = 0; return variant; }
void player_ui_set_game_variant(struct game_variant *variant) { assert(variant); }
void ui_widgets_close_all(void) {}
void *ui_widget_load_by_name_or_tag(const char *a, int b, void *c, int d, int e, int f, int g)
{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; return &mock_client; }
short network_game_client_get_state(struct network_game_client *value, short *data)
{ (void)data; return value->state; }
short network_game_client_get_error(struct network_game_client *value) { return value->error; }
short network_game_client_get_seconds_to_game_start(struct network_game_client *value)
{ assert(value == client); return (short)countdown; }
boolean network_game_client_has_local_player(struct network_game_client *value, short controller)
{ assert(controller == 0); return value->player; }
boolean network_game_client_add_player(struct network_game_client *value, short controller)
{ assert(value == client && controller == 0); add_requests++; return TRUE; }
boolean network_game_client_server_has_started_game(struct network_game_client *value)
{ (void)value; return FALSE; }
short network_game_client_quick_join(unsigned long address)
{ searches++; last_target = address; return join_result; }

static void reset(const char *setting)
{
	memset(&quick_play, 0, sizeof(quick_play));
	memset(&mock_client, 0, sizeof(mock_client));
	client = NULL; server = NULL; mode = setting; target = "";
	clock_ms = 1000; last_target = 0; connection = 0; join_result = 0;
	cancel = created = aborts = menus = add_requests = starts = searches = reports = maps = 0;
	quick_policy = TRUE; policy_requests = 0; countdown = NONE;
	last_phase[0] = 0;
}
static void step(unsigned long elapsed, boolean menu)
{ clock_ms += elapsed; quick_play_update(menu); }
static void launch(const char *setting)
{ reset(setting); step(0, TRUE); step(2000, TRUE); assert(created == 1); }

int main(void)
{
	unsigned long address;
	/* Must match transport_client_start's SWAP4(socket address), not the
	JS ring's little-endian socket word; otherwise every elected host is skipped. */
	assert(quick_play_parse_address("100.86.56.19", &address) && address == 0x64563813UL);
	assert(quick_play_parse_address("10.1.2.3", &address) && address == 0x0A010203UL);
	assert(!quick_play_parse_address("100.86.56.19 trailing", &address));
	assert(!quick_play_parse_address("999999999999999999.1.1.1", &address));
	assert(!quick_play_parse_address("224.0.0.1", &address));
	assert(!quick_play_parse_address("0.0.0.0", &address));
	assert(!quick_play_parse_address("100.86.256.19", &address));
	assert(!quick_play_parse_address("100.86.56", &address));

	reset(""); step(1000000, TRUE); assert(!created && !reports);
	/* Manual System Link has no quick-play mode, but must keep simulating
	while hidden so later players can discover and join its host. */
	assert(!web_multiplayer_active()); client = &mock_client;
	assert(!web_multiplayer_active()); connection = _game_connection_network_server;
	assert(web_multiplayer_active()); connection = _game_connection_network_client;
	assert(web_multiplayer_active()); client = NULL; assert(!web_multiplayer_active());
	reset("invalid"); step(0, TRUE); assert(!strcmp(last_phase, "error") && !created);
	reset("join"); target = "invalid"; step(0, TRUE); assert(!strcmp(last_phase, "error") && !created);
	reset("join"); step(0, FALSE); cancel = 1; step(1, FALSE);
	assert(!strcmp(last_phase, "menu") && !created); step(1000000, TRUE); assert(!created);

	launch("join"); target = "100.86.56.19"; /* Target is captured only at launch. */
	step(499, TRUE); assert(!searches); step(1, TRUE); assert(searches == 1);
	step(60001, TRUE); assert(!strcmp(last_phase, "error") && aborts == 1);
	step(1000000, TRUE); assert(searches == 1 && created == 1 && aborts == 1);
	reset("join"); target = "100.86.56.19"; step(0, TRUE); step(2000, TRUE); step(500, TRUE);
	assert(last_target == 0x64563813UL);

	launch("join"); join_result = -1; step(500, TRUE);
	assert(!strcmp(last_phase, "error") && aborts == 1 && searches == 1);
	launch("join"); cancel = 1; step(1, TRUE);
	assert(!strcmp(last_phase, "menu") && aborts == 1 && !searches);
	launch("join"); connection = _game_connection_local; step(1, TRUE);
	assert(!strcmp(last_phase, "menu") && !aborts); step(1000000, TRUE); assert(created == 1);

	/* A successful send is not an acknowledged player: never start without it. */
	launch("host"); mock_client.state = _network_game_client_state_pregame;
	step(500, TRUE); assert(add_requests == 1 && maps == 1);
	step(3000, TRUE); assert(!starts); step(120001, TRUE);
	assert(!starts && aborts == 1 && !strcmp(last_phase, "error"));

	launch("host"); mock_client.state = _network_game_client_state_pregame;
	step(500, TRUE); mock_client.player = TRUE; step(1, TRUE);
	step(2999, TRUE); assert(!starts); step(1, TRUE); assert(starts == 1);
	step(999, TRUE); assert(starts == 1); step(1, TRUE); assert(starts == 2);
	countdown = 0; step(10000, TRUE); assert(starts == 2 && maps == 1 && policy_requests == 1);
	mock_client.state = _network_game_client_state_ingame; step(1, FALSE);
	assert(!strcmp(last_phase, "playing")); step(1000000, FALSE);
	assert(starts == 2 && !aborts); cancel = 1; step(1, FALSE);
	assert(!strcmp(last_phase, "menu") && aborts == 1);

	launch("host"); quick_policy = FALSE; mock_client.state = _network_game_client_state_pregame;
	step(500, TRUE); assert(!strcmp(last_phase, "error") && !starts && aborts == 1);
	launch("host"); mock_client.state = _network_game_client_state_pregame;
	step(500, TRUE); mock_client.player = TRUE; step(1, TRUE); step(3000, TRUE);
	assert(starts == 1); step(1000, TRUE); assert(starts == 2);
	step(120001, TRUE); assert(!strcmp(last_phase, "error") && starts == 2 && aborts == 1);

	/* A late join follows normal pregame/player acceptance, then enters the
	already running match without issuing a host start or scripted input. */
	launch("join"); join_result = 1; step(500, TRUE);
	mock_client.state = _network_game_client_state_pregame; step(500, TRUE);
	assert(add_requests == 1 && !starts); mock_client.player = TRUE; step(1, TRUE);
	mock_client.state = _network_game_client_state_ingame; step(1, FALSE);
	assert(!strcmp(last_phase, "playing") && !starts);
	mock_client.state = _network_game_client_state_postgame; step(1, FALSE);
	assert(!strcmp(last_phase, "menu") && aborts == 1);
	step(1000000, TRUE); assert(created == 1 && searches == 1 && !starts);
	puts("Quick-play lifecycle, confirmation, targeting, timeout, cancellation and late-join tests passed.");
	return 0;
}
