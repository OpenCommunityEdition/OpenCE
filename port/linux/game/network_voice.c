/*
NETWORK_VOICE.C

Voice chat (port/linux/NETCODE.md): each machine's microphone, encoded with
Opus (port/linux/src/voice_audio.c), goes to the host, which sends it on to
the machines that may hear it. The host decides everything, from its own
config.toml:

- In the lobby, before and after a game (and in a game's postgame), every
  player hears every other (network.voice_lobby).
- In a game, network.voice_mode: off; teammates near; anyone near; all
  teammates; or all teammates and enemies near ("near": within
  network.voice_proximity world units of each other, both alive). A game
  without teams has only enemies, and co-op only teammates. A voice heard
  for being near is quieter further away, and comes from the speaker's side
  (each listener's own reckoning of where everyone is).
- The quality: network.voice_kbps (8 to 64), in the lobby and in a game,
  which the host tells each machine to encode at, and holds it to.

What the host takes is held to that too: a machine's frames only with the
key the host gave it over its stream (a datagram's source address is all
that names its machine, and anyone can send one as from another's), no
larger than the quality allows, no more than VOICE_FRAMES_PER_SECOND (with
a little burst), only while voice is on, and no more than
VOICE_MAXIMUM_TALKERS machines at once. A machine is sent voices once it
has said it hears them (a frame, or an empty one every two seconds), so a
build without voice chat is never sent any. The host relays only: it never
decodes, and a machine's own voice never comes back to it.

Each machine decides only for itself: whether it talks (audio.voice_chat:
push to talk, open mic, or not), how loud the others are
(audio.voice_volume), and whom it mutes (the scoreboard's menu,
game_engine.c), which it does not tell anyone.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "memory/data.h"
#include "math/integer_math.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmap_group_lookup.h"
#include "interface/ui_widget.h"
#include "tag_files/tag_groups.h"
#include "camera/observer.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "main/console.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "objects/objects.h"
#include "network_distributed.h"
#include "network_voice.h"
#include "../src/voice_audio.h"

#include <math.h>

/* port_config.c's, network_game_globals.c's, network_server_message_handler.c's,
xinput_sdl.c's, posix_net.c's, cseries_windows.c's */
int config_boolean(const char *name);
long config_integer(const char *name);
double config_real(const char *name);
const char *config_string(const char *name);
unsigned long config_changes(void);
boolean network_distributed_client_send(void *message, word size);
boolean network_distributed_server_send_to_machine(long machine_index, void *message, word size);
boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
short network_distributed_server_machines(long *machine_indices, short maximum);
int halo_push_to_talk_held(void);
void posix_random_bytes(void *buffer, unsigned int size);
unsigned long system_milliseconds(void);

/* ---------- constants */

enum
{
	VOICE_MACHINES = HALO_PORT_MAXIMUM_NETWORK_MACHINES,
	/* a frame every 20 ms, and a burst of a few (a client's frames sent
	in a clump after a stall) */
	VOICE_FRAMES_PER_SECOND = 50,
	VOICE_FRAME_BURST = 10,
	/* machines heard at once, at most */
	VOICE_MAXIMUM_TALKERS = 8,
	/* a machine talking: one whose frame came this lately (ms) */
	VOICE_TALKING_MS = 500,
	/* a machine that hears voices: one heard from this lately (ms) */
	VOICE_LISTENING_MS = 6000,
	/* how often a client says it hears voices, and the host repeats its
	settings (ms) */
	VOICE_HELLO_MS = 2000,
	VOICE_CONFIG_MS = 10000,
	/* a client forgets the host's settings unheard this long (ms) */
	VOICE_CONFIG_TIMEOUT_MS = 25000,
	/* a speaker's icon shows this long after their last frame (ms) */
	VOICE_SPEAKING_MS = 300,
	/* open mic: talking while the microphone's level is over this (about
	-40 dB), and for this long after (ms) */
	VOICE_OPEN_MIC_HOLD_MS = 400,
	VOICE_MINIMUM_KBPS = 8,
	VOICE_MAXIMUM_KBPS = 64,
};

/* the speaker's bitmap (network_voice_draw_icon): the menus' voice/speaker,
as their bitmaps are named (menu_tags.c) */
#define VOICE_ICON_BITMAP "pc\\voice\\speaker"
#define VOICE_OPEN_MIC_LEVEL 0.01f
/* proximity: full volume this near, quieter further (world units) */
#define VOICE_NEAR_DISTANCE 3.0f
#define VOICE_FARTHEST_GAIN 0.2f

/* how the host's settings travel */
enum
{
	_voice_config_lobby_bit,
};

/* network_client_manager.c's client state in game (its enum is its own) */
enum
{
	_voice_client_state_ingame = 3,
};

/* ---------- structures */

/* a client's frame to the host (its Opus packet follows; none: it hears
voices, and says so) */
struct distributed_voice_up
{
	unsigned long key;
	word sequence;
	byte length;
	byte pad;
};

/* a frame relayed to a client (the packet follows) */
struct distributed_voice_down
{
	unsigned long key;
	word sequence;
	/* the machine that spoke, and its first player (absolute; NO_PLAYER
	in the lobby) */
	byte machine_index;
	byte player_index;
	/* how it is heard (_voice_route_*) */
	byte route;
	byte length;
	byte pad[2];
};

/* the host's settings, to each client over its stream, with its key */
struct distributed_voice_config
{
	unsigned long key;
	byte flags;
	byte mode;
	byte kbps;
	byte pad0;
	word proximity_tenths;
	byte pad[2];
};

typedef char distributed_voice_up_size_assert[sizeof(struct distributed_voice_up) == 8 ? 1 : -1];
typedef char distributed_voice_down_size_assert[sizeof(struct distributed_voice_down) == 12 ? 1 : -1];
typedef char distributed_voice_config_size_assert[sizeof(struct distributed_voice_config) == 12 ? 1 : -1];

/* the host's settings as they are used */
struct voice_settings
{
	boolean lobby;
	short mode;
	short kbps;
	real proximity;
};

/* ---------- globals */

/* the host: each machine slot's key, whether it was told it, and when it
was told the settings last; when it was heard from; its frames' allowance
and when it was topped up; when its last frame was relayed */
static struct
{
	unsigned long key;
	boolean told;
	unsigned long told_at;
	unsigned long heard_at;
	boolean heard;
	real allowance;
	unsigned long allowance_at;
	unsigned long talked_at;
	boolean talked;
} voice_machines[VOICE_MACHINES];
static struct voice_settings voice_host_settings;
static unsigned long voice_host_settings_read_at = (unsigned long)-1;
/* (the settings told, to tell again when they change) */
static struct voice_settings voice_host_settings_told;

/* this machine: the host's settings and its key (heard: a client), when
they came; the sequence of its frames; whether it talked lately, and the
microphone was asked for */
static struct voice_settings voice_settings;
static unsigned long voice_key;
static boolean voice_settings_heard;
static unsigned long voice_settings_heard_at;
static word voice_sequence;
static unsigned long voice_hello_at;
static unsigned long voice_talked_at;
static boolean voice_talked;
static unsigned long voice_open_mic_until;
static boolean voice_microphone_wanted;
static boolean voice_session;

/* every machine slot's last frame heard (for its icon), and whether this
machine mutes it, with the names it had then (a new machine at the slot is
not muted) */
static unsigned long voice_heard_at[VOICE_MACHINES];
static boolean voice_heard[VOICE_MACHINES];
static boolean voice_muted[VOICE_MACHINES];
static wchar_t voice_muted_names[VOICE_MACHINES][12];

/* ---------- the route (pure: tools/harness/tests/voice_route.c) */

/* how a listener hears a speaker: in the lobby (and the postgame), anyone
if the lobby has voice; in a game, by the mode, teammates anywhere or near,
enemies near ("near": within the range, both alive) */
static short voice_route(
	short mode,
	boolean lobby,
	boolean lobby_voice,
	boolean teammates,
	boolean both_alive,
	real distance,
	real range)
{
	boolean near = both_alive && distance <= range;

	if (lobby)
		return lobby_voice ? _voice_route_global : _voice_route_none;
	switch (mode)
	{
	case _voice_mode_team_proximity:
		return teammates && near ? _voice_route_proximity : _voice_route_none;
	case _voice_mode_team_enemy_proximity:
		return near ? _voice_route_proximity : _voice_route_none;
	case _voice_mode_team_global:
		return teammates ? _voice_route_global : _voice_route_none;
	case _voice_mode_team_global_enemy_proximity:
		if (teammates)
			return _voice_route_global;
		return near ? _voice_route_proximity : _voice_route_none;
	default:
		return _voice_route_none;
	}
}

/* the largest packet a frame may be at the quality (kilobits a second):
twice its share, for Opus's variable rate, and a little */
static long voice_packet_limit(
	short kbps)
{
	long limit = (long)kbps * 1000 / 8 / VOICE_FRAMES_PER_SECOND * 2 + 16;

	return MIN(limit, VOICE_MAXIMUM_PACKET);
}

/* ---------- settings */

static short voice_mode_from_text(
	char const *text)
{
	static char const *const names[NUMBER_OF_VOICE_MODES] =
	{
		"off", "team_proximity", "team_enemy_proximity", "team_global", "team_global_enemy_proximity",
	};
	short mode;

	for (mode = 0; text && mode < NUMBER_OF_VOICE_MODES; mode++)
	{
		if (!csstrcmp(text, names[mode]))
			return mode;
	}
	return _voice_mode_team_global_enemy_proximity;
}

/* the host's, from its config.toml (read again as it changes) */
static void voice_host_settings_read(
	void)
{
	if (voice_host_settings_read_at == config_changes())
		return;
	voice_host_settings_read_at = config_changes();
	voice_host_settings.lobby = config_boolean("network.voice_lobby") != 0;
	voice_host_settings.mode = voice_mode_from_text(config_string("network.voice_mode"));
	voice_host_settings.kbps = (short)PIN(config_integer("network.voice_kbps"), VOICE_MINIMUM_KBPS,
		VOICE_MAXIMUM_KBPS);
	voice_host_settings.proximity = (real)PIN(config_real("network.voice_proximity"), 5.0, 100.0);
}

static boolean voice_host(
	void)
{
	return global_network_game_server_get() != NULL;
}

/* in the lobby, as voice has it: this machine's network game not in game
(the menus' own game, the main menu's, runs meanwhile), or in a multiplayer
game's postgame (co-op, with no game engine, has none) */
static boolean voice_in_lobby(
	void)
{
	struct network_game_client *client = global_network_game_client_get();
	short state_data;

	if (!client || network_game_client_get_state(client, &state_data) != _voice_client_state_ingame ||
		!game_in_progress())
	{
		return TRUE;
	}
	return game_engine && game_engine_showing_postgame();
}

/* ---------- players */

/* the network game this machine is in (the host's, its client's), or NULL */
static struct network_game *voice_network_game(
	void)
{
	struct network_game_client *client = global_network_game_client_get();

	return client ? network_game_client_get_game(client) : NULL;
}

/* a machine's first player in the game (its datum), or NULL */
static struct player_datum *voice_machine_player(
	long machine_index,
	long *player_index)
{
	struct data_iterator iterator;
	struct player_datum *player;

	if (!game_in_progress() || !player_data)
		return NULL;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (!player->quit_out_of_game && player->network_player_data.machine_index == machine_index)
		{
			if (player_index)
				*player_index = iterator.datum_index;
			return player;
		}
	}
	return NULL;
}

/* a machine's first player's name as the network game has it (empty if
none) */
static void voice_machine_name(
	long machine_index,
	wchar_t *name)
{
	struct network_game *game = voice_network_game();
	short index;

	csmemset(name, 0, 12 * sizeof(*name));
	for (index = 0; game && index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		struct network_player *player = &game->players[index];

		if (network_player_is_valid(player) && player->machine_index == machine_index)
		{
			csmemcpy(name, player->name, sizeof(player->name));
			return;
		}
	}
}

/* a player's living unit's position */
static boolean voice_player_position(
	struct player_datum const *player,
	real_point3d *position)
{
	long unit_index = player ? distributed_living_unit(player) : NONE;

	if (unit_index == NONE)
		return FALSE;
	object_get_origin(unit_index, position);
	return TRUE;
}

/* (the host) each machine's first player in the game (NULL: none), found
once for a frame's relaying */
static void voice_machine_players(
	struct player_datum **players)
{
	struct data_iterator iterator;
	struct player_datum *player;

	csmemset(players, 0, VOICE_MACHINES * sizeof(*players));
	if (voice_in_lobby() || !player_data)
		return;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		long machine_index = player->network_player_data.machine_index;

		if (!player->quit_out_of_game && machine_index >= 0 && machine_index < VOICE_MACHINES &&
			!players[machine_index])
		{
			players[machine_index] = player;
		}
	}
}

/* (the host) how a listener's machine hears a speaker's, now, by their
first players */
static short voice_route_between(
	struct player_datum *speaker,
	struct player_datum *listener)
{
	real_point3d speaker_position;
	real_point3d listener_position;
	boolean both_alive;
	boolean teammates;
	real distance = 0.0f;

	if (voice_in_lobby())
		return voice_route(voice_host_settings.mode, TRUE, voice_host_settings.lobby, FALSE, FALSE, 0.0f, 0.0f);
	if (!speaker || !listener)
		return _voice_route_none;
	/* (co-op: one team; a game without teams: none) */
	teammates = !game_engine || (game_engine_has_teams() && speaker->team_index == listener->team_index);
	both_alive = voice_player_position(speaker, &speaker_position) &&
		voice_player_position(listener, &listener_position);
	if (both_alive)
		distance = distance3d(&speaker_position, &listener_position);
	return voice_route(voice_host_settings.mode, FALSE, voice_host_settings.lobby, teammates, both_alive, distance,
		voice_host_settings.proximity);
}

/* a message's headers: the transport's (its size: the connection checks it)
and the distributed netcode's (its time the game's, none in the lobby) */
static void voice_fill_header(
	struct distributed_message_header *header,
	byte type,
	word size)
{
	header->header = 0;
	build_message_header(&header->header, size, 2, 0);
	header->type = type;
	header->count = 1;
	header->game_time = game_in_progress() ? game_time_get() : 0;
}

/* ---------- the host */

static boolean voice_machine_valid(
	long machine_index)
{
	return machine_index >= 0 && machine_index < VOICE_MACHINES;
}

/* (network_server_manager.c) a machine joined at the slot: a new key, told
to it before anything is taken from it */
void network_voice_machine_joined(
	long machine_index)
{
	if (!voice_machine_valid(machine_index))
		return;
	csmemset(&voice_machines[machine_index], 0, sizeof(voice_machines[machine_index]));
	do
	{
		posix_random_bytes(&voice_machines[machine_index].key, sizeof(voice_machines[machine_index].key));
	} while (!voice_machines[machine_index].key);
}

/* the settings and its key, to one client, over its stream */
static void voice_host_tell(
	long machine_index)
{
	struct
	{
		struct distributed_message_header header;
		struct distributed_voice_config config;
	} message;

	csmemset(&message, 0, sizeof(message));
	voice_fill_header(&message.header, _distributed_message_voice_config, (word)sizeof(message));
	message.config.key = voice_machines[machine_index].key;
	message.config.flags = voice_host_settings.lobby ? FLAG(_voice_config_lobby_bit) : 0;
	message.config.mode = (byte)voice_host_settings.mode;
	message.config.kbps = (byte)voice_host_settings.kbps;
	message.config.proximity_tenths = (word)(voice_host_settings.proximity * 10.0f);
	if (network_distributed_server_send_to_machine_reliably(machine_index, &message, (word)sizeof(message)))
	{
		voice_machines[machine_index].told = TRUE;
		voice_machines[machine_index].told_at = system_milliseconds();
	}
}

/* whether a machine is one this machine plays a voice of: not muted */
static boolean voice_audible(
	long machine_index)
{
	return voice_machine_valid(machine_index) && !voice_muted[machine_index];
}

/* the gain and pan of a voice heard for being near (this machine's own
reckoning: its first player's view, and the speaker's unit) */
static void voice_proximity_gain(
	short player_index,
	real *gain,
	real *pan)
{
	struct player_datum *player = player_index != NONE ? distributed_player(player_index) : NULL;
	struct observer_result const *camera;
	real_point3d position;
	real_vector3d direction;
	real_vector3d right;
	real distance;
	real range = voice_settings.proximity > VOICE_NEAR_DISTANCE ? voice_settings.proximity : VOICE_NEAR_DISTANCE + 1.0f;

	*gain = 1.0f;
	*pan = 0.0f;
	if (!voice_player_position(player, &position) || local_player_get_next(NONE) == NONE)
		return;
	camera = observer_get_camera((short)local_player_get_next(NONE));
	if (!camera)
		return;
	vector_from_points3d(&camera->position, &position, &direction);
	distance = magnitude3d(&direction);
	if (distance > VOICE_NEAR_DISTANCE)
	{
		real t = PIN((distance - VOICE_NEAR_DISTANCE) / (range - VOICE_NEAR_DISTANCE), 0.0f, 1.0f);

		*gain = 1.0f - (1.0f - VOICE_FARTHEST_GAIN) * t;
	}
	if (distance > 0.01f)
	{
		cross_product3d(&camera->forward, &camera->up, &right);
		normalize3d(&right);
		scale_vector3d(&direction, 1.0f / distance, &direction);
		*pan = 0.8f * dot_product3d(&direction, &right);
	}
}

/* a frame heard here (the host's own listening, or one relayed to a
client); the speaker's first player (absolute index, NONE for none) */
static void voice_play(
	long machine_index,
	short player_index,
	short route,
	word sequence,
	byte const *packet,
	long length)
{
	real gain = 1.0f;
	real pan = 0.0f;

	if (!voice_machine_valid(machine_index) || length <= 0)
		return;
	voice_heard_at[machine_index] = system_milliseconds();
	voice_heard[machine_index] = TRUE;
	if (!voice_audible(machine_index))
		return;
	if (route == _voice_route_proximity)
		voice_proximity_gain(player_index, &gain, &pan);
	voice_audio_play((int)machine_index, sequence, packet, (int)length, gain, pan);
}

/* how many machines talk now, but the one */
static short voice_host_talkers(
	long except_machine,
	unsigned long now)
{
	short count = 0;
	long index;

	for (index = 0; index < VOICE_MACHINES; index++)
	{
		if (index != except_machine && voice_machines[index].talked &&
			now - voice_machines[index].talked_at < VOICE_TALKING_MS)
		{
			count++;
		}
	}
	return count;
}

/* (the host) a frame of a machine (its own, or a client's), checked, and
sent to those that hear it */
static void voice_host_relay(
	long speaker_machine,
	word sequence,
	byte const *packet,
	long length)
{
	static struct player_datum *players[VOICE_MACHINES];
	long machine_indices[VOICE_MACHINES];
	short count = network_distributed_server_machines(machine_indices, VOICE_MACHINES);
	short local_machine = network_game_client_get_local_machine_index();
	long speaker_player = NONE;
	unsigned long now = system_milliseconds();
	short index;

	if (!voice_machine_valid(speaker_machine) || length <= 0 ||
		length > voice_packet_limit(voice_host_settings.kbps))
	{
		return;
	}
	/* (no more than so many at once: those talking keep talking) */
	if (!(voice_machines[speaker_machine].talked && now - voice_machines[speaker_machine].talked_at < VOICE_TALKING_MS) &&
		voice_host_talkers(speaker_machine, now) >= VOICE_MAXIMUM_TALKERS)
	{
		return;
	}
	voice_machines[speaker_machine].talked = TRUE;
	voice_machines[speaker_machine].talked_at = now;
	voice_machine_players(players);
	/* (the lobby has no players' positions: the menus' game has players of
	its own) */
	if (!voice_in_lobby())
		voice_machine_player(speaker_machine, &speaker_player);
	for (index = 0; index < count; index++)
	{
		long listener = machine_indices[index];
		short route;

		if (!voice_machine_valid(listener) || listener == speaker_machine)
			continue;
		route = voice_route_between(players[speaker_machine], players[listener]);
		if (route == _voice_route_none)
			continue;
		if (listener == local_machine)
		{
			voice_play(speaker_machine, speaker_player != NONE ? (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(speaker_player) :
				NONE, route, sequence, packet, length);
			continue;
		}
		/* (only to a machine that has said it hears voices) */
		if (!voice_machines[listener].heard || now - voice_machines[listener].heard_at >= VOICE_LISTENING_MS ||
			!voice_machines[listener].told)
		{
			continue;
		}
		{
			struct
			{
				struct distributed_message_header header;
				struct distributed_voice_down down;
				byte packet[VOICE_MAXIMUM_PACKET];
			} message;

			csmemset(&message, 0, sizeof(message.header) + sizeof(message.down));
			voice_fill_header(&message.header, _distributed_message_voice_down,
				(word)(sizeof(message.header) + sizeof(message.down) + length));
			message.down.key = voice_machines[listener].key;
			message.down.sequence = sequence;
			message.down.machine_index = (byte)speaker_machine;
			message.down.player_index = speaker_player != NONE ?
				(byte)DATUM_INDEX_TO_ABSOLUTE_INDEX(speaker_player) : NO_PLAYER;
			message.down.route = (byte)route;
			message.down.length = (byte)length;
			csmemcpy(message.packet, packet, length);
			network_distributed_server_send_to_machine(listener, &message,
				(word)(sizeof(message.header) + sizeof(message.down) + length));
		}
	}
}

/* (the host) a client's frame: its key, its allowance, the settings' */
static void voice_host_handle_up(
	long machine_index,
	struct distributed_voice_up const *up,
	byte const *packet,
	long length)
{
	unsigned long now = system_milliseconds();

	if (!voice_machine_valid(machine_index) || !voice_machines[machine_index].key ||
		up->key != voice_machines[machine_index].key || up->length != length)
	{
		return;
	}
	voice_machines[machine_index].heard = TRUE;
	voice_machines[machine_index].heard_at = now;
	/* (an empty frame: it hears voices) */
	if (!length)
		return;
	/* (its allowance: so many frames a second, a few at once) */
	if (!voice_machines[machine_index].allowance_at)
		voice_machines[machine_index].allowance = VOICE_FRAME_BURST;
	else
	{
		voice_machines[machine_index].allowance += (real)(now - voice_machines[machine_index].allowance_at) *
			VOICE_FRAMES_PER_SECOND / 1000.0f;
	}
	voice_machines[machine_index].allowance = MIN(voice_machines[machine_index].allowance, (real)VOICE_FRAME_BURST);
	voice_machines[machine_index].allowance_at = now;
	if (voice_machines[machine_index].allowance < 1.0f)
		return;
	voice_machines[machine_index].allowance -= 1.0f;
	voice_host_relay(machine_index, up->sequence, packet, length);
}

/* ---------- messages (network_distributed.c) */

boolean network_voice_handles_message(
	word const *message,
	word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	csmemcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_voice_up || header.type == _distributed_message_voice_down ||
		header.type == _distributed_message_voice_config;
}

void network_voice_handle_message(
	long machine_index,
	word const *message,
	word size)
{
	struct distributed_message_header header;
	byte const *entries = (byte const *)message + sizeof(header);
	long length = (long)size - (long)sizeof(header);

	if (!network_voice_handles_message(message, size))
		return;
	csmemcpy(&header, message, sizeof(header));
	switch (header.type)
	{
	case _distributed_message_voice_up:
	{
		struct distributed_voice_up up;

		if (machine_index == NONE || !voice_host() || length < (long)sizeof(up))
			return;
		csmemcpy(&up, entries, sizeof(up));
		voice_host_handle_up(machine_index, &up, entries + sizeof(up), length - (long)sizeof(up));
		break;
	}
	case _distributed_message_voice_down:
	{
		struct distributed_voice_down down;

		if (machine_index != NONE || voice_host() || !voice_settings_heard || length < (long)sizeof(down))
			return;
		csmemcpy(&down, entries, sizeof(down));
		if (down.key != voice_key || down.length != length - (long)sizeof(down) ||
			down.machine_index == network_game_client_get_local_machine_index())
		{
			return;
		}
		voice_play(down.machine_index, down.player_index != NO_PLAYER && !voice_in_lobby() ? down.player_index : NONE,
			down.route, down.sequence, entries + sizeof(down), down.length);
		break;
	}
	case _distributed_message_voice_config:
	{
		struct distributed_voice_config config;

		if (machine_index != NONE || voice_host() || length < (long)sizeof(config))
			return;
		csmemcpy(&config, entries, sizeof(config));
		voice_key = config.key;
		voice_settings.lobby = TEST_FLAG(config.flags, _voice_config_lobby_bit);
		voice_settings.mode = config.mode < NUMBER_OF_VOICE_MODES ? config.mode : _voice_mode_off;
		voice_settings.kbps = (short)PIN(config.kbps, VOICE_MINIMUM_KBPS, VOICE_MAXIMUM_KBPS);
		voice_settings.proximity = config.proximity_tenths / 10.0f;
		voice_settings_heard = TRUE;
		voice_settings_heard_at = system_milliseconds();
		break;
	}
	}
}

/* ---------- every frame (main.c) */

/* whether this machine may talk now, as the host's settings have it */
static boolean voice_may_talk(
	void)
{
	if (voice_in_lobby())
		return voice_settings.lobby;
	return voice_settings.mode != _voice_mode_off;
}

/* the host: every client told its key and the settings (again as they
change, and now and then) */
static void voice_host_update(
	void)
{
	long machine_indices[VOICE_MACHINES];
	short count = network_distributed_server_machines(machine_indices, VOICE_MACHINES);
	short local_machine = network_game_client_get_local_machine_index();
	boolean changed;
	unsigned long now = system_milliseconds();
	short index;

	voice_host_settings_read();
	changed = csmemcmp(&voice_host_settings, &voice_host_settings_told, sizeof(voice_host_settings)) != 0;
	voice_host_settings_told = voice_host_settings;
	/* (the host's own, as a client hears them) */
	voice_settings = voice_host_settings;
	voice_settings_heard = TRUE;
	voice_settings_heard_at = now;
	for (index = 0; index < count; index++)
	{
		long machine_index = machine_indices[index];

		if (!voice_machine_valid(machine_index) || machine_index == local_machine)
			continue;
		if (!voice_machines[machine_index].key)
			network_voice_machine_joined(machine_index);
		if (!voice_machines[machine_index].told || changed ||
			now - voice_machines[machine_index].told_at >= VOICE_CONFIG_MS)
		{
			voice_host_tell(machine_index);
		}
	}
}

/* this machine's frame: the host's own relayed, a client's sent up (none:
it hears voices) */
static void voice_send(
	byte const *packet,
	long length)
{
	if (voice_host())
	{
		if (length > 0)
			voice_host_relay(network_game_client_get_local_machine_index(), voice_sequence, packet, length);
	}
	else
	{
		struct
		{
			struct distributed_message_header header;
			struct distributed_voice_up up;
			byte packet[VOICE_MAXIMUM_PACKET];
		} message;

		csmemset(&message, 0, sizeof(message.header) + sizeof(message.up));
		voice_fill_header(&message.header, _distributed_message_voice_up,
			(word)(sizeof(message.header) + sizeof(message.up) + length));
		message.up.key = voice_key;
		message.up.sequence = voice_sequence;
		message.up.length = (byte)length;
		if (length > 0)
			csmemcpy(message.packet, packet, length);
		network_distributed_client_send(&message, (word)(sizeof(message.header) + sizeof(message.up) + length));
	}
	if (length > 0)
		voice_sequence++;
}

/* the session over (or none): the microphone closed, voices forgotten */
static void voice_end_session(
	void)
{
	if (!voice_session)
		return;
	voice_session = FALSE;
	voice_microphone_wanted = FALSE;
	voice_audio_microphone(FALSE);
	voice_audio_forget_all();
	voice_settings_heard = FALSE;
	voice_talked = FALSE;
	csmemset(voice_heard, 0, sizeof(voice_heard));
}

void network_voice_update(
	void)
{
	char const *chat = config_string("audio.voice_chat");
	boolean open_mic = chat && !csstrcmp(chat, "open_mic");
	boolean push_to_talk = chat && !csstrcmp(chat, "push_to_talk");
	boolean held;
	unsigned long now = system_milliseconds();
	float frame[VOICE_FRAME_SAMPLES];
	short local_machine = network_game_client_get_local_machine_index();
	long index;

	voice_audio_set_volume((float)PIN(config_real("audio.voice_volume"), 0.0, 2.0));
	/* (a network game: the host's, or one joined) */
	if (!voice_network_game() || local_machine == NONE)
	{
		voice_end_session();
		return;
	}
	voice_session = TRUE;
	if (voice_host())
		voice_host_update();
	else if (voice_settings_heard && now - voice_settings_heard_at >= VOICE_CONFIG_TIMEOUT_MS)
	{
		/* (the host repeats them: one that stopped has no voice chat) */
		voice_settings_heard = FALSE;
	}
	/* (mutes of a machine whose players changed are forgotten) */
	for (index = 0; index < VOICE_MACHINES; index++)
	{
		wchar_t name[12];

		if (!voice_muted[index])
			continue;
		voice_machine_name(index, name);
		if (csmemcmp(name, voice_muted_names[index], sizeof(name)))
			voice_muted[index] = FALSE;
	}
	if (!voice_settings_heard)
	{
		voice_audio_microphone(FALSE);
		voice_microphone_wanted = FALSE;
		return;
	}
	/* (a client says it hears voices) */
	if (!voice_host() && now - voice_hello_at >= VOICE_HELLO_MS)
	{
		voice_hello_at = now;
		voice_send(NULL, 0);
	}
	/* the microphone: open mic, open while voice is on; push to talk, from
	the first press (none asked for until then) */
	held = push_to_talk && halo_push_to_talk_held() && !console_is_active();
	if (!voice_may_talk() || (!open_mic && !push_to_talk))
		voice_microphone_wanted = FALSE;
	else if (open_mic || held)
		voice_microphone_wanted = TRUE;
	voice_audio_microphone(voice_microphone_wanted);
	while (voice_microphone_wanted && voice_audio_read_frame(frame))
	{
		boolean talk = held;
		byte packet[VOICE_MAXIMUM_PACKET];
		long length;

		if (open_mic)
		{
			if (voice_audio_level(frame) >= VOICE_OPEN_MIC_LEVEL)
				voice_open_mic_until = now + VOICE_OPEN_MIC_HOLD_MS;
			talk = (long)(voice_open_mic_until - now) > 0;
		}
		if (!talk || !voice_may_talk())
			continue;
		length = voice_audio_encode(frame, voice_settings.kbps * 1000, packet,
			(int)MIN(voice_packet_limit(voice_settings.kbps), VOICE_MAXIMUM_PACKET));
		if (length <= 0)
			continue;
		voice_send(packet, length);
		voice_talked = TRUE;
		voice_talked_at = now;
	}
}

/* ---------- the menus and the scoreboard (network_voice.h) */

boolean network_voice_machine_speaking(
	long machine_index)
{
	unsigned long now = system_milliseconds();

	if (!voice_session || !voice_machine_valid(machine_index))
		return FALSE;
	if (machine_index == network_game_client_get_local_machine_index())
		return voice_talked && now - voice_talked_at < VOICE_SPEAKING_MS;
	return voice_heard[machine_index] && now - voice_heard_at[machine_index] < VOICE_SPEAKING_MS &&
		!voice_muted[machine_index];
}

boolean network_voice_machine_muted(
	long machine_index)
{
	return voice_machine_valid(machine_index) && voice_muted[machine_index];
}

void network_voice_mute_machine(
	long machine_index,
	boolean mute)
{
	if (!voice_machine_valid(machine_index) || machine_index == network_game_client_get_local_machine_index())
		return;
	voice_muted[machine_index] = mute;
	voice_machine_name(machine_index, voice_muted_names[machine_index]);
	if (mute)
		voice_audio_forget((int)machine_index);
}

boolean network_voice_available(
	void)
{
	return voice_session && voice_settings_heard;
}

/* a speaker drawn in the rectangle (a square of its height, at its left;
muted: struck through, in red), fading with alpha: Lucide's icons
(port/assets/icons/lucide), drawn white as the menus' bitmap voice/speaker,
a frame each (tools/ce_menus.py), and tinted; none without the menus'
bitmaps */
void network_voice_draw_icon(
	rectangle2d const *bounds,
	boolean muted,
	real alpha)
{
	long bitmap_index = tag_loaded(BITMAP_GROUP_TAG, VOICE_ICON_BITMAP);
	struct bitmap_data *bitmap = bitmap_index != NONE ? bitmap_group_try_and_get_bitmap(bitmap_index, muted ? 1 : 0) :
		NULL;
	pixel32 a = (pixel32)(PIN(alpha, 0.0f, 1.0f) * 255.0f + 0.5f) << 24;
	rectangle2d square = *bounds;

	if (!bitmap || bounds->y1 - bounds->y0 < 4)
		return;
	square.x1 = (short)(square.x0 + (square.y1 - square.y0));
	draw_bitmap_in_rect(bitmap, &square, NULL, NULL, a | (muted ? 0x00E05A5A : 0x0050E050), NULL, TRUE);
}
