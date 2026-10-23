/* infection.h: Infection, a mode of team Slayer (port/linux/game/infection.c) */
#ifndef HALO_INFECTION_H
#define HALO_INFECTION_H

struct game_engine;
union real_vector3d;

/* the game being played is Infection (game_modes_mode's) */
boolean infection_enabled(void);
/* Slayer's engine with Infection's rules (game_modes_engine) */
struct game_engine *infection_engine_get(struct game_engine const *slayer);
/* a player spawned: a zombie's skull (game_modes_postspawn) */
void infection_postspawn(long player_index);

/* a living zombie's unit */
boolean infection_unit_is_zombie(long unit_index);
/* a player may drop the weapon (a zombie keeps its skull: players.c, units.c) */
boolean infection_allow_weapon_drop(long unit_index, long weapon_index);
/* the skull may be deleted (weapons.c's and objects.c's checks that a
game's objective is not) */
boolean infection_can_delete_skull(long object_index);
/* a unit's melee damage multiplier: a zombie's skull's */
real infection_melee_multiplier(long unit_index);
/* a zombie's melee hits a human riding in an open seat, not the vehicle
(unit_cause_player_melee_damage) */
boolean infection_melee_rider_target(long attacker_index, long target_index);
/* ... which the host checks of a client's report: its own line from the
attacker's head to the rider (network_damage.c) */
boolean infection_melee_rider_visible(long attacker_index, long target_index, union real_vector3d const *direction);
/* a player's team, speed and shields as their role has them (a unit
attached: network_player_attach_unit) */
void infection_reconcile_player(long player_index);
/* the round's time left (the score's title) */
long infection_remaining_ticks(void);
/* the state the host sends its clients (game_engine_write_network_state) */
long infection_write_network_state(byte *buffer, long size);
boolean infection_read_network_state(byte const *buffer, long size, boolean first);

#endif
