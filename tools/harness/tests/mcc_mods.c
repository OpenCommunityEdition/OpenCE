#define _CRT_SECURE_NO_WARNINGS
#include "harness.h"

static long updates;
static long releases;
static unsigned long scans;
#define csstrncpy strncpy
#define console_printf(...) ((void)0)
#define console_warning(...) ((void)0)
static long port_mods_scan(void) { return ++scans; }
static unsigned long port_mods_scans(void) { return scans; }
static boolean port_mods_played(void) { return TRUE; }
static long update(boolean remove_all, boolean at_load)
{
    if (!remove_all) ++updates;
    return 1;
}
#define mod_strings_update update
#define mod_bitmaps_update update
#define mod_sounds_update update
#define mod_models_update update
static void mod_sounds_tags_unloaded(void) { ++releases; }
static void mod_bitmaps_tags_unloaded(void) {}
static void mod_models_tags_unloaded(boolean give_back) {}
static void mod_strings_tags_unloaded(void) {}
#include "under_test.inc"

int main(int argc, char **argv)
{
    long before;
    mods_tags_loaded("bloodgulch");
    CHECK(mods_map_loaded() && updates == 4, "Xbox mods apply");
    /* A load that never reached normal unload must still drop old overrides. */
    mods_tags_loaded("mcc_maps\\package\\bloodgulch");
    CHECK(!mods_map_loaded() && releases == 1, "MCC clears previous mods");
    before = updates;
    mods_enable(TRUE);
    mods_reload();
    mods_apply();
    CHECK(updates == before, "console and menu cannot enable MCC mods");
    mods_tags_unloaded();
    mods_tags_loaded("custom_maps\\deathisland");
    CHECK(mods_map_loaded() && updates == before + 4, "CE mods resume");
    mods_enable(FALSE);
    before = updates;
    mods_reload();
    CHECK(updates == before, "upstream manual disable preserved");
    mods_tags_loaded("MCC_MAPS/package/a10");
    CHECK(!mods_map_loaded(), "case and slash independent MCC exclusion");
    mods_enable(TRUE);
    CHECK(updates == before, "disabled CE to MCC stays inactive");
    mods_tags_loaded("a10");
    CHECK(mods_map_loaded() && updates == before + 4, "Xbox mods resume without settings reset");
    mods_tags_unloaded();
    CHECK(!mods_map_loaded(), "unload clears eligibility");
    return 0;
}
