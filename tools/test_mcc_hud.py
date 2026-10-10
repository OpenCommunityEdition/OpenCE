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


@pytest.fixture(scope="module")
def anchor_tool(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is needed")
    work = tmp_path_factory.mktemp("mcc-anchors")
    source = (ROOT / "port/linux/game/mcc_hud_draw.c").read_text()
    routines = "\n".join(function(source, n) for n in ["mcc_hud_edge", "mcc_hud_anchor_point",
                            "mcc_hud_anchor_bounds", "mcc_hud_anchor_number"])
    code = r'''
#include <stdlib.h>
#include <stdio.h>
typedef float real;
typedef int boolean;
typedef struct {short x,y;} point2d;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
typedef struct {float x0,x1,y0,y1;} real_rectangle2d;
struct bitmap_data {short width,height;point2d registration_point;};
struct hud_placement_definition {point2d offset;};
#define TRUE 1
#define FALSE 0
static int loaded;
static int mcc_cache_tags_loaded(void) {return loaded;}
''' + routines + r'''
int main(int argc,char **argv) {
    struct hud_placement_definition placement={{10,20}};
    rectangle2d window={40,30,840,430},viewport={20,10,860,450};
    point2d point={-99,-99};
    real_rectangle2d bounds={-99,-99,-99,-99};
    short cursor=-99,anchor;int a,b,c;float scale;
    if(argc!=4)return 2;
    loaded=atoi(argv[1]);anchor=(short)atoi(argv[2]);scale=(float)atof(argv[3]);
    a=mcc_hud_anchor_point(anchor,&placement,NULL,scale,&window,&viewport,&point);
    b=mcc_hud_anchor_bounds(anchor,200,80,&bounds);
    c=mcc_hud_anchor_number(anchor,100,300,&cursor);
    printf("%d %d %d %d %d %.1f %.1f %.1f %.1f %d",a,b,c,point.x,point.y,
        bounds.x0,bounds.x1,bounds.y0,bounds.y1,cursor);
    return 0;
}
'''
    path=work / "anchors.c"
    path.write_text(code)
    binary=work / ("anchors.exe" if sys.platform=="win32" else "anchors")
    command=[compiler,"-std=gnu89","-fsanitize=undefined","-fsanitize-trap=undefined",str(path),"-o",str(binary)]
    if sys.platform=="win32":
        command[1:1]=["--target=i686-pc-windows-msvc","-fuse-ld=lld"]
    result=subprocess.run(command,capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    return binary


@pytest.mark.parametrize("scale", [1.0, 0.75])
@pytest.mark.parametrize("anchor,origin,direction,bounds,cursor", [
    (5, (420,20), (1,1), (-100,100,0,80), 350),
    (6, (420,420), (1,-1), (-100,100,-80,0), 350),
    (7, (20,220), (1,1), (0,200,-40,40), 400),
    (8, (820,220), (-1,1), (-200,0,-40,40), 300)])
def test_mcc_edge_anchors_follow_player_window(anchor_tool,scale,anchor,origin,direction,bounds,cursor):
    result=subprocess.run([str(anchor_tool),"1",str(anchor),str(scale)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    values=list(map(float,result.stdout.split()))
    assert values[:3]==[1,1,1]
    assert values[3:5]==[int(origin[0]+10*scale*direction[0]),int(origin[1]+20*scale*direction[1])]
    assert values[5:9]==list(bounds)
    assert values[9]==cursor


@pytest.mark.parametrize("loaded,anchor", [(0,n) for n in range(10)]+[(1,n) for n in [-1,0,1,2,3,4,9]])
def test_edge_dispatch_preserves_legacy_and_unknown_anchors(anchor_tool,loaded,anchor):
    result=subprocess.run([str(anchor_tool),str(loaded),str(anchor),"1"],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    assert list(map(float,result.stdout.split()))==[0,0,0]+[-99]*7


@pytest.fixture(scope="module")
def terminal_tool(tmp_path_factory):
    compiler=shutil.which("clang")
    if not compiler:
        pytest.skip("clang is needed")
    work=tmp_path_factory.mktemp("mcc-terminal")
    source=(ROOT / "port/linux/game/mcc_terminal.c").read_text()
    routines="\n".join(function(source,n) for n in ["mcc_terminal_scale","mcc_terminal_line_height","mcc_terminal_draw"])
    code=r'''
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef float real;typedef int boolean;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
struct font_header {short ascending_height,descending_height,leading_height;};
#define TRUE 1
#define FALSE 0
#define NONE (-1)
static int loaded,calls,draws,lookups;
static real current=1,used,origin_x,origin_y;
static rectangle2d drawn;
static struct font_header font;
static int mcc_cache_tags_loaded(void) {return loaded;}
static struct font_header *font_definition_get(long index) {if(index!=7)abort();lookups++;return &font;}
static void rasterizer_text_set_scale(real scale,real x,real y) {current=scale;calls++;if(scale!=1){origin_x=x;origin_y=y;}}
static void rasterizer_draw_string(rectangle2d const *b,void *clip,void *cursor,short height,char const *text) {
    if(clip||cursor||height||strcmp(text,"100% unchanged"))abort();draws++;drawn=*b;used=current;
}
'''+routines+r'''
int main(int argc,char **argv) {
    rectangle2d bounds={100,400,740,415};struct font_header saved;short h;int handled;
    if(argc!=3)return 2;loaded=atoi(argv[1]);font.ascending_height=(short)atoi(argv[2]);saved=font;
    h=mcc_terminal_line_height(7,font.ascending_height);
    handled=mcc_terminal_draw(7,&bounds,"100% unchanged");
    if(memcmp(&saved,&font,sizeof(font))||current!=1)return 3;
    if(!loaded && lookups)return 4;
    printf("%d %d %d %d %.6f %.1f %.1f %d %d",h,handled,calls,draws,used,origin_x,origin_y,drawn.x1,drawn.y1);
    return 0;
}
'''
    path=work / "terminal.c";path.write_text(code)
    binary=work / ("terminal.exe" if sys.platform=="win32" else "terminal")
    command=[compiler,"-std=gnu89",str(path),"-o",str(binary)]
    if sys.platform=="win32":command[1:1]=["--target=i686-pc-windows-msvc","-fuse-ld=lld"]
    result=subprocess.run(command,capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    return binary


@pytest.mark.parametrize("loaded,height",[(0,33),(0,15),(0,-1),(1,15),(1,12),(1,33),(1,30),(1,32767)])
def test_terminal_scales_only_oversized_mcc_text(terminal_tool,loaded,height):
    result=subprocess.run([str(terminal_tool),str(loaded),str(height)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    values=list(map(float,result.stdout.split()))
    if loaded and height>15:
        assert values[:4]==[15,1,2,1]
        assert values[4]==pytest.approx(15/height,abs=0.000001)
        assert values[5:7]==[100,400]
        assert values[7:]==[min(32767,int(100+640/(15/height))),min(32767,401+height)]
    else:
        assert values[:4]==[height,0,0,0]
