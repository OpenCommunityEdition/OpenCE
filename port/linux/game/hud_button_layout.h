#ifndef HUD_BUTTON_LAYOUT_H
#define HUD_BUTTON_LAYOUT_H

/* Half-alpha bounds in the embedded 8x artwork. Match the visible Xbox
 * A/B/X/Y face (21x26), rather than its transparent texture padding.
 * Black/White retain their existing layout. */
static void hud_button_face_bounds(short style, short index,
    real *x, real *y, real *width, real *height)
{
    static short const bounds[3][4][4] = {
        {{5,6,160,201}, {1,5,162,202}, {5,6,160,201}, {5,6,160,201}},
        {{5,8,161,197}, {5,8,161,197}, {5,8,161,197}, {5,8,161,197}},
        {{5,5,160,201}, {1,5,162,202}, {5,5,160,201}, {5,5,160,201}}
    };
    short const *face;
    real sx, sy;

    if (index < 0 || index >= 4 || style < 0 || style >= 3) return;
    face = bounds[style][index];
    sx = *width / (face[2] - face[0]);
    sy = *height / (face[3] - face[1]);
    *x -= face[0] * sx;
    *y -= face[1] * sy;
    *width = 168.0f * sx;
    *height = 208.0f * sy;
}

/* HUD controller indices 14/15 animate a stick press; 16/17 show a stick.
 * Follow the original three-frame sequence and its authored tick divisor. */
static short hud_button_control_slot(short index, long frame)
{
    if (index == 6 || index == 7) return index;
    if (index == 16 || index == 17) return index - 8;
    if (index == 14 || index == 15)
        return (short)(index - 6 + 2 * (frame < 0 ? 0 : frame % 3));
    return -1;
}

static char const *hud_button_control_name(short slot)
{
    static char const *names[] = {
        "ui\\hud\\buttons\\trigger_left", "ui\\hud\\buttons\\trigger_right",
        "ui\\hud\\buttons\\thumbstick_left", "ui\\hud\\buttons\\thumbstick_right",
        "ui\\hud\\buttons\\thumbstick-halfway_left", "ui\\hud\\buttons\\thumbstick-halfway_right",
        "ui\\hud\\buttons\\thumbstick-full_left", "ui\\hud\\buttons\\thumbstick-full_right"
    };
    return slot >= 6 && slot < 14 ? names[slot - 6] : 0;
}

/* Original combined HUD sheet: triggers have a 44x33 visible face and
 * both sticks have 32x34 in all three frames. Exclude texture padding. */
static void hud_button_control_bounds(short slot, real *x, real *y, real *width, real *height)
{
    real sx, sy;
    if (slot < 6 || slot >= 14) return;
    sx = *width / (slot < 8 ? 344.0f : 272.0f);
    sy = *height / (slot < 8 ? 264.0f : (slot & 1) ? 294.0f : 293.0f);
    *x -= (slot < 8 ? 28.0f : 8.0f) * sx;
    *y -= (slot < 8 ? 8.0f : 13.0f) * sy;
    *width = (slot < 8 ? 400.0f : 288.0f) * sx;
    *height = (slot < 8 ? 280.0f : 320.0f) * sy;
}

#endif
