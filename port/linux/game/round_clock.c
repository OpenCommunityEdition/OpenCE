/*
ROUND_CLOCK.C

The clock of the game running, told to the players where the game's own is
not: its time limit is behind the scores' button, and a player watching the
game has no other way to know how much of it is left.

In the lobby, before a game, it is the countdown the server sends that is
told: the one the game will start on, so that a player who has just joined
knows the game is counting down rather than waiting for someone.

The notice goes in the chat area of every machine's own players, every
minute of the last five and each of the last seconds, and in the lobby at
thirty, twenty, ten and the last five. Each machine computes it from the
game's options, its clock and the countdown it was sent, so nothing new goes
between them and a machine of another build shows it too. Displayed by
display.time_notices (off: the stock game's screens alone). Called every
frame from main.c, before the frame is drawn.
*/

#include "cseries.h"
#include <stdio.h>
#include "round_clock.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "networking/network_game_globals.h"
#include "networking/network_client_manager.h"
#include "network_chat.h"

/* the platform layer's (port/linux/src/port_config.c) */
boolean config_boolean(char const *name);

/* the seconds left the last notice named, -1 outside a game, and the seconds
the lobby counted down the last time, NONE with none */
static long round_clock_left = -1;
static short round_clock_lobby = NONE;

void round_clock_update(
	boolean main_menu_loaded,
	real seconds)
{
	if (!config_boolean("display.time_notices"))
		return;

	(void)seconds;

	if (!main_menu_loaded && game_in_progress())
	{
		long limit;
		long left;

		round_clock_lobby = NONE;
		limit = (long)game_variant_options_get()->time_limit;
		if (limit <= 0)
		{
			round_clock_left = -1;
			return;
		}

		/* (the game's own clock: the same on every machine, the ticks each of
		them has played) */
		left = limit * 60L - game_time_get() / TICKS_PER_SECOND;
		if (left < 0)
			left = 0;
		if (left == round_clock_left)
			return;
		round_clock_left = left;

		{
			boolean say = FALSE;

			if (left == 300 || left == 600)
				say = TRUE;
			else if (left < 300 && left > 0 && (left % 60) == 0)
				say = TRUE;
			else if (left == 30 || (left <= 10 && left > 0))
				say = TRUE;

			if (say)
			{
				char text[64];

				if (left >= 60)
					snprintf(text, sizeof(text), "%ld %s left",
						left / 60, left / 60 == 1 ? "minute" : "minutes");
				else
					snprintf(text, sizeof(text), "%ld %s left",
						left, left == 1 ? "second" : "seconds");
				network_chat_add_notice(text);
			}
		}
		return;
	}

	round_clock_left = -1;

	/* the lobby, before a game begins: the countdown the server sends, which
	is the one the game will start on. The lobby's own panel draws it; this
	says it in words, so that a player who has just come in knows the game is
	counting down and not waiting for someone */
	{
		struct network_game_client *client = global_network_game_client_get();
		short left = client ? network_game_client_get_seconds_to_game_start(client) : NONE;

		if (left == round_clock_lobby)
			return;
		round_clock_lobby = left;
		if (left > 0 && (left == 30 || left == 20 || left == 15 || left == 10 || left <= 5))
		{
			char text[64];

			snprintf(text, sizeof(text), "the game starts in %d %s",
				(int)left, left == 1 ? "second" : "seconds");
			network_chat_add_notice(text);
		}
	}
}
