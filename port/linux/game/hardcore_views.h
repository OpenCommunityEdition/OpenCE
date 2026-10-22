#ifndef HARDCORE_VIEWS_H
#define HARDCORE_VIEWS_H

struct observer_result;

/* How many panes the one local player gets this frame: 1, or up to 4 when
teammate views are on. */
short hardcore_views_window_count(void);

/* Local players this screen draws. A hardcore team game draws only the
first local player's team. */
short hardcore_views_local_count(void);

/* The next local player this screen draws after previous, or NONE. */
short hardcore_views_next_local(short previous);

/* The camera for a teammate pane. A living teammate is first person.
A dead one uses the death camera. The local player's pane is this
machine's display.teammate_view_slot. */
boolean hardcore_views_camera(short window_index, struct observer_result *result);

/* A pane that shows a teammate, not the local player and not the console. */
boolean hardcore_views_is_teammate_window(short window_index);

/* The pane that is the local player while teammate views split the screen. */
boolean hardcore_views_is_local_window(short window_index);

/* Teammate views are splitting this screen, so messages use the
split-screen font and line spacing. */
boolean hardcore_views_split(void);

/* Spare local-player slot this teammate pane borrows so the HUD and the
first-person weapon will draw. A slot that already has a player is left
alone. Each pane keeps its own slot, so the weapon animation continues. */
short hardcore_views_hud_slot(short window_index);

/* Once a tick, advance the weapons those panes are holding. */
void hardcore_views_weapons_tick(void);

/* The pane being drawn, so that body is hidden the way a first-person view is. */
void hardcore_views_begin(short window_index);
void hardcore_views_end(void);
boolean hardcore_views_hides_unit(long object_index);

#endif
