"""Exercise the real director selection and modifier-wheel routing."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def run_c(tmp_path, code):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    source = tmp_path / "check.c"
    source.write_text(code)
    exe = tmp_path / ("check.exe" if sys.platform == "win32" else "check")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else ["-lm"]
    built = subprocess.run([compiler, str(source), *flags, "-o", str(exe)], capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


def test_director_transitions(tmp_path):
    actual = (ROOT / "source/camera/director.c").read_text()
    feature = (ROOT / "port/linux/game/third_person_director.c").read_text()
    code = r'''
#include <stdio.h>
#include <string.h>
typedef int boolean;
typedef short director_perspective;
#define FALSE 0
#define TRUE 1
#define MAXIMUM_NUMBER_OF_LOCAL_PLAYERS 4
enum {_director_perspective_first_person, _director_perspective_third_person};
typedef void (*director_camera_update_proc)(void *,void *,void *);
struct first_person_camera {float fov;};
struct first_person_camera_action {short local_player;};
struct first_person_camera_result {int sentinel;};
struct following_camera {int unused;};
struct director {director_camera_update_proc camera_proc;float camera_change_pause,debug_input_scale;
 int perspective,seat_state,debug_controls;char camera_data[64];};
static struct director directors[4];
static int available[4],active[4],following,seat,passenger;
static int passenger_camera_enabled(short p){return p==0&&passenger;}
static struct director *director_get(short p){return &directors[p];}
static int third_person_available(short p){return available[p];}
static int third_person_active(short p){return active[p];}
static long player_control_get_unit_index(short p){return p;}
static short director_desired_perspective(long u,director_perspective *p){(void)u;*p=seat;return following;}
static void first_person_camera_new(struct first_person_camera *c){c->fov=1.2f;}
static void following_camera_new(struct following_camera *c){(void)c;}
static void first_person_camera_update(struct first_person_camera *c,struct first_person_camera_action const *a,
 struct first_person_camera_result *r){(void)c;r->sentinel=123+a->local_player;}
static void following_camera_update(void *a,void *b,void *c){(void)a;(void)b;(void)c;}
static void scripted_camera_update(void *a,void *b,void *c){(void)a;(void)b;(void)c;}
static void other_camera(void *a,void *b,void *c){(void)a;(void)b;(void)c;}
void third_person_camera_update(void *,void *,void *);
'''
    code += function(actual, "director_camera_is")
    code += re.sub(r"^#include[^\n]*", "", feature, flags=re.M)
    code += function(actual, "director_set_camera")
    code += function(actual, "director_get_perspective")
    code += function(actual, "director_choose_game_perspective")
    code += r'''
#define CHECK(c) do{if(!(c)){printf("line %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
 struct director *d=&directors[0];struct first_person_camera_action a={0};struct first_person_camera_result r={0};
 d->camera_proc=(director_camera_update_proc)first_person_camera_update;
 director_choose_game_perspective(0,0);CHECK(!third_person_director_active(0));
 available[0]=1;d->camera_change_pause=1;director_choose_game_perspective(0,0);
 CHECK(third_person_director_active(0)&&d->camera_change_pause==0);
 d->camera_proc(d->camera_data,&a,&r);CHECK(r.sentinel==123);
 active[0]=1;CHECK(director_get_perspective(0)==1);
 active[0]=0;CHECK(director_get_perspective(0)==0); /* same-frame collision fallback */
 active[0]=1;CHECK(director_get_perspective(0)==1);
 available[0]=0;CHECK(director_get_perspective(0)==0); /* scope/policy before next render */
 director_choose_game_perspective(0,0);CHECK(d->camera_proc==(director_camera_update_proc)first_person_camera_update);
 available[0]=1;director_choose_game_perspective(0,0);CHECK(third_person_director_active(0));
 available[0]=0;following=1;seat=2;director_choose_game_perspective(0,0);
 CHECK(d->camera_proc==following_camera_update); /* preserve the vehicle camera */
 following=0;seat=0;available[0]=1;director_choose_game_perspective(0,0);
 director_choose_game_perspective(0,0);CHECK(third_person_director_active(0));
 /* Passenger override can change without changing the seat identity. */
 available[0]=0;passenger=1;director_choose_game_perspective(0,0);
 CHECK(d->camera_proc==following_camera_update);
 passenger=0;director_choose_game_perspective(0,0);
 CHECK(d->camera_proc==(director_camera_update_proc)first_person_camera_update);
 available[0]=1;director_choose_game_perspective(0,0);CHECK(third_person_director_active(0));
 d->camera_proc=other_camera;director_choose_game_perspective(0,0);CHECK(d->camera_proc==other_camera);
 d->camera_proc=scripted_camera_update;director_choose_game_perspective(0,0);CHECK(d->camera_proc==scripted_camera_update);
 director_choose_game_perspective(0,1);CHECK(third_person_director_active(0)); /* script release */
 directors[1].camera_proc=(director_camera_update_proc)first_person_camera_update;
 director_choose_game_perspective(1,0);CHECK(!third_person_director_active(1)&&third_person_director_active(0));
 CHECK(!third_person_director_active(-1)&&!third_person_director_active(4));
 return 0;
}
'''
    run_c(tmp_path, code)


def test_camera_scroll_input(tmp_path):
    feature = (ROOT / "port/linux/game/third_person_input.c").read_text()
    platform = (ROOT / "port/linux/src/xinput_sdl.c").read_text()
    code = '#include "' + (ROOT / 'port/linux/include/halo_keyboard.h').as_posix() + '"\n'
    code += r"""
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define FALSE 0
#define TRUE 1
#define PIN(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))
#define DISTANCE (1UL<<HALO_KEYBOARD_CAMERA_DISTANCE)
#define HEIGHT (1UL<<HALO_KEYBOARD_CAMERA_HEIGHT)
#define OFFSET (1UL<<HALO_KEYBOARD_CAMERA_OFFSET)
#define MAXIMUM_BINDINGS 2
#define SDL_SCANCODE_COUNT 512
#define INPUT_MOUSE 512
#define INPUT_WHEEL 528
#define INPUT_WHEEL_UP 529
#define INPUT_WHEEL_DOWN 530
typedef int BOOL;
struct platform_input_state {int menus,mouse_released;float mouse_wheel;int keys[512],mouse_buttons[16];};
static int active=1,available=1,console,chat,writes,left;
static int mouse_lock,wheel_press_until_ms,wheel_moved_ms,wheel_scrolling,wheel_direction;
static float mouse_wheel_accumulated;
static double distance=.85,height=.12,offset=.24;
static int bindings[NUMBER_OF_HALO_KEYBOARD_ACTIONS][MAXIMUM_BINDINGS];
static int third_person_active(short p){return p==0&&active;}
static int third_person_available(short p){return p==0&&available;}
double config_real(char const *n){if(!strcmp(n,"display.third_person_distance"))return distance;
 if(!strcmp(n,"display.third_person_height"))return height;
 if(!strcmp(n,"display.third_person_offset"))return offset;abort();}
int config_boolean(char const *n){if(strcmp(n,"display.third_person_left_shoulder"))abort();return left;}
int config_write(char const *n,char const *v){writes++;
 if(!strcmp(n,"display.third_person_distance"))distance=atof(v);
 else if(!strcmp(n,"display.third_person_height"))height=atof(v);
 else if(!strcmp(n,"display.third_person_offset"))offset=atof(v);
 else if(!strcmp(n,"display.third_person_left_shoulder"))left=!strcmp(v,"true");
 else abort();return 1;}
static int console_is_active(void){return console;}
static int network_chat_typing(void){return chat;}
static int SDL_GetTicks(void){return 0;}
static void bindings_read(void){}
static void pthread_mutex_lock(int *m){(void)m;}
static void pthread_mutex_unlock(int *m){(void)m;}
"""
    code += re.sub(r"^#include[^\n]*", "", feature, flags=re.M)
    for name in ['input_held', 'keyboard_bound_actions', 'camera_scroll_capture']:
        code += function(platform, name)
    code += r"""
#define CHECK(c) do{if(!(c)){printf("line %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){struct platform_input_state i={0};int saved,axis,j;
 unsigned long actions[]={DISTANCE,HEIGHT,OFFSET};int keys[]={226,225,57};
 memset(bindings,-1,sizeof(bindings));
 bindings[HALO_KEYBOARD_CAMERA_DISTANCE][0]=226; /* SDL LALT */
 bindings[HALO_KEYBOARD_CAMERA_HEIGHT][0]=225; /* SDL LSHIFT */
 bindings[HALO_KEYBOARD_CAMERA_HEIGHT][1]=229; /* SDL RSHIFT */
 bindings[HALO_KEYBOARD_CAMERA_OFFSET][0]=57; /* SDL CAPSLOCK: physical state */
 i.keys[226]=1;i.mouse_wheel=1;
 wheel_scrolling=1;wheel_press_until_ms=100;mouse_wheel_accumulated=1;wheel_direction=1;wheel_moved_ms=10;
 camera_scroll_capture(&i);CHECK(fabs(distance-.77)<.00001&&i.mouse_wheel==0&&writes==1);
 CHECK(!wheel_scrolling&&!wheel_press_until_ms&&!mouse_wheel_accumulated&&!wheel_direction&&!wheel_moved_ms);
 i.mouse_wheel=-.5f;camera_scroll_capture(&i);CHECK(fabs(distance-.81)<.00001);
 i.keys[226]=0;i.keys[225]=1;i.mouse_wheel=1;camera_scroll_capture(&i);CHECK(height==.16&&offset==.24);
 i.keys[225]=0;i.keys[229]=1;i.mouse_wheel=-.5f;camera_scroll_capture(&i);CHECK(height==.14);
 i.keys[229]=0;i.keys[57]=1;i.mouse_wheel=-7;camera_scroll_capture(&i);CHECK(offset==.04&&left);
 i.mouse_wheel=1;camera_scroll_capture(&i);CHECK(offset==0&&left); /* retain shoulder at centre */
 i.mouse_wheel=.0001f;camera_scroll_capture(&i);CHECK(offset==0&&left);
 i.mouse_wheel=1;camera_scroll_capture(&i);CHECK(offset==.04&&!left);
 i.mouse_wheel=-.5f;camera_scroll_capture(&i);CHECK(offset==.02&&!left);
 /* Released Caps Lock never captures later scrolls, even after toggling it. */
 i.keys[57]=0;i.mouse_wheel=1;saved=writes;camera_scroll_capture(&i);CHECK(i.mouse_wheel==1&&writes==saved);
 CHECK(third_person_camera_scroll(1,DISTANCE|HEIGHT|OFFSET)&&distance==.73&&height==.14&&offset==.02);
 CHECK(third_person_camera_scroll(1,HEIGHT|OFFSET)&&height==.18&&offset==.02);
 for(axis=0;axis<3;axis++){
  for(j=0;j<512;j++)i.keys[j]=0;i.keys[keys[axis]]=1;i.mouse_wheel=1;saved=writes;
  i.menus=1;camera_scroll_capture(&i);CHECK(i.mouse_wheel==1&&writes==saved);
  i.menus=0;console=1;camera_scroll_capture(&i);CHECK(i.mouse_wheel==1&&writes==saved);
  console=0;chat=1;camera_scroll_capture(&i);CHECK(i.mouse_wheel==1&&writes==saved);
  chat=0;i.mouse_released=1;camera_scroll_capture(&i);CHECK(i.mouse_wheel==1&&writes==saved);
  i.mouse_released=0;active=0;camera_scroll_capture(&i);CHECK(i.mouse_wheel==1&&writes==saved);
  active=1;available=0;camera_scroll_capture(&i);CHECK(i.mouse_wheel==1&&writes==saved);
  available=1;
  CHECK(!third_person_camera_scroll(NAN,actions[axis])&&!third_person_camera_scroll(INFINITY,actions[axis]));
  CHECK(!third_person_camera_scroll(101,actions[axis])&&!third_person_camera_scroll(-101,actions[axis]));
  CHECK(!third_person_camera_scroll(0,actions[axis])&&writes==saved);
 }
 CHECK(third_person_camera_scroll(100,DISTANCE)&&distance==.3);
 CHECK(third_person_camera_scroll(-100,DISTANCE)&&distance==2);
 CHECK(third_person_camera_scroll(100,HEIGHT)&&height==.6);
 CHECK(third_person_camera_scroll(-100,HEIGHT)&&height==-.2);
 CHECK(third_person_camera_scroll(100,OFFSET)&&offset==.6&&!left);
 CHECK(third_person_camera_scroll(-100,OFFSET)&&offset==.6&&left);
 i.mouse_wheel=-1;camera_scroll_capture(&i);CHECK(i.mouse_wheel==0&&offset==.6&&left); /* limit still captures */
 distance=NAN;height=2;offset=-1;left=0;
 CHECK(third_person_camera_scroll(1,DISTANCE)&&distance==.77);
 CHECK(third_person_camera_scroll(1,HEIGHT)&&height==.16);
 CHECK(third_person_camera_scroll(1,OFFSET)&&offset==.28);
 CHECK(!third_person_camera_scroll(1,1UL<<HALO_KEYBOARD_FIRE));
 return 0;}
"""
    run_c(tmp_path, code)


def test_new_bindings_and_interpolation_wiring():
    config = (ROOT / "port/linux/src/port_config.c").read_text()
    for action, key in [("swap_shoulder", "B"), ("toggle_perspective", "N"),
                        ("camera_distance", "Left Alt"), ("camera_height", "Left Shift, Right Shift"),
                        ("camera_offset", "CapsLock"), ("push_to_talk", "V")]:
        assert f'"controls.{action}", _config_string, "\\"{key}\\""' in config
    platform = (ROOT / "port/linux/src/xinput_sdl.c").read_text()
    poll = function(platform, "XInputGetState")
    assert poll.index("keys_held_over_switch(&input)") < poll.index("camera_scroll_capture(&input)")
    assert poll.count("keys_held_over_switch(&input)") == 1
    assert poll.index("camera_scroll_capture(&input)") < poll.index("mouse_poll(&input)") < poll.index("wheel_update()")
    interpolation = function((ROOT / "port/linux/game/render_interpolation.c").read_text(), "render_interpolation_direct_camera")
    assert "!third_person_director_active(local_player_index)" in interpolation


def test_browser_build_includes_camera_modules():
    from test_web_build import graph, compile_flags
    text = graph().replace("\\", "/")
    for source in ["third_person_director.c", "third_person_input.c"]:
        flags = compile_flags(text, "port/linux/game/" + source)
        assert "-DHALO_WEB=1" in flags and "-ffp-contract=off" in flags
