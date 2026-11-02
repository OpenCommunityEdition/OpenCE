/* Instance access stays beside the menu adapter's existing widget helpers.
 * These callbacks never load/release tag tables or touch a map's pause UI. */
#include "frontend_menu.h"
#include "controller_buttons.h"
#include "tag_files/tag_files.h"
#include "frontend_pages.inl"
#include "frontend_multiplayer.inl"

boolean network_game_is_active(void);

static boolean frontend_menu_can_edit(void)
{
	return frontend_menu_available() && main_menu_is_active() && !network_game_is_active();
}

static boolean frontend_menu_legacy_root(struct widget_instance *widget)
{
	char const *name;

	if (!widget || widget->parent || !frontend_menu_can_edit())
		return FALSE;
	name = tag_get_name(widget->definition_tag_index);
	return name && (!strcmp(name, FRONTEND_MENU_PC) || !strcmp(name, FRONTEND_MENU_XBOX));
}

void frontend_menu_attach(struct widget_instance *widget)
{
	struct widget_instance *entry, *last;

	if (!frontend_menu_legacy_root(widget))
		return;
	entry = ui_widget_load_by_name_or_tag("pc\\modern\\entry", NONE, widget,
		widget->local_player_index, NONE, NONE, NONE);
	if (!entry)
		return;
	entry->horizontal_offset = 450;
	entry->vertical_offset = 4;
	last = widget->child;
	if (last)
	{
		while (last->next)
			last = last->next;
		last->next = entry;
		entry->previous = last;
	}
	else
		widget->child = entry;
}

int frontend_menu_shortcut(struct widget_instance *widget, struct event_record *event)
{
	/* event_manager's button event (3), Y (also the keyboard's Tab).
	 * A repeat must not reopen the freshly created screen. */
	if (!frontend_menu_legacy_root(widget) || event->type != 3 ||
		event->data.button.index != _gamepad_analog_button_y || event->data.button.value != 1)
		return FALSE;
	return ui_widget_load_by_name_or_tag(FRONTEND_MENU_SCREEN, NONE, NULL,
		widget->local_player_index, NONE, NONE, NONE) != NULL;
}

/* Tags live for one ui.map. Keep indices, never widget pointers: changing a
 * page destroys its instances. No config writes occur while browsing pages. */
#define FRONTEND_PENDING_MAX 256
#define FRONTEND_ROWS_VISIBLE 8
static struct
{
	boolean owns_profile, error;
	short count;
	short first[NUMBEROF(frontend_pages)];
	struct { long tag; short index, original; } pending[FRONTEND_PENDING_MAX];
} frontend_session;

boolean ui_widget_port_open(struct widget_instance *, char const *, boolean *);
static void settings_help(struct widget_instance *list);
static void master_servers_show(struct widget_instance *list);

void frontend_menu_session_reset(void)
{
	if (frontend_session.owns_profile)
		player_ui_end_editing_profile();
	memset(&frontend_session, 0, sizeof(frontend_session));
}

static short frontend_page(struct widget_instance *root)
{
	char const *path = root ? tag_get_name(root->definition_tag_index) : NULL;
	short page;
	for (page = 0; path && page < NUMBEROF(frontend_pages); page++)
		if (!strcmp(path, frontend_pages[page].path)) return page;
	return NONE;
}

static void frontend_error(struct widget_instance *root, wchar_t const *message)
{
	frontend_session.error = TRUE;
	text_set(named(root, "status", 0), message);
}

static short frontend_pending(long tag)
{
	short i;
	for (i = 0; i < frontend_session.count; i++)
		if (frontend_session.pending[i].tag == tag) return i;
	return NONE;
}

static boolean frontend_restore(struct widget_instance *spinner, struct pc_menu_setting *setting)
{
	short entry = frontend_pending(spinner->definition_tag_index);
	setting->loaded_index = NONE;
	setting_load(spinner);
	if (entry != NONE)
	{
		spinner->parameters.list.selected_index = frontend_session.pending[entry].index;
		setting->loaded_index = frontend_session.pending[entry].original;
	}
	/* No profile is silently created, and an unavailable profile cannot be
	 * edited as though its zero-initialized spinner were a real value. */
	if (!strncmp(setting->setting, "profile.", 8))
	{
		spinner->parent->disabled = !frontend_session.owns_profile;
		spinner->parent->visible = frontend_session.owns_profile;
	}
	return TRUE;
}

static boolean frontend_capture(struct widget_instance *widget)
{
	struct widget_instance *child;
	struct pc_menu_setting *setting;
	short entry, index;
	for (child = widget->child; child; child = child->next)
	{
		setting = child->type == 2 && !spinner_item(child) ?
			pc_menu_setting_get(child->definition_tag_index) : NULL;
		if (!setting)
		{
			if (!frontend_capture(child)) return FALSE;
			continue;
		}
		if (setting->loaded_index == NONE) continue;
		index = child->parameters.list.selected_index;
		if (index < 0 || index >= setting->value_count) return FALSE;
		entry = frontend_pending(child->definition_tag_index);
		if (entry == NONE)
		{
			if (index == setting->loaded_index) continue;
			if (frontend_session.count == FRONTEND_PENDING_MAX) return FALSE;
			entry = frontend_session.count++;
			frontend_session.pending[entry].tag = child->definition_tag_index;
			frontend_session.pending[entry].original = setting->loaded_index;
		}
		frontend_session.pending[entry].index = index;
	}
	return TRUE;
}

static boolean frontend_commit(void)
{
	struct pc_menu_setting *setting;
	short entry, index;
	/* Validate the whole batch before writing. Individual config writes may
	 * still fail; keep pending values and report partial application for retry. */
	for (entry = 0; entry < frontend_session.count; entry++)
	{
		setting = pc_menu_setting_get(frontend_session.pending[entry].tag);
		index = frontend_session.pending[entry].index;
		if (!setting || index < 0 || index >= setting->value_count) return FALSE;
	}
	for (entry = 0; entry < frontend_session.count; entry++)
	{
		setting = pc_menu_setting_get(frontend_session.pending[entry].tag);
		index = frontend_session.pending[entry].index;
		if (index != frontend_session.pending[entry].original &&
			!setting_write(setting->setting, setting->values[index])) return FALSE;
	}
	if (frontend_session.owns_profile && player_ui_edit_profile_is_dirty() &&
		!player_ui_save_profile()) return FALSE;
	return TRUE;
}

static boolean frontend_open_page(struct widget_instance *root, short page, boolean *deleted)
{
	if (page < 0 || page >= NUMBEROF(frontend_pages)) return FALSE;
	if (!frontend_capture(root))
	{
		frontend_error(root, L"Could not keep pending changes. Stay here and retry.");
		return FALSE;
	}
	frontend_session.error = FALSE;
	*deleted = ui_widget_load_by_name_or_tag(frontend_pages[page].path, NONE, NULL,
		root->local_player_index, NONE, NONE, NONE) != NULL;
	return *deleted;
}

static boolean frontend_editor(struct widget_instance *widget, boolean *deleted)
{
	char const *target = NULL;
	char const *key = widget->name;
	struct widget_instance *root = screen_of(widget);
	short entry;
	struct pc_menu_setting *setting;
	if (!frontend_capture(root)) return FALSE;
	if (!strcmp(key, "row_profiles"))
	{
		for (entry = 0; entry < frontend_session.count; entry++)
		{
			setting = pc_menu_setting_get(frontend_session.pending[entry].tag);
			if (setting && !strncmp(setting->setting, "profile.", 8) &&
				frontend_session.pending[entry].index != frontend_session.pending[entry].original) break;
		}
		if (entry < frontend_session.count ||
			(frontend_session.owns_profile && player_ui_edit_profile_is_dirty()))
		{
			frontend_error(root, L"Save or cancel profile edits before changing profile.");
			return FALSE;
		}
		/* Unchanged profile entries must not be restored over another profile. */
		for (entry = 0; entry < frontend_session.count;)
		{
			setting = pc_menu_setting_get(frontend_session.pending[entry].tag);
			if (setting && !strncmp(setting->setting, "profile.", 8))
				frontend_session.pending[entry] = frontend_session.pending[--frontend_session.count];
			else entry++;
		}
		if (frontend_session.owns_profile) player_ui_end_editing_profile();
		frontend_session.owns_profile = FALSE;
		frontend_session.error = FALSE;
		return ui_widget_port_open(widget, "pc\\main_menu\\profile_manager\\player_profile_manager_screen", deleted);
	}
	if (!strcmp(key, "row_keyboard")) target = "controls_setup\\controls_settings_screen";
	else if (!strcmp(key, "row_gamepad")) target = "controller_remapping\\controller_remapping_screen";
	else
	{
		if (!frontend_session.owns_profile)
		{
			frontend_error(root, L"Choose SELECT PROFILE to select or create a profile.");
			return FALSE;
		}
		if (!strcmp(key, "row_name"))
		{
			/* The existing name editor owns text entry and validation. */
			return game_function("player profile change name", widget, NULL, deleted);
		}
		if (!strcmp(key, "row_color")) target = "color_edit\\color_select_screen";
	}
	if (target)
	{
		char path[256];
		snprintf(path, sizeof(path), "pc\\main_menu\\settings_select\\player_setup\\player_profile_edit\\%s", target);
		return ui_widget_port_open(widget, path, deleted);
	}
	return FALSE;
}

#include "frontend_navigation.inl"

static boolean frontend_menu_event(struct widget_instance *widget, char const *name, boolean *deleted)
{
	struct widget_instance *root = screen_of(widget);
	struct widget_instance *list, *child;
	short index, page;
	if (!frontend_menu_can_edit()) return FALSE;
	if (frontend_navigation_root(root)) return frontend_navigation_event(widget, name);
	if (!strcmp(name, "port frontend focus") && frontend_menu_legacy_root(root))
	{
		for (list = root->child; list; list = list->next)
		{
			if (list->type != 3 || !list->child || !strcmp(list->name, "entry")) continue;
			child = list->child;
			for (index = 0; child->next && index < list->parameters.list.selected_index; index++) child = child->next;
			list->focused_child = child;
			root->focused_child = list;
			return TRUE;
		}
		return FALSE;
	}
	if (!strcmp(name, "port frontend open") && frontend_menu_legacy_root(root))
	{
		frontend_menu_session_reset();
		*deleted = ui_widget_load_by_name_or_tag(FRONTEND_MENU_SCREEN, NONE, NULL,
			root->local_player_index, NONE, NONE, NONE) != NULL;
		return *deleted;
	}
	page = frontend_page(root);
	if (page == NONE) return FALSE;
	if (!strcmp(name, "port frontend init"))
	{
		if (page == 5 && !frontend_session.owns_profile)
			frontend_session.owns_profile = pc_menu_profile_edit_begin();
		settings_each(root, frontend_restore);
		if (page == 5 && !frontend_session.owns_profile)
			frontend_error(root, L"Choose SELECT PROFILE to select or create a profile.");
		list = named(root, "options", 0);
		child = named(root, "tabs", 0);
		if (child)
		{
			struct widget_instance *tab = child->child;
			for (index = 0; tab && index < frontend_pages[page].tab; index++) tab = tab->next;
			child->focused_child = tab;
			child->parameters.list.selected_index = frontend_pages[page].tab;
		}
		/* Start on the first option, while Up still reaches the tab bar. */
		for (child = list ? list->child : NULL; child; child = child->next)
			if (!strncmp(child->name, "row_", 4) && !child->disabled)
			{
				list->focused_child = child;
				break;
			}
		return TRUE;
	}
	if (!strcmp(name, "port frontend defaults"))
	{
		settings_each(root, setting_default_show);
		return TRUE;
	}
	if (!strcmp(name, "port frontend editor")) return frontend_editor(widget, deleted);
	if (!strcmp(name, "port frontend page"))
	{
		index = !strncmp(widget->name, "tab_", 4) ? atoi(widget->name + 4) :
			!strncmp(widget->name, "row_link_", 9) ? atoi(widget->name + 9) : NONE;
		return frontend_open_page(root, index, deleted);
	}
	if (!strcmp(name, "port frontend next tab"))
		return frontend_open_page(root, (short)((frontend_pages[page].tab + 1) % 6), deleted);
	if (!strcmp(name, "port frontend back") && page >= 6)
		return frontend_open_page(root, frontend_pages[page].parent, deleted);
	if (!strcmp(name, "port frontend apply"))
	{
		if (!frontend_capture(root) || !frontend_commit())
		{
			frontend_error(root, L"Save failed; some changes may be active. Retry OK.");
			return FALSE;
		}
		platform_display_apply();
	}
	else if (strcmp(name, "port frontend cancel") && strcmp(name, "port frontend back")) return FALSE;
	frontend_menu_session_reset();
	main_goto_main_menu();
	return TRUE;
}

/* A render-only exclusion: navigation still sees every enabled row. The
 * engine calls this before drawing AND before registering mouse targets. */
int frontend_menu_row_drawable(struct widget_instance *widget)
{
	if (!frontend_menu_available() || !widget || !widget->parent) return TRUE;
	if (frontend_multiplayer_root(screen_of(widget))) return frontend_multiplayer_drawable(widget);
	if (frontend_navigation_root(screen_of(widget))) return frontend_navigation_drawable(widget);
	if (strncmp(widget->name, "row_", 4) || frontend_page(screen_of(widget)) == NONE) return TRUE;
	return widget->vertical_offset >= 137 && widget->vertical_offset <= 347;
}

static struct widget_instance *frontend_setting_row(struct widget_instance *list, char const *key)
{
	struct widget_instance *row, *child;
	struct pc_menu_setting *setting;
	for (row = list->child; row; row = row->next)
		for (child = row->child; child; child = child->next)
		{
			setting = child->type == 2 ? pc_menu_setting_get(child->definition_tag_index) : NULL;
			if (setting && !strcmp(setting->setting, key)) return row;
		}
	return NULL;
}

static char const *frontend_row_value(struct widget_instance *row)
{
	struct widget_instance *child;
	struct pc_menu_setting *setting;
	short index;
	for (child = row ? row->child : NULL; child; child = child->next)
	{
		setting = child->type == 2 ? pc_menu_setting_get(child->definition_tag_index) : NULL;
		index = child->parameters.list.selected_index;
		if (setting && index >= 0 && index < setting->value_count) return setting->values[index];
	}
	return "";
}

/* Match the native controller-family selection, using pending values so hiding
 * a row never changes its saved preference. High-Res HUD gates these controls. */
static void frontend_button_rows_show(struct widget_instance *list)
{
	static char const *styles[] = {"display.hud_button_style", "display.hud_button_style_xbox",
		"display.hud_button_style_ps", "display.hud_button_style_switch"};
	struct widget_instance *hud = frontend_setting_row(list, "display.high_res_hud");
	struct widget_instance *automatic = frontend_setting_row(list, "input.auto_button_detection");
	struct widget_instance *keyboard = frontend_setting_row(list, "display.keyboard_glyphs");
	struct widget_instance *bumper = frontend_setting_row(list, "display.hud_bumper_style");
	struct widget_instance *shown, *row;
	struct controller_device device;
	boolean enabled, detect;
	short family = 0, index;
	if (!hud || !automatic) return;
	enabled = !strcmp(frontend_row_value(hud), "true");
	detect = !strcmp(frontend_row_value(automatic), "true");
	controller_device_get(controller_of(list), &device);
	if (detect && device.family >= 0 && device.family <= 3) family = (short)device.family;
	shown = frontend_setting_row(list, styles[family]);
	automatic->visible = enabled;
	if (keyboard) keyboard->visible = enabled && detect;
	for (index = 0; index < 4; index++)
	{
		row = frontend_setting_row(list, styles[index]);
		if (row)
		{
			row->visible = enabled && row == shown;
			row->disabled = FALSE;
			row->alpha_modifier = 1.0f;
		}
	}
	if (bumper)
		bumper->visible = enabled && shown && frontend_row_value(shown)[0] &&
			controller_button_style_index(frontend_row_value(shown)) < 3;
	if (list->focused_child && !list->focused_child->visible)
		list->focused_child = enabled && shown ? shown : hud;
	for (row = list->child, index = 0; row; row = row->next, index++)
		if (row == list->focused_child) list->parameters.list.selected_index = index;
}

static void frontend_rows_show(struct widget_instance *list)
{
	struct widget_instance *resolution = frontend_setting_row(list, "display.resolution");
	struct widget_instance *window = frontend_setting_row(list, "display.window_size");
	struct widget_instance *shown;
	if (resolution && window)
	{
		shown = !strcmp(frontend_row_value(frontend_setting_row(list, "display.mode")), "windowed") ? window : resolution;
		resolution->visible = shown == resolution;
		window->visible = shown == window;
		if (list->focused_child == (shown == resolution ? window : resolution)) list->focused_child = shown;
	}
	frontend_button_rows_show(list);
}

static void frontend_menu_help(struct widget_instance *list)
{
	struct widget_instance *root = screen_of(list);
	struct widget_instance *row, *child, *title, *value;
	struct pc_menu_setting *setting;
	short page = frontend_page(root), count = 0, focused = NONE, index = 0;
	short *first;
	long strings;
	char path[256];
	wchar_t status[ROW_TEXT_LENGTH];
	if (!frontend_menu_can_edit() || page == NONE || strcmp(list->name, "options")) return;
	frontend_rows_show(list);
	master_servers_show(list);
	first = &frontend_session.first[page];
	for (row = list->child; row; row = row->next)
	{
		if (strncmp(row->name, "row_", 4) || !row->visible) continue;
		if (row == list->focused_child) focused = count;
		count++;
	}
	if (focused != NONE && focused < *first) *first = focused;
	if (focused >= *first + FRONTEND_ROWS_VISIBLE) *first = focused - FRONTEND_ROWS_VISIBLE + 1;
	if (*first > MAX(0, count - FRONTEND_ROWS_VISIBLE)) *first = MAX(0, count - FRONTEND_ROWS_VISIBLE);
	for (row = list->child; row; row = row->next)
		if (!strncmp(row->name, "row_", 4) && row->visible)
			row->vertical_offset = (short)(137 + 30 * (index++ - *first));
	if (!frontend_session.error)
	{
		if (count > FRONTEND_ROWS_VISIBLE)
			usnprintf(status, NUMBEROF(status) - 1, L"%d-%d / %d   UP / DOWN TO SCROLL", *first + 1, MIN(count, *first + FRONTEND_ROWS_VISIBLE), count);
		else status[0] = 0;
		text_set(named(root, "status", 0), status);
	}
	settings_help(list);
	title = named(root, "help_title", 0);
	value = named(root, "help_value", 0);
	row = list->focused_child;
	if (row && strncmp(row->name, "row_", 4) && list->parameters.list.extended_description)
		list->parameters.list.extended_description->parameters.text_box.string_list_index = 0;
	index = list->parameters.list.extended_description ? list->parameters.list.extended_description->parameters.text_box.string_list_index - 1 : NONE;
	if (!title || !value) return;
	title->visible = value->visible = index >= 0;
	if (index < 0 || !row) return;
	title->parameters.text_box.string_list_index = index;
	text_set(value, L"");
	for (child = row->child; child; child = child->next)
	{
		setting = child->type == 2 ? pc_menu_setting_get(child->definition_tag_index) : NULL;
		if (!setting) continue;
		snprintf(path, sizeof(path), "%s strings", tag_get_name(child->definition_tag_index));
		strings = tag_loaded('ustr', path);
		index = child->parameters.list.selected_index;
		if (strings != NONE && index >= 0 && index < setting->value_count)
			text_set(value, unicode_string_list_get_string(strings, index));
	}
}

