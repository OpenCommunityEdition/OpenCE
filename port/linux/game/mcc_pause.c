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
enum { SOLO, COOP_HOST, COOP_CLIENT, MP_LOCAL, MP_HOST, MP_CLIENT, MODE_COUNT };
enum { LABEL_RESUME, LABEL_REVERT, LABEL_SAVE_QUIT, LABEL_LEAVE, LABEL_SETTINGS,
    LABEL_END, LABEL_OBJECTIVES, LABEL_CONFIRM_REVERT, LABEL_CONFIRM_RESTART,
    LABEL_CONFIRM_SAVE_QUIT, LABEL_ATTENTION, LABEL_RESTART_LEVEL, LABEL_FOOTER,
    LABEL_SELECT, LABEL_ACCEPT, LABEL_SAVING, LABEL_FRIENDLY_ON, LABEL_FRIENDLY_OFF, LABEL_COUNT };

enum { CONFIRM_REVERT, CONFIRM_RESTART, CONFIRM_SAVE_QUIT, CONFIRM_COUNT };
static char const *const mcc_pause_labels[LABEL_COUNT] = {
    "RESUME GAME", "REVERT TO SAVED", "SAVE AND QUIT", "LEAVE GAME", "SETTINGS",
    "END GAME", "MISSION OBJECTIVES:",
    "All progress since the\r\nlast checkpoint will\r\nbe lost.",
    "All progress on this \r\nlevel will be lost.",
    "Saving last checkpoint...\r\nplease don't turn off\r\nyour Xbox console.",
    "ATTENTION", "RESTART LEVEL", "=CANCEL", "=SELECT", "=OK", "SAVING...",
    "FRIENDLY FIRE ON", "FRIENDLY FIRE OFF"
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

/* Borrow only the port's redraws through independent bitmap groups. The
 * stock three-piece layout tiles the middle and overlaps the right cap. */
static long mcc_pause_panel(short layout, short width, short height, enum mcc_pause_art_kind kind)
{
    long panel = mcc_pause_widget("panel", 0, width, height, layout, NONE);
    if ((kind == MCC_PAUSE_ART_FOOTER_PANEL || kind == MCC_PAUSE_ART_OBJECTIVES_PANEL) &&
        (height == 194 || height == 229 || height == 195 || height == 225)) {
        long pieces[3];
        short x[3] = {0, 16, 0}, y[3] = {0}, i;
        enum mcc_pause_art_kind first = kind == MCC_PAUSE_ART_OBJECTIVES_PANEL ?
            MCC_PAUSE_ART_OBJECTIVES_LEFT : MCC_PAUSE_ART_FOOTER_LEFT;
        x[2] = width - 16;
        for (i = 0; i < 3; i++) {
            short piece_width = i == 1 ? width - 24 : 16;
            pieces[i] = mcc_pause_widget("panel_piece", 0, piece_width, height, layout, NONE);
            mcc_pause_background(pieces[i], piece_width, height, first + i);
        }
        mcc_pause_children(panel, pieces, x, y, 3);
    } else mcc_pause_background(panel, width, height, kind);
    return panel;
}

static long mcc_pause_key(short layout, boolean confirm)
{
    long key = mcc_pause_widget("button_key", 0, 200, 22, layout, NONE), children[4];
    short x[4] = {0, 16, 90, 107}, y[4] = {0}, i;
    if (confirm) { x[2] = 88; x[3] = 103; }
    for (i = 0; i < 4; i++) {
        struct mcc_pause_widget *widget;
        children[i] = mcc_pause_widget("key_item", i % 2 ? 1 : 0,
            i % 2 ? (i == 1 ? 63 : (confirm ? 60 : 93)) : 16, 20, layout,
            i == 1 ? LABEL_FOOTER : i == 3 ? (confirm ? LABEL_ACCEPT : LABEL_SELECT) : NONE);
        widget = mcc_pause_definition(children[i]);
        if (widget && i % 2) widget->font.index = mcc_pause.assets.small_font[layout];
        if (!(i % 2)) mcc_pause_background(children[i], 16, 20,
            i == 0 ? MCC_PAUSE_ART_BUTTON_B : MCC_PAUSE_ART_BUTTON_A);
    }
    mcc_pause_children(key, children, x, y, 4);
    return key;
}

static long mcc_pause_dialog(short layout, boolean pauses, short kind)
{
    static short const actions[CONFIRM_COUNT] = {MCC_PAUSE_ACTION_REVERT, MCC_PAUSE_ACTION_RESTART,
        MCC_PAUSE_ACTION_QUIT};
    long *saved = &mcc_pause.confirm[layout][pauses != FALSE][kind];
    long root, children[4];
    short width = layout == MCC_PAUSE_QUARTER ? 320 : 640, height = layout == MCC_PAUSE_FULL ? 480 : 240;
    short px = (width - 256) / 2, py = layout == MCC_PAUSE_FULL ? 176 : 40;
    short xs[4] = {0}, ys[4] = {0};
    struct mcc_pause_widget *widget;
    struct mcc_pause_event *events;
    boolean saving = kind == CONFIRM_SAVE_QUIT;
    short i;
    if (*saved != NONE) return *saved;
    root = mcc_pause_widget("confirmation", 0, width, height, layout, NONE);
    widget = mcc_pause_definition(root);
    if (!widget) return NONE;
    widget->flags = 1 | (pauses ? 2 : 0);
    mcc_pause_background(root, width, height, MCC_PAUSE_ART_DIM);
    /* Root-level A/Start and B/Back match the Xbox modal. The callback owns
     * controller-local cleanup; never close another split-screen tree. */
    events = mcc_pause_allocate((saving ? 1 : 5) * sizeof(*events));
    if (!events) return NONE;
    widget->events.address = events; widget->events.count = saving ? 1 : 5;
    widget->close_ms = saving ? 3000 : 0;
    for (i = 0; i < widget->events.count; i++) {
        events[i].event = saving ? 25 : i == 0 ? 1 : i == 1 ? 13 : i == 2 ? 0 : i == 3 ? 12 : 28;
        events[i].flags = saving || i >= 2 ? 128 : 512;
        events[i].function = saving || i >= 2 ? actions[kind] : 0;
        mcc_pause_reference(&events[i].widget, 'DeLa', NONE);
        mcc_pause_reference(&events[i].sound, 'snd!', NONE);
    }
    children[0] = mcc_pause_panel(layout, 256, 160, MCC_PAUSE_ART_ALERT);
    xs[0] = px; ys[0] = py;
    children[1] = mcc_pause_widget("attention", 1, 220, 20, layout, saving ? LABEL_SAVING : LABEL_ATTENTION);
    xs[1] = px + 9; ys[1] = py + 4;
    children[2] = mcc_pause_widget("confirmation_text", 1, 230, 92, layout, LABEL_CONFIRM_REVERT + kind);
    xs[2] = px + 9; ys[2] = py + 36;
    if (!saving) {
        children[3] = mcc_pause_key(layout, TRUE); xs[3] = px + 76; ys[3] = py + 134;
        widget = mcc_pause_definition(children[3]);
        if (widget) widget->bounds.right = 163;
    }
    mcc_pause_children(root, children, xs, ys, saving ? 3 : 4);
    *saved = root;
    return root;
}

static long mcc_pause_screen(short mode, short layout)
{
    boolean campaign = mode <= COOP_CLIENT, pauses = mode == SOLO;
    boolean host = mode == COOP_HOST || mode == MP_HOST;
    long root, list, children[6], buttons[6];
    short width = layout == MCC_PAUSE_QUARTER ? 320 : 640, height = layout == MCC_PAUSE_FULL ? 480 : 240;
    short count = 0, child_count = 0, xs[6] = {0}, ys[6] = {0}, bx[6] = {0}, by[6] = {0};
    short row = campaign ? (layout == MCC_PAUSE_QUARTER ? 24 : 28) : (layout == MCC_PAUSE_FULL ? 35 : 27);
    short campaign_growth = campaign && mcc_pause.assets.settings && layout == MCC_PAUSE_FULL ?
        (mode == SOLO ? 36 : 66) : 0;
    short button_width = campaign && layout == MCC_PAUSE_QUARTER ? 264 : 202;
    short button_height = row < 28 ? row - 1 : 27, x, y, panel_height, i;
    struct mcc_pause_widget *widget;
    if (campaign_growth) row = 30;
    root = mcc_pause_widget("pause", 0, width, height, layout, NONE);
    widget = mcc_pause_definition(root); if (!widget) return NONE;
    widget->flags = 1 | (pauses ? 2 : 0);
    mcc_pause_background(root, width, height, MCC_PAUSE_ART_DIM);
    /* The action closes this controller's tree and history. Native close-all
     * flags would also dismiss other split-screen players' menus. */
    mcc_pause_events(root, MCC_PAUSE_ACTION_RESUME, NONE, 128, TRUE);
    buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_RESUME, MCC_PAUSE_ACTION_RESUME, NONE, 128);
    if (campaign_growth)
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_SETTINGS, MCC_PAUSE_ACTION_SETTINGS, NONE, 128);
    if (campaign_growth && mode != SOLO)
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_FRIENDLY_ON, MCC_PAUSE_ACTION_FRIENDLY_FIRE, NONE, 128 | 16);
    if (campaign) {
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_REVERT, 0,
            mcc_pause_dialog(layout, pauses, CONFIRM_REVERT), 8);
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_RESTART_LEVEL, 0,
            mcc_pause_dialog(layout, pauses, CONFIRM_RESTART), 8);
    }
    /* The native profile editor is 640x480. Its lower controls do not fit
     * split-screen viewports; those players use Settings from the main menu. */
    if (!campaign && mcc_pause.assets.settings && layout == MCC_PAUSE_FULL)
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_SETTINGS, MCC_PAUSE_ACTION_SETTINGS, NONE, 128);
    if (!campaign && host)
        buttons[count++] = mcc_pause_button(layout, button_width, button_height, LABEL_END, MCC_PAUSE_ACTION_END_GAME, NONE, 128);
    buttons[count++] = mcc_pause_button(layout, button_width, button_height, campaign ? LABEL_SAVE_QUIT : LABEL_LEAVE,
        campaign ? 0 : MCC_PAUSE_ACTION_QUIT,
        campaign ? mcc_pause_dialog(layout, pauses, CONFIRM_SAVE_QUIT) : NONE, campaign ? 8 : 128);
    if (campaign) {
        x = layout == MCC_PAUSE_QUARTER ? 28 : 72;
        y = layout == MCC_PAUSE_FULL ? 171 : (layout == MCC_PAUSE_HALF ? 51 : 26);
        children[child_count] = mcc_pause_panel(layout, layout == MCC_PAUSE_QUARTER ? 296 : 218,
            layout == MCC_PAUSE_QUARTER ? 228 : 159 + campaign_growth,
            layout == MCC_PAUSE_QUARTER ? MCC_PAUSE_ART_CAMPAIGN_PANEL : MCC_PAUSE_ART_FOOTER_PANEL);
        xs[child_count] = layout == MCC_PAUSE_QUARTER ? 12 : 64;
        ys[child_count++] = layout == MCC_PAUSE_FULL ? 162 : (layout == MCC_PAUSE_HALF ? 42 : 10);
        if (layout != MCC_PAUSE_QUARTER) {
            children[child_count] = mcc_pause_panel(layout, 285, 159 + campaign_growth, MCC_PAUSE_ART_OBJECTIVES_PANEL);
            xs[child_count] = 287; ys[child_count++] = y - 9;
        }
    } else {
        panel_height = layout == MCC_PAUSE_FULL ? 159 + (count - 2) * 35 : count * row + 48;
        x = layout == MCC_PAUSE_FULL ? 218 : (width - 202) / 2;
        y = layout == MCC_PAUSE_FULL ? 207 - (count - 2) * 35 / 2 : (height - panel_height) / 2 + 10;
        children[child_count] = mcc_pause_panel(layout, 226, panel_height, MCC_PAUSE_ART_FOOTER_PANEL);
        xs[child_count] = x - (layout == MCC_PAUSE_FULL ? 12 : 8);
        ys[child_count++] = y - (layout == MCC_PAUSE_FULL ? 43 : 10);
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
        long footer = mcc_pause_key(layout, FALSE);
        children[child_count] = footer;
        xs[child_count] = campaign ? (layout == MCC_PAUSE_QUARTER ? 28 : 87) : x + 17;
        ys[child_count++] = campaign ? (layout == MCC_PAUSE_FULL ? 296 + campaign_growth : layout == MCC_PAUSE_HALF ? 176 : 216) :
            y - (layout == MCC_PAUSE_FULL ? 43 : 10) + panel_height - 25;
    }
    mcc_pause_children(root, children, xs, ys, child_count);
    return root;
}

void mcc_pause_friendly_fire(boolean enabled)
{
    long i;
    for (i = 0; i < mcc_pause.count; i++) {
        struct mcc_pause_widget *widget = mcc_pause_definition(mcc_pause.tags[i].handle);
        if (widget && (widget->string_index == LABEL_FRIENDLY_ON || widget->string_index == LABEL_FRIENDLY_OFF))
            widget->string_index = enabled ? LABEL_FRIENDLY_ON : LABEL_FRIENDLY_OFF;
    }
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
    (void)teams; /* The Xbox pause menu does not vary with the team setting. */
    if (!mcc_pause.arena || mcc_pause.failed || local_count < 1 || local_count > 4) return NONE;
    layout = local_count == 1 ? MCC_PAUSE_FULL : local_count == 2 || (local_count == 3 && first_player) ? MCC_PAUSE_HALF : MCC_PAUSE_QUARTER;
    mode = campaign ? (!network ? SOLO : host ? COOP_HOST : COOP_CLIENT) :
        !network ? MP_LOCAL : host ? MP_HOST : MP_CLIENT;
    return mcc_pause.roots[mode][layout];
}
