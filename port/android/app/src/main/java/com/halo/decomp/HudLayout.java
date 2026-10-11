package com.halo.decomp;

import java.util.Locale;

/**
 * Edit in-game UI: where the player put the game's own HUD panels. Each offset
 * is the fraction of the screen its panel is moved by. It is saved in the app's
 * preferences on its own: a touch layout file never holds it.
 */
public final class HudLayout {
    public static final int WEAPONS = 0, HEALTH = 1, RADAR = 2, COUNT = 3;
    private static final String VERSION = "hud1";
    private static final String[] NAMES = {"WEAPONS", "HEALTH", "RADAR"};
    /** the game's HUD is 480 units high and as wide as the screen's shape */
    private static final float UNITS = 480f;
    /** each panel at rest in HUD units: from its side, from its top or bottom, width, height */
    private static final float[][] PANELS = {
        {16, 14, 270, 82},  // the weapons: from the left and the top
        {16, 14, 196, 70},  // the health: from the right and the top
        {16, 16, 120, 120}, // the radar: from the left and the bottom
    };
    private final float[] offsets = new float[COUNT * 2];

    public static String name(int panel) { return NAMES[panel]; }

    /** the panel's left, top, right and bottom on a screen of this size, in the size's unit */
    public float[] bounds(int panel, float width, float height) {
        float unit = height / UNITS;
        float[] rest = PANELS[panel];
        float w = rest[2] * unit, h = rest[3] * unit;
        float left = (panel == HEALTH ? width - rest[0] * unit - w : rest[0] * unit) + offsets[panel * 2] * width;
        float top = (panel == RADAR ? height - rest[1] * unit - h : rest[1] * unit) + offsets[panel * 2 + 1] * height;
        return new float[] {left, top, left + w, top + h};
    }

    /** the panel under a point (the last drawn first), or -1 */
    public int hit(float x, float y, float width, float height) {
        for (int panel = COUNT - 1; panel >= 0; panel--) {
            float[] b = bounds(panel, width, height);
            if (x >= b[0] && x < b[2] && y >= b[1] && y < b[3]) return panel;
        }
        return -1;
    }

    /** moves the panel by a distance on a screen of this size; it stays whole on the screen */
    public void move(int panel, float dx, float dy, float width, float height) {
        if (!(width > 0) || !(height > 0) || !Float.isFinite(dx) || !Float.isFinite(dy)) return;
        float[] b = bounds(panel, width, height);
        dx = Math.max(-b[0], Math.min(width - b[2], dx));
        dy = Math.max(-b[1], Math.min(height - b[3], dy));
        offsets[panel * 2] = clamp(offsets[panel * 2] + dx / width);
        offsets[panel * 2 + 1] = clamp(offsets[panel * 2 + 1] + dy / height);
    }

    public void reset() { java.util.Arrays.fill(offsets, 0); }

    /** x and y of each panel, as the game takes them */
    public float[] offsets() { return offsets.clone(); }

    public String save() {
        StringBuilder text = new StringBuilder(VERSION);
        for (int panel = 0; panel < COUNT; panel++)
            text.append(String.format(Locale.US, ";%.5f,%.5f", offsets[panel * 2], offsets[panel * 2 + 1]));
        return text.toString();
    }

    /** what save() wrote; anything else (nothing saved yet, or damaged) is every panel at rest */
    public static HudLayout load(String text) {
        HudLayout loaded = new HudLayout();
        if (text == null) return loaded;
        String[] parts = text.trim().split(";");
        if (parts.length != COUNT + 1 || !parts[0].equals(VERSION)) return loaded;
        float[] values = new float[COUNT * 2];
        try {
            for (int panel = 0; panel < COUNT; panel++) {
                String[] pair = parts[panel + 1].split(",");
                if (pair.length != 2) return loaded;
                for (int axis = 0; axis < 2; axis++) {
                    float value = Float.parseFloat(pair[axis]);
                    if (!Float.isFinite(value) || value < -1 || value > 1) return loaded;
                    values[panel * 2 + axis] = value;
                }
            }
        } catch (NumberFormatException e) {
            return loaded;
        }
        System.arraycopy(values, 0, loaded.offsets, 0, values.length);
        return loaded;
    }

    private static float clamp(float value) { return Math.max(-1, Math.min(1, value)); }
}
