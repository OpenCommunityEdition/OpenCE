/* The changelog's optional entry in the generated PC main menu. */
#ifndef CHANGELOG_MENU_H
#define CHANGELOG_MENU_H

struct halo_menus;

/* Called once after reading the menus. Their storage owns all changes. */
void changelog_menu_configure(struct halo_menus *menus, int enabled);

#endif
