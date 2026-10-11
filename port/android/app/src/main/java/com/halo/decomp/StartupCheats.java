package com.halo.decomp;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.AtomicMoveNotSupportedException;

/** Only the marked section belongs to the menu; preserve all user commands. */
final class StartupCheats {
    static final String BEGIN = "; BEGIN ANDROID STARTUP CHEATS";
    static final String END = "; END ANDROID STARTUP CHEATS";
    static final String[] COMMANDS = {
        "set cheat_deathless_player", "set cheat_jetpack", "set cheat_infinite_ammo",
        "set cheat_bump_possession", "set cheat_super_jump", "set cheat_reflexive_damage_effects",
        "set cheat_medusa", "set cheat_omnipotent", "set cheat_controller", "set cheat_bottomless_clip",
        "cheat_active_camouflage_local_player 0", "cheat_active_camouflage", "cheat_all_powerups",
        "cheat_all_vehicles", "cheat_all_weapons", "cheat_teleport_to_camera"
    };
    private final Path file;
    private boolean[] enabled = new boolean[COMMANDS.length];

    StartupCheats(Path file) throws IOException {
        this.file = file;
        String text = read();
        int start = text.indexOf(BEGIN), end = text.indexOf(END);
        if (start >= 0 && end > start) {
            String block = text.substring(start+BEGIN.length(), end);
            for (String line : block.split("\\R")) {
                for (int i = 0; i < COMMANDS.length; i++)
                    if (line.trim().equals(COMMANDS[i]+(i < 10 ? " 1" : ""))) enabled[i] = true;
            }
        }
    }

    boolean enabled(int id) { return enabled[id]; }
    private String read() throws IOException {
        return Files.exists(file) ? new String(Files.readAllBytes(file), StandardCharsets.UTF_8) : "";
    }

    void toggle(int id) throws IOException {
        boolean[] next = enabled.clone(); next[id] = !next[id];
        String text = read();
        int start = text.indexOf(BEGIN), end = text.indexOf(END);
        if ((start >= 0) != (end >= 0) || (start >= 0 && end < start))
            throw new IOException("Incomplete startup cheats section in init.txt");
        StringBuilder block = new StringBuilder(BEGIN+"\n");
        for (int i = 0; i < COMMANDS.length; i++) {
            if (i < 10) block.append(COMMANDS[i]).append(next[i] ? " 1\n" : " 0\n");
            else if (next[i]) block.append(COMMANDS[i]).append('\n');
        }
        block.append(END);
        String output = start >= 0 ? text.substring(0, start)+block+text.substring(end+END.length())
            : text+(text.isEmpty() || text.endsWith("\n") ? "" : "\n")+block+"\n";
        Files.createDirectories(file.getParent());
        Path temp = Files.createTempFile(file.getParent(), "init-", ".tmp");
        try {
            Files.write(temp, output.getBytes(StandardCharsets.UTF_8));
            try { Files.move(temp, file, StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING); }
            catch (AtomicMoveNotSupportedException e) { Files.move(temp, file, StandardCopyOption.REPLACE_EXISTING); }
            enabled = next;
        } finally { Files.deleteIfExists(temp); }
    }
}
