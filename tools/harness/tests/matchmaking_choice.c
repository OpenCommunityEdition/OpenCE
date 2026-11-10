/*
MATCHMAKING_CHOICE.C (test)

The MATCHMAKING screen's rules (menu_functions.c).
*/

#include "harness.h"
#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("ctf_only")
	{
		CHECK(!mm_playlist_has_size(1u << P2P_MM_CTF, P2P_MM_FFA_6), "E62: only CTF: FFA 6 greyed");
		CHECK(mm_playlist_has_size(1u << P2P_MM_CTF, P2P_MM_4V4), "E62: team sizes stay");
		CHECK(mm_playlist_has_size(1u << P2P_MM_CTF | 1u << P2P_MM_SLAYER, P2P_MM_FFA_6),
			"E62: with Slayer, FFA 6 is offered");
		CHECK(matchmaking_size_after(1u << P2P_MM_CTF, P2P_MM_FFA_6) == P2P_MM_2V2, "E62: the set size moves to 2V2");
		CHECK(matchmaking_size_after(1u << P2P_MM_CTF, P2P_MM_3V3) == P2P_MM_3V3, "an allowed size stays");
		CHECK(mm_playlist_has_size(0, P2P_MM_FFA_6), "no playlist: FFA 6 not greyed");
		CHECK(matchmaking_size_after(0, P2P_MM_FFA_6) == P2P_MM_FFA_6, "no playlist: FFA 6 kept");
		CHECK(mm_playlist_has_size(1u << P2P_MM_CTF | 1u << P2P_MM_KING, P2P_MM_FFA_6), "CTF and KING: FFA 6 offered");
	}
	return 0;
}
