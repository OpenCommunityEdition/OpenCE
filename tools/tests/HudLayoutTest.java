package com.halo.decomp;

/** Edit in-game UI: the panels' rest places, dragging, saving, and that a touch layout file never holds them. */
public final class HudLayoutTest {
    private static void check(boolean ok, String why) { if (!ok) throw new AssertionError(why); }
    private static boolean near(float a, float b) { return Math.abs(a - b) < 0.01f; }

    public static void main(String[] args) {
        // a 20:9 phone, in the game's 480 lines: 1068 wide
        float width = 1068, height = 480;
        HudLayout hud = new HudLayout();
        float[] weapons = hud.bounds(HudLayout.WEAPONS, width, height);
        float[] health = hud.bounds(HudLayout.HEALTH, width, height);
        float[] radar = hud.bounds(HudLayout.RADAR, width, height);
        check(near(weapons[0], 16) && near(weapons[1], 14), "Weapons rest at the top left");
        check(near(health[2], width - 16) && near(health[1], 14), "Health rests at the top right");
        check(near(radar[0], 16) && near(radar[3], height - 16), "Radar rests at the bottom left");
        for (float value : hud.offsets()) check(value == 0, "A new layout moves nothing");

        // the same places at any screen size: the editor's own units are not the game's
        float[] scaled = hud.bounds(HudLayout.HEALTH, width * 2.25f, height * 2.25f);
        check(near(scaled[0], health[0] * 2.25f) && near(scaled[3], health[3] * 2.25f), "Panels scale with the screen");

        check(hud.hit(30, 30, width, height) == HudLayout.WEAPONS, "A point on the weapons is the weapons");
        check(hud.hit(width - 30, 30, width, height) == HudLayout.HEALTH, "A point on the health is the health");
        check(hud.hit(60, height - 60, width, height) == HudLayout.RADAR, "A point on the radar is the radar");
        check(hud.hit(width / 2, height / 2, width, height) == -1, "The middle of the screen is no panel");

        hud.move(HudLayout.RADAR, 400, -100, width, height);
        float[] moved = hud.bounds(HudLayout.RADAR, width, height);
        check(near(moved[0], 416) && near(moved[3], height - 116), "A drag moves the panel as far");
        check(near(hud.offsets()[4], 400 / width) && near(hud.offsets()[5], -100 / height),
            "The game takes fractions of the screen");
        check(hud.offsets()[0] == 0 && hud.offsets()[2] == 0, "The other panels stay");
        check(hud.hit(60, height - 60, width, height) == -1 && hud.hit(450, 300, width, height) == HudLayout.RADAR,
            "The panel is found where it went");

        hud.move(HudLayout.RADAR, 5000, 5000, width, height);
        moved = hud.bounds(HudLayout.RADAR, width, height);
        check(near(moved[2], width) && near(moved[3], height), "A panel stops at the screen's edges");
        hud.move(HudLayout.HEALTH, -5000, -5000, width, height);
        moved = hud.bounds(HudLayout.HEALTH, width, height);
        check(near(moved[0], 0) && near(moved[1], 0), "A panel stops at the far corner too");
        hud.move(HudLayout.WEAPONS, Float.NaN, 10, width, height);
        hud.move(HudLayout.WEAPONS, 10, 10, 0, 0);
        check(hud.offsets()[0] == 0 && hud.offsets()[1] == 0, "A bad drag moves nothing");

        String saved = hud.save();
        float[] restored = HudLayout.load(saved).offsets(), expected = hud.offsets();
        for (int i = 0; i < expected.length; i++)
            check(Math.abs(restored[i] - expected[i]) < 0.0001f, "Saved panels come back after a restart");
        for (String damaged : new String[] {null, "", "hud1", "hud1;0,0;0,0", "hud2;0,0;0,0;0,0",
                "hud1;0,0;0,0;x,0", "hud1;0,0;0,0;2,0", "hud1;0,0;0,0;NaN,0", "hud1;0;0,0;0,0"}) {
            for (float value : HudLayout.load(damaged).offsets())
                check(value == 0, "A damaged or missing save is every panel at rest: " + damaged);
        }
        hud.reset();
        for (float value : hud.offsets()) check(value == 0, "Reset puts every panel back");

        // the overlay's export never carries the HUD: it is the touch layout's own format
        String exported = new TouchLayout().exportConfiguration(2f);
        check(!exported.contains("hud") && TouchLayout.importConfiguration(exported) != null,
            "A touch layout file holds no HUD panels");
        System.out.println("HUD panel rest places, dragging, screen limits, saving and export separation passed");
    }
}
