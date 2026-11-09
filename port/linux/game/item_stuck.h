/*
ITEM_STUCK.H

An item wedged where it cannot come to rest, hitting the ground every tick
without getting anywhere, put at rest where it is (item_stuck.c).
*/

#ifndef __ITEM_STUCK_H
#define __ITEM_STUCK_H

/* counts a falling item's ticks (stuck_ticks, its own): one it hit the
ground (the structure, scenery or a device) where it does not rest, moving
no further than ITEM_STUCK_DISTANCE, is one more; any other starts the count
again. TRUE (and the count starts again) once it has hit it so on
ITEM_STUCK_TICKS ticks in a row: the item is put at rest where it is */
boolean item_stuck_count(short *stuck_ticks, boolean hit_ground, real distance_moved);

#endif
