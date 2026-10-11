/*
RAGDOLL.H

Dynamic ragdolls for dead bipeds (port/linux/game/ragdoll.c), simulated by
Jolt Physics (port/linux/physics/ragdoll_physics.cpp).
*/

#ifndef __RAGDOLL_H
#define __RAGDOLL_H
#pragma once

/* compile-time switch: 0 compiles every hook out, the game is stock. On by
itself where the build has Jolt (port/third_party/JoltPhysics: the build
defines HALO_JOLT) */
#ifndef RAGDOLLS
#ifdef HALO_JOLT
#define RAGDOLLS 1
#else
#define RAGDOLLS 0
#endif
#endif

/* a bit of the biped's flags (bipeds.h) the stock game never uses: the
biped's body is the ragdoll's (simulated, or frozen where it came to rest) */
#define BIPED_PORT_RAGDOLL_BIT 16
/* the biped's skeleton cannot be a ragdoll: it dies as stock */
#define BIPED_PORT_RAGDOLL_FAILED_BIT 17
/* (biped flags: died as the game plays it, not a ragdoll: falling to its
death, killed in a vehicle's seat and thrown out) */
#define BIPED_PORT_RAGDOLL_CLASSIC_BIT 18

#if RAGDOLLS

struct damage_data;

void ragdolls_initialize_for_new_map(
	void);
void ragdolls_dispose_from_old_map(
	void);

/* from biped_update, first: TRUE when the ragdoll has the biped this tick
(dead and simulated, or dead and frozen), and biped_update does nothing else */
boolean ragdoll_biped_update(
	long biped_index);

/* from unit_kill: a unit killed in a seat (a vehicle's), before it is
thrown out: it dies as the game has it die */
void ragdoll_unit_killed_in_seat(
	long unit_index);

/* from biped_delete */
void ragdoll_biped_deleted(
	long biped_index);

/* the damage categories that push a body their own way (damage.c's
_damage_category_sniper, _damage_category_melee) */
#define RAGDOLL_DAMAGE_CATEGORY_FALLING 1
#define RAGDOLL_DAMAGE_CATEGORY_SNIPER 5
#define RAGDOLL_DAMAGE_CATEGORY_MELEE 6

/* from object_damage_aftermath: a hit (or a killing blow) pushes the body;
nothing here changes the damage done, only how the body moves */
void ragdoll_note_damage(
	long object_index,
	struct damage_data const *damage,
	real damage_amount,
	real instantaneous_acceleration,
	real acceleration_scale,
	boolean lethal,
	short damage_category);

/* ... the killing blow just noted was a multiplayer game's headshot (a
sniper's: the body always goes limp at once; a pistol's, or another weapon's
that headshots in multiplayer: half of them, by the body, the others play
the death animation as any other death) */
void ragdoll_note_multiplayer_headshot(
	long object_index);

/* network play, where each machine may have ragdolls or not: only how this
machine draws the dead, nothing the game decides (port/linux/NETCODE.md).
A client keeps a body its ragdoll has where it lies here, unless the host
moves it far (TRUE: the host's position not taken); a host reports a body
its ragdoll has where it fell, still (FALSE: the object's own) */
boolean ragdoll_network_keeps_position(
	long object_index,
	real_point3d const *host_position);
boolean ragdoll_network_reported(
	long object_index,
	real_point3d *position);

/* from object_set_position: a ragdolled body moved (the host's word on where
it lies, a script, a teleport) moves whole, in the pose it has; TRUE when it
was, and object_set_position does nothing else (it would put the body back in
its animation's pose) */
boolean ragdoll_object_set_position(
	long object_index,
	real_point3d const *position);

/* from biped_render_debug */
void ragdoll_render_debug(
	long biped_index);

/* from hs_compile_and_evaluate_command: NONE when the expression is no
ragdoll command, else whether the command succeeded */
long ragdoll_console_command(
	char const *expression);

#endif // RAGDOLLS

#endif // __RAGDOLL_H
