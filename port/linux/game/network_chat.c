/*
NETWORK_CHAT.C

Text chat in network games, multiplayer and co-op, in the lobby before and
after a game too (Settings > Network Setup > Text Chat):

- T opens a line at the left of the screen, a little below the middle; the
  keys type into it and move nothing (xinput_sdl.c's platform_game_typing).
  Enter sends it (an empty one closes), Escape cancels, and Tab sends it to
  the player's team only or to all (a gametype with teams; co-op and a
  gametype without teams have one choice, all). The choice is shown before
  the line, [All] or [Team], in the menus' font and color.
- The lines come above it, in the HUD's font: the player's name, then what
  they said. In a gametype without teams the name is in the player's armor
  color; with teams, red or blue for the team. A line to a team only is
  marked so. Lines fade after a while, and come back while typing.
- game.text_chat: whether this machine shows chat and opens it at all;
  game.text_chat_opacity, how opaque it is drawn; game.text_chat_size, how
  large (large: the HUD's own size).

Who said a line is the machine it came from (its first player in the
network game's players), so a line has the same name in the lobby and in
the game. The host decides who is sent a line: a client sends its line to
the host reliably, which the host takes only from that client's own stream,
no longer than a line and no faster than a few a second; the host sends it
on, reliably, to every machine (a team's line, to its team's), its own lines
too. A line holds printable characters only. Machines whose game.text_chat
is off are still sent lines, and show them once it is on.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "memory/data.h"
#include "bungie_net/common/message_header.h"
#include "cutscene/cinematics.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "input/input.h"
#include "interface/hud.h"
#include "interface/hud_messaging.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "saved games/player_profile.h"
#include "tag_files/tag_groups.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "text/unicode.h"
#include "network_distributed.h"
#include "network_chat.h"

#include <string.h>

/* port_config.c's, xinput_sdl.c's, cseries_windows.c's, network_distributed.c's,
cache_files.c's */
int config_boolean(const char *name);
long config_integer(const char *name);
const char *config_string(const char *name);
void platform_game_typing(int typing);
int platform_chat_key(void);
unsigned long system_milliseconds(void);
boolean network_distributed_client_send_reliably(void *message, word size);
boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
short network_distributed_server_machines(long *machine_indices, short maximum);
long tag_loaded(long group_tag, char const *name);

/* ---------- constants */

enum
{
	/* a line's characters at most */
	CHAT_TEXT_LENGTH = 80,
	/* the lines kept, and shown while typing */
	CHAT_LINES = 8,
	/* the lines shown when not typing */
	CHAT_LINES_SHOWN = 6,
	/* a line shows this long, then fades over CHAT_FADE_MS (ms) */
	CHAT_SHOW_MS = 10000,
	CHAT_FADE_MS = 1000,
	/* (the host) a client's lines: a burst of this many, then one every
	CHAT_INTERVAL_MS */
	CHAT_BURST = 4,
	CHAT_INTERVAL_MS = 750,
	CHAT_MACHINES = HALO_PORT_MAXIMUM_NETWORK_MACHINES,
	CHAT_NO_MACHINE = 0xFF,
};

/* the menus' font, and their labels' color (#2896FF) */
#define CHAT_MENU_FONT "ui\\large_ui"
#define CHAT_MENU_RED (0x28 / 255.0f)
#define CHAT_MENU_GREEN (0x96 / 255.0f)
#define CHAT_MENU_BLUE (0xFF / 255.0f)

/* ---------- structures */

/* a line: to the host from a client (its machine is the connection's), and
from the host to the machines that are sent it */
struct distributed_chat
{
	/* the machine that said it (CHAT_NO_MACHINE to the host) */
	byte machine_index;
	/* TRUE: to the speaker's team only */
	byte team_only;
	byte length;
	byte pad;
	word text[CHAT_TEXT_LENGTH];
};

struct distributed_chat_message
{
	struct distributed_message_header header;
	struct distributed_chat entry;
};

typedef char distributed_chat_size_assert[sizeof(struct distributed_chat) == 4 + 2 * CHAT_TEXT_LENGTH ? 1 : -1];

struct chat_line
{
	wchar_t name[12];
	real_argb_color name_color;
	boolean team_only;
	wchar_t text[CHAT_TEXT_LENGTH + 1];
	unsigned long time;
};

/* ---------- globals */

static struct chat_line chat_lines[CHAT_LINES];
static short chat_line_count = 0;

/* what is being typed */
static struct
{
	boolean open;
	boolean team_only;
	char text[CHAT_TEXT_LENGTH + 1];
	short length;
} chat_typing = { FALSE, FALSE, "", 0 };

/* (the host) each client's allowance of lines, and when it was topped up */
static real chat_allowance[CHAT_MACHINES];
static unsigned long chat_allowance_at[CHAT_MACHINES];
static boolean chat_allowance_set[CHAT_MACHINES];

/* ---------- private code */

static boolean chat_enabled(
	void)
{
	return config_boolean("game.text_chat") != 0;
}

/* game.text_chat_opacity: 25 to 100 (percent) */
static real chat_opacity(
	void)
{
	long percent = config_integer("game.text_chat_opacity");

	return PIN(percent, 10, 100) / 100.0f;
}

/* game.text_chat_size: small, medium or large (the HUD's own size) */
static real chat_scale(
	void)
{
	char const *size = config_string("game.text_chat_size");

	if (size && !strcmp(size, "small"))
		return 0.6f;
	if (size && !strcmp(size, "medium"))
		return 0.8f;
	return 1.0f;
}

/* the network game this machine is in (the host's too), or NULL: the lobby,
the game and its postgame alike */
static struct network_game *chat_network_game(
	void)
{
	struct network_game_client *client = global_network_game_client_get();

	return client ? network_game_client_get_game(client) : NULL;
}

static boolean chat_host(
	void)
{
	return global_network_game_server_get() != NULL;
}

static long chat_local_machine(
	void)
{
	return network_game_client_get_local_machine_index();
}

/* a machine's first player in the network game, or NULL */
static struct network_player *chat_machine_player(
	struct network_game *game,
	long machine_index)
{
	short index;

	for (index = 0; game && index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		struct network_player *player = &game->players[index];

		if (network_player_is_valid(player) && player->machine_index == machine_index)
			return player;
	}
	return NULL;
}

/* a gametype with teams (co-op's one team, and a gametype without teams:
none) */
static boolean chat_has_teams(
	void)
{
	struct network_game *game = chat_network_game();

	if (!game || game->cooperative_flags)
		return FALSE;
	if (game_in_progress() && !game_engine)
		return FALSE;
	return game->variant.universal_variant.teams != 0;
}

static void chat_close(
	void)
{
	if (chat_typing.open)
		platform_game_typing(FALSE);
	chat_typing.open = FALSE;
	chat_typing.length = 0;
	chat_typing.text[0] = 0;
}

/* the color a player's name is in: the team's with teams, else the player's
armor's */
static void chat_name_color(
	struct network_player const *player,
	real_argb_color *color)
{
	color->alpha = 1.0f;
	if (chat_has_teams())
	{
		if (player->team_index == 0)
		{
			color->red = 1.0f;
			color->green = 0.3f;
			color->blue = 0.25f;
		}
		else
		{
			color->red = 0.35f;
			color->green = 0.55f;
			color->blue = 1.0f;
		}
	}
	else
	{
		real_rgb_color armor = player_profile_get_rgb_color(player->primary_color_index);

		/* (a dark armor, as black, lifted to be read) */
		color->red = MAX(armor.red, 0.35f);
		color->green = MAX(armor.green, 0.35f);
		color->blue = MAX(armor.blue, 0.35f);
	}
}

/* a line kept to be shown, from a machine */
static void chat_add_line(
	long machine_index,
	boolean team_only,
	word const *text,
	short length)
{
	struct network_player *player = chat_machine_player(chat_network_game(), machine_index);
	struct chat_line *line;
	short index;

	if (!player)
		return;
	if (chat_line_count == CHAT_LINES)
	{
		for (index = 1; index < CHAT_LINES; index++)
			chat_lines[index - 1] = chat_lines[index];
		chat_line_count--;
	}
	line = &chat_lines[chat_line_count++];
	csmemset(line, 0, sizeof(*line));
	csmemcpy(line->name, player->name, sizeof(line->name));
	line->name[NUMBEROF(line->name) - 1] = 0;
	chat_name_color(player, &line->name_color);
	line->team_only = team_only;
	for (index = 0; index < length && index < CHAT_TEXT_LENGTH; index++)
		line->text[index] = (wchar_t)text[index];
	line->text[index] = 0;
	line->time = system_milliseconds();
}

/* a line's text made safe: printable characters, no more than a line, the
spaces at its ends dropped; its length (0: nothing to say) */
static short chat_clean(
	word *text,
	short length)
{
	short from;
	short to = 0;

	length = (short)PIN(length, 0, CHAT_TEXT_LENGTH);
	for (from = 0; from < length; from++)
	{
		word character = text[from];

		if (character < 32 || character == 127)
			continue;
		if (character == ' ' && to == 0)
			continue;
		text[to++] = character;
	}
	while (to > 0 && text[to - 1] == ' ')
		to--;
	return to;
}

/* a message's headers: the transport's (its size: the connection checks it)
and the distributed netcode's (its time the game's, none in the lobby) */
static void chat_fill_message(
	struct distributed_chat_message *message,
	byte type,
	long machine_index,
	boolean team_only,
	word const *text,
	short length)
{
	csmemset(message, 0, sizeof(*message));
	build_message_header(&message->header.header, (word)sizeof(*message), 2, 0);
	message->header.type = type;
	message->header.count = 1;
	message->header.game_time = game_in_progress() ? game_time_get() : 0;
	message->entry.machine_index = machine_index >= 0 && machine_index < CHAT_MACHINES ?
		(byte)machine_index : CHAT_NO_MACHINE;
	message->entry.team_only = (byte)team_only;
	message->entry.length = (byte)length;
	csmemcpy(message->entry.text, text, length * sizeof(word));
}

/* whether a machine has a player on the team */
static boolean chat_machine_on_team(
	struct network_game *game,
	long machine_index,
	short team_index)
{
	short index;

	for (index = 0; game && index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		struct network_player *player = &game->players[index];

		if (network_player_is_valid(player) && player->machine_index == machine_index &&
			player->team_index == team_index)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/* (the host) a machine's line sent on to the machines that may read it, and
shown here if this machine may */
static void chat_host_send(
	long speaker_machine,
	boolean team_only,
	word *text,
	short length)
{
	struct network_game *game = chat_network_game();
	struct network_player *speaker = chat_machine_player(game, speaker_machine);
	struct distributed_chat_message message;
	long machine_indices[CHAT_MACHINES];
	short machine_count;
	short index;
	long local_machine = chat_local_machine();

	if (!speaker)
		return;
	length = chat_clean(text, length);
	if (length <= 0)
		return;
	team_only = team_only && chat_has_teams();
	chat_fill_message(&message, _distributed_message_chat, speaker_machine, team_only, text, length);
	machine_count = network_distributed_server_machines(machine_indices, CHAT_MACHINES);
	for (index = 0; index < machine_count; index++)
	{
		long machine_index = machine_indices[index];

		if (machine_index == local_machine)
			continue;
		if (!team_only || chat_machine_on_team(game, machine_index, speaker->team_index))
			network_distributed_server_send_to_machine_reliably(machine_index, &message, (word)sizeof(message));
	}
	if (!team_only || chat_machine_on_team(game, local_machine, speaker->team_index))
		chat_add_line(speaker_machine, team_only, text, length);
}

/* what this machine typed: shown and sent on (the host), or sent to the
host */
static void chat_send(
	void)
{
	word text[CHAT_TEXT_LENGTH];
	short length = chat_typing.length;
	short index;

	if (length <= 0)
		return;
	for (index = 0; index < length; index++)
		text[index] = (word)(unsigned char)chat_typing.text[index];
	if (chat_host())
	{
		chat_host_send(chat_local_machine(), chat_typing.team_only, text, length);
	}
	else
	{
		struct distributed_chat_message message;

		length = chat_clean(text, length);
		if (length <= 0)
			return;
		chat_fill_message(&message, _distributed_message_chat_up, NONE,
			chat_typing.team_only && chat_has_teams(), text, length);
		network_distributed_client_send_reliably(&message, (word)sizeof(message));
	}
}

/* whether chat may be typed and shown now */
static boolean chat_available(
	void)
{
	return chat_network_game() != NULL && chat_machine_player(chat_network_game(), chat_local_machine()) != NULL;
}

/* the keys typed into the line, and T to open it */
static void chat_update_typing(
	void)
{
	struct key_stroke key;
	int open_key = platform_chat_key();

	if (!chat_available() || !chat_enabled())
	{
		chat_close();
		return;
	}
	if (!chat_typing.open)
	{
		if (!open_key)
			return;
		chat_typing.open = TRUE;
		chat_typing.length = 0;
		chat_typing.text[0] = 0;
		chat_typing.team_only = FALSE;
		platform_game_typing(TRUE);
		/* (not the T that opened it) */
		while (input_get_key(&key))
		{
		}
		return;
	}
	if (!chat_has_teams())
		chat_typing.team_only = FALSE;
	while (chat_typing.open && input_get_key(&key))
	{
		switch (key.key_code)
		{
		case _key_escape:
			chat_close();
			break;
		case _key_return:
		case _keypad_enter:
			if (chat_typing.length > 0)
				chat_send();
			chat_close();
			break;
		case _key_tab:
			if (chat_has_teams())
				chat_typing.team_only = !chat_typing.team_only;
			break;
		case _key_backspace:
			if (chat_typing.length > 0)
				chat_typing.text[--chat_typing.length] = 0;
			break;
		default:
			if (key.ascii_code >= 32 && key.ascii_code < 127 && chat_typing.length < CHAT_TEXT_LENGTH)
			{
				chat_typing.text[chat_typing.length++] = key.ascii_code;
				chat_typing.text[chat_typing.length] = 0;
			}
			break;
		}
	}
}

/* the HUD's font, else the menus' (the main menu's map may have no HUD) */
static long chat_text_font(
	long menu_font)
{
	long font_index = hud_globals ? hud_get_font_index() : NONE;

	return font_index != NONE ? font_index : menu_font;
}

/* a string drawn from the left of the bounds; where it ended */
static short chat_draw_text(
	rectangle2d const *bounds,
	wchar_t const *text,
	long font_index,
	real_argb_color const *color)
{
	rectangle2d text_bounds;
	rectangle2d cursor_bounds;

	draw_string_set_draw_mode(font_index, NONE, 0, 0, color);
	rasterizer_draw_unicode_string(bounds, NULL, NULL, 0, text);
	draw_unicode_string_compute_bounds(bounds, text, &text_bounds, &cursor_bounds);
	return cursor_bounds.x0;
}

static short chat_line_height(
	long font_index)
{
	struct font_header *font = font_definition_get(font_index);

	return (short)(font->leading_height + font->descending_height + font->ascending_height);
}

static void chat_draw(
	void)
{
	rectangle2d screen = render.camera.window_bounds;
	long menu_font = tag_loaded(FONT_GROUP_TAG, CHAT_MENU_FONT);
	long font_index = chat_text_font(menu_font);
	unsigned long now = system_milliseconds();
	real opacity = chat_opacity();
	real scale = chat_scale();
	short line_height;
	short left;
	short bottom;
	short width;
	short first;
	short index;
	short row;

	if (font_index == NONE)
		return;
	if (menu_font == NONE)
		menu_font = font_index;
	offset_rectangle2d(&screen, -render.camera.viewport_bounds.x0, -render.camera.viewport_bounds.y0);
	line_height = chat_line_height(font_index);
	if (line_height <= 0)
		return;
	left = (short)(screen.x0 + 16);
	/* (laid out at the HUD's size, and drawn scaled about the typed line's
	top left) */
	width = (short)(MIN(460, (screen.x1 - screen.x0) - 32) / scale);
	/* (the typed line's top, a little below the middle) */
	bottom = (short)(screen.y0 + (screen.y1 - screen.y0) * 58 / 100);
	first = (short)MAX(0, chat_line_count - (chat_typing.open ? CHAT_LINES : CHAT_LINES_SHOWN));

	if (chat_typing.open)
	{
		rectangle2d backdrop;
		long alpha = (long)(0x90 * opacity);

		backdrop.x0 = (short)(left - 6);
		backdrop.x1 = (short)(left + width * scale);
		backdrop.y0 = (short)(bottom - ((chat_line_count - first) * line_height + 4) * scale);
		backdrop.y1 = (short)(bottom + (line_height + 4) * scale);
		draw_quad(&backdrop, (pixel32)((alpha << 24) | 0x101010));
	}
	rasterizer_text_set_scale(scale, left, bottom);
	for (index = first, row = (short)(chat_line_count - first); index < chat_line_count; index++, row--)
	{
		struct chat_line *line = &chat_lines[index];
		unsigned long age = now - line->time;
		real alpha = opacity;
		real_argb_color color;
		rectangle2d bounds;
		wchar_t prefix[32];

		if (!chat_typing.open)
		{
			if (age >= CHAT_SHOW_MS + CHAT_FADE_MS)
				continue;
			if (age > CHAT_SHOW_MS)
				alpha *= 1.0f - (real)(age - CHAT_SHOW_MS) / CHAT_FADE_MS;
		}
		bounds.x0 = left;
		bounds.x1 = (short)(left + width);
		bounds.y0 = (short)(bottom - row * line_height);
		bounds.y1 = (short)(bounds.y0 + line_height);
		color.alpha = alpha;
		color.red = color.green = color.blue = 0.75f;
		if (line->team_only)
		{
			usprintf(prefix, L"(Team) ");
			bounds.x0 = chat_draw_text(&bounds, prefix, font_index, &color);
		}
		color = line->name_color;
		color.alpha = alpha;
		bounds.x0 = chat_draw_text(&bounds, line->name, font_index, &color);
		color.red = color.green = color.blue = 0.95f;
		usprintf(prefix, L": ");
		bounds.x0 = chat_draw_text(&bounds, prefix, font_index, &color);
		chat_draw_text(&bounds, line->text, font_index, &color);
	}
	if (chat_typing.open)
	{
		real_argb_color color;
		rectangle2d bounds;
		wchar_t text[CHAT_TEXT_LENGTH + 32];
		short length;
		short index;

		bounds.x0 = left;
		bounds.x1 = (short)(left + width);
		bounds.y0 = bottom;
		bounds.y1 = (short)(bottom + line_height);
		/* (the choice in the menus' font and color) */
		color.alpha = opacity;
		color.red = CHAT_MENU_RED;
		color.green = CHAT_MENU_GREEN;
		color.blue = CHAT_MENU_BLUE;
		usprintf(text, chat_typing.team_only ? L"[Team]  " : L"[All]  ");
		bounds.x0 = chat_draw_text(&bounds, text, menu_font, &color);
		for (index = 0; index < chat_typing.length; index++)
			text[index] = (wchar_t)(unsigned char)chat_typing.text[index];
		length = chat_typing.length;
		text[length++] = (now / 500) % 2 ? L'_' : L' ';
		text[length] = 0;
		color.red = color.green = color.blue = 1.0f;
		chat_draw_text(&bounds, text, font_index, &color);
	}
	rasterizer_text_set_scale(1.0f, 0.0f, 0.0f);
	draw_string_set_draw_mode(font_index, NONE, 0, 0, global_real_argb_white);
}

/* (the host) a client's line */
static void chat_handle_up(
	long machine_index,
	struct distributed_chat *entry,
	boolean from_stream)
{
	unsigned long now = system_milliseconds();

	if (!chat_host() || !from_stream || machine_index < 0 || machine_index >= CHAT_MACHINES ||
		entry->length == 0 || entry->length > CHAT_TEXT_LENGTH)
	{
		return;
	}
	/* (no faster than its allowance: more are dropped, untold) */
	if (!chat_allowance_set[machine_index])
	{
		chat_allowance_set[machine_index] = TRUE;
		chat_allowance[machine_index] = CHAT_BURST;
		chat_allowance_at[machine_index] = now;
	}
	chat_allowance[machine_index] += (real)(now - chat_allowance_at[machine_index]) / CHAT_INTERVAL_MS;
	chat_allowance[machine_index] = MIN(chat_allowance[machine_index], (real)CHAT_BURST);
	chat_allowance_at[machine_index] = now;
	if (chat_allowance[machine_index] < 1.0f)
		return;
	chat_allowance[machine_index] -= 1.0f;
	/* (whoever the client says spoke, it was its own machine) */
	chat_host_send(machine_index, entry->team_only != 0, entry->text, entry->length);
}

/* (a client) a line the host sent on */
static void chat_handle(
	struct distributed_chat *entry)
{
	short length;

	if (chat_host() || entry->machine_index == CHAT_NO_MACHINE)
		return;
	length = chat_clean(entry->text, entry->length);
	if (length > 0)
		chat_add_line(entry->machine_index, entry->team_only != 0, entry->text, length);
}

/* ---------- public code */

void network_chat_render(
	void)
{
	/* (out of a network game: nothing kept for the next) */
	if (!chat_network_game())
	{
		chat_close();
		chat_line_count = 0;
		csmemset(chat_allowance_set, 0, sizeof(chat_allowance_set));
		platform_chat_key();
		return;
	}
	chat_update_typing();
	if (!chat_available() || !chat_enabled())
		return;
	if (chat_line_count > 0 || chat_typing.open)
		chat_draw();
}

boolean network_chat_handles_message(
	word const *message,
	word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	csmemcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_chat_up || header.type == _distributed_message_chat;
}

void network_chat_handle_message(
	long machine_index,
	word const *message,
	word size,
	boolean from_stream)
{
	struct distributed_message_header header;
	struct distributed_chat entry;

	if (!network_chat_handles_message(message, size) || size < sizeof(header) + sizeof(entry))
		return;
	csmemcpy(&header, message, sizeof(header));
	csmemcpy(&entry, (byte const *)message + sizeof(header), sizeof(entry));
	if (header.type == _distributed_message_chat_up)
	{
		if (machine_index != NONE)
			chat_handle_up(machine_index, &entry, from_stream);
	}
	else if (machine_index == NONE)
	{
		chat_handle(&entry);
	}
}
