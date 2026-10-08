/*
NETWORK_VOTEKICK.C

The players' vote to kick a player (port/linux/NETCODE.md). A player starts
one by picking another's name on the scoreboard (game_engine.c) or with the
console's "votekick <name>"; the others vote the same way. The host alone
counts, and kicks the machine of the player once enough have voted. A
vote's power is against the game's other players, so the host decides
every part of it and trusts nothing a client says but which player it
votes against:

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
- More than half of the players who may vote, the player voted against
  counted among them (who never votes for it), and at least two: one team
  alone kicks nobody from the other in an even game, and two people never
  kick each other.
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
  and hardware id, for votekick_ban_minutes.

A client hears the host's count every second (and as it changes), for the
scoreboard. A build without votes drops both messages as kinds it does not
know: its players cannot vote, and count as players who did not.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "memory/data.h"
#include "game/game.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "network_distributed.h"
#include "network_votekick.h"

#include <stdio.h>

/* port_config.c's, network_server_manager.c's, cseries_windows.c's */
int config_boolean(const char *name);
long config_integer(const char *name);
void network_game_server_kick_machine(long machine_index, boolean kept_out);
boolean network_game_server_kick_machine_of_player(long machine_index, boolean ban);
char const *network_game_server_machine_hardware_id(long machine_index);
void p2p_hardware_id_sanitize(char *destination, int size, const char *source);
unsigned long system_milliseconds(void);
void console_warning(const char *format, ...);

/* ---------- constants */

enum
{
	/* p2p.h's P2P_HARDWARE_ID_SIZE */
	VOTEKICK_HARDWARE_ID_SIZE = 33,
	VOTE_SECONDS = 45,
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
};

/* the status's flags */
enum
{
	/* this machine (the one told) has voted */
	_votekick_status_voted_bit,
	/* ... may vote */
	_votekick_status_may_vote_bit,
};

/* ---------- structures */

struct distributed_votekick_request
{
	/* the player voted against (absolute index) */
	byte player_index;
	byte pad[3];
};

struct distributed_votekick_status
{
	/* the player voted against (absolute index), NO_PLAYER for no vote */
	byte player_index;
	byte votes;
	byte needed;
	byte seconds_left;
	byte flags;
	byte pad[3];
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
	boolean voted;
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
	/* the machines that voted (as they were at their slots), the host's */
	boolean voted[MAXIMUM_VOTEKICK_MACHINES];
	long voted_generations[MAXIMUM_VOTEKICK_MACHINES];
	boolean host_voted;
	/* as last counted, and told */
	short votes;
	short needed;
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

/* the votes that kick: more than half of those who may vote, and at least
MINIMUM_VOTES */
static short votekick_votes_needed(
	short electorate)
{
	short needed = (short)(electorate / 2 + 1);

	return needed < MINIMUM_VOTES ? MINIMUM_VOTES : needed;
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

static long votekick_voter_ticks(
	void)
{
	return MIN(votekick_starter_ticks(), VOTER_MINIMUM_SECONDS * TICKS_PER_SECOND);
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
	return votekick_remembered(votekick_kept_out_identities, distributed_real_address(address), hardware_id, NULL);
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
		voter->voted = votekick.active && votekick.host_voted;
		return;
	}
	voter->address = distributed_machine_real_address(machine_index);
	p2p_hardware_id_sanitize(voter->hardware_id, VOTEKICK_HARDWARE_ID_SIZE,
		network_game_server_machine_hardware_id(machine_index));
	voter->eligible = votekick_played_ticks[machine_index] >= votekick_voter_ticks();
	voter->target = votekick.active && machine_index == votekick.target_machine;
	voter->voted = votekick.active && votekick.voted[machine_index] &&
		votekick.voted_generations[machine_index] == votekick_generations[machine_index];
}

/* the target's machine still the one voted against, in the game */
static boolean votekick_target_present(
	void)
{
	long machine_indices[MAXIMUM_VOTEKICK_MACHINES + 1];
	short count = votekick_machines(machine_indices, (short)NUMBEROF(machine_indices));
	short index;

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
	votekick.needed = votekick_votes_needed(electorate);
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
		message.status.player_index = NO_PLAYER;
		if (votekick.active)
		{
			unsigned long elapsed = now - votekick.started_at;
			long left = VOTE_SECONDS - (long)(elapsed / 1000);

			votekick_voter_get(machine_indices[index], &voter);
			message.status.player_index = distributed_player_to_byte(votekick.target_player);
			message.status.votes = (byte)PIN(votekick.votes, 0, 255);
			message.status.needed = (byte)PIN(votekick.needed, 0, 255);
			message.status.seconds_left = (byte)PIN(left, 0, 255);
			if (voter.voted)
				message.status.flags |= FLAG(_votekick_status_voted_bit);
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

static void votekick_pass(
	void)
{
	char notice[160];
	long ban_minutes = PIN(config_integer("network.votekick_ban_minutes"), 1, 24 * 60);

	snprintf(notice, sizeof(notice), "%s kicked by vote (%d of %d), kept out for %ld minute%s",
		votekick.target_names, votekick.votes, votekick.needed, ban_minutes, ban_minutes == 1 ? "" : "s");
	distributed_send_notice(notice);
	votekick_remember(votekick_kept_out_identities, votekick.target_address, votekick.target_hardware_id,
		(unsigned long)ban_minutes * 60 * 1000);
	network_game_server_kick_machine(votekick.target_machine, FALSE);
	votekick_end();
}

static void votekick_fail(
	void)
{
	char notice[160];

	snprintf(notice, sizeof(notice), "The vote to kick %s failed (%d of %d)", votekick.target_names, votekick.votes,
		votekick.needed);
	distributed_send_notice(notice);
	votekick_remember(votekick_cooling_starters, votekick.starter_address, votekick.starter_hardware_id,
		STARTER_COOLDOWN_SECONDS * 1000UL);
	votekick_remember(votekick_protected_targets, votekick.target_address, votekick.target_hardware_id,
		TARGET_PROTECTION_SECONDS * 1000UL);
	votekick_end();
}

/* a refusal, to the asking machine alone */
static void votekick_refuse(
	long machine_index,
	char const *text)
{
	distributed_send_notice_to_machine(machine_index, text);
}

/* a machine's vote against a player (NONE: the host's own machine): a new
vote, or one for the vote running */
static void votekick_host_request(
	long machine_index,
	short player_index)
{
	struct player_datum *player = distributed_player(player_index);
	long target_machine = player ? distributed_player_machine(player_index) : NONE;
	struct votekick_voter voter;
	struct votekick_voter target;
	char names[64];
	char notice[160];
	unsigned long now = system_milliseconds();
	unsigned long left;

	if (!config_boolean("network.votekick"))
	{
		votekick_refuse(machine_index, "votekick: the host has turned votes off");
		return;
	}
	if (machine_index == NONE && local_player_get_next(NONE) == NONE)
		return;
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
	votekick_voter_get(machine_index, &voter);
	votekick_voter_get(target_machine, &target);
	target.target = TRUE;
	if (votekick_same_person(&voter, &target))
	{
		votekick_refuse(machine_index, "votekick: not against your own machine");
		return;
	}
	if (!voter.host && !voter.address)
	{
		votekick_refuse(machine_index, "votekick: the host does not know your address");
		return;
	}
	/* a vote for the vote running */
	if (votekick.active)
	{
		if (target_machine != votekick.target_machine)
		{
			snprintf(notice, sizeof(notice), "votekick: wait for the vote to kick %s to end", votekick.target_names);
			votekick_refuse(machine_index, notice);
			return;
		}
		if (!votekick_may_vote(&voter))
		{
			votekick_refuse(machine_index, "votekick: you have not played long enough on this server to vote");
			return;
		}
		if (voter.voted)
			return;
		if (machine_index == NONE)
			votekick.host_voted = TRUE;
		else
		{
			votekick.voted[machine_index] = TRUE;
			votekick.voted_generations[machine_index] = votekick_generations[machine_index];
		}
		{
			short votes = votekick.votes;

			votekick_recount();
			/* (told only as the count changes: a second machine of one
			person's adds nothing) */
			if (votekick.votes != votes && votekick.votes < votekick.needed)
			{
				votekick_names(machine_index, names, sizeof(names));
				snprintf(notice, sizeof(notice), "%s voted to kick %s (%d of %d)", names, votekick.target_names,
					votekick.votes, votekick.needed);
				distributed_send_notice(notice);
			}
		}
		if (votekick.votes >= votekick.needed)
			votekick_pass();
		else
			votekick_send_status();
		return;
	}
	/* a new vote */
	if (votekick_gap && !votekick_time_reached(now, votekick_next_allowed))
	{
		snprintf(notice, sizeof(notice), "votekick: the next vote can start in %lu seconds",
			(votekick_next_allowed - now + 999) / 1000);
		votekick_refuse(machine_index, notice);
		return;
	}
	if (!voter.host && votekick_played_ticks[machine_index] < votekick_starter_ticks())
	{
		long minutes = votekick_starter_ticks() / (60 * TICKS_PER_SECOND);

		snprintf(notice, sizeof(notice),
			"votekick: play %ld minute%s on this server to start a vote (%ld played)", minutes,
			minutes == 1 ? "" : "s", votekick_played_ticks[machine_index] / (60 * TICKS_PER_SECOND));
		votekick_refuse(machine_index, notice);
		return;
	}
	if (!voter.host && votekick_remembered(votekick_cooling_starters, voter.address, voter.hardware_id, &left))
	{
		snprintf(notice, sizeof(notice), "votekick: your last vote failed: wait %lu seconds", (left + 999) / 1000);
		votekick_refuse(machine_index, notice);
		return;
	}
	if (votekick_remembered(votekick_protected_targets, target.address, target.hardware_id, &left))
	{
		snprintf(notice, sizeof(notice), "votekick: a vote against that player failed lately: wait %lu seconds",
			(left + 999) / 1000);
		votekick_refuse(machine_index, notice);
		return;
	}
	csmemset(&votekick, 0, sizeof(votekick));
	votekick.active = TRUE;
	votekick.target_machine = target_machine;
	votekick.target_generation = votekick_generations[target_machine];
	votekick.target_player = player_index;
	votekick.target_address = target.address;
	csstrcpy(votekick.target_hardware_id, target.hardware_id);
	votekick_names(target_machine, votekick.target_names, sizeof(votekick.target_names));
	votekick.starter_address = voter.address;
	csstrcpy(votekick.starter_hardware_id, voter.hardware_id);
	votekick.started_at = now;
	if (machine_index == NONE)
		votekick.host_voted = TRUE;
	else
	{
		votekick.voted[machine_index] = TRUE;
		votekick.voted_generations[machine_index] = votekick_generations[machine_index];
	}
	votekick_recount();
	votekick_names(machine_index, names, sizeof(names));
	snprintf(notice, sizeof(notice), "%s started a vote to kick %s (%d of %d): open the scores and pick the name to vote",
		names, votekick.target_names, votekick.votes, votekick.needed);
	distributed_send_notice(notice);
	error(_error_log, "votekick: started by machine %ld against machine %ld", machine_index, target_machine);
	if (votekick.votes >= votekick.needed)
		votekick_pass();
	else
		votekick_send_status();
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
	if (request.player_index >= MAXIMUM_TRACKED_PLAYERS)
		return;
	votekick_host_request(machine_index, request.player_index);
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
	if (!votekick_target_present())
	{
		char notice[160];

		snprintf(notice, sizeof(notice), "%s left: the vote ended", votekick.target_names);
		distributed_send_notice(notice);
		votekick_end();
		return;
	}
	/* (those gone counted no more, and those who have since played long
	enough to vote among those who may) */
	if (game_time_get() % TICKS_PER_SECOND == 0)
	{
		votekick_recount();
		if (votekick.votes >= votekick.needed)
		{
			votekick_pass();
			return;
		}
		if (votekick_time_reached(system_milliseconds(), votekick.started_at + VOTE_SECONDS * 1000))
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
	status->player_index = NONE;
	if (votekick_host())
	{
		struct votekick_voter voter;
		unsigned long elapsed;

		if (!votekick.active)
			return FALSE;
		votekick_voter_get(NONE, &voter);
		elapsed = system_milliseconds() - votekick.started_at;
		status->player_index = votekick.target_player;
		status->votes = votekick.votes;
		status->needed = votekick.needed;
		status->seconds_left = (short)MAX(0, VOTE_SECONDS - (long)(elapsed / 1000));
		status->voted = votekick.host_voted;
		status->may_vote = TRUE;
		return TRUE;
	}
	if (game_connection() != _game_connection_network_client || !votekick_status_heard ||
		votekick_time_reached(system_milliseconds(), votekick_status_heard + STATUS_TIMEOUT_MILLISECONDS) ||
		votekick_status.player_index == NO_PLAYER)
	{
		return FALSE;
	}
	status->player_index = distributed_player_from_byte(votekick_status.player_index) == NONE ? NONE :
		votekick_status.player_index;
	status->votes = votekick_status.votes;
	status->needed = votekick_status.needed;
	status->seconds_left = votekick_status.seconds_left;
	status->voted = TEST_FLAG(votekick_status.flags, _votekick_status_voted_bit);
	status->may_vote = TEST_FLAG(votekick_status.flags, _votekick_status_may_vote_bit);
	return status->player_index != NONE;
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

boolean network_votekick_request(
	short player_index)
{
	struct
	{
		struct distributed_message_header header;
		struct distributed_votekick_request request;
	} message;

	if (!distributed_player(player_index))
		return FALSE;
	if (distributed_player_is_local(player_index))
	{
		console_warning("votekick: not against your own players");
		return FALSE;
	}
	if (votekick_host())
	{
		votekick_host_request(NONE, player_index);
		return TRUE;
	}
	if (game_connection() != _game_connection_network_client)
	{
		console_warning("votekick: only in a network game");
		return FALSE;
	}
	csmemset(&message, 0, sizeof(message));
	message.request.player_index = distributed_player_to_byte(player_index);
	distributed_send(&message, _distributed_message_votekick, 1, (word)sizeof(message), _distributed_to_host_reliably);
	return TRUE;
}
