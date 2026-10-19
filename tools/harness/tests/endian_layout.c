/*
ENDIAN_LAYOUT.C (test)

Two places where the game's code assumed a little-endian layout: it reached into a value through a pointer to a part of
it. On the Mac's PowerPC (big-endian) each read the wrong part: a contrail's end points were not faded (byte 3 of a
pixel32 is blue there, not alpha) and the idle speech's variation was lost (a short * to a long is its high half). The
statements are taken from the sources; test_endian_layout.py compiles them as they are (this host's layout) and as a
big-endian machine would run them: the byte and short accesses through pointers rewritten to read the other end of the
value. (The material effect's long over a short and its pad is replaced on the Mac by tools/mac_material_effects_view.py.)
*/

#include "harness.h"
#include "snippets.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	/* the first and last points' alpha is cleared; their colour is not touched */
	CASE("contrail-alpha")
	{
		struct contrail_vertex vertices[4];
		int i;

		for (i = 0; i < 4; i++)
			vertices[i].color = 0xFF102030u;
		contrail_fade(&vertices[0], &vertices[0], 4, 0);
		CHECK(vertices[0].color == 0x00102030u, "first %08x", vertices[0].color);
		CHECK(vertices[1].color == 0x00102030u, "second %08x", vertices[1].color);
		CHECK(vertices[2].color == 0x00102030u, "second last %08x", vertices[2].color);
		CHECK(vertices[3].color == 0x00102030u, "last %08x", vertices[3].color);
		for (i = 0; i < 4; i++)
			vertices[i].color = 0xFF102030u;
		contrail_fade(&vertices[0], &vertices[0], 4, 3);
		CHECK(vertices[0].color == 0xFF102030u && vertices[3].color == 0xFF102030u, "unfaded ends changed");
	}

	/* the combat idle variation reaches unit_test_speech as the short it reads */
	CASE("idle-vocalization")
	{
		CHECK(idle_vocalization(TRUE) == 1, "combat idle read %d", idle_vocalization(TRUE));
		CHECK(idle_vocalization(FALSE) == 0, "calm idle read %d", idle_vocalization(FALSE));
	}

	return 0;
}
