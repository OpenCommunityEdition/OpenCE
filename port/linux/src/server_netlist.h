/*
SERVER_NETLIST.H

A dedicated server's netlists (server_netlist.c; docs/master_protocol.md,
"The netlist"; docs/dedicated_server.md, "VPNs and linked Discord
accounts"): the IPv4 ranges of VPNs, Tor exits and datacenters each master
it publishes to signs, fetched every hour on a thread of their own and kept
on disk beside bans.txt (netlist-<master key id>.bin), or server.vpn_list's
file instead; vpn_allow.txt's ranges, never refused, and vpn_block.txt's,
refused besides. The game's side (network_link.c) asks whether a joining
machine's address is listed, and counts the joins it refused.
*/

#ifndef __HALO_LINUX_SERVER_NETLIST_H
#define __HALO_LINUX_SERVER_NETLIST_H

struct server_json;

enum
{
	SERVER_NETLIST_HEADER_SIZE = 32,
	SERVER_NETLIST_MAXIMUM_RANGES = 1048576,
};

/* ---------- the format (pure: the harness tests them) */

/* whether the body reads as a netlist (its magic, format, reserved bytes,
size, and ranges ascending, neither overlapping nor touching): 1, with its
generated time, serial and count of ranges; else 0 and why */
int server_netlist_read(const unsigned char *body, unsigned long size, unsigned long long *generated,
	unsigned long long *serial, unsigned long *count, char *why, int why_size);
/* whether the address (host byte order) is in one of a netlist's ranges
(its bytes after the header, count of them) */
int server_netlist_ranges_contain(const unsigned char *ranges, unsigned long count, unsigned long address);
/* the ranges of a text (vpn_allow.txt's): "203.0.113.0/24", "198.51.100.7",
"198.51.100.1-198.51.100.9", one a line, "#" a comment; firsts and lasts
(host byte order) in pairs, merged, up to maximum ranges: how many. A line
that is none is counted in bad */
unsigned long server_netlist_parse_ranges(const char *text, unsigned long *pairs, unsigned long maximum, int *bad);

/* ---------- the server's */

/* the lists held read from disk (and server.vpn_list's, the allowed and
blocked ranges' files), and fetched every hour from the masters the server
publishes to; nothing but in a dedicated server */
void server_netlist_start(void);
/* sv_reload: server.vpn_list's file, vpn_allow.txt and vpn_block.txt read
again */
void server_netlist_reload(void);
/* sv_vpn_fetch: the masters asked now */
void server_netlist_fetch_now(void);
/* whether a list is held (a master's, or server.vpn_list's) */
int server_netlist_held(void);
/* whether the address (host byte order) is listed: 1, by "netlist" or
"vpn_block.txt"; 0, by "vpn_allow.txt" when it is allowed, else "" */
int server_netlist_listed(unsigned long address, char *by, int by_size);
/* a join refused for the policy (its reason, "vpn_link_required"...),
counted for /api/vpn */
void server_netlist_count_refusal(const char *reason);
/* /api/vpn's members: policy, fail, lists[], file, allow_ranges,
block_ranges, held, refused{} */
void server_netlist_status(struct server_json *json);
/* sv_vpn's lines, into text */
void server_netlist_describe(char *text, int size);

#endif
