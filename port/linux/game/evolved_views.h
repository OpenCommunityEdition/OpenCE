#ifndef EVOLVED_VIEWS_H
#define EVOLVED_VIEWS_H

struct observer_result;

/* How many panes the one local player gets this frame: 1, or up to 4 when
teammate views are on. */
short evolved_views_window_count(void);

/* The camera for a teammate pane. The local player's pane is whichever
region display.teammate_view_slot names. */
boolean evolved_views_camera(short window_index, struct observer_result *result);

/* A pane that shows a teammate, not the local player and not the console. */
boolean evolved_views_is_teammate_window(short window_index);

/* The pane that is the local player while teammate views split the screen. */
boolean evolved_views_is_local_window(short window_index);

/* Spare local-player slot a teammate pane borrows so the HUD and the
first-person weapon will draw. The one local player keeps slot 0. */
short evolved_views_hud_slot(void);

/* The pane being drawn, so that body is hidden the way a first-person view is. */
void evolved_views_begin(short window_index);
void evolved_views_end(void);
boolean evolved_views_hides_unit(long object_index);

#endif
