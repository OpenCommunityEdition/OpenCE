/* Fixed menu actions are separate from the gameplay binding lookup in
 * halo_keyboard.h. A remapped Jump key must never relabel menu Select. */
#include "cseries.h"
#include "frontend_menu.h"
#include "frontend_prompts.h"
#include "frontend_input.h"
#include "keyboard_prompts.h"
#include "keyboard_glyphs.h"
#include "tag_files/tag_files.h"
#include "rasterizer/rasterizer.h"
#include "text/draw_string.h"
#include <string.h>

void halo_input_shown_name(char const *binding, char *shown, unsigned int size);

int frontend_prompt_button(int action)
{
    return action == FRONTEND_SELECT ? 0 : action == FRONTEND_CANCEL ? 1 : action == FRONTEND_STYLE ? 3 : -1;
}

char const *frontend_prompt_key(int action)
{
    return action == FRONTEND_SELECT ? "Return" : action == FRONTEND_CANCEL ? "Escape" : action == FRONTEND_STYLE ? "Tab" : "";
}

static void frontend_prompt_text(rectangle2d *cursor, char const *text)
{
    wchar_t wide[160];
    rectangle2d ink, end;
    unsigned int i;
    for (i = 0; text[i] && i < NUMBEROF(wide) - 1; i++) wide[i] = (unsigned char)text[i];
    wide[i] = 0;
    rasterizer_draw_unicode_string(cursor, cursor, NULL, 0, wide);
    draw_unicode_string_compute_bounds(cursor, wide, &ink, &end);
    cursor->x0 = (short)(ink.x1 + 8);
}

static void frontend_prompt_action(rectangle2d *cursor, int device, int action, char const *caption, pixel32 color)
{
    char key[64];
    if (keyboard_prompt_menu_enabled(device))
    {
        halo_input_shown_name(frontend_prompt_key(action), key, sizeof(key));
        keyboard_glyph_draw(key, cursor, color, TRUE);
    }
    else if (device >= 0)
    {
        if (!frontend_glyph_draw((short)device, (short)frontend_prompt_button(action), cursor, color))
            frontend_prompt_text(cursor, action == FRONTEND_SELECT ? "[South]" : action == FRONTEND_CANCEL ? "[East]" : "[North]");
    }
    else if (device == -1)
    {
        halo_input_shown_name(frontend_prompt_key(action), key, sizeof(key));
        frontend_prompt_text(cursor, key);
    }
    frontend_prompt_text(cursor, caption);
}

void frontend_prompts_draw(long tag, rectangle2d const *bounds)
{
    char const *name;
    rectangle2d cursor;
    real_argb_color color;
    pixel32 packed;
    int device, navigation;
    if (!frontend_menu_available()) return;
    name = tag_get_name(tag);
    if (!name || strncmp(name, "pc\\modern\\", 10)) return;
    navigation = !strncmp(name + 10, "multiplayer\\", 12) || !strncmp(name + 10, "host\\", 5) || !strncmp(name + 10, "network\\", 8);
    name = strrchr(name, '\\') + 1;
    if (strcmp(name, "prompts") && strcmp(name, "entry_button")) return;
    device = frontend_input_device();
    cursor = *bounds;
    draw_string_get_color(&color);
    packed = ((unsigned long)(color.alpha * 255.0f) << 24) | 0xffffffUL;
    /* Both generated text widgets use left justification and the small font. */
    if (!strcmp(name, "entry_button"))
        frontend_prompt_action(&cursor, device, FRONTEND_STYLE, "MENU STYLE", packed);
    else if (device == -2 && !keyboard_prompt_menu_enabled(device))
        frontend_prompt_text(&cursor, navigation ? "Tap an option to select it." : "Tap arrows to change a value. Tap OK or CANCEL.");
    else
    {
        if (keyboard_prompt_menu_enabled(device))
        {
            keyboard_glyph_draw("Up", &cursor, packed, TRUE);
            keyboard_glyph_draw("Down", &cursor, packed, TRUE);
            keyboard_glyph_draw("Left", &cursor, packed, TRUE);
            keyboard_glyph_draw("Right", &cursor, packed, TRUE);
            frontend_prompt_text(&cursor, "MOVE");
        }
        else frontend_prompt_text(&cursor, device >= 0 ? "D-pad: MOVE" : "Arrows: MOVE");
        cursor.x0 = (short)(bounds->x0 + 180);
        frontend_prompt_action(&cursor, device, FRONTEND_SELECT, "SELECT", packed);
        cursor.x0 = (short)(bounds->x0 + 300);
        frontend_prompt_action(&cursor, device, FRONTEND_CANCEL, navigation ? "BACK" : "CANCEL", packed);
        if (!navigation)
        {
            cursor.x0 = (short)(bounds->x0 + 440);
            frontend_prompt_action(&cursor, device, FRONTEND_STYLE, "TABS", packed);
        }
    }
}
