/* The changelog's eight visible text rows read paired immutable body and
 * category-heading string lists; exactly one contains text at each index.
 * The container's otherwise unused list fields keep its own line offset;
 * deleting or opening a build discards that state with the instance. */

#include "tag_files/tag_files.h"

#define CHANGELOG_VISIBLE_LINES 8

static boolean changelog_menu_main_focus(struct widget_instance *widget, boolean corner)
{
	struct widget_instance *main_list, *corner_list, *button, *child;
	char const *name;
	short index;

	while (widget && widget->parent)
		widget = widget->parent;
	name = widget ? tag_get_name(widget->definition_tag_index) : NULL;
	if (!name || strcmp(name, "pc\\main_menu\\main_menu"))
		return FALSE;
	main_list = named(widget, "main_menu_select_list", 0);
	corner_list = named(widget, "changelog_corner", 0);
	button = named(corner_list, "main_menu_item_changelog", 0);
	if (!main_list || main_list->type != 3 || !corner_list || corner_list->type != 3 || !button)
		return FALSE;
	if (corner)
	{
		main_list->focused_child = NULL;
		corner_list->focused_child = button;
		widget->focused_child = corner_list;
	}
	else
	{
		/* Mouse focus clears the old chain, but retains the list's selected
		index. Restore that child without changing the six-row tab behavior. */
		if (!main_list->focused_child)
		{
			child = main_list->child;
			for (index = 0; child && child->next && index < main_list->parameters.list.selected_index; index++)
				child = child->next;
			main_list->focused_child = child;
		}
		if (main_list->focused_child && main_list->focused_child->animation.number_of_sprite_frames == 2)
			main_list->focused_child->animation.current_frame_index = 1;
		widget->focused_child = main_list;
	}
	return TRUE;
}

static void changelog_menu_focus_update(struct widget_instance *widget)
{
	static char const prefix[] = "pc\\main_menu\\changelog\\build_";
	struct widget_instance *corner, *button, *main_list, *child;
	char const *name = widget ? tag_get_name(widget->definition_tag_index) : NULL;
	short index;

	if (!name)
		return;
	if (widget->type == 0 && !widget->parent && !strncmp(name, prefix, sizeof(prefix) - 1) &&
		name[sizeof(prefix) - 1] && strspn(name + sizeof(prefix) - 1, "0123456789") == strlen(name + sizeof(prefix) - 1))
	{
		short list_index;

		/* The engine updates only the focused list. Synchronize our two
		button groups so the inactive group cannot retain a white outline. */
		for (list_index = 0; list_index < 2; list_index++)
		{
			main_list = named(widget, list_index ? "buttons" : "scroll_controls", 0);
			if (!main_list || main_list->type != 3)
				continue;
			for (child = main_list->child, index = 0; child && index < 3; child = child->next, index++)
				if (child->animation.number_of_sprite_frames == 2)
					child->animation.current_frame_index = !child->disabled &&
						widget->focused_child == main_list && main_list->focused_child == child;
		}
		return;
	}
	if (strcmp(name, "pc\\main_menu\\main_menu"))
		return;
	corner = named(widget, "changelog_corner", 0);
	button = named(corner, "main_menu_item_changelog", 0);
	if (corner && button)
	{
		button->animation.current_frame_index = widget->focused_child == corner && corner->focused_child == button;
		main_list = named(widget, "main_menu_select_list", 0);
		/* Mouse focus bypasses the Right handler. The inactive list is not
		updated by the engine, so clear its old glow before it is rendered. */
		if (widget->focused_child == corner && main_list)
			for (child = main_list->child, index = 0; child && index < 6; child = child->next, index++)
				if (child->animation.number_of_sprite_frames == 2)
					child->animation.current_frame_index = 0;
	}
}

static boolean changelog_menu_event(struct widget_instance *widget, char const *event)
{
	static char const prefix[] = "pc\\main_menu\\changelog\\build_";
	struct widget_instance *root = widget;
	struct string_list *strings, *headings;
	char const *name;
	char list_name[256], row_name[32], position[64];
	wchar_t wide_position[64];
	long list_tag, heading_tag, count, first, maximum, row;
	boolean initialize = !strcmp(event, "port changelog init");
	long direction = !strcmp(event, "port changelog down") ? 1 : -1;

	if (!strcmp(event, "port changelog corner focus") || !strcmp(event, "port changelog main focus"))
		return changelog_menu_main_focus(widget, !strcmp(event, "port changelog corner focus"));
	if (!initialize && strcmp(event, "port changelog up") && strcmp(event, "port changelog down"))
		return FALSE;
	while (root && root->parent)
		root = root->parent;
	if (!root || root->type != 0)
		return FALSE;
	name = tag_get_name(root->definition_tag_index);
	if (!name || strncmp(name, prefix, sizeof(prefix) - 1))
		return FALSE;
	if (snprintf(list_name, sizeof(list_name), "%s\\lines", name) >= (long)sizeof(list_name))
		return FALSE;
	list_tag = tag_loaded(UNICODE_STRING_LIST_TAG, list_name);
	if (list_tag == NONE)
		return FALSE;
	strings = unicode_string_list_definition_get(list_tag);
	if (!strings || strings->strings.count <= CHANGELOG_VISIBLE_LINES || strings->strings.count > 32767)
		return FALSE;
	if (snprintf(list_name, sizeof(list_name), "%s\\headings", name) >= (long)sizeof(list_name))
		return FALSE;
	heading_tag = tag_loaded(UNICODE_STRING_LIST_TAG, list_name);
	if (heading_tag == NONE)
		return FALSE;
	headings = unicode_string_list_definition_get(heading_tag);
	if (!headings || headings->strings.count != strings->strings.count)
		return FALSE;
	/* The trailing empty strings let a short document fill the viewport. */
	count = strings->strings.count - CHANGELOG_VISIBLE_LINES;
	maximum = MAX(0, count - CHANGELOG_VISIBLE_LINES);
	first = initialize ? 0 : PIN((long)root->parameters.list.selected_index + direction, 0, maximum);
	root->parameters.list.selected_index = (short)first;
	for (row = 0; row < CHANGELOG_VISIBLE_LINES; row++)
	{
		struct widget_instance *text;

		snprintf(row_name, sizeof(row_name), "row_%ld", row);
		text = named(root, row_name, 0);
		if (text && text->type == 1)
			text->parameters.text_box.string_list_index = (short)(first + row);
		snprintf(row_name, sizeof(row_name), "heading_row_%ld", row);
		text = named(root, row_name, 0);
		if (text && text->type == 1)
			text->parameters.text_box.string_list_index = (short)(first + row);
	}
	snprintf(position, sizeof(position), "Lines %ld-%ld of %ld", first + 1,
		MIN(first + CHANGELOG_VISIBLE_LINES, count), count);
	halo_menus_utf16(position, (unsigned short *)wide_position, NUMBEROF(wide_position));
	text_set(named(root, "scroll_position", 0), wide_position);
	/* At either end, the buttons stay visible but cannot take focus/clicks. */
	widget = named(root, "scroll_up", 0);
	if (widget)
		widget->disabled = first == 0;
	widget = named(root, "scroll_down", 0);
	if (widget)
		widget->disabled = first == maximum;
	return TRUE;
}
