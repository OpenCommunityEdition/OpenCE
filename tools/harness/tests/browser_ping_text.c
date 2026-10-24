/*
BROWSER_PING_TEXT.C (test)

The server browser's ping cell (menu_functions.c's browser_ping_text) for a
game open ('o'), closed ('c'), live ('l') or failed ('f'), from
test_browser_ping_text.py's listing.inc and under_test.inc.
*/

#include "harness.h"
#include "listing.inc"

#include <wchar.h>

#define usnprintf(buffer, size, ...) swprintf(buffer, size, __VA_ARGS__)

#include "under_test.inc"

static int shows(char state, int ping, int probing, unsigned long now, wchar_t const *expected)
{
	struct p2p_listing game;
	wchar_t text[16];

	memset(&game, 0, sizeof(game));
	game.open = state != 'c';
	game.in_progress = state == 'l';
	game.failed = state == 'f';
	game.ping = (short)ping;
	game.probing = (unsigned char)probing;
	return !wcscmp(browser_ping_text(&game, now, text, 16), expected);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("number")
		CHECK(shows('o', 42, 0, 0, L"42") && shows('o', 42, 1, 700, L"42"), "a ping shows, kept while timed again");
	else CASE("dots")
	{
		CHECK(shows('o', -1, 1, 299, L".") && shows('o', -1, 1, 300, L"..") && shows('o', -1, 1, 600, L"...") &&
			shows('o', -1, 1, 900, L"."), "the dots step every 300 ms");
		CHECK(shows('c', -1, 1, 300, L"..") && shows('l', -1, 1, 300, L".."), "a closed or live game timed: dots");
	}
	else CASE("status")
		CHECK(shows('c', -1, 0, 0, L"CLOSED") && shows('l', -1, 0, 0, L"LIVE") && shows('f', 42, 1, 0, L"FAILED") &&
			shows('f', -1, 1, 0, L"FAILED"), "CLOSED or LIVE when not timed, FAILED over a ping or dots");
	else CASE("dash")
		CHECK(shows('o', -1, 0, 300, L"-"), "an open game not timed shows a dash");
	else return 2;
	return 0;
}
