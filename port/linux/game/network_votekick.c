/*
NETWORK_VOTEKICK.C

The players' votes (port/linux/NETCODE.md): to kick a player, and to end
the game. A player starts a vote to kick by picking another's name on the
scoreboard (game_engine.c) or with the console's "votekick <name>", and one
to end the game by saying "!endgame" in the chat (network_chat.c). While a
vote runs every machine shows its box (hud.c), and F1 votes yes, F2 no
(picking the name on the scoreboard votes yes as well). The host alone
counts, and kicks the machine of the player, or ends the game (as
sv_end_game does: its scores and postgame), once enough have voted yes. A
vote's power is against the game's other players, so the host decides
every part of it and trusts nothing a client says but which vote it
chooses:

- A vote counts only from its machine's own stream: a datagram's source
  address is all that names its machine, and anyone can send one as from
  another's.
- One vote a person. The votes and the players who may vote are counted by
  their machines' real addresses (an internet play peer's endpoint, not its
  stand-in, which the peer can make anew), and the votes again by hardware
  id: two machines of one address or one hardware id are one vote. The
  hardware id a machine tells the host joining (anyone's it likes), so it
  only ever takes votes away: the players who may vote are counted by their
  addresses alone, or a machine could copy another's id and so lower the
  votes needed.
- Time played: on this host, counted in its own ticks of game, not told
  by the client: votekick_minutes (config.toml) to start a vote, and a
  part of that to vote, so that a player who has only just joined (or a
  cheater's second machine) neither starts nor counts. Joining again starts
  over.
- Yes votes scaled with the players who may vote, the player voted against
  counted among them (who never votes): more than half of them up to 8,
  then 40% (at least 5) up to 32, then 30% (at least 13), and at least two;
  and more yes than no. One team alone kicks nobody from the other in an
  even small game, and two people never kick each other. A vote the no
  votes have made lost ends at once. A vote to end the game counts as a
  kick's, with no one voted against, from 30 seconds played (and not in a
  game's first minute).
- One vote at a time, for VOTE_SECONDS, then a pause before the next. A
  player whose vote failed starts none for STARTER_COOLDOWN_SECONDS, and a
  player a vote failed against is not voted against again for
  TARGET_PROTECTION_SECONDS, both by address and hardware id.
- The host's own players are never voted against; the host kicks and bans
  as it likes (network_server_manager.c), and its machine votes as any
  other.
- Only the host speaks: what a client did wrong is told to it alone, at
  most once a second; the votes to everyone, only as they change.
- A machine kicked by a vote is kept out of the host's games, by address
  and hardware id, for votekick_ban_minutes. One that leaves during a vote
  against it is too, as if the vote passed, and while the vote runs it is
  refused if it tries to join again: no one dodges a vote by leaving.

A client hears the host's count every second (and as it changes), for the
scoreboard. A build without votes drops both messages as kinds it does not
know: its players cannot vote, and count as players who did not.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "memory/data.h"
#include "game/game.h"
#include "game/players.h"
#include "game/game_engine.h"
#include "networking/network_game_globals.h"
#include "network_distributed.h"
#include "network_votekick.h"
#include "server_director.h"
#include "halo_keyboard.h"
#include "cutscene/cinematics.h"
#include "interface/hud.h"
#include "interface/hud_messaging.h"
#include "interface/ui_widget.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "text/unicode.h"

#include <stdio.h>
#include <string.h>

/* port_config.c's, network_server_manager.c's, cseries_windows.c's */
int config_boolean(const char *name);
long config_integer(const char *name);
const char *config_string(const char *name);
void network_game_server_kick_machine(long machine_index, boolean kept_out);
boolean network_game_server_kick_machine_of_player(long machine_index, boolean ban);
char const *network_game_server_machine_hardware_id(long machine_index);
void p2p_hardware_id_sanitize(char *destination, int size, const char *source);
unsigned long system_milliseconds(void);
void console_warning(const char *format, ...);
int halo_server_mode(void);
unsigned char network_chat_keyboard_open(void);

/* ---------- constants */

enum
{
	/* p2p.h's P2P_HARDWARE_ID_SIZE */
	VOTEKICK_HARDWARE_ID_SIZE = 33,
	VOTE_SECONDS = 45,
	/* the time played to vote to end the game, at most */
	END_GAME_VOTER_SECONDS = 30,
	/* a game this young is not voted to an end */
	END_GAME_MINIMUM_GAME_SECONDS = 60,
	/* after a vote, before the next */
	VOTE_GAP_SECONDS = 30,
	STARTER_COOLDOWN_SECONDS = 5 * 60,
	TARGET_PROTECTION_SECONDS = 10 * 60,
	/* the time played to vote, at most (and no more than to start one) */
	VOTER_MINIMUM_SECONDS = 2 * 60,
	/* a machine's requests the host takes, at most one this often */
	REQUEST_INTERVAL_MILLISECONDS = 1000,
	MINIMUM_VOTES = 2,
	/* the addresses (and hardware ids) remembered: kept out, cooling down,
	protected; the oldest forgotten first */
	MAXIMUM_REMEMBERED = 32,
	/* a client forgets the host's count it has not heard again this long */
	STATUS_TIMEOUT_MILLISECONDS = 3000,
	MAXIMUM_VOTEKICK_MACHINES = HALO_PORT_MAXIMUM_NETWORK_MACHINES,
	/* the vote's box: its width, in lines of the HUD's font */
	VOTE_BOX_WIDTH_LINES = 13,
};

/* the box's text, of the HUD font's size (as the chat's: network_chat.c) */
#define VOTE_BOX_TEXT_SCALE 0.5f

/* the status's flags */
enum
{
	/* this machine (the one told) has voted yes */
	_votekick_status_voted_bit,
	/* ... may vote */
	_votekick_status_may_vote_bit,
	/* ... has voted no */
	_votekick_status_voted_no_bit,
};

/* a machine's choice in the vote */
enum
{
	_votekick_choice_none,
	_votekick_choice_yes,
	_votekick_choice_no,
};

/* ---------- structures */

struct distributed_votekick_request
{
	/* the vote's kind (_network_vote_kick, _network_vote_end_game) */
	byte kind;
	/* the player voted against (absolute index; a kick's) */
	byte player_index;
	/* _votekick_choice_yes (which starts a vote when none runs), _no */
	byte choice;
	byte pad;
};

struct distributed_votekick_status
{
	/* the vote's kind, _network_vote_none for no vote */
	byte kind;
	/* the player voted against (absolute index), NO_PLAYER for none */
	byte player_index;
	byte votes;
	byte no_votes;
	byte needed;
	byte seconds_left;
	byte flags;
	byte pad;
};

typedef char distributed_votekick_request_size_assert[sizeof(struct distributed_votekick_request) == 4 ? 1 : -1];
typedef char distributed_votekick_status_size_assert[sizeof(struct distributed_votekick_status) == 8 ? 1 : -1];

/* a machine as the count sees it (votekick_count) */
struct votekick_voter
{
	/* its real address (host byte order; 0: not known) */
	unsigned long address;
	char hardware_id[VOTEKICK_HARDWARE_ID_SIZE];
	/* the host's own machine */
	boolean host;
	/* the machine of the player voted against */
	boolean target;
	/* has played long enough to vote (the host's, always) */
	boolean eligible;
	/* voted yes; voted no */
	boolean voted;
	boolean voted_no;
};

/* an address and hardware id remembered until a time (system_milliseconds) */
struct votekick_identity
{
	unsigned long address;
	char hardware_id[VOTEKICK_HARDWARE_ID_SIZE];
	unsigned long until;
	boolean valid;
};

/* ---------- globals */

/* the host: each machine slot's time in game, in the host's ticks, since it
joined (network_votekick_machine_joined), and how many machines joined at
it (a vote names the one it was) */
static long votekick_played_ticks[MAXIMUM_VOTEKICK_MACHINES];
static long votekick_generations[MAXIMUM_VOTEKICK_MACHINES];
/* ... when it last asked something (system_milliseconds), and whether it
has */
static unsigned long votekick_request_times[MAXIMUM_VOTEKICK_MACHINES];
static boolean votekick_requested[MAXIMUM_VOTEKICK_MACHINES];

/* the host: the vote */
static struct
{
	boolean active;
	/* _network_vote_kick, _network_vote_end_game */
	short kind;
	/* the machine voted against, and its players' first (for the clients) */
	long target_machine;
	long target_generation;
	short target_player;
	unsigned long target_address;
	char target_hardware_id[VOTEKICK_HARDWARE_ID_SIZE];
	char target_names[64];
	/* who started it, cooling down if it fails */
	unsigned long starter_address;
	char starter_hardware_id[VOTEKICK_HARDWARE_ID_SIZE];
	unsigned long started_at;
	/* the machines that voted (as they were at their slots: their
	_votekick_choice), the host's */
	byte voted[MAXIMUM_VOTEKICK_MACHINES];
	long voted_generations[MAXIMUM_VOTEKICK_MACHINES];
	byte host_voted;
	/* as last counted, and told: yes, no, needed, and those who may vote */
	short votes;
	short no_votes;
	short needed;
	short electorate;
} votekick;

/* the host: no vote before this (a vote's end and the gap after) */
static unsigned long votekick_next_allowed;
static boolean votekick_gap;

static struct votekick_identity votekick_kept_out_identities[MAXIMUM_REMEMBERED];
static struct votekick_identity votekick_cooling_starters[MAXIMUM_REMEMBERED];
static struct votekick_identity votekick_protected_targets[MAXIMUM_REMEMBERED];

/* a client: the host's count, as last heard (the time: 0, none) */
static struct distributed_votekick_status votekick_status;
static unsigned long votekick_status_heard;

/* ---------- the count (pure: tools/harness/tests/votekick_count.c) */

/* whether two machines are one person's by address: the host's own is
only itself; an address not known, no other's */
static boolean votekick_same_address(
	struct votekick_voter const *a,
	struct votekick_voter const *b)
{
	if (a->host || b->host)
		return a->host && b->host;
	return a->address != 0 && a->address == b->address;
}

/* ... or by hardware id too (none told: no other's) */
static boolean votekick_same_person(
	struct votekick_voter const *a,
	struct votekick_voter const *b)
{
	if (votekick_same_address(a, b))
		return TRUE;
	return !a->host && !b->host && a->hardware_id[0] && !csstrcmp(a->hardware_id, b->hardware_id);
}

/* whether a machine is one of those who may vote: played long enough, and
its address known (else it could not be told from another) */
static boolean votekick_may_vote(
	struct votekick_voter const *voter)
{
	return voter->eligible && !voter->target && (voter->host || voter->address != 0);
}

/* the players who may vote (by address: the target's among them, once) and
the votes for (by address and hardware id, none of the target's person) */
static void votekick_count(
	struct votekick_voter const *voters,
	short count,
	short *votes,
	short *electorate)
{
	short index;
	short other;

	*votes = 0;
	*electorate = 0;
	for (index = 0; index < count; index++)
	{
		boolean counted = votekick_may_vote(&voters[index]) || voters[index].target;

		/* (an address counted once: the first of its machines) */
		for (other = 0; counted && other < index; other++)
		{
			if ((votekick_may_vote(&voters[other]) || voters[other].target) &&
				votekick_same_address(&voters[index], &voters[other]))
			{
				counted = FALSE;
			}
		}
		if (counted)
			(*electorate)++;
	}
	for (index = 0; index < count; index++)
	{
		boolean counted = voters[index].voted && votekick_may_vote(&voters[index]);

		for (other = 0; counted && other < count; other++)
		{
			/* (not the target's person, whatever it says; a person once) */
			if (voters[other].target && votekick_same_person(&voters[index], &voters[other]))
				counted = FALSE;
			if (other < index && voters[other].voted && votekick_may_vote(&voters[other]) &&
				votekick_same_person(&voters[index], &voters[other]))
			{
				counted = FALSE;
			}
		}
		if (counted)
			(*votes)++;
	}
}

/* the yes votes that pass a vote. percent 0, network.votekick_threshold
"dynamic": scaled with the lobby, more than half of those who may vote up
to 8 of them, then 40% (at least 5) up to 32, then 30% (at least 13).
Else "static": that percentage of those who may vote, rounded up. At least
MINIMUM_VOTES either way. (A vote passes with more yes than no too:
votekick_passes) */
static short votekick_votes_needed(
	short electorate,
	short percent)
{
	short needed;

	if (percent > 0)
		needed = (short)(((long)electorate * percent + 99) / 100);
	else if (electorate <= 8)
		needed = (short)(electorate / 2 + 1);
	else if (electorate <= 32)
		needed = (short)MAX(5, (electorate * 4 + 9) / 10);
	else
		needed = (short)MAX(13, (electorate * 3 + 9) / 10);
	return needed < MINIMUM_VOTES ? MINIMUM_VOTES : needed;
}

/* the host's threshold: 0 for "dynamic" (network.votekick_threshold),
else network.votekick_percent (1 to 100) for "static" */
static short votekick_threshold_percent(
	void)
{
	if (strcmp(config_string("network.votekick_threshold"), "static"))
		return 0;
	return (short)PIN(config_integer("network.votekick_percent"), 1, 100);
}

/* whether the votes pass it: enough yes, and more yes than no */
static boolean votekick_passes(
	short votes,
	short no_votes,
	short needed)
{
	return votes >= needed && votes > no_votes;
}

/* whether a time (system_milliseconds) has come; times wrap, so by their
difference */
static boolean votekick_time_reached(
	unsigned long now,
	unsigned long time)
{
	return (long)(now - time) >= 0;
}

/* ---------- remembered identities */

static void votekick_remember(
	struct votekick_identity *identities,
	unsigned long address,
	char const *hardware_id,
	unsigned long milliseconds)
{
	unsigned long now = system_milliseconds();
	short index;
	short oldest = 0;

	/* (a free entry, one gone by, else the one that ends soonest) */
	for (index = 0; index < MAXIMUM_REMEMBERED; index++)
	{
		if (!identities[index].valid || votekick_time_reached(now, identities[index].until))
		{
			oldest = index;
			break;
		}
		if ((long)(identities[index].until - identities[oldest].until) < 0)
			oldest = index;
	}
	identities[oldest].valid = TRUE;
	identities[oldest].address = address;
	p2p_hardware_id_sanitize(identities[oldest].hardware_id, VOTEKICK_HARDWARE_ID_SIZE, hardware_id);
	identities[oldest].until = now + milliseconds;
}

/* whether an address or hardware id is remembered (not gone by); the
milliseconds left */
static boolean votekick_remembered(
	struct votekick_identity const *identities,
	unsigned long address,
	char const *hardware_id,
	unsigned long *milliseconds_left)
{
	unsigned long now = system_milliseconds();
	short index;

	for (index = 0; index < MAXIMUM_REMEMBERED; index++)
	{
		struct votekick_identity const *identity = &identities[index];

		if (!identity->valid || votekick_time_reached(now, identity->until))
			continue;
		if ((address && identity->address == address) ||
			(hardware_id && hardware_id[0] && identity->hardware_id[0] &&
				!csstrcmp(hardware_id, identity->hardware_id)))
		{
			if (milliseconds_left)
				*milliseconds_left = identity->until - now;
			return TRUE;
		}
	}
	return FALSE;
}

/* ---------- the host */

static boolean votekick_host(
	void)
{
	return game_connection() == _game_connection_network_server;
}

/* the time played to start a vote, and to vote (in ticks) */
static long votekick_starter_ticks(
	void)
{
	long minutes = config_integer("network.votekick_minutes");

	return PIN(minutes, 0, 60) * 60 * TICKS_PER_SECOND;
}

/* (a vote to end the game: END_GAME_VOTER_SECONDS at most, to start it too) */
static long votekick_voter_ticks(
	void)
{
	long ticks = MIN(votekick_starter_ticks(), VOTER_MINIMUM_SECONDS * TICKS_PER_SECOND);

	if (votekick.kind == _network_vote_end_game)
		ticks = MIN(ticks, END_GAME_VOTER_SECONDS * TICKS_PER_SECOND);
	return ticks;
}

static boolean votekick_machine_valid(
	long machine_index)
{
	return machine_index >= 0 && machine_index < MAXIMUM_VOTEKICK_MACHINES;
}

/* (network_server_manager.c) a machine joined the host at the slot: its
time played starts over, and no vote of the machine before is its */
void network_votekick_machine_joined(
	long machine_index)
{
	if (!votekick_machine_valid(machine_index))
		return;
	votekick_played_ticks[machine_index] = 0;
	votekick_generations[machine_index]++;
	votekick_requested[machine_index] = FALSE;
	votekick.voted[machine_index] = FALSE;
}

/* (network_server_manager.c) whether a machine of the address (as it
joins from: host byte order) and hardware id is kept out, kicked by a vote */
boolean network_votekick_kept_out(
	unsigned long address,
	char const *hardware_id)
{
	unsigned long real_address = distributed_real_address(address);

	/* (nor the player a vote runs against, joining again during it: it
	would drop the machine voted against for a new one, out of the vote) */
	if (votekick.active && votekick_host() &&
		((real_address && real_address == votekick.target_address) ||
			(hardware_id && hardware_id[0] && votekick.target_hardware_id[0] &&
				!csstrcmp(hardware_id, votekick.target_hardware_id))))
	{
		return TRUE;
	}
	return votekick_remembered(votekick_kept_out_identities, real_address, hardware_id, NULL);
}

/* the client machines in the game with a player who has not quit, and the
host's own if it has one (NONE); their count */
static short votekick_machines(
	long *machine_indices,
	short maximum)
{
	struct data_iterator iterator;
	struct player_datum *player;
	short count = 0;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		long machine_index = distributed_player_machine((short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index));
		short index;

		if (player->quit_out_of_game || (machine_index != NONE && !votekick_machine_valid(machine_index)))
			continue;
		for (index = 0; index < count && machine_indices[index] != machine_index; index++)
			;
		if (index == count && count < maximum)
			machine_indices[count++] = machine_index;
	}
	return count;
}

/* a machine (NONE: the host's own) as the count sees it */
static void votekick_voter_get(
	long machine_index,
	struct votekick_voter *voter)
{
	csmemset(voter, 0, sizeof(*voter));
	if (machine_index == NONE)
	{
		voter->host = TRUE;
		voter->eligible = TRUE;
		voter->voted = votekick.active && votekick.host_voted == _votekick_choice_yes;
		voter->voted_no = votekick.active && votekick.host_voted == _votekick_choice_no;
		return;
	}
	voter->address = distributed_machine_real_address(machine_index);
	p2p_hardware_id_sanitize(voter->hardware_id, VOTEKICK_HARDWARE_ID_SIZE,
		network_game_server_machine_hardware_id(machine_index));
	voter->eligible = votekick_played_ticks[machine_index] >= votekick_voter_ticks();
	voter->target = votekick.active && votekick.kind == _network_vote_kick && machine_index == votekick.target_machine;
	if (votekick.active && votekick.voted_generations[machine_index] == votekick_generations[machine_index])
	{
		voter->voted = votekick.voted[machine_index] == _votekick_choice_yes;
		voter->voted_no = votekick.voted[machine_index] == _votekick_choice_no;
	}
}

/* the target's machine still the one voted against, in the game */
static boolean votekick_target_present(
	void)
{
	long machine_indices[MAXIMUM_VOTEKICK_MACHINES + 1];
	short count = votekick_machines(machine_indices, (short)NUMBEROF(machine_indices));
	short index;

	if (votekick.kind != _network_vote_kick)
		return TRUE;
	if (votekick_generations[votekick.target_machine] != votekick.target_generation)
		return FALSE;
	for (index = 0; index < count; index++)
	{
		if (machine_indices[index] == votekick.target_machine)
			return TRUE;
	}
	return FALSE;
}

/* the count now, of the machines in the game (the target's among them:
votekick_target_present) */
static void votekick_recount(
	void)
{
	struct votekick_voter voters[MAXIMUM_VOTEKICK_MACHINES + 1];
	long machine_indices[MAXIMUM_VOTEKICK_MACHINES + 1];
	short count = votekick_machines(machine_indices, (short)NUMBEROF(machine_indices));
	short electorate;
	short index;

	for (index = 0; index < count; index++)
		votekick_voter_get(machine_indices[index], &voters[index]);
	votekick_count(voters, count, &votekick.votes, &electorate);
	votekick.needed = votekick_votes_needed(electorate, votekick_threshold_percent());
	/* (a static percentage asks no more than those who can vote yes: the
	player voted against never does) */
	if (votekick_threshold_percent() > 0)
	{
		short voters = (short)(electorate - (votekick.kind == _network_vote_kick ? 1 : 0));

		votekick.needed = (short)MAX(MINIMUM_VOTES, MIN(votekick.needed, voters));
	}
	votekick.electorate = electorate;
	/* (the no votes counted as the yes are) */
	for (index = 0; index < count; index++)
		voters[index].voted = voters[index].voted_no;
	votekick_count(voters, count, &votekick.no_votes, &electorate);
}

/* whether the no votes have made it lost: too few left who may vote yes
(the target never does) */
static boolean votekick_lost(
	void)
{
	short possible = (short)(votekick.electorate - votekick.no_votes - (votekick.kind == _network_vote_kick ? 1 : 0));

	return votekick.no_votes > 0 && (possible < votekick.needed || possible <= votekick.no_votes);
}

/* the vote's subject for a notice: "kick Bob", "end the game" */
static void votekick_subject(
	char *text,
	long size)
{
	if (votekick.kind == _network_vote_end_game)
		snprintf(text, (size_t)size, "end the game");
	else
		snprintf(text, (size_t)size, "kick %s", votekick.target_names);
}

/* the name of a machine's players for a notice (NONE: the host's own) */
static void votekick_names(
	long machine_index,
	char *names,
	long size)
{
	distributed_machine_player_names(machine_index, names, size);
}

/* the count to every client: what it may do, and has */
static void votekick_send_status(
	void)
{
	long machine_indices[MAXIMUM_VOTEKICK_MACHINES];
	short count = distributed_client_machines(machine_indices, MAXIMUM_VOTEKICK_MACHINES);
	unsigned long now = system_milliseconds();
	short index;

	for (index = 0; index < count; index++)
	{
		struct
		{
			struct distributed_message_header header;
			struct distributed_votekick_status status;
		} message;
		struct votekick_voter voter;

		csmemset(&message, 0, sizeof(message));
		message.status.kind = _network_vote_none;
		message.status.player_index = NO_PLAYER;
		if (votekick.active)
		{
			unsigned long elapsed = now - votekick.started_at;
			long left = VOTE_SECONDS - (long)(elapsed / 1000);

			votekick_voter_get(machine_indices[index], &voter);
			message.status.kind = (byte)votekick.kind;
			if (votekick.kind == _network_vote_kick)
				message.status.player_index = distributed_player_to_byte(votekick.target_player);
			message.status.votes = (byte)PIN(votekick.votes, 0, 255);
			message.status.no_votes = (byte)PIN(votekick.no_votes, 0, 255);
			message.status.needed = (byte)PIN(votekick.needed, 0, 255);
			message.status.seconds_left = (byte)PIN(left, 0, 255);
			if (voter.voted)
				message.status.flags |= FLAG(_votekick_status_voted_bit);
			if (voter.voted_no)
				message.status.flags |= FLAG(_votekick_status_voted_no_bit);
			if (votekick_may_vote(&voter))
				message.status.flags |= FLAG(_votekick_status_may_vote_bit);
		}
		distributed_send_to_machine(machine_indices[index], &message, _distributed_message_votekick_status, 1,
			(word)sizeof(message));
	}
}

/* the vote over (passed or not: its notice said), and none for a while */
static void votekick_end(
	void)
{
	votekick.active = FALSE;
	votekick_next_allowed = system_milliseconds() + VOTE_GAP_SECONDS * 1000;
	votekick_gap = TRUE;
	votekick_send_status();
}

/* (a vote to end the game passed) the game ended as sv_end_game ends it:
its scores, its postgame; a dedicated server's director then plays the
next of its rotation */
static void votekick_end_the_game(
	void)
{
	if (halo_server_mode())
		server_director_end_game(_server_end_vote, FALSE);
	else if (game_engine_running())
		game_engine_end_game();
}

/* why the game cannot be voted to an end now; NULL if it can */
static char const *votekick_end_game_refused(
	void)
{
	if (!config_boolean("network.vote_end_game"))
		return "The host has turned votes to end the game off";
	if (halo_server_mode() ? !server_director_playing() : !game_engine_running())
		return "There is no game running to end";
	if (game_time_get() < END_GAME_MINIMUM_GAME_SECONDS * TICKS_PER_SECOND)
		return "The game has only just started";
	return NULL;
}

/* the vote passed: the player kicked and kept out, or the game ended; or
(left) the player left during a vote to kick them, which passes it too (no
one dodges a vote by leaving), and is kept out as well */
static void votekick_pass(
	boolean left)
{
	char notice[160];
	long ban_minutes = PIN(config_integer("network.votekick_ban_minutes"), 1, 24 * 60);

	if (votekick.kind == _network_vote_end_game)
	{
		snprintf(notice, sizeof(notice), "The vote to end the game passed (%d yes, %d no): the game ends",
			votekick.votes, votekick.no_votes);
		distributed_send_notice(notice);
		error(_error_log, "vote: the game ended by vote (%d yes, %d no)", votekick.votes, votekick.no_votes);
		votekick_end();
		votekick_end_the_game();
		return;
	}
	if (left)
	{
		snprintf(notice, sizeof(notice), "%s left during the vote to kick them, kept out for %ld minute%s",
			votekick.target_names, ban_minutes, ban_minutes == 1 ? "" : "s");
	}
	else
	{
		snprintf(notice, sizeof(notice), "%s kicked by vote (%d yes, %d no), kept out for %ld minute%s",
			votekick.target_names, votekick.votes, votekick.no_votes, ban_minutes, ban_minutes == 1 ? "" : "s");
	}
	distributed_send_notice(notice);
	votekick_remember(votekick_kept_out_identities, votekick.target_address, votekick.target_hardware_id,
		(unsigned long)ban_minutes * 60 * 1000);
	/* (one that left is gone: its slot may be another's now) */
	if (!left)
		network_game_server_kick_machine(votekick.target_machine, FALSE);
	votekick_end();
}

static void votekick_fail(
	void)
{
	char notice[160];
	char subject[96];

	votekick_subject(subject, sizeof(subject));
	snprintf(notice, sizeof(notice), "The vote to %s failed (%d yes, %d no, %d needed)", subject, votekick.votes,
		votekick.no_votes, votekick.needed);
	distributed_send_notice(notice);
	votekick_remember(votekick_cooling_starters, votekick.starter_address, votekick.starter_hardware_id,
		STARTER_COOLDOWN_SECONDS * 1000UL);
	if (votekick.kind == _network_vote_kick)
	{
		votekick_remember(votekick_protected_targets, votekick.target_address, votekick.target_hardware_id,
			TARGET_PROTECTION_SECONDS * 1000UL);
	}
	votekick_end();
}

/* a refusal, to the asking machine alone */
static void votekick_refuse(
	long machine_index,
	char const *text)
{
	if (machine_index == NONE)
		console_warning("%s", text);
	else
		distributed_send_notice_to_machine(machine_index, text);
}

/* the count again after a vote, passed or lost; or told */
static void votekick_settle(
	void)
{
	votekick_recount();
	if (votekick_passes(votekick.votes, votekick.no_votes, votekick.needed))
		votekick_pass(FALSE);
	else if (votekick_lost())
		votekick_fail();
	else
		votekick_send_status();
}

/* a machine's choice (NONE: the host's own machine) in the vote running */
static void votekick_host_choose(
	long machine_index,
	byte choice)
{
	struct votekick_voter voter;
	char names[64];
	char subject[96];
	char notice[160];
	short votes = votekick.votes;
	short no_votes = votekick.no_votes;

	votekick_voter_get(machine_index, &voter);
	if (voter.target)
	{
		votekick_refuse(machine_index, "vote: not in the vote against you");
		return;
	}
	if (!votekick_may_vote(&voter))
	{
		votekick_refuse(machine_index, "vote: you have not played long enough on this server to vote");
		return;
	}
	/* (one vote, not changed) */
	if (voter.voted || voter.voted_no)
		return;
	if (machine_index == NONE)
		votekick.host_voted = choice;
	else
	{
		votekick.voted[machine_index] = choice;
		votekick.voted_generations[machine_index] = votekick_generations[machine_index];
	}
	votekick_recount();
	/* (told only as the count changes: a second machine of one person's adds
	nothing) */
	if ((votekick.votes != votes || votekick.no_votes != no_votes) &&
		!votekick_passes(votekick.votes, votekick.no_votes, votekick.needed))
	{
		votekick_names(machine_index, names, sizeof(names));
		votekick_subject(subject, sizeof(subject));
		snprintf(notice, sizeof(notice), "%s voted %s to %s (%d yes, %d no, %d needed)", names,
			choice == _votekick_choice_yes ? "yes" : "no", subject, votekick.votes, votekick.no_votes, votekick.needed);
		distributed_send_notice(notice);
	}
	votekick_settle();
}

/* a machine's vote (NONE: the host's own machine): a new vote of the kind
(yes; against the player for a kick), or its choice in the one running */
static void votekick_host_request(
	long machine_index,
	short kind,
	short player_index,
	byte choice)
{
	struct player_datum *player = NULL;
	long target_machine = NONE;
	struct votekick_voter voter;
	struct votekick_voter target;
	char names[64];
	char subject[96];
	char notice[192];
	unsigned long now = system_milliseconds();
	unsigned long left;
	char const *refused;

	if (machine_index == NONE && local_player_get_next(NONE) == NONE)
		return;
	/* a choice in the vote running */
	if (votekick.active)
	{
		if (kind != votekick.kind || (kind == _network_vote_kick && player_index != votekick.target_player))
		{
			votekick_subject(subject, sizeof(subject));
			snprintf(notice, sizeof(notice), "vote: wait for the vote to %s to end", subject);
			votekick_refuse(machine_index, notice);
			return;
		}
		votekick_host_choose(machine_index, choice);
		return;
	}
	if (choice != _votekick_choice_yes)
	{
		votekick_refuse(machine_index, "vote: no vote is running");
		return;
	}
	csmemset(&target, 0, sizeof(target));
	if (kind == _network_vote_kick)
	{
		if (!config_boolean("network.votekick"))
		{
			votekick_refuse(machine_index, "votekick: the host has turned votes off");
			return;
		}
		player = distributed_player(player_index);
		target_machine = player ? distributed_player_machine(player_index) : NONE;
		if (!player || player->quit_out_of_game)
		{
			votekick_refuse(machine_index, "votekick: no such player");
			return;
		}
		if (target_machine == NONE)
		{
			votekick_refuse(machine_index, "votekick: the host's players cannot be voted out");
			return;
		}
		if (!votekick_machine_valid(target_machine) || target_machine == machine_index)
		{
			votekick_refuse(machine_index, "votekick: not against your own players");
			return;
		}
		votekick_voter_get(target_machine, &target);
		target.target = TRUE;
	}
	else if ((refused = votekick_end_game_refused()) != NULL)
	{
		votekick_refuse(machine_index, refused);
		return;
	}
	/* (the time played to start one, as for the kind) */
	votekick.kind = kind;
	votekick_voter_get(machine_index, &voter);
	if (kind == _network_vote_kick && votekick_same_person(&voter, &target))
	{
		votekick_refuse(machine_index, "votekick: not against your own machine");
		return;
	}
	if (!voter.host && !voter.address)
	{
		votekick_refuse(machine_index, "vote: the host does not know your address");
		return;
	}
	if (votekick_gap && !votekick_time_reached(now, votekick_next_allowed))
	{
		snprintf(notice, sizeof(notice), "vote: the next vote can start in %lu seconds",
			(votekick_next_allowed - now + 999) / 1000);
		votekick_refuse(machine_index, notice);
		return;
	}
	{
		long needed_ticks = kind == _network_vote_kick ? votekick_starter_ticks() : votekick_voter_ticks();

		if (!voter.host && votekick_played_ticks[machine_index] < needed_ticks)
		{
			long seconds = needed_ticks / TICKS_PER_SECOND;

			if (seconds >= 60)
			{
				snprintf(notice, sizeof(notice), "vote: play %ld minute%s on this server to start a vote (%ld played)",
					seconds / 60, seconds / 60 == 1 ? "" : "s", votekick_played_ticks[machine_index] / (60 * TICKS_PER_SECOND));
			}
			else
			{
				snprintf(notice, sizeof(notice), "vote: play %ld seconds on this server to start a vote", seconds);
			}
			votekick_refuse(machine_index, notice);
			return;
		}
	}
	if (!voter.host && votekick_remembered(votekick_cooling_starters, voter.address, voter.hardware_id, &left))
	{
		snprintf(notice, sizeof(notice), "vote: your last vote failed: wait %lu seconds", (left + 999) / 1000);
		votekick_refuse(machine_index, notice);
		return;
	}
	if (kind == _network_vote_kick &&
		votekick_remembered(votekick_protected_targets, target.address, target.hardware_id, &left))
	{
		snprintf(notice, sizeof(notice), "votekick: a vote against that player failed lately: wait %lu seconds",
			(left + 999) / 1000);
		votekick_refuse(machine_index, notice);
		return;
	}
	csmemset(&votekick, 0, sizeof(votekick));
	votekick.active = TRUE;
	votekick.kind = kind;
	votekick.target_machine = target_machine;
	votekick.target_player = NONE;
	if (kind == _network_vote_kick)
	{
		votekick.target_generation = votekick_generations[target_machine];
		votekick.target_player = player_index;
		votekick.target_address = target.address;
		csstrcpy(votekick.target_hardware_id, target.hardware_id);
		votekick_names(target_machine, votekick.target_names, sizeof(votekick.target_names));
	}
	votekick.starter_address = voter.address;
	csstrcpy(votekick.starter_hardware_id, voter.hardware_id);
	votekick.started_at = now;
	if (machine_index == NONE)
		votekick.host_voted = _votekick_choice_yes;
	else
	{
		votekick.voted[machine_index] = _votekick_choice_yes;
		votekick.voted_generations[machine_index] = votekick_generations[machine_index];
	}
	votekick_recount();
	votekick_names(machine_index, names, sizeof(names));
	votekick_subject(subject, sizeof(subject));
	snprintf(notice, sizeof(notice), "%s started a vote to %s (%d needed): F1 or D-pad left votes yes, F2 or right no", names, subject,
		votekick.needed);
	distributed_send_notice(notice);
	error(_error_log, "vote: %s started by machine %ld (against machine %ld)", subject, machine_index, target_machine);
	votekick_settle();
}

void network_votekick_handle_request(
	long machine_index,
	void const *entries,
	boolean from_stream)
{
	struct distributed_votekick_request request;
	unsigned long now = system_milliseconds();

	if (!votekick_host() || !votekick_machine_valid(machine_index) || !from_stream)
		return;
	/* (at most one a second: more are dropped, untold) */
	if (votekick_requested[machine_index] &&
		!votekick_time_reached(now, votekick_request_times[machine_index] + REQUEST_INTERVAL_MILLISECONDS))
	{
		return;
	}
	votekick_requested[machine_index] = TRUE;
	votekick_request_times[machine_index] = now;
	csmemcpy(&request, entries, sizeof(request));
	if ((request.kind != _network_vote_kick && request.kind != _network_vote_end_game) ||
		(request.choice != _votekick_choice_yes && request.choice != _votekick_choice_no) ||
		(request.kind == _network_vote_kick && request.player_index >= MAXIMUM_TRACKED_PLAYERS))
	{
		return;
	}
	votekick_host_request(machine_index, request.kind,
		request.kind == _network_vote_kick ? (short)request.player_index : NONE, request.choice);
}

word network_votekick_request_entry_size(
	void)
{
	return sizeof(struct distributed_votekick_request);
}

void network_votekick_new_game(
	void)
{
	/* (a vote ends with its game, failing nobody; the time played stays) */
	if (votekick.active && votekick_host())
		distributed_send_notice("The vote ended with the game");
	votekick.active = FALSE;
	csmemset(&votekick_status, 0, sizeof(votekick_status));
	votekick_status_heard = 0;
}

void network_votekick_host_tick(
	void)
{
	long machine_indices[MAXIMUM_VOTEKICK_MACHINES];
	short count = distributed_client_machines(machine_indices, MAXIMUM_VOTEKICK_MACHINES);
	short index;

	for (index = 0; index < count; index++)
	{
		if (votekick_machine_valid(machine_indices[index]))
			votekick_played_ticks[machine_indices[index]]++;
	}
	if (!votekick.active)
		return;
	/* (the player left: the vote passes) */
	if (!votekick_target_present())
	{
		votekick_pass(TRUE);
		return;
	}
	/* (those gone counted no more, and those who have since played long
	enough to vote among those who may) */
	if (game_time_get() % TICKS_PER_SECOND == 0)
	{
		votekick_recount();
		if (votekick_passes(votekick.votes, votekick.no_votes, votekick.needed))
		{
			votekick_pass(FALSE);
			return;
		}
		/* (or, the game over, a vote to end it) */
		if (votekick.kind == _network_vote_end_game &&
			(halo_server_mode() ? !server_director_playing() : !game_engine_running()))
		{
			distributed_send_notice("The vote to end the game ended with it");
			votekick_end();
			return;
		}
		if (votekick_lost() || votekick_time_reached(system_milliseconds(), votekick.started_at + VOTE_SECONDS * 1000))
		{
			votekick_fail();
			return;
		}
		votekick_send_status();
	}
}

/* ---------- a client */

void network_votekick_handle_status(
	void const *entries)
{
	if (game_connection() != _game_connection_network_client)
		return;
	csmemcpy(&votekick_status, entries, sizeof(votekick_status));
	votekick_status_heard = system_milliseconds();
	if (votekick_status.player_index != NO_PLAYER && !distributed_player(votekick_status.player_index))
		votekick_status.player_index = NO_PLAYER;
	/* (a kick against no player known, or a kind not known: no vote) */
	if ((votekick_status.kind == _network_vote_kick && votekick_status.player_index == NO_PLAYER) ||
		votekick_status.kind >= NUMBER_OF_NETWORK_VOTES)
	{
		votekick_status.kind = _network_vote_none;
	}
	/* (0: none heard) */
	if (!votekick_status_heard)
		votekick_status_heard = 1;
}

word network_votekick_status_entry_size(
	void)
{
	return sizeof(struct distributed_votekick_status);
}

/* ---------- every machine (network_votekick.h) */

boolean network_votekick_get_status(
	struct network_votekick_status *status)
{
	csmemset(status, 0, sizeof(*status));
	status->kind = _network_vote_none;
	status->player_index = NONE;
	if (votekick_host())
	{
		unsigned long elapsed;

		if (!votekick.active)
			return FALSE;
		elapsed = system_milliseconds() - votekick.started_at;
		status->kind = votekick.kind;
		status->player_index = votekick.target_player;
		status->votes = votekick.votes;
		status->no_votes = votekick.no_votes;
		status->needed = votekick.needed;
		status->seconds_left = (short)MAX(0, VOTE_SECONDS - (long)(elapsed / 1000));
		status->voted = votekick.host_voted == _votekick_choice_yes;
		status->voted_no = votekick.host_voted == _votekick_choice_no;
		/* (the host's machine with a player to vote) */
		status->may_vote = local_player_get_next(NONE) != NONE;
		return TRUE;
	}
	if (game_connection() != _game_connection_network_client || !votekick_status_heard ||
		votekick_time_reached(system_milliseconds(), votekick_status_heard + STATUS_TIMEOUT_MILLISECONDS) ||
		votekick_status.kind == _network_vote_none)
	{
		return FALSE;
	}
	status->kind = votekick_status.kind;
	status->player_index = distributed_player_from_byte(votekick_status.player_index) == NONE ? NONE :
		votekick_status.player_index;
	status->votes = votekick_status.votes;
	status->no_votes = votekick_status.no_votes;
	status->needed = votekick_status.needed;
	status->seconds_left = votekick_status.seconds_left;
	status->voted = TEST_FLAG(votekick_status.flags, _votekick_status_voted_bit);
	status->voted_no = TEST_FLAG(votekick_status.flags, _votekick_status_voted_no_bit);
	status->may_vote = TEST_FLAG(votekick_status.flags, _votekick_status_may_vote_bit);
	return status->kind != _network_vote_kick || status->player_index != NONE;
}

/* a player's name in ASCII (player_name_character_ascii), at most size */
static void votekick_player_name(
	struct player_datum const *player,
	char *text,
	long size)
{
	long index;

	for (index = 0; index < (long)NUMBEROF(player->name) && player->name[index] && index < size - 1; index++)
		text[index] = player_name_character_ascii(player->name[index]);
	text[index] = 0;
}

/* whether a name begins with the text, in either case; and is it */
static boolean votekick_name_begins_with(
	char const *name,
	char const *text,
	boolean whole)
{
	for (; *text; name++, text++)
	{
		char a = *name >= 'A' && *name <= 'Z' ? *name - 'A' + 'a' : *name;
		char b = *text >= 'A' && *text <= 'Z' ? *text - 'A' + 'a' : *text;

		if (!*name || a != b)
			return FALSE;
	}
	return !whole || !*name;
}

short network_votekick_matching_player_names(
	char const *text,
	char (*names)[VOTEKICK_NAME_TEXT_SIZE],
	short maximum_count)
{
	struct data_iterator iterator;
	struct player_datum *player;
	short count = 0;

	if (!network_votekick_available())
		return 0;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL && count < maximum_count)
	{
		if (player->quit_out_of_game || player->local_player_index != NONE)
			continue;
		votekick_player_name(player, names[count], VOTEKICK_NAME_TEXT_SIZE);
		if (votekick_name_begins_with(names[count], text, FALSE))
			count++;
	}
	return count;
}

boolean network_votekick_player_named(
	char const *text)
{
	struct data_iterator iterator;
	struct player_datum *player;
	long found = NONE;
	short matches = 0;
	short exact = 0;
	long exact_found = NONE;

	if (!network_votekick_available())
	{
		console_warning("votekick: only in a network game");
		return FALSE;
	}
	if (!text[0])
	{
		console_warning("votekick: give a player's name (Tab completes it)");
		return FALSE;
	}
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		char name[VOTEKICK_NAME_TEXT_SIZE];

		if (player->quit_out_of_game)
			continue;
		votekick_player_name(player, name, sizeof(name));
		if (votekick_name_begins_with(name, text, TRUE))
		{
			exact++;
			exact_found = iterator.datum_index;
		}
		if (votekick_name_begins_with(name, text, FALSE))
		{
			matches++;
			found = iterator.datum_index;
		}
	}
	if (exact == 1)
	{
		found = exact_found;
		matches = 1;
	}
	if (matches > 1)
	{
		console_warning("votekick: %d players' names begin with \"%s\": give more of it", matches, text);
		return FALSE;
	}
	if (matches == 0)
	{
		console_warning("votekick: no player's name begins with \"%s\"", text);
		return FALSE;
	}
	return network_votekick_request((short)DATUM_INDEX_TO_ABSOLUTE_INDEX(found));
}

/* (the host) its Kick and Ban on the scoreboard: the player's machine */
boolean network_votekick_host_kick(
	short player_index,
	boolean ban)
{
	long machine_index;

	if (!votekick_host() || !distributed_player(player_index))
		return FALSE;
	machine_index = distributed_player_machine(player_index);
	if (machine_index == NONE)
	{
		console_warning("%s: not a player of the host's own machine", ban ? "ban" : "kick");
		return FALSE;
	}
	return network_game_server_kick_machine_of_player(machine_index, ban);
}

boolean network_votekick_host(
	void)
{
	return votekick_host();
}

boolean network_votekick_available(
	void)
{
	return game_connection() == _game_connection_network_client ||
		game_connection() == _game_connection_network_server;
}

/* this machine's vote of the kind (a kick's against the player) and choice:
the host counts it at once, a client sends it */
static boolean votekick_send_request(
	short kind,
	short player_index,
	byte choice)
{
	struct
	{
		struct distributed_message_header header;
		struct distributed_votekick_request request;
	} message;

	if (votekick_host())
	{
		votekick_host_request(NONE, kind, player_index, choice);
		return TRUE;
	}
	if (game_connection() != _game_connection_network_client)
	{
		console_warning("vote: only in a network game");
		return FALSE;
	}
	csmemset(&message, 0, sizeof(message));
	message.request.kind = (byte)kind;
	message.request.player_index = kind == _network_vote_kick ? distributed_player_to_byte(player_index) : NO_PLAYER;
	message.request.choice = choice;
	distributed_send(&message, _distributed_message_votekick, 1, (word)sizeof(message), _distributed_to_host_reliably);
	return TRUE;
}

boolean network_votekick_request(
	short player_index)
{
	if (!distributed_player(player_index))
		return FALSE;
	if (distributed_player_is_local(player_index))
	{
		console_warning("votekick: not against your own players");
		return FALSE;
	}
	return votekick_send_request(_network_vote_kick, player_index, _votekick_choice_yes);
}

boolean network_vote_end_game(
	void)
{
	return votekick_send_request(_network_vote_end_game, NONE, _votekick_choice_yes);
}

boolean network_vote_choose(
	boolean yes)
{
	struct network_votekick_status status;

	if (!network_votekick_get_status(&status) || !status.may_vote || status.voted || status.voted_no)
		return FALSE;
	return votekick_send_request(status.kind, status.player_index, yes ? _votekick_choice_yes : _votekick_choice_no);
}

/* ---------- the vote's box (hud.c) */

/* a row of the box: its text in the colour, at the row's place (laid out
at the font's size, drawn scaled about the box's corner) */
static void votekick_draw_row(
	long font_index,
	short left,
	short top,
	short width,
	short line_height,
	short row,
	wchar_t const *text,
	real red,
	real green,
	real blue)
{
	real_argb_color color;
	rectangle2d bounds;

	bounds.x0 = (short)(left + line_height / 3);
	bounds.x1 = (short)(left + width);
	bounds.y0 = (short)(top + row * line_height + line_height / 4);
	bounds.y1 = (short)(bounds.y0 + line_height);
	color.alpha = 1.0f;
	color.red = red;
	color.green = green;
	color.blue = blue;
	draw_string_set_draw_mode(font_index, NONE, 0 /* left */, 0, &color);
	rasterizer_draw_unicode_string(&bounds, NULL, NULL, 0, text);
}

void network_votekick_draw(
	short local_player_index)
{
	struct network_votekick_status status;
	long font_index = hud_get_font_index();
	rectangle2d window = render.camera.window_bounds;
	struct font_header *font;
	boolean touch = halo_chat_touch_layout() != 0;
	real scale = touch ? 1.0f : VOTE_BOX_TEXT_SCALE;
	short line_height;
	short window_width;
	short window_height;
	short width;
	short left;
	short top;
	rectangle2d box;
	wchar_t text[96];
	wchar_t name[VOTEKICK_NAME_TEXT_SIZE + 1];
	short rows = 4;

	if (local_player_index != local_player_get_next(NONE) || font_index == NONE || !network_votekick_get_status(&status) ||
		ui_widgets_active() || cinematic_in_progress() || (hud_scripted_globals && !hud_scripted_globals->show_hud))
	{
		return;
	}
	offset_rectangle2d(&window, -render.camera.viewport_bounds.x0, -render.camera.viewport_bounds.y0);
	window_width = (short)(window.x1 - window.x0);
	window_height = (short)(window.y1 - window.y0);
	font = font_definition_get(font_index);
	line_height = font->ascending_height + font->descending_height + font->leading_height;
	if (line_height <= 0 || window_width <= 0 || window_height <= 0)
		return;
	width = (short)(VOTE_BOX_WIDTH_LINES * line_height);
	/* (at the left, a third of the way down: with the touch controls, whose
	stick is at the bottom left and the chat at the top left, at the right) */
	if (touch)
		left = (short)(window.x1 - window_width / 50 - width * scale);
	else
		left = (short)(window.x0 + window_width / 50);
	top = (short)(window.y0 + window_height * 3 / 10);
	box.x0 = left;
	box.y0 = top;
	box.x1 = (short)(left + width * scale);
	box.y1 = (short)(top + (rows * line_height + line_height / 2) * scale);
	draw_quad(&box, 0xB0000000);
	rasterizer_text_set_scale(scale, (real)left, (real)top);
	name[0] = 0;
	if (status.kind == _network_vote_kick)
	{
		struct player_datum *player = distributed_player(status.player_index);
		short index;

		for (index = 0; player && index < (short)NUMBEROF(player->name) && index < VOTEKICK_NAME_TEXT_SIZE &&
			player->name[index]; index++)
		{
			name[index] = player->name[index];
		}
		name[index] = 0;
		usprintf(text, L"Vote: kick %s", name[0] ? name : L"?");
	}
	else
	{
		usprintf(text, L"Vote: end the game");
	}
	votekick_draw_row(font_index, left, top, width, line_height, 0, text, 1.0f, 0.8f, 0.35f);
	/* (the keys of what the player last used: the controller's D-pad, the
	keyboard's F1 and F2, or the chat's words with the touch controls) */
	{
		boolean pad = halo_gamepad_last_used() != 0;

		usprintf(text, pad ? L"D-pad Left   Yes: %d" : touch ? L"!yes   Yes: %d" : L"F1   Yes: %d", status.votes);
		votekick_draw_row(font_index, left, top, width, line_height, 1, text, status.voted ? 0.5f : 0.45f,
			status.voted ? 1.0f : 0.85f, 0.45f);
		usprintf(text, pad ? L"D-pad Right  No: %d" : touch ? L"!no    No: %d" : L"F2   No: %d", status.no_votes);
	}
	votekick_draw_row(font_index, left, top, width, line_height, 2, text, status.voted_no ? 1.0f : 0.9f,
		status.voted_no ? 0.45f : 0.5f, 0.4f);
	if (status.voted || status.voted_no)
		usprintf(text, L"%d needed, %d s (you voted %s)", status.needed, status.seconds_left, status.voted ? L"yes" : L"no");
	else if (!status.may_vote)
		usprintf(text, L"%d needed, %d s (too new to vote)", status.needed, status.seconds_left);
	else
		usprintf(text, L"%d needed, %d s left", status.needed, status.seconds_left);
	votekick_draw_row(font_index, left, top, width, line_height, 3, text, 0.75f, 0.75f, 0.75f);
	rasterizer_text_set_scale(1.0f, 0.0f, 0.0f);
}

/* (xinput_sdl.c) whether the D-pad's left and right are the vote's now,
not the game's: a vote runs that this machine may vote in and has not, the
chat's on-screen keyboard is not up, and nothing hides the HUD (a menu, a
cinematic) */
unsigned char network_vote_pad_open(
	void)
{
	struct network_votekick_status status;

	/* (asked of every read of the controllers, the first before the game's
	state exists: nothing but a network game in progress) */
	if (!network_votekick_available() || !game_time_initialized() || !game_in_progress())
		return FALSE;
	if (ui_widgets_active() || cinematic_in_progress() || network_chat_keyboard_open() ||
		!network_votekick_get_status(&status))
	{
		return FALSE;
	}
	return status.may_vote && !status.voted && !status.voted_no;
}
