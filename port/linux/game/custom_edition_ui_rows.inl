/* port: rows added to a map's own list at run time (ui_widget_port_inject_row). Old and staged pages coexist during replacement. A row's
label and description are text of the port's own, drawn through string list indices from PORT_ROW_TEXT_BASE
up (two each: the label's, then the description's); its spinner may have values of the port's own too */
#define PORT_ROW_TEXT_BASE 0x7100
#define PORT_MAXIMUM_ROWS 32 /* port: old and staged CE pages coexist until commit */

static struct
{
	struct widget_instance *row;
	struct widget_instance *spinner;
	wchar_t const *label;
	wchar_t const *description;
	wchar_t const *const *values;
	short value_count;
} ui_widget_port_rows[PORT_MAXIMUM_ROWS];

wchar_t const *ui_widget_port_row_text(short string_list_index)
{
	short slot = (short)((string_list_index - PORT_ROW_TEXT_BASE) / 2);

	if (string_list_index < PORT_ROW_TEXT_BASE || slot >= PORT_MAXIMUM_ROWS || !ui_widget_port_rows[slot].row)
		return NULL;
	return ((string_list_index - PORT_ROW_TEXT_BASE) % 2) ? ui_widget_port_rows[slot].description :
		ui_widget_port_rows[slot].label;
}

short ui_widget_port_row_description(struct widget_instance *row)
{
	short slot;

	for (slot = 0; slot < PORT_MAXIMUM_ROWS; slot++)
	{
		if (ui_widget_port_rows[slot].row == row)
			return (short)(PORT_ROW_TEXT_BASE + 2 * slot + 1);
	}
	return NONE;
}

wchar_t const *ui_widget_port_spinner_string(struct widget_instance *spinner, short index)
{
	short slot;

	for (slot = 0; slot < PORT_MAXIMUM_ROWS; slot++)
	{
		if (ui_widget_port_rows[slot].row && ui_widget_port_rows[slot].spinner == spinner &&
			ui_widget_port_rows[slot].values && index >= 0 && index < ui_widget_port_rows[slot].value_count)
		{
			return ui_widget_port_rows[slot].values[index];
		}
	}
	return NULL;
}

static void ui_widget_port_row_forget(struct widget_instance *widget)
{
	short slot;

	for (slot = 0; slot < PORT_MAXIMUM_ROWS; slot++)
	{
		if (ui_widget_port_rows[slot].row == widget)
			memset(&ui_widget_port_rows[slot], 0, sizeof(ui_widget_port_rows[slot]));
	}
}

/* a copy of the row `source` (a child of a list), put in the list before `before` (NULL: last) at the
vertical offset given, its label and description the text given; values: the names of its spinner's values
(NULL: the source's own), value_count of them. NULL if it could not be made. */
struct widget_instance *ui_widget_port_inject_row(
	struct widget_instance *source,
	struct widget_instance *before,
	short vertical_offset,
	wchar_t const *label,
	wchar_t const *description,
	wchar_t const *const *values,
	short value_count)
{
	struct widget_instance *row;
	struct widget_instance *child;
	struct widget_instance *label_box = NULL;
	short slot;

	for (slot = 0; slot < PORT_MAXIMUM_ROWS && ui_widget_port_rows[slot].row; slot++)
		;
	if (slot == PORT_MAXIMUM_ROWS || !custom_edition_menu_widget(source) || !source->parent)
		return NULL;
	row = ui_widget_load_by_name_or_tag(NULL, source->definition_tag_index, source->parent,
		source->local_player_index, NONE, NONE, NONE);
	if (!row)
		return NULL;
	row->horizontal_offset = source->horizontal_offset;
	row->vertical_offset = vertical_offset;
	if (before && before->parent == source->parent)
	{
		row->previous = before->previous;
		row->next = before;
		if (before->previous)
			before->previous->next = row;
		else
			source->parent->child = row;
		before->previous = row;
	}
	else
		ui_widget_add_child(source->parent, row);
	ui_widget_port_rows[slot].row = row;
	ui_widget_port_rows[slot].spinner = NULL;
	ui_widget_port_rows[slot].label = label;
	ui_widget_port_rows[slot].description = description;
	ui_widget_port_rows[slot].values = values;
	ui_widget_port_rows[slot].value_count = values ? value_count : 0;
	for (child = row->child; child; child = child->next)
	{
		if (child->type == _ui_widget_type_text_box && !label_box)
			label_box = child;
		else if (child->type == _ui_widget_type_spinner_list && !ui_widget_port_rows[slot].spinner)
			ui_widget_port_rows[slot].spinner = child;
	}
	if (!label_box || !ui_widget_port_rows[slot].spinner)
	{
		ui_widget_delete(row);
		return NULL;
	}
	if (label_box)
		label_box->parameters.text_box.string_list_index = (short)(PORT_ROW_TEXT_BASE + 2 * slot);
	if (values && ui_widget_port_rows[slot].spinner)
		ui_widget_port_rows[slot].spinner->parameters.list.number_of_items = (word)value_count;
	return row;
}


/* port: the top and bottom, on screen, of the visible widgets with a size under a widget (as
widget_instance_render_recursive places them: each widget's offset is added on the way down) */
static void ui_widget_port_description_extent(
	struct widget_instance *widget,
	long y,
	long *top,
	long *bottom)
{
	struct ui_widget_definition *definition = ui_widget_definition_get(widget->definition_tag_index);
	struct widget_instance *child;

	if (!widget->visible)
		return;
	y += widget->vertical_offset;
	if (!widget->child && definition->bounds.y1 > definition->bounds.y0)
	{
		long y0 = y + definition->bounds.y0 + definition->vertical_offset;
		long y1 = y + definition->bounds.y1 + definition->vertical_offset;

		if (y0 < *top)
			*top = y0;
		if (y1 > *bottom)
			*bottom = y1;
	}
	for (child = widget->child; child; child = child->next)
		ui_widget_port_description_extent(child, y, top, bottom);
}

/* port: a map's own list that has had rows injected into it (ui_widget_port_inject_row) is taller than its
screen was made for, so its side description (the extended description, which the map placed below the rows
it had) is drawn over the new rows. Each frame, while the list has injected rows, the description is moved
down to just below the lowest row if it would overlap them. Lists with none are left as the map made them. */
void ui_widget_port_description_clear(
	struct widget_instance *list)
{
	struct widget_instance *description = list->parameters.list.extended_description;
	struct widget_instance *row;
	struct widget_instance *ancestor;
	long base = 0;
	long rows_bottom = NONE;
	long top = 10000;
	long bottom = -10000;
	boolean injected = FALSE;
	short slot;

	if (!description)
		return;
	for (slot = 0; slot < PORT_MAXIMUM_ROWS; slot++)
	{
		if (ui_widget_port_rows[slot].row && ui_widget_port_rows[slot].row->parent == list)
			injected = TRUE;
	}
	if (!injected)
		return;
	for (ancestor = list; ancestor; ancestor = ancestor->parent)
		base += ancestor->vertical_offset;
	for (row = list->child; row; row = row->next)
	{
		if (row->visible && row->type == _ui_widget_type_container)
		{
			long row_bottom = base + row->vertical_offset +
				ui_widget_definition_get(row->definition_tag_index)->bounds.y1;

			if (row_bottom > rows_bottom)
				rows_bottom = row_bottom;
		}
	}
	description->vertical_offset = 0;
	ui_widget_port_description_extent(description, base, &top, &bottom);
	if (rows_bottom != NONE && top < rows_bottom + 8)
		description->vertical_offset = (short)(rows_bottom + 8 - top);
}

/* port: a copy of a row of a list, last in it (the Gamepads screen of a map's own pause menu) */
struct widget_instance *ui_widget_port_clone_row(struct widget_instance *source)
{
	struct widget_instance *row;

	if (!source->parent)
		return NULL;
	row = ui_widget_load_by_name_or_tag(NULL, source->definition_tag_index, source->parent,
		source->local_player_index, NONE, NONE, NONE);
	if (!row)
		return NULL;
	row->horizontal_offset = source->horizontal_offset;
	row->vertical_offset = source->vertical_offset;
	ui_widget_add_child(source->parent, row);
	return row;
}


/* port: a definition of the name below a screen's, through its children and what its handlers open */
static long ui_widget_port_find_definition(long tag_index, char const *name, long depth)
{
	struct ui_widget_definition *definition;
	long index;
	long found;

	if (tag_index == NONE || depth > 6)
		return NONE;
	definition = ui_widget_definition_get(tag_index);
	if (!strcmp(definition->name, name))
		return tag_index;
	for (index = 0; index < definition->child_widgets.count; index++)
	{
		struct ui_widget_child_reference *reference =
			(struct ui_widget_child_reference *)definition->child_widgets.address + index;

		found = ui_widget_port_find_definition(reference->widget_tag.index, name, depth + 1);
		if (found != NONE)
			return found;
	}
	for (index = 0; index < definition->event_handlers.count; index++)
	{
		struct ui_widget_event_handler_reference *handler =
			(struct ui_widget_event_handler_reference *)definition->event_handlers.address + index;

		found = ui_widget_port_find_definition(handler->widget_tag.index, name, depth + 1);
		if (found != NONE)
			return found;
	}
	return NONE;
}

/* port: the donor settings screen's spinner (found from the screen this one was opened from), or NONE */
long ui_widget_port_donor_spinner(struct widget_instance *screen)
{
	short stack = screen->local_player_index == NONE ? 0 : screen->local_player_index;
	struct widget_stack_node *node = widget_globals.widget_stack[stack];

	if (!node || node->data.previous_widget_tag == NONE)
		return NONE;
	return ui_widget_port_find_definition(node->data.previous_widget_tag, "specular_spinner", 0);
}

/* port: a spinner of the definition given, as a child of a row, its values the names given */
struct widget_instance *ui_widget_port_add_spinner(
	struct widget_instance *row,
	long spinner_tag,
	short x,
	short y,
	wchar_t const *const *values,
	short value_count)
{
	struct widget_instance *spinner;
	short slot;

	for (slot = 0; slot < PORT_MAXIMUM_ROWS && ui_widget_port_rows[slot].row; slot++)
		;
	if (slot == PORT_MAXIMUM_ROWS || spinner_tag == NONE)
		return NULL;
	spinner = ui_widget_load_by_name_or_tag(NULL, spinner_tag, row, row->local_player_index, NONE, NONE, NONE);
	if (!spinner)
		return NULL;
	spinner->horizontal_offset = x;
	spinner->vertical_offset = y;
	ui_widget_add_child(row, spinner);
	ui_widget_port_rows[slot].row = row;
	ui_widget_port_rows[slot].spinner = spinner;
	ui_widget_port_rows[slot].label = NULL;
	ui_widget_port_rows[slot].description = NULL;
	ui_widget_port_rows[slot].values = values;
	ui_widget_port_rows[slot].value_count = value_count;
	spinner->parameters.list.number_of_items = (word)value_count;
	return spinner;
}
