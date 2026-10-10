"""Exercise real scenario-load dispatch against a failed initial MCC BSP."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def test_initial_mcc_bsp_failure_balances_state_and_preserves_legacy_rollback(tmp_path):
    """Exercise real BSP switching with and without the MCC dispatch active."""
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    current = function((ROOT / "source/scenario/scenario.c").read_text(), "scenario_switch_structure_bsp")
    prefix = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int boolean;
#define FALSE 0
#define TRUE 1
#define NONE (-1)
#define _error_silent 0
#define _error_immediate 1
#define MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO 32
#define match_assert(file,line,condition) do {if(!(condition))exit(20);}while(0)
#define TAG_BLOCK_GET_ELEMENT(block,index,type) ((type *)(block)->address+(index))
struct block {int count;void *address;};
struct scenario_structure_bsp_reference {struct {int index;} structure_bsp;};
struct collision_bsp {int unused;};
struct bsp3d {int unused;};
struct structure_bsp {struct block collision_bsp;};
static struct {struct block structure_bsp_references;} scenario,*global_scenario=&scenario;
static struct {int structure_bsp_index;} globals,*scenario_globals=&globals;
static struct scenario_structure_bsp_reference references[2]={{{0}},{{1}}};
static struct collision_bsp collision;
static struct structure_bsp bsp={{1,&collision}};
static struct structure_bsp *global_structure_bsp;
static struct collision_bsp *global_collision_bsp;
static struct bsp3d *global_bsp3d;
static short global_structure_bsp_index;
static int mcc,mode;
static struct results {
    int stopped,started,collision_disabled,collision_enabled,disconnects,reconnects,
        unloads,loads,verifies,modal,silent,menu,returned,index,scenario_index,pointers;
} observed;
static int mcc_cache_tags_loaded(void) {return mcc;}
static void main_stop_time(void) {observed.stopped++;}
static void main_start_time(void) {observed.started++;}
static void collision_log_enable(int enabled) {
    if(enabled)observed.collision_enabled++;else observed.collision_disabled++;
}
static void scenario_call_disconnect_from_structure_bsp_procs(void) {observed.disconnects++;}
static void scenario_call_reconnect_to_structure_bsp_procs(void) {observed.reconnects++;}
static void scenario_structure_bsp_unload(struct scenario_structure_bsp_reference *reference) {
    (void)reference;observed.unloads++;
}
static int scenario_structure_bsp_load(struct scenario_structure_bsp_reference *reference) {
    (void)reference;observed.loads++;
    return mode==0 || mode==4 || (mode==2&&observed.loads==2);
}
static struct structure_bsp *structure_bsp_definition_get(int index) {(void)index;return &bsp;}
static int structure_bsp_port_verify(struct structure_bsp *value) {
    (void)value;observed.verifies++;return mode!=4 || observed.verifies>1;
}
static void main_goto_main_menu(void) {observed.menu++;}
static void error(int priority,char const *format,...) {
    (void)format;if(priority==_error_immediate)observed.modal++;else observed.silent++;
}
static void reset(int old) {
    memset(&observed,0,sizeof(observed));
    scenario.structure_bsp_references.count=2;scenario.structure_bsp_references.address=references;
    global_structure_bsp_index=globals.structure_bsp_index=(short)old;
    global_structure_bsp=old==NONE?NULL:&bsp;
    global_collision_bsp=old==NONE?NULL:&collision;
    global_bsp3d=old==NONE?NULL:(void *)&collision;
}
static void finish(int result) {
    observed.returned=result;observed.index=global_structure_bsp_index;
    observed.scenario_index=globals.structure_bsp_index;
    observed.pointers=!!global_structure_bsp+!!global_collision_bsp+!!global_bsp3d;
}
''' + current + r'''
int main(void) {
    int old,requested;
    for(old=NONE;old<=0;old++)
        for(requested=NONE;requested<=2;requested++)for(mode=0;mode<=4;mode++) {
            struct results expected;
            /* Xbox and CE share the inactive MCC gate. Their preserved flow
             * remains the reference for all non-admission cases. */
            mcc=0;reset(old);finish(scenario_switch_structure_bsp((short)requested));expected=observed;
            if(old==NONE&&requested>=0&&requested<2&&mode!=0) {
                if(expected.modal!=1||expected.returned||expected.pointers||expected.loads!=1||
                    expected.stopped!=1||expected.started!=1||expected.collision_disabled!=1||
                    expected.collision_enabled!=1||expected.menu||expected.index!=NONE)return 3;
            }
            if(old==0&&requested==1&&mode==2) {
                if(expected.modal!=1||expected.loads!=2||expected.returned||expected.pointers!=3||
                    expected.disconnects!=1||expected.reconnects!=1||expected.menu||
                    expected.index!=0||expected.scenario_index!=0)return 4;
            }
            mcc=1;reset(old);finish(scenario_switch_structure_bsp((short)requested));
            if(old==NONE&&requested>=0&&requested<2&&mode!=0) {
                if(observed.modal||observed.returned||observed.pointers||observed.loads!=1||
                    observed.stopped!=1||observed.started!=1||observed.collision_disabled!=1||
                    observed.collision_enabled!=1||observed.disconnects||observed.reconnects||
                    observed.menu||observed.index!=NONE||observed.scenario_index!=NONE)return 1;
                /* The sole behavioral difference is avoiding the modal error. */
                observed.modal=expected.modal;observed.silent=expected.silent;
            }
            if(memcmp(&observed,&expected,sizeof(expected))) {
                fprintf(stderr,"dispatch changed: old=%d new=%d mode=%d\n",old,requested,mode);
                return 2;
            }
        }
    return 0;
}
'''
    path = tmp_path / "bsp_dispatch.c"
    path.write_text(prefix)
    binary = tmp_path / ("bsp_flow.exe" if sys.platform == "win32" else "bsp_flow")
    flags = (["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else [])
    built = subprocess.run([compiler, *flags, "-std=c99", "-Wall", "-Wextra", "-Werror",
                            str(path), "-o", str(binary)], capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


def test_failed_mcc_bsp_releases_io_owner(tmp_path):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    actual = function((ROOT / "source/scenario/scenario.c").read_text(), "scenario_load")
    prefix = r'''
#include <stdio.h>
#include <string.h>
typedef int boolean;
#define FALSE 0
#define TRUE 1
#define NONE (-1)
#define GAME_GLOBALS_TAG 0
#define _error_delayed 0
static int scenario_memory_status_attributed, global_scenario_index;
static int mcc_owner, bsp_result, unloads, tag_result = 1;
static void *global_game_globals;
struct scenario { struct { int count; } structure_bsp_references; };
static struct scenario scenario = {{1}}, *global_scenario;
static void check_memory_status(int a, char const *b) {(void)a;(void)b;}
static int scenario_tags_load(char const *name) {(void)name;return tag_result;}
static struct scenario *scenario_definition_get(int index) {(void)index;return &scenario;}
static int tag_loaded(int group,char const *name) {(void)group;(void)name;return 0;}
static void *game_globals_definition_get(int index) {(void)index;return &scenario;}
static int scenario_switch_structure_bsp(int index) {(void)index;return bsp_result;}
static void error(int priority,char const *format,...) {(void)priority;(void)format;}
static int mcc_cache_tags_loaded(void) {return mcc_owner;}
static void scenario_unload(void) {unloads++;mcc_owner=0;global_scenario=NULL;}
'''
    checks = r'''
int main(void) {
    mcc_owner=1;
    if(scenario_load("mcc_maps\\a10") || unloads!=1 || mcc_owner || global_scenario) return 1;
    /* A subsequent ordinary map must retain its existing lifetime behavior. */
    unloads=0;
    if(scenario_load("a10") || unloads || mcc_owner) return 2;
    bsp_result=1;
    if(!scenario_load("custom_maps\\hugeass") || unloads) return 3;
    mcc_owner=1;
    if(!scenario_load("mcc_maps\\dangercanyon") || unloads || !mcc_owner) return 4;
    /* Failed conversion already disposed its private state; no double unload. */
    mcc_owner=0;tag_result=NONE;
    if(scenario_load("mcc_maps\\bad") || unloads) return 5;
    return 0;
}
'''
    source = tmp_path / "lifecycle.c"
    source.write_text(prefix + actual + checks)
    binary = tmp_path / ("lifecycle.exe" if sys.platform == "win32" else "lifecycle")
    flags = (["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else [])
    built = subprocess.run([compiler, *flags, "-std=c99", "-Wall", "-Wextra", "-Werror",
                            str(source), "-o", str(binary)], capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


def test_failed_mcc_map_returns_to_ui_before_game_initialization(tmp_path):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    main = function((ROOT / "source/main/main.c").read_text(), "main_new_map")
    recovery = function((ROOT / "port/linux/game/mcc_main.c").read_text(), "mcc_main_load_failed")
    source = r'''
#include <string.h>
#include <wchar.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define _error_immediate 1
#define _error_silent 0
struct game_options {char const *map_name;};
static struct {int reset_map,defer_map_change,revert_map,skip_cinematic,saving_map,
    won_map,lost_map,respawn,save_core,switch_to_structure_bsp_index,load_core,
    load_core_at_startup,allow_persistent_storage;} main_globals;
static int loads,initialized,players,pulses,menus,popups,clears,errors,gotos,refuse=1;
static void main_new_map(struct game_options *options);
static void input_flush(void) {}
static int game_load(struct game_options *options) {
    loads++; return !strcmp(options->map_name,"levels\\ui\\ui") || !refuse;
}
static void game_initialize_for_new_map(void) {initialized++;}
static void error(int priority,char const *format,...) {(void)format;if(priority)errors++;}
static int errors_handle(void) {return errors!=0;}
static void create_local_players(void) {players++;}
static void game_time_start(void) {}
static void game_initial_pulse(void) {pulses++;}
static void game_state_try_and_load_from_persistent_storage(void) {}
static void ui_widgets_disable_pause_game(int ticks) {(void)ticks;}
static int mcc_level_name(char const *name) {return !strncmp(name,"mcc_maps\\",9);}
static void display_error_text_when_main_menu_loaded(wchar_t const *text) {(void)text;popups++;}
static void errors_clear(void) {errors=0;clears++;}
static void main_goto_main_menu(void) {gotos++;}
static void main_menu_load(void) {
    struct game_options ui={"levels\\ui\\ui"};menus++;main_new_map(&ui);
}
''' + recovery + main + r'''
int main(void) {
    struct game_options options={"mcc_maps\\bad_bsp"};
    errors=1; /* A failed initial BSP already reported a native error. */
    main_new_map(&options);
    if(loads!=2 || initialized!=1 || players!=1 || pulses!=1 || menus!=1 ||
       popups!=1 || clears!=1 || gotos!=1 || errors) return 1;
    options.map_name="a10";main_new_map(&options);
    if(menus!=1 || clears!=1 || initialized!=1 || loads!=3) return 2;
    options.map_name="custom_maps\\bad";main_new_map(&options);
    if(menus!=1 || clears!=1 || initialized!=1 || loads!=4) return 3;
    options.map_name="mcc_maps\\good";refuse=0;errors=0;main_new_map(&options);
    if(menus!=1 || clears!=1 || initialized!=2 || loads!=5) return 4;
    return 0;
}
'''
    path = tmp_path / "recovery.c"
    path.write_text(source)
    binary = tmp_path / ("recovery.exe" if sys.platform == "win32" else "recovery")
    flags = (["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else [])
    built = subprocess.run([compiler, *flags, "-std=c99", "-Wall", "-Wextra", "-Werror",
                            str(path), "-o", str(binary)], capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
