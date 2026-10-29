/*
NETWORK_LINK.H

A dedicated server's door for VPNs, and the Discord accounts its machines
show (network_link.c; docs/dedicated_server.md, "VPNs and linked Discord
accounts").
*/

#ifndef __HALO_LINUX_NETWORK_LINK_H
#define __HALO_LINUX_NETWORK_LINK_H

struct network_game;

/* (the host: network_server_message_handler.c) a joining machine's join
request: the pass and proof it showed (all zero: none) and its name */
void network_link_set_machine_pass(
	long machine_index,
	void const *pass,
	void const *proof,
	char const *name);

/* (the host: network_server_manager.c, before a machine is accepted, and
before its bans are looked at) its pass checked, if it shows one, for its
address as it joined (host byte order; an internet play peer's stand-in):
the account it shows taken as the machine's if it checks. Only a dedicated
server's */
void network_link_check_pass(
	long machine_index,
	unsigned long address);

/* (the host: then, after its bans) the VPN policy (server.vpn_policy) for
its real address (host byte order): the rejection code that refuses it, or
NONE. A machine of the host's own, or any in a game a player hosts, is let
in */
short network_link_door(
	struct network_game const *game,
	long machine_index,
	unsigned long real_address,
	boolean local);

/* (the host: then) a dedicated server's limit of machines from one real
address (server.max_machines_per_address; 0: none): the rejection code that
refuses the machine, or NONE */
short network_link_address_limit(
	struct network_game const *game,
	long machine_index,
	unsigned long real_address,
	boolean local);

/* whether the machine showed a pass that checked: its account's id (decimal
text) and username */
boolean network_link_machine_verified(
	long machine_index,
	char *discord_id,
	long discord_id_size,
	char *discord_name,
	long discord_name_size);

#endif
