/*
ITEM_STUCK.C

A weapon, a piece of equipment or garbage that falls (items.c, item_update)
comes to rest only on ground no steeper than 45 degrees (the structure,
scenery or a device), and only once it meets it slowly. Wedged where it
cannot (in a crease, against a wall, on a steep slope, where each tick's hit
put it back where the tick began while gravity went on adding to its speed),
it hit the ground every tick without getting anywhere, on the Xbox too: it
spun on (an item's spin slows only at rest) and played its collision sound
with each hit, as loud as it was fast, until it was picked up.

An item that hits the ground where it does not rest on ITEM_STUCK_TICKS
ticks in a row (a third of a second), moving no further than
ITEM_STUCK_DISTANCE (3 cm) in any of them, is put at rest where it is: as
one that lands, but in the pose it has (not turned to lie on what it hit, a
wall or a slope) and no longer spinning. A tick it does not hit the ground,
or moves further, starts the count again, so an item that bounces, slides or
falls is never stopped: each of its hits is a tick in the air, or a move
apart. The host's items are the ones every machine plays (NETCODE.md,
corrections), and whether each is at rest is the host's word, as before.
*/

/* ---------- headers */

#include "cseries.h"
#include "item_stuck.h"

/* ---------- constants */

#define ITEM_STUCK_TICKS 10
#define ITEM_STUCK_DISTANCE 0.01f

/* ---------- public code */

boolean item_stuck_count(
	short *stuck_ticks,
	boolean hit_ground,
	real distance_moved)
{
	if (!hit_ground || !(distance_moved <= ITEM_STUCK_DISTANCE))
	{
		*stuck_ticks = 0;
		return FALSE;
	}

	if (*stuck_ticks < 0 || *stuck_ticks >= ITEM_STUCK_TICKS)
	{
		*stuck_ticks = 0;
	}
	(*stuck_ticks)++;
	if (*stuck_ticks < ITEM_STUCK_TICKS)
	{
		return FALSE;
	}

	*stuck_ticks = 0;

	return TRUE;
}
