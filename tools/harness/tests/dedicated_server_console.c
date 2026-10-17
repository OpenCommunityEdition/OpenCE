/*
DEDICATED_SERVER_CONSOLE.C (test)

What a dedicated server takes from its console and from the network
(port/linux/game/dedicated_server.c): a command's words and text kept to
printable ASCII and their buffers, a map's name that is never a path, an
address that is exactly a.b.c.d, the connections an address may make a
minute (an internet play peer's by its real address), and a password's
command never logged. test_dedicated_server_console.py takes the constants
and structures (config.inc) and the code (under_test.inc).
*/

#include "harness.h"
#include "config.inc"

#include <stdarg.h>
#include <string.h>
#include <strings.h>

#define csstrlen(string) strlen(string)
#define csstrcpy(destination, source) strcpy(destination, source)
#define csstrcmp(first, second) strcmp(first, second)
#define stristr(haystack, needle) strcasestr(haystack, needle)
#define NUMBEROF(array) (sizeof(array) / sizeof((array)[0]))

/* the clock (seconds), as time() would read it */
static unsigned long fake_now = 1000000;
#define time(pointer) ((time_t)fake_now)

/* the server's setting (server.dedicated) */
static int config_boolean(char const *name)
{
	(void)name;
	return 1;
}

/* the log: everything platform_log wrote */
static char logged[8192];

static void platform_log(char const *format, ...)
{
	va_list arguments;
	size_t length = strlen(logged);

	va_start(arguments, format);
	vsnprintf(logged + length, sizeof(logged) - length, format, arguments);
	va_end(arguments);
	length = strlen(logged);
	if (length + 1 < sizeof(logged))
		strcat(logged, "\n");
}

/* internet play's peers: a stand-in (network byte order) and its real
address (network byte order), 0 if none */
static unsigned long fake_stand_in[4], fake_real[4];

static unsigned long p2p_peer_endpoint_address(unsigned long virtual_address)
{
	int index;

	for (index = 0; index < 4; index++)
		if (fake_stand_in[index] && fake_stand_in[index] == virtual_address)
			return fake_real[index];
	return 0;
}

static unsigned long network_order(unsigned long address)
{
	return (address >> 24) | ((address >> 8) & 0xFF00) | ((address << 8) & 0xFF0000) | (address << 24);
}

/* the server's state the code reads */
static struct
{
	long active;
	struct join_address joins[MAXIMUM_JOIN_ADDRESSES];
} dedicated = { -1 };

#include "under_test.inc"

#define ADDRESS(a, b, c, d) ((unsigned long)(a) << 24 | (unsigned long)(b) << 16 | (unsigned long)(c) << 8 | (d))

/* the connections an address may make now, counted to the first refusal
(at most 100) */
static int joins_allowed(unsigned long address)
{
	int count = 0;

	while (count < 100 && dedicated_server_join_allowed(address))
		count++;
	return count;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* printable ASCII only, cut to the buffer with its end */
	CASE("printable")
	{
		char text[8];

		memset(text, 'x', sizeof(text));
		printable(text, sizeof(text), "a\tb\x1b[2Jc\xc3\xa9\x7f" "d");
		CHECK(!strcmp(text, "ab[2Jcd"), "kept \"%s\"", text);
		printable(text, sizeof(text), "0123456789");
		CHECK(!strcmp(text, "0123456"), "a long text kept as \"%s\"", text);
		printable(text, 1, "abc");
		CHECK(!text[0], "a buffer of one kept \"%s\"", text);
		printable(text, sizeof(text), NULL);
		CHECK(!text[0], "no text kept \"%s\"", text);
	}

	/* a command's words: spaces between them, a quoted one whole, no more
	than there is room for */
	CASE("split")
	{
		char line[COMMAND_SIZE];
		char *words[MAXIMUM_ARGUMENTS + 1];
		long count;
		long index;

		strcpy(line, "  sv_mapcycle_add\tbloodgulch   \"team slayer\"  ");
		count = split(line, words, MAXIMUM_ARGUMENTS);
		CHECK(count == 3, "%ld words", count);
		CHECK(!strcmp(words[0], "sv_mapcycle_add") && !strcmp(words[1], "bloodgulch") &&
			!strcmp(words[2], "team slayer"), "words \"%s\" \"%s\" \"%s\"", words[0], words[1], words[2]);
		strcpy(line, "\"unterminated quote");
		count = split(line, words, MAXIMUM_ARGUMENTS);
		CHECK(count == 1 && !strcmp(words[0], "unterminated quote"), "an open quote: %ld words", count);
		/* (more words than room: the rest stay in the last one's line) */
		line[0] = 0;
		for (index = 0; index < MAXIMUM_ARGUMENTS + 4; index++)
			strcat(line, "w ");
		words[MAXIMUM_ARGUMENTS] = (char *)"untouched";
		count = split(line, words, MAXIMUM_ARGUMENTS);
		CHECK(count == MAXIMUM_ARGUMENTS, "%ld words of %d at most", count, MAXIMUM_ARGUMENTS);
		CHECK(!strcmp(words[MAXIMUM_ARGUMENTS], "untouched"), "a word written past the array");
		strcpy(line, " \t ");
		CHECK(split(line, words, MAXIMUM_ARGUMENTS) == 0, "blanks made a word");
	}

	/* the text after the command (sv_name's, sv_password's), its quotes
	taken off, cut to the buffer */
	CASE("text")
	{
		char text[16];

		unquoted(text, sizeof(text), rest_of_line("sv_name   \"My Server\"  "));
		CHECK(!strcmp(text, "My Server"), "a quoted name kept as \"%s\"", text);
		unquoted(text, sizeof(text), rest_of_line("sv_name plain words"));
		CHECK(!strcmp(text, "plain words"), "a name kept as \"%s\"", text);
		unquoted(text, sizeof(text), rest_of_line("sv_password"));
		CHECK(!text[0], "no password kept as \"%s\"", text);
		unquoted(text, sizeof(text), rest_of_line("sv_name \"a very long name that does not fit\""));
		CHECK(strlen(text) == sizeof(text) - 1, "a long name kept as \"%s\"", text);
		unquoted(text, sizeof(text), rest_of_line("sv_name \"tab\there\""));
		CHECK(!strcmp(text, "tabhere"), "a control character kept: \"%s\"", text);
	}

	/* a map's name: letters, digits, "_" and "-", never a path */
	CASE("map-names")
	{
		static char const *const refused[] =
		{
			"", "../bloodgulch", "levels\\test\\bloodgulch", "/etc/passwd", "blood gulch", "map.map", "c:map",
			"0123456789012345678901234567890123",
		};
		size_t index;

		CHECK(map_name_valid("bloodgulch"), "bloodgulch refused");
		CHECK(map_name_valid("My_Map-2"), "My_Map-2 refused");
		CHECK(map_name_valid("0123456789012345678901234567890"), "a name of 31 characters refused");
		for (index = 0; index < NUMBEROF(refused); index++)
			CHECK(!map_name_valid(refused[index]), "\"%s\" taken for a map's name", refused[index]);
	}

	/* an address: exactly four numbers of 0 to 255 */
	CASE("addresses")
	{
		static char const *const refused[] =
		{
			"", "1.2.3", "1.2.3.4.5", "256.1.1.1", "1.2.3.4 ", " 1.2.3.4", "1..2.3", "1234.1.1.1", "a.b.c.d",
			"1.2.3.-4", "1.2.3.4/8",
		};
		unsigned long address = 0;
		size_t index;
		char text[32];

		CHECK(parse_address("192.168.0.20", &address) && address == ADDRESS(192, 168, 0, 20), "192.168.0.20 read as %lx",
			address);
		CHECK(parse_address("0.0.0.0", &address) && address == 0, "0.0.0.0 refused");
		CHECK(parse_address("255.255.255.255", &address) && address == 0xFFFFFFFFUL, "255.255.255.255 refused");
		for (index = 0; index < NUMBEROF(refused); index++)
			CHECK(!parse_address(refused[index], &address), "\"%s\" taken for an address", refused[index]);
		address_text(ADDRESS(10, 0, 0, 7), text, sizeof(text));
		CHECK(!strcmp(text, "10.0.0.7"), "10.0.0.7 written as %s", text);
	}

	/* so many connections a minute from an address, then one again each
	JOIN_REFILL_SECONDS; another address has its own */
	CASE("join-limit")
	{
		unsigned long first = ADDRESS(203, 0, 113, 5);
		unsigned long second = ADDRESS(203, 0, 113, 6);

		CHECK(joins_allowed(first) == DEDICATED_JOINS_PER_MINUTE, "%d connections at once, not %d",
			joins_allowed(first), DEDICATED_JOINS_PER_MINUTE);
		CHECK(strstr(logged, "203.0.113.5 connects too often"), "a refusal not logged: %s", logged);
		CHECK(joins_allowed(second) == DEDICATED_JOINS_PER_MINUTE, "another address took the first's refusal");
		fake_now += JOIN_REFILL_SECONDS - 1;
		CHECK(!dedicated_server_join_allowed(first), "a connection came back early");
		fake_now += 1;
		CHECK(joins_allowed(first) == 1, "%d came back after %d seconds, not 1", joins_allowed(first),
			JOIN_REFILL_SECONDS);
		fake_now += 3600;
		CHECK(joins_allowed(first) == DEDICATED_JOINS_PER_MINUTE, "the whole allowance not back after an hour");
		/* (the host's own client, an address of 0, is never counted) */
		CHECK(joins_allowed(0) == 100, "address 0 counted");
	}

	/* an internet play peer's connections come from its stand-in
	(100.64.0.0/10): counted by its real address, so two stand-ins of one
	machine share one allowance */
	CASE("join-limit-peers")
	{
		unsigned long real = ADDRESS(198, 51, 100, 9);
		unsigned long stand_in = ADDRESS(100, 64, 1, 2);
		unsigned long other_stand_in = ADDRESS(100, 64, 7, 8);
		int count = 0;

		fake_stand_in[0] = network_order(stand_in);
		fake_real[0] = network_order(real);
		fake_stand_in[1] = network_order(other_stand_in);
		fake_real[1] = network_order(real);
		while (count < 100 && dedicated_server_join_allowed(count % 2 ? stand_in : other_stand_in))
			count++;
		CHECK(count == DEDICATED_JOINS_PER_MINUTE, "two stand-ins of one address made %d connections, not %d", count,
			DEDICATED_JOINS_PER_MINUTE);
		CHECK(!dedicated_server_join_allowed(real), "the real address was not counted with its stand-ins");
	}

	/* a table full of addresses makes room for a new one, which gets its
	whole allowance */
	CASE("join-limit-full")
	{
		unsigned long address;
		int count;

		for (address = 1; address <= MAXIMUM_JOIN_ADDRESSES; address++)
		{
			fake_now++;
			CHECK(joins_allowed(ADDRESS(10, 1, 0, 0) + address) == DEDICATED_JOINS_PER_MINUTE, "address %lu limited",
				address);
		}
		fake_now++;
		count = joins_allowed(ADDRESS(10, 2, 0, 1));
		CHECK(count == DEDICATED_JOINS_PER_MINUTE, "a new address in a full table made %d connections", count);
	}

	/* the console's commands are logged, a password's without the rest */
	CASE("password-not-logged")
	{
		logged[0] = 0;
		log_command("sv_name", "sv_name \"My Server\"");
		CHECK(strstr(logged, "My Server"), "sv_name not logged: %s", logged);
		log_command("sv_password", "sv_password hunter2");
		log_command("SV_PASSWORD", "SV_PASSWORD hunter3");
		log_command("sv_rcon_password", "sv_rcon_password hunter4");
		CHECK(!strstr(logged, "hunter"), "a password logged: %s", logged);
		CHECK(strstr(logged, "sv_password (the rest not shown)"), "sv_password not logged at all: %s", logged);
	}

	return 0;
}
