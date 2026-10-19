/*
BYTE_ORDER_PORTS.C (test)

internet play converts ports and addresses between the host's order and the network's in several files, each with its
own small function. Each swapped unconditionally, which is right on every little-endian build and wrong on the Mac's
PowerPC (big-endian): the signalling broker's port 1883 became 23303 there, no broker was ever reached and a joining
G4 found "no answer from the invite's host". The functions are taken from the sources and run as this host (little
endian) and as a big-endian machine would run them (the byte that tells the order read from the other end).
*/

#include "harness.h"
#include "snippets.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("signal-port")
	{
		CHECK(signal_network_short(1883) == EXPECT_SHORT(1883), "got %04x", signal_network_short(1883));
	}
	CASE("upnp-port")
	{
		CHECK(upnp_swap_short(1883) == EXPECT_SHORT(1883), "got %04x", upnp_swap_short(1883));
	}
	CASE("p2p-short")
	{
		CHECK(p2p_network_short(3478) == EXPECT_SHORT(3478), "got %04x", p2p_network_short(3478));
	}
	CASE("p2p-long")
	{
		CHECK(p2p_network_long(0x7F000001UL) == EXPECT_LONG(0x7F000001UL), "got %08lx", p2p_network_long(0x7F000001UL));
	}

	return 0;
}
