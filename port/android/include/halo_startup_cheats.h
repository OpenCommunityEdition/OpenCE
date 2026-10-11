/* Keep commands in the same order as StartupCheats.java and TouchControls. */
#ifndef HALO_STARTUP_CHEATS_H
#define HALO_STARTUP_CHEATS_H
#define ANDROID_CHEATS_BEGIN "; BEGIN ANDROID STARTUP CHEATS"
#define ANDROID_CHEATS_END "; END ANDROID STARTUP CHEATS"
static char const *const android_startup_commands[] = {
    "set cheat_deathless_player", "set cheat_jetpack", "set cheat_infinite_ammo",
    "set cheat_bump_possession", "set cheat_super_jump", "set cheat_reflexive_damage_effects",
    "set cheat_medusa", "set cheat_omnipotent", "set cheat_controller", "set cheat_bottomless_clip",
    "cheat_active_camouflage_local_player 0", "cheat_active_camouflage", "cheat_all_powerups",
    "cheat_all_vehicles", "cheat_all_weapons", "cheat_teleport_to_camera"
};
#endif
