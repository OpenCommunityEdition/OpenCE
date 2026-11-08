/* A clip is one campaign level or one multiplayer game, from the tick the
level starts until it ends or the player leaves. It is listed under each
local profile that sat in that run. Watching one does not record another.
The tape is player actions and a picture of the local controls. It does
not store an address or a hardware id, and it adds nothing on the wire.

The list screen is pc\\main_menu\\clips\\clips_screen. A menu opens it by
clips_menu_set_profile on the highlighted profile, then the event
"port clips open". Where that control sits is the menu's own choice. */

#ifndef CLIPS_H
#define CLIPS_H

struct player_action;
union rectangle2d;

/* a clip waiting to play fills the map, difficulty and seed, and sets the
multiplayer variant. returns 0 when nothing is waiting */
int clips_fill_options(char *map_name, long map_name_size, short *difficulty, unsigned long *random_seed);
void clips_map_started(void);
void clips_map_ended(void);
int clips_replaying(void);
/* the fullscreen free camera. the player panes are hidden while it is on */
int clips_flying(void);
/* the level is running forward to the tick a backward scrub will land on.
the picture stays where it was until that tick is ready. */
int clips_catching_up(void);
/* a line for the free camera's text stream: pickups, deaths, and the rest */
void clips_feed_note(wchar_t const *text);
/* the same stream, with this player's name in front of an item message */
void clips_feed_item(long player_index, long definition_index, short quantity, int message_offset);
/* -1 when this frame is not limited. otherwise how many ticks it may run.
*exact is 1 when the clip wants that many, not merely no more than that. */
long clips_tick_allowance(int *exact);
/* 1 when this tick was recorded while the match clock waited, so playback
keeps the clock on that same tick. */
int clips_clock_held(void);
void clips_on_actions(struct player_action *actions, int capacity);
void clips_frame(void);
void clips_after_director(void);
void clips_draw(short local_player_index, union rectangle2d const *window);
int clips_return_pending(void);
int clips_take_return(void);

/* a text widget shows these words instead of a string from its list */
#define CLIPS_OWN_TEXT ((short)-2)

void clips_menu_set_profile(long profile_index);
long clips_menu_profile(void);
int clips_menu_take_restore(long *profile_index);
void clips_menu_load(void);
int clips_menu_count(void);
void clips_menu_scroll(int delta);
void clips_menu_note_focus(int visible_row);
/* the clip row that stays highlighted, or -1 when the list is empty */
int clips_menu_focused_row(void);
void clips_menu_line(int visible_row, wchar_t *text, int length);
int clips_menu_play(int visible_row, char *map_name, long map_name_size, short *difficulty);
int clips_menu_delete(int visible_row);
/* how many clips this profile keeps. the newest stay, and older ones past
that number are removed */
int clips_menu_keep(void);
void clips_menu_keep_adjust(int delta);

#endif
