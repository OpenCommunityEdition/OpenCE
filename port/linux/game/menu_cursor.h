/*
MENU_CURSOR.H

The mouse pointer the menus draw (input.menu_cursor), as Halo Custom Edition's
menus do: an arrow that follows the mouse over the menu, in place of the
system's cursor, which stays hidden over the window while the menus are up.
Hovering, clicking (A), the right click (B) and the wheel are the menus' own
(ui_widget.c's pointer code); this only shows where the pointer is. A
controller used last takes the arrow away, the mouse moving brings it back
(input.prompts' tracker: platform_prompts_keyboard).
*/

#ifndef MENU_CURSOR_H
#define MENU_CURSOR_H

#include "halo_ui_pointer.h"

/* each frame the menus read the pointer (ui_widget.c): where it is, in the
menus' coordinates, and whether it was read (menus up); a touchscreen's is
never drawn */
void menu_cursor_note(int active, const struct halo_ui_pointer *pointer);

/* the arrow, drawn last over the menus (menus' coordinates, the centring on) */
void menu_cursor_draw(void);

/* whether the arrow stands in for the system's cursor now (a menu is up with
the mouse, and the setting is on), so that the platforms hide theirs over
the window; true as well while a controller has taken the arrow away */
int menu_cursor_replaces_system(void);

#endif
