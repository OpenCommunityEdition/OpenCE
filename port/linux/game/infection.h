#ifndef INFECTION_H
#define INFECTION_H

struct game_variant;
struct infection_options;
struct game_engine;
union real_vector3d;

struct infection_options const *infection_options_get(struct game_variant const *variant);
struct infection_options *infection_options_edit(struct game_variant *variant);
boolean infection_enabled(void);
struct game_engine *infection_engine_get(void);
boolean infection_unit_is_zombie(long unit_index);
boolean infection_allow_weapon_drop(long unit_index, long weapon_index);
boolean infection_can_delete_skull(long object_index);
real infection_melee_multiplier(long unit_index);
boolean infection_melee_rider_target(long attacker_index, long target_index);
boolean infection_melee_rider_visible(long attacker_index, long target_index, union real_vector3d const *direction);
void infection_reconcile_player(long player_index);
long infection_remaining_ticks(void);
long infection_write_network_state(byte *buffer, long size);
boolean infection_read_network_state(byte const *buffer, long size, boolean first);

#endif
