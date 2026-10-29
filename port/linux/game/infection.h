/* infection.h: Infection, a mode of team Slayer (port/linux/game/infection.c) */
#ifndef HALO_INFECTION_H
#define HALO_INFECTION_H

struct game_engine;
struct render_fog;
union real_rgb_color;
union real_vector3d;

/* the game being played is Infection (game_modes_mode's) */
boolean infection_enabled(void);
/* Slayer's engine with Infection's rules (game_modes_engine) */
struct game_engine *infection_engine_get(struct game_engine const *slayer);
/* a player spawned: a zombie's skull (game_modes_postspawn) */
void infection_postspawn(long player_index);

/* Floodfection's look (the gametype's mode_variant): a player's armour
colour in place of its team's, a zombie's green and a human's their own
(game_engine_player_get_change_color), and the fog over the map in place of
its own, closer indoors (the camera's cluster has no sky), TRUE when it is
(render.c: the sky is then not drawn) */
void infection_change_color(long player_index, union real_rgb_color *color);
/* the game being played is Floodfection */
boolean infection_floodfection_enabled(void);
/* the object is a Floodfection zombie's unit: hit, it bleeds as the Flood
(projectiles.c), and a headshot does not kill it outright (damage.c) */
boolean infection_floodfection_zombie(long object_index);
boolean infection_fog(short local_player_index, boolean indoors, struct render_fog *fog);
/* the level loading is a Floodfection game's, and its tags of Two Betrayals
(the burst, the flood bits, the zombies' forms) added to the level's import
(level_tag_import.c, tag_import.h) */
boolean infection_tag_import_wanted(void);
void infection_tag_import(void);
/* while set, an effect made does no damage and makes no objects but
garbage, which every machine keeps for itself (Floodfection's burst and
bits, played by each machine of a multiplayer game for itself: effects.c) */
extern boolean infection_effect_local;

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
