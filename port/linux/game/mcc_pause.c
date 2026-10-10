/* MCC-owned native pause definitions. The layouts follow the stock Xbox
 * logical viewport sizes; no embedded map widget or CE converter is edited. */
#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "text/text_group.h"
#include "mcc_pause.h"
#include "mcc_ui.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct mcc_pause_rect { short top, left, bottom, right; };
struct mcc_pause_event {
    long flags;
    short event, function;
    struct tag_reference widget, sound;
    char script[32];
};
struct mcc_pause_child {
    struct tag_reference widget;
    char name[32];
    long flags;
    short controller, y, x;
    unsigned char padding[0x50 - 0x3A];
};
struct mcc_pause_input { short function; unsigned char padding[0x22]; };
struct mcc_pause_widget {
    short type, controller;
    char name[32];
    struct mcc_pause_rect bounds;
    long flags, close_ms, fade_ms;
    struct tag_reference background;
    struct tag_block inputs, events, replacements;
    unsigned char padding06C[0xEC - 0x6C];
    struct tag_reference text, font;
    float color[4];
    short justification;
    unsigned short text_flags;
    unsigned char padding120[0x12E - 0x120];
    short string_index, text_x, text_y;
    unsigned char padding134[0x150 - 0x134];
    long list_flags;
    struct tag_reference list_header, list_footer;
    struct mcc_pause_rect list_header_bounds, list_footer_bounds;
    unsigned char padding184[0x1A4 - 0x184];
    struct tag_reference description;
    unsigned char padding1B4[0x2D4 - 0x1B4];
    struct tag_block conditional;
    unsigned char padding2E0[0x3E0 - 0x2E0];
    struct tag_block children;
};
typedef char mcc_pause_widget_size[sizeof(struct mcc_pause_widget) == 0x3EC ? 1 : -1];
typedef char mcc_pause_event_size[sizeof(struct mcc_pause_event) == 0x48 ? 1 : -1];
typedef char mcc_pause_child_size[sizeof(struct mcc_pause_child) == 0x50 ? 1 : -1];
typedef char mcc_pause_child_offset[offsetof(struct mcc_pause_widget, children) == 0x3E0 ? 1 : -1];

enum { MCC_PAUSE_MAX_TAGS = 512, MCC_PAUSE_ARENA_BYTES = 2 * 1024 * 1024 };
enum { SOLO, COOP_HOST, COOP_CLIENT, MP_LOCAL, MP_HOST, MP_HOST_TEAMS, MP_CLIENT, MP_CLIENT_TEAMS, MODE_COUNT };
enum { LABEL_RESUME, LABEL_REVERT, LABEL_RESTART, LABEL_SAVE_QUIT, LABEL_LEAVE, LABEL_SETTINGS,
    LABEL_TEAMS, LABEL_END, LABEL_RED, LABEL_BLUE, LABEL_CANCEL, LABEL_OK, LABEL_OBJECTIVES,
    LABEL_CONFIRM_REVERT, LABEL_CONFIRM_RESTART, LABEL_CONFIRM_SAVE_QUIT, LABEL_CONFIRM_LEAVE,
    LABEL_CONFIRM_END, LABEL_ATTENTION, LABEL_TEAM_TITLE, LABEL_RESTART_LEVEL, LABEL_FOOTER, LABEL_COUNT };
enum { CONFIRM_REVERT, CONFIRM_RESTART, CONFIRM_SAVE_QUIT, CONFIRM_LEAVE, CONFIRM_END, CONFIRM_COUNT };
static char const *const mcc_pause_labels[LABEL_COUNT] = {
    "RESUME GAME", "REVERT TO SAVED", "RESTART GAME", "SAVE AND QUIT", "LEAVE GAME", "SETTINGS",
    "CHOOSE TEAM", "END GAME", "RED TEAM", "BLUE TEAM", "CANCEL", "OK", "MISSION OBJECTIVES:",
    "All progress since the last checkpoint will be lost.",
    "Restart this game? Current progress will be lost.",
    "Save your last checkpoint and return to the main menu?",
    "Are you sure you want to leave this game?",
    "End this round and return to the lobby?", "ATTENTION", "CHOOSE TEAM", "RESTART LEVEL", "%a-button SELECT   %b-button BACK"
};

static struct {
    unsigned char *arena;
    unsigned long used;
    long count, first, strings;
    unsigned short salt;
    boolean failed;
    struct mcc_pause_assets assets;
    struct mcc_pause_tag tags[MCC_PAUSE_MAX_TAGS];
    long roots[MODE_COUNT][MCC_PAUSE_LAYOUT_COUNT];
    long confirm[MCC_PAUSE_LAYOUT_COUNT][2][CONFIRM_COUNT];
    long team[MCC_PAUSE_LAYOUT_COUNT];
} mcc_pause;

static void *mcc_pause_allocate(unsigned long size)
{
    unsigned long start = (mcc_pause.used + 7u) & ~7u;
    void *result;
    if (!mcc_pause.arena || start > MCC_PAUSE_ARENA_BYTES || size > MCC_PAUSE_ARENA_BYTES - start) {
        mcc_pause.failed = TRUE;
        return NULL;
    }
    result = mcc_pause.arena + start;
    mcc_pause.used = start + size;
    memset(result, 0, size);
    return result;
}

static void mcc_pause_reference(struct tag_reference *reference, long group, long handle)
{
    reference->group_tag = group;
    reference->index = handle;
}

static long mcc_pause_add(long group, char const *stem, void *data)
{
    struct mcc_pause_tag *tag;
    char *name;
    long ordinal = mcc_pause.count;
    if (!data || ordinal >= MCC_PAUSE_MAX_TAGS || mcc_pause.first + ordinal > 0xFFFFL) {
        mcc_pause.failed = TRUE;
        return NONE;
    }
    name = mcc_pause_allocate(96);
    if (!name) return NONE;
    snprintf(name, 96, "mcc_pause\\%s_%03ld", stem, ordinal);
    tag = &mcc_pause.tags[mcc_pause.count++];
    tag->group = group;
    tag->handle = (long)(((uint32_t)(unsigned short)(mcc_pause.salt + ordinal) << 16) |
        (uint32_t)(mcc_pause.first + ordinal));
    tag->name = name;
    tag->data = data;
    return tag->handle;
}

static struct mcc_pause_widget *mcc_pause_definition(long handle)
{
    unsigned long ordinal = ((unsigned long)handle & 0xFFFFu) - (unsigned long)mcc_pause.first;
    return ordinal < (unsigned long)mcc_pause.count && mcc_pause.tags[ordinal].handle == handle &&
        mcc_pause.tags[ordinal].group == 'DeLa' ? mcc_pause.tags[ordinal].data : NULL;
}

static long mcc_pause_widget(char const *name, short type, short width, short height, short layout, short label)
{
    struct mcc_pause_widget *widget = mcc_pause_allocate(sizeof(*widget));
    if (!widget) return NONE;
    widget->type = type;
    widget->controller = 4;
    strncpy(widget->name, name, sizeof(widget->name) - 1);
    widget->bounds.right = width;
    widget->bounds.bottom = height;
    mcc_pause_reference(&widget->background, 'bitm', NONE);
    mcc_pause_reference(&widget->text, 'ustr', label == NONE ? NONE : mcc_pause.strings);
    mcc_pause_reference(&widget->font, 'font', type == 1 ? mcc_pause.assets.font[layout] : NONE);
    mcc_pause_reference(&widget->list_header, 'bitm', NONE);
    mcc_pause_reference(&widget->list_footer, 'bitm', NONE);
    mcc_pause_reference(&widget->description, 'DeLa', NONE);
    widget->string_index = label == NONE ? 0 : label;
    widget->color[0] = 1.0f;
    widget->color[1] = 0.156863f;
    widget->color[2] = 0.588235f;
    widget->color[3] = 1.0f;
    return mcc_pause_add('DeLa', name, widget);
}

static void mcc_pause_background(long handle, short width, short height, enum mcc_pause_art_kind kind)
{
    struct mcc_pause_widget *widget = mcc_pause_definition(handle);
    long bitmap;
    if (!widget || mcc_pause.failed) return;
    bitmap = mcc_pause.assets.art(width, height, kind);
    if (bitmap == NONE) { mcc_pause.failed = TRUE; return; }
    mcc_pause_reference(&widget->background, 'bitm', bitmap);
}

static void mcc_pause_events(long handle, short action, long target, long flags, boolean back)
{
    struct mcc_pause_widget *widget = mcc_pause_definition(handle);
    struct mcc_pause_event *events;
    short i, count = 2;
    if (!widget) return;
    events = mcc_pause_allocate(count * sizeof(*events));
    if (!events) return;
    widget->events.count = count;
    widget->events.address = events;
    for (i = 0; i < count; ++i) {
        events[i].event = back ? (i ? 13 : 1) : (i ? 28 : 0);
        events[i].flags = flags;
        events[i].function = action;
        mcc_pause_reference(&events[i].widget, 'DeLa', target);
        mcc_pause_reference(&events[i].sound, 'snd!', NONE);
    }
}

static void mcc_pause_children(long handle, long const *children, short const *x, short const *y, short count)
{
    struct mcc_pause_widget *widget = mcc_pause_definition(handle);
    struct mcc_pause_child *references;
    short i;
    if (!widget || count < 1) return;
    references = mcc_pause_allocate(count * sizeof(*references));
    if (!references) return;
    widget->children.count = count;
    widget->children.address = references;
    for (i = 0; i < count; ++i) {
        mcc_pause_reference(&references[i].widget, 'DeLa', children[i]);
        references[i].x = x[i];
        references[i].y = y[i];
        snprintf(references[i].name, sizeof(references[i].name), "item_%d", (int)i);
    }
}

static long mcc_pause_button(short layout, short width, short height, short label, short action, long target, long flags)
{
    long handle = mcc_pause_widget("button", 1, width, height, layout, label);
    struct mcc_pause_widget *widget = mcc_pause_definition(handle);
    if (!widget) return NONE;
    widget->justification = 2;
    widget->text_y = 3;
    if (layout == MCC_PAUSE_QUARTER)
        widget->font.index = mcc_pause.assets.small_font[layout];
    mcc_pause_background(handle, width, height, MCC_PAUSE_ART_HIGHLIGHT);
    mcc_pause_events(handle, action, target, flags, FALSE);
    return handle;
}

static long mcc_pause_panel(short layout, short width, short height)
{
    long panel = mcc_pause_widget("panel", 0, width, height, layout, NONE);
    mcc_pause_background(panel, width, height, MCC_PAUSE_ART_PANEL);
    return panel;
}

static long mcc_pause_dialog(short layout, boolean pauses, short kind)
{
    static short const actions[CONFIRM_COUNT] = {MCC_PAUSE_ACTION_REVERT, MCC_PAUSE_ACTION_RESTART,
        MCC_PAUSE_ACTION_QUIT, MCC_PAUSE_ACTION_QUIT, MCC_PAUSE_ACTION_END_GAME};
    long *saved = &mcc_pause.confirm[layout][pauses != FALSE][kind];
    long root, children[5], buttons[2], list;
    short width = layout == MCC_PAUSE_QUARTER ? 320 : 640, height = layout == MCC_PAUSE_FULL ? 480 : 240;
    short panel_width = layout == MCC_PAUSE_QUARTER ? 292 : 360, panel_height = 176;
    short px = (short)((width - panel_width) / 2), py = (short)((height - panel_height) / 2);
    short xs[5] = {0}, ys[5] = {0}, bx[2] = {0}, by[2] = {0};
    struct mcc_pause_widget *widget;
    if (*saved != NONE) return *saved;
    root = mcc_pause_widget("confirmation", 0, width, height, layout, NONE);
    widget = mcc_pause_definition(root);
    if (!widget) return NONE;
    widget->flags = 1 | (pauses ? 2 : 0);
    mcc_pause_background(root, width, height, MCC_PAUSE_ART_DIM);
    mcc_pause_events(root, 0, NONE, 512, TRUE);
    children[0] = mcc_pause_panel(layout, panel_width, panel_height);
    xs[0] = px; ys[0] = py;
    children[1] = mcc_pause_widget("attention", 1, panel_width - 24, 24, layout, LABEL_ATTENTION);
    xs[1] = px + 12; ys[1] = py + 8;
    children[2] = mcc_pause_widget("confirmation_text", 1, panel_width - 24, 84, layout, LABEL_CONFIRM_REVERT + kind);
    widget = mcc_pause_definition(children[2]);
    if (widget) widget->font.index = mcc_pause.assets.small_font[layout];
    xs[2] = px + 12; ys[2] = py + 40;
    list = mcc_pause_widget("confirmation_choices", 3, panel_width - 16, 30, layout, NONE);
    widget = mcc_pause_definition(list);
    if (widget) widget->flags = 1 | 64; /* Left/right navigation. */
    /* Cancel is first so an accidental second accept cannot destroy progress. */
    buttons[0] = mcc_pause_button(layout, (panel_width - 20) / 2, 27, LABEL_CANCEL, 0, NONE, 512);
    buttons[1] = mcc_pause_button(layout, (panel_width - 20) / 2, 27, LABEL_OK, actions[kind], NONE,
        128 | (kind <= CONFIRM_RESTART ? 4 : 0));
    bx[1] = (panel_width - 16) / 2;
    mcc_pause_children(list, buttons, bx, by, 2);
    children[3] = list; xs[3] = px + 8; ys[3] = py + 136;
    mcc_pause_children(root, children, xs, ys, 4);
    *saved = root;
    return root;
}

static long mcc_pause_team_screen(short layout)
{
    long root, children[3], buttons[3], list;
    short width = layout == MCC_PAUSE_QUARTER ? 320 : 640, height = layout == MCC_PAUSE_FULL ? 480 : 240;
    short x[3] = {0}, y[3] = {0}, bx[3] = {0}, by[3] = {0, 32, 64};
    struct mcc_pause_widget *widget;
    if (mcc_pause.team[layout] != NONE) return mcc_pause.team[layout];
    root = mcc_pause_widget("choose_team", 0, width, height, layout, NONE);
    widget = mcc_pause_definition(root);
    if (!widget) return NONE;
    widget->flags = 1;
    mcc_pause_background(root, width, height, MCC_PAUSE_ART_DIM);
    mcc_pause_events(root, 0, NONE, 512, TRUE);
    children[0] = mcc_pause_panel(layout, 226, 156); x[0] = (width - 226) / 2; y[0] = (height - 156) / 2;
    children[1] = mcc_pause_widget("choose_team_caption", 1, 202, 24, layout, LABEL_TEAM_TITLE);
    x[1] = (width - 202) / 2; y[1] = y[0] + 8;
    list = mcc_pause_widget("teams", 3, 202, 96, layout, NONE);
    widget = mcc_pause_definition(list); if (widget) widget->flags = 0x21;
    buttons[0] = mcc_pause_button(layout, 202, 27, LABEL_RED, MCC_PAUSE_ACTION_RED_TEAM, NONE, 128);
    buttons[1] = mcc_pause_button(layout, 202, 27, LABEL_BLUE, MCC_PAUSE_ACTION_BLUE_TEAM, NONE, 128);
    buttons[2] = mcc_pause_button(layout, 202, 27, LABEL_CANCEL, 0, NONE, 512);
    mcc_pause_children(list, buttons, bx, by, 3);
    children[2] = list; x[2] = (width - 202) / 2; y[2] = y[0] + 44;
    mcc_pause_children(root, children, x, y, 3);
    mcc_pause.team[layout] = root;
    return root;
}

static long mcc_pause_screen(short mode, short layout)
{
    boolean campaign = mode <= COOP_CLIENT, pauses = mode == SOLO;
    boolean host = mode == COOP_HOST || mode == MP_HOST || mode == MP_HOST_TEAMS;
    boolean teams = mode == MP_HOST_TEAMS || mode == MP_CLIENT_TEAMS;
    long root, list, children[6], buttons[6];
    short width = layout == MCC_PAUSE_QUARTER ? 320 : 640, height = layout == MCC_PAUSE_FULL ? 480 : 240;
    short count = 0, child_count = 0, xs[6] = {0}, ys[6] = {0}, bx[6] = {0}, by[6] = {0};
    short row = campaign ? (layout == MCC_PAUSE_QUARTER ? 24 : 28) : (layout == MCC_PAUSE_FULL ? 35 : 27);
    short button_width = campaign && layout == MCC_PAUSE_QUARTER ? 264 : 202;
    short button_height = row < 28 ? row - 1 : 27, x, y, panel_height, i;
    struct mcc_pause_widget *widget;
    root = mcc_pause_widget("pause", 0, width, height, layout, NONE);
    widget = mcc_pause_definition(root); if (!widget) return NONE;
    widget->flags = 1 | (pauses ? 2 : 0);
    mcc_pause_background(root, width, height, MCC_PAUSE_ART_DIM);
    /* The action closes this controller's tree and history. Native close-all
     * flags would also dismiss other split-screen players' menus. */
    mcc_pause_events(root, MCC_PAUSE_ACTION_RESUME, NONE, 128, TRUE);
    buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_RESUME, MCC_PAUSE_ACTION_RESUME, NONE, 128);
    if (campaign && mode != COOP_CLIENT) {
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_REVERT, 0,
            mcc_pause_dialog(layout, pauses, CONFIRM_REVERT), 8);
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_RESTART_LEVEL, 0,
            mcc_pause_dialog(layout, pauses, CONFIRM_RESTART), 8);
    }
    if (teams) buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_TEAMS, 0, mcc_pause_team_screen(layout), 8);
    /* The native profile editor is 640x480. Its lower controls do not fit
     * split-screen viewports; those players use Settings from the main menu. */
    if (!campaign && mcc_pause.assets.settings && layout == MCC_PAUSE_FULL)
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_SETTINGS, MCC_PAUSE_ACTION_SETTINGS, NONE, 128);
    if (!campaign && (host || mode == MP_LOCAL))
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_RESTART, 0,
            mcc_pause_dialog(layout, FALSE, CONFIRM_RESTART), 8);
    if (!campaign && host)
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_END, 0,
            mcc_pause_dialog(layout, FALSE, CONFIRM_END), 8);
    buttons[count++] = mcc_pause_button(layout, button_width, button_height, mode == SOLO ? LABEL_SAVE_QUIT : LABEL_LEAVE, 0,
        mcc_pause_dialog(layout, pauses, mode == SOLO ? CONFIRM_SAVE_QUIT : CONFIRM_LEAVE), 8);
    if (campaign) {
        x = layout == MCC_PAUSE_QUARTER ? 28 : 72;
        y = layout == MCC_PAUSE_FULL ? 171 : (layout == MCC_PAUSE_HALF ? 51 : 26);
        children[child_count] = mcc_pause_panel(layout, layout == MCC_PAUSE_QUARTER ? 296 : 218,
            layout == MCC_PAUSE_QUARTER ? 228 : 159);
        xs[child_count] = layout == MCC_PAUSE_QUARTER ? 12 : 64;
        ys[child_count++] = layout == MCC_PAUSE_FULL ? 162 : (layout == MCC_PAUSE_HALF ? 42 : 10);
        if (layout != MCC_PAUSE_QUARTER) {
            children[child_count] = mcc_pause_panel(layout, 285, 159);
            xs[child_count] = 287; ys[child_count++] = y - 9;
        }
    } else {
        panel_height = count * row + 48;
        x = (width - 202) / 2; y = (height - panel_height) / 2 + 10;
        children[child_count] = mcc_pause_panel(layout, 226, panel_height);
        xs[child_count] = x - 8; ys[child_count++] = y - 10;
    }
    list = mcc_pause_widget("pause_actions", 3, button_width, count * row, layout, NONE);
    widget = mcc_pause_definition(list); if (widget) widget->flags = 0x21;
    for (i = 0; i < count; ++i) by[i] = i * row;
    mcc_pause_children(list, buttons, bx, by, count);
    children[child_count] = list; xs[child_count] = x; ys[child_count++] = y;
    if (campaign) {
        long caption = mcc_pause_widget("objective_caption", 1, layout == MCC_PAUSE_QUARTER ? 264 : 270, 22, layout, LABEL_OBJECTIVES);
        long objective = mcc_pause_widget("objective", 1, layout == MCC_PAUSE_QUARTER ? 264 : 270,
            layout == MCC_PAUSE_QUARTER ? 56 : 94, layout, NONE);
        struct mcc_pause_input *input = mcc_pause_allocate(sizeof(*input));
        widget = mcc_pause_definition(caption);
        if (widget && layout == MCC_PAUSE_QUARTER) widget->font.index = mcc_pause.assets.small_font[layout];
        widget = mcc_pause_definition(objective);
        if (widget && input) {
            widget->font.index = mcc_pause.assets.small_font[layout];
            widget->color[1] = 0.0f; widget->color[2] = 0.5f;
            input->function = 18; /* Native objective text, driven by the scenario. */
            widget->inputs.count = 1; widget->inputs.address = input;
        }
        children[child_count] = caption; xs[child_count] = layout == MCC_PAUSE_QUARTER ? 28 : 302;
        ys[child_count++] = layout == MCC_PAUSE_FULL ? 166 : (layout == MCC_PAUSE_HALF ? 46 : 130);
        children[child_count] = objective; xs[child_count] = layout == MCC_PAUSE_QUARTER ? 28 : 302;
        ys[child_count++] = layout == MCC_PAUSE_FULL ? 195 : (layout == MCC_PAUSE_HALF ? 75 : 154);
    }
    {
        short footer_width = campaign ? (layout == MCC_PAUSE_QUARTER ? 264 : 180) : 202;
        long footer = mcc_pause_widget("button_key", 1, footer_width, 18, layout, LABEL_FOOTER);
        widget = mcc_pause_definition(footer);
        /* Native icon text advances through each token; centering the
         * individual fragments would separate the glyphs from their labels. */
        if (widget) { widget->font.index = mcc_pause.assets.small_font[layout]; widget->justification = 0; }
        children[child_count] = footer;
        xs[child_count] = campaign ? (layout == MCC_PAUSE_QUARTER ? 28 : 87) : x;
        ys[child_count++] = campaign ? (layout == MCC_PAUSE_FULL ? 296 : layout == MCC_PAUSE_HALF ? 176 : 216) :
            y - 10 + panel_height - 24;
    }
    mcc_pause_children(root, children, xs, ys, child_count);
    return root;
}

void mcc_pause_dispose(void)
{
    if (mcc_pause.arena) free(mcc_pause.arena);
    memset(&mcc_pause, 0, sizeof(mcc_pause));
}

boolean mcc_pause_build(long first_index, short first_salt, struct mcc_pause_assets const *assets)
{
    struct string_list *strings;
    struct string_list_entry *entries;
    short mode, layout, label;
    mcc_pause_dispose();
    if (!assets || !assets->art || first_index < 0 || first_index > 0xFFFFL - MCC_PAUSE_MAX_TAGS) return FALSE;
    for (layout = 0; layout < MCC_PAUSE_LAYOUT_COUNT; ++layout)
        if (assets->font[layout] == NONE || assets->small_font[layout] == NONE) return FALSE;
    mcc_pause.first = first_index; mcc_pause.salt = (unsigned short)first_salt; mcc_pause.assets = *assets;
    mcc_pause.arena = malloc(MCC_PAUSE_ARENA_BYTES);
    if (!mcc_pause.arena) return FALSE;
    memset(mcc_pause.confirm, 0xFF, sizeof(mcc_pause.confirm));
    memset(mcc_pause.team, 0xFF, sizeof(mcc_pause.team));
    strings = mcc_pause_allocate(sizeof(*strings));
    entries = mcc_pause_allocate(LABEL_COUNT * sizeof(*entries));
    if (!strings || !entries) goto failed;
    strings->strings.count = LABEL_COUNT; strings->strings.address = entries;
    for (label = 0; label < LABEL_COUNT; ++label) {
        size_t i, length = strlen(mcc_pause_labels[label]);
        unsigned short *text = mcc_pause_allocate((unsigned long)(length + 1) * 2);
        if (!text) goto failed;
        for (i = 0; i < length; ++i) text[i] = (unsigned char)mcc_pause_labels[label][i];
        entries[label].string.size = (long)(length + 1) * 2; entries[label].string.address = text;
    }
    mcc_pause.strings = mcc_pause_add('ustr', "strings", strings);
    for (mode = 0; mode < MODE_COUNT && !mcc_pause.failed; ++mode)
        for (layout = 0; layout < MCC_PAUSE_LAYOUT_COUNT && !mcc_pause.failed; ++layout)
            mcc_pause.roots[mode][layout] = mcc_pause_screen(mode, layout);
    if (mcc_pause.failed) goto failed;
    return TRUE;
failed:
    mcc_pause_dispose();
    return FALSE;
}

long mcc_pause_tag_count(void) { return mcc_pause.count; }
struct mcc_pause_tag const *mcc_pause_tag_at(long ordinal)
{
    return ordinal >= 0 && ordinal < mcc_pause.count ? &mcc_pause.tags[ordinal] : NULL;
}
boolean mcc_pause_owns(long handle) { return mcc_pause_definition(handle) != NULL; }
boolean mcc_pause_text_wrap_needed(long handle)
{
    struct mcc_pause_widget const *widget = mcc_pause_definition(handle);
    return widget && (!strcmp(widget->name, "objective") || !strcmp(widget->name, "confirmation_text"));
}
long mcc_pause_select(boolean campaign, boolean network, boolean host, boolean teams, short local_count, boolean first_player)
{
    short layout, mode;
    if (!mcc_pause.arena || mcc_pause.failed || local_count < 1 || local_count > 4) return NONE;
    layout = local_count == 1 ? MCC_PAUSE_FULL : local_count == 2 || (local_count == 3 && first_player) ? MCC_PAUSE_HALF : MCC_PAUSE_QUARTER;
    mode = campaign ? (!network ? SOLO : host ? COOP_HOST : COOP_CLIENT) :
        !network ? MP_LOCAL : host ? (teams ? MP_HOST_TEAMS : MP_HOST) : (teams ? MP_CLIENT_TEAMS : MP_CLIENT);
    return mcc_pause.roots[mode][layout];
}
