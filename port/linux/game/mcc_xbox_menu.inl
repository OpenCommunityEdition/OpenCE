/* Included beside the menu instance helpers. Only the dedicated MCC browser
 * owns this state; native Xbox/CE lists and their generated indices stay intact. */
#define MCC_XBOX_ROOT "pc\\mcc_xbox\\"
#define MCC_XBOX_NATIVE "ui\\shell\\main_menu\\multiplayer_type_select\\"
#include "mcc_progress.h"

static struct mcc_map_browser mcc_xbox_browser;
static boolean mcc_xbox_difficulty;
static short mcc_xbox_slot;
static long mcc_xbox_profile;

static void mcc_xbox_reset(void)
{
    memset(&mcc_xbox_browser, 0, sizeof(mcc_xbox_browser));
    mcc_xbox_difficulty = FALSE;
    mcc_xbox_slot = 0;
    mcc_xbox_profile = NONE;
}

static char const *mcc_xbox_route(char const *name)
{
    if (!strcmp(name, FRONTEND_MENU_XBOX))
        return config_boolean("game.mcc_campaigns") ? "campaign" : NULL;
    if (!strcmp(name, MCC_XBOX_NATIVE "connected\\connected_map_select_wrapper")) return "connected";
    if (!strcmp(name, MCC_XBOX_NATIVE "split_screen\\splitscreen_map_select_wrapper")) return "split";
    if (!strcmp(name, MCC_XBOX_NATIVE "connected\\connected_map_select_postgame_wrapper")) return "connected_postgame";
    if (!strcmp(name, MCC_XBOX_NATIVE "split_screen\\splitscreen_map_select_postgame_wrapper")) return "split_postgame";
    return NULL;
}

static boolean mcc_xbox_available(void)
{
    return frontend_menu_available() && main_menu_is_active() && !strcmp(frontend_menu_style(), "xbox");
}

static void mcc_xbox_attach(struct widget_instance *widget)
{
    struct widget_instance *entry, *last, *list, *before, *row;
    char const *route;
    if (!widget || widget->parent || !mcc_xbox_available()) return;
    route = mcc_xbox_route(tag_get_name(widget->definition_tag_index));
    if (!route) return;
    if (!strcmp(route, "campaign"))
    {
        list = named(widget, "main_menu_select_list", 0);
        before = named(list, "multiplayer_item", 0);
        if (!list || !before || named(list, "campaign_entry", 0)) return;
        entry = ui_widget_load_by_name_or_tag(MCC_XBOX_ROOT "campaign_entry",
            NONE, list, widget->local_player_index, NONE, NONE, NONE);
        if (!entry) return;
        /* Insert only a live MCC row. Native definitions and actions remain
         * intact; subsequent rows retain the main menu's 36-pixel spacing. */
        entry->horizontal_offset = before->horizontal_offset;
        entry->vertical_offset = before->vertical_offset;
        entry->previous = before->previous;
        entry->next = before;
        if (before->previous) before->previous->next = entry;
        else list->child = entry;
        before->previous = entry;
        for (row = before; row; row = row->next) row->vertical_offset += 36;
        return;
    }
    entry = ui_widget_load_by_name_or_tag(MCC_XBOX_ROOT "multiplayer_entry",
        NONE, widget, widget->local_player_index, NONE, NONE, NONE);
    if (!entry) return;
    last = widget->child;
    while (last && last->next) last = last->next;
    entry->previous = last;
    if (last) last->next = entry;
    else widget->child = entry;
}

static boolean mcc_xbox_open(struct widget_instance *widget, boolean *deleted)
{
    struct widget_instance *root = screen_of(widget);
    char const *route;
    char path[64];
    if (!mcc_xbox_available()) return FALSE;
    route = mcc_xbox_route(tag_get_name(root->definition_tag_index));
    if (!route) return FALSE;
    snprintf(path, sizeof(path), MCC_XBOX_ROOT "%s", !strcmp(route, "campaign") ? "profiles" : route);
    return ui_widget_port_open(widget, path, deleted);
}

static boolean mcc_xbox_shortcut(struct widget_instance *widget, struct event_record *event)
{
    boolean deleted = FALSE;
    if (!widget || widget->parent || event->type != 3 || event->data.button.index != BUTTON_X ||
        event->data.button.value != 1) return FALSE;
    if (!strcmp(tag_get_name(widget->definition_tag_index), FRONTEND_MENU_XBOX)) return FALSE;
    return mcc_xbox_open(widget, &deleted);
}

static void mcc_xbox_show(struct widget_instance *root)
{
    struct widget_instance *card, *picture, *badge, *difficulty = named(root, "difficulty_list", 0);
    short slot, row, index, marker, first = mcc_xbox_browser.chosen - mcc_xbox_slot;
    unsigned completed;
    wchar_t text[128];
    char name[32];
    visible_set(named(root, "heading_campaigns", 0), !mcc_xbox_difficulty && mcc_xbox_browser.campaign && !mcc_xbox_browser.package[0]);
    visible_set(named(root, "heading_levels", 0), !mcc_xbox_difficulty && mcc_xbox_browser.campaign && mcc_xbox_browser.package[0]);
    visible_set(named(root, "heading_maps", 0), !mcc_xbox_difficulty && !mcc_xbox_browser.campaign);
    visible_set(named(root, "heading_difficulty", 0), mcc_xbox_difficulty);
    visible_set(named(root, "previous", 0), !mcc_xbox_difficulty && mcc_xbox_browser.count > 1);
    visible_set(named(root, "next", 0), !mcc_xbox_difficulty && mcc_xbox_browser.count > 1);
    visible_set(difficulty, mcc_xbox_difficulty);
    root->focused_child = mcc_xbox_difficulty ? difficulty : NULL;
    for (slot = 0; slot < 3; slot++)
    {
        snprintf(name, sizeof(name), "card_%d", slot);
        card = named(root, name, 0);
        if (!card) continue;
        row = first + slot;
        card->visible = row < mcc_xbox_browser.count && !mcc_xbox_difficulty;
        if (!card->visible) continue;
        card->animation.current_frame_index = row == mcc_xbox_browser.chosen ? 1 : 0;
        index = mcc_map_browser_index(&mcc_xbox_browser, row);
        picture = named(card, "picture", 0);
        if (picture) picture->animation.current_frame_index = index == NONE ? 13 : MCC_MAPS_FIRST_DISPLAY_INDEX + index;
        mcc_map_browser_text(&mcc_xbox_browser, row, text);
        text_set(named(card, "mcc_folder_name", 0), text);
        text_set_length(named(card, "mcc_folder_help", 0),
            index == NONE ? (mcc_xbox_browser.campaign ? L"Open folder to choose a mission." :
                L"Open folder to choose a map.") : mcc_maps_description(index), 128);
        completed = index == NONE || !mcc_xbox_browser.campaign ? 0 :
            mcc_progress_flags(mcc_xbox_profile, mcc_maps_level_name(index));
        for (marker = 0; marker < 3; marker++)
        {
            snprintf(name, sizeof(name), "badge_%d", marker);
            badge = named(card, name, 0);
            if (badge)
            {
                badge->visible = (completed & (1u << (marker + 1))) != 0;
                badge->animation.current_frame_index = marker + 1;
            }
        }
    }
    if (!mcc_xbox_browser.count)
        text_set_length(named(root, "status", 0), L"No MCC maps found. Check MCC Packages in Settings.", 128);
    else if (mcc_xbox_difficulty)
        text_set_length(named(root, "status", 0), L"", 128);
    else
    {
        usnprintf(text, NUMBEROF(text), L"%d / %d", mcc_xbox_browser.chosen + 1, mcc_xbox_browser.count);
        text_set_length(named(root, "status", 0), text, 128);
    }
}

static boolean mcc_xbox_event(char const *action, struct widget_instance *widget,
    struct event_record *event, boolean *deleted)
{
    struct widget_instance *root = screen_of(widget);
    char const *name = tag_get_name(root->definition_tag_index), *route, *level, *destination;
    short row, index;
    struct player_profile profile;
    if (!strcmp(action, "open")) return mcc_xbox_open(widget, deleted);
    if (!mcc_xbox_available() || strncmp(name, MCC_XBOX_ROOT, sizeof(MCC_XBOX_ROOT) - 1)) return FALSE;
    route = name + sizeof(MCC_XBOX_ROOT) - 1;
    if (!strcmp(action, "profile"))
    {
        if (strcmp(route, "profiles") || !game_function("sp set p1 controller choice", root, event, deleted) ||
            !game_function("3wide plyr prof set for game", root, event, deleted)) return FALSE;
        return ui_widget_port_open(root, MCC_XBOX_ROOT "campaign", deleted);
    }
    if (!strcmp(action, "init"))
    {
        mcc_maps_rescan();
        mcc_xbox_reset();
        mcc_map_browser_open(&mcc_xbox_browser, !strcmp(route, "campaign"));
        mcc_xbox_profile = player_ui_get_active_player_profile_index(0);
    }
    else if (!strcmp(action, "back"))
    {
        if (mcc_xbox_difficulty) mcc_xbox_difficulty = FALSE;
        else if (!mcc_map_browser_back(&mcc_xbox_browser))
        {
            ui_widget_port_go_back(root);
            *deleted = TRUE;
            return TRUE;
        }
        ui_play_audio_feedback_sound(SOUND_BACK);
        mcc_xbox_slot = (short)MIN(mcc_xbox_browser.chosen, 2);
    }
    else if (!strcmp(action, "left") || !strcmp(action, "right"))
    {
        short direction = !strcmp(action, "left") ? -1 : 1;
        if (!mcc_xbox_difficulty && mcc_xbox_browser.count)
        {
            row = mcc_xbox_browser.chosen + direction;
            if (row < 0) { row = mcc_xbox_browser.count - 1; mcc_xbox_slot = (short)MIN(row, 2); }
            else if (row >= mcc_xbox_browser.count) { row = 0; mcc_xbox_slot = 0; }
            else mcc_xbox_slot = (short)PIN(mcc_xbox_slot + direction, 0, 2);
            mcc_map_browser_select(&mcc_xbox_browser, row);
        }
    }
    else if (!strcmp(action, "choose") || !strcmp(action, "card"))
    {
        if (!strcmp(action, "card") && !mcc_xbox_difficulty)
        {
            name = tag_get_name(widget->definition_tag_index);
            index = (short)(name[strlen(name) - 1] - '0');
            if (index < 0 || index > 2) return FALSE;
            row = mcc_xbox_browser.chosen - mcc_xbox_slot + index;
            if (row < 0 || row >= mcc_xbox_browser.count) return FALSE;
            mcc_map_browser_select(&mcc_xbox_browser, row);
            mcc_xbox_slot = index;
        }
        if (!mcc_xbox_difficulty && mcc_map_browser_enter(&mcc_xbox_browser, mcc_xbox_browser.chosen))
        {
            mcc_xbox_slot = 0;
            ui_play_audio_feedback_sound(SOUND_FORWARD);
        }
        else
        {
            index = mcc_map_browser_index(&mcc_xbox_browser, mcc_xbox_browser.chosen);
            level = mcc_maps_level_name(index);
            if (!level) return campaign_fail();
            if (mcc_xbox_browser.campaign)
            {
                if (!mcc_xbox_difficulty) mcc_xbox_difficulty = TRUE;
                else
                {
                    if (mcc_xbox_profile == NONE || player_ui_get_active_player_profile_index(0) != mcc_xbox_profile ||
                        !player_profile_get(mcc_xbox_profile, &profile))
                    {
                        text_set_length(named(root, "status", 0), L"Create a player profile in Campaign before starting a mission.", 128);
                        return campaign_fail();
                    }
                    row = ui_widget_port_list_index(named(root, "difficulty_list", 0));
                    if (row < 0 || row >= NUMBER_OF_GAME_DIFFICULTY_LEVELS) return campaign_fail();
                    campaign_start(level, row, event->controller_index);
                    return TRUE;
                }
            }
            else
            {
                if (!ui_widget_port_mcc_multiplayer_map_choose(level)) return campaign_fail();
                destination = !strncmp(route, "connected", 9) ? MCC_XBOX_NATIVE "connected\\gametype_select_screen_wrapper" :
                    MCC_XBOX_NATIVE "split_screen\\gametype_select_screen_wrapper";
                return ui_widget_port_open(root, destination, deleted);
            }
        }
    }
    mcc_xbox_show(root);
    return TRUE;
}
