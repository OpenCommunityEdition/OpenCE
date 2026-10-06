/*
COOP_SPECTATE.H

Spectating in network co-op, and the cutscene skip vote's line
(coop_spectate.c).
*/

#ifndef __COOP_SPECTATE_H
#define __COOP_SPECTATE_H

struct dead_camera;

/* whether this is network co-op: a network game with no game engine (a campaign map) */
boolean coop_spectating(
	void);

/* the unit a dead local player watches (A switches teammate), or NONE if
the player is alive or nobody else is */
long coop_spectate_unit(
	short local_player_index);

/* TRUE if the teammate this local player is spectating (while dead or not
yet spawned) rides in an AI-driven vehicle, such as an insertion Pelican.
network_coop.c then shows that seat instead of the scripted camera. */
boolean coop_spectate_watching_rider(
	short local_player_index);

/* whether a local player has nothing to look at: no unit of its own and
no living teammate to watch (network_coop.c shows it the host's view) */
boolean coop_spectate_nothing_to_watch(
	short local_player_index);

/* keeps the dead camera behind the watched unit as it turns */
void coop_spectate_camera(
	struct dead_camera *camera);

/* the HUD line saying who a dead local player is watching */
void coop_spectate_draw(
	short local_player_index);

/* during a skippable cutscene in network co-op, the vote count and how to vote */
void coop_skip_vote_draw(
	short local_player_index);

#endif
