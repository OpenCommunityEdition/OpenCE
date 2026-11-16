/*
MATCHMAKING_SEARCH_PROFILE.C (test)

SEARCH with player 1's profile (menu_functions.c).
*/

#include <wchar.h>
#include "harness.h"

#define MAXIMUM_LOCAL_PLAYERS 4
#define SOUND_ERROR 4
#define SOUND_FORWARD 2
#define NUMBEROF(array) ((short)(sizeof(array) / sizeof((array)[0])))

struct player_profile { wchar_t player_name[12]; };
struct p2p_mm_choice { unsigned playlists; int size; };

static struct
{
	unsigned playlists;
	short size;
	short refusal;
} matchmaking_screen;

static boolean has_profile, name_valid;
static int searches, sounds, last_sound;
static wchar_t const *error_text;

static boolean player_ui_local_player_wants_to_play_multiplayer(short controller)
{
	return controller == 0;
}

static boolean campaign_profile(short controller, struct player_profile *profile)
{
	wcscpy(profile->player_name, L"Chief");
	return has_profile;
}

static boolean player_name_valid(wchar_t const *name, long count)
{
	return name_valid;
}

static void display_error_text_deferred(wchar_t const *text, short controller)
{
	error_text = text;
}

static void ui_play_audio_feedback_sound(short sound)
{
	sounds++;
	last_sound = sound;
}

static void p2p_mm_search(const struct p2p_mm_choice *choice)
{
	searches++;
}

#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	matchmaking_screen.playlists = 1;
	has_profile = name_valid = TRUE;
	CASE("invalid_name_refused")
	{
		name_valid = FALSE;
		CHECK(!matchmaking_screen_search(), "SEARCH refused");
		CHECK(searches == 0, "no search begins, not %d", searches);
		CHECK(error_text && wcsstr(error_text, L"can't be used in"), "the profile's error shows");
		CHECK(sounds == 1 && last_sound == SOUND_ERROR, "the error sound once, not %d sounds (last %d)", sounds,
			last_sound);
	}
	CASE("no_profile_refused")
	{
		has_profile = FALSE;
		CHECK(!matchmaking_screen_search(), "SEARCH refused");
		CHECK(searches == 0, "no search begins, not %d", searches);
		CHECK(sounds == 1 && last_sound == SOUND_ERROR, "the error sound once, not %d sounds (last %d)", sounds,
			last_sound);
	}
	CASE("valid_name_searches")
	{
		CHECK(matchmaking_screen_search(), "SEARCH accepted");
		CHECK(searches == 1, "the search begins once, not %d times", searches);
		CHECK(!error_text, "no error");
		CHECK(last_sound == SOUND_FORWARD, "the forward sound, not %d", last_sound);
	}
	return 0;
}
