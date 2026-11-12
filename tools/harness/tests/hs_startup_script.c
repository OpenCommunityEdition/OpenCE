#include "harness.h"
#include <string.h>
typedef unsigned short word;
#include "enums.inc"
static struct { boolean initialized; short executing_thread_index; } hs_runtime_globals = { TRUE, 0 };
struct hs_thread_datum { short type, script_index; };
struct hs_script { short script_type; };
static struct hs_thread_datum threads[2];
static struct hs_script scripts[2];
static struct { struct { long count; } hs_scripts; } scenario;
#define hs_thread_get(i) (&threads[i])
#define global_scenario_get() (&scenario)
#define TAG_BLOCK_GET_ELEMENT(block, i, type) ((void)(block), &scripts[i])
static short global_designator = NONE, global_type = _hs_type_boolean;
static long global_value = 1;
static boolean disabled;
static short hs_find_global_by_name(char const *name) { return !strcmp(name, "vehicle_riding") ? global_designator : NONE; }
static short hs_global_get_type(short index) { (void)index; return global_type; }
static long hs_global_evaluate(short index) { (void)index; return global_value; }
static boolean hs_scenario_global_initializer_disabled(short index) { (void)index; return disabled; }
#include "under_test.inc"
int main(int argc, char **argv)
{
    CHECK(argc == 2, "case");
    char const *case_name = argv[1];
    scripts[0].script_type = _hs_script_startup;
    CASE("startup") { CHECK(hs_runtime_port_executing_script_type() == _hs_script_startup, "startup lost"); }
    else CASE("dormant") {
        scripts[0].script_type = _hs_script_dormant;
        CHECK(hs_runtime_port_executing_script_type() == _hs_script_dormant, "dormant lost");
        scripts[0].script_type = _hs_script_continuous;
        CHECK(hs_runtime_port_executing_script_type() == _hs_script_continuous, "continuous lost");
    }
    else CASE("console") { threads[0].type = _hs_thread_type_console_command; CHECK(hs_runtime_port_executing_script_type() == NONE, "console counted"); }
    else CASE("initializer") { threads[0].type = _hs_thread_type_global_initialize; CHECK(hs_runtime_port_executing_script_type() == NONE, "initializer counted"); }
    else CASE("idle") {
        hs_runtime_globals.executing_thread_index = NONE;
        CHECK(hs_runtime_port_executing_script_type() == NONE, "idle read");
        hs_runtime_globals.executing_thread_index = 0;
        hs_runtime_globals.initialized = FALSE;
        CHECK(hs_runtime_port_executing_script_type() == NONE, "uninitialized read");
    }
    else CASE("global-boolean") {
        global_designator = 0;
        CHECK(hs_runtime_port_scenario_boolean("vehicle_riding") == TRUE, "true lost");
        global_value = 0;
        CHECK(hs_runtime_port_scenario_boolean("vehicle_riding") == FALSE, "false lost");
    }
    else CASE("global-absent") { CHECK(hs_runtime_port_scenario_boolean("vehicle_riding") == NONE, "absent read"); }
    else CASE("global-other-type") { global_designator = 0; global_type = _hs_type_short_integer; CHECK(hs_runtime_port_scenario_boolean("vehicle_riding") == NONE, "other type read"); }
    else CASE("global-disabled") { global_designator = 0; disabled = TRUE; CHECK(hs_runtime_port_scenario_boolean("vehicle_riding") == NONE, "disabled read"); }
    else CASE("global-engine") { global_designator = (short)0x8000; CHECK(hs_runtime_port_scenario_boolean("vehicle_riding") == NONE, "engine read"); }
    else return 2;
    return 0;
}
