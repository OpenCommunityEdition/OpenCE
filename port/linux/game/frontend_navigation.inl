/* Landing screens keep their navigation in widget instances and use the
 * existing browser, host and co-op callbacks through generated XML. */
static boolean frontend_navigation_root(struct widget_instance *root)
{
	char const *name = root ? tag_get_name(root->definition_tag_index) : NULL;
	return name && (!strcmp(name, "pc\\modern\\multiplayer\\screen") || !strcmp(name, "pc\\modern\\host\\screen"));
}

static boolean frontend_navigation_content(struct widget_instance *widget)
{
	return !strncmp(widget->name, "row_", 4) || !strncmp(widget->name, "section_", 8) || !strncmp(widget->name, "rule_", 5);
}

static boolean frontend_navigation_drawable(struct widget_instance *widget)
{
	short height = !strncmp(widget->name, "rule_", 5) ? 1 : 26;
	return !frontend_navigation_content(widget) ||
		(widget->vertical_offset >= 82 && widget->vertical_offset + height <= 382);
}

static boolean frontend_navigation_event(struct widget_instance *widget, char const *name)
{
	struct widget_instance *root = screen_of(widget), *list, *row;
	if (!frontend_menu_can_edit() || !frontend_navigation_root(root)) return FALSE;
	if (strcmp(name, "port frontend navigation init")) return FALSE;
	list = named(root, "options", 0);
	for (row = list ? list->child : NULL; row; row = row->next)
		if (!strncmp(row->name, "row_", 4) && row->visible && !row->disabled)
		{
			list->focused_child = row;
			break;
		}
	return TRUE;
}

static void frontend_navigation_update(struct widget_instance *list)
{
	struct widget_instance *root = screen_of(list), *row, *focus, *title;
	short shift = 0, index;
	boolean overflow = FALSE;
	if (!frontend_menu_can_edit() || !frontend_navigation_root(root) || strcmp(list->name, "options")) return;
	focus = list->focused_child;
	if (focus && !strncmp(focus->name, "row_", 4))
	{
		if (focus->vertical_offset < 110) shift = 110 - focus->vertical_offset;
		else if (focus->vertical_offset > 356) shift = 356 - focus->vertical_offset;
	}
	/* Move section labels and rules with their rows. Never hide off-screen
	 * instances: the existing keyboard, pad and pointer navigation needs them. */
	for (row = list->child; row; row = row->next)
		if (frontend_navigation_content(row))
		{
			row->vertical_offset += shift;
			if (!strncmp(row->name, "row_", 4) && !frontend_navigation_drawable(row)) overflow = TRUE;
		}
	text_set(named(root, "status", 0), overflow ? L"UP / DOWN TO SCROLL" : L"");
	settings_help(list);
	index = focus && !strncmp(focus->name, "row_", 4) && list->parameters.list.extended_description ?
		list->parameters.list.extended_description->parameters.text_box.string_list_index - 1 : NONE;
	if (index == NONE && list->parameters.list.extended_description)
		list->parameters.list.extended_description->parameters.text_box.string_list_index = 0;
	title = named(root, "help_title", 0);
	if (title)
	{
		title->visible = index >= 0;
		if (index >= 0) title->parameters.text_box.string_list_index = index;
	}
}
