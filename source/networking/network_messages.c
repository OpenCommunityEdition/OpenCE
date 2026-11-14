/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "bungie_net/common/message_header.h"
#include "memory/data_packet_groups.h"
#include "networking/network_messages.h"

/* cseries_windows.c's */
unsigned long system_milliseconds(void);

/* ---------- constants */

/* ---------- macros */

#define DATA_PACKET_FIELD(type, count) { type, count, 0, 0, 0 }
#define DATA_PACKET_FIELD_END DATA_PACKET_FIELD(_data_packet_field_end, 0)
#define NETWORK_GAME_MESSAGE_DEFINITION(member, name, structure) \
	{ name, 0, sizeof(structure), 1, data_0030aa68.member##_fields, FALSE }

/* ---------- structures */

struct network_game_message_packet_definitions
{
	struct data_packet_field client_broadcast_game_search_fields[3];
	struct data_packet_definition client_broadcast_game_search;
	struct data_packet_field client_ping_fields[4];
	struct data_packet_definition client_ping;
	struct data_packet_field server_game_advertise_fields[2];
	struct data_packet_definition server_game_advertise;
	struct data_packet_field server_pong_fields[2];
	struct data_packet_definition server_pong;
	struct data_packet_field server_machine_accepted_fields[4];
	struct data_packet_definition server_machine_accepted;
	struct data_packet_field server_machine_rejected_fields[2];
	struct data_packet_definition server_machine_rejected;
	struct data_packet_field server_game_settings_update_fields[4];
	struct data_packet_definition server_game_settings_update;
	struct data_packet_field server_pregame_countdown_fields[2];
	struct data_packet_definition server_pregame_countdown;
	struct data_packet_field server_pregame_keep_alive_fields[2];
	struct data_packet_definition server_pregame_keep_alive;
	struct data_packet_field server_postgame_keep_alive_fields[2];
	struct data_packet_definition server_postgame_keep_alive;
	struct data_packet_field server_begin_game_fields[3];
	struct data_packet_definition server_begin_game;
	struct data_packet_field server_graceful_game_exit_pregame_fields[2];
	struct data_packet_definition server_graceful_game_exit_pregame;
	struct data_packet_field client_join_game_request_fields[6]; /* port: shared agreement */
	struct data_packet_definition client_join_game_request;
	struct data_packet_field client_add_player_request_pregame_fields[4];
	struct data_packet_definition client_add_player_request_pregame;
	struct data_packet_field client_remove_player_request_pregame_fields[4];
	struct data_packet_definition client_remove_player_request_pregame;
	struct data_packet_field client_settings_request_fields[4];
	struct data_packet_definition client_settings_request;
	struct data_packet_field client_player_settings_request_fields[4];
	struct data_packet_definition client_player_settings_request;
	struct data_packet_field client_game_start_request_fields[2];
	struct data_packet_definition client_game_start_request;
	struct data_packet_field client_graceful_game_exit_pregame_fields[2];
	struct data_packet_definition client_graceful_game_exit_pregame;
	struct data_packet_field client_map_is_precached_pregame_fields[2];
	struct data_packet_definition client_map_is_precached_pregame;
	struct data_packet_field server_game_update_fields[8];
	struct data_packet_definition server_game_update;
	struct data_packet_field server_add_player_ingame_fields[4];
	struct data_packet_definition server_add_player_ingame;
	struct data_packet_field server_remove_player_ingame_fields[5];
	struct data_packet_definition server_remove_player_ingame;
	struct data_packet_field server_game_over_fields[2];
	struct data_packet_definition server_game_over;
	struct data_packet_field client_loaded_fields[2];
	struct data_packet_definition client_loaded;
	struct data_packet_field client_game_update_fields[8];
	struct data_packet_definition client_game_update;
	struct data_packet_field client_add_player_request_ingame_fields[4];
	struct data_packet_definition client_add_player_request_ingame;
	struct data_packet_field client_remove_player_request_ingame_fields[4];
	struct data_packet_definition client_remove_player_request_ingame;
	struct data_packet_field client_host_crashed_cry_for_help_fields[4];
	struct data_packet_definition client_host_crashed_cry_for_help;
	struct data_packet_field client_join_new_host_fields[4];
	struct data_packet_definition client_join_new_host;
	struct data_packet_field server_switch_to_pregame_fields[2];
	struct data_packet_definition server_switch_to_pregame;
	struct data_packet_field server_graceful_game_exit_postgame_fields[2];
	struct data_packet_definition server_graceful_game_exit_postgame;
	struct data_packet_field client_remove_player_request_postgame_fields[4];
	struct data_packet_definition client_remove_player_request_postgame;
	struct data_packet_field client_switch_to_pregame_fields[2];
	struct data_packet_definition client_switch_to_pregame;
	struct data_packet_field client_graceful_game_exit_postgame_fields[2];
	struct data_packet_definition client_graceful_game_exit_postgame;
	struct data_packet_entry packets[35];
	struct data_packet_group_definition group;
};

union network_game_message_size
{
	long value;
	short encoded;
};

#define DEFINE_NETWORK_GAME_MESSAGE(name, size) typedef struct name { byte opaque[size]; } name

DEFINE_NETWORK_GAME_MESSAGE(message_client_broadcast_game_search, 0x0C);
DEFINE_NETWORK_GAME_MESSAGE(message_client_ping, 0x08);
DEFINE_NETWORK_GAME_MESSAGE(message_server_game_advertise, 0x114);
DEFINE_NETWORK_GAME_MESSAGE(message_server_pong, 0x04);
DEFINE_NETWORK_GAME_MESSAGE(message_server_machine_accepted, 0x08);
DEFINE_NETWORK_GAME_MESSAGE(message_server_machine_rejected, 0x02);
/* one piece of the game settings record: total size, offset, length, then the
bytes (network_server_message_handler.c) */
DEFINE_NETWORK_GAME_MESSAGE(message_server_game_settings_update, 8 + HALO_PORT_NETWORK_GAME_SETTINGS_FRAGMENT_SIZE);
DEFINE_NETWORK_GAME_MESSAGE(message_server_pregame_countdown, 0x02);
DEFINE_NETWORK_GAME_MESSAGE(message_server_begin_game, 0x04);
DEFINE_NETWORK_GAME_MESSAGE(message_server_graceful_game_exit_pregame, 0x04);
/* port: a long on the wire, though the Xbox game sent a short (see
server_pregame_keep_alive_fields) */
DEFINE_NETWORK_GAME_MESSAGE(message_server_pregame_keep_alive, 0x04);
DEFINE_NETWORK_GAME_MESSAGE(message_server_postgame_keep_alive, 0x02);
/* (port: the joining machine's hardware id after the Xbox's, 0x20 bytes of
hex: p2p.c's p2p_hardware_id; then a Discord account's pass, 170 bytes, and
its proof, 64: port/linux/src/player_link.c) */
DEFINE_NETWORK_GAME_MESSAGE(message_client_join_game_request, 0x70 + 170 + 64 + 2); /* port: shared agreement */
DEFINE_NETWORK_GAME_MESSAGE(message_client_add_player_request_pregame, 0x20);
DEFINE_NETWORK_GAME_MESSAGE(message_client_remove_player_request_pregame, 0x20);
DEFINE_NETWORK_GAME_MESSAGE(message_client_settings_request, 0x44);
DEFINE_NETWORK_GAME_MESSAGE(message_client_player_settings_request, 0x20);
DEFINE_NETWORK_GAME_MESSAGE(message_client_game_start_request, 0x02);
DEFINE_NETWORK_GAME_MESSAGE(message_client_graceful_game_exit_pregame, 0x04);
DEFINE_NETWORK_GAME_MESSAGE(message_client_map_is_precached_pregame, 0x100);
/* one 0x20-byte action per player */
DEFINE_NETWORK_GAME_MESSAGE(message_server_game_update, 0x10 + HALO_PORT_MAXIMUM_NETWORK_PLAYERS * 0x20);
DEFINE_NETWORK_GAME_MESSAGE(message_server_add_player_ingame, 0x20);
DEFINE_NETWORK_GAME_MESSAGE(message_server_remove_player_ingame, 0x24);
DEFINE_NETWORK_GAME_MESSAGE(message_server_game_over, 0x04);
DEFINE_NETWORK_GAME_MESSAGE(message_client_loaded, 0x04);
DEFINE_NETWORK_GAME_MESSAGE(message_client_game_update, 0x88);
DEFINE_NETWORK_GAME_MESSAGE(message_client_add_player_request_ingame, 0x20);
DEFINE_NETWORK_GAME_MESSAGE(message_client_remove_player_request_ingame, 0x20);
DEFINE_NETWORK_GAME_MESSAGE(message_client_host_crashed_cry_for_help, 0x10);
DEFINE_NETWORK_GAME_MESSAGE(message_client_join_new_host, 0x10);
DEFINE_NETWORK_GAME_MESSAGE(message_server_switch_to_pregame, 0x04);
DEFINE_NETWORK_GAME_MESSAGE(message_server_graceful_game_exit_postgame, 0x04);
DEFINE_NETWORK_GAME_MESSAGE(message_client_remove_player_request_postgame, 0x20);
DEFINE_NETWORK_GAME_MESSAGE(message_client_switch_to_pregame, 0x04);
DEFINE_NETWORK_GAME_MESSAGE(message_client_graceful_game_exit_postgame, 0x04);

#undef DEFINE_NETWORK_GAME_MESSAGE

/* ---------- prototypes */

/* ---------- globals */

static struct network_game_message_packet_definitions data_0030aa68 =
{
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 2),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 8),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_broadcast_game_search, "message_client_broadcast_game_search_packet", message_client_broadcast_game_search),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 1),
		DATA_PACKET_FIELD(_data_packet_field_pad, 2),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_ping, "message_client_ping_packet", message_client_ping),
	{
		DATA_PACKET_FIELD(_data_packet_field_raw, 276),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_game_advertise, "message_server_game_advertise_packet", message_server_game_advertise),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_pong, "message_server_pong_packet", message_server_pong),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 1),
		DATA_PACKET_FIELD(_data_packet_field_pad, 2),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_machine_accepted, "message_server_machine_accepted_packet", message_server_machine_accepted),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_machine_rejected, "message_server_machine_rejected_packet", message_server_machine_rejected),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 3),
		DATA_PACKET_FIELD(_data_packet_field_pad, 2),
		DATA_PACKET_FIELD(_data_packet_field_raw, HALO_PORT_NETWORK_GAME_SETTINGS_FRAGMENT_SIZE),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_game_settings_update, "message_server_game_settings_update_packet", message_server_game_settings_update),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_pregame_countdown, "message_server_pregame_countdown_packet", message_server_pregame_countdown),
	/* port: the Xbox game's packet table had begin game, graceful exit and
	keep-alive each on its neighbor's field list (table order keep-alive,
	begin game, exit; type order begin game, exit, keep-alive): the wire, which
	upstream and every earlier build share, carries begin game as a short
	(16 bits of its long), exit and keep-alive as a long. The field lists
	here are those of the wire under the right messages, and a keep-alive is
	a long (a short read and written as a long overran it by 2 bytes, sending
	2 bytes of stack and writing 2 bytes from the network past it) */
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_pregame_keep_alive, "message_server_pregame_keep_alive_packet", message_server_pregame_keep_alive),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_postgame_keep_alive, "message_server_postgame_keep_alive_packet", message_server_postgame_keep_alive),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 1),
		DATA_PACKET_FIELD(_data_packet_field_pad, 2),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_begin_game, "message_server_begin_game_packet", message_server_begin_game),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_graceful_game_exit_pregame, "message_server_graceful_game_exit_pregame_packet", message_server_graceful_game_exit_pregame),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 32),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 16),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 32),
		/* (port: the Discord pass, 170 bytes, and its proof, 64) */
		DATA_PACKET_FIELD(_data_packet_field_bytes, 234),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 1), /* port: shared agreement */
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_join_game_request, "message_client_join_game_request_packet", message_client_join_game_request),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 12),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 2),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 4),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_add_player_request_pregame, "message_client_add_player_request_pregame_packet", message_client_add_player_request_pregame),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 12),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 2),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 4),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_remove_player_request_pregame, "message_client_remove_player_request_pregame_packet", message_client_remove_player_request_pregame),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 32),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 1),
		DATA_PACKET_FIELD(_data_packet_field_pad, 3),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_settings_request, "message_client_settings_request_packet", message_client_settings_request),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 12),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 2),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 4),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_player_settings_request, "message_client_player_settings_request_packet", message_client_player_settings_request),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_game_start_request, "message_client_game_start_request_packet", message_client_game_start_request),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_graceful_game_exit_pregame, "message_client_graceful_game_exit_pregame_packet", message_client_graceful_game_exit_pregame),
	{
		DATA_PACKET_FIELD(_data_packet_field_bytes, 256),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_map_is_precached_pregame, "message_client_map_is_precached_pregame_packet", message_client_map_is_precached_pregame),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 3),
		DATA_PACKET_FIELD(_data_packet_field_pad, 2),
		DATA_PACKET_FIELD(_data_packet_field_array, HALO_PORT_MAXIMUM_NETWORK_PLAYERS),
		DATA_PACKET_FIELD(_data_packet_field_longs, 6),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 3),
		DATA_PACKET_FIELD(_data_packet_field_pad, 2),
		DATA_PACKET_FIELD_END,
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_game_update, "message_server_game_update_packet", message_server_game_update),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 12),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 2),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 4),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_add_player_ingame, "message_server_add_player_ingame_packet", message_server_add_player_ingame),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 12),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 2),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 4),
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_remove_player_ingame, "message_server_remove_player_ingame_packet", message_server_remove_player_ingame),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_game_over, "message_server_game_over_packet", message_server_game_over),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_loaded, "message_client_loaded_packet", message_client_loaded),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD(_data_packet_field_pad, 2),
		DATA_PACKET_FIELD(_data_packet_field_array, 4),
		DATA_PACKET_FIELD(_data_packet_field_longs, 6),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 3),
		DATA_PACKET_FIELD(_data_packet_field_pad, 2),
		DATA_PACKET_FIELD_END,
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_game_update, "message_client_game_update_packet", message_client_game_update),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 12),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 2),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 4),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_add_player_request_ingame, "message_client_add_player_request_ingame_packet", message_client_add_player_request_ingame),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 12),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 2),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 4),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_remove_player_request_ingame, "message_client_remove_player_request_ingame_packet", message_client_remove_player_request_ingame),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 3),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 1),
		DATA_PACKET_FIELD(_data_packet_field_pad, 2),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_host_crashed_cry_for_help, "message_client_host_crashed_cry_for_help_packet", message_client_host_crashed_cry_for_help),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 3),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 1),
		DATA_PACKET_FIELD(_data_packet_field_pad, 2),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_join_new_host, "message_client_join_new_host_packet", message_client_join_new_host),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_switch_to_pregame, "message_server_switch_to_pregame_packet", message_server_switch_to_pregame),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(server_graceful_game_exit_postgame, "message_server_graceful_game_exit_postgame_packet", message_server_graceful_game_exit_postgame),
	{
		DATA_PACKET_FIELD(_data_packet_field_shorts, 12),
		DATA_PACKET_FIELD(_data_packet_field_shorts, 2),
		DATA_PACKET_FIELD(_data_packet_field_bytes, 4),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_remove_player_request_postgame, "message_client_remove_player_request_postgame_packet", message_client_remove_player_request_postgame),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_switch_to_pregame, "message_client_switch_to_pregame_packet", message_client_switch_to_pregame),
	{
		DATA_PACKET_FIELD(_data_packet_field_longs, 1),
		DATA_PACKET_FIELD_END,
	},
	NETWORK_GAME_MESSAGE_DEFINITION(client_graceful_game_exit_postgame, "message_client_graceful_game_exit_postgame_packet", message_client_graceful_game_exit_postgame),
	{
		{ 0, 0, &data_0030aa68.client_broadcast_game_search },
		{ 0, 0, &data_0030aa68.client_ping },
		{ 1, 0, &data_0030aa68.server_game_advertise },
		{ 1, 0, &data_0030aa68.server_pong },
		{ 2, 0, &data_0030aa68.server_machine_accepted },
		{ 2, 0, &data_0030aa68.server_machine_rejected },
		{ 2, 0, &data_0030aa68.server_game_settings_update },
		{ 2, 0, &data_0030aa68.server_pregame_countdown },
		{ 2, 0, &data_0030aa68.server_begin_game },
		{ 2, 0, &data_0030aa68.server_graceful_game_exit_pregame },
		{ 2, 0, &data_0030aa68.server_pregame_keep_alive },
		{ 6, 0, &data_0030aa68.server_postgame_keep_alive },
		{ 3, 0, &data_0030aa68.client_join_game_request },
		{ 3, 0, &data_0030aa68.client_add_player_request_pregame },
		{ 3, 0, &data_0030aa68.client_remove_player_request_pregame },
		{ 3, 0, &data_0030aa68.client_settings_request },
		{ 3, 0, &data_0030aa68.client_player_settings_request },
		{ 3, 0, &data_0030aa68.client_game_start_request },
		{ 3, 0, &data_0030aa68.client_graceful_game_exit_pregame },
		{ 3, 0, &data_0030aa68.client_map_is_precached_pregame },
		{ 4, 0, &data_0030aa68.server_game_update },
		{ 4, 0, &data_0030aa68.server_add_player_ingame },
		{ 4, 0, &data_0030aa68.server_remove_player_ingame },
		{ 4, 0, &data_0030aa68.server_game_over },
		{ 5, 0, &data_0030aa68.client_loaded },
		{ 5, 0, &data_0030aa68.client_game_update },
		{ 5, 0, &data_0030aa68.client_add_player_request_ingame },
		{ 5, 0, &data_0030aa68.client_remove_player_request_ingame },
		{ 5, 0, &data_0030aa68.client_host_crashed_cry_for_help },
		{ 5, 0, &data_0030aa68.client_join_new_host },
		{ 6, 0, &data_0030aa68.server_switch_to_pregame },
		{ 6, 0, &data_0030aa68.server_graceful_game_exit_postgame },
		{ 7, 0, &data_0030aa68.client_remove_player_request_postgame },
		{ 7, 0, &data_0030aa68.client_switch_to_pregame },
		{ 7, 0, &data_0030aa68.client_graceful_game_exit_postgame },
	},
	{
		"network_game_messages_group",
		35,
		8,
		/* the per-tick update of 128 players decodes to 0x1010 bytes */
		HALO_PORT_NETWORK_PACKET_SIZE,
		HALO_PORT_NETWORK_PACKET_SIZE,
		data_0030aa68.packets,
	},
};

static byte network_game_message_buffer[HALO_PORT_MAXIMUM_NETWORK_MESSAGE_SIZE + 4];

/* ---------- public code */

void initialize_network_game_packets(
	void)
{
	data_packet_group_initialize(&data_0030aa68.group);

	return;
}

void network_event(
	char *format,
	...)
{
	va_list arguments;

#line 331 "c:\\halo\\SOURCE\\networking\\network_messages.c"
	match_assert(__FILE__, __LINE__, format);

	/* port: no more than NETWORK_EVENTS_PER_SECOND lines a second, then how
	many were left out (a flood of datagrams, which anyone may send, each
	logged, stalled the game writing its log a line at a time) */
	{
		enum
		{
			NETWORK_EVENTS_PER_SECOND = 64,
		};
		static unsigned long second_time = 0;
		static long second_count = 0;
		static long left_out_count = 0;
		unsigned long now = system_milliseconds();

		if (!second_time || now - second_time >= 1000)
		{
			second_time = now ? now : 1;
			second_count = 0;
			if (left_out_count)
				error(3, "(%ld more network events not logged)", left_out_count);
			left_out_count = 0;
		}
		if (++second_count > NETWORK_EVENTS_PER_SECOND)
		{
			left_out_count++;
			return;
		}
	}

	va_start(arguments, format);
	_vsnprintf(temporary, NUMBEROF(temporary) - 1, format, arguments);
	va_end(arguments);

	/* (formatted already: what it holds, names and addresses from the
	network among it, is not a format) */
	error(3, "%s", temporary);

	return;
}

static boolean encode_network_game_message(
	const void *message_struct,
	void *encoded_message,
	short *encoded_message_size,
	enum network_game_message_type message_type,
	long message_version)
{
#line 353 "c:\\halo\\SOURCE\\networking\\network_messages.c"
	match_assert(__FILE__, __LINE__, message_struct && encoded_message && encoded_message_size && (*encoded_message_size>0));

	return data_packet_group_encode_packet(&data_0030aa68.group, message_struct, encoded_message, encoded_message_size, message_type, message_version);
}

void *create_network_game_message(
	enum network_game_message_type message_type,
	const void *message_struct,
	short message_struct_size)
{
	/* as large as the packet group lets the encoder write */
	byte encoded_message[HALO_PORT_NETWORK_PACKET_SIZE];
	union network_game_message_size encoded_message_size;
	void *message;

	encoded_message_size.value = sizeof(encoded_message);

	switch ((short)message_type)
	{
	case _message_client_broadcast_game_search:
#line 160 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_broadcast_game_search));
		break;
	case _message_client_ping:
#line 161 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_ping));
		break;
	case _message_server_game_advertise:
#line 164 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_game_advertise));
		break;
	case _message_server_pong:
#line 165 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_pong));
		break;
	case _message_server_machine_accepted:
#line 168 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_machine_accepted));
		break;
	case _message_server_machine_rejected:
#line 169 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_machine_rejected));
		break;
	case _message_server_game_settings_update:
#line 170 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_game_settings_update));
		break;
	case _message_server_pregame_countdown:
#line 171 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_pregame_countdown));
		break;
	case _message_server_pregame_keep_alive:
#line 172 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_pregame_keep_alive));
		break;
	case _message_server_begin_game:
#line 173 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_begin_game));
		break;
	case _message_server_graceful_game_exit_pregame:
#line 174 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_graceful_game_exit_pregame));
		break;
	case _message_server_postgame_keep_alive:
#line 177 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_postgame_keep_alive));
		break;
	case _message_client_join_game_request:
#line 180 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_join_game_request));
		break;
	case _message_client_add_player_request_pregame:
#line 181 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_add_player_request_pregame));
		break;
	case _message_client_remove_player_request_pregame:
#line 182 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_remove_player_request_pregame));
		break;
	case _message_client_settings_request:
#line 183 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_settings_request));
		break;
	case _message_client_player_settings_request:
#line 184 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_player_settings_request));
		break;
	case _message_client_game_start_request:
#line 185 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_game_start_request));
		break;
	case _message_client_graceful_game_exit_pregame:
#line 186 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_graceful_game_exit_pregame));
		break;
	case _message_client_map_is_precached_pregame:
#line 187 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_map_is_precached_pregame));
		break;
	case _message_server_game_update:
#line 190 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_game_update));
		break;
	case _message_server_add_player_ingame:
#line 191 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_add_player_ingame));
		break;
	case _message_server_remove_player_ingame:
#line 192 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_remove_player_ingame));
		break;
	case _message_server_game_over:
#line 193 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_game_over));
		break;
	case _message_client_loaded:
#line 196 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_loaded));
		break;
	case _message_client_game_update:
#line 197 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_game_update));
		break;
	case _message_client_add_player_request_ingame:
#line 198 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_add_player_request_ingame));
		break;
	case _message_client_remove_player_request_ingame:
#line 199 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_remove_player_request_ingame));
		break;
	case _message_client_host_crashed_cry_for_help:
#line 201 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_host_crashed_cry_for_help));
		break;
	case _message_client_join_new_host:
#line 202 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_join_new_host));
		break;
	case _message_server_switch_to_pregame:
#line 205 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_switch_to_pregame));
		break;
	case _message_server_graceful_game_exit_postgame:
#line 206 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_server_graceful_game_exit_postgame));
		break;
	case _message_client_remove_player_request_postgame:
#line 209 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_remove_player_request_postgame));
		break;
	case _message_client_switch_to_pregame:
#line 210 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_switch_to_pregame));
		break;
	case _message_client_graceful_game_exit_postgame:
#line 211 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_assert(__FILE__, __LINE__, message_struct_size==sizeof(message_client_graceful_game_exit_postgame));
		break;
	default:
#line 213 "c:\\halo\\SOURCE\\networking\\network_messages.c"
		match_vassert(__FILE__, __LINE__, FALSE, "unknown network game message structure type");
		break;
	}

	if (encode_network_game_message(message_struct, encoded_message, &encoded_message_size.encoded, message_type, 1))
	{
		/* a message header holds lengths up to HALO_PORT_MAXIMUM_NETWORK_MESSAGE_SIZE-1,
		header included: a longer message would go out with a wrong length and
		derail the stream (create_message only asserts on it) */
		if (encoded_message_size.value + sizeof(word) >= HALO_PORT_MAXIMUM_NETWORK_MESSAGE_SIZE)
		{
			network_event("create_network_game_message(): the encoded message is too long");
			message = NULL;
		}
		else
		message = create_message(3, encoded_message, encoded_message_size.value, network_game_message_buffer, sizeof(network_game_message_buffer));
		if (!message)
		{
			network_event("create_message() failed");
		}
	}
	else
	{
		network_event("encode_network_game_message() failed");
		message = NULL;
	}

	return message;
}

boolean decode_network_game_message(
	void *message_struct,
	const void *encoded_message,
	short *encoded_message_size,
	short *packet_type,
	short *packet_version,
	long expected_packet_class)
{
	boolean result;

#line 313 "c:\\halo\\SOURCE\\networking\\network_messages.c"
	match_assert(__FILE__, __LINE__, message_struct && encoded_message && encoded_message_size && (*encoded_message_size>0) && packet_type && (*packet_type>=0) && packet_version && (*packet_version>0));

	result = data_packet_group_decode_packet(&data_0030aa68.group, message_struct, encoded_message, encoded_message_size, packet_type, packet_version, expected_packet_class);

	if (!result)
	{
		network_event("decode_network_game_message() failed");
	}

	return result;
}

/* ---------- private code */
