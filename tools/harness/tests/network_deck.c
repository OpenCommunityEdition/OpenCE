/*
NETWORK_DECK.C (test)

The deck part of a unit's state on the wire (port/linux/game/
network_distributed.c's distributed_unit_state_write and _read, and
vehicle_riding.c's checks of it), taken from the sources by
test_network_deck.py, against a fake count of a vehicle's nodes: written
and read back whole, refused whole with the rest of the state read on, a
value not finite never written, and a state with a part this version does
not know, or a deck part where no place is, refused.
*/

#include "harness.h"
#include <math.h>
typedef unsigned short word;
typedef struct { real i, j; } real_vector2d;
#define csmemset memset
#define csmemcpy memcpy
#define SET_FLAG(v, b, on) ((v) = (on) ? ((v) | FLAG(b)) : ((v) & ~FLAG(b)))
#include "types.inc"

/* the vehicle the parts name, with three nodes; any other index is no
vehicle on this machine (deleted, its slot reused, or another object) */
#define VEHICLE 0x10000001
static int node_count_asked;

short biped_riding_vehicle_node_count(long vehicle_index)
{
	node_count_asked++;
	return vehicle_index == VEHICLE ? 3 : NONE;
}

/* (the scripted pose validation: this fixture sends none) */
static boolean distributed_scripted_pose_valid(struct distributed_unit_state const *state) { return TRUE; }
#include "under_test.inc"

#define NEAR(a, b) CHECK(fabs((double)(a) - (double)(b)) < .001, "%s: %g, not %g", #a, (double)(a), (double)(b))
/* where the deck part starts in living(): after the player, flags and
parts, the unit, and the placed parts (no seat, up straight up) */
#define DECK_AT (3 + 4 + 12 + 6 + 6)

/* a living, placed player riding VEHICLE's node 2 */
static struct distributed_unit_state living(void)
{
	struct distributed_unit_state state;

	memset(&state, 0, sizeof(state));
	state.player_index = 1;
	state.flags = FLAG(_distributed_unit_alive_bit) | FLAG(_distributed_unit_placed_bit);
	state.killing_player_index = NO_PLAYER;
	state.unit_index = 0x20000002;
	state.vehicle_index = NONE;
	state.seat_index = NONE;
	state.position.x = 10.f;
	state.position.y = 20.f;
	state.position.z = 30.f;
	state.up.k = (short)DISTRIBUTED_UNIT_SCALE;
	state.body_vitality = state.shield_vitality = 16384;
	state.deck_present = TRUE;
	state.deck.vehicle_index = VEHICLE;
	state.deck.node_index = 2;
	state.deck.ticks = 60;
	state.deck.place.x = 1.5f;
	state.deck.place.y = -.25f;
	state.deck.place.z = .75f;
	state.deck.velocity.i = .125f;
	state.deck.velocity.j = -.0625f;
	state.deck.velocity.k = .5f;
	return state;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	struct distributed_unit_state state;
	struct distributed_unit_state decoded;
	struct vehicle_riding_deck empty;
	byte buffer[DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE + 2];
	word size;

	memset(&empty, 0, sizeof(empty));
	CASE("deck-standing-verdict")
	{
		struct vehicle_riding_state riding;
		struct vehicle_riding_vehicle vehicle;
		struct vehicle_riding_cabin cabin;
		real_point3d base = { 1.f, 0.f, .5f };
		real_vector3d world = { 0.f, 0.f, 0.f };
		short verdict;

		memset(&riding, 0, sizeof(riding));
		memset(&vehicle, 0, sizeof(vehicle));
		memset(&cabin, 0, sizeof(cabin));
		vehicle.object_index = VEHICLE;
		vehicle.node_index = NONE;
		vehicle.forward.i = vehicle.body_forward.i = 1.f;
		vehicle.up.k = vehicle.body_up.k = 1.f;
		riding.ticks = 60;
		riding.object_index = VEHICLE;
		riding.node_index = NONE;
		cabin.unit_index = 0x20000002;
		cabin.vehicle_index = VEHICLE;
		/* a standing rider's verdict, made by the writer, through the wire and back: the same 28 bytes, falling clear */
		for (verdict = _vehicle_riding_enclosure_open; verdict <= _vehicle_riding_enclosure_enclosed; verdict++)
		{
			cabin.standing = verdict;
			state = living();
			CHECK(vehicle_riding_deck_make(&riding, &vehicle, &base, &world, NULL, &cabin, &state.deck),
				"a standing rider's part refused");
			size = distributed_unit_state_write(&state, TRUE, buffer);
			CHECK(size == DECK_AT + DISTRIBUTED_DECK_SIZE + 4, "a rider's state of %d bytes", (int)size);
			CHECK(distributed_unit_state_read(buffer, buffer + size, &decoded) == size && decoded.deck_present,
				"a standing verdict's part lost");
			CHECK(decoded.deck.ticks == 60 && (decoded.deck.flags >> VEHICLE_RIDING_DECK_VERDICT_SHIFT) == verdict &&
				!TEST_FLAG(decoded.deck.flags, _vehicle_riding_deck_falling_bit), "verdict %d riding: flags %u", verdict,
				(unsigned)decoded.deck.flags);
		}
		return 0;
	}
	CASE("deck-roundtrip")
	{
		short verdict;

		CHECK(DISTRIBUTED_DECK_SIZE == 28, "a deck part of %d bytes", (int)DISTRIBUTED_DECK_SIZE);
		CHECK(DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE == 122, "a state of at most %d bytes",
			(int)DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE);
		/* every part a living state has: one byte under the most, in the buffer
		the message loops give it (the guard bytes either side kept) */
		state = living();
		state.vehicle_index = 0x10010002;
		state.seat_index = 1;
		state.up.i = 100;
		state.up.k = 32000;
		state.flags |= FLAG(_distributed_unit_predicted_bit);
		state.predicted_time = 123;
		state.current_body_damage = state.recent_shield_damage = 7;
		state.powerup_durations[0] = 9;
		state.unit_flags = 1;
		state.active_camouflage = 5;
		state.scripted_pose = TRUE;
		memset(buffer, 0xA5, sizeof(buffer));
		size = distributed_unit_state_write(&state, TRUE, buffer + 1);
		CHECK(size == DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE - 1, "a full living state of %d bytes", (int)size);
		CHECK(buffer[0] == 0xA5 && buffer[DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE + 1] == 0xA5, "past the buffer");
		CHECK(distributed_unit_state_read(buffer + 1, buffer + 1 + size, &decoded) == size, "not read back whole");
		CHECK(decoded.deck_present && decoded.deck.vehicle_index == VEHICLE && decoded.deck.node_index == 2 &&
			decoded.deck.ticks == 60 && decoded.deck.flags == 0, "the deck's vehicle, node, ticks or flags");
		NEAR(decoded.deck.place.x, 1.5f);
		NEAR(decoded.deck.place.y, -.25f);
		NEAR(decoded.deck.place.z, .75f);
		NEAR(decoded.deck.velocity.i, .125f);
		NEAR(decoded.deck.velocity.j, -.0625f);
		NEAR(decoded.deck.velocity.k, .5f);
		NEAR(decoded.deck.peak, 0.f);
		CHECK(decoded.predicted_time == 123 && decoded.position.x == 10.f && decoded.powerup_durations[0] == 9 &&
			decoded.active_camouflage == 5, "the parts after the deck misread");
		/* in the air: falling and each verdict as they were, the air frame and the peak */
		for (verdict = 0; verdict <= 2; verdict++)
		{
			state = living();
			state.deck.ticks = 40;
			state.deck.flags = (byte)(FLAG(_vehicle_riding_deck_falling_bit) | (verdict << VEHICLE_RIDING_DECK_VERDICT_SHIFT));
			state.deck.velocity.i = .25f;
			state.deck.velocity.j = .375f;
			state.deck.velocity.k = -.125f;
			state.deck.peak = .5f;
			size = distributed_unit_state_write(&state, TRUE, buffer);
			CHECK(size == DECK_AT + DISTRIBUTED_DECK_SIZE + 4, "a jumper's state of %d bytes", (int)size);
			CHECK(distributed_unit_state_read(buffer, buffer + size, &decoded) == size && decoded.deck_present,
				"a jumper's part lost");
			CHECK(decoded.deck.ticks == 40 && decoded.deck.flags == state.deck.flags, "verdict %d: flags %u", verdict,
				(unsigned)decoded.deck.flags);
			NEAR(decoded.deck.velocity.j, .375f);
			NEAR(decoded.deck.peak, .5f);
		}
		/* a peak past what 16 bits carry comes back as the most they do */
		state.deck.peak = 40.f;
		size = distributed_unit_state_write(&state, TRUE, buffer);
		CHECK(distributed_unit_state_read(buffer, buffer + size, &decoded) == size && decoded.deck_present,
			"a large peak's part lost");
		NEAR(decoded.deck.peak, 32767.f/1024.f);
		/* without the part: 28 bytes fewer, and none read */
		state.deck_present = FALSE;
		size = distributed_unit_state_write(&state, TRUE, buffer);
		CHECK(size == DECK_AT + 4, "a state with no part of %d bytes", (int)size);
		CHECK(distributed_unit_state_read(buffer, buffer + size, &decoded) == size && !decoded.deck_present &&
			!memcmp(&decoded.deck, &empty, sizeof(empty)), "a part read where none was");
		return 0;
	}
	CASE("deck-truncate")
	{
		word cut;

		state = living();
		size = distributed_unit_state_write(&state, TRUE, buffer);
		for (cut = 0; cut < size; cut++)
		{
			CHECK(distributed_unit_state_read(buffer, buffer + cut, &decoded) == 0, "cut at %d read", (int)cut);
		}
		return 0;
	}
	CASE("deck-invalid-whole")
	{
		short fault;

		for (fault = 0; fault < 11; fault++)
		{
			long index = fault == 0 ? 0x10020003 : 0x20000001;
			short node = fault == 2 ? 3 : -2;
			real value = fault == 4 ? NAN : INFINITY;
			short peak = -1;
			byte byte_value = fault == 6 ? 0 : fault == 7 ? 61 : fault == 8 ? 6 : 8;

			state = living();
			size = distributed_unit_state_write(&state, TRUE, buffer);
			CHECK(!memcmp(buffer + DECK_AT, &state.deck.vehicle_index, 4), "the part is not where this case writes");
			/* no such vehicle; a reused slot (another identifier); a node past the
			vehicle's, or below 0; a place not finite; no window, too long a
			window; a verdict of 3; another flag; a peak below 0 */
			if (fault <= 1)
				memcpy(buffer + DECK_AT, &index, 4);
			else if (fault <= 3)
				memcpy(buffer + DECK_AT + 4, &node, 2);
			else if (fault <= 5)
				memcpy(buffer + DECK_AT + 6 + (fault == 4 ? 0 : 8), &value, 4);
			else if (fault <= 7)
				buffer[DECK_AT + 24] = byte_value;
			else if (fault <= 9)
				buffer[DECK_AT + 25] = byte_value;
			else
				memcpy(buffer + DECK_AT + 26, &peak, 2);
			CHECK(distributed_unit_state_read(buffer, buffer + size, &decoded) == size, "fault %d: the state's bytes lost", fault);
			CHECK(!decoded.deck_present && !memcmp(&decoded.deck, &empty, sizeof(empty)), "fault %d: a part kept in pieces", fault);
			CHECK(decoded.position.x == 10.f && decoded.body_vitality == 16384, "fault %d: the rest of the state lost", fault);
		}
		return 0;
	}
	CASE("deck-cabin-bit")
	{
		byte flags[3] = { FLAG(_vehicle_riding_deck_cabin_bit) | (2 << VEHICLE_RIDING_DECK_VERDICT_SHIFT),
			FLAG(_vehicle_riding_deck_cabin_bit), 0x10 | (1 << VEHICLE_RIDING_DECK_VERDICT_SHIFT) };
		short fault;

		/* a jump's part with the bit, falling and a verdict beside it: the same 28 bytes, flags as they were */
		state = living();
		state.deck.ticks = 40;
		state.deck.flags = (byte)(FLAG(_vehicle_riding_deck_falling_bit) | (1 << VEHICLE_RIDING_DECK_VERDICT_SHIFT) |
			FLAG(_vehicle_riding_deck_cabin_bit));
		state.deck.peak = .5f;
		size = distributed_unit_state_write(&state, TRUE, buffer);
		CHECK(size == DECK_AT + DISTRIBUTED_DECK_SIZE + 4, "a part with the bit of %d bytes", (int)size);
		CHECK(distributed_unit_state_read(buffer, buffer + size, &decoded) == size && decoded.deck_present,
			"a part with the bit lost");
		CHECK(decoded.deck.flags == state.deck.flags && TEST_FLAG(decoded.deck.flags, _vehicle_riding_deck_falling_bit) &&
			((decoded.deck.flags >> VEHICLE_RIDING_DECK_VERDICT_SHIFT) & VEHICLE_RIDING_DECK_VERDICT_MASK) ==
			_vehicle_riding_enclosure_open, "flags %u", (unsigned)decoded.deck.flags);
		/* at ticks 60, with no verdict, or with a bit from 4 up: dropped whole, the rest of the state read on */
		for (fault = 0; fault < 3; fault++)
		{
			state = living();
			state.deck.ticks = fault == 0 ? 60 : 40;
			state.deck.flags = fault == 0 ? (byte)(1 << VEHICLE_RIDING_DECK_VERDICT_SHIFT) : 0;
			size = distributed_unit_state_write(&state, TRUE, buffer);
			CHECK(size == DECK_AT + DISTRIBUTED_DECK_SIZE + 4, "fault %d: a part of %d bytes", fault, (int)size);
			buffer[DECK_AT + 25] = flags[fault];
			CHECK(distributed_unit_state_read(buffer, buffer + size, &decoded) == size,
				"fault %d: the state's bytes lost", fault);
			CHECK(!decoded.deck_present && !memcmp(&decoded.deck, &empty, sizeof(empty)), "fault %d: a part kept", fault);
			CHECK(decoded.position.x == 10.f && decoded.body_vitality == 16384, "fault %d: the rest lost", fault);
		}
		return 0;
	}
	CASE("deck-zero-salt")
	{
		long index = 0x00000001;

		state = living();
		size = distributed_unit_state_write(&state, TRUE, buffer);
		memcpy(buffer + DECK_AT, &index, 4);
		node_count_asked = 0;
		CHECK(distributed_unit_state_read(buffer, buffer + size, &decoded) == size && !decoded.deck_present,
			"an identifier of 0 resolved");
		CHECK(node_count_asked == 0, "an identifier of 0 looked up");
		return 0;
	}
	CASE("deck-not-finite")
	{
		state = living();
		state.deck.velocity.j = NAN;
		CHECK(distributed_unit_state_write(&state, TRUE, buffer) == DECK_AT + 4, "a velocity not finite written");
		state = living();
		state.deck.ticks = 40;
		state.deck.peak = INFINITY;
		CHECK(distributed_unit_state_write(&state, TRUE, buffer) == DECK_AT + 4, "a peak not finite written");
		return 0;
	}
	CASE("deck-grammar")
	{
		state = living();
		size = distributed_unit_state_write(&state, TRUE, buffer);
		/* seated (not placed), and dead: no place for the part to be read after */
		buffer[1] &= (byte)~FLAG(_distributed_unit_placed_bit);
		CHECK(distributed_unit_state_read(buffer, buffer + size, &decoded) == 0, "a deck part read with no place");
		size = distributed_unit_state_write(&state, TRUE, buffer);
		buffer[1] &= (byte)~FLAG(_distributed_unit_alive_bit);
		CHECK(distributed_unit_state_read(buffer, buffer + size, &decoded) == 0, "a deck part read on a dead state");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
