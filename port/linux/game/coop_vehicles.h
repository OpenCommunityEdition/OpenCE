/* The host's optional campaign vehicle copies (coop_vehicles.c). */
#ifndef __COOP_VEHICLES_H
#define __COOP_VEHICLES_H

struct scenario_object_datum;
void coop_vehicles_new_game(void);
void coop_vehicles_register(long object_index, struct scenario_object_datum const *source);
void coop_vehicles_update(void);
void coop_vehicles_before_save(void);
void coop_vehicles_after_load(void);

#endif
