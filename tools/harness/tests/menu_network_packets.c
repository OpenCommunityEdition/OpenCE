/* The real packet admission guard, kick/ban notices and ban records. Only
   dispatch after admission and external IO are recorders; no network or profile. */
#include "harness.h"
typedef unsigned short word;
typedef word message_header;
#include "config.inc"
#define BANS_FILE "fake bans"
#define _error_log 0

static struct { boolean initialized, active, paused; } clock_state = { TRUE, TRUE, FALSE }, *game_time_globals = &clock_state;
static boolean menu;
static int admitted, broadcasts, warnings, errors, identity_reads, record_count;
static char record_line[512];
static struct distributed_client_identity distributed_client_identities[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
static boolean main_menu_is_active(void) { return menu; }
static void distributed_send_notice(const char *text) { CHECK(text[0], "empty notice"); broadcasts++; }
static void console_warning(const char *format, ...) { warnings++; }
static void error(int level, const char *format, ...) { errors++; }
static void distributed_address_text(unsigned long address, char *text, int size) { snprintf(text, size, "127.0.0.3"); }
static const char *network_game_server_machine_hardware_id(long index) { return "current-machine"; }
static void p2p_hardware_id_sanitize(char *to, int size, const char *from) { snprintf(to, size, "%s", from); }
static void p2p_discord_sanitize(char *to, int size, const char *from, int name)
{
	identity_reads++;
	snprintf(to, size, "%s", from);
}
static FILE *record_open(const char *path, const char *mode)
{
	CHECK(!strcmp(path, BANS_FILE) && !strcmp(mode, "a"), "wrong record target/mode");
	FILE *file = tmpfile();
	CHECK(file, "temporary record file unavailable");
	record_count++;
	return file;
}
static int record_close(FILE *file)
{
	rewind(file);
	CHECK(fgets(record_line, sizeof(record_line), file), "ban record not written");
	return fclose(file);
}
#define fopen record_open
#define fclose record_close
#include "under_test.inc"
#undef fopen
#undef fclose

static void packet(word size)
{
	struct distributed_message_header header = { 0, 0, 0, 9000 };
	network_distributed_handle_message(1, (word *)&header, size);
}
static void check_local_notice(void)
{
	CHECK(!broadcasts && warnings == 1 && errors == 1, "lobby notice entered gameplay transport");
	CHECK(!identity_reads, "lobby notice read an identity from the previous round");
}
int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	strcpy(distributed_client_identities[1].discord_id, "old-id");
	strcpy(distributed_client_identities[1].discord_name, "old-name");
	CASE("menu-packets")
	{
		menu = TRUE;
		packet(sizeof(struct distributed_message_header));
		CHECK(!admitted, "delayed gameplay packet admitted into running menu scene");
		return 0;
	}
	CASE("game-packets")
	{
		packet(sizeof(struct distributed_message_header));
		CHECK(admitted == 1, "active gameplay packet dropped");
		return 0;
	}
	CASE("paused-game-packets")
	{
		clock_state.active = FALSE; clock_state.paused = TRUE;
		packet(sizeof(struct distributed_message_header));
		CHECK(admitted == 1, "paused gameplay packet dropped");
		return 0;
	}
	CASE("loading-packets")
	{
		clock_state.active = FALSE;
		packet(sizeof(struct distributed_message_header));
		clock_state.active = TRUE; clock_state.initialized = FALSE;
		packet(sizeof(struct distributed_message_header));
		CHECK(!admitted, "inactive/uninitialized scene admitted gameplay packet");
		return 0;
	}
	CASE("short-packets")
	{
		for (word size = 0; size < sizeof(struct distributed_message_header); size++) packet(size);
		CHECK(!admitted, "short packet admitted");
		return 0;
	}
	CASE("packet-transitions")
	{
		menu = TRUE; packet(sizeof(struct distributed_message_header));
		CHECK(!admitted, "lobby admitted gameplay packet");
		menu = FALSE; clock_state.active = FALSE;
		packet(sizeof(struct distributed_message_header));
		CHECK(!admitted, "loading admitted gameplay packet");
		clock_state.active = TRUE; packet(sizeof(struct distributed_message_header));
		CHECK(admitted == 1, "loaded game rejected gameplay packet");
		menu = TRUE; packet(sizeof(struct distributed_message_header));
		CHECK(admitted == 1, "return to lobby admitted old-round packet");
		menu = FALSE; packet(sizeof(struct distributed_message_header));
		CHECK(admitted == 2, "next round rejected gameplay packet");
		return 0;
	}
	CASE("menu-kick")
	{
		menu = TRUE; network_distributed_kick("player"); check_local_notice();
		CHECK(!record_count, "kick persisted a ban");
		return 0;
	}
	CASE("game-kick")
	{
		network_distributed_kick("player");
		CHECK(broadcasts == 1 && !warnings && !errors && !record_count, "game kick notice/record changed");
		return 0;
	}
	CASE("menu-ban")
	{
		menu = TRUE; network_distributed_ban(1, 1, "player"); check_local_notice();
		CHECK(record_count == 1 && strstr(record_line, "hwid=current-machine") &&
			strstr(record_line, "discord_id=none") && !strstr(record_line, "old-"), "lobby ban used previous round identity or lost record");
		return 0;
	}
	CASE("game-ban")
	{
		network_distributed_ban(1, 1, "player");
		CHECK(broadcasts == 1 && !warnings && !errors && identity_reads == 4 && record_count == 1 &&
			strstr(record_line, "discord_id=old-id"), "game ban lost identity/notice/record");
		return 0;
	}
	CASE("loading-ban")
	{
		clock_state.active = FALSE; network_distributed_ban(1, 1, "player"); check_local_notice();
		CHECK(record_count == 1 && !strstr(record_line, "old-"), "loading ban lost record/used old identity");
		return 0;
	}
	CHECK(FALSE, "unknown case: %s", case_name);
}
