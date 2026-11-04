#include "harness.h"
#define BOTS_MAXIMUM 63
#define csmemset memset
static boolean bots_enhanced_running, enabled;
static long stock_releases, enhanced_starts, enhanced_stops, management_resets;
static struct {
    struct { long player_index; } bots[BOTS_MAXIMUM];
    boolean map_ready;
    long roam_point_count, ticks, searches, search_failures, search_partials;
    long search_overflows, search_unavoided, search_unsmoothed, stuck_events, stuck_giveups;
    long combat_counts[20], last_report_tick;
} bots_globals;
static struct { long count; } bots_weapon_kinds;
static boolean config_boolean(char const *name) { CHECK(!strcmp(name,"bots.enhanced"),"setting"); return enabled; }
static void bots_management_reset(void) { management_resets++; }
static void enhanced_controller_initialize(void) { enhanced_starts++; }
static void enhanced_controller_dispose(void) { enhanced_stops++; }
static void bots_paths_release(void) { stock_releases++; }
#include "under_test.inc"
int main(void)
{
    bots_initialize_for_new_map();
    CHECK(!bots_enhanced_running && !enhanced_starts,"stock initialized enhanced state");
    enabled=TRUE;
    bots_dispose_from_old_map();
    CHECK(stock_releases==1 && !enhanced_stops,"changed setting disposed the wrong map owner");
    bots_initialize_for_new_map();
    CHECK(bots_enhanced_running && enhanced_starts==1,"enhanced controller was not selected");
    enabled=FALSE;
    CHECK(bots_enhanced_running,"setting changed the running controller mid-map");
    bots_dispose_from_old_map();
    CHECK(!bots_enhanced_running && enhanced_stops==1 && stock_releases==1,"enhanced cleanup leaked into stock");
    bots_initialize_for_new_map();
    CHECK(!bots_enhanced_running && enhanced_starts==1,"enhanced mode leaked into the next stock map");
    bots_dispose_from_old_map();
    CHECK(stock_releases==2 && management_resets==6,"common admission lifecycle was skipped");
    return 0;
}
