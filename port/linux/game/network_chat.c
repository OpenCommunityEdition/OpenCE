/*
NETWORK_CHAT.C

The text chat of network games (port/linux/NETCODE.md), as the PC version
had it: T (controls.chat) starts a line to everyone, Y (controls.team_chat)
one to the player's team; Enter sends it, Escape lets it go. The lines are
drawn above the motion sensor, each with its player's name in their team's
colour, and fade after a while.

A client sends its line to the host (_distributed_message_chat_up,
reliably), which takes it only from the client's own stream, from one of
that machine's players, at most CHAT_BURST lines at once and one each
CHAT_REFILL_MILLISECONDS after, its text cut to printable ASCII. The host
sends it on to every machine that hears it (_distributed_message_chat_down,
reliably): all of them, or a team's line only those with a player of the
team (every machine in a game without teams). A client shows only what the
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
#include "interface/hud_messaging.h"
#include "main/console.h"
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
	/* the caret's half period while typing */
	CHAT_CARET_MILLISECONDS = 500,
	/* a key's Ctrl (struct key_stroke's modifier_flags: menu_functions.c's) */
	CHAT_KEY_MODIFIER_CONTROL_BIT = 1,
};

/* the line's flags */
enum
{
	/* to the sender's team alone */
	_chat_line_team_bit,
};

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
	boolean team;
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
	boolean team;
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

/* whether a listener of a team hears a line: one to everyone, a team's in a
game without teams, else a team's by the same team alone */
static boolean chat_hears(
	boolean team_line,
	boolean has_teams,
	long sender_team,
	long listener_team)
{
	return !team_line || !has_teams || sender_team == listener_team;
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
	boolean team,
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
	line->team = team;
	line->time = system_milliseconds();
	/* (debug.txt has the lines, in ASCII) */
	{
		char name[CHAT_NAME_SIZE];

		for (index = 0; line->name[index]; index++)
			name[index] = player_name_character_ascii(line->name[index]);
		name[index] = 0;
		error(_error_log, "chat%s: %s: %s", team ? " (team)" : "", name, line->text);
	}
}

/* ---------- the host */

static boolean chat_host(
	void)
{
	return game_connection() == _game_connection_network_server;
}

/* (the host) a player's line, sent on to every machine that hears it and
shown here if a player of this machine's does */
static void chat_host_send(
	short player_index,
	boolean team,
	char const *text)
{
	struct player_datum *sender = distributed_player(player_index);
	boolean has_teams = game_engine_has_teams();
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
	csmemset(&message, 0, sizeof(message));
	message.line.player_index = distributed_player_to_byte(player_index);
	if (team)
		message.line.flags |= FLAG(_chat_line_team_bit);
	csstrncpy(message.line.text, text, CHAT_TEXT_SIZE - 1);
	for (index = 0; index < count; index++)
	{
		struct data_iterator iterator;
		struct player_datum *player;
		boolean hears = FALSE;

		data_iterator_new(&iterator, player_data);
		while (!hears && (player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		{
			hears = !player->quit_out_of_game &&
				distributed_machine_has_player(machine_indices[index],
					(short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index)) &&
				chat_hears(team, has_teams, sender->team_index, player->team_index);
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
			chat_hears(team, has_teams, sender->team_index, player_get(local_index)->team_index))
		{
			chat_add_line(player_index, team, text);
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
	chat_host_send(line.player_index, TEST_FLAG(line.flags, _chat_line_team_bit), text);
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
	chat_add_line(line.player_index, TEST_FLAG(line.flags, _chat_line_team_bit), text);
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
	boolean team,
	char const *typed)
{
	char text[CHAT_TEXT_SIZE];
	short player_index = chat_speaker();

	if (player_index == NONE || !chat_text_clean(text, typed, CHAT_TEXT_SIZE))
		return;
	if (chat_host())
	{
		chat_host_send(player_index, team, text);
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
		if (team)
			message.line.flags |= FLAG(_chat_line_team_bit);
		csstrncpy(message.line.text, text, CHAT_TEXT_SIZE - 1);
		distributed_send(&message, _distributed_message_chat_up, 1, (word)sizeof(message),
			_distributed_to_host_reliably);
	}
}

void network_chat_say(
	boolean team,
	char const *text)
{
	if (chat_network_game() && game_in_progress())
		chat_send(team, text);
}

static void chat_input_begin(
	boolean team)
{
	struct key_stroke key;

	csmemset(&chat_input, 0, sizeof(chat_input));
	chat_input.active = TRUE;
	chat_input.team = team;
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
			chat_input_begin(!TEST_FLAG(pressed, HALO_KEYBOARD_CHAT));
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
			chat_send(chat_input.team, chat_input.text);
			chat_input_end();
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

static void chat_color(
	short color,
	real alpha,
	real_argb_color *result)
{
	result->alpha = alpha;
	if (color == _chat_color_red)
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

/* the message's colour: a pale blue */
static void chat_text_color(
	real alpha,
	real_argb_color *result)
{
	result->alpha = alpha;
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
	char const *text,
	real alpha)
{
	wchar_t wide[CHAT_TEXT_SIZE + 2];
	rectangle2d name_bounds;
	rectangle2d cursor_bounds;
	rectangle2d text_bounds = *bounds;
	real_argb_color color;
	short index;

	draw_string_set_draw_mode(font_index, NONE, 0 /* left */, 0, name_color);
	draw_unicode_string_compute_bounds(bounds, name, &name_bounds, &cursor_bounds);
	for (index = 0; text[index] && index < CHAT_TEXT_SIZE + 1; index++)
		wide[index] = (wchar_t)(unsigned char)text[index];
	wide[index] = 0;
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

/* a line's name as drawn: "Name:", or "Name (team):" */
static void chat_line_name(
	struct chat_line const *line,
	wchar_t *name,
	short size)
{
	wchar_t const *suffix = line->team ? L" (team):" : L":";
	short length = 0;
	short index;

	for (index = 0; line->name[index] && length < size - 1; index++)
		name[length++] = line->name[index];
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
	short bottom;
	short shown;
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
	/* (above the motion sensor, a little in from its left; where it has not
	been drawn, where it would be) */
	if (chat_motion_sensors[local_player_index].valid)
	{
		left = chat_motion_sensors[local_player_index].left + line_height / 2;
		bottom = chat_motion_sensors[local_player_index].top - line_height / 2;
	}
	else
	{
		left = window.x0 + 2 * line_height;
		bottom = window.y1 - 9 * line_height;
	}
	if (typing)
	{
		real_argb_color color;
		rectangle2d bounds;
		char text[CHAT_TEXT_SIZE + 2];
		short length;

		bounds.x0 = left;
		bounds.x1 = window.x1;
		bounds.y1 = bottom;
		bounds.y0 = (short)(bottom - line_height);
		csstrcpy(text, chat_input.text);
		length = (short)csstrlen(text);
		if ((now / CHAT_CARET_MILLISECONDS) % 2 == 0)
		{
			text[length] = '_';
			text[length + 1] = 0;
		}
		chat_color(_chat_color_blue, 1.0f, &color);
		chat_draw_line(font_index, &bounds, chat_input.team ? L"Say to team:" : L"Say:", &color, text, 1.0f);
		bottom = bounds.y0;
	}
	for (shown = 0, index = 0; index < chat_line_count && shown < CHAT_LINES_SHOWN; index++)
	{
		struct chat_line const *line =
			&chat_lines[(chat_newest_line - index + CHAT_MAXIMUM_LINES) % CHAT_MAXIMUM_LINES];
		real alpha = chat_line_alpha(now - line->time, typing);
		wchar_t name[CHAT_NAME_SIZE + 16];
		real_argb_color color;
		rectangle2d bounds;

		/* (the older ones have faded too) */
		if (alpha <= 0.0f)
			break;
		chat_line_name(line, name, NUMBEROF(name));
		bounds.x0 = left;
		bounds.x1 = window.x1;
		bounds.y1 = bottom;
		bounds.y0 = (short)(bottom - line_height);
		chat_color(line->color, alpha, &color);
		chat_draw_line(font_index, &bounds, name, &color, line->text, alpha);
		bottom = bounds.y0;
		shown++;
	}
}
