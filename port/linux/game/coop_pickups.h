/*
COOP_PICKUPS.H

The co-op host's extra campaign pickups (coop_pickups.c).
*/
#ifndef __COOP_PICKUPS_H
#define __COOP_PICKUPS_H

struct scenario_object_datum;
struct tag_block;

void coop_pickups_new_game(void);
void coop_pickups_register(long object_index, struct scenario_object_datum const *source, struct tag_block *palette);
void coop_pickups_update(void);

#endif
