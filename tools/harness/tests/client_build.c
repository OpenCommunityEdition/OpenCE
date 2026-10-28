/*
CLIENT_BUILD.C (test)

What a host keeps of a client's build as the client tells it
(port/linux/game/network_distributed.c's distributed_build_text): another
machine's text, so printable ASCII alone, line ends only where a field has
lines (the mods), within its size, ended, its trailing spaces and line ends
left out. test_client_build.py takes the code (under_test.inc).
*/

#include "harness.h"

#include "under_test.inc"

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	char text[16];

	/* printable ASCII is kept as it is, the rest left out */
	CASE("printable")
	{
		distributed_build_text(text, sizeof(text), "OpenCE | b\x01\x7f\xe9!", FALSE);
		CHECK(!strcmp(text, "OpenCE | b!"), "'%s'", text);
	}

	/* line ends: kept in a field of lines, else left out; none trailing */
	CASE("lines")
	{
		distributed_build_text(text, sizeof(text), "a 1.0\nb\r\n\n", TRUE);
		CHECK(!strcmp(text, "a 1.0\nb"), "'%s'", text);
		distributed_build_text(text, sizeof(text), "a\nb  ", FALSE);
		CHECK(!strcmp(text, "ab"), "'%s'", text);
	}

	/* within its size, and ended, however long what is told */
	CASE("bounds")
	{
		text[sizeof(text) - 1] = 'x';
		distributed_build_text(text, sizeof(text), "0123456789abcdefghijklmnop", FALSE);
		CHECK(strlen(text) == sizeof(text) - 1 && !strcmp(text, "0123456789abcde"), "'%s'", text);
		distributed_build_text(text, 1, "abc", FALSE);
		CHECK(text[0] == 0, "nothing fits, yet '%s'", text);
	}

	return 0;
}
