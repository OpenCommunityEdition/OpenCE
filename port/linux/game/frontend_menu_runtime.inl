/* Instance access stays beside the menu adapter's existing widget helpers.
 * These callbacks never load/release tag tables or touch a map's pause UI. */
#include "frontend_menu.h"
#include "controller_buttons.h"
#include "tag_files/tag_files.h"
#include "frontend_pages.inl"
#include "frontend_multiplayer.inl"
#include "frontend_legacy_footers.inl"

boolean network_game_is_active(void);
boolean ui_widget_port_open(struct widget_instance *, char const *, boolean *);

#include "mcc_xbox_menu.inl"

static boolean frontend_menu_can_edit(void)
{
	return frontend_menu_available();
}

static boolean frontend_menu_legacy_root(struct widget_instance *widget)
{
	char const *name;

	if (!widget || widget->parent || !frontend_menu_can_edit() || !main_menu_is_active() || network_game_is_active())
		return FALSE;
	name = tag_get_name(widget->definition_tag_index);
	return name && (!strcmp(name, FRONTEND_MENU_PC) || !strcmp(name, FRONTEND_MENU_XBOX));
}

void frontend_menu_attach(struct widget_instance *widget)
{
    static char const *settings[] = {"audio", "video", "network"};
    static char const *controls[] = {"keyboard", "controller", "glyphs"};
    char const *name, **links;
    char path[96];
    struct widget_instance *entry, *last, *save, *before, *description;
    int index;
    mcc_xbox_attach(widget);
    if (!widget || !frontend_menu_can_edit() || strcmp(frontend_menu_style(), "xbox")) return;
    name = tag_get_name(widget->definition_tag_index);
    if (!name) return;
    /* Gameplay caches may omit ui.map's button-key art. Use the same Xbox
     * redraws as the existing HUD, never the user's modern glyph preference. */
    for (index = 0; index < 3; index++)
    {
        static char const glyphs[] = "abx";
        snprintf(path, sizeof(path), "pc\\frontend_legacy\\footer_glyph_%c", glyphs[index]);
        if (!strcmp(name, path))
        {
            long fallback;
            snprintf(path, sizeof(path), "ui\\shell\\bitmaps\\%c_butn", glyphs[index]);
            if (tag_loaded('bitm', path) != NONE) return;
            snprintf(path, sizeof(path), "pc\\frontend_legacy\\footer_glyph_%c_fallback", glyphs[index]);
            fallback = tag_loaded('DeLa', path);
            if (fallback != NONE) widget->definition_tag_index = fallback;
            return;
        }
    }
    /* Only this live Xbox instance borrows the matching footer definition.
     * Its native callbacks/name stay intact; no stock tag is modified. */
    for (index = 0; index < NUMBEROF(frontend_legacy_footers); index++)
        if (!strcmp(name, frontend_legacy_footers[index].native))
        {
            long replacement = tag_loaded('DeLa', frontend_legacy_footers[index].replacement);
            char const *root_name = tag_get_name(screen_of(widget)->definition_tag_index);
            static char const profile[] = "pc\\main_menu\\settings_select\\player_setup\\player_profile_edit\\";
            if (!root_name || strncmp(root_name, profile, sizeof(profile) - 1)) return;
            if (replacement == NONE) return;
            widget->definition_tag_index = replacement;
            widget->parameters.text_box.string_list_index = 0;
            entry = ui_widget_load_by_name_or_tag(frontend_legacy_footers[index].glyph, NONE,
                widget, widget->local_player_index, NONE, NONE, NONE);
            if (entry) widget->child = entry;
            return;
        }
    if (!strcmp(name, "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\player_profile_edit_screen"))
    {
        entry = named(widget, "blueline", 0);
        if (entry) entry->vertical_offset += 99;
        return;
    }
    if (!strcmp(name, "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\profile_edit_select_list"))
        links = settings;
    else if (!strcmp(name, "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\advanced_controls\\advanced_controls_menu"))
        links = controls;
    else return;
    save = links == settings ? named(widget, "save_changes_profile_item", 0) : NULL;
    before = links == settings ? named(widget, "color_profile_item", 0) : NULL;
    for (index = 0; index < 3; index++)
    {
        snprintf(path, sizeof(path), "pc\\frontend_legacy\\%s", links[index]);
        entry = ui_widget_load_by_name_or_tag(path, NONE, widget, widget->local_player_index, NONE, NONE, NONE);
        if (!entry) continue;
        last = before ? before->previous : widget->child;
        if (!before) while (last && last->next) last = last->next;
        entry->previous = last;
        entry->next = before;
        if (last) last->next = entry;
        else widget->child = entry;
        if (before) before->previous = entry;
    }
    if (before) before->vertical_offset += 99;
    if (save) save->vertical_offset += 99;
    description = links == settings ? widget->parameters.list.extended_description : NULL;
    if (description)
    {
        entry = ui_widget_load_by_name_or_tag("pc\\frontend_legacy\\description", NONE, description,
            widget->local_player_index, NONE, NONE, NONE);
        if (entry)
        {
            last = description->child;
            while (last && last->next) last = last->next;
            entry->previous = last;
            if (last) last->next = entry;
            else description->child = entry;
            entry->visible = FALSE;
        }
    }
}

/* Native settings use X for defaults and A to accept a value page. Binding
 * editors retain A-to-bind/X-to-clear while a binding row owns focus. */
static boolean frontend_menu_legacy_footer_action(struct widget_instance *root, struct event_record *event)
{
	struct widget_instance *focused = root, *button = NULL, *spinner;
	char const *name = tag_get_name(root->definition_tag_index), *path;
	boolean deleted = FALSE;
	if (event->type != 3 || event->data.button.value != 1) return FALSE;
	while (focused->focused_child) focused = focused->focused_child;
	if (event->data.button.index == 2 && name &&
		!strstr(name, "\\controls_setup\\") && !strstr(name, "\\controller_remapping\\"))
		button = named(root, "button_defaults", 0);
	else if (event->data.button.index == 0)
		for (spinner = focused; spinner && spinner != root; spinner = spinner->parent)
			if (spinner->type == 2 && pc_menu_setting_get(spinner->definition_tag_index))
			{
				button = named(root, "button_ok", 0);
				break;
			}
	path = button ? tag_get_name(button->definition_tag_index) : NULL;
	if (!path || strncmp(path, "pc\\frontend_legacy\\footer\\", 26)) return FALSE;
	return ui_widget_port_dispatch_event(button, 0, event->controller_index, &deleted);
}

int frontend_menu_shortcut(struct widget_instance *widget, struct event_record *event)
{
	struct widget_instance *focused;
	char const *name, *target = NULL;
	char path[256];
	boolean deleted = FALSE;
	if (mcc_xbox_shortcut(widget, event)) return TRUE;
	if (widget && !widget->parent && frontend_menu_can_edit() &&
		!strcmp(frontend_menu_style(), "xbox") &&
		frontend_menu_legacy_footer_action(widget, event)) return TRUE;
	/* The native Advanced Controls list accepts A itself, before dispatching
	 * to a child. Open our appended editor actions before that save/back event. */
	if (widget && !widget->parent && frontend_menu_can_edit() &&
		!strcmp(frontend_menu_style(), "xbox") && event->type == 3 &&
		(event->data.button.index == 0 || event->data.button.index == 12) && event->data.button.value == 1)
	{
		focused = widget;
		while (focused->focused_child) focused = focused->focused_child;
		name = tag_get_name(focused->definition_tag_index);
		if (name && !strcmp(name, "pc\\frontend_legacy\\keyboard")) target = "controls_setup\\controls_settings_screen";
		else if (name && !strcmp(name, "pc\\frontend_legacy\\controller")) target = "controller_remapping\\controller_remapping_screen";
		else if (name && !strcmp(name, "pc\\frontend_legacy\\glyphs")) target = "controller_appearance\\controller_appearance_screen";
		if (target && focused->parent)
		{
			if (!game_function("plyr prf save adv cntl set", focused->parent, event, &deleted)) return FALSE;
			snprintf(path, sizeof(path), "pc\\main_menu\\settings_select\\player_setup\\player_profile_edit\\%s", target);
			return ui_widget_port_open(focused, path, &deleted);
		}
	}
	/* event_manager's button event (3), Y (also the keyboard's Tab).
	 * A repeat must not reopen the freshly created screen. */
	if (!frontend_menu_legacy_root(widget) || event->type != 3 ||
		event->data.button.index != _gamepad_analog_button_y || event->data.button.value != 1)
		return FALSE;
	return ui_widget_load_by_name_or_tag(FRONTEND_MENU_SCREEN, NONE, NULL,
		widget->local_player_index, NONE, NONE, NONE) != NULL;
}

/* The native description's frames and strings correspond only to native
 * rows. Inserted links have no matching frame; Save retains its old index. */
short frontend_menu_legacy_description_index(struct widget_instance *list, short index)
{
	struct widget_instance *child, *detail;
	char const *name;
	short native_index = 0;
	boolean added;
	if (!list || !frontend_menu_can_edit() || !main_menu_is_active() || network_game_is_active() ||
		strcmp(frontend_menu_style(), "xbox")) return index;
	name = tag_get_name(list->definition_tag_index);
	if (!name || strcmp(name, "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\profile_edit_select_list")) return index;
	for (child = list->child; child; child = child->next)
	{
		name = tag_get_name(child->definition_tag_index);
		added = name && !strncmp(name, "pc\\frontend_legacy\\", 19);
		if (child == list->focused_child)
		{
			if (list->parameters.list.extended_description)
				for (detail = list->parameters.list.extended_description->child; detail; detail = detail->next)
				{
					char const *path = tag_get_name(detail->definition_tag_index);
					boolean help = path && !strcmp(path, "pc\\frontend_legacy\\description");
					detail->visible = help ? added : !added;
					if (help && added)
						text_set_length(detail, !strcmp(name + 19, "audio") ? L"Adjust music, sound effects|nand audio playback.|n|nCurrent Profile:" :
							!strcmp(name + 19, "video") ? L"Adjust the display, graphics|nand menu style.|n|nCurrent Profile:" :
							L"Configure network connections|nand online play.|n|nCurrent Profile:", 128);
				}
			return added ? NONE : native_index;
		}
		if (!added) native_index++;
	}
	return index;
}

/* Advanced Controls' native help requires a spinner in every selected row.
 * The appended editor links are actions, so leave that service to its rows. */
int frontend_menu_legacy_controls_help(struct widget_instance *list)
{
	char const *name;
	boolean added;
	if (!list || !frontend_menu_can_edit() || strcmp(frontend_menu_style(), "xbox")) return FALSE;
	name = tag_get_name(list->definition_tag_index);
	if (!name || strcmp(name, "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\advanced_controls\\advanced_controls_menu")) return FALSE;
	name = list->focused_child ? tag_get_name(list->focused_child->definition_tag_index) : NULL;
	added = name && !strncmp(name, "pc\\frontend_legacy\\", 19);
	if (list->parameters.list.extended_description) list->parameters.list.extended_description->visible = !added;
	return added;
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

static void settings_help(struct widget_instance *list);
static void master_servers_show(struct widget_instance *list);

void frontend_menu_session_reset(void)
{
	mcc_xbox_reset();
	if (frontend_session.owns_profile)
		player_ui_end_editing_profile();
	memset(&frontend_session, 0, sizeof(frontend_session));
}

static short frontend_page(struct widget_instance *root)
{
	char const *path = root ? tag_get_name(root->definition_tag_index) : NULL;
	short page;
	for (page = 0; path && page < NUMBEROF(frontend_pages); page++)
		if (!strcmp(path, frontend_pages[page].path) || !strcmp(path, frontend_pages[page].paused)) return page;
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
	*deleted = ui_widget_load_by_name_or_tag(main_menu_is_active() ? frontend_pages[page].path : frontend_pages[page].paused, NONE, NULL,
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
	else if (!strcmp(key, "row_mcc_downloads")) target = "network_setup\\mcc_downloads\\screen";
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
	page = frontend_page(root);
	if (page == NONE) return FALSE;
	if (!strcmp(name, "port frontend init"))
	{
		/* The pause entry already owns a controller-specific profile session.
		 * Adopt it instead of beginning another (especially on MCC). */
		if (!main_menu_is_active() && player_ui_get_edit_player_profile())
			frontend_session.owns_profile = TRUE;
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
	if (main_menu_is_active()) main_goto_main_menu();
	else
	{
		ui_widget_port_go_back(root);
		*deleted = TRUE;
	}
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
	struct widget_instance *keyboard = named(list, "row_detected_button_style", 0);
	struct widget_instance *bumper = frontend_setting_row(list, "display.hud_bumper_style");
	struct widget_instance *shown, *row;
	struct controller_device device;
	boolean enabled, detect;
	short family = 0, index, port;
	int active;
	if (!hud || !automatic) return;
	enabled = !strcmp(frontend_row_value(hud), "true");
	detect = !strcmp(frontend_row_value(automatic), "true");
	active = frontend_input_device();
	port = active >= 0 && active < 4 ? (short)active : controller_of(list);
	controller_device_get(port, &device);
	if (detect && device.family >= 0 && device.family <= 3) family = (short)device.family;
	shown = frontend_setting_row(list, styles[family]);
	if (detect && active == -1 && keyboard) shown = keyboard;
	automatic->visible = enabled;
	if (keyboard) keyboard->visible = enabled && shown == keyboard;
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
	struct widget_instance *third = frontend_setting_row(list, "display.third_person");
	struct widget_instance *row, *child;
	struct pc_menu_setting *setting;
	short index;
	if (resolution && window)
	{
		shown = !strcmp(frontend_row_value(frontend_setting_row(list, "display.mode")), "windowed") ? window : resolution;
		resolution->visible = shown == resolution;
		window->visible = shown == window;
		if (list->focused_child == (shown == resolution ? window : resolution)) list->focused_child = shown;
	}
	frontend_button_rows_show(list);
	if (third)
		for (row = list->child; row; row = row->next)
			for (child = row->child; child; child = child->next)
			{
				setting = child->type == 2 ? pc_menu_setting_get(child->definition_tag_index) : NULL;
				if (!setting || strncmp(setting->setting, "display.third_person_", 21)) continue;
				row->visible = !strcmp(frontend_row_value(third), "true");
				if (!row->visible && list->focused_child == row) list->focused_child = third;
			}
	for (row = list->child, index = 0; row; row = row->next, index++)
		if (row == list->focused_child) list->parameters.list.selected_index = index;
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
	if (!main_menu_is_active())
		for (row = list->child; row; row = row->next)
		{
			/* Retain the native in-game Settings restrictions: no profile
			 * replacement or connection changes while a game is running. */
			if (!strncmp(row->name, "row_", 4) && (!strcmp(row->name, "row_profiles") || !strcmp(row->name, "row_name") ||
				!strcmp(row->name, "row_color") || (page == 3 &&
				strncmp(row->name, "row_link_", 9)) || (page == 3 && !strcmp(row->name, "row_link_11"))))
				row->visible = FALSE;
		}
	if (list->focused_child && !list->focused_child->visible)
		for (row = list->child; row; row = row->next)
			if (row->visible && !row->disabled) { list->focused_child = row; break; }
	for (row = list->child, index = 0; row; row = row->next, index++)
		if (row == list->focused_child) list->parameters.list.selected_index = index;
	index = 0;
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
