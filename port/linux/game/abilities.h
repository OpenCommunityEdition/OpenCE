/* abilities.h: sprint and armour lock (port/linux/game/abilities.c)

Every player sprints; armour lock is a gametype's (OPENCE OPTIONS' ARMOR
LOCK: game_engine.h's game_variant_options.armor_lock). Their controls ride
above the unit's own in the player's action (units.h's
UNIT_CONTROL_PORT_SPRINT_BIT and UNIT_CONTROL_PORT_ARMOR_LOCK_BIT), so every
machine runs them from the same input as it runs the rest of the player's
movement; only the host's copy decides damage. */
#ifndef HALO_ABILITIES_H
#define HALO_ABILITIES_H

struct player_action;

/* the player's action for its unit this tick (players.c, before the unit is
controlled): sprinting lowers the weapon (no trigger, grenade, reload or
zoom), and armour lock holds the unit still, crouched, with nothing pressed
but the lock. TRUE while locked: the unit then keeps facing where it faced */
boolean abilities_player_action(long player_index, long unit_index, struct player_action *action);
/* how much faster a unit moves than it runs (bipeds.c's player physics):
1 unless it sprints */
real abilities_speed_scale(long unit_index);
/* the fastest any player moves for an ability, of their run (the host's
check of a client's moves, network_distributed.c) */
real abilities_maximum_speed_scale(void);
/* whether an object takes no damage for an ability: a unit in armour lock
(damage.c's object_cause_damage, on the host) */
boolean abilities_blocks_damage(long object_index);
/* a unit's energy for its sprint and its armour lock, 0 to 1, and whether
each is in use; FALSE for a unit that is no player's (or not run lately) */
boolean abilities_unit_state(long unit_index, real *sprint_energy, boolean *sprinting, real *armor_lock_energy,
	boolean *armor_locked);

#endif
