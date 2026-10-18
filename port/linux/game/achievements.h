/*
ACHIEVEMENTS.H

The calls Halo's own code (source/) makes into achievements.c: optional
achievements (the game.achievements setting, off by default). Each call
returns at once when they are off. Nothing here is sent to another machine.
*/

#ifndef __ACHIEVEMENTS_H
#define __ACHIEVEMENTS_H

struct damage_data;

/* a map is loaded (game_initialize_for_new_map), every tick, the level won */
void achievements_new_map(void);
void achievements_tick(void);
void achievements_won(void);

/* a checkpoint saved, and one loaded (a revert); a saved game resumed; a
debug core loaded */
void achievements_checkpoint_save(void);
void achievements_checkpoint_load(void);
void achievements_note_resumed(void);
void achievements_note_core_loaded(void);

/* a console, telnet or cheat-button command: it disqualifies the level */
void achievements_note_command(char const *expression);

/* what the players did */
void achievements_note_damage(long object_index, struct damage_data const *damage,
	unsigned long being_damaged_flags, float shield_damage, float body_damage);
void achievements_note_player_died(long player_index);
void achievements_note_pickup(long player_index, short kind, long definition_index);
void achievements_note_vehicle_entered(long player_index, long vehicle_index);
void achievements_note_weapon_fired(long weapon_index, long owner_object_index);
void achievements_note_projectile_attached(long projectile_index, long target_object_index);
void achievements_note_jump(long biped_index);

/* the unlock notice, drawn over every frame (interface_draw_fullscreen_overlays) */
void achievements_render(void);

#endif
