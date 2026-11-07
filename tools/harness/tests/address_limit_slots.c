/*
ADDRESS_LIMIT_SLOTS.C (test)

The real network_link_address_limit (network_link.c), a dedicated server's
server.max_machines_per_address, over a game whose machines are in the
slots of their indices (network_game_add_machine), with gaps where machines
left: a machine joining from an address that has the limit's machines in the
game already is refused, wherever in the slots they are. The first
machine_count slots alone missed a machine in a slot after a gap, and a
second machine of one address got in once a slot before it was free.
test_address_limit_slots.py takes the structure, constants and code from the
sources (config.inc, under_test.inc).
*/

#include <stddef.h>
#include "harness.h"

#include "config.inc"

/* the game: its machines in the slots of their indices, NONE for none */
struct network_game
{
	short machine_count;
	struct network_machine machines[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
};
static struct network_game game;
static unsigned long addresses[HALO_PORT_MAXIMUM_NETWORK_MACHINES];

static unsigned long distributed_machine_real_address(long machine_index)
{
	CHECK(machine_index >= 0 && machine_index < HALO_PORT_MAXIMUM_NETWORK_MACHINES, "no machine %ld", machine_index);
	return addresses[machine_index];
}

/* the server */
static long limit;
static boolean server_mode = TRUE;
static int halo_server_mode(void) { return server_mode; }
static long config_integer(char const *name)
{
	CHECK(!strcmp(name, "server.max_machines_per_address"), "the setting %s read", name);
	return limit;
}

/* the refusals told */
static int refusals;
static short network_link_refuse(long machine_index, unsigned long real_address, short code, char const *name,
	char const *why)
{
	refusals++;
	return code;
}

#include "under_test.inc"

static void reset(long maximum)
{
	long slot;

	memset(&game, 0, sizeof(game));
	for (slot = 0; slot < HALO_PORT_MAXIMUM_NETWORK_MACHINES; slot++)
		game.machines[slot].machine_index = NONE;
	memset(addresses, 0, sizeof(addresses));
	limit = maximum;
	server_mode = TRUE;
	refusals = 0;
}

/* a machine joins the game in its slot, or leaves it */
static void join(long slot, unsigned long address)
{
	game.machines[slot].machine_index = (char)slot;
	addresses[slot] = address;
	game.machine_count++;
}

static void leave(long slot)
{
	game.machines[slot].machine_index = NONE;
	game.machine_count--;
}

#define ADDRESS_X 0x0A000001UL
#define ADDRESS_Y 0x0A000002UL

static boolean refused(long machine_index, unsigned long address, boolean local)
{
	addresses[machine_index] = address;
	return network_link_address_limit(&game, machine_index, address, local) == _rejection_code_address_full;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* one machine an address: the first of address X in slot 1, slot 0's
	machine gone, and a second of X takes slot 0 */
	CASE("gap")
	{
		reset(1);
		join(0, ADDRESS_Y);
		join(1, ADDRESS_X);
		leave(0);
		CHECK(refused(0, ADDRESS_X, FALSE), "a second machine of one address joined in the slot before the first's");
		CHECK(refusals == 1, "the refusal was not told");
		CHECK(!refused(0, ADDRESS_Y, FALSE), "a machine of another address refused");
		return 0;
	}
	/* two machines an address, in the game's far slots */
	CASE("far-slots")
	{
		long slot;

		reset(2);
		for (slot = 0; slot < 40; slot++)
			join(slot, ADDRESS_Y);
		for (slot = 0; slot < 40; slot++)
			leave(slot);
		join(90, ADDRESS_X);
		CHECK(!refused(3, ADDRESS_X, FALSE), "the second machine of an address refused, under the limit");
		join(3, ADDRESS_X);
		CHECK(refused(0, ADDRESS_X, FALSE), "a third machine of one address joined, the others in slots 3 and 90");
		join(HALO_PORT_MAXIMUM_NETWORK_MACHINES - 1, ADDRESS_Y);
		CHECK(!refused(5, ADDRESS_Y, FALSE), "another address's machine refused for X's");
		return 0;
	}
	/* a machine's own slot is not another machine of its address */
	CASE("self")
	{
		reset(1);
		join(2, ADDRESS_X);
		CHECK(!refused(2, ADDRESS_X, FALSE), "a machine counted itself");
		return 0;
	}
	/* none refused: no limit, a local machine, not a dedicated server */
	CASE("unlimited")
	{
		reset(0);
		join(1, ADDRESS_X);
		CHECK(!refused(0, ADDRESS_X, FALSE), "refused with no limit");
		limit = 1;
		CHECK(!refused(0, ADDRESS_X, TRUE), "a local machine refused");
		server_mode = FALSE;
		CHECK(!refused(0, ADDRESS_X, FALSE), "refused by a game that is not a dedicated server's");
		CHECK(!refusals, "a refusal told");
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
