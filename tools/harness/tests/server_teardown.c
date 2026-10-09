/* Actual server disposal and menu callbacks; the network/transport are ordered
   recorders. No sockets, assets, user profile or wall-clock sleeps. */
#include "harness.h"
#include "config.inc"
#define NETWORK_SERVER_MANAGER_FILE "server"
#define _error_silent 0
#define csmemset memset
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))

struct network_connection { int unused; };
struct network_game_server
{
	short state;
	unsigned flags;
	struct network_connection *connection;
	unsigned long time_of_first_client_loading_completion;
	boolean sent_start_game_message;
};
struct network_game_client { int unused; };
struct network_message { int unused; };
struct widget_instance { int unused; };
struct event_record { int unused; };
static struct
{
	struct network_game_server *server;
	struct network_game_client *client;
	boolean quickstart_local, client_started;
} bss_004566dc;
static char network_game_server_cooperative_next_map[128];
static boolean network_game_server_memory_do_not_use_directly_in_use;
static struct network_message message;
static struct network_game_server *disposing;
static char order[64];
static int slept, messages, broadcasts, handled, closed, terminated, errors;
static int new_servers, clients_closed, joins_cleared, variants_cleared;
static short expected_message;
static boolean message_fails, send_fails, machines_fail;

static void note(char event)
{
	size_t length = strlen(order);
	CHECK(length + 1 < sizeof(order), "event recorder overflow");
	order[length] = event; order[length + 1] = 0;
}
static void network_event(const char *text, ...) { }
static void error(int level, const char *text, ...) { errors++; }
static struct network_message *create_network_game_message(short kind, const void *packet, long size)
{
	CHECK(kind == expected_message && size == sizeof(long) && *(const long *)packet == 0,
		"graceful-exit packet changed or contained uninitialized data");
	messages++; note('M');
	return message_fails ? NULL : &message;
}
static boolean network_game_server_send_message_to_all_machines(struct network_game_server *server, struct network_message *packet)
{
	CHECK(server == disposing && packet == &message && !closed && !terminated, "exit notice sent after cleanup");
	broadcasts++; note('B'); return !send_fails;
}
static boolean network_game_server_handle_client_machines(struct network_game_server *server)
{
	CHECK(server == disposing && !server->time_of_first_client_loading_completion && !server->sent_start_game_message &&
		!TEST_FLAG(server->flags, _network_game_server_game_open_bit), "teardown could start another game or accept clients");
	CHECK(!network_game_server_cooperative_next_map[0], "next co-op map survived teardown");
	handled++; note('H'); return !machines_fail;
}
static void network_connection_delete(struct network_connection *connection)
{
	CHECK(connection == disposing->connection && handled == 1 && !terminated, "socket cleanup order changed");
	closed++; note('C');
}
static void SleepEx(unsigned long milliseconds, boolean alertable)
{
	CHECK(closed == 1 && !terminated && !alertable, "legacy wait moved before connection cleanup");
	slept += milliseconds; note('S');
}
static short transport_server_terminate(void)
{
	CHECK(closed == 1 && slept == EXPECTED_SLEEP, "transport terminated before cleanup/required legacy grace");
	terminated++; note('T'); return 0;
}
static void p2p_set_game_player_counts(int count, int maximum)
{
	struct network_game_server empty = { 0 };
	CHECK(count == 0 && maximum == 0 && terminated == 1 &&
		!network_game_server_memory_do_not_use_directly_in_use && !memcmp(disposing, &empty, sizeof(empty)),
		"server state/counts not cleared after transport cleanup");
	note('P');
}
static void network_game_client_dispose(struct network_game_client *client)
{
	CHECK(client == bss_004566dc.client && !messages && !handled, "local client disposal order changed");
	clients_closed++; note('L');
}
static void player_ui_clear_multiplayer_joins(void) { joins_cleared++; note('J'); }
static void player_ui_clear_multiplayer_variant(void) { variants_cleared++; note('V'); }
static boolean network_game_start_new_server(struct widget_instance *widget, struct event_record *event, boolean *deleted)
{
	CHECK(!bss_004566dc.client && !bss_004566dc.server, "new host started before old host/client disposed");
	new_servers++; note('N'); return TRUE;
}

#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	struct network_connection connection = { 0 };
	struct network_game_client client = { 0 };
	struct network_game_server host = { _network_game_server_state_pregame,
		FLAG(_network_game_server_game_open_bit) | FLAG(_network_game_server_game_valid_bit), &connection, 123, TRUE };
	boolean deleted = FALSE;
	disposing = &host;
	bss_004566dc.server = &host; bss_004566dc.client = &client;
	bss_004566dc.quickstart_local = bss_004566dc.client_started = TRUE;
	network_game_server_memory_do_not_use_directly_in_use = TRUE;
	strcpy(network_game_server_cooperative_next_map, "next-level");
	expected_message = _message_server_graceful_game_exit_pregame;
	boolean back = !strcmp(case_name, "back"), reopen = !strcmp(case_name, "reopen");
	CASE("postgame") { host.state = _network_game_server_state_postgame; expected_message = _message_server_graceful_game_exit_postgame; }
	CASE("ingame") { host.state = _network_game_server_state_ingame; }
	CASE("message-failure") { message_fails = TRUE; }
	CASE("send-failure") { send_fails = TRUE; }
	CASE("machine-failure") { machines_fail = TRUE; }
	CASE("no-server")
	{
		bss_004566dc.server = NULL;
		CHECK(ui_widget_port_host(NULL, NULL, &deleted), "new host failed");
		CHECK(!slept && !closed && !handled && !terminated && clients_closed == 1 && new_servers == 1,
			"first host creation performed nonexistent server teardown");
		return 0;
	}
	if (back) CHECK(clear_multiplayer_player_joins(NULL, NULL, &deleted), "back callback failed");
	else if (reopen) CHECK(ui_widget_port_host(NULL, NULL, &deleted), "reopen callback failed");
	else network_game_server_dispose(&host);
	CHECK(slept == EXPECTED_SLEEP && closed == 1 && handled == 1 && terminated == 1, "wrong sleep or incomplete teardown: %s", order);
	CHECK(errors == (machines_fail ? 1 : 0), "error handling changed");
	int expected_messages = strcmp(case_name, "ingame") ? 1 : 0;
	CHECK(messages == expected_messages && broadcasts == (message_fails ? 0 : expected_messages), "graceful-exit notice lost");
	char expected[64] = "";
	if (back || reopen) strcat(expected, "L");
	if (expected_messages) strcat(expected, message_fails ? "M" : "MB");
	strcat(expected, EXPECTED_SLEEP ? "HCSTP" : "HCTP");
	if (back) strcat(expected, "JV");
	if (reopen) strcat(expected, "N");
	CHECK(!strcmp(order, expected), "cleanup sequence changed: %s != %s", order, expected);
	if (back || reopen)
		CHECK(!bss_004566dc.server && !bss_004566dc.client && !bss_004566dc.quickstart_local && !bss_004566dc.client_started &&
			clients_closed == 1 && new_servers == reopen && joins_cleared == back && variants_cleared == back,
			"menu lifecycle state changed");
	return 0;
}
