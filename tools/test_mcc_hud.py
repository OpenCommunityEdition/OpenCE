"""MCC canvas and full-resolution icon geometry, using the real C routines.

The observed Ruby fixtures use 30x38 unflagged pause glyphs and 93x93
half-HUD-scale pickup glyphs. Cover their independent sizing and text advances,
including split-screen and the neutral Xbox/CE dispatch.
"""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

from harness import function

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def hud_tool(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is needed for MCC HUD tests")
    work = tmp_path_factory.mktemp("mcc-hud")
    source = (ROOT / "port/linux/game/mcc_hud_draw.c").read_text()
    functions = "\n".join(function(source, name) for name in [
        "mcc_hud_canvas_scale", "mcc_hud_bitmap_scale", "mcc_hud_icon_draw"])
    code = r'''
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
typedef float real;
typedef unsigned char boolean;
typedef unsigned long pixel32;
typedef struct {short x,y;} point2d;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
typedef struct {float x0,x1,y0,y1;} real_rectangle2d;
struct bitmap_data {short width,height;};
struct bitmap_group {unsigned flags;};
struct icon_hud_element_definition {
    short sequence_index,width_offset;point2d offset;
    unsigned long color;char frame_rate;unsigned char flags;short text_index;
};
#define NONE (-1)
#define TRUE 1
#define FALSE 0
#define _hud_anchor_bottom_left 2
static int mcc,players,lookups,draws;
static float drawn_scale;
static point2d drawn_point;
static pixel32 drawn_color;
static struct bitmap_group group;
static int mcc_cache_tags_loaded(void) {return mcc;}
static int local_player_count(void) {return players;}
static struct bitmap_group *bitmap_group_get(long id) {
    if(id!=17)abort();lookups++;return &group;
}
static short draw_unicode_string_capital_middle(rectangle2d const *bounds,wchar_t const *text) {
    if(text[0])abort();return (short)(bounds->y0+8);
}
static void hud_draw_bitmap_direct(struct bitmap_data const *bitmap,short corner,
    point2d const *point,real_rectangle2d const *clip,real scale,real theta,
    pixel32 color,boolean linear) {
    (void)bitmap;(void)clip;
    if(corner!=2||theta||linear)abort();
    draws++;drawn_scale=scale;drawn_point=*point;drawn_color=color;
}
''' + functions + r'''
int main(int argc,char **argv) {
    struct bitmap_data bitmap={256,256};
    real_rectangle2d clip={0.0f,30.0f/256,0.0f,38.0f/256};
    rectangle2d cursor={100,120,200,140},before=cursor;
    struct icon_hud_element_definition icon={0,-6,{-2,15},0xFF123456,0,2,0},saved=icon;
    int menu,handled;
    if(argc!=6 && argc!=7)return 2;
    mcc=atoi(argv[1]);players=atoi(argv[2]);menu=atoi(argv[3]);
    group.flags=(unsigned)atoi(argv[4]);icon.flags=(unsigned char)atoi(argv[5]);
    if(argc==7)icon.offset.y=(short)atoi(argv[6]);saved=icon;
    if(!menu) {clip.x1=clip.y1=93.0f/256;}
    handled=mcc_hud_icon_draw(17,&bitmap,&clip,&cursor,0xFFABCDEF,&icon,menu);
    if(memcmp(&icon,&saved,sizeof(icon)))return 3;
    if(!mcc && (memcmp(&cursor,&before,sizeof(cursor))||draws||lookups||handled))return 4;
    if(mcc_hud_bitmap_scale(NONE)!=1.0f)return 5;
    printf("%d %d %.3f %d %d %d %lu %.3f\n",handled,draws,drawn_scale,
        drawn_point.x,drawn_point.y,cursor.x0,drawn_color,mcc_hud_canvas_scale());
    return 0;
}
'''
    path = work / "hud.c"
    binary = work / ("hud.exe" if sys.platform == "win32" else "hud")
    path.write_text(code)
    command = [compiler, "-std=gnu99", "-Wall", "-Wextra", "-Werror", str(path), "-o", str(binary)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return binary


@pytest.mark.parametrize("players", [1, 2, 4])
@pytest.mark.parametrize("menu", [False, True])
@pytest.mark.parametrize("flags", [0, 0x10, 0x80, 0x90])
@pytest.mark.parametrize("icon_flags", [0, 2, 4, 6])
def test_icon_canvas_size_and_cursor_advance(hud_tool, players, menu, flags, icon_flags):
    result = subprocess.run([str(hud_tool), "1", str(players), str(int(menu)), str(flags), str(icon_flags)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    handled, draws, scale, x, y, cursor, color, canvas = result.stdout.split()
    viewport = 0.75 if players > 1 and not menu else 1.0
    expected_scale = viewport * (0.25 if flags else 0.5)
    offset_scale = viewport
    expected_x = int(100 - 2 * offset_scale + int(menu))
    expected_y = int(128 + 38 * expected_scale * 0.5) if menu else int(140 - 15 * offset_scale)
    advance = -6 * offset_scale
    if not icon_flags & 4:
        advance += (30 if menu else 93) * expected_scale
    assert (handled, draws) == ("1", "1")
    assert float(scale) == pytest.approx(expected_scale, abs=0.0006)
    assert (int(x), int(y), int(cursor)) == (expected_x, expected_y, int(expected_x + advance))
    assert int(color) == (0xFF123456 if icon_flags & 2 else 0xFFABCDEF)
    assert float(canvas) == 0.5


@pytest.mark.parametrize("menu", [False, True])
def test_xbox_and_ce_dispatch_are_neutral(hud_tool, menu):
    result = subprocess.run([str(hud_tool), "0", "2", str(int(menu)), "16", "2"],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    assert result.stdout.split()[:2] == ["0", "0"]
    assert float(result.stdout.split()[-1]) == 1.0


@pytest.mark.parametrize("players", [1, 2])
@pytest.mark.parametrize("offset_y", [-5, 15])
def test_messaging_offsets_preserve_stock_and_map_authored_values(hud_tool, players, offset_y):
    # Xbox/Mercury/Nitra use -5; Ruby changes only Y to +15. These text
    # offsets must survive independently of the MCC bitmap canvas factor.
    result = subprocess.run([str(hud_tool), "1", str(players), "0", "16", "2", str(offset_y)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    viewport = 0.75 if players > 1 else 1.0
    assert int(result.stdout.split()[4]) == int(140 - offset_y * viewport)
