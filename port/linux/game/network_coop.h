/*
NETWORK_COOP.H

The calls Halo's own code (source/) makes into network_coop.c. On a co-op
host the note_ functions send what the scripts just did to the clients; on
any other machine they do nothing, so the clients can call the same engine
functions to apply what they receive.
*/

#ifndef __NETWORK_COOP_H
#define __NETWORK_COOP_H

/* ---------- constants */

/* network_coop_note_hud */
enum
{
	_coop_hud_help_text,
	_coop_hud_objective,
	_coop_hud_help_flash,
	_coop_hud_messages_clear,
	_coop_hud_checkpoint,
};

/* network_coop_note_player_effect */
enum
{
	_coop_player_effect_translation,
	_coop_player_effect_rotation,
	_coop_player_effect_start,
	_coop_player_effect_stop,
};

/* network_coop_note_nav_point */
enum
{
	_coop_nav_point_team_flag,
	_coop_nav_point_team_object,
	_coop_nav_point_unit_flag,
	_coop_nav_point_unit_object,
};

/* ---------- prototypes/NETWORK_COOP.C */

/* whether this is a network co-op game (not its lobby, whose menu scene also
has no game engine) */
boolean network_coop_active(void);
/* game_state.c: remember the team only after a checkpoint write succeeds. */
void network_coop_checkpoint_saved(void);
/* main.c: host team-wipe policy, including checkpoint roster compatibility. */
boolean network_coop_wipe_reverts(void);
/* whether the players collide with each other: FALSE in a co-op game whose
host set Server Setup's PLAYER COLLISIONS off (bipeds.c, collisions.c) */
boolean network_coop_player_collisions(void);
/* whether the players hurt their AI allies: FALSE in a co-op game whose host
set Server Setup's ALLY FRIENDLY FIRE off (game_engine.c's
game_engine_friendly_damage) */
boolean network_coop_ally_friendly_fire(void);
/* whether this machine's players play with friendly fire between players
(network.coop_own_friendly_fire: the pause menu's FRIENDLY FIRE) */
boolean network_coop_own_friendly_fire(void);
/* host: the choice a client's player's input says is theirs */
void network_coop_note_player_friendly_fire(long player_index, boolean friendly_fire);
/* host: whether one player's hit may hurt another: both chose friendly fire,
or the one hit has been away from the controls for over 70 seconds */
boolean network_coop_players_friendly_fire(long attacker_player_index, long victim_player_index);
/* host: whether the player has been away from the controls for over 70
seconds (as friendly fire counts it) */
boolean network_coop_player_away(long player_index);
/* whether the game's host is a dedicated server (its co-op flag), whose
loading zones ask for most of the players (players.c) */
boolean network_coop_host_dedicated(void);
/* host: a dedicated server's loading zones: whether a zone switches the BSP
only for most of the living players who are not away (server.coop_bsp_switch
"majority"), and the ticks after a switch before a zone may switch it again
(server.coop_bsp_switch_cooldown_seconds; 0 elsewhere) */
boolean network_coop_bsp_switch_majority(void);
long network_coop_bsp_switch_cooldown_ticks(void);
/* host: a player standing in a loading zone that needs more players (more:
how many; in, total: those in such zones and those who count), or one that
waits for the cooldown (more 0, seconds left in total): told every few
seconds */
void network_coop_tell_bsp_switch_waiting(long player_index, long more, long in, long total);
/* host: a loading zone switched the BSP (from, to the structure BSP
indices), crossed by these players: everyone told who, the server's log and
an event (coop_bsp_switch) */
void network_coop_note_bsp_switch(short from, short to, long const *player_indices, long count);
/* whether this machine is a co-op client, whose devices only the host moves */
boolean network_coop_devices_remote(void);
/* devices.c: a group's devices were set straight to its value */
void network_coop_note_device_snap(short group_index);
/* unit_scripting_commands.c: a script set a unit's maximum or current
vitality; TRUE if it was a co-op player's, now set on every player's */
boolean network_coop_set_players_vitality(long unit_index, boolean maximum, real body, real shield);
/* host: whether the player's machine has the host's structure BSP loaded
(always outside co-op, and for the host's own players). Until a client has,
the host takes none of its players' movement, which falls there with no
floor, and lets them trigger no BSP switch (players.c). */
boolean network_coop_player_has_structure_bsp(long player_index);
/* network_distributed.c: the structure BSP a client's input says it has */
void network_coop_note_player_structure_bsp(short player_index, short structure_bsp_index);
/* units.c: a vehicle left the vehicle carrying it (a Pelican's Warthog drop) */
void network_coop_vehicle_dropped(long vehicle_index, long carrier_index);

/* cinematics.c: a chapter title */
void network_coop_note_title(short title_index, real delay);
/* hud_messaging.c, hud.c: value is a message index, or TRUE/FALSE */
void network_coop_note_hud(short kind, short value);
/* player_effects.c: the script screen shake (a, b, c as the function takes them) */
void network_coop_note_player_effect(short kind, real a, real b, real c);
/* hud_nav_points.c: activate (or, with nav_index NONE, deactivate) a nav point.
target is the team or the unit; marker is the cutscene flag or the object. */
void network_coop_note_nav_point(short kind, short nav_index, long target, long marker, real vertical_offset);

/* units.c: a unit started a custom animation (animation_index NONE: stopped) */
void network_coop_note_unit_animation(long unit_index, long animation_graph_index, short animation_index,
	boolean interpolate);
/* units.c: a unit (a dropship) opened or closed (unit_open, unit_close) */
void network_coop_note_unit_open(long unit_index, boolean open);
/* units.c: unit_custom_animation_at_frame moved it to a frame */
void network_coop_note_unit_animation_frame(long unit_index, short frame_index);
/* scenery.c: a scenery animation started */
/* the scripts' effects (hs_library_external.c): at a cutscene flag, or on an
object's marker */
void network_coop_note_effect(long effect_definition_index, short cutscene_flag_index);
/* the scripts' objects_attach and objects_detach (objects.c) */
void network_coop_note_attach(long parent_index, char const *parent_marker_name, long child_index,
	char const *child_marker_name);
void network_coop_note_detach(long parent_index, long child_index);
void network_coop_note_object_effect(long effect_definition_index, long object_index, char const *marker_name);
/* breakable_surfaces.c: a breakable surface (glass) broke, from damage at
`epicenter`. Each machine's own shots break glass too, but with their own
random damage, so the host's breaks are the ones everyone keeps. */
void network_coop_note_surface_broken(short breakable_surface_index, real_point3d const *epicenter);
void network_coop_note_scenery_animation(long object_index, long animation_graph_index, short animation_index,
	short frame_index);

/* player_control.c: whether the skip key should do anything, and the press.
network_coop_vote_skip returns FALSE outside network co-op, where the
cinematic is skipped as usual. */
boolean network_coop_skip_offered(void);
boolean network_coop_vote_skip(void);
/* main.c: after a co-op host reverts to its last saved state (to skip a
cinematic, or because the mission failed); now is the game time before the
revert */
void network_coop_reverted(long now);
/* main.c: the skip vote is over once the skip has reverted */
void network_coop_skip_done(void);
/* the vote count to show, if a skippable cinematic is playing */
boolean network_coop_skip_vote_status(short *votes, short *voters, boolean *voted);

#endif
