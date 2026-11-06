"""DualSense production scheduling and game-context integration, with simulated SDL/engine inputs."""
from pathlib import Path
import pytest
from harness import function
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def source(path):
    return '\n'.join(line for line in (ROOT / path).read_text().splitlines()
                     if not line.startswith('#include')) + '\n'


RULES = source('port/linux/src/dualsense_rules.h') + source('port/linux/src/dualsense_rules.c')
HEADER = source('port/linux/src/dualsense.h')
PLATFORM = source('port/linux/src/dualsense.c')
COMMON = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
/* Exercise the portable SDL backend, also on Linux CI. */
#undef __linux__
'''
SDL = r'''
typedef unsigned long long Uint64;
typedef unsigned int SDL_JoystickID, SDL_PropertiesID;
typedef int SDL_SpinLock;
typedef struct SDL_Gamepad { unsigned id; int vendor, product, wireless, index; } SDL_Gamepad;
static SDL_Gamepad devices[2]={{1,0x054c,0x0ce6,1,2},{2,0x054c,0x0df2,0,-1}};
static Uint64 now;
static int triggers=1, lights, fail, fail_led, attempts, sent, rgb[3];
static unsigned long changes;
static unsigned char reports[1000][47];
static Uint64 sent_at[1000];
static unsigned long config_changes(void){return changes;}
static int config_boolean(char const *key){return triggers;}
static char const *config_string(char const *key){return !strcmp(key,"input.lightbar")?(lights?"player_color":"off"):"normal";}
static void platform_log(char const *fmt,...){(void)fmt;}
static void SDL_LockSpinlock(SDL_SpinLock *l){(void)l;}
static void SDL_UnlockSpinlock(SDL_SpinLock *l){(void)l;}
static Uint64 SDL_GetTicks(void){return now;}
static void SDL_Delay(unsigned ms){now+=ms;}
static SDL_JoystickID SDL_GetGamepadID(SDL_Gamepad *g){return g->id;}
static SDL_Gamepad *SDL_GetGamepadFromID(SDL_JoystickID id){return id==1?devices:id==2?devices+1:NULL;}
static char const *SDL_GetGamepadName(SDL_Gamepad *g){return "simulated";}
static SDL_PropertiesID SDL_GetGamepadProperties(SDL_Gamepad *g){return 1;}
#define SDL_PROP_GAMEPAD_CAP_RGB_LED_BOOLEAN "led"
static int SDL_GetBooleanProperty(unsigned p,char const *key,int fallback){return 1;}
static int SDL_GetGamepadVendor(SDL_Gamepad *g){return g->vendor;}
static int SDL_GetGamepadProduct(SDL_Gamepad *g){return g->product;}
#define SDL_JOYSTICK_CONNECTION_WIRELESS 1
static int SDL_GetGamepadConnectionState(SDL_Gamepad *g){return g->wireless;}
#define SDL_GAMEPAD_AXIS_RIGHT_TRIGGER 0
#define SDL_GAMEPAD_AXIS_LEFT_TRIGGER 1
static int SDL_GetGamepadAxis(SDL_Gamepad *g,int axis){return 0;}
static int SDL_GetGamepadPlayerIndex(SDL_Gamepad *g){return g->index;}
static int SDL_SendGamepadEffect(SDL_Gamepad *g,void const *body,int size){
 attempts++; if(fail)return 0; assert(size==47 && sent<1000);
 memcpy(reports[sent],body,47);sent_at[sent++]=now;return 1;
}
static int SDL_SetGamepadLED(SDL_Gamepad *g,unsigned char r,unsigned char v,unsigned char b){
 rgb[0]=r;rgb[1]=v;rgb[2]=b;return !fail && !fail_led;
}
static int SDL_SetGamepadPlayerIndex(SDL_Gamepad *g,int index){g->index=index;return !fail;}
static char const *SDL_GetError(void){return "simulated busy";}
'''
HELPERS = r'''
static struct dualsense_wish pistol(void){
 struct dualsense_wish w={0};struct dualsense_weapon_state state={0};
 state.kind=DUALSENSE_WEAPON_SEMI;w.playing=1;w.context=1;w.kick_trigger=1;
 dualsense_right_held(w.right,&state,100);dualsense_effect_off(w.left);
 w.kick_ms=dualsense_event(w.kick,state.kind,DUALSENSE_EVENT_SHOT,100);return w;
}
static void frame(int port,struct dualsense_wish *w,Uint64 time){now=time;dualsense_wish_set(port,w);dualsense_frame();}
static void connect(void){dualsense_port(0,devices,0,0);now=900;dualsense_frame();sent=attempts=0;}
'''


@pytest.mark.parametrize('offset', [1, 10, 34, 49])
def test_bluetooth_kick_waits_for_delivery(tmp_path, offset):
    run_c(tmp_path, COMMON + RULES + HEADER + SDL + PLATFORM + HELPERS + r'''
int main(void){struct dualsense_wish w=pistol();connect();
 frame(0,&w,1000);w.kicks=1;frame(0,&w,1000+OFFSET);
 frame(0,&w,1049);assert(sent==1);
 frame(0,&w,1050);assert(sent==2 && !memcmp(reports[1]+10,w.kick,11));
 assert(pads[0].kick_until==1085 && !pads[0].kick_pending);
 frame(0,&w,1086);assert(sent==2);frame(0,&w,1100);
 assert(sent==3 && !memcmp(reports[2]+10,w.right,11));return 0;}
'''.replace('OFFSET', str(offset)))


@pytest.mark.parametrize('cancel', ['pause', 'context', 'stale', 'disable'])
def test_pending_kicks_cancel_on_context_change(tmp_path, cancel):
    action = {
        'pause': 'w.playing=0;frame(0,&w,1050);assert(reports[sent-1][10]==5);',
        'context': 'w.context++;frame(0,&w,1050);',
        'stale': 'now=1400;dualsense_frame();assert(reports[sent-1][10]==5);',
        'disable': 'triggers=0;changes++;now=1020;dualsense_frame();assert(reports[sent-1][10]==5);'
    }[cancel]
    run_c(tmp_path, COMMON + RULES + HEADER + SDL + PLATFORM + HELPERS + r'''
int main(void){struct dualsense_wish w=pistol();connect();frame(0,&w,1000);
 w.kicks++;frame(0,&w,1001);assert(pads[0].kick_pending);
 ACTION
 assert(!pads[0].kick_pending && !pads[0].kick_until);return 0;}
'''.replace('ACTION', action))


def test_busy_retry_left_trigger_and_reset(tmp_path):
    run_c(tmp_path, COMMON + RULES + HEADER + SDL + PLATFORM + HELPERS + r'''
int main(void){struct dualsense_wish w=pistol();unsigned char held[11];connect();
 w.kick_trigger=2;memcpy(held,w.right,11);memcpy(w.left,held,11);dualsense_effect_off(w.right);
 frame(0,&w,1000);w.kicks++;fail=1;frame(0,&w,1050);
 assert(pads[0].kick_pending && !pads[0].kick_until && sent==1);
 frame(0,&w,1099);assert(attempts==2);fail=0;frame(0,&w,1100);
 assert(sent==2 && reports[1][10]==5 && !memcmp(reports[1]+21,w.kick,11));
 assert(pads[0].kick_until==1135);
 fail=1;triggers=0;changes++;now=1101;dualsense_frame();assert(pads[0].reset_pending);
 fail=0;now=1102;dualsense_frame();assert(!pads[0].reset_pending && reports[sent-1][21]==5);
 triggers=1;changes++;now=1103;dualsense_frame();assert(!pads[0].kick_until);return 0;}
''')


def test_no_replay_on_rebinding_or_clear_and_restore_lights(tmp_path):
    run_c(tmp_path, COMMON + RULES + HEADER + SDL + PLATFORM + HELPERS + r'''
int main(void){struct dualsense_wish w=pistol();connect();lights=1;changes++;
 w.red=100;w.player_lights=dualsense_player_lights(1);frame(0,&w,1000);
 assert(devices[0].index==0 && rgb[0]==100);
 w.kicks++;frame(0,&w,1001);dualsense_port(1,devices,0,0);
 assert(!shared.wished[0] && !shared.wished[1] && !shared.bound[0]);
 now=1050;dualsense_frame();assert(!pads[0].kick_pending && !pads[0].kick_until);
 frame(1,&w,1100);assert(!pads[0].kick_until);
 dualsense_clear();now=1150;dualsense_frame();now=1151;dualsense_frame();
 assert(reports[sent-1][10]==5 && devices[0].index==2 && rgb[0]==0 && rgb[1]==64 && rgb[2]==0);
 dualsense_removed(devices);assert(!shared.wished[1] && !shared.active[1]);return 0;}
''')


def test_busy_light_restore_keeps_original_player_index(tmp_path):
    run_c(tmp_path, COMMON + RULES + HEADER + SDL + PLATFORM + HELPERS + r'''
int main(void){struct dualsense_wish w=pistol();connect();lights=1;changes++;
 w.red=100;w.player_lights=dualsense_player_lights(1);frame(0,&w,1000);
 assert(devices[0].index==0);dualsense_clear();now=1050;dualsense_frame();
 fail=1;now=1100;dualsense_frame();assert(pads[0].reset_pending && pads[0].lights_owned);
 assert(devices[0].index==2);fail=0;now=1150;dualsense_frame();
 assert(!pads[0].lights_owned && devices[0].index==2 && rgb[1]==64);
 now=1200;dualsense_frame();assert(!pads[0].reset_pending && devices[0].index==2);return 0;}
''')


def test_partial_first_batch_keeps_reset_while_other_feature_runs(tmp_path):
    run_c(tmp_path, COMMON + RULES + HEADER + SDL + PLATFORM + HELPERS + r'''
int main(void){struct dualsense_wish w=pistol();lights=1;fail_led=1;
 dualsense_port(0,devices,0,0);w.player_lights=dualsense_player_lights(1);
 frame(0,&w,1000);assert(sent==1 && !pads[0].link.known);
 triggers=0;changes++;fail=1;now=1050;dualsense_frame();
 assert(pads[0].reset_pending & DUALSENSE_FIELD_TRIGGERS);
 fail=fail_led=0;now=1100;dualsense_frame();
 assert(!pads[0].reset_pending && sent==2 && reports[1][10]==5);return 0;}
''')


def test_other_controllers_and_default_off_are_untouched(tmp_path):
    run_c(tmp_path, COMMON + RULES + HEADER + SDL + PLATFORM + HELPERS + r'''
int main(void){struct dualsense_wish w=pistol();triggers=0;connect();frame(0,&w,1000);assert(!sent && !attempts);
 triggers=1;changes++;devices[0].vendor=123;now=1001;dualsense_frame();frame(0,&w,1100);
 assert(!sent && !attempts && !dualsense_port_active(0));return 0;}
''')


GAME_SOURCE = (ROOT / 'port/linux/game/dualsense_game.c').read_text()
GAME_STATE = GAME_SOURCE[GAME_SOURCE.index('/* Game-thread history'):GAME_SOURCE.index('static void clear_round')]
GAME = GAME_STATE + '\n'.join(function(GAME_SOURCE, name) for name in (
    'channel', 'clear_round', 'dualsense_game_reset', 'mapped_effect', 'dualsense_player', 'dualsense_game_update'))
GAME_FAKE = r'''
#define NONE (-1)
#define MAXIMUM_LOCAL_PLAYERS 4
typedef float real;
typedef struct {float red,green,blue;} real_rgb_color;
struct player_datum {long unit_index;short local_player_index;};
struct unit_datum {struct {short current_weapon_index,current_grenade_index,grenade_throw_state;} unit;};
struct weapon_datum {struct {long game_time_last_fired;} weapon;};
enum {_unit_grenade_throw_wind_up=1,_unit_grenade_throw_in_hand=2};
static struct player_datum players[4]={{10,0},{11,1},{12,2},{13,3}};
static struct unit_datum units[4];
static struct weapon_datum weapons[4];
static struct dualsense_weapon_state states[4];
static unsigned char mappings[4][CONTROLLER_REMAP_ACTIONS];
static struct dualsense_wish wishes[4];
static struct dualsense_settings settings={1,100,0};
static int paused,menu,inhibited,blocked[4],live[4]={1,1,1,1},active[4]={1,1,1,1},custom=1,cleared;
static int ticks=100;
static struct player_datum *player_get(long id){assert(id>=0 && id<4);return players+id;}
static struct unit_datum *unit_get(long id){assert(id>=10 && id<14);return units+id-10;}
static struct weapon_datum *weapon_get(long id){assert(id>=20 && id<24);return weapons+id-20;}
static long local_player_get_player_index(short local){return live[local]?local:NONE;}
static short controller_remap_local_player(short port){return live[port]?port:NONE;}
static int controller_remap_playing(short port){return !blocked[port];}
static int controller_remap_read(short port,unsigned char *layout){if(!custom)return 0;memcpy(layout,mappings[port],CONTROLLER_REMAP_ACTIONS);return 1;}
static void controller_remap_profile(short port,unsigned char *layout){memcpy(layout,mappings[port],CONTROLLER_REMAP_ACTIONS);}
static void controller_device_get(short port,struct controller_device *device){memset(device,0,sizeof(*device));device->instance=port+1;}
static struct dualsense_settings dualsense_settings_get(void){return settings;}
static int dualsense_port_active(int port){return active[port];}
static void dualsense_wish_set(int port,struct dualsense_wish const *wish){wishes[port]=*wish;}
static void dualsense_clear(void){memset(wishes,0,sizeof(wishes));cleared++;}
static int game_time_initialized(void){return 1;}
static int game_in_progress(void){return 1;}
static int main_menu_is_active(void){return menu;}
static int game_time_get_paused(void){return paused;}
static int game_time_get(void){return ticks;}
static int director_inhibited_input(short local){return inhibited;}
static int game_engine_running(void){return 0;}
static void game_engine_player_get_change_color(real_rgb_color *color,long player){assert(0);}
static short player_control_get_zoom_level(short local){return NONE;}
static long unit_inventory_get_weapon(long id,short slot){return id+10;}
static long seated_weapon(long unit,long weapon){return weapon;}
static int unit_get_grenade_count(long unit,int type){return 2;}
static void weapon_state(long weapon,struct dualsense_weapon_state *state){*state=states[weapon-20];}
static void setup(void){int i;for(i=0;i<4;i++){
 memset(mappings[i],255,CONTROLLER_REMAP_ACTIONS);mappings[i][PAD_FIRE]=7;mappings[i][PAD_GRENADE]=6;
 states[i].kind=DUALSENSE_WEAPON_SEMI;weapons[i].weapon.game_time_last_fired=10;
}}
'''
# The header's public declarations apply to the fake engine definitions too.
GAME_FAKE = GAME_FAKE.replace('static struct dualsense_settings dualsense_settings_get',
                              'struct dualsense_settings dualsense_settings_get')
for _name in ('dualsense_port_active', 'dualsense_wish_set', 'dualsense_clear',
              'controller_remap_local_player', 'controller_remap_playing', 'controller_remap_read',
              'controller_remap_profile', 'controller_device_get'):
    GAME_FAKE = GAME_FAKE.replace('static int ' + _name, 'int ' + _name).replace(
        'static short ' + _name, 'short ' + _name).replace('static void ' + _name, 'void ' + _name)


def game_code():
    return COMMON + RULES + HEADER + source('port/linux/include/controller_remap.h') + \
        source('port/linux/include/controller_buttons.h') + GAME_FAKE + GAME


@pytest.mark.parametrize('reason', ['pause', 'menu', 'director', 'widgets', 'death', 'disconnect'])
def test_gameplay_suppression_cancels_history(tmp_path, reason):
    block = {
        'pause': 'paused=1;', 'menu': 'menu=1;', 'director': 'inhibited=1;',
        'widgets': 'blocked[0]=1;', 'death': 'players[0].unit_index=NONE;', 'disconnect': 'active[0]=0;'
    }[reason]
    run_c(tmp_path, game_code() + r'''
int main(void){setup();dualsense_game_update();assert(wishes[0].playing && !wishes[0].kicks);
 weapons[0].weapon.game_time_last_fired++;dualsense_game_update();assert(wishes[0].kicks==1);
 BLOCK
 dualsense_game_update();assert(!wishes[0].playing && !rounds[0].valid);
 if(!menu)assert(wishes[0].right[0]==5 && wishes[0].left[0]==5);
 paused=menu=inhibited=blocked[0]=0;players[0].unit_index=10;active[0]=1;
 weapons[0].weapon.game_time_last_fired++;dualsense_game_update();
 assert(wishes[0].playing && !wishes[0].kicks);return 0;}
'''.replace('BLOCK', block))


@pytest.mark.parametrize('custom', [0, 1])
def test_physical_mapping_and_independent_ports(tmp_path, custom):
    run_c(tmp_path, game_code() + r'''
int main(void){setup();custom=CUSTOM;mappings[0][PAD_FIRE]=6;mappings[0][PAD_GRENADE]=7;
 mappings[1][PAD_FIRE]=0;mappings[1][PAD_GRENADE]=1;
 dualsense_game_update();assert(wishes[0].kick_trigger==2 && wishes[0].left[0]==0x25);
 assert(wishes[1].kick_trigger==0 && wishes[1].right[0]==5 && wishes[1].left[0]==5);
 assert(wishes[2].kick_trigger==1 && wishes[2].right[0]==0x25);
 weapons[0].weapon.game_time_last_fired++;dualsense_game_update();assert(wishes[0].kicks==1);
 mappings[0][PAD_FIRE]=0;dualsense_game_update();assert(wishes[0].right[0]!=0x25 && wishes[0].left[0]==5);
 assert(!wishes[0].kick_trigger && !wishes[0].kicks);return 0;}
'''.replace('CUSTOM', str(custom)))


def test_map_checkpoint_and_setting_resets_do_not_replay_shots(tmp_path):
    run_c(tmp_path, game_code() + r'''
int main(void){unsigned long context;setup();dualsense_game_update();context=wishes[0].context;
 weapons[0].weapon.game_time_last_fired++;dualsense_game_update();assert(wishes[0].kicks==1);
 dualsense_game_reset();assert(cleared && !rounds[0].valid);dualsense_game_update();
 assert(wishes[0].context!=context && !wishes[0].kicks);
 settings.triggers=0;dualsense_game_update();weapons[0].weapon.game_time_last_fired++;
 settings.triggers=1;dualsense_game_update();assert(!wishes[0].kicks);
 settings.triggers=0;settings.lights=1;dualsense_game_update();assert(!wishes[0].playing && wishes[0].player_lights);
 settings.triggers=1;weapons[0].weapon.game_time_last_fired++;dualsense_game_update();assert(!wishes[0].kicks);
 return 0;}
''')
