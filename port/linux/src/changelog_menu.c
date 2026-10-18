/*
CHANGELOG_MENU.C

The generated main menu keeps its original positions, with the optional
changelog in the lower right corner. Disabling it removes only its own
entry. Custom layouts keep their positions. The parsed menu owns these
changes for its lifetime, with no state carried between game maps.
*/

#include "changelog_menu.h"
#include "halo_menus.h"

#include <string.h>

void changelog_menu_configure(struct halo_menus *menus, int enabled)
{
	long widget;

	if (enabled)
		return;
	for (widget = 0; widget < menus->widget_count; widget++)
	{
		struct halo_menu_widget *list = &menus->widgets[widget];
		long count;
		long *link;

		if (!list->name || strcmp(list->name, "main_menu/main_menu"))
			continue;
		/* Unlink without freeing: children share the parser's allocation. */
		link = &list->first_child;
		for (count = 0; *link != HALO_MENU_NONE && count < menus->child_count; count++)
		{
			struct halo_menu_child *entry;

			if (*link < 0 || *link >= menus->child_count)
				break;
			entry = &menus->children[*link];
			if (entry->nested == HALO_MENU_NONE && entry->widget &&
				!strcmp(entry->widget, "main_menu/changelog_corner"))
				*link = entry->next;
			else
				link = &entry->next;
		}
	}
}
