#!/usr/bin/env python3
"""Exercise Android's deferred campaign launch using the actual C functions."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
host = (ROOT / 'port/android/host/host_touch.c').read_text()
menu = (ROOT / 'port/linux/game/menu_functions.c').read_text()
host = host[host.index('static int layout_available;'):]
pending = menu[menu.index('#ifdef HALO_ANDROID\nint host_touch_layout_request'):menu.index('static boolean campaign_fail(void)')]
start = menu[menu.index('static boolean difficulty_start('):menu.index('static short difficulty_shown(')]
stubs = r'''
#include <assert.h>
#include <pthread.h>
#include <string.h>
#define HALO_ANDROID 1
#define TRUE 1
#define FALSE 0
#define PIN(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
#define JNIEXPORT
#define JNICALL
typedef void JNIEnv;
typedef void *jclass;
typedef unsigned char jboolean;
typedef int boolean;
static pthread_mutex_t touch_lock = PTHREAD_MUTEX_INITIALIZER;
struct player_profile { int unused; };
static int starts, last_difficulty, last_controller;
static char last_map[256];
static int profile_ok = 1;
static const char *main_get_map_name(void) { return "levels/a10"; }
static const char *main_get_solo_level_name(int level) { (void)level; return "levels/a10"; }
static int campaign_profile(short controller, struct player_profile *profile)
{ (void)controller; (void)profile; return profile_ok; }
static int custom_edition_maps_level_campaign(const char *map) { (void)map; return 1; }
static int campaign_fail(void) { return 0; }
static void campaign_start(const char *map, short difficulty, short controller)
{ starts++; strcpy(last_map, map); last_difficulty = difficulty; last_controller = controller; }
'''
checks = r'''
int main(void) {
    /* Controller-only launch does not wait for an Android dialog. */
    assert(difficulty_start(1, 0));
    assert(starts == 1);
    Java_com_halo_decomp_TouchControls_nativeLayoutAvailable(0, 0, 1);
    assert(difficulty_start(3, 2));
    assert(starts == 1);
    assert(Java_com_halo_decomp_TouchControls_nativeLayoutRequested(0, 0));
    /* Menu updates and repeated activation cannot start or replace the choice. */
    menu_functions_touch_layout_update();
    assert(difficulty_start(0, 0));
    menu_functions_touch_layout_update();
    assert(starts == 1);
    Java_com_halo_decomp_TouchControls_nativeLayoutChosen(0, 0);
    menu_functions_touch_layout_update();
    assert(starts == 2 && last_difficulty == 3 && last_controller == 2);
    assert(!strcmp(last_map, "levels/a10"));
    assert(!Java_com_halo_decomp_TouchControls_nativeLayoutRequested(0, 0));
    menu_functions_touch_layout_update();
    assert(starts == 2);
    /* A later game asks again; closing/reopening the app does not auto-accept. */
    assert(difficulty_start(2, 0));
    Java_com_halo_decomp_TouchControls_nativeLayoutAvailable(0, 0, 0);
    menu_functions_touch_layout_update();
    assert(starts == 2);
    Java_com_halo_decomp_TouchControls_nativeLayoutChosen(0, 0);
    menu_functions_touch_layout_update();
    assert(starts == 3 && last_difficulty == 2);
    profile_ok = 0;
    assert(!difficulty_start(1, 0));
    assert(starts == 3);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    source = Path(tmp) / 'test.c'
    binary = Path(tmp) / 'test'
    source.write_text(stubs + host + pending + start + checks)
    subprocess.run(['gcc', '-std=gnu89', '-Wall', '-Wextra', '-Werror', '-pthread', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Deferred touch layout campaign launch: passed')
