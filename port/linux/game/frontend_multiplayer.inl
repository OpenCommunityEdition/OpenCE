/* Presentation only. Native callbacks run first and own the session, map
 * families, list contents, paging, joins and host authority. */
static boolean frontend_multiplayer_root(struct widget_instance *root)
{
	char const *name = root ? tag_get_name(root->definition_tag_index) : NULL;
	return frontend_menu_available() && main_menu_is_active() && name &&
		!strncmp(name, "pc\\modern\\network\\", 18);
}

static boolean frontend_multiplayer_row(struct widget_instance *row)
{
	if (!row->parent || row->parent->type != 3) return FALSE;
	return (!strncmp(row->name, "list_item_", 10) && row->name[10] >= '0' && row->name[10] <= '9') ||
		(!strncmp(row->name, "op_", 3)) || strstr(row->name, "_difficulty_item") != NULL;
}

static boolean frontend_multiplayer_drawable(struct widget_instance *widget)
{
	short top = 110;
	return !frontend_multiplayer_row(widget) ||
		(widget->vertical_offset >= top && widget->vertical_offset <= 350);
}

/* The service owns the selected map and difficulty; this changes only the
 * Modern widgets used by its embedded difficulty step. */
static void frontend_multiplayer_difficulty(struct widget_instance *list, short difficulty)
{
	struct widget_instance *root = screen_of(list), *description, *icon, *text, *row, *label;
	char path[256];
	char const *name;
	long replacement;
	size_t length;
	if (!frontend_multiplayer_root(root)) return;
	description = list->parameters.list.extended_description;
	icon = named(description, "difficulty_options", 0);
	text = named(description, "difficulty_extended_description", 0);
	if (!icon || !text) return;
	icon->visible = text->visible = difficulty >= 0 && difficulty < 4;
	if (icon->visible)
	{
		visible_set(named(description, "mp_map_right_item", 0), FALSE);
		icon->animation.current_frame_index = difficulty;
		text->parameters.text_box.string_list_index = difficulty;
	}
	for (row = list->child; row; row = row->next)
	{
		label = named(row, "list_item_text", 0);
		name = label ? tag_get_name(label->definition_tag_index) : NULL;
		if (!name) continue;
		length = strlen(name);
		if (length >= sizeof(path) - 10) continue;
		strcpy(path, name);
		if (length >= 9 && !strcmp(path + length - 9, "_centered")) path[length - 9] = 0;
		if (icon->visible) strcat(path, "_centered");
		replacement = tag_loaded('DeLa', path);
		if (replacement != NONE) label->definition_tag_index = replacement;
	}
}

static void frontend_multiplayer_update(struct widget_instance *list)
{
	struct widget_instance *root = screen_of(list), *row, *first_row = NULL;
	short count = 0, focus = NONE, first = 0, index = 0;
	short top = 110;
	short capacity = 9;
	wchar_t status[64];
	if (!frontend_multiplayer_root(root)) return;
	for (row = list->child; row; row = row->next)
		if (frontend_multiplayer_row(row) && row->visible)
		{
			if (!first_row) first_row = row;
			if (row == list->focused_child) focus = count;
			count++;
		}
	/* Off-screen rows remain navigable; only rendering and pointer targets
	 * are clipped. Derive the viewport from live instances, never a global. */
	if (first_row) first = (short)MAX(0, (top - first_row->vertical_offset) / 30);
	if (focus != NONE && focus < first) first = focus;
	if (focus >= first + capacity) first = (short)(focus - capacity + 1);
	first = (short)PIN(first, 0, MAX(0, count - capacity));
	for (row = list->child; row; row = row->next)
		if (frontend_multiplayer_row(row) && row->visible)
			row->vertical_offset = (short)(top + 30 * (index++ - first));
	status[0] = 0;
	if (count > capacity)
		usnprintf(status, NUMBEROF(status) - 1, L"%d-%d / %d   UP / DOWN TO SCROLL", first + 1, MIN(count, first + capacity), count);
	text_set(named(root, "status", 0), status);
	/* The native service's instruction exceeds the compact value column.
	 * Keep the action explicit without truncating an actual invite/address. */
	row = named(list, "invite_value", 0);
	if (row && row->parameters.text_box.text &&
		!wcscmp(row->parameters.text_box.text, L"SELECT TO COPY LINK"))
		text_set(row, L"COPY LINK");
	/* Text fields can be longer than their inline value column. Keep the
	 * full selected value readable above its explanation in Server Setup. */
	if (named(root, "help_value", 0))
	{
		wchar_t const *value = L"";
		for (row = list->focused_child ? list->focused_child->child : NULL; row; row = row->next)
			if (row->type == 1 && strstr(row->name, "_value") && row->parameters.text_box.text)
				value = row->parameters.text_box.text;
		text_set(named(root, "help_value", 0), value);
	}
}
