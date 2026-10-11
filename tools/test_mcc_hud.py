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
        "mcc_hud_glyphs_dispose", "mcc_hud_button_index", "mcc_hud_button_bitmap", "mcc_hud_button_draw",
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
struct bitmap_data {short width,height;unsigned long *hardware_format;};
struct bitmap_group {unsigned flags;};
struct dynamic_screen_vertex {struct {float x,y;}position,texture_coordinates;pixel32 color;};
struct rasterizer_dynamic_screen_geometry_parameters {
    struct bitmap_data *map[3];
    struct {float i,j;}map_texture_scale[3],map_scale[3];
    short framebuffer_blend_function;
};
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
static struct {struct bitmap_data *bitmap;long asset;boolean attempted;} mcc_buttons[6];
static struct {struct {struct {long count;void *address;} button_icons;} messaging;} globals,*hud_globals;
static int allocations,registrations,deletions;
static struct bitmap_data native_bitmap;
static unsigned long hardware[2];
static char const *assets[]={"ui\\shell\\bitmaps\\a_butn","ui\\shell\\bitmaps\\b_butn",
    "ui\\shell\\bitmaps\\x_butn","ui\\shell\\bitmaps\\y_butn",
    "mcc\\buttons\\black","mcc\\buttons\\white"};
static long hud_hires_asset_count(void) {return 6;}
static char const *hud_hires_asset_tag(long i) {return assets[i];}
static struct bitmap_data *bitmap_2d_new(short w,short h,short m,short f) {
    if(w!=32||h!=32||m||f!=11)abort();allocations++;
    native_bitmap.width=w;native_bitmap.height=h;return &native_bitmap;
}
static int rasterizer_bitmap_new(struct bitmap_data *b) {b->hardware_format=hardware;return 1;}
static void hud_hires_register_placeholder(long a,unsigned long const *t) {
    if(a<0||a>=6||(t&&t!=hardware))abort();registrations+=t?1:-1;
}
static void xgpu_mcc_texture_unbind(unsigned long const *t) {if(t!=hardware)abort();}
static void bitmap_delete(struct bitmap_data *b) {if(b!=&native_bitmap)abort();deletions++;}
static void rasterizer_psuedo_dynamic_screen_quad_draw(
    struct rasterizer_dynamic_screen_geometry_parameters *p,struct dynamic_screen_vertex *v) {
    if(p->framebuffer_blend_function||p->map[0]!=&native_bitmap||p->map[1]||p->map[2])abort();
    if(p->map_scale[0].i!=1||p->map_scale[0].j!=1||
       p->map_texture_scale[0].i!=1||p->map_texture_scale[0].j!=1)abort();
    if(v[0].texture_coordinates.x||v[0].texture_coordinates.y||
       v[2].texture_coordinates.x!=21.0f/32||v[2].texture_coordinates.y!=26.0f/32)abort();
    draws++;drawn_scale=(v[2].position.y-v[0].position.y)/26;
    drawn_point.x=(short)v[0].position.x;drawn_point.y=(short)v[2].position.y;drawn_color=v[0].color;
}
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
    struct bitmap_data bitmap={256,256,NULL};
    real_rectangle2d clip={0.0f,30.0f/256,0.0f,38.0f/256};
    rectangle2d cursor={100,120,200,140},before=cursor;
    struct icon_hud_element_definition icon={0,-6,{-2,15},0xFF123456,0,2,0},saved=icon;
    struct icon_hud_element_definition buttons[18],*active=&icon;
    int menu,handled;
    if(argc!=6 && argc!=7 && argc!=8)return 2;
    mcc=atoi(argv[1]);players=atoi(argv[2]);menu=atoi(argv[3]);
    group.flags=(unsigned)atoi(argv[4]);icon.flags=(unsigned char)atoi(argv[5]);
    if(argc==7)icon.offset.y=(short)atoi(argv[6]);saved=icon;
    if(argc==8) {
        int b=atoi(argv[7]);if(b<0||b>=18)return 8;
        memset(buttons,0,sizeof(buttons));buttons[b]=icon;buttons[b].sequence_index=99;
        globals.messaging.button_icons.count=18;globals.messaging.button_icons.address=buttons;
        hud_globals=&globals;active=&buttons[b];
    }
    if(!menu) {clip.x1=clip.y1=93.0f/256;}
    handled=mcc_hud_icon_draw(17,&bitmap,&clip,&cursor,0xFFABCDEF,active,menu);
    if(memcmp(&icon,&saved,sizeof(icon)))return 3;
    if(!mcc && (memcmp(&cursor,&before,sizeof(cursor))||draws||lookups||handled))return 4;
    if(mcc_hud_bitmap_scale(NONE)!=1.0f)return 5;
    if(argc==8 && mcc && !menu && atoi(argv[7])<6) {
        if(allocations!=1||registrations!=1||lookups)return 9;
        if(mcc_hud_button_bitmap((short)atoi(argv[7]))!=&native_bitmap||allocations!=1)return 10;
        mcc_hud_glyphs_dispose();mcc_hud_glyphs_dispose();
        if(registrations||deletions!=1)return 11;
    } else if(allocations||registrations)return 12;
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


@pytest.mark.parametrize("button", range(6))
@pytest.mark.parametrize("players", [1, 2, 4])
@pytest.mark.parametrize("flags", [0, 16, 128])
def test_gameplay_native_buttons_ignore_mcc_sheet_and_offsets(hud_tool, button, players, flags):
    result=subprocess.run([str(hud_tool),"1",str(players),"0",str(flags),"6","0",str(button)],
                          capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    handled,draws,scale,x,y,cursor,color,canvas=result.stdout.split()
    viewport=1.0 if players==1 else 0.75
    assert (handled,draws)==("1","1")
    assert int(x)==int(100+2*viewport)
    assert float(scale)==pytest.approx(viewport*20/26,abs=0.0006)
    assert int(y)==int(128+10*viewport)
    assert int(cursor)==int(int(100+2*viewport)+(21*20/26-3)*viewport)
    assert int(color)==0xFFFFFFFF


@pytest.mark.parametrize("loaded,menu,button", [(0,0,0),(0,1,2),(1,1,0),(1,1,1),(1,0,6),(1,0,17)])
def test_native_glyph_scope_preserves_menus_other_formats_and_icons(hud_tool, loaded, menu, button):
    result=subprocess.run([str(hud_tool),str(loaded),"1",str(menu),"16","2","0",str(button)],
                          capture_output=True,text=True)
    assert result.returncode==0,result.stderr


@pytest.fixture(scope="module")
def anchor_tool(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is needed")
    work = tmp_path_factory.mktemp("mcc-anchors")
    source = (ROOT / "port/linux/game/mcc_hud_draw.c").read_text()
    routines = "\n".join(function(source, n) for n in ["mcc_hud_edge", "mcc_hud_frame_inset", "mcc_hud_anchor_point",
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
#define MIN(a,b) ((a)<(b)?(a):(b))
static int loaded;
static long halo_screen_width(void) {return 840;}
static int mcc_cache_tags_loaded(void) {return loaded;}
''' + routines + r'''
int main(int argc,char **argv) {
    struct hud_placement_definition placement={{10,20}};
    rectangle2d window={40,30,840,430},viewport={20,10,860,450};
    point2d point={-99,-99};
    real_rectangle2d bounds={-99,-99,-99,-99};
    short cursor=-99,anchor;int a,b,c;float scale;
    if(argc!=4 && argc!=5)return 2;
    if(argc==5) {
        if(atoi(argv[4])==1) {window=(rectangle2d){64,36,422,236};viewport=(rectangle2d){0,0,426,240};}
        else {window=(rectangle2d){180,30,700,430};}
    }
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


@pytest.fixture(scope="module")
def child_anchor_tool(tmp_path_factory):
    compiler=shutil.which("clang")
    if not compiler:pytest.skip("clang is needed")
    work=tmp_path_factory.mktemp("mcc-child-anchors")
    source=(ROOT/"port/linux/game/mcc_hud_draw.c").read_text()
    code=r'''
#include <stdlib.h>
#include <string.h>
struct hud_absolute_placement_definition {short corner,pad;long unused[8];};
static int loaded;
static int mcc_cache_tags_loaded(void) {return loaded;}
'''+function(source,"mcc_hud_child_placement")+r'''
int main(int argc,char **argv) {
    unsigned short header[18]={0};
    struct hud_absolute_placement_definition parent={4,71,{22}},saved=parent,out={-1,0,{0}};
    struct hud_absolute_placement_definition const *result;
    int child;
    if(argc!=3)return 2;loaded=atoi(argv[1]);child=atoi(argv[2]);header[3]=(unsigned short)child;
    result=mcc_hud_child_placement(&parent,loaded?header:NULL,&out);
    if(memcmp(&parent,&saved,sizeof(parent)))return 3;
    if(loaded && child>=1 && child<=9) {
        if(result!=&out||out.corner!=child-1||out.pad!=71||out.unused[0]!=22)return 4;
    } else if(result!=&parent||out.corner!=-1)return 5;
    /* Each element must start from the parent, not the previous override. */
    header[3]=0;result=mcc_hud_child_placement(&parent,header,&out);
    return result!=&parent||result->corner!=4;
}
'''
    path=work/"child.c";path.write_text(code)
    binary=work/("child.exe" if sys.platform=="win32" else "child")
    command=[compiler,"-std=gnu89","-Wall","-Wextra","-Werror",str(path),"-o",str(binary)]
    if sys.platform=="win32":command[1:1]=["--target=i686-pc-windows-msvc","-fuse-ld=lld"]
    result=subprocess.run(command,capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    return binary


@pytest.mark.parametrize("loaded",[0,1])
@pytest.mark.parametrize("child",[-1,*range(11),32767])
def test_child_anchors_are_mcc_only_bounded_and_do_not_mutate_parent(child_anchor_tool, loaded, child):
    result=subprocess.run([str(child_anchor_tool),str(loaded),str(child)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr


@pytest.mark.parametrize("scale", [1.0, 0.75])
@pytest.mark.parametrize("anchor,origin,direction,bounds,cursor", [
    (5, (420,6), (1,1), (-100,100,0,80), 350),
    (6, (420,434), (1,-1), (-100,100,-80,0), 350),
    (7, (6,220), (1,1), (0,200,-40,40), 400),
    (8, (834,220), (-1,1), (-200,0,-40,40), 300)])
def test_mcc_edge_anchors_follow_player_window(anchor_tool,scale,anchor,origin,direction,bounds,cursor):
    result=subprocess.run([str(anchor_tool),"1",str(anchor),str(scale)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    values=list(map(float,result.stdout.split()))
    assert values[:3]==[1,1,1]
    assert values[3:5]==[int(origin[0]+10*scale*direction[0]),int(origin[1]+20*scale*direction[1])]
    assert values[5:9]==list(bounds)
    assert values[9]==cursor


@pytest.mark.parametrize("loaded,anchor", [(0,n) for n in range(10)]+[(1,n) for n in [-1,4,9]])
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


def test_shoulder_redraws_match_native_controller_order():
    import json
    import struct
    assets = ROOT / "port/assets/buttons"
    entries = json.loads((assets / "buttons.json").read_text())["assets"]
    by_tag = {entry["tag"]: entry for entry in entries}
    source = (ROOT / "port/linux/game/mcc_hud_draw.c").read_text()
    # Controller entries 4/5 are black/white; physical RB/LB map to them.
    body = function(source, "mcc_hud_button_bitmap")
    assert body.index('buttons\\\\black') < body.index('buttons\\\\white')
    for color in ("black", "white"):
        entry = by_tag["mcc\\buttons\\" + color]
        assert entry["crc"] == 0
        assert (entry["width"], entry["height"], entry["scale"]) == (32, 32, 8)
        png = (assets / (entry["name"] + ".png")).read_bytes()
        assert png[:8] == b"\x89PNG\r\n\x1a\n"
        assert struct.unpack_from(">II", png, 16) == (256, 256)
        assert png[24:26] == bytes((8, 6))  # RGBA8


@pytest.mark.parametrize("anchor,expected", [(0,(16,26)),(1,(824,26)),(2,(16,414)),(3,(824,414))])
def test_mcc_corner_anchors_use_mcc_safe_frame(anchor_tool, anchor, expected):
    result=subprocess.run([str(anchor_tool),"1",str(anchor),"1"],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    values=list(map(float,result.stdout.split()))
    assert values[:3]==[1,0,0]
    assert values[3:5]==list(expected)


@pytest.mark.parametrize("layout,anchor,expected", [(1,1,(412,32)),(1,3,(412,216)),(2,0,(128,26))])
def test_mcc_safe_frame_preserves_split_seams_and_zoom_inset(anchor_tool,layout,anchor,expected):
    result=subprocess.run([str(anchor_tool),"1",str(anchor),"1",str(layout)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    assert list(map(float,result.stdout.split()))[3:5]==list(expected)
