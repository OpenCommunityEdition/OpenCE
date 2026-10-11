"""Run the game's actual HUD placement with the panels the player moved (Edit in-game UI).

Builds hud_calculate_point from source/interface/hud_draw.c as the Android
build does (HALO_ANDROID) into a freestanding wasm test (tools/wasm_test.py runs
it): the weapons' panel, the health panel and the motion sensor move by their
saved fractions of the screen, and nothing else does. Where the panels draw
must still be checked on a device.
"""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import re, subprocess
import wasm_test

root = Path(__file__).resolve().parent.parent
out = root / 'build/hud-layout-tests'
out.mkdir(parents=True, exist_ok=True)
ndk = Path('C:/Android/ndk/29.0.14206865/toolchains/llvm/prebuilt/windows-x86_64/bin')
source = (root / 'source/interface/hud_draw.c').read_text()

def function(name):
    """the function's definition: the last use of its name that has a body"""
    for match in reversed(list(re.finditer(r'^\w[\w \*]*\b' + name + r'\s*\(', source, re.M))):
        brace = source.find('{', match.end())
        semicolon = source.find(';', match.end())
        if brace < 0 or (0 <= semicolon < brace):
            continue
        depth, end = 1, brace + 1
        while depth:
            depth += (source[end] == '{') - (source[end] == '}')
            end += 1
        return source[match.start():end]
    raise ValueError(name)

code = r'''
#define HALO_ANDROID 1
typedef float real;
typedef int boolean;
#define NULL ((void*)0)
#define TRUE 1
#define FALSE 0
#define FLAG(bit) (1<<(bit))
#define TEST_FLAG(flags,bit) (((flags)&FLAG(bit))!=0)
#define STACK_BUFFER_LENGTH 4
#define match_assert(file,line,expression) ((void)0)
#define match_assert_stack_frame(file,line) ((void)0)
#define _hud_anchor_right_bit 0
#define _hud_anchor_bottom_bit 1
enum hud_anchor {_hud_anchor_top_left,_hud_anchor_top_right,_hud_anchor_bottom_left,_hud_anchor_bottom_right,_hud_anchor_center};
typedef struct {short x,y;} point2d;
typedef struct {real x,y;} real_point2d;
typedef union {struct {short y0,x0,y1,x1;}; short v[4];} rectangle2d;
struct hud_absolute_placement_definition {short corner;};
struct hud_placement_definition {point2d offset;};
struct bitmap_data {short width,height;point2d registration_point;};
static struct {short local_player_index; struct {rectangle2d window_bounds,viewport_bounds;} camera;} render;
static int players=1,reads;
static float saved[6];
long get_return_eip(void){return 0;}
void csmemset(void*d,int v,unsigned long n){unsigned char*a=d;while(n--)*a++=(unsigned char)v;}
real hud_globals_get_scale(boolean multiplayer){return 1.0f;}
long fast_ftol(real value){return (long)value;}
short local_player_count(void){return (short)players;}
void host_touch_hud_read(float *offsets){int i;for(i=0;i<6;++i)offsets[i]=saved[i];++reads;}
static boolean hud_android_panel_offsets = TRUE;
''' + function('hud_android_panel_offsets_enable') + '\n' + function('hud_calculate_point') + r'''
static point2d place(short corner){
 struct hud_absolute_placement_definition absolute={corner};
 struct hud_placement_definition placement={{10,20}};
 point2d result;
 hud_calculate_point(0,&absolute,&placement,NULL,FALSE,0.0f,&result);
 return result;
}
static int at(short corner,short x,short y){point2d p=place(corner);return p.x==x&&p.y==y;}
int run_tests(void){
 render.camera.window_bounds.x0=0;render.camera.window_bounds.y0=0;
 render.camera.window_bounds.x1=1000;render.camera.window_bounds.y1=480;
 /* nothing moved: the game's own places */
 if(!at(_hud_anchor_top_left,10,20)||!at(_hud_anchor_top_right,990,20))return 1;
 if(!at(_hud_anchor_bottom_left,10,460)||!at(_hud_anchor_bottom_right,990,460)||!at(_hud_anchor_center,510,260))return 2;
 /* the weapons right and down, the health left and down, the radar right and up */
 saved[0]=0.125f;saved[1]=0.25f;saved[2]=-0.25f;saved[3]=0.5f;saved[4]=0.5f;saved[5]=-0.25f;
 if(!at(_hud_anchor_top_left,135,140))return 3;
 if(!at(_hud_anchor_top_right,740,260))return 4;
 if(!at(_hud_anchor_bottom_left,510,340))return 5;
 /* what the player cannot move stays */
 if(!at(_hud_anchor_bottom_right,990,460)||!at(_hud_anchor_center,510,260))return 6;
 /* a picture's own corner still counts from the moved point */
 {struct hud_absolute_placement_definition absolute={_hud_anchor_top_right};
  struct hud_placement_definition placement={{10,20}};struct bitmap_data bitmap={64,32,{4,2}};point2d result;
  hud_calculate_point(0,&absolute,&placement,&bitmap,FALSE,0.0f,&result);
  if(result.x!=740+4-64||result.y!=260+2)return 7;}
 /* the HUD's messages are no part of a panel (hud_messaging.c) */
 hud_android_panel_offsets_enable(FALSE);
 reads=0;if(!at(_hud_anchor_top_left,10,20)||!at(_hud_anchor_bottom_left,10,460)||reads)return 8;
 hud_android_panel_offsets_enable(TRUE);
 if(!at(_hud_anchor_top_left,135,140))return 9;
 /* a split screen keeps its layout: the panels are the touch player's whole screen's */
 players=2;if(!at(_hud_anchor_top_left,10,20)||!at(_hud_anchor_top_right,990,20)||!at(_hud_anchor_bottom_left,10,460))return 10;
 players=1;
 /* a window that does not start at the corner (the fractions are of its size) */
 render.camera.window_bounds.x0=100;render.camera.window_bounds.x1=900;
 render.camera.window_bounds.y0=40;render.camera.window_bounds.y1=440;
 if(!at(_hud_anchor_top_left,100+10+100,40+20+100))return 11;
 return 0;
}
'''
(out / 'test.c').write_text(code)
subprocess.run([str(ndk / 'clang.exe'), '--target=wasm32', '-std=c11', '-ffreestanding', '-fno-builtin', '-c',
                str(out / 'test.c'), '-o', str(out / 'test.o')], check=True)
subprocess.run([str(ndk / 'ld.lld.exe'), '-flavor', 'wasm', '--no-entry', '--export=run_tests',
                str(out / 'test.o'), '-o', str(out / 'test.wasm')], check=True)
wasm_test.check(out / 'test.wasm', 'HUD layout',
                'Moved weapons, health and radar panels, fixed messages, split screen and window origin passed')
