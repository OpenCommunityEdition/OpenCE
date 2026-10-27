"""Production remapping policy, physical input, capture and pending menu state."""
from pathlib import Path
import json
import re
import xml.etree.ElementTree as ET

from test_hud_button_style import run_c
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def source(path):
    return '\n'.join(line for line in (ROOT / path).read_text().splitlines()
                     if not line.startswith('#include')) + '\n'


COMMON = r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned long revision;
static char settings[4][192];
static unsigned long config_changes(void) {return revision;}
static char const *config_string(char const *name) {return settings[name[strlen(name)-1]-'1'];}
static int config_write(char const *name,char const *value) {
 strcpy(settings[name[strlen(name)-1]-'1'],value); revision++; return 1;
}
'''
POLICY = (source('port/linux/include/controller_buttons.h') +
          source('port/linux/include/controller_remap.h') + source('port/linux/src/controller_remap.c'))


def test_policy_roundtrip_swap_invalid_and_independent_ports(tmp_path):
    run_c(tmp_path, COMMON + POLICY + r'''
int main(void) {
 unsigned char a[10]={0,4,2,3,1,5,6,7,14,15},b[10],unchanged[10];
 char text[192]; int i,j;
 memcpy(unchanged,a,10);
 for(i=0;i<10;i++) for(j=0;j<10;j++) {
  memcpy(a,unchanged,10); controller_remap_swap(a,i,unchanged[j]);
  assert(a[i]==unchanged[j] && a[j]==unchanged[i]);
  controller_remap_format(a,text,sizeof(text));
  assert(controller_remap_parse(text,b) && !memcmp(a,b,10));
 }
 memcpy(b,unchanged,10);
 assert(!controller_remap_parse("",b));
 assert(!controller_remap_parse("south",b));
 assert(!controller_remap_parse("south,east,west,north,rb,lb,lt,rt,ls,ls",b));
 assert(!controller_remap_parse("south,east,west,north,rb,lb,lt,rt,ls,rs,",b));
 assert(!controller_remap_parse("south,east,west,north,rb,lb,lt,rt,ls,start",b));
 assert(!memcmp(b,unchanged,10));
 controller_remap_format(unchanged,text,sizeof(text)); config_write("input.controller_p2",text);
 assert(!controller_remap_read(0,b) && controller_remap_read(1,b) && !memcmp(b,unchanged,10));
 config_write("input.controller_p2","");assert(!controller_remap_read(1,b));
 assert(!controller_remap_read(-1,b) && !controller_remap_read(4,b));
 assert(!strcmp(controller_remap_button_name(0,CONTROLLER_PLAYSTATION),"CROSS"));
 assert(!strcmp(controller_remap_button_name(0,CONTROLLER_UNKNOWN),"A"));
 assert(!strcmp(controller_remap_button_name(2,CONTROLLER_UNKNOWN),"X"));
 assert(!strcmp(controller_remap_button_name(0,CONTROLLER_SWITCH),"B"));
 assert(!strcmp(controller_remap_button_name(5,CONTROLLER_PLAYSTATION),"L1"));
 assert(controller_remap_cycle(0,-1)==10 && controller_remap_cycle(10,1)==0);
 controller_remap_swap(a,0,10);controller_remap_format(a,text,sizeof(text));
 assert(controller_remap_parse(text,b) && b[0]==10);
 controller_remap_swap(b,0,12);assert(b[0]==10);
 return 0;
}
''')


INPUT = r'''
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long long Uint64;
typedef struct _XINPUT_GAMEPAD {
 WORD wButtons; BYTE bAnalogButtons[8]; short sThumbLX,sThumbLY,sThumbRX,sThumbRY;
} XINPUT_GAMEPAD;
#define XINPUT_GAMEPAD_DPAD_UP 1
#define XINPUT_GAMEPAD_DPAD_DOWN 2
#define XINPUT_GAMEPAD_DPAD_LEFT 4
#define XINPUT_GAMEPAD_DPAD_RIGHT 8
#define XINPUT_GAMEPAD_START 16
#define XINPUT_GAMEPAD_BACK 32
#define XINPUT_GAMEPAD_LEFT_THUMB 64
#define XINPUT_GAMEPAD_RIGHT_THUMB 128
static Uint64 now;
static int menu[4],key_result,key_cancels;
static unsigned int instance[4]={1,2,3,4};
static Uint64 SDL_GetTicks(void){return now;}
static void platform_binding_capture_begin(void) {key_result=0;}
static void platform_binding_capture_cancel(void) {key_result=0;key_cancels++;}
static int platform_binding_capture_poll(int *key) {int r=key_result;key_result=0;*key=0;return r;}
void controller_device_get(short port,struct controller_device *d){
 memset(d,0,sizeof(*d));
 if(port>=0 && port<4){d->instance=instance[port];if(instance[port])strcpy(d->name,"Test pad");}
}
void controller_remap_profile(short port,unsigned char *buttons) {
 static unsigned char const normal[10]={0,4,2,3,1,5,6,7,14,15};
 memcpy(buttons,normal,10);
 if(port==1){buttons[0]=5;buttons[5]=0;} /* Non-default profile. */
}
int controller_remap_playing(short port){return !menu[port];}
'''


def test_physical_remap_preserves_pressure_profile_menu_and_other_inputs(tmp_path):
    run_c(tmp_path, COMMON + POLICY + INPUT + source('port/linux/src/controller_remap_input.c') + r'''
int main(void) {
 XINPUT_GAMEPAD pad={0},stock,keyboard={0}; unsigned char b[10]; char text[192];
 controller_remap_profile(0,b);controller_remap_swap(b,0,1);
 controller_remap_format(b,text,sizeof(text));config_write("input.controller_p1",text);
 assert(!controller_remap_input(0,&pad,1));
 pad.bAnalogButtons[1]=177;pad.sThumbLX=12000;pad.wButtons=17;stock=pad;
 controller_remap_input(0,&pad,1);
 assert(pad.bAnalogButtons[0]==177 && !pad.bAnalogButtons[1] && pad.sThumbLX==12000 && pad.wButtons==17);
 keyboard.bAnalogButtons[1]=255;controller_remap_merge(&pad,&keyboard);
 assert(keyboard.bAnalogButtons[1]==255 && keyboard.bAnalogButtons[0]==177);
 menu[0]=1;pad=stock;controller_remap_input(0,&pad,1);assert(!memcmp(&pad,&stock,sizeof(pad)));
 menu[0]=0;pad=stock;controller_remap_input(0,&pad,1);assert(!pad.bAnalogButtons[0]);
 memset(&pad,0,sizeof(pad));controller_remap_input(0,&pad,1);
 pad=stock;controller_remap_input(0,&pad,1);assert(pad.bAnalogButtons[0]==177);
 pad=stock;controller_remap_input(2,&pad,1);assert(!memcmp(&pad,&stock,sizeof(pad)));
 config_write("input.controller_p2",text);memset(&pad,0,sizeof(pad));controller_remap_input(1,&pad,1);
 pad.bAnalogButtons[1]=255;controller_remap_input(1,&pad,1);assert(pad.bAnalogButtons[5]==255);
 config_write("input.controller_p1","");pad=stock;controller_remap_input(0,&pad,1);assert(!memcmp(&pad,&stock,sizeof(pad)));
 return 0;
}
''')


def test_disabled_matches_original_sdl_merge(tmp_path):
    original = (ROOT / 'port/linux/src/xinput_sdl.c').read_text()
    run_c(tmp_path, COMMON + POLICY + INPUT + r'''
typedef int BOOL;
typedef short SHORT;
typedef short Sint16;
#define FALSE 0
#define TRUE 1
#define XINPUT_GAMEPAD_A 0
#define XINPUT_GAMEPAD_B 1
#define XINPUT_GAMEPAD_X 2
#define XINPUT_GAMEPAD_Y 3
#define XINPUT_GAMEPAD_BLACK 4
#define XINPUT_GAMEPAD_WHITE 5
#define XINPUT_GAMEPAD_LEFT_TRIGGER 6
#define XINPUT_GAMEPAD_RIGHT_TRIGGER 7
typedef enum {SDL_GAMEPAD_BUTTON_DPAD_UP,SDL_GAMEPAD_BUTTON_DPAD_DOWN,
 SDL_GAMEPAD_BUTTON_DPAD_LEFT,SDL_GAMEPAD_BUTTON_DPAD_RIGHT,SDL_GAMEPAD_BUTTON_START,
 SDL_GAMEPAD_BUTTON_BACK,SDL_GAMEPAD_BUTTON_LEFT_STICK,SDL_GAMEPAD_BUTTON_RIGHT_STICK,
 SDL_GAMEPAD_BUTTON_SOUTH,SDL_GAMEPAD_BUTTON_EAST,SDL_GAMEPAD_BUTTON_WEST,SDL_GAMEPAD_BUTTON_NORTH,
 SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER} SDL_GamepadButton;
enum {SDL_GAMEPAD_AXIS_LEFT_TRIGGER,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,SDL_GAMEPAD_AXIS_LEFTX,
 SDL_GAMEPAD_AXIS_LEFTY,SDL_GAMEPAD_AXIS_RIGHTX,SDL_GAMEPAD_AXIS_RIGHTY};
typedef struct {short axes[6];int buttons[14];} SDL_Gamepad;
static int SDL_GetGamepadButton(SDL_Gamepad *p,int b){return p->buttons[b];}
static short SDL_GetGamepadAxis(SDL_Gamepad *p,int a){return p->axes[a];}
''' + '\n'.join(function(original, name) for name in ['stick', 'merge_button', 'sdl_gamepad_state']) +
          source('port/linux/src/controller_remap_input.c') + r'''
int main(void) {
 SDL_Gamepad device;XINPUT_GAMEPAD old,current,physical;int n,i;
 for(n=0;n<200;n++) {
  memset(&old,0,sizeof(old));memset(&physical,0,sizeof(physical));
  for(i=0;i<8;i++)old.bAnalogButtons[i]=(unsigned char)(n*7+i*33);
  old.wButtons=(unsigned short)n;old.sThumbLX=(short)(n*1091);old.sThumbRY=(short)(-n*719);
  current=old;
  for(i=0;i<6;i++)device.axes[i]=(short)(n*931+i*9271);
  device.axes[0]&=32767;device.axes[1]&=32767;
  for(i=0;i<14;i++)device.buttons[i]=(n>>(i%7))&1;
  sdl_gamepad_state(&device,&old);
  sdl_gamepad_state(&device,&physical);
  assert(!controller_remap_input(0,&physical,1));
  controller_remap_merge(&physical,&current);
  assert(!memcmp(&old,&current,sizeof(old)));
 }
 return 0;
}
''')


def test_capture_release_cancel_disconnect_timeout_and_port_isolation(tmp_path):
    run_c(tmp_path, COMMON + POLICY + INPUT + source('port/linux/src/controller_remap_input.c') + r'''
int main(void) {
 XINPUT_GAMEPAD pad={0};int button=-1;
 menu[0]=menu[1]=1;controller_remap_capture_begin(1);
 pad.bAnalogButtons[0]=255;assert(controller_remap_input(1,&pad,1));
 assert(!controller_remap_capture_poll(&button));
 pad.bAnalogButtons[1]=255;assert(controller_remap_input(0,&pad,1));
 assert(!controller_remap_capture_poll(&button));
 memset(&pad,0,sizeof(pad));controller_remap_input(1,&pad,1);
 pad.bAnalogButtons[1]=255;controller_remap_input(1,&pad,1);
 assert(controller_remap_capture_poll(&button)==1 && button==1 && key_cancels);
 pad.bAnalogButtons[1]=255;assert(controller_remap_input(1,&pad,1) && !pad.bAnalogButtons[1]);
 controller_remap_input(1,&pad,1);assert(!capture.phase);
 controller_remap_capture_begin(0);controller_remap_input(0,&pad,1);
 pad.wButtons=XINPUT_GAMEPAD_BACK;controller_remap_input(0,&pad,1);
 assert(controller_remap_capture_poll(&button)==3);
 controller_remap_input(0,&pad,1);
 controller_remap_capture_begin(0);key_result=3;assert(controller_remap_capture_poll(&button)==3);
 controller_remap_input(0,&pad,1);
 controller_remap_capture_begin(0);instance[0]++;controller_remap_input(0,&pad,1);
 assert(controller_remap_capture_poll(&button)==3);controller_remap_input(0,&pad,1);
 controller_remap_capture_begin(0);controller_remap_input(0,&pad,0);
 assert(controller_remap_capture_poll(&button)==3);controller_remap_input(0,&pad,0);
 controller_remap_capture_begin(0);now+=1001;controller_remap_input(0,&pad,1);
 assert(!capture.phase); /* Screen stopped polling; no stale capture. */
 controller_remap_capture_begin(3);instance[3]=0;
 assert(controller_remap_capture_poll(&button)==3 && !capture.phase);
 assert(!controller_remap_input(0,&pad,1)); /* Unplugged P4 is never polled. */
 instance[3]=4;controller_remap_capture_begin(3);controller_remap_capture_end();
 instance[3]=0;assert(!controller_remap_input(0,&pad,1) && !capture.phase);
 instance[3]=4;controller_remap_capture_begin(3);now+=1001;
 controller_remap_input(0,&pad,1);assert(capture.phase==4);
 controller_remap_input(3,&pad,1);assert(!capture.phase);
 return 0;
}
''')


def test_generated_controller_submenu_and_stock_keyboard_menu():
    import port_settings
    files = port_settings.settings_files()
    prefix = port_settings.PE.replace('/', '.')
    generated = files[prefix + '.controller_remapping.xml']
    root = ET.fromstring('\n'.join(generated))
    help_widgets = [widget for widget in root.iter('widget') if widget.get('name', '').endswith('/help')]
    assert help_widgets and all('string_list' not in widget.attrib for widget in help_widgets)
    assert 'ce/' + prefix + '.controller_remapping.xml' in json.loads(
        (ROOT / 'port/assets/menus/menus.json').read_text())['files']
    parent = ET.fromstring('\n'.join(files[prefix + '.gamepad_setup.xml']))
    assert any('controller_remapping_screen' in event.get('open', '') for event in parent.iter('on'))
    assert any(widget.get('name', '').endswith('/pad_player') and widget.get('strings') == 'P1|P2|P3|P4'
               for widget in root.iter('widget'))
    for filename in (prefix + '.controller_remapping.xml', prefix + '.gamepad_setup.xml', prefix + '.controls_setup.xml'):
        assert (ROOT / 'port/assets/menus/ce' / filename).read_text() == '\n'.join(files[filename])


def test_profile_fallback_context_and_viewport_prompt_mapping(tmp_path):
    run_c(tmp_path, COMMON + POLICY + r'''
#define NONE -1
#define csmemcpy memcpy
struct game_input_preferences {unsigned char game_control_to_xbox_buttons[12];};
static struct game_input_preferences profile;
static int in_game=1,menus,chat;
static struct {short local_player_index;} fake_player={2};
static void input_abstraction_get_local_player_preferences(short port,struct game_input_preferences *p){*p=profile;}
static int game_in_progress(void){return in_game;}
static int ui_widgets_active_for_local_player(short port){return menus;}
unsigned char network_chat_keyboard_open(void){return chat;}
static long local_player_get_player_index(short viewport){return viewport==1?7:NONE;}
#define player_get(index) (&fake_player)
''' + source('port/linux/game/controller_remap_game.c') + r'''
int main(void) {
 unsigned char b[10];struct game_input_preferences original;char text[192];int i;
 controller_remap_profile(0,b);assert(b[0]==0 && b[8]==14); /* Uninitialized profile. */
 for(i=0;i<10;i++)profile.game_control_to_xbox_buttons[controller_remap_controls[i]]=b[i];
 original=profile;controller_remap_swap(b,0,1);controller_remap_format(b,text,sizeof(text));
 config_write("input.controller_p3",text);
 controller_remap_prompt(1,&profile);assert(profile.game_control_to_xbox_buttons[0]==1);
 profile=original;controller_remap_prompt(0,&profile);assert(!memcmp(&profile,&original,sizeof(profile)));
 assert(controller_remap_playing(0));menus=1;assert(!controller_remap_playing(0));menus=0;
 chat=1;assert(!controller_remap_playing(0) && controller_remap_playing(1));
 chat=0;in_game=0;assert(!controller_remap_playing(0));
 return 0;
}
''')


def test_pending_editor_save_cancel_defaults_and_binding_swap(tmp_path):
    run_c(tmp_path, COMMON + POLICY + r'''
#include <wchar.h>
#define NONE -1
#define TRUE 1
#define FALSE 0
typedef int boolean;
struct widget_instance {
 char const *name;short local_player_index;int visible;
 struct widget_instance *parent,*child,*next,*focused_child;
 struct {struct {short selected_index;struct widget_instance *extended_description;} list;} parameters;
};
static struct widget_instance screen,player_spinner,group_spinner,row;
static struct widget_instance *screen_of(struct widget_instance *w){return &screen;}
static struct widget_instance *named(struct widget_instance *w,char const *name,int n) {
 if(!strcmp(name,"pad_player"))return &player_spinner;
 if(!strcmp(name,"pad_group"))return &group_spinner;return 0;
}
static short controller_of(struct widget_instance *w){return w->local_player_index;}
static void text_set(struct widget_instance *w,wchar_t const *t){}
static void text_set_length(struct widget_instance *w,wchar_t const *t,short length){}
static void focus_row(struct widget_instance *w,int n){}
static int capture_button,capture_result,capture_port=-1;
void controller_device_get(short port,struct controller_device *d){memset(d,0,sizeof(*d));}
void controller_remap_capture_begin(short port){capture_port=port;}
void controller_remap_capture_end(void){capture_port=-1;}
int controller_remap_capture_poll(int *button){int r=capture_result;*button=capture_button;capture_result=0;return r;}
void controller_remap_profile(short port,unsigned char *b){
 static unsigned char const normal[10]={0,4,2,3,1,5,6,7,14,15};memcpy(b,normal,10);
}
''' + source('port/linux/game/controller_remap_menu.inl') + r'''
int main(void) {
 unsigned char b[10];screen.local_player_index=1;row.name="op_pad_1";row.parent=&screen;
 pad_editor_event(&screen,"port controller init");assert(player_spinner.parameters.list.selected_index==1);
 pad_editor_event(&row,"port controller right");assert(pad_editor.buttons[1][0]==1 && !*settings[1]);
 pad_editor_event(&screen,"port controller close");pad_editor_event(&screen,"port controller init");
 assert(pad_editor.buttons[1][0]==0 && !pad_editor.custom[1]); /* Cancel discarded. */
 pad_editor_event(&row,"port controller bind");assert(capture_port==1);
 capture_button=5;capture_result=1;pad_editor_update(&screen);
 assert(pad_editor.buttons[1][0]==5 && pad_editor.buttons[1][5]==0);
 assert(pad_editor_event(&screen,"port controller save"));
 assert(controller_remap_read(1,b) && b[0]==5 && !*settings[0] && !*settings[2]);
 pad_editor_event(&screen,"port controller defaults");assert(!pad_editor.custom[1] && *settings[1]);
 pad_editor_event(&screen,"port controller save");assert(!*settings[1]);
 return 0;
}
''')



def test_all_upstream_presets_and_dpad_action_translation(tmp_path):
    """Use every production preset and exercise D-pad Left as a custom binding."""
    player_ui = (ROOT / 'source/interface/player_ui.c').read_text()
    presets = re.findall(r'case _button_preset_\w+:\s*((?:preferences\.game_control_to_xbox_buttons\[\d+\] = \w+;\s*)+)break;', player_ui)
    assert presets
    cases = '\n'.join(f'case {i}: {body} break;' for i, body in enumerate(presets))
    inputs = (ROOT / 'source/input/input.h').read_text()
    enums = inputs[inputs.index('FIRST_GAMEPAD_ANALOG_BUTTON ='):inputs.index('NUMBER_OF_GAMEPAD_BUTTONS,')]
    physical = INPUT[:INPUT.index('void controller_remap_profile(')]
    run_c(tmp_path, COMMON + POLICY + physical + r"""
#define NONE -1
#define csmemcpy memcpy
struct game_input_preferences {unsigned char game_control_to_xbox_buttons[12];};
static struct game_input_preferences profile;
static void input_abstraction_get_local_player_preferences(short port,struct game_input_preferences *p){*p=profile;}
static int game_in_progress(void){return 1;}
static int ui_widgets_active_for_local_player(short port){return menu[port];}
unsigned char network_chat_keyboard_open(void){return 0;}
static long local_player_get_player_index(short port){return NONE;}
static struct {short local_player_index;} fake_player;
#define player_get(index) (&fake_player)
""" + '#define PRESET_COUNT ' + str(len(presets)) + '\nenum {' + enums + '};\n' + source('port/linux/game/controller_remap_game.c') +
          source('port/linux/src/controller_remap_input.c') + r"""
static void select_preset(int preset) {
 struct game_input_preferences preferences;
 memset(&preferences,0,sizeof(preferences));
 switch(preset) {
""" + cases + r"""
 }
 profile=preferences;
}
int main(void) {
 unsigned char targets[10],layout[10],values[16];
 char text[192];XINPUT_GAMEPAD pad,original;int p,a,i,shift;
 for(p=0;p<PRESET_COUNT;p++) {
  select_preset(p);controller_remap_profile(0,targets);
  for(i=0;i<10;i++)assert(targets[i]==profile.game_control_to_xbox_buttons[controller_remap_controls[i]]);
  config_write("input.controller_p1","");
  memset(&pad,0,sizeof(pad));pad.wButtons=255;memset(pad.bAnalogButtons,173,8);original=pad;
  controller_remap_input(0,&pad,1);assert(!memcmp(&pad,&original,sizeof(pad)));
  for(shift=0;shift<11;shift++) {
   for(i=0;i<10;i++)layout[i]=remap_buttons[(i+shift)%11];
   controller_remap_format(layout,text,sizeof(text));config_write("input.controller_p1",text);
   for(a=0;a<10;a++) {
    memset(&pad,0,sizeof(pad));controller_remap_input(0,&pad,1);
    if(layout[a]<8)pad.bAnalogButtons[layout[a]]=173;
    else pad.wButtons=digital_masks[layout[a]-8];
    pad.wButtons|=XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_BACK|XINPUT_GAMEPAD_DPAD_UP;
    pad.sThumbLX=1234;controller_remap_input(0,&pad,1);physical_values(&pad,values);
    for(i=0;i<10;i++)assert(values[targets[i]]==(i==a?(layout[a]<8 && targets[i]<8?173:255):0));
    assert(pad.sThumbLX==1234 && (pad.wButtons&49)==49);
   }
  }
 }
 /* D-pad Left keeps movement when neither profile nor layout assigns it. */
 select_preset(0);controller_remap_profile(0,layout);controller_remap_format(layout,text,sizeof(text));
 config_write("input.controller_p1",text);memset(&pad,0,sizeof(pad));controller_remap_input(0,&pad,1);
 pad.wButtons=XINPUT_GAMEPAD_DPAD_LEFT;controller_remap_input(0,&pad,1);assert(pad.wButtons==4);
 /* It is also a capturable physical button. */
 menu[0]=1;controller_remap_capture_begin(0);memset(&pad,0,sizeof(pad));controller_remap_input(0,&pad,1);
 pad.wButtons=4;controller_remap_input(0,&pad,1);assert(controller_remap_capture_poll(&i)==1 && i==10);
 return 0;
}
""")
