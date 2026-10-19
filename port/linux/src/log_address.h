/*
LOG_ADDRESS.H

The one way an IP address is written into the log (debug.txt, the terminal,
the console, a dedicated server's journal): players post debug.txt in
public, and a host's log names everyone who joined it. So a public address
is never written whole, only as a tag, "addr#3f2a9c", the same for the same
address all run long (a hash of it with a secret random salt of the run's),
so a log's lines can still be matched up, but one run's tags say nothing of
another's, and nothing of the address. The port follows as it is
(":51234"): it is not who someone is, and it is what NAT debugging needs.

Addresses that are no one's on the internet are written as they are:
loopback, unspecified, the private ranges (10/8, 172.16/12, 192.168/16),
link-local (169.254/16), 100.64/10 (carrier NAT, and internet play's
stand-in addresses of its peers, xnet.c), and IPv6's ::1, ::, fc00::/7 and
fe80::/10.

debug.log_addresses (config.toml) writes every address whole, for a
developer's own debugging; a log written that way is not one to post.

Anything new that logs an address goes through log_address: never a
"%u.%u.%u.%u" of its own.
*/

#ifndef LOG_ADDRESS_H
#define LOG_ADDRESS_H

enum
{
	/* the longest text log_address writes, with its terminator ("[" an IPv6
	address "]:65535") */
	LOG_ADDRESS_SIZE = 56,
};

/* an address as the log may show it (above): length 4 (IPv4) or 16 (IPv6)
bytes, first byte first (the order an address is written in, network byte
order); port in host byte order, or negative for none. Writes text (size
bytes, LOG_ADDRESS_SIZE enough for any) and returns it */
const char *log_address(const unsigned char *bytes, int length, int port, char *text, int size);

/* the same for an IPv4 address in network byte order (a sockaddr_in's), and
a port in network byte order (0: none) */
const char *log_address_ipv4(unsigned long network_address, unsigned short network_port, char *text, int size);

#endif
