/* Original geometric keycaps, matching the approved GulchCE prompt mock-up.
 * Text uses the active project font; no generated mock-up pixels are shipped. */
#include "cseries.h"
#include "keyboard_glyphs.h"
#include "keyboard_prompts.h"
#include "halo_keyboard.h"
#include "game/players.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "rasterizer/rasterizer.h"
#include <string.h>
#include <wchar.h>

static void keyboard_glyph_draw_padded(char const *label, rectangle2d *cursor, pixel32 color, int menu, real padding, real text_scale)
{
    wchar_t text[64];
    rectangle2d measure = *cursor;
    real_rectangle2d ink, capitals;
    real_argb_color old_color, text_color;
    unsigned int i;
    short initial_indent, paragraph_indent;
    real width, height, x, y, middle;
    int symbol = !strcmp(label,"Up") ? 2 : !strcmp(label,"Down") ? 3 :
        !strcmp(label,"Left") ? 4 : !strcmp(label,"Right") ? 5 :
        !strcmp(label,"Mouse") ? 6 : !strcmp(label,"Mouse Left") ? 7 :
        !strcmp(label,"Mouse Right") ? 8 : !strcmp(label,"Mouse Middle") ? 9 :
        !strcmp(label,"Wheel Up") ? 10 : !strcmp(label,"Wheel Down") ? 11 :
        !strcmp(label,"Wheel") ? 12 : 0;
    pixel32 alpha = color & 0xff000000UL;
    pixel32 edge = alpha | (menu ? 0x2896ffUL : 0xa8d9ffUL);
    if (!strcmp(label, "Return")) label = "Enter";
    else if (!strcmp(label, "Escape")) label = "Esc";
    else if (!strcmp(label, "Left Shift")) label = "Shift";
    else if (!strcmp(label, "Left Ctrl")) label = "Ctrl";
    else if (!strcmp(label, "Left Alt")) label = "Alt";
    for (i = 0; label[i] && i < NUMBEROF(text) - 1; i++) text[i] = (unsigned char)label[i];
    text[i] = 0;
    draw_string_get_color(&old_color);
    text_color = old_color;
    text_color.alpha = (real)(alpha >> 24) / 255.0f;
    text_color.red = 0.92f; text_color.green = 0.97f; text_color.blue = 1.0f;
    draw_string_set_color(&text_color);
    draw_string_get_indents(&initial_indent, &paragraph_indent);
    draw_string_set_indents(0, 0);
    measure.x0 = 0; measure.x1 = 2048;
    rasterizer_unicode_string_ink_bounds(&measure, L"H", &capitals);
    rasterizer_unicode_string_ink_bounds(&measure, text, &ink);
    middle = (capitals.y0 + capitals.y1) * .5f;
    height = MAX(20.0f, capitals.y1 - capitals.y0 + 10.0f);
    width = symbol ? height : MAX(height, (ink.x1 - ink.x0) * text_scale + padding);
    /* HUD text retracts its cursor by three units after each text segment.
     * Recenter within that existing advance so gaps on either side match. */
    x = cursor->x0 + (menu ? 4.0f : 7.0f);
    y = middle - height * .5f;
    if (!keyboard_glyph_background(symbol, x, y, width, height, edge))
    {
        symbol = 0;
        width = MAX(height, (ink.x1 - ink.x0) * text_scale + padding);
    }
    if (!symbol)
    {
        if (text_scale != 1.0f)
            rasterizer_draw_unicode_string_scaled_offset(&measure, text, text_scale,
                x + (width - (ink.x1 - ink.x0) * text_scale) * .5f - ink.x0 * text_scale,
                middle - (ink.y0 + ink.y1) * .5f * text_scale);
        else
            rasterizer_draw_unicode_string_offset(&measure, text,
                x + (width - (ink.x1 - ink.x0)) * .5f - ink.x0,
                middle - (ink.y0 + ink.y1) * .5f);
    }
    draw_string_set_color(&old_color);
    draw_string_set_indents(initial_indent, paragraph_indent);
    cursor->x0 = (short)(x + width + (menu ? 4.5f : 1.5f));
}

/* Preserve GulchCE's original dimensions for gameplay and ordinary menus. */
void keyboard_glyph_draw(char const *label, rectangle2d *cursor, pixel32 color, int menu)
{
    keyboard_glyph_draw_padded(label, cursor, color, menu, 12.0f, 1.0f);
}

/* Smaller pause labels leave padding inside the complete, full-height outline. */
void keyboard_glyph_draw_compact(char const *label, rectangle2d *cursor, pixel32 color)
{
    keyboard_glyph_draw_padded(label, cursor, color, TRUE,
        KEYBOARD_GLYPH_COMPACT_PADDING, KEYBOARD_GLYPH_COMPACT_TEXT_SCALE);
}

/* HUD message element semantics precede controller/profile translation.
 * Authored custom icons and ambiguous physical-button literals stay native. */
static int keyboard_hud_action(short icon)
{
    static int const actions[] = {
        HALO_KEYBOARD_ACTION, HALO_KEYBOARD_THROW_GRENADE, HALO_KEYBOARD_FIRE,
        HALO_KEYBOARD_FLASHLIGHT, HALO_KEYBOARD_JUMP, HALO_KEYBOARD_MELEE,
        HALO_KEYBOARD_SWITCH_WEAPON, HALO_KEYBOARD_SWITCH_GRENADE,
        HALO_KEYBOARD_CROUCH, HALO_KEYBOARD_ZOOM
    };
    if (icon >= 18 && icon < 28) return actions[icon - 18];
    if (icon == 12) return HALO_KEYBOARD_PAUSE;
    if (icon == 13) return HALO_KEYBOARD_SCOREBOARD;
    return -1;
}

int keyboard_hud_prompt(short local_player, short icon, rectangle2d *cursor, pixel32 color)
{
    long player;
    short port;
    int action;
    char label[64];
    if (local_player < 0 || local_player >= 4) return FALSE;
    player = local_player_get_player_index(local_player);
    if (player == NONE) return FALSE;
    port = player_get(player)->local_player_index;
    if (!keyboard_prompt_enabled(port)) return FALSE;
    if (icon == 28 || icon == 29 || icon == 31)
    {
        keyboard_glyph_draw(icon == 28 ? "Enter" : icon == 29 ? "Esc" : "Mouse", cursor, color, FALSE);
        return TRUE;
    }
    if (icon == 30)
    {
        int direction;
        for (direction = HALO_KEYBOARD_MOVE_FORWARD; direction <= HALO_KEYBOARD_STRAFE_RIGHT; direction++)
        {
            if (!halo_keyboard_binding_label(direction, 0, label, sizeof(label)) &&
                !halo_keyboard_binding_label(direction, 1, label, sizeof(label))) strcpy(label, "Unbound");
            keyboard_glyph_draw(label, cursor, color, FALSE);
        }
        return TRUE;
    }
    action = keyboard_hud_action(icon);
    if (action < 0) return FALSE;
    if (!halo_keyboard_binding_label(action, 0, label, sizeof(label)) &&
        !halo_keyboard_binding_label(action, 1, label, sizeof(label)))
        strcpy(label, "Unbound");
    keyboard_glyph_draw(label, cursor, color, FALSE);
    return TRUE;
}

/* Inline HUD strings use the same semantic icon vocabulary as HUD tag
 * elements. Menu callers do not use this hook. The viewport owns the player. */
int keyboard_hud_inline(short icon, rectangle2d *cursor)
{
    real_argb_color color;
    draw_string_get_color(&color);
    return keyboard_hud_prompt(render.local_player_index, icon, cursor,
        ((unsigned long)(color.alpha * 255.0f) << 24) | 0xffffffUL);
}

/* The score hint is authored as plain BACK text (PC maps may use %s).
 * Only its identified game-engine message is offered here. Preserve the
 * surrounding localized text; unknown wording stays native. */
void keyboard_score_hint(long player, wchar_t *text, long capacity)
{
    wchar_t *at;
    wchar_t const *token = L"%back-button";
    size_t old_length, tail, replacement = wcslen(token), length;
    if (player == NONE || !keyboard_prompt_enabled(player_get(player)->local_player_index)) return;
    at = wcsstr(text, L"BACK"); old_length = 4;
    if (!at) { at = wcsstr(text, L"%s"); old_length = 2; }
    if (!at) return;
    length = wcslen(text);
    if (capacity <= 0 || length + replacement - old_length >= (size_t)capacity) return;
    tail = wcslen(at + old_length) + 1;
    memmove(at + replacement, at + old_length, tail * sizeof(wchar_t));
    memcpy(at, token, replacement * sizeof(wchar_t));
}
