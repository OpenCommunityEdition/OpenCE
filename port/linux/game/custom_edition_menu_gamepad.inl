/* ---------- Gamepads: Halo PC's device screen of a map's own pause menu (gamepad_setup_screen: created 129,
deleted 130, OK 131, a row 132, game data 47), made the port's: its settings, and its button remapping,
appearance and dead zones as pages of the one screen. Both columns are staged before replacing
the previous page, and drawn by pc_menu_gamepad_text (ui_widget.c). Values are kept here until OK. */

extern struct widget_instance *ui_widget_port_clone_row(struct widget_instance *source);
extern long ui_widget_port_donor_spinner(struct widget_instance *screen);
extern struct widget_instance *ui_widget_port_add_spinner(struct widget_instance *row, long spinner_tag,
	short x, short y, wchar_t const *const *values, short value_count);
static boolean profile_settings_save(struct widget_instance *widget);

#define GP_OPTIONS 18
#define GP_MAXIMUM_ROWS 16
#define GP_STYLE_FIRST 11
#define GP_BUMPER 15
#define GP_CATEGORY 100
#define GP_PLAYER 200
#define GP_GROUP 201
#define GP_ACTION 300

struct gp_option
{
	char const *key;
	wchar_t const *label;
	short count;
	wchar_t const *const *shown;
	char const *const *real;
	wchar_t const *help;
	short value;
	short loaded;
};

static wchar_t const *const gp_no_yes[2] = { L"NO", L"YES" };
static wchar_t const *const gp_off_on[2] = { L"OFF", L"ON" };
static char const *const gp_false_true[2] = { "false", "true" };
static wchar_t const *const gp_sensitivity_names[10] =
	{ L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"10" };
static char const *const gp_sensitivity_values[10] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
static wchar_t const *const gp_layout_names[8] = { L"DEFAULT", L"SWAP TRIGGERS", L"SWAP A, L TRIGGER",
	L"SWAP B, L TRIGGER", L"SWAP B, R STICK", L"BUMPER JUMPER", L"FISHSTICKS", L"RECON" };
static char const *const gp_layout_values[8] = { "0", "1", "2", "3", "4", "5", "6", "7" };
static wchar_t const *const gp_stick_names[4] = { L"DEFAULT", L"SOUTHPAW", L"LEGACY", L"LEGACY SOUTHPAW" };
static char const *const gp_stick_values[4] = { "0", "1", "2", "3" };
static wchar_t const *const gp_dead_names[12] = { L"DEFAULT", L"0%", L"5%", L"10%", L"15%", L"20%", L"25%",
	L"30%", L"35%", L"40%", L"45%", L"50%" };
static char const *const gp_dead_values[12] = { "-1", "0", "5", "10", "15", "20", "25", "30", "35", "40", "45",
	"50" };
static wchar_t const *const gp_manual_names[12] = { L"XBOX CLASSIC", L"XBOX GEMS", L"XBOX DARK GEMS",
	L"XBOX MODERN", L"PS CLASSIC", L"PS GEMS", L"PS DARK GEMS", L"PS MODERN", L"SWITCH CLASSIC",
	L"SWITCH GEMS", L"SWITCH MODERN", L"KEYBOARD" };
static char const *const gp_manual_values[12] = { "classic", "gems", "menu_gems", "xbox_modern", "ps_classic",
	"ps_gems", "ps_dark_gems", "ps_modern", "switch_classic", "switch_gems", "switch_modern", "keyboard" };
static wchar_t const *const gp_xbox_names[4] = { L"XBOX CLASSIC", L"XBOX GEMS", L"XBOX DARK GEMS",
	L"XBOX MODERN" };
static char const *const gp_xbox_values[4] = { "classic", "gems", "menu_gems", "xbox_modern" };
static wchar_t const *const gp_ps_names[4] = { L"PS CLASSIC", L"PS GEMS", L"PS DARK GEMS", L"PS MODERN" };
static char const *const gp_ps_values[4] = { "ps_classic", "ps_gems", "ps_dark_gems", "ps_modern" };
static wchar_t const *const gp_switch_names[3] = { L"SWITCH CLASSIC", L"SWITCH GEMS", L"SWITCH MODERN" };
static char const *const gp_switch_values[3] = { "switch_classic", "switch_gems", "switch_modern" };
static wchar_t const *const gp_bumper_names[2] = { L"ORIGINAL XBOX", L"MODERN XBOX" };
static char const *const gp_bumper_values[2] = { "original", "modern" };
static wchar_t const *const gp_group_names[5] = { L"MOVEMENT", L"COMBAT", L"GENERAL", L"COMMUNICATION",
	L"DEBUG" };
static wchar_t const *const gp_category_names[3] = { L"BUTTON REMAPPING", L"BUTTON APPEARANCE",
	L"STICK DEAD ZONES" };
static wchar_t const *const gp_category_help[3] =
{
	L"Custom button layouts for P1-P4.\r\nUse / Reload stays combined.",
	L"Keyboard/controller glyphs and PC-menu legends.\r\nGameplay controller art needs High-Res HUD.",
	L"How far each stick moves before it does\r\nanything."
};

static struct gp_option gp_options[GP_OPTIONS] =
{
	{ "profile.look_sensitivity", L"LOOK SENSITIVITY", 10, gp_sensitivity_names, gp_sensitivity_values,
		L"How fast the right stick turns the view.", 0, 0 },
	{ "profile.invert_look", L"INVERT LOOK", 2, gp_no_yes, gp_false_true,
		L"Pushing the right stick up looks down.", 0, 0 },
	{ "profile.flight_inversion", L"INVERT FLIGHT", 2, gp_no_yes, gp_false_true,
		L"Flying vehicles pitch as aircraft do:\r\npushing up flies down.", 0, 0 },
	{ "profile.autocenter", L"AUTO-CENTER LOOK", 2, gp_no_yes, gp_false_true,
		L"Level the view while walking.", 0, 0 },
	{ "profile.button_preset", L"BUTTON LAYOUT", 8, gp_layout_names, gp_layout_values,
		L"Which of the controller's buttons does what\r\n(not the keyboard's: Controls Setup sets those).", 0, 0 },
	{ "profile.joystick_preset", L"STICK LAYOUT", 4, gp_stick_names, gp_stick_values,
		L"Which stick moves and which looks.", 0, 0 },
	{ "profile.vibration", L"VIBRATION", 2, gp_off_on, gp_false_true, L"Rumble the controller.", 0, 0 },
	{ "profile.ingame_help", L"IN-GAME HELP", 2, gp_off_on, gp_false_true,
		L"Show the game's hints about its controls.", 0, 0 },
	{ "display.menu_button_glyphs", L"MENU GLYPHS", 2, gp_off_on, gp_false_true,
		L"Use the selected keyboard/controller style in\r\nexisting PC-menu face-button legends.", 0, 0 },
	{ "display.keyboard_glyphs", L"KEYBOARD GLYPHS", 2, gp_off_on, gp_false_true,
		L"Opt in to automatic keyboard/mouse keycaps.\r\nRequires Auto Button Detection.", 0, 0 },
	{ "input.auto_button_detection", L"AUTO BUTTON DETECTION", 2, gp_off_on, gp_false_true,
		L"Detect controller artwork and, with Keyboard\r\nGlyphs enabled, follow keyboard/mouse activity.", 0, 0 },
	{ "display.hud_button_style", L"BUTTON STYLE", 12, gp_manual_names, gp_manual_values,
		L"Manual style when auto detection is off.\r\nControllers need High-Res HUD.", 0, 0 },
	{ "display.hud_button_style_xbox", L"BUTTON STYLE", 4, gp_xbox_names, gp_xbox_values,
		L"Preferred Xbox artwork for automatic detection.\r\nRequires High-Res HUD in Graphics.", 0, 0 },
	{ "display.hud_button_style_ps", L"BUTTON STYLE", 4, gp_ps_names, gp_ps_values,
		L"Preferred PlayStation artwork for auto detection.\r\nRequires High-Res HUD in Graphics.", 0, 0 },
	{ "display.hud_button_style_switch", L"BUTTON STYLE", 3, gp_switch_names, gp_switch_values,
		L"Preferred Switch artwork for automatic detection.\r\nRequires High-Res HUD in Graphics.", 0, 0 },
	{ "display.hud_bumper_style", L"BUMPER STYLE", 2, gp_bumper_names, gp_bumper_values,
		L"Original: black/white buttons. Modern: LB/RB.\r\nKept when automatic detection is on.", 0, 0 },
	{ "input.left_stick_dead_zone", L"LEFT STICK", 12, gp_dead_names, gp_dead_values,
		L"How far the left stick moves before you move.\r\nDefault: the game's own, 27%.", 0, 0 },
	{ "input.right_stick_dead_zone", L"RIGHT STICK", 12, gp_dead_names, gp_dead_values,
		L"How far the right stick moves before the view\r\nturns. Default: the game's own, 27%.", 0, 0 },
};

static wchar_t const *const gp_player_names[4] = { L"P1", L"P2", L"P3", L"P4" };
static wchar_t const *const gp_short_labels[GP_OPTIONS] =
{
	L"SENSITIVITY", L"INVERT LOOK", L"INV. FLIGHT", L"AUTO-CENTER", L"LAYOUT", L"STICKS", L"VIBRATION",
	L"HINTS", L"MENU GLYPHS", L"KB GLYPHS", L"AUTO DETECT", L"STYLE", L"STYLE", L"STYLE", L"STYLE",
	L"BUMPERS", L"LEFT STICK", L"RIGHT STICK"
};

static struct
{
	boolean active;
	struct widget_instance *screen;
	/* the page shown, the page to build next frame and the row to focus then (NONE: none) */
	short page, pending, focus_col, focus_row;
	short base, pitch, help_y;
	short owner, port, group;
	long spinner_tag;
	/* a row of each column's list, copied to make the others from */
	struct widget_instance tpl[2];
	short count[2];
	short rows[2][GP_MAXIMUM_ROWS];
	short built_count[2];
	short shown[2][GP_MAXIMUM_ROWS];
	short built[2][GP_MAXIMUM_ROWS];
} gp;

static wchar_t gp_text[256];

/* ui_widget.c: how far the arrows of a Gamepads slider are pulled toward each other (less: wider slider) */
short pc_menu_gamepad_arrow_pull(struct widget_instance *spinner)
{
	struct widget_instance *row = spinner->parent;
	short column, index, code;

	if (!gp.active || !row || !row->parent || !row->parent->name)
		return 39;
	column = !strcmp(row->parent->name, "gamepad_used_list") ? 1 : 0;
	index = sibling_index(row);
	if (index < 0 || index >= gp.built_count[column])
		return 39;
	code = gp.built[column][index];
	if (code == GP_BUMPER)
		return 17;
	if (code == GP_GROUP)
		return 15;
	return (code >= GP_STYLE_FIRST && code < GP_BUMPER) ? 0 : 39;
}

/* where a row's slider starts: the style rows' and the bumpers' are wider, so they start further right */
static short gp_slider_x(short code)
{
	return code == GP_BUMPER ? 98 : code == GP_GROUP ? 97 : (code >= GP_STYLE_FIRST && code < GP_BUMPER) ? 81 : 120;
}

static short gp_option_page(short index)
{
	return index < 8 ? 0 : index < 16 ? 2 : 3;
}

static void gp_widen(char const *text, wchar_t *wide, short size)
{
	short index;

	for (index = 0; text[index] && index < size - 1; index++)
		wide[index] = (wchar_t)(unsigned char)text[index];
	wide[index] = 0;
}

static short gp_value_find(struct gp_option const *option, char const *text)
{
	short index;
	double first, second;

	for (index = 0; index < option->count; index++)
	{
		if (!strcmp(text, option->real[index]) ||
			(text_is_number(text, &first) && text_is_number(option->real[index], &second) && first == second))
		{
			return index;
		}
	}
	return 0;
}

static void gp_values_read(boolean defaults, short page)
{
	short index;

	for (index = 0; index < GP_OPTIONS; index++)
	{
		struct gp_option *option = &gp_options[index];
		char text[128];
		short found = 0;

		if (page != NONE && gp_option_page(index) != page)
			continue;
		if (setting_text(option->key, text, sizeof(text), defaults))
			found = gp_value_find(option, text);
		option->value = found;
		if (!defaults)
			option->loaded = found;
	}
}

static void gp_pad_init(void)
{
	short port;

	controller_remap_capture_end();
	pad_editor.capturing = pad_editor.proposed = NONE;
	for (port = 0; port < 4; port++)
	{
		controller_remap_profile(port, pad_editor.buttons[port]);
		pad_editor.custom[port] = (boolean)controller_remap_read(port, pad_editor.buttons[port]);
	}
}

static short gp_style_option(void)
{
	struct controller_device device;
	short family = 0;

	if (gp_options[10].value == 1)
	{
		controller_device_get(gp.owner, &device);
		family = (short)device.family;
		if (family < 0 || family > 3)
			family = 0;
	}
	return (short)(GP_STYLE_FIRST + family);
}

static boolean gp_bumper_visible(void)
{
	struct gp_option const *style = &gp_options[gp_style_option()];
	char const *value = style->real[style->value];

	return strcmp(value, "keyboard") && controller_button_style_index(value) < 3;
}

/* a row's slider: its value names, how many, and the value (FALSE: not a slider) */
static boolean gp_slider(short code, wchar_t const *const **values, short *count, short **value)
{
	if (code < GP_CATEGORY)
	{
		struct gp_option *option = &gp_options[code];

		*values = option->shown;
		*count = option->count;
		*value = &option->value;
		return TRUE;
	}
	if (code == GP_PLAYER)
	{
		*values = gp_player_names;
		*count = 4;
		*value = &gp.port;
		return TRUE;
	}
	if (code == GP_GROUP)
	{
		*values = gp_group_names;
		*count = 5;
		*value = &gp.group;
		return TRUE;
	}
	return FALSE;
}

static void gp_add(short column, short code)
{
	if (gp.count[column] < GP_MAXIMUM_ROWS)
		gp.rows[column][gp.count[column]++] = code;
}

static void gp_rows_build(void)
{
	static short const general[6] = { 0, 1, 2, 3, 6, 7 };
	short index;

	gp.count[0] = gp.count[1] = 0;
	if (gp.page == 0)
	{
		for (index = 0; index < 6; index++)
			gp_add(0, general[index]);
		gp_add(1, 4);
		gp_add(1, 5);
		for (index = 1; index <= 3; index++)
			gp_add(1, (short)(GP_CATEGORY + index));
	}
	else if (gp.page == 1)
	{
		gp_add(0, GP_PLAYER);
		gp_add(0, GP_GROUP);
		for (index = 0; index < CONTROLLER_REMAP_ACTIONS; index++)
		{
			if (pad_action_groups[index] == gp.group)
				gp_add(0, (short)(GP_ACTION + index));
		}
	}
	else if (gp.page == 2)
	{
		gp_add(0, 8);
		gp_add(0, 9);
		gp_add(0, 10);
		gp_add(0, gp_style_option());
		if (gp_bumper_visible())
			gp_add(0, GP_BUMPER);
	}
	else
	{
		gp_add(0, 16);
		gp_add(0, 17);
	}
}

static struct widget_instance *gp_list(struct widget_instance *screen, short column)
{
	struct widget_instance *options = named(screen, "gamepad_setup_options", 0);

	return options ? named(options, column ? "gamepad_used_list" : "gamepad_avail_list", 0) : NULL;
}

/* the list and row with the focus; FALSE if a button or nothing has it */
static boolean gp_focus_get(struct widget_instance *screen, short *column, short *index)
{
	struct widget_instance *options = named(screen, "gamepad_setup_options", 0);
	short c;

	if (!options)
		return FALSE;
	for (c = 0; c < 2; c++)
	{
		struct widget_instance *list = gp_list(screen, c);

		if (list && options->focused_child == list && list->focused_child)
		{
			*column = c;
			*index = sibling_index(list->focused_child);
			return TRUE;
		}
	}
	return FALSE;
}

static void gp_focus(struct widget_instance *screen, short column, short index)
{
	struct widget_instance *options = named(screen, "gamepad_setup_options", 0);
	struct widget_instance *list = gp_list(screen, column);
	struct widget_instance *row;
	short at;

	if (!options || !list || !list->child)
		return;
	row = list->child;
	for (at = 0; row->next && at < index; row = row->next)
		at++;
	list->focused_child = row;
	list->parameters.list.selected_index = at;
	options->focused_child = list;
}

static struct widget_instance *gp_row_spinner(struct widget_instance *row)
{
	struct widget_instance *child;

	for (child = row->child; child && child->type != 2; child = child->next)
		;
	return child;
}

/* a list's rows made again when its page's are not those it has */
static boolean gp_sync_columns(struct widget_instance *screen)
{
	struct widget_instance staging[2];
	struct widget_instance templates[2];
	struct widget_instance *lists[2];
	struct widget_instance *row;
	boolean changed[2];
	short column, index;

	for (column = 0; column < 2; column++)
	{
		lists[column] = gp_list(screen, column);
		staging[column] = *lists[column];
		staging[column].child = staging[column].focused_child = NULL;
		templates[column] = gp.tpl[column];
		templates[column].parent = &staging[column];
		changed[column] = gp.built_count[column] != gp.count[column] ||
			memcmp(gp.built[column], gp.rows[column], gp.count[column] * sizeof(short));
	}
	for (column = 0; column < 2; column++)
	{
		if (!changed[column])
			continue;
		for (index = 0; index < gp.count[column]; index++)
		{
			wchar_t const *const *values;
			short count;
			short *value;

			row = ui_widget_port_clone_row(&templates[column]);
			if (!row)
				goto failed;
			row->vertical_offset = (short)(gp.base + index * gp.pitch);
			row->horizontal_offset = 0;
			if (gp.spinner_tag != NONE && gp_slider(gp.rows[column][index], &values, &count, &value) &&
				!ui_widget_port_add_spinner(row, gp.spinner_tag, gp_slider_x(gp.rows[column][index]), 2, values, count))
				goto failed;
		}
	}
	for (column = 0; column < 2; column++)
	{
		if (!changed[column])
			continue;
		lists[column]->focused_child = NULL;
		while (lists[column]->child)
			ui_widget_delete(lists[column]->child);
		lists[column]->child = staging[column].child;
		for (row = lists[column]->child; row; row = row->next)
			row->parent = lists[column];
		gp.built_count[column] = gp.count[column];
		memcpy(gp.built[column], gp.rows[column], sizeof(gp.built[column]));
		lists[column]->focused_child = lists[column]->child;
		lists[column]->parameters.list.selected_index = 0;
	}
	return TRUE;

failed:
	for (column = 0; column < 2; column++)
	{
		while (staging[column].child)
			ui_widget_delete(staging[column].child);
		gp.count[column] = MAX(0, gp.built_count[column]);
		memcpy(gp.rows[column], gp.built[column], sizeof(gp.rows[column]));
	}
	return FALSE;
}

static wchar_t const *gp_row_text(short column, short index, boolean spinner)
{
	short code;

	gp_text[0] = 0;
	if (index < 0 || index >= gp.count[column])
		return gp_text;
	code = gp.rows[column][index];
	if (code >= GP_ACTION)
	{
		short action = (short)(code - GP_ACTION);
		struct controller_device device;
		wchar_t shown[64];
		char const *name;

		controller_device_get(gp.port, &device);
		name = pad_editor.capturing == action && pad_editor.port == gp.port ? "PRESS A BUTTON" :
			controller_remap_button_name(pad_editor.buttons[gp.port][action], device.family);
		gp_widen(name, shown, NUMBEROF(shown));
		usnprintf(gp_text, NUMBEROF(gp_text) - 1, L"%s: %s", pad_action_labels[action], shown);
	}
	else if (code > GP_CATEGORY && code < GP_PLAYER)
		usnprintf(gp_text, NUMBEROF(gp_text) - 1, L"%s", gp_category_names[code - GP_CATEGORY - 1]);
	else
	{
		wchar_t const *label = code == GP_PLAYER ? L"PLAYER" : code == GP_GROUP ? L"SHOW" : gp_short_labels[code];
		wchar_t const *const *values;
		short count;
		short *value;

		if (spinner || !gp_slider(code, &values, &count, &value))
			usnprintf(gp_text, NUMBEROF(gp_text) - 1, L"%s", label);
		else
			usnprintf(gp_text, NUMBEROF(gp_text) - 1, L"%s: < %s >", label, values[*value]);
	}
	gp_text[NUMBEROF(gp_text) - 1] = 0;
	return gp_text;
}

static wchar_t const *gp_help_text(struct widget_instance *screen)
{
	static wchar_t help[256];
	struct controller_device device;
	char caption[256];
	short column, slot, code, index;

	help[0] = 0;
	if (!gp_focus_get(screen, &column, &slot) || slot >= gp.count[column])
		return help;
	code = gp.rows[column][slot];
	controller_device_get(gp.port, &device);
	caption[0] = 0;
	if (gp.page == 1 && pad_editor.proposed != NONE)
	{
		wchar_t const *label = pad_action_labels[pad_editor.conflict];
		char conflict[64];

		for (index = 0; label[index] && index < 63; index++)
			conflict[index] = (char)label[index];
		conflict[index] = 0;
		snprintf(caption, sizeof(caption), "%s already uses %s.\r\nReplacing will leave that action UNBOUND.\r\nPress %s or Enter to replace it.\r\nPress %s or Escape to cancel.",
			conflict, controller_remap_button_name(pad_editor.button, device.family),
			controller_remap_button_name(0, device.family), controller_remap_button_name(1, device.family));
	}
	else if (gp.page == 1 && pad_editor.capturing != NONE)
		snprintf(caption, sizeof(caption), "Release the button, then press a new one\r\non player %d's controller.\r\nPress Start or Escape to cancel.\r\nDelete on the keyboard leaves it UNBOUND.", pad_editor.port + 1);
	else if (code >= GP_ACTION)
		snprintf(caption, sizeof(caption), "Press %s or Enter to choose a new button.\r\nLeft or Right also changes the button.\r\nPress %s on the controller or Delete\r\non the keyboard to leave this action UNBOUND.",
			controller_remap_button_name(0, device.family), controller_remap_button_name(2, device.family));
	else if (code == GP_PLAYER)
		snprintf(caption, sizeof(caption), "Choose which player's buttons to edit.\r\nEach player has a separate saved layout.\r\nButton names follow their connected controller.\r\nOK saves changes; Cancel discards them.");
	else if (code == GP_GROUP)
		snprintf(caption, sizeof(caption), "Choose a group of actions with Left or Right.\r\nThen select an action to change its button.\r\nUNBOUND means no button is assigned.");
	else if (code > GP_CATEGORY && code < GP_PLAYER)
		ustrncpy(help, gp_category_help[code - GP_CATEGORY - 1], NUMBEROF(help) - 1);
	else if (code < GP_CATEGORY)
		ustrncpy(help, gp_options[code].help, NUMBEROF(help) - 1);
	if (caption[0])
		gp_widen(caption, help, NUMBEROF(help));
	help[NUMBEROF(help) - 1] = 0;
	return help;
}

/* ui_widget.c: the text of the Gamepads screen's rows, titles and help in place of their own (NULL: not ours) */
wchar_t const *pc_menu_gamepad_text(struct widget_instance *widget)
{
	if (!custom_edition_menu_widget(widget) || !gp.active || screen_of(widget) != gp.screen || !widget->name)
		return NULL;
	if (!strcmp(widget->name, "gamepad_item_label"))
	{
		struct widget_instance *row = widget->parent;

		if (!row || !row->parent || !row->parent->name)
			return NULL;
		if (!strcmp(row->parent->name, "gamepad_avail_list"))
			return gp_row_text(0, sibling_index(row), gp_row_spinner(row) != NULL);
		if (!strcmp(row->parent->name, "gamepad_used_list"))
			return gp_row_text(1, sibling_index(row), gp_row_spinner(row) != NULL);
		return NULL;
	}
	if (!strcmp(widget->name, "gamepad_used_label"))
	{
		if (widget->horizontal_offset < 200)
			return gp.page == 0 ? L"GENERAL" : gp_category_names[gp.page - 1];
		return gp.page == 0 ? L"BUTTONS" : L"DESCRIPTION";
	}
	if (!strcmp(widget->name, "gamepad_setup_help"))
	{
		struct widget_instance *screen = screen_of(widget);

		if (!screen->name || strcmp(screen->name, "gamepad_setup_screen"))
			return NULL;
		return gp_help_text(screen);
	}
	return NULL;
}

/* game data 47, each frame */
void pc_menu_gamepad_update(struct widget_instance *options)
{
	struct widget_instance *screen = screen_of(options);
	struct widget_instance *help = named(options, "gamepad_setup_help", 0);
	short column, index, focus_column = NONE, focus_index = 0;
	boolean on_list;

	if (!gp.active || screen != gp.screen || !gp_list(screen, 0) || !gp_list(screen, 1))
		return;
	if (pad_editor.proposed != NONE && (gp.port != pad_editor.port || gp.group != pad_editor.group))
		pad_editor.proposed = NONE;
	if (pad_editor.capturing != NONE)
	{
		int button, result = controller_remap_capture_poll(&button);

		if (result)
		{
			if (result == 1 || result == 2)
				pad_editor_propose(pad_editor.port, pad_editor.capturing,
					result == 2 ? CONTROLLER_REMAP_UNBOUND : button);
			pad_editor.capturing = NONE;
		}
	}
	/* Generic spinner events and mouse arrows edit the widget first. Capture
	those edits before publishing settings back to it; root handlers can also
	edit the setting, without a stale widget value overwriting their change. */
	for (column = 0; column < 2; column++)
	{
		struct widget_instance *row;

		for (index = 0, row = gp_list(screen, column)->child;
			row && index < gp.built_count[column]; row = row->next, index++)
		{
			struct widget_instance *spinner = gp_row_spinner(row);
			wchar_t const *const *values;
			short count;
			short *value;

			if (spinner && gp_slider(gp.built[column][index], &values, &count, &value) &&
				spinner->parameters.list.selected_index != gp.shown[column][index])
				*value = (short)PIN(spinner->parameters.list.selected_index, 0, count - 1);
		}
	}
	on_list = gp_focus_get(screen, &focus_column, &focus_index);
	if (gp.pending != NONE)
	{
		gp.page = gp.pending;
		gp.pending = NONE;
		pad_editor.proposed = NONE;
		if (gp.focus_col == NONE)
		{
			gp.focus_col = 0;
			gp.focus_row = 0;
		}
	}
	if (gp.focus_col != NONE)
	{
		focus_column = gp.focus_col;
		focus_index = gp.focus_row;
		gp.focus_col = NONE;
		on_list = TRUE;
	}
	gp_rows_build();
	if (!gp_sync_columns(screen))
		return;
	if (on_list)
	{
		if (!gp.count[focus_column])
		{
			focus_column = 0;
			focus_index = 0;
		}
		gp_focus(screen, focus_column, (short)PIN(focus_index, 0, gp.count[focus_column] - 1));
	}
	for (column = 0; column < 2; column++)
	{
		struct widget_instance *row;

		for (index = 0, row = gp_list(screen, column)->child; row; row = row->next, index++)
		{
			struct widget_instance *spinner = gp_row_spinner(row);
			wchar_t const *const *values;
			short count;
			short *value;

			if (spinner && index < gp.count[column] && gp_slider(gp.rows[column][index], &values, &count, &value))
				{
				spinner->parameters.list.selected_index = *value;
				gp.shown[column][index] = *value;
			}
		}
	}
	if (help)
		help->vertical_offset = (short)(gp.help_y + (gp.page == 0 ? 30 : -121));
}

/* B, the Back button: from a page to the main one; FALSE on the main page (the screen then goes back) */
boolean pc_menu_gamepad_back(struct widget_instance *widget)
{
	struct widget_instance *screen;

	if (!gp.active || !widget)
		return FALSE;
	screen = screen_of(widget);
	if (!screen->name || strcmp(screen->name, "gamepad_setup_screen"))
		return FALSE;
	if (pad_editor.proposed != NONE)
	{
		pad_editor.proposed = NONE;
		return TRUE;
	}
	if (pad_editor.capturing != NONE)
	{
		pad_editor.capturing = NONE;
		controller_remap_capture_end();
		return TRUE;
	}
	if (gp.page == 0)
		return FALSE;
	gp.pending = 0;
	gp.focus_col = 1;
	gp.focus_row = (short)(2 + gp.page - 1);
	return TRUE;
}

/* ui_widget.c: the d-pad, X and Y on the Gamepads screen */
boolean pc_menu_gamepad_button(struct widget_instance *widget, short button)
{
	struct widget_instance *options;
	wchar_t const *const *values;
	short column, index, code, count, direction = 0;
	short *value;

	if (!gp.active || widget->parent || !widget->name || strcmp(widget->name, "gamepad_setup_screen"))
		return FALSE;
	options = named(widget, "gamepad_setup_options", 0);
	if (!options)
		return FALSE;
	if (!gp_focus_get(widget, &column, &index))
	{
		/* a button has the focus: up goes back to the last row */
		if (button == _gamepad_binary_button_dpad_up && gp.count[0])
		{
			gp_focus(widget, gp.count[1] ? 1 : 0, (short)((gp.count[1] ? gp.count[1] : gp.count[0]) - 1));
			return TRUE;
		}
		return FALSE;
	}
	if (index >= gp.count[column])
		return FALSE;
	code = gp.rows[column][index];
	if (button == _gamepad_binary_button_dpad_down && index == gp.count[column] - 1)
	{
		if (column == 0 && gp.count[1])
			gp_focus(widget, 1, 0);
		else if (named(options, "common_button_back", 0))
			options->focused_child = named(options, "common_button_back", 0);
		return TRUE;
	}
	if (button == _gamepad_binary_button_dpad_up && index == 0 && column == 1)
	{
		gp_focus(widget, 0, (short)(gp.count[0] - 1));
		return TRUE;
	}
	if (button == _gamepad_binary_button_dpad_left)
		direction = -1;
	else if (button == _gamepad_binary_button_dpad_right)
		direction = 1;
	if (direction)
	{
		if (pad_editor.proposed != NONE || pad_editor.capturing != NONE)
			return TRUE;
		if (gp_slider(code, &values, &count, &value))
		{
			*value = (short)((*value + direction + count) % count);
			return TRUE;
		}
		if (code >= GP_ACTION)
		{
			short action = (short)(code - GP_ACTION);

			pad_editor_propose(gp.port, action, controller_remap_cycle(pad_editor.buttons[gp.port][action], direction));
			return TRUE;
		}
		return FALSE;
	}
	if (button == BUTTON_X && code >= GP_ACTION)
	{
		if (pad_editor.proposed == NONE && pad_editor.capturing == NONE)
			pad_editor_propose(gp.port, (short)(code - GP_ACTION), CONTROLLER_REMAP_UNBOUND);
		return TRUE;
	}
	if (button == BUTTON_Y)
	{
		if (gp.page == 1)
		{
			controller_remap_profile(gp.port, pad_editor.buttons[gp.port]);
			pad_editor.custom[gp.port] = FALSE;
			pad_editor.proposed = NONE;
		}
		else
			gp_values_read(TRUE, gp.page);
		return TRUE;
	}
	return FALSE;
}

/* 132: A on a row */
static boolean gp_activate(struct widget_instance *widget)
{
	struct widget_instance *row = widget;
	wchar_t const *const *values;
	short column, index, code, count;
	short *value;

	while (row && !(row->parent && row->parent->name &&
		(!strcmp(row->parent->name, "gamepad_avail_list") || !strcmp(row->parent->name, "gamepad_used_list"))))
	{
		row = row->parent;
	}
	if (!row)
		return TRUE;
	column = !strcmp(row->parent->name, "gamepad_used_list") ? 1 : 0;
	index = sibling_index(row);
	if (index < 0 || index >= gp.count[column])
		return TRUE;
	code = gp.rows[column][index];
	if (pad_editor.proposed != NONE)
	{
		controller_remap_assign(pad_editor.buttons[pad_editor.port], pad_editor.proposed, pad_editor.button);
		pad_editor.custom[pad_editor.port] = TRUE;
		pad_editor.proposed = NONE;
		return TRUE;
	}
	if (pad_editor.capturing != NONE)
		return TRUE;
	if (code > GP_CATEGORY && code < GP_PLAYER)
	{
		gp.pending = (short)(code - GP_CATEGORY);
		gp.focus_col = NONE;
	}
	else if (gp_slider(code, &values, &count, &value))
		*value = (short)((*value + 1) % count);
	else if (code >= GP_ACTION)
	{
		pad_editor.capturing = (short)(code - GP_ACTION);
		pad_editor.port = gp.port;
		controller_remap_capture_begin(gp.port);
	}
	return TRUE;
}

/* 131: OK */
static boolean gp_save(struct widget_instance *widget)
{
	short index, port;
	boolean display = FALSE, result = TRUE;

	if (pad_editor.proposed != NONE || pad_editor.capturing != NONE)
		return FALSE;
	for (index = 0; index < GP_OPTIONS; index++)
	{
		struct gp_option *option = &gp_options[index];

		if (option->value == option->loaded)
			continue;
		if (setting_write(option->key, option->real[option->value]))
		{
			option->loaded = option->value;
			if (!strncmp(option->key, "display.", 8))
				display = TRUE;
		}
		else
			platform_log("menus: could not write %s", option->key);
	}
	for (port = 0; port < 4; port++)
	{
		char text[CONTROLLER_REMAP_TEXT] = { 0 };

		if (pad_editor.custom[port])
			controller_remap_format(pad_editor.buttons[port], text, sizeof(text));
		if (strcmp(text, config_string(controller_remap_setting(port))) &&
			!config_write(controller_remap_setting(port), text))
		{
			result = FALSE;
		}
	}
	if (display)
		platform_display_apply();
	profile_settings_save(widget);
	return result;
}

/* NONE if the function is not this screen's, else its result */
short pc_menu_gamepad_function(struct widget_instance *widget, word function)
{
	struct widget_instance *screen = screen_of(widget);

	if (!screen->name || strcmp(screen->name, "gamepad_setup_screen"))
		return NONE;
	if (function == 129)
	{
		struct widget_instance *avail = named(widget, "gamepad_avail_list", 0);
		struct widget_instance *used = named(widget, "gamepad_used_list", 0);
		struct widget_instance *help = named(widget, "gamepad_setup_help", 0);

		if (!avail || !avail->child || !used)
			return TRUE;
		memset(&gp, 0, sizeof(gp));
		gp.screen = screen;
		gp.pending = NONE;
		gp.focus_col = 0;
		gp.focus_row = 0;
		gp.owner = controller_of(screen);
		gp.port = gp.owner;
		gp.tpl[0] = *avail->child;
		gp.tpl[1] = used->child ? *used->child : *avail->child;
		gp.tpl[1].parent = used;
		gp.base = avail->child->vertical_offset;
		gp.pitch = avail->child->next ? (short)(avail->child->next->vertical_offset - avail->child->vertical_offset) : 30;
		if (gp.pitch <= 0)
			gp.pitch = 30;
		gp.help_y = help ? help->vertical_offset : 0;
		gp.spinner_tag = ui_widget_port_donor_spinner(screen);
		gp.built_count[0] = gp.built_count[1] = -1;
		widget->focused_child = avail;
		gp_values_read(FALSE, NONE);
		gp_pad_init();
		gp.active = TRUE;
	}
	else if (function == 130)
	{
		gp.active = FALSE;
		gp.screen = NULL;
		pad_editor.capturing = pad_editor.proposed = NONE;
		controller_remap_capture_end();
	}
	else if (function == 131)
		return gp_save(widget);
	else if (function == 132)
	{
		if (gp.active)
			gp_activate(widget);
	}
	else
		return NONE;
	return TRUE;
}

void custom_edition_menu_widget_deleted(struct widget_instance *widget)
{
	if (ce_video_list == widget)
		ce_video_list = NULL;
	if (ce_network_list == widget)
		ce_network_list = NULL;
	if (gp.active && widget == gp.screen)
	{
		gp.active = FALSE;
		gp.screen = NULL;
		pad_editor.capturing = pad_editor.proposed = NONE;
		controller_remap_capture_end();
	}
}
