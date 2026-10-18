/* The changelog's eight visible text rows read paired immutable body and
 * category-heading string lists; exactly one contains text at each index.
 * The container's otherwise unused list fields keep its own line offset;
 * deleting or opening a build discards that state with the instance. */

#include "tag_files/tag_files.h"

#define CHANGELOG_VISIBLE_LINES 8

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
