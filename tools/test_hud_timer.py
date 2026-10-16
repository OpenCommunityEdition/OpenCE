"""Exercise the scripted timer's real renderer with poisoned automatic storage."""
from pathlib import Path
import shutil
import subprocess

import pytest
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def timer_tools(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is needed for the timer renderer test")
    work = tmp_path_factory.mktemp("hud-timer")
    native = (ROOT / "source/interface/hud_messaging.c").read_text()
    definitions = (ROOT / "source/interface/hud_definitions.h").read_text()
    types = "\n".join(structure(definitions, name) for name in [
        "hud_absolute_placement_definition", "hud_placement_definition", "hud_color_definition"])
    types += "\n" + "\n".join(structure(native, name) for name in [
        "hud_timer_data_definition", "number_hud_element_definition", "hud_number_definition"])
    prefix = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
typedef float real;
typedef unsigned char boolean, byte;
typedef unsigned short word;
typedef struct { short x,y; } point2d;
typedef struct { real i,j; } real_vector2d;
struct tag_reference { long group; char *name; long length,index; };
#define NONE (-1)
#define TRUE 1
#define FALSE 0
#define FLAG(x) (1u<<(x))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define TICKS_PER_SECOND 30
#define SECONDS_PER_MINUTE 60
#define _interface_hud_digits 0
#define _hud_number_show_all_leading_zeros_bit 0
enum { _hud_corner_top_left, _hud_corner_top_right, _hud_corner_bottom_left,
       _hud_corner_bottom_right, _hud_corner_center };
#define match_assert(file,line,cond) assert(cond)
'''
    stubs = r'''
static struct { struct hud_timer_data_definition timer; } messaging;
#define hud_messaging_globals (&messaging)
static struct { struct { struct hud_color_definition color,time_up_color; } timer_definition; } globals;
#define hud_globals (&globals)
static struct hud_number_definition digits;
#define HUD_NUMBER_DEFINITION_GET(index) (&digits)
static struct {short local_player_index;} render;
static long time_code_time=NONE,time_code_stop_time=NONE;
static char temporary[64];
static short remaining;
static int calls,missing;
static long game_time_get(void) { return 100; }
static short scripted_hud_get_timer_ticks(void) { return remaining; }
static long interface_get_tag_index(int kind) { return missing?NONE:0; }
static void render_debug_string(int active,char *text) { }
static void hud_draw_numbers(short player, struct hud_absolute_placement_definition const *absolute,
    struct number_hud_element_definition const *number, short value, short decimal,
    short flags, long reference, real scale) {
    int step=missing?0:24;
    int first=40;
    int values[3]={4,35,43};
    if(absolute->corner==1 || absolute->corner==3) { first+=5*step;step=-step; }
    if(absolute->corner==4) first-=3*step;
    assert(number->placement.multiplayer_scaling_flags==0);
    assert(number->placement.scale.i==1 && number->placement.scale.j==1);
    assert(number->placement.offset.x==first+calls*step*5/2);
    assert(number->placement.offset.y==20);
    assert(number->digits==2 && number->number_flags==1);
    assert(decimal==NONE && scale==2);
    assert(value==(remaining>0?values[calls]:0));
    assert(number->colors.color==(remaining>0?0x1234u:0x5678u));
    assert(flags==(remaining<=0));
    calls++;
}
'''
    main = r'''
int main(int argc,char **argv) {
    messaging.timer.corner=(short)atoi(argv[1]);
    remaining=(short)(atoi(argv[2])?0:((4*60+35)*30+13));
    missing=atoi(argv[3]);
    messaging.timer.enabled=(boolean)atoi(argv[4]);
    messaging.timer.position.x=40;messaging.timer.position.y=20;
    messaging.timer.ticks=remaining;messaging.timer.reference_time=100;
    globals.timer_definition.color.color=0x1234;
    globals.timer_definition.time_up_color.color=0x5678;
    digits.screen_width=12;
    hud_render_timer();
    assert(calls==(messaging.timer.enabled?3:0));
    return 0;
}
'''
    code = function(native, "hud_render_timer")
    executables = []
    for name, body in [("fixed", code), ("uninitialized", code.replace(
            "numbers.placement.multiplayer_scaling_flags = 0;", ""))]:
        source = work / (name + ".c")
        source.write_text(prefix + types + stubs + body + main)
        exe = work / (name + ".exe")
        subprocess.run([clang, "-m32", "-std=gnu89", "-O1",
                        "-ftrivial-auto-var-init=pattern", str(source), "-o", str(exe)], check=True)
        executables.append(exe)
    return executables


@pytest.mark.parametrize("corner", range(5))
@pytest.mark.parametrize("expired", [0, 1])
def test_timer_placement_and_scaling(timer_tools, corner, expired):
    subprocess.run([str(timer_tools[0]), str(corner), str(expired), "0", "1"], check=True)


@pytest.mark.parametrize("missing,enabled", [(1, 1), (0, 0)])
def test_missing_digits_or_hidden_timer(timer_tools, missing, enabled):
    subprocess.run([str(timer_tools[0]), "3", "0", str(missing), str(enabled)], check=True)


def test_uninitialized_flags_are_detected(timer_tools):
    result = subprocess.run([str(timer_tools[1]), "3", "0", "0", "1"], capture_output=True)
    assert result.returncode != 0
