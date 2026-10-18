/*
CHANGELOG_MENU.C

The generated main menu keeps its original positions. Enabling the
changelog makes room for one more row; disabling it removes only its own
entry. Custom layouts keep their positions. The parsed menu owns these
changes for its lifetime, with no state carried between game maps.
*/

#include "changelog_menu.h"
#include "halo_menus.h"

#include <string.h>

void changelog_menu_configure(struct halo_menus *menus, int enabled)
{
	static char const *const names[] =
	{
		"main_menu/main_menu_item_load_camp",
		"main_menu/main_menu_item_multiplayer",
		"main_menu/main_menu_item_profile",
		"main_menu/main_menu_item_settings",
		"main_menu/main_menu_item_mods",
		"main_menu/main_menu_item_changelog",
		"main_menu/main_menu_item_quit_game"
	};
	long widget;

	for (widget = 0; widget < menus->widget_count; widget++)
	{
		struct halo_menu_widget *list = &menus->widgets[widget];
		long child, row, count = 0;
		long indices[7];
		long *link;
		int stock = 1;

		if (!list->name || strcmp(list->name, "main_menu/main_menu_select_list"))
			continue;
		/* Only the known generated layout needs automatic spacing. A menu
		override may have arranged its own rows or omitted the entry. */
		for (child = list->first_child; child != HALO_MENU_NONE; child = menus->children[child].next)
		{
			struct halo_menu_child const *entry;

			if (child < 0 || child >= menus->child_count || count >= 7)
			{
				stock = 0;
				break;
			}
			entry = &menus->children[child];
			indices[count] = child;
			if (entry->nested != HALO_MENU_NONE || !entry->widget || strcmp(entry->widget, names[count]) ||
				entry->x != (count == 0 ? 194 : 192) || entry->y != (count < 6 ? 247 + count * 36 : 427))
				stock = 0;
			count++;
		}
		if (enabled)
		{
			if (stock && count == 7)
			{
				for (row = 0; row < 6; row++)
					menus->children[indices[row]].y -= 36;
			}
			continue;
		}
		/* Unlink without freeing: children share the parser's allocation. */
		link = &list->first_child;
		for (count = 0; *link != HALO_MENU_NONE && count < menus->child_count; count++)
		{
			struct halo_menu_child *entry;

			if (*link < 0 || *link >= menus->child_count)
				break;
			entry = &menus->children[*link];
			if (entry->nested == HALO_MENU_NONE && entry->widget && !strcmp(entry->widget, names[5]))
				*link = entry->next;
			else
				link = &entry->next;
		}
	}
}
