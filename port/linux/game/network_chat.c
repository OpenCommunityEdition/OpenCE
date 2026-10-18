/*
NETWORK_CHAT.C

The text chat of network games (port/linux/NETCODE.md), as the PC version
had it: T (controls.chat) starts a line to everyone, Y (controls.team_chat)
one to the player's team; Tab, while typing, turns it to everyone, the team
or the vehicle in turn; Enter sends it, Escape lets it go. The lines are
drawn above the motion sensor, each with its player's name in their team's
colour, and fade after a while; on Android, where the touch controls hold the
bottom left, they go at the top left under the weapon's panel, the newest
highest.

A client sends its line to the host (_distributed_message_chat_up,
reliably), which takes it only from the client's own stream, from one of
that machine's players, at most CHAT_BURST lines at once and one each
CHAT_REFILL_MILLISECONDS after, its text cut to printable ASCII. The host
sends it on to every machine that hears it (_distributed_message_chat_down,
reliably): all of them, a team's line only those with a player of the
team (every machine in a game without teams), and a vehicle's line only
those with a player riding in the vehicle the sender rides in (the
outermost: a vehicle on another's back is its), whatever the map's vehicles
are, and the sender's own. A client shows only what the
host sends, its own lines too, so every machine shows the same lines in the
same order. A build without the chat drops both kinds as unknown: its
players neither send nor see lines.

The host may type "/mute <player> [minutes]" (5 when it gives none, a day
at most) or "/unmute <player>", the player as the console's kick finds one
(the whole name in either case, else the one it begins). It keeps the mute
by the player's name, which then sends no line further than the host till
the minutes are up (or the host unmutes it) and is told for how long; every
machine is told of each mute and unmute, a line of the host's alone in the
chat (_chat_line_notice_bit). A client's /mute is not sent: only the host
mutes.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "memory/data.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "input/input.h"
#include "interface/hud_draw.h"
#include "interface/hud_messaging.h"
#include "main/console.h"
#include "objects/objects.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "text/text_group.h"
#include "network_distributed.h"
#include "network_chat.h"
#include "halo_keyboard.h"

#include <stdio.h>

/* the platform layer's */
unsigned long system_milliseconds(void);
int platform_clipboard_get(char *text, int size);
/* port_config.c's */
const char *config_string(const char *name);
unsigned long config_changes(void);

/* ---------- constants */

enum
{
	/* a line's text, with its end */
	CHAT_TEXT_SIZE = 64,
	/* a player's name, with its end */
	CHAT_NAME_SIZE = 12,
	/* the lines kept, and shown at once */
	CHAT_MAXIMUM_LINES = 8,
	CHAT_LINES_SHOWN = 6,
	/* a line shown, then faded out */
	CHAT_LINE_MILLISECONDS = 10000,
	CHAT_FADE_MILLISECONDS = 1000,
	/* the host: the lines a machine sends at once, and the time it gains
	another in */
	CHAT_BURST = 5,
	CHAT_REFILL_MILLISECONDS = 1500,
	MAXIMUM_CHAT_MACHINES = HALO_PORT_MAXIMUM_NETWORK_MACHINES,
	/* a line's name and text as drawn (streamer mode's words may be longer) */
	CHAT_DRAWN_SIZE = 2 * CHAT_TEXT_SIZE,
	/* the caret's half period while typing */
	CHAT_CARET_MILLISECONDS = 500,
	/* Android: the top of the lines, in the HUD's units (480 high, before
	its scale), below the tallest weapon's panel at the top left */
	CHAT_TOP_UNITS = 60,
	/* Android: the lines shown at once there, the one being typed among
	them (a fifth would reach the tops of the touch controls' Up and Fire) */
	CHAT_LINES_SHOWN_ANDROID = 4,
	/* a key's Ctrl (struct key_stroke's modifier_flags: menu_functions.c's) */
	CHAT_KEY_MODIFIER_CONTROL_BIT = 1,
	/* the host's /mute: the minutes when it gives none, and the most (a day) */
	CHAT_MUTE_DEFAULT_MINUTES = 5,
	CHAT_MUTE_MAXIMUM_MINUTES = 1440,
	CHAT_MILLISECONDS_PER_MINUTE = 60000,
	/* the players muted at once */
	CHAT_MAXIMUM_MUTES = MAXIMUM_TRACKED_PLAYERS,
	/* (chat_name_match) a name more than one player's begins with */
	CHAT_NAME_AMBIGUOUS = -2,
};

/* the line's flags */
enum
{
	/* to the sender's team alone */
	_chat_line_team_bit,
	/* to those in the sender's vehicle alone */
	_chat_line_vehicle_bit,
	/* the host's notice (a mute's): its text alone, from no player */
	_chat_line_notice_bit,
};

/* who a line is to (network_chat.h's _chat_channel_*) */

/* a line's colour: its sender's team's, or a notice's */
enum
{
	_chat_color_red,
	_chat_color_blue,
	_chat_color_notice,
};

/* what a typed line asks (chat_command_parse) */
enum
{
	/* nothing: it is said */
	_chat_command_none,
	_chat_command_mute,
	_chat_command_unmute,
	/* /mute or /unmute without a player, or a mute of no minutes */
	_chat_command_usage,
};

/* ---------- structures */

/* a line: up, the sender as it says (one of its machine's players); down,
the sender as the host found it */
struct distributed_chat_line
{
	/* the sender (absolute index) */
	byte player_index;
	byte flags;
	byte pad[2];
	char text[CHAT_TEXT_SIZE];
};

typedef char distributed_chat_line_size_assert[sizeof(struct distributed_chat_line) == 68 ? 1 : -1];

/* the host: a machine's lines allowed (chat_rate_take) */
struct chat_rate
{
	boolean started;
	short tokens;
	/* (system_milliseconds) when the last token came, or the bucket was last full */
	unsigned long refilled;
};

/* the host: a player muted, by name (one who leaves and joins again under
it still is), till its minutes are up */
struct chat_mute
{
	boolean active;
	char name[CHAT_NAME_SIZE];
	/* (system_milliseconds) */
	unsigned long started;
	unsigned long milliseconds;
};

/* a line shown */
struct chat_line
{
	wchar_t name[CHAT_NAME_SIZE];
	char text[CHAT_TEXT_SIZE];
	short color;
	short channel;
	/* (system_milliseconds) */
	unsigned long time;
};

/* ---------- globals */

/* the lines, the newest at chat_newest_line */
static struct chat_line chat_lines[CHAT_MAXIMUM_LINES];
static short chat_line_count;
static short chat_newest_line;

/* the line typed */
static struct
{
	boolean active;
	short channel;
	char text[CHAT_TEXT_SIZE];
} chat_input;

/* the keyboard's chat actions held last frame (a press starts a line) */
static unsigned long chat_actions_held;

/* the host's */
static struct chat_rate chat_rates[MAXIMUM_CHAT_MACHINES];
static struct chat_mute chat_mutes[CHAT_MAXIMUM_MUTES];

/* each local player's motion sensor as last drawn (the window's units),
and whether it has been */
static struct
{
	boolean valid;
	short left;
	short top;
} chat_motion_sensors[MAXIMUM_LOCAL_PLAYERS];

/* ---------- the rules (pure: tools/harness/tests/chat_logic.c) */

/* what another machine sent, or the keyboard typed, as a line's text: at most
CHAT_TEXT_SIZE - 1 characters, to the source's end or source_size (what
another machine sent need not be ended), printable ASCII ("?" for anything
else), without the spaces it begins and ends with; its length */
static short chat_text_clean(
	char *destination,
	char const *source,
	short source_size)
{
	short length = 0;
	short index;

	for (index = 0; index < source_size && source[index] && length < CHAT_TEXT_SIZE - 1; index++)
	{
		char character = source[index];

		if (character < ' ' || character > '~')
			character = '?';
		if (character == ' ' && length == 0)
			continue;
		destination[length++] = character;
	}
	while (length > 0 && destination[length - 1] == ' ')
		length--;
	destination[length] = 0;
	return length;
}

/* the typed text (or pasted) added to a line, as far as it holds: printable
ASCII only, the rest left out */
static void chat_text_append(
	char *text,
	char const *typed)
{
	short length = (short)csstrlen(text);

	for (; *typed && length < CHAT_TEXT_SIZE - 1; typed++)
	{
		if (*typed >= ' ' && *typed <= '~')
			text[length++] = *typed;
	}
	text[length] = 0;
}

/* whether a machine may send a line now (now: system_milliseconds), and it
counted: CHAT_BURST at once, and one more each CHAT_REFILL_MILLISECONDS
since the last came (or since the bucket was last full) */
static boolean chat_rate_take(
	struct chat_rate *rate,
	unsigned long now)
{
	unsigned long gained;

	if (!rate->started)
	{
		rate->started = TRUE;
		rate->tokens = CHAT_BURST;
		rate->refilled = now;
	}
	gained = (now - rate->refilled) / CHAT_REFILL_MILLISECONDS;
	if (gained > 0)
	{
		rate->tokens = (short)MIN((unsigned long)CHAT_BURST, rate->tokens + gained);
		rate->refilled += gained * CHAT_REFILL_MILLISECONDS;
	}
	/* (full: the next token comes a whole period after one is spent) */
	if (rate->tokens >= CHAT_BURST)
		rate->refilled = now;
	if (rate->tokens <= 0)
		return FALSE;
	rate->tokens--;
	return TRUE;
}

/* whether a listener hears a line of a channel: one to everyone; a team's
by the same team, or by everyone in a game without teams; a vehicle's by
those riding in the vehicle the sender rides in (NONE: none), none when the
sender rides in none (the sender sees their own lines whatever this says) */
static boolean chat_hears(
	short channel,
	boolean has_teams,
	long sender_team,
	long listener_team,
	long sender_vehicle,
	long listener_vehicle)
{
	switch (channel)
	{
	case _chat_channel_team:
		return !has_teams || sender_team == listener_team;
	case _chat_channel_vehicle:
		return sender_vehicle != NONE && sender_vehicle == listener_vehicle;
	default:
		return TRUE;
	}
}

/* a line's channel from its flags, and back */
static short chat_channel_from_flags(
	byte flags)
{
	if (TEST_FLAG(flags, _chat_line_vehicle_bit))
		return _chat_channel_vehicle;
	return TEST_FLAG(flags, _chat_line_team_bit) ? _chat_channel_team : _chat_channel_everyone;
}

static byte chat_channel_flags(
	short channel)
{
	if (channel == _chat_channel_vehicle)
		return (byte)FLAG(_chat_line_vehicle_bit);
	return channel == _chat_channel_team ? (byte)FLAG(_chat_line_team_bit) : 0;
}

/* how much of a line shows at its age (milliseconds): all of it for a while,
then fading out; all the lines kept while a line is typed */
static real chat_line_alpha(
	unsigned long age,
	boolean typing)
{
	if (typing || age < CHAT_LINE_MILLISECONDS)
		return 1.0f;
	if (age >= CHAT_LINE_MILLISECONDS + CHAT_FADE_MILLISECONDS)
		return 0.0f;
	return 1.0f - (real)(age - CHAT_LINE_MILLISECONDS) / (real)CHAT_FADE_MILLISECONDS;
}

static char chat_lower(
	char character)
{
	return character >= 'A' && character <= 'Z' ? (char)(character - 'A' + 'a') : character;
}

/* what follows a command's word (lower case) at the start of a line, in
either case, after its spaces; NULL when the line does not start with it
(a word of its own: "/muted" is not "/mute") */
static char const *chat_command_word(
	char const *text,
	char const *word)
{
	short index;

	for (index = 0; word[index]; index++)
	{
		if (chat_lower(text[index]) != word[index])
			return NULL;
	}
	if (text[index] && text[index] != ' ')
		return NULL;
	while (text[index] == ' ')
		index++;
	return text + index;
}

/* what a typed line (chat_text_clean's) asks: "/mute <player> [minutes]" or
"/unmute <player>", the word in either case; the player's name as typed (at
most name_size with its end: it may have spaces) and the minutes (a mute's
last word when all digits after a name; CHAT_MUTE_DEFAULT_MINUTES when it
gives none, CHAT_MUTE_MAXIMUM_MINUTES at most); _chat_command_none for any
other line, said as it is */
static short chat_command_parse(
	char const *text,
	char *name,
	short name_size,
	short *minutes)
{
	char const *rest;
	short command;
	short length;
	short last_word;
	short index;

	name[0] = 0;
	*minutes = CHAT_MUTE_DEFAULT_MINUTES;
	if ((rest = chat_command_word(text, "/mute")) != NULL)
		command = _chat_command_mute;
	else if ((rest = chat_command_word(text, "/unmute")) != NULL)
		command = _chat_command_unmute;
	else
		return _chat_command_none;
	length = (short)csstrlen(rest);
	for (last_word = length; last_word > 0 && rest[last_word - 1] != ' '; last_word--)
		;
	if (command == _chat_command_mute && last_word > 0 && last_word < length)
	{
		long typed_minutes = 0;

		for (index = last_word; index < length && rest[index] >= '0' && rest[index] <= '9'; index++)
			typed_minutes = MIN(typed_minutes * 10 + (rest[index] - '0'), (long)CHAT_MUTE_MAXIMUM_MINUTES);
		if (index == length)
		{
			if (typed_minutes == 0)
				return _chat_command_usage;
			*minutes = (short)typed_minutes;
			for (length = last_word; length > 0 && rest[length - 1] == ' '; length--)
				;
		}
	}
	for (index = 0; index < length && index < name_size - 1; index++)
		name[index] = rest[index];
	name[index] = 0;
	return index ? command : _chat_command_usage;
}

/* the player a typed name means among the players' names (ASCII; "" where
there is no player), as the console's kick and ban find one
(network_server_manager.c): the one of that name, in either case, else the
one whose name begins with it; NONE for none, CHAT_NAME_AMBIGUOUS for more
than one */
static short chat_name_match(
	char const *typed,
	char const (*names)[CHAT_NAME_SIZE],
	short count)
{
	short exact_index = NONE;
	short exact_count = 0;
	short begins_index = NONE;
	short begins_count = 0;
	short index;

	for (index = 0; typed[0] && index < count; index++)
	{
		char const *name = names[index];
		short length;

		/* (a name's end differs from any character typed: never read past it) */
		for (length = 0; typed[length] && chat_lower(name[length]) == chat_lower(typed[length]); length++)
			;
		if (typed[length])
			continue;
		begins_index = index;
		begins_count++;
		if (!name[length])
		{
			exact_index = index;
			exact_count++;
		}
	}
	if (exact_count == 1)
		return exact_index;
	if (exact_count > 1 || begins_count > 1)
		return CHAT_NAME_AMBIGUOUS;
	return begins_index;
}

/* the minutes a mute has left at now (system_milliseconds), the one begun
counted whole; 0 once it has run out, when it is let go */
static short chat_mute_minutes_left(
	struct chat_mute *mute,
	unsigned long now)
{
	unsigned long elapsed = now - mute->started;

	if (!mute->active)
		return 0;
	if (elapsed >= mute->milliseconds)
	{
		mute->active = FALSE;
		return 0;
	}
	return (short)((mute->milliseconds - elapsed + CHAT_MILLISECONDS_PER_MINUTE - 1) / CHAT_MILLISECONDS_PER_MINUTE);
}

/* the mute of a player's name, while it runs; NULL */
static struct chat_mute *chat_mute_find(
	struct chat_mute *mutes,
	short count,
	char const *name,
	unsigned long now)
{
	short index;

	for (index = 0; index < count; index++)
	{
		if (chat_mute_minutes_left(&mutes[index], now) && !csstrcmp(mutes[index].name, name))
			return &mutes[index];
	}
	return NULL;
}

/* a player's name muted for minutes from now: its mute again if it has one,
else one run out; FALSE when every one is held */
static boolean chat_mute_set(
	struct chat_mute *mutes,
	short count,
	char const *name,
	short minutes,
	unsigned long now)
{
	struct chat_mute *mute = chat_mute_find(mutes, count, name, now);
	short index;

	for (index = 0; !mute && index < count; index++)
	{
		if (!chat_mute_minutes_left(&mutes[index], now))
			mute = &mutes[index];
	}
	if (!mute)
		return FALSE;
	mute->active = TRUE;
	csstrncpy(mute->name, name, CHAT_NAME_SIZE - 1);
	mute->name[CHAT_NAME_SIZE - 1] = 0;
	mute->started = now;
	mute->milliseconds = (unsigned long)minutes * CHAT_MILLISECONDS_PER_MINUTE;
	return TRUE;
}

/* the minutes in a notice: "5 minutes", "1 minute" */
static char const *chat_minutes_plural(
	short minutes)
{
	return minutes == 1 ? "" : "s";
}

/* ---------- the lines */

static boolean chat_network_game(
	void)
{
	short connection = game_connection();

	return connection == _game_connection_network_client || connection == _game_connection_network_server;
}

/* a player's name in ASCII ("?" for what is not), as debug.txt and /mute
have it */
static void chat_player_name_ascii(
	struct player_datum const *player,
	char *name)
{
	short index;

	for (index = 0; index < CHAT_NAME_SIZE - 1 && index < (short)NUMBEROF(player->name) && player->name[index];
		index++)
	{
		name[index] = player_name_character_ascii(player->name[index]);
	}
	name[index] = 0;
}

/* the newest line, cleared (the oldest gives way) */
static struct chat_line *chat_new_line(
	char const *text)
{
	struct chat_line *line;

	chat_newest_line = (short)((chat_newest_line + 1) % CHAT_MAXIMUM_LINES);
	if (chat_line_count < CHAT_MAXIMUM_LINES)
		chat_line_count++;
	line = &chat_lines[chat_newest_line];
	csmemset(line, 0, sizeof(*line));
	csstrncpy(line->text, text, CHAT_TEXT_SIZE - 1);
	line->time = system_milliseconds();
	return line;
}

/* a line shown, from a player (absolute index) */
static void chat_add_line(
	short player_index,
	short channel,
	char const *text)
{
	struct player_datum *player = distributed_player(player_index);
	struct chat_line *line;
	char name[CHAT_NAME_SIZE];
	short index;

	if (!player)
		return;
	line = chat_new_line(text);
	for (index = 0; index < CHAT_NAME_SIZE - 1 && index < (short)NUMBEROF(player->name) && player->name[index];
		index++)
	{
		line->name[index] = player->name[index];
	}
	line->color = game_engine_has_teams() && player->team_index == 0 ? _chat_color_red : _chat_color_blue;
	line->channel = channel;
	/* (debug.txt has the lines, in ASCII) */
	chat_player_name_ascii(player, name);
	error(_error_log, "chat%s: %s: %s", channel == _chat_channel_team ? " (team)" :
		channel == _chat_channel_vehicle ? " (vehicle)" : "", name, line->text);
}

/* a notice shown (the host's): its text alone */
static void chat_add_notice(
	char const *text)
{
	struct chat_line *line = chat_new_line(text);

	line->color = _chat_color_notice;
	line->channel = _chat_channel_everyone;
	error(_error_log, "chat notice: %s", line->text);
}

/* ---------- the host */

static boolean chat_host(
	void)
{
	return game_connection() == _game_connection_network_server;
}

/* the vehicle a player rides in, the outermost (a vehicle carried by another
is in that one); NONE for none, or no unit */
static long chat_player_vehicle(
	struct player_datum const *player)
{
	long object_index = player->unit_index;
	long vehicle_index = NONE;

	/* (each object riding in its parent: a unit in a seat is its vehicle's child) */
	while (object_index != NONE)
	{
		struct object_datum *object = object_try_and_get(object_index);

		if (!object || object->object.parent_object_index == NONE)
			break;
		vehicle_index = object->object.parent_object_index;
		object_index = vehicle_index;
	}
	return vehicle_index;
}

/* (the host) a player's line, sent on to every machine that hears it (and
the sender's) and shown here if a player of this machine's hears it or said
it */
static void chat_host_send(
	short player_index,
	short channel,
	char const *text)
{
	struct player_datum *sender = distributed_player(player_index);
	boolean has_teams = game_engine_has_teams();
	long sender_vehicle;
	long machine_indices[MAXIMUM_CHAT_MACHINES];
	short count = distributed_client_machines(machine_indices, MAXIMUM_CHAT_MACHINES);
	struct
	{
		struct distributed_message_header header;
		struct distributed_chat_line line;
	} message;
	short index;
	short local_player_index;

	if (!sender || sender->quit_out_of_game)
		return;
	sender_vehicle = chat_player_vehicle(sender);
	csmemset(&message, 0, sizeof(message));
	message.line.player_index = distributed_player_to_byte(player_index);
	message.line.flags = chat_channel_flags(channel);
	csstrncpy(message.line.text, text, CHAT_TEXT_SIZE - 1);
	for (index = 0; index < count; index++)
	{
		struct data_iterator iterator;
		struct player_datum *player;
		boolean hears = distributed_machine_has_player(machine_indices[index], player_index);

		data_iterator_new(&iterator, player_data);
		while (!hears && (player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		{
			hears = !player->quit_out_of_game &&
				distributed_machine_has_player(machine_indices[index],
					(short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index)) &&
				chat_hears(channel, has_teams, sender->team_index, player->team_index, sender_vehicle,
					chat_player_vehicle(player));
		}
		if (hears)
		{
			distributed_send_to_machine_reliably(machine_indices[index], &message, _distributed_message_chat_down, 1,
				(word)sizeof(message));
		}
	}
	for (local_player_index = local_player_get_next(NONE); local_player_index != NONE;
		local_player_index = local_player_get_next(local_player_index))
	{
		long local_index = local_player_get_player_index(local_player_index);

		if (local_index != NONE &&
			((short)DATUM_INDEX_TO_ABSOLUTE_INDEX(local_index) == player_index ||
				chat_hears(channel, has_teams, sender->team_index, player_get(local_index)->team_index,
					sender_vehicle, chat_player_vehicle(player_get(local_index)))))
		{
			chat_add_line(player_index, channel, text);
			break;
		}
	}
}

/* (the host) a notice to a client machine; or (NONE) to every one, and
shown here */
static void chat_host_notice(
	long machine_index,
	char const *text)
{
	long machine_indices[MAXIMUM_CHAT_MACHINES];
	short count;
	struct
	{
		struct distributed_message_header header;
		struct distributed_chat_line line;
	} message;
	short index;

	csmemset(&message, 0, sizeof(message));
	message.line.flags = (byte)FLAG(_chat_line_notice_bit);
	csstrncpy(message.line.text, text, CHAT_TEXT_SIZE - 1);
	if (machine_index != NONE)
	{
		distributed_send_to_machine_reliably(machine_index, &message, _distributed_message_chat_down, 1,
			(word)sizeof(message));
		return;
	}
	count = distributed_client_machines(machine_indices, MAXIMUM_CHAT_MACHINES);
	for (index = 0; index < count; index++)
	{
		distributed_send_to_machine_reliably(machine_indices[index], &message, _distributed_message_chat_down, 1,
			(word)sizeof(message));
	}
	chat_add_notice(text);
}

/* (the host) its player's /mute or /unmute: the player named muted for the
minutes, or let speak again, every machine told; what is wrong with it shown
to the host alone; FALSE for a line that is no command (said as it is) */
static boolean chat_host_command(
	short speaker_index,
	char const *text)
{
	char typed[CHAT_TEXT_SIZE];
	char names[MAXIMUM_TRACKED_PLAYERS][CHAT_NAME_SIZE];
	char notice[2 * CHAT_TEXT_SIZE];
	unsigned long now = system_milliseconds();
	struct data_iterator iterator;
	struct player_datum *player;
	struct chat_mute *mute;
	short minutes;
	short command = chat_command_parse(text, typed, sizeof(typed), &minutes);
	short found;

	if (command == _chat_command_none)
		return FALSE;
	if (command == _chat_command_usage)
	{
		chat_add_notice("Say /mute <player> [minutes] or /unmute <player>");
		return TRUE;
	}
	csmemset(names, 0, sizeof(names));
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		short index = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);

		if (!player->quit_out_of_game && index >= 0 && index < MAXIMUM_TRACKED_PLAYERS)
			chat_player_name_ascii(player, names[index]);
	}
	found = chat_name_match(typed, (char const (*)[CHAT_NAME_SIZE])names, MAXIMUM_TRACKED_PLAYERS);
	if (found == NONE)
		snprintf(notice, sizeof(notice), "No player's name begins with \"%s\"", typed);
	else if (found == CHAT_NAME_AMBIGUOUS)
		snprintf(notice, sizeof(notice), "More than one player's name begins with \"%s\"", typed);
	else if (found == speaker_index)
		snprintf(notice, sizeof(notice), "You can't %s yourself", command == _chat_command_mute ? "mute" : "unmute");
	else if (command == _chat_command_mute)
	{
		if (chat_mute_set(chat_mutes, NUMBEROF(chat_mutes), names[found], minutes, now))
		{
			snprintf(notice, sizeof(notice), "%s muted for %d minute%s", names[found], minutes,
				chat_minutes_plural(minutes));
			chat_host_notice(NONE, notice);
			return TRUE;
		}
		snprintf(notice, sizeof(notice), "Too many players are muted");
	}
	else if ((mute = chat_mute_find(chat_mutes, NUMBEROF(chat_mutes), names[found], now)) != NULL)
	{
		mute->active = FALSE;
		snprintf(notice, sizeof(notice), "%s unmuted", names[found]);
		chat_host_notice(NONE, notice);
		return TRUE;
	}
	else
		snprintf(notice, sizeof(notice), "%s isn't muted", names[found]);
	chat_add_notice(notice);
	return TRUE;
}

void network_chat_handle_up(
	long machine_index,
	void const *entries,
	boolean from_stream)
{
	struct distributed_chat_line line;
	char text[CHAT_TEXT_SIZE];
	char name[CHAT_NAME_SIZE];
	struct player_datum *sender;
	struct chat_mute *mute;
	unsigned long now = system_milliseconds();

	if (!chat_host() || machine_index < 0 || machine_index >= MAXIMUM_CHAT_MACHINES || !from_stream)
		return;
	csmemcpy(&line, entries, sizeof(line));
	/* (one of the machine's own players; then counted, and its text) */
	if (line.player_index >= MAXIMUM_TRACKED_PLAYERS ||
		!distributed_machine_has_player(machine_index, line.player_index) ||
		!chat_rate_take(&chat_rates[machine_index], now) ||
		!chat_text_clean(text, line.text, CHAT_TEXT_SIZE) ||
		(sender = distributed_player(line.player_index)) == NULL)
	{
		return;
	}
	/* (a muted player's line goes no further: they are told for how long) */
	chat_player_name_ascii(sender, name);
	if ((mute = chat_mute_find(chat_mutes, NUMBEROF(chat_mutes), name, now)) != NULL)
	{
		short minutes = chat_mute_minutes_left(mute, now);
		char notice[CHAT_TEXT_SIZE];

		snprintf(notice, sizeof(notice), "You are muted for %d more minute%s", minutes, chat_minutes_plural(minutes));
		chat_host_notice(machine_index, notice);
		return;
	}
	chat_host_send(line.player_index, chat_channel_from_flags(line.flags), text);
}

word network_chat_line_entry_size(
	void)
{
	return sizeof(struct distributed_chat_line);
}

/* ---------- a client */

void network_chat_handle_down(
	void const *entries)
{
	struct distributed_chat_line line;
	char text[CHAT_TEXT_SIZE];

	if (game_connection() != _game_connection_network_client)
		return;
	csmemcpy(&line, entries, sizeof(line));
	if (!chat_text_clean(text, line.text, CHAT_TEXT_SIZE))
		return;
	if (TEST_FLAG(line.flags, _chat_line_notice_bit))
	{
		chat_add_notice(text);
		return;
	}
	if (distributed_player_from_byte(line.player_index) == NONE)
		return;
	chat_add_line(line.player_index, chat_channel_from_flags(line.flags), text);
}

/* ---------- typing */

/* the first local player's (absolute index), who speaks; NONE */
static short chat_speaker(
	void)
{
	short local_player_index = local_player_get_next(NONE);
	long player_index = local_player_index != NONE ? local_player_get_player_index(local_player_index) : NONE;

	return player_index != NONE ? (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index) : NONE;
}

static void chat_send(
	short channel,
	char const *typed)
{
	char text[CHAT_TEXT_SIZE];
	char name[CHAT_TEXT_SIZE];
	short minutes;
	short player_index = chat_speaker();

	if (player_index == NONE || !chat_text_clean(text, typed, CHAT_TEXT_SIZE))
		return;
	if (chat_host())
	{
		if (!chat_host_command(player_index, text))
			chat_host_send(player_index, channel, text);
	}
	/* (the host's alone: a client's is not sent) */
	else if (chat_command_parse(text, name, sizeof(name), &minutes) != _chat_command_none)
	{
		chat_add_notice("Only the host can mute players");
	}
	else
	{
		struct
		{
			struct distributed_message_header header;
			struct distributed_chat_line line;
		} message;

		csmemset(&message, 0, sizeof(message));
		message.line.player_index = distributed_player_to_byte(player_index);
		message.line.flags = chat_channel_flags(channel);
		csstrncpy(message.line.text, text, CHAT_TEXT_SIZE - 1);
		distributed_send(&message, _distributed_message_chat_up, 1, (word)sizeof(message),
			_distributed_to_host_reliably);
	}
}

void network_chat_say(
	short channel,
	char const *text)
{
	if (chat_network_game() && game_in_progress() && channel >= 0 && channel < NUMBER_OF_CHAT_CHANNELS)
		chat_send(channel, text);
}

static void chat_input_begin(
	short channel)
{
	struct key_stroke key;

	csmemset(&chat_input, 0, sizeof(chat_input));
	chat_input.active = TRUE;
	chat_input.channel = channel;
	/* (the keys pressed before) */
	while (input_get_key(&key))
		;
}

static void chat_input_end(
	void)
{
	chat_input.active = FALSE;
}

void network_chat_update(
	void)
{
	unsigned long held;
	unsigned long pressed;
	struct key_stroke key;

	if (!chat_network_game() || !game_in_progress() || chat_speaker() == NONE)
	{
		chat_input_end();
		chat_actions_held = 0;
		return;
	}
	if (!chat_input.active)
	{
		held = halo_keyboard_actions(0) &
			(FLAG(HALO_KEYBOARD_CHAT) | FLAG(HALO_KEYBOARD_TEAM_CHAT));
		pressed = held & ~chat_actions_held;
		chat_actions_held = held;
		if (pressed && !console_is_active())
			chat_input_begin(TEST_FLAG(pressed, HALO_KEYBOARD_CHAT) ? _chat_channel_everyone : _chat_channel_team);
		return;
	}
	chat_actions_held = 0;
	/* (the console, opened over it, takes the keys) */
	if (console_is_active())
		return;
	while (chat_input.active && input_get_key(&key))
	{
		short length = (short)csstrlen(chat_input.text);

		switch (key.key_code)
		{
		case _key_escape:
			chat_input_end();
			break;
		case _key_return:
		case _keypad_enter:
			chat_send(chat_input.channel, chat_input.text);
			chat_input_end();
			break;
		/* (to everyone, the team, the vehicle, in turn) */
		case _key_tab:
			chat_input.channel = (short)((chat_input.channel + 1) % NUMBER_OF_CHAT_CHANNELS);
			break;
		case _key_backspace:
			if (length)
				chat_input.text[length - 1] = 0;
			break;
		default:
			/* (Ctrl+V pastes) */
			if (TEST_FLAG(key.modifier_flags, CHAT_KEY_MODIFIER_CONTROL_BIT) && key.key_code == _key_v)
			{
				char clipboard[CHAT_TEXT_SIZE];

				if (platform_clipboard_get(clipboard, sizeof(clipboard)))
					chat_text_append(chat_input.text, clipboard);
			}
			else if (key.ascii_code >= ' ' && key.ascii_code <= '~')
			{
				char typed[2];

				typed[0] = key.ascii_code;
				typed[1] = 0;
				chat_text_append(chat_input.text, typed);
			}
			break;
		}
	}
}

boolean network_chat_typing(
	void)
{
	return chat_input.active;
}

/* ---------- drawing */

void network_chat_note_motion_sensor(
	short local_player_index,
	short left,
	short top)
{
	if (local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS)
		return;
	chat_motion_sensors[local_player_index].valid = TRUE;
	chat_motion_sensors[local_player_index].left = left;
	chat_motion_sensors[local_player_index].top = top;
}

/* whether the lines are all white, as Custom Edition draws them
(display.chat_style "white"), not the PC version's colours ("default") */
static boolean chat_style_white(
	void)
{
	static boolean white;
	static unsigned long read_at = (unsigned long)-1;

	/* (read again when Settings changes it) */
	if (read_at != config_changes())
	{
		const char *style = config_string("display.chat_style");

		read_at = config_changes();
		white = style && !csstrcmp(style, "white");
	}
	return white;
}

/* a name's colour: its team's (a notice's a pale gold), or white */
static void chat_color(
	short color,
	real alpha,
	real_argb_color *result)
{
	result->alpha = alpha;
	if (chat_style_white())
	{
		result->red = result->green = result->blue = 1.0f;
	}
	else if (color == _chat_color_red)
	{
		result->red = 0.86f;
		result->green = 0.32f;
		result->blue = 0.28f;
	}
	else if (color == _chat_color_notice)
	{
		result->red = 0.93f;
		result->green = 0.82f;
		result->blue = 0.45f;
	}
	else
	{
		result->red = 0.34f;
		result->green = 0.51f;
		result->blue = 0.86f;
	}
}

/* the message's colour: a pale blue, or white */
static void chat_text_color(
	real alpha,
	real_argb_color *result)
{
	result->alpha = alpha;
	if (chat_style_white())
	{
		result->red = result->green = result->blue = 1.0f;
		return;
	}
	result->red = 0.64f;
	result->green = 0.75f;
	result->blue = 0.87f;
}

/* a string's soft shadow, down and right of it: copies in a translucent
black about its offset, which add up to a dark middle with a soft edge (the
text's own shadow, a hard copy a unit down and right, is not drawn) */
static void chat_draw_shadow(
	long font_index,
	rectangle2d const *bounds,
	wchar_t const *string,
	real alpha)
{
	static struct
	{
		short x;
		short y;
		real alpha;
	} const copies[] =
	{
		{ 1, 1, 0.45f },
		{ 2, 2, 0.35f },
		{ 2, 1, 0.25f },
		{ 1, 2, 0.25f },
		{ 3, 2, 0.12f },
		{ 2, 3, 0.12f },
		{ 3, 3, 0.10f },
	};
	short index;

	for (index = 0; index < (short)NUMBEROF(copies); index++)
	{
		real_argb_color color;
		rectangle2d shadow_bounds = *bounds;

		color.alpha = copies[index].alpha * alpha;
		color.red = color.green = color.blue = 0.0f;
		offset_rectangle2d(&shadow_bounds, copies[index].x, copies[index].y);
		draw_string_set_draw_mode(font_index, NONE, 0 /* left */, 0, &color);
		rasterizer_draw_unicode_string(&shadow_bounds, NULL, NULL, 0, string);
	}
}

/* a line: the name (or prompt) in its colour, then the text in the pale
blue, over their shadows; bounds' top and bottom the line's */
static void chat_draw_line(
	long font_index,
	rectangle2d const *bounds,
	wchar_t const *name,
	real_argb_color const *name_color,
	wchar_t const *wide,
	real alpha)
{
	rectangle2d name_bounds;
	rectangle2d cursor_bounds;
	rectangle2d text_bounds = *bounds;
	real_argb_color color;

	draw_string_set_draw_mode(font_index, NONE, 0 /* left */, 0, name_color);
	draw_unicode_string_compute_bounds(bounds, name, &name_bounds, &cursor_bounds);
	/* (a space's width after the colon) */
	text_bounds.x0 = (short)(name_bounds.x1 + (bounds->y1 - bounds->y0) / 4);
	/* (the text's own shadow drawn clear: a colour of no alpha that is not
	0, which would be the text's black) */
	rasterizer_text_set_shadow_color(0x00000001);
	chat_draw_shadow(font_index, bounds, name, alpha);
	chat_draw_shadow(font_index, &text_bounds, wide, alpha);
	draw_string_set_draw_mode(font_index, NONE, 0 /* left */, 0, name_color);
	rasterizer_draw_unicode_string(bounds, NULL, NULL, 0, name);
	chat_text_color(alpha, &color);
	draw_string_set_draw_mode(font_index, NONE, 0 /* left */, 0, &color);
	rasterizer_draw_unicode_string(&text_bounds, NULL, NULL, 0, wide);
	rasterizer_text_set_shadow_color(0);
}

/* a line's text as drawn (streamer mode hides its words as in every text the
game draws: draw_string.c, text_filter.c) */
static void chat_line_text(
	char const *text,
	wchar_t *drawn,
	short size)
{
	short index;

	for (index = 0; text[index] && index < size - 1; index++)
		drawn[index] = (wchar_t)(unsigned char)text[index];
	drawn[index] = 0;
}

/* a line's name as drawn: "Name:", "Name (team):" or "Name (vehicle):" */
static void chat_line_name(
	struct chat_line const *line,
	wchar_t *name,
	short size)
{
	wchar_t const *suffix = line->channel == _chat_channel_team ? L" (team):" :
		line->channel == _chat_channel_vehicle ? L" (vehicle):" : L":";
	wchar_t const *player_name = line->name;
	short length = 0;
	short index;

	for (index = 0; player_name[index] && length < size - 1; index++)
		name[length++] = player_name[index];
	for (index = 0; suffix[index] && length < size - 1; index++)
		name[length++] = suffix[index];
	name[length] = 0;
}

void network_chat_draw(
	short local_player_index)
{
	long font_index = hud_get_font_index();
	unsigned long now = system_milliseconds();
	boolean typing = chat_input.active && !console_is_active();
	rectangle2d window = render.camera.window_bounds;
	struct font_header *font;
	short line_height;
	short left;
	/* the edge the next line is drawn from, and from it to the next's */
	short edge;
	short step;
	short shown;
	short maximum_shown = CHAT_LINES_SHOWN;
	short index;

	if (!chat_network_game() || font_index == NONE || local_player_index < 0 ||
		local_player_index >= MAXIMUM_LOCAL_PLAYERS || (!chat_line_count && !typing))
	{
		return;
	}
	offset_rectangle2d(&window, -render.camera.viewport_bounds.x0, -render.camera.viewport_bounds.y0);
	font = font_definition_get(font_index);
	line_height = font->ascending_height + font->descending_height + font->leading_height;
	if (line_height <= 0)
		return;
	/* (above the motion sensor, a little in from its left, the newest
	lowest; where it has not been drawn, where it would be) */
	if (chat_motion_sensors[local_player_index].valid)
	{
		left = chat_motion_sensors[local_player_index].left + line_height / 2;
		edge = chat_motion_sensors[local_player_index].top - line_height / 2;
	}
	else
	{
		left = window.x0 + 2 * line_height;
		edge = window.y1 - 9 * line_height;
	}
	step = -line_height;
#ifdef HALO_ANDROID
	/* (the touch controls' stick and buttons are at the bottom left: the
	lines go at the top left, under the weapon's panel, in the same column,
	the newest highest) */
	edge = (short)(window.y0 + CHAT_TOP_UNITS * hud_globals_get_scale(local_player_count() > 1));
	step = line_height;
	maximum_shown = typing ? CHAT_LINES_SHOWN_ANDROID - 1 : CHAT_LINES_SHOWN_ANDROID;
#endif
	if (typing)
	{
		real_argb_color color;
		rectangle2d bounds;
		wchar_t text[CHAT_TEXT_SIZE + 2];
		short length;

		bounds.x0 = left;
		bounds.x1 = window.x1;
		bounds.y0 = step < 0 ? (short)(edge - line_height) : edge;
		bounds.y1 = (short)(bounds.y0 + line_height);
		/* (streamer mode replaces its words as in every text drawn) */
		for (length = 0; chat_input.text[length]; length++)
			text[length] = (wchar_t)(unsigned char)chat_input.text[length];
		text[length] = 0;
		if ((now / CHAT_CARET_MILLISECONDS) % 2 == 0)
		{
			text[length] = '_';
			text[length + 1] = 0;
		}
		chat_color(_chat_color_blue, 1.0f, &color);
		chat_draw_line(font_index, &bounds, chat_input.channel == _chat_channel_team ? L"Say to team:" :
			chat_input.channel == _chat_channel_vehicle ? L"Say to vehicle:" : L"Say:", &color, text, 1.0f);
		edge = (short)(edge + step);
	}
	for (shown = 0, index = 0; index < chat_line_count && shown < maximum_shown; index++)
	{
		struct chat_line const *line =
			&chat_lines[(chat_newest_line - index + CHAT_MAXIMUM_LINES) % CHAT_MAXIMUM_LINES];
		real alpha = chat_line_alpha(now - line->time, typing);
		wchar_t name[CHAT_DRAWN_SIZE];
		wchar_t text[CHAT_DRAWN_SIZE];
		real_argb_color color;
		rectangle2d bounds;

		/* (the older ones have faded too) */
		if (alpha <= 0.0f)
			break;
		/* (a notice's text alone, in the name's place) */
		if (line->color == _chat_color_notice)
		{
			chat_line_text(line->text, name, NUMBEROF(name));
			text[0] = 0;
		}
		else
		{
			chat_line_name(line, name, NUMBEROF(name));
			chat_line_text(line->text, text, NUMBEROF(text));
		}
		bounds.x0 = left;
		bounds.x1 = window.x1;
		bounds.y0 = step < 0 ? (short)(edge - line_height) : edge;
		bounds.y1 = (short)(bounds.y0 + line_height);
		chat_color(line->color, alpha, &color);
		chat_draw_line(font_index, &bounds, name, &color, text, alpha);
		edge = (short)(edge + step);
		shown++;
	}
}
