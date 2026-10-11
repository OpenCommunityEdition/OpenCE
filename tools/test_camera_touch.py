"""Execute the real director input function: tap switching and flying swipe ownership."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import wasm_test
import re, shutil, subprocess

root=Path(__file__).resolve().parent.parent
out=root/'build/camera-touch-tests';out.mkdir(parents=True,exist_ok=True)
source=(root/'source/camera/director.c').read_text()
match=re.search(r'static boolean director_update_controls\(\s*short local_player_index,\s*struct camera_control \*controls\)\s*\{',source)
start=source.index('{',match.start());end=start+1;depth=1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
query_match=re.search(r'boolean director_flying_controls_active\(short local_player_index\)\s*\{',source)
query_start=source.index('{',query_match.start());query_end=query_start+1;depth=1
while depth:
    depth+=(source[query_end]=='{')-(source[query_end]=='}');query_end+=1
query=source[query_match.start():query_end]
player_source=(root/'source/game/player_control.c').read_text()
condition=re.search(r'if \(\s*!director_flying_controls_active.*?halo_linux_mouse_look\(gamepad_index, &mouse_yaw, &mouse_pitch\)\)',player_source,re.S).group(0)
player_poll='void poll_player_look(void){short local_player_index=0,gamepad_index=0;real mouse_yaw,mouse_pitch;'+condition+'{player_yaw+=mouse_yaw;player_pitch+=mouse_pitch;}}'
code=r'''
#define HALO_ANDROID 1
typedef int boolean;typedef unsigned char byte;typedef float real;
#define FALSE 0
#define TRUE 1
#define NONE (-1)
#define NULL ((void*)0)
#define MAXIMUM_NUMBER_OF_LOCAL_PLAYERS 4
#define TICKS_PER_SECOND 30
#define SET_FLAG(v,b,on) do{if(on)(v)|=1u<<(b);else(v)&=~(1u<<(b));}while(0)
enum{_gamepad_analog_button_black,_gamepad_binary_button_right_thumb,_gamepad_analog_button_right_trigger,_gamepad_analog_button_left_trigger,_gamepad_binary_button_dpad_up,_gamepad_binary_button_dpad_down};
enum{_gamepad_stick_left,_gamepad_stick_right};
enum{_camera_control_forward_bit,_camera_control_reverse_bit,_camera_control_left_bit,_camera_control_right_bit,_camera_control_up_bit,_camera_control_down_bit,_camera_control_roll_left_bit,_camera_control_roll_right_bit};
enum{_variable_height,_variable_roll,_variable_forward,_variable_right};
enum{_key_backspace,_key_tab,_key_w,_key_s,_key_a,_key_d,_key_r,_key_f,_key_t,_key_g};
typedef void(*director_camera_update_proc)(void);
struct camera_control{short local_player_index;boolean active;byte pad3;real seconds_elapsed;struct{real yaw,pitch,roll;}facing_delta;struct{real i,j,k;}position_delta;real wheel_delta;};
struct director{director_camera_update_proc camera_proc;boolean debug_controls;real debug_input_scale;struct{real delta;}debug_variables[4];};
struct gamepad_state{byte buttons[6];struct{short x,y;}sticks[2];};
struct mouse_state{long x,y,wheel_delta;byte buttons[2];};
static struct director d;static struct gamepad_state pad;
static struct player_datum{short local_player_index;}player;
static struct{real dtime;}director_globals;
static boolean director_camera_switch_fast;
static int pending,look_calls,inhibited,facing_inhibited;
static int simulate_consumption,look_pending;static real look_scale=1,player_yaw,player_pitch;
void first_person_camera_update(void){}void following_camera_update(void){}void flying_camera_update(void){}
struct director*director_get(short i){return &d;}
int local_player_get_player_index(short i){return i;}
struct player_datum*player_get(int i){return &player;}
int input_has_gamepad(short i){return TRUE;}
const struct gamepad_state*input_get_gamepad_state(short i){return &pad;}
const struct mouse_state*input_get_mouse_state(void){return NULL;}
int input_key_is_down(int key){return FALSE;}
void director_process_variables(short i,unsigned long flags,real delta){}
void director_inhibit_input(short i){inhibited++;}
void director_inhibit_facing(short i){facing_inhibited++;}
int halo_linux_mouse_look(short i,real*yaw,real*pitch){
 look_calls++;*yaw=*pitch=0;
 if(simulate_consumption&&!look_pending)return FALSE;
 look_pending=0;*yaw=-0.22f*look_scale;*pitch=0.11f*look_scale;return TRUE;
}
int host_touch_camera_read(void){int result=pending;pending=0;return result;}
void*csmemset(void*p,int v,unsigned long n){byte*q=p;while(n--)*q++=v;return p;}
'''+query+'\n'+player_poll+'\n'+source[match.start():end]+r'''
int run_tests(void){
 struct camera_control controls;player.local_player_index=0;director_globals.dtime=1.f/60;d.debug_input_scale=1;
 d.camera_proc=first_person_camera_update;pending=1;
 if(!director_update_controls(0,&controls)||pending||look_calls)return 1;
 if(director_update_controls(0,&controls))return 2;
 d.camera_proc=flying_camera_update;pad.buttons[_gamepad_binary_button_right_thumb]=1;
 pad.sticks[_gamepad_stick_left].y=16000;
 if(director_update_controls(0,&controls)||!d.debug_controls||!controls.active||look_calls!=1)return 3;
 if(controls.facing_delta.yaw!=-0.22f||controls.facing_delta.pitch!=0.11f||controls.position_delta.i<=0||!inhibited||!facing_inhibited)return 4;
 director_update_controls(0,&controls);
 if(!d.debug_controls||look_calls!=2)return 9;
 if(controls.position_delta.i<0.06f||controls.position_delta.i>0.07f)return 11;
 pad.buttons[_gamepad_binary_button_right_thumb]=5;director_update_controls(0,&controls);
 if(!d.debug_controls||look_calls!=3)return 10;
 pad.buttons[_gamepad_binary_button_right_thumb]=0;director_update_controls(0,&controls);
 if(look_calls!=4||!d.debug_controls)return 5;
 pad.buttons[_gamepad_binary_button_right_thumb]=1;director_update_controls(0,&controls);
 if(look_calls!=4||d.debug_controls||controls.active)return 6;
 pad.buttons[_gamepad_binary_button_right_thumb]=0;pad.buttons[_gamepad_analog_button_black]=30;
 if(!director_update_controls(0,&controls)||director_update_controls(0,&controls))return 7;
 pending=1;if(!director_update_controls(0,&controls)||director_update_controls(0,&controls))return 8;
 /* Actual frame order: player polls first, then the director reads the same
    accumulator. The player must leave it intact while the free camera owns it. */
 simulate_consumption=1;d.camera_proc=flying_camera_update;d.debug_controls=TRUE;
 pad.buttons[_gamepad_analog_button_black]=0;look_pending=1;int before=look_calls;
 poll_player_look();
 if(look_calls!=before||player_yaw||player_pitch||!look_pending)return 12;
 director_update_controls(0,&controls);
 if(look_pending||controls.facing_delta.yaw!=-0.22f||controls.facing_delta.pitch!=0.11f)return 13;
 look_scale=2;look_pending=1;poll_player_look();director_update_controls(0,&controls);
 if(controls.facing_delta.yaw!=-0.44f||controls.facing_delta.pitch!=0.22f)return 14;
 /* Normal camera uses that exact reader, including the same sensitivity. */
 d.camera_proc=first_person_camera_update;look_pending=1;poll_player_look();
 if(player_yaw!=-0.44f||player_pitch!=0.22f||look_pending)return 15;
 d.camera_proc=following_camera_update;
 if(director_flying_controls_active(0))return 16;
 d.camera_proc=flying_camera_update;d.debug_controls=FALSE;look_pending=1;poll_player_look();
 if(look_pending||director_flying_controls_active(0))return 17;
 return 0;
}
'''
(out/'test.c').write_text(code)
ndk=Path('C:/Android/ndk/29.0.14206865/toolchains/llvm/prebuilt/windows-x86_64/bin')
subprocess.run([str(ndk/'clang.exe'),'--target=wasm32','-DHALO_ANDROID=1','-std=c99','-ffreestanding','-c',str(out/'test.c'),'-o',str(out/'test.o')],check=True)
subprocess.run([str(ndk/'ld.lld.exe'),'-flavor','wasm','--no-entry','--export=run_tests',str(out/'test.o'),'-o',str(out/'test.wasm')],check=True)
wasm_test.check(out/'test.wasm', 'Camera', 'Camera tap, held grenade compatibility, flying swipe, Zoom ownership and movement passed')
