/*
NETWORK_LINK.C

A dedicated server's door for VPNs, and the Discord accounts its machines
show (network_link.h; docs/dedicated_server.md, "VPNs and linked Discord
accounts"; docs/master_protocol.md, "Player keys, links and passes").

A machine joining from an address the masters' netlist lists (a VPN's, a
Tor exit's, a datacenter's: server_netlist.c) is refused, unless it shows
a pass of a Discord account linked at a master the server publishes to
(player_link.c checks it: its master's signature, its times, and its proof
bound to this connection's tunnel). The game of an updated client, refused
with _rejection_code_vpn_link_required, joins again at once with its pass.
A pass that checks makes the machine a verified Discord account, whatever
address it joins from: its id and username replace what the machine says
of itself (network_distributed.c), and a ban by the account
(network_distributed_banned) keeps it out.

The host decides it all; a pass is untrusted until it checks: its bytes
come from the wire.
*/

#include "cseries.h"
#include "networking/network_game_manager.h"
#include "networking/network_game_protocol.h"
#include "networking/network_messages.h"
#include "network_link.h"

#include "../src/player_link.h" /* port: port/linux/src/player_link.c */
#include "../src/server_netlist.h" /* port: port/linux/src/server_netlist.c */
#include "../src/server_events.h" /* port: port/linux/src/server_events.c */
#include "../src/server_json.h" /* port: port/linux/src/server_json.c */
#include "../src/server_log.h" /* port: port/linux/src/server_log.c */
#include "../src/port_config.h" /* port: port/linux/src/port_config.c */

#include <stdio.h>
#include <string.h>
#include <time.h>

int halo_server_mode(void);
char const *network_game_server_machine_hardware_id(long machine_index);
unsigned long distributed_machine_real_address(long machine_index);

/* each machine's pass as it joined, and what checked of it (by machine
index) */
static struct
{
	byte pass[PLAYER_LINK_PASS_SIZE];
	byte proof[PLAYER_LINK_PROOF_SIZE];
	char name[32];
	boolean verified;
	char discord_id[PLAYER_LINK_DISCORD_ID_TEXT_SIZE];
	char discord_name[PLAYER_LINK_NAME_SIZE + 1];
	unsigned long created;
	/* why its pass did not check */
	char why[160];
} network_link_machines[HALO_PORT_MAXIMUM_NETWORK_MACHINES];

void network_link_set_machine_pass(
	long machine_index,
	void const *pass,
	void const *proof,
	char const *name)
{
	if (machine_index < 0 || machine_index >= HALO_PORT_MAXIMUM_NETWORK_MACHINES)
		return;
	csmemcpy(network_link_machines[machine_index].pass, pass, PLAYER_LINK_PASS_SIZE);
	csmemcpy(network_link_machines[machine_index].proof, proof, PLAYER_LINK_PROOF_SIZE);
	snprintf(network_link_machines[machine_index].name, sizeof(network_link_machines[machine_index].name), "%s",
		name ? name : "");
	network_link_machines[machine_index].verified = FALSE;
	network_link_machines[machine_index].discord_id[0] = 0;
	network_link_machines[machine_index].discord_name[0] = 0;
	network_link_machines[machine_index].created = 0;
	network_link_machines[machine_index].why[0] = 0;
}

boolean network_link_machine_verified(
	long machine_index,
	char *discord_id,
	long discord_id_size,
	char *discord_name,
	long discord_name_size)
{
	if (machine_index < 0 || machine_index >= HALO_PORT_MAXIMUM_NETWORK_MACHINES ||
		!network_link_machines[machine_index].verified)
	{
		return FALSE;
	}
	if (discord_id && discord_id_size > 0)
		snprintf(discord_id, (size_t)discord_id_size, "%s", network_link_machines[machine_index].discord_id);
	if (discord_name && discord_name_size > 0)
		snprintf(discord_name, (size_t)discord_name_size, "%s", network_link_machines[machine_index].discord_name);
	return TRUE;
}

/* an IPv4 address (host byte order) that the internet routes: none of IANA's
special-purpose ranges (docs/master_protocol.md, "Publishing") */
static boolean network_link_address_global(
	unsigned long address)
{
	static struct
	{
		unsigned long first;
		long bits;
	} const special[] =
	{
		{ 0x00000000, 8 }, { 0x0A000000, 8 }, { 0x64400000, 10 }, { 0x7F000000, 8 }, { 0xA9FE0000, 16 },
		{ 0xAC100000, 12 }, { 0xC0000000, 24 }, { 0xC0000200, 24 }, { 0xC0586300, 24 }, { 0xC0A80000, 16 },
		{ 0xC6120000, 15 }, { 0xC6336400, 24 }, { 0xCB007100, 24 }, { 0xE0000000, 4 }, { 0xF0000000, 4 },
	};
	long index;

	for (index = 0; index < (long)NUMBEROF(special); index++)
	{
		unsigned long mask = 0xFFFFFFFFUL << (32 - special[index].bits) & 0xFFFFFFFFUL;

		if ((address & mask) == special[index].first)
			return FALSE;
	}
	return TRUE;
}

/* a refusal: counted, logged, and an event (join_refused) */
static short network_link_refuse(
	long machine_index,
	unsigned long real_address,
	short code,
	char const *reason,
	char const *why)
{
	struct server_json json[2];
	char address[32];
	char const *hardware_id = network_game_server_machine_hardware_id(machine_index);
	long pass;

	snprintf(address, sizeof(address), "%lu.%lu.%lu.%lu", (real_address >> 24) & 255, (real_address >> 16) & 255,
		(real_address >> 8) & 255, real_address & 255);
	network_event("refusing a machine @ %s: %s: %s", address, reason, why);
	server_log(_server_log_info, "player", "%s refused at the door (%s): %s",
		network_link_machines[machine_index].name[0] ? network_link_machines[machine_index].name : "a machine", reason,
		why);
	server_netlist_count_refusal(reason);
	for (pass = 0; pass < 2; pass++)
	{
		server_json_begin(&json[pass]);
		server_json_object_begin(&json[pass], NULL);
		server_json_string(&json[pass], "event", "join_refused");
		server_json_string(&json[pass], "name", network_link_machines[machine_index].name);
		server_json_string(&json[pass], "reason", reason);
		if (pass)
		{
			server_json_string(&json[pass], "address", address);
			server_json_string(&json[pass], "hwid", hardware_id ? hardware_id : "");
		}
		server_json_object_end(&json[pass]);
	}
	server_event_add_addressed("moderation", server_json_text(&json[0]), server_json_text(&json[1]));
	server_json_free(&json[0]);
	server_json_free(&json[1]);
	return code;
}

void network_link_check_pass(
	long machine_index,
	unsigned long address)
{
	unsigned long virtual_address, created = 0;
	char why[160] = "";

	if (!halo_server_mode() || machine_index < 0 || machine_index >= HALO_PORT_MAXIMUM_NETWORK_MACHINES)
		return;
	network_link_machines[machine_index].verified = FALSE;
	if (!player_link_pass_shown(network_link_machines[machine_index].pass))
		return;
	/* (its tunnel's binding: an internet play peer's stand-in, in
	100.64.0.0/10, in network byte order) */
	virtual_address = (address & 0xFFC00000) == 0x64400000 ?
		(address >> 24) | ((address >> 8) & 0xFF00) | ((address << 8) & 0xFF0000) | (address << 24) : 0;
	network_link_machines[machine_index].verified = player_link_server_check(network_link_machines[machine_index].pass,
		network_link_machines[machine_index].proof, virtual_address,
		network_link_machines[machine_index].discord_id, sizeof(network_link_machines[machine_index].discord_id),
		network_link_machines[machine_index].discord_name, sizeof(network_link_machines[machine_index].discord_name),
		&created, why, sizeof(why)) != 0;
	network_link_machines[machine_index].created = created;
	if (network_link_machines[machine_index].verified)
	{
		server_log(_server_log_info, "player", "%s shows the Discord account %s (%s)",
			network_link_machines[machine_index].name, network_link_machines[machine_index].discord_name,
			network_link_machines[machine_index].discord_id);
	}
	else
	{
		snprintf(network_link_machines[machine_index].why, sizeof(network_link_machines[machine_index].why), "%s", why);
		server_log(_server_log_info, "player", "%s shows a Discord pass that does not check: %s",
			network_link_machines[machine_index].name, why);
	}
}

short network_link_door(
	struct network_game const *game,
	long machine_index,
	unsigned long real_address,
	boolean local)
{
	char const *policy = config_string("server.vpn_policy");
	boolean shown, checked, listed;
	char by[64] = "", why[200] = "";

	if (!halo_server_mode() || local || machine_index < 0 || machine_index >= HALO_PORT_MAXIMUM_NETWORK_MACHINES)
		return NONE;
	shown = player_link_pass_shown(network_link_machines[machine_index].pass) != 0;
	checked = network_link_machines[machine_index].verified;
	if (shown && !checked)
		snprintf(why, sizeof(why), "%s", network_link_machines[machine_index].why);
	if (!strcmp(policy, "off") || !real_address)
		return NONE;
	/* (only an address the internet routes: a LAN's machines are not
	checked, but for a test) */
	if (!network_link_address_global(real_address) && !config_boolean("server.vpn_check_lan"))
		return NONE;
	/* (vpn_allow.txt's ranges first, then vpn_block.txt's and the lists'; with
	no list held yet, server.vpn_fail closed refuses every address the
	internet routes but the allowed) */
	listed = server_netlist_listed(real_address, by, sizeof(by)) != 0;
	if (!listed && strcmp(by, "vpn_allow.txt") && !server_netlist_held() &&
		!strcmp(config_string("server.vpn_fail"), "closed"))
	{
		snprintf(why, sizeof(why), "no netlist is held, and server.vpn_fail is closed");
		return network_link_refuse(machine_index, real_address, _rejection_code_vpn_blocked, "vpn_blocked", why);
	}
	if (!listed)
		return NONE;
	if (!strcmp(policy, "block"))
	{
		snprintf(why, sizeof(why), "its address is listed (%s), and server.vpn_policy is block", by);
		return network_link_refuse(machine_index, real_address, _rejection_code_vpn_blocked, "vpn_blocked", why);
	}
	if (!shown)
	{
		snprintf(why, sizeof(why), "its address is listed (%s), and it shows no Discord account", by);
		return network_link_refuse(machine_index, real_address, _rejection_code_vpn_link_required, "vpn_link_required",
			why);
	}
	if (!checked)
		return network_link_refuse(machine_index, real_address, _rejection_code_vpn_link_invalid, "vpn_link_invalid",
			why);
	{
		long days = config_integer("server.link_min_age_days");
		unsigned long now = (unsigned long)time(NULL);

		if (days > 0 && network_link_machines[machine_index].created + (unsigned long)days * 86400UL > now)
		{
			snprintf(why, sizeof(why), "the Discord account %s is %lu days old, younger than server.link_min_age_days "
				"(%ld)", network_link_machines[machine_index].discord_id,
				now > network_link_machines[machine_index].created ?
					(now - network_link_machines[machine_index].created) / 86400UL : 0, days);
			return network_link_refuse(machine_index, real_address, _rejection_code_vpn_link_too_new,
				"vpn_link_too_new", why);
		}
	}
	if (config_boolean("server.link_one_seat") && game)
	{
		long other;

		for (other = 0; other < HALO_PORT_MAXIMUM_NETWORK_MACHINES; other++)
		{
			if (other != machine_index && network_link_machines[other].verified &&
				game->machines[other].machine_index >= 0 &&
				game->machines[other].machine_index < HALO_PORT_MAXIMUM_NETWORK_MACHINES &&
				!strcmp(network_link_machines[other].discord_id, network_link_machines[machine_index].discord_id))
			{
				snprintf(why, sizeof(why), "the Discord account %s is in the game on another machine",
					network_link_machines[machine_index].discord_id);
				return network_link_refuse(machine_index, real_address, _rejection_code_vpn_link_in_use,
					"vpn_link_in_use", why);
			}
		}
	}
	return NONE;
}

short network_link_address_limit(
	struct network_game const *game,
	long machine_index,
	unsigned long real_address,
	boolean local)
{
	long limit = config_integer("server.max_machines_per_address");
	long other, count = 0;

	if (!halo_server_mode() || local || limit <= 0 || !real_address || !game)
		return NONE;
	/* (the machines in the game from the same real address: an internet play
	peer's endpoint, a LAN's address) */
	for (other = 0; other < game->machine_count && other < HALO_PORT_MAXIMUM_NETWORK_MACHINES; other++)
	{
		long slot = game->machines[other].machine_index;

		if (slot >= 0 && slot < HALO_PORT_MAXIMUM_NETWORK_MACHINES && slot != machine_index &&
			distributed_machine_real_address(slot) == real_address)
		{
			count++;
		}
	}
	if (count < limit)
		return NONE;
	{
		char why[96];

		snprintf(why, sizeof(why), "%ld machines of its address play already (server.max_machines_per_address)", count);
		network_link_refuse(machine_index, real_address, _rejection_code_address_full, "address_full", why);
	}
	return _rejection_code_address_full;
}
