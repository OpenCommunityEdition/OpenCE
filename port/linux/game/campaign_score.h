/*
CAMPAIGN_SCORE.H

The campaign's score (display.campaign_score: campaign_score.c): each
enemy a player kills is worth points by its actor variant.
*/

#ifndef __CAMPAIGN_SCORE_H
#define __CAMPAIGN_SCORE_H

/* display.campaign_score, and a campaign level being played */
boolean campaign_score_enabled(void);

/* (interface.c's hud_initialize_for_new_map) a map loads: a new level's
score begins; a level just won has its summary shown over this one's start */
void campaign_score_new_map(void);

/* (damage.c's object_damage_aftermath) a unit's body was depleted: if it
was an enemy, `killing_player_index` (NONE for no player) is given the
points of its actor variant */
void campaign_score_record_kill(
	long unit_index,
	long killing_player_index);

/* (main.c's main_won_map) the level is won: its summary is written to the
log and kept for the next level's start */
void campaign_score_level_won(void);

/* (interface.c's interface_draw_fullscreen_overlays) the score, above the
match clock at the bottom right, and the last level's summary */
void campaign_score_draw(void);

#endif
