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
};

/* the line's flags */
enum
{
	/* to the sender's team alone */
	_chat_line_team_bit,
	/* to those in the sender's vehicle alone */
	_chat_line_vehicle_bit,
};

/* who a line is to (network_chat.h's _chat_channel_*) */

/* a line's colour: its sender's team's */
enum
{
	_chat_color_red,
	_chat_color_blue,
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

/* ---------- the lines */

static boolean chat_network_game(
	void)
{
	short connection = game_connection();

	return connection == _game_connection_network_client || connection == _game_connection_network_server;
}

/* a line shown, from a player (absolute index) */
static void chat_add_line(
	short player_index,
	short channel,
	char const *text)
{
	struct player_datum *player = distributed_player(player_index);
	struct chat_line *line;
	short index;

	if (!player)
		return;
	chat_newest_line = (short)((chat_newest_line + 1) % CHAT_MAXIMUM_LINES);
	if (chat_line_count < CHAT_MAXIMUM_LINES)
		chat_line_count++;
	line = &chat_lines[chat_newest_line];
	csmemset(line, 0, sizeof(*line));
	for (index = 0; index < CHAT_NAME_SIZE - 1 && index < (short)NUMBEROF(player->name) && player->name[index];
		index++)
	{
		line->name[index] = player->name[index];
	}
	csstrncpy(line->text, text, CHAT_TEXT_SIZE - 1);
	line->color = game_engine_has_teams() && player->team_index == 0 ? _chat_color_red : _chat_color_blue;
	line->channel = channel;
	line->time = system_milliseconds();
	/* (debug.txt has the lines, in ASCII) */
	{
		char name[CHAT_NAME_SIZE];

		for (index = 0; line->name[index]; index++)
			name[index] = player_name_character_ascii(line->name[index]);
		name[index] = 0;
		error(_error_log, "chat%s: %s: %s", channel == _chat_channel_team ? " (team)" :
			channel == _chat_channel_vehicle ? " (vehicle)" : "", name, line->text);
	}
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

void network_chat_handle_up(
	long machine_index,
	void const *entries,
	boolean from_stream)
{
	struct distributed_chat_line line;
	char text[CHAT_TEXT_SIZE];

	if (!chat_host() || machine_index < 0 || machine_index >= MAXIMUM_CHAT_MACHINES || !from_stream)
		return;
	csmemcpy(&line, entries, sizeof(line));
	/* (one of the machine's own players; then counted, and its text) */
	if (line.player_index >= MAXIMUM_TRACKED_PLAYERS ||
		!distributed_machine_has_player(machine_index, line.player_index) ||
		!chat_rate_take(&chat_rates[machine_index], system_milliseconds()) ||
		!chat_text_clean(text, line.text, CHAT_TEXT_SIZE))
	{
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
	if (distributed_player_from_byte(line.player_index) == NONE || !chat_text_clean(text, line.text, CHAT_TEXT_SIZE))
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
	short player_index = chat_speaker();

	if (player_index == NONE || !chat_text_clean(text, typed, CHAT_TEXT_SIZE))
		return;
	if (chat_host())
	{
		chat_host_send(player_index, channel, text);
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

/* a name's colour: its team's, or white */
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

/* a line's text as drawn (streamer mode's words are replaced as in every text
the game draws: draw_string.c, chat_filter.c) */
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
		chat_line_name(line, name, NUMBEROF(name));
		chat_line_text(line->text, text, NUMBEROF(text));
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
