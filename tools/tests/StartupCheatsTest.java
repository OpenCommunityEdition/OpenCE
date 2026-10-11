package com.halo.decomp;

import java.nio.file.*;
import java.nio.charset.StandardCharsets;

public final class StartupCheatsTest {
    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
    public static void main(String[] args) throws Exception {
        Path folder = Files.createTempDirectory("halo-startup-cheats-test");
        Path file = folder.resolve("init.txt");
        try {
            StartupCheats cheats = new StartupCheats(file);
            for (int i = 0; i < 16; i++) check(!cheats.enabled(i), "All cheats start disabled");
            check(!Files.exists(file), "Reading defaults should not create init.txt");
            String custom = "; My settings\r\nset game_speed 1\r\n";
            Files.write(file, custom.getBytes(StandardCharsets.UTF_8));
            cheats.toggle(0); cheats.toggle(14);
            String saved = Files.readString(file);
            check(saved.startsWith(custom), "User commands must survive byte for byte");
            check(saved.contains("set cheat_deathless_player 1\n") && saved.contains("cheat_all_weapons\n"), "Write executable commands");
            StartupCheats reopened = new StartupCheats(file);
            check(reopened.enabled(0) && reopened.enabled(14) && !reopened.enabled(2), "Read selected cheats after restart");
            reopened.toggle(0); reopened.toggle(14);
            saved = Files.readString(file);
            check(saved.contains("set cheat_deathless_player 0\n") && !saved.contains("cheat_all_weapons\n"), "Disabling removes one-shot commands");
            check(saved.indexOf(StartupCheats.BEGIN) == saved.lastIndexOf(StartupCheats.BEGIN), "No duplicated sections");
            for (int i = 0; i < 16; i++) reopened.toggle(i);
            StartupCheats all = new StartupCheats(file);
            for (int i = 0; i < 16; i++) check(all.enabled(i), "Every startup cheat can be enabled");
            String broken = custom+StartupCheats.BEGIN+"\n";
            Files.writeString(file, broken);
            try { all.toggle(1); throw new AssertionError("Malformed block must be rejected"); }
            catch (java.io.IOException expected) {}
            check(Files.readString(file).equals(broken) && all.enabled(1), "Failed save preserves file and selected state");
            System.out.println("Startup cheat defaults, toggles, persistence and user init.txt preservation passed");
        } finally { Files.deleteIfExists(file); Files.deleteIfExists(folder); }
    }
}
