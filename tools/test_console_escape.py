"""Check console Escape ownership through production keyboard and main-loop code.

--baseline-ref upstream/main reproduces the old failure without opening a game.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def block(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


PRELUDE = r'''
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int boolean,BOOL;typedef unsigned DWORD;typedef unsigned char byte;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define WINAPI
#define ERROR_SUCCESS 0
#define ERROR_HANDLE_EOF 38
#define XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP 1
#define XINPUT_DEBUG_KEYSTROKE_FLAG_SHIFT 4
#define XINPUT_DEBUG_KEYSTROKE_FLAG_CTRL 8
#define XINPUT_DEBUG_KEYSTROKE_FLAG_ALT 16
#define VK_OEM_3_BACKQUOTE 0xc0
#define XDEVICE_TYPE_DEBUG_KEYBOARD 1
#define XDEVICE_NO_SLOT 0
#define LONG_BITS 32
#define TICKS_PER_SECOND 30
#define UNSIGNED_CHAR_MAX 255
#define MAXIMUM_BUFFERED_KEYSTROKES 64
#define MAXIMUM_NUMBER_OF_PREVIOUS_COMMANDS 8
#define NUMBER_OF_VIRTUAL_CODES 256
#define NUMBER_OF_ASCII_CODES 128
#define FLAG(n) (1u<<(n))
#define SET_FLAG(v,b,s) ((v)=(s)?((v)|FLAG(b)):((v)&~FLAG(b)))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define PIN(v,a,b) MAX((a),MIN((v),(b)))
#define csmemset memset
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);exit(1);}}while(0)
#define match_assert(file,line,c) CHECK(c)
enum {_key_modifier_control=1,_key_modifier_alt=2};
struct key_stroke {short key_code,ascii_code,modifier_flags;};
struct platform_keystroke {int virtual_key,ascii,flags;};
typedef struct {int VirtualKey,Ascii,Flags;} XINPUT_DEBUG_KEYSTROKE,*PXINPUT_DEBUG_KEYSTROKE;
static struct platform_keystroke queue[64];static int read_at,write_at;
static int text_typing,now,engine,editor,editor_exit,movie_stops,commands,completions,closes;
static struct {int switch_to_structure_bsp_index,saving_map,reset_map,lost_map;} main_globals;
static int platform_next_keystroke(struct platform_keystroke *k){if(read_at==write_at)return 0;*k=queue[read_at++];return 1;}
static void enqueue(int vk,int flags){CHECK(write_at<64);queue[write_at++]=(struct platform_keystroke){vk,0,flags};}
static long system_milliseconds(void){return now;}
static int XGetDeviceChanges(int type,unsigned long *a,unsigned long *b){(void)type;(void)a;(void)b;return 0;}
static void XInputClose(void *handle){(void)handle;}
static void *XInputOpen(int a,long b,int c,void *d){(void)a;(void)b;(void)c;(void)d;return NULL;}
static int GetLastError(void){return 0;}
#define _error_silent 0
static void error(int level,const char *message,...){(void)level;(void)message;CHECK(0);}
static int game_in_editor(void){return editor;}
static int editor_should_exit(void){return editor_exit;}
static int game_engine_running(void){return engine;}
static void main_movie_stop(void){movie_stops++;}
struct edit_text {int unused;};
struct terminal_gets_state {char result[512];short key_count;struct key_stroke keys[64];struct edit_text edit;};
static boolean terminal_gets_begin(struct terminal_gets_state *s){s->key_count=0;return TRUE;}
static void terminal_gets_end(struct terminal_gets_state *s){(void)s;closes++;}
static void edit_text_selection_reset(struct edit_text *e){(void)e;}
static int profile_global_enable;
static boolean console_process_command(const char *s){(void)s;commands++;return TRUE;}
static void console_complete(void){completions++;}
enum {SDL_SCANCODE_ESCAPE,SDL_SCANCODE_END,SDL_SCANCODE_W,SDL_SCANCODE_COUNT=512};
struct platform_input_state {byte keys[SDL_SCANCODE_COUNT];int menus;};
'''
TESTS = r'''
static unsigned probes;
static void reset(void){
 memset(&input_globals,0,sizeof(input_globals));memset(&console_globals,0,sizeof(console_globals));
 memset(&main_globals,0,sizeof(main_globals));memset(consumed_keys,0,sizeof(consumed_keys));
 read_at=write_at=engine=editor=editor_exit=movie_stops=commands=completions=closes=0;now=0;
 memset(input_globals.gamepad_states,0x35,sizeof(input_globals.gamepad_states));
}
static void tick(void){now+=16;input_update_keyboard_devices();read_at=write_at=0;}
static void terminal_input(void){
 console_globals.input_state.key_count=0;
 struct key_stroke key;while(input_get_key(&key))console_globals.input_state.keys[console_globals.input_state.key_count++]=key;
}
static void press_escape(int fast){enqueue(0x1b,0);if(fast)enqueue(0x1b,XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP);tick();}
static void escape_close(int fast,int multiplayer){
 reset();engine=multiplayer;console_open();strcpy(console_globals.input_state.result,"(quit)");
 unsigned char controllers[sizeof(input_globals.gamepad_states)];memcpy(controllers,input_globals.gamepad_states,sizeof(controllers));
 press_escape(fast);reset_hotkey();terminal_input();console_update();
 if(BASELINE){CHECK(main_globals.reset_map==!multiplayer);CHECK(console_is_active());return;}
 CHECK(!main_globals.reset_map&&!movie_stops&&!console_is_active()&&closes==1&&commands==0);
 CHECK(!memcmp(controllers,input_globals.gamepad_states,sizeof(controllers)));
 CHECK(!input_key_is_down(_key_escape));
 for(int frame=0;frame<300;frame++){
  if(frame%10==0)enqueue(0x1b,2); /* auto-repeat, filtered after close */
  tick();reset_hotkey();CHECK(!main_globals.reset_map);CHECK(!input_key_is_down(_key_escape));probes++;
 }
 enqueue(0x1b,XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP);tick();
 console_open();press_escape(0);terminal_input();console_update();CHECK(!console_is_active());
}
static void end_and_other_close(void){
 for(int closing=0;closing<3;closing++){
  reset();console_open();enqueue(0x23,0);tick();reset_hotkey();
  CHECK(!main_globals.reset_map);CHECK(input_key_is_down(_key_end));
  if(closing==0)console_close();
  else{enqueue(closing==1?VK_OEM_3_BACKQUOTE:0x0d,0);tick();terminal_input();console_update();}
  CHECK(!console_is_active());
  for(int frame=0;frame<300;frame++){tick();reset_hotkey();CHECK(!main_globals.reset_map&&!input_key_is_down(_key_end));probes++;}
  enqueue(0x23,XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP);tick();console_open();enqueue(0x23,0);tick();CHECK(input_key_is_down(_key_end));
 }
 /* Commands and completion retain their normal behavior. */
 reset();console_open();strcpy(console_globals.input_state.result,"fov_desired 100");
 enqueue(0x0d,0);tick();terminal_input();console_update();CHECK(commands==1&&console_is_active());
 enqueue(0x09,0);tick();terminal_input();console_update();CHECK(completions==1&&console_is_active());
}
static void consumption(void){
 /* Releasing on the first frame after close must not expose a new pulse. */
 reset();console_open();press_escape(0);reset_hotkey();terminal_input();console_update();
 enqueue(0x1b,XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP);tick();reset_hotkey();
 CHECK(!console_is_active()&&!main_globals.reset_map&&!input_key_is_down(_key_escape));
 /* Escape cancels the remaining command keys in the same buffered batch. */
 reset();console_open();strcpy(console_globals.input_state.result,"(quit)");
 enqueue(0x1b,0);enqueue(0x0d,0);tick();terminal_input();console_update();
 CHECK(!console_is_active()&&!commands);
 reset();input_globals.key_latches[_key_escape]=TRUE;input_globals.key_ticks[_key_escape]=5;
 input_globals.key_latches[_key_w]=TRUE;input_globals.key_ticks[_key_w]=7;
 input_consume_key(_key_escape);CHECK(!input_key_is_down(_key_escape));CHECK(input_key_is_down(_key_w)==7);
 console_open();enqueue(0x1b,2);tick();CHECK(input_globals.buffered_key_write_index==0&&!input_key_is_down(_key_escape));
 enqueue(0x1b,XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP);tick();CHECK(!consumed_keys[_key_escape]);
 enqueue(0x1b,0);tick();CHECK(input_key_is_down(_key_escape));
 /* A full flush clears consumption; invalid keys cannot touch state. */
 input_consume_key(_key_escape);input_flush();CHECK(!consumed_keys[_key_escape]);
 input_consume_key(-1);input_consume_key(NUMBER_OF_KEYS);CHECK(!input_key_is_down(_key_escape));
}
static void ownership(void){
 for(int menu=0;menu<2;menu++)for(int fast=0;fast<2;fast++){
  reset();struct platform_input_state input={.menus=menu};keys_held_over_switch(&input);
  console_open();keys_held_over_switch(&input);
  input.keys[SDL_SCANCODE_ESCAPE]=1;input.keys[SDL_SCANCODE_W]=1;keys_held_over_switch(&input);
  console_close();
  for(int frame=0;frame<300;frame++){
   input.keys[SDL_SCANCODE_ESCAPE]=!fast;input.keys[SDL_SCANCODE_W]=!fast;keys_held_over_switch(&input);
   CHECK(!input.keys[SDL_SCANCODE_ESCAPE]&&!input.keys[SDL_SCANCODE_W]);probes++;
  }
  memset(input.keys,0,sizeof(input.keys));keys_held_over_switch(&input);
  input.keys[SDL_SCANCODE_ESCAPE]=1;keys_held_over_switch(&input);CHECK(input.keys[SDL_SCANCODE_ESCAPE]);
  /* The existing pause/menu transition still swallows the opening press. */
  input.menus=!menu;keys_held_over_switch(&input);CHECK(!input.keys[SDL_SCANCODE_ESCAPE]);
  memset(input.keys,0,sizeof(input.keys));keys_held_over_switch(&input);
  input.keys[SDL_SCANCODE_ESCAPE]=1;keys_held_over_switch(&input);CHECK(input.keys[SDL_SCANCODE_ESCAPE]);
 }
}
static void legacy_and_editor(void){
 reset();input_globals.key_ticks[_key_escape]=1;reset_hotkey();CHECK(main_globals.reset_map&&movie_stops==1);
 reset();input_globals.key_ticks[_key_end]=1;reset_hotkey();CHECK(main_globals.reset_map);
 reset();engine=1;input_globals.key_ticks[_key_escape]=1;reset_hotkey();CHECK(!main_globals.reset_map&&movie_stops==1);
 reset();editor=1;input_globals.key_ticks[_key_escape]=1;reset_hotkey();CHECK(!main_globals.reset_map);
 console_open();editor_exit=1;reset_hotkey();CHECK(main_globals.reset_map&&movie_stops==1);
}
int main(void){
 for(int fast=0;fast<2;fast++)for(int multiplayer=0;multiplayer<2;multiplayer++)escape_close(fast,multiplayer);
 if(BASELINE){puts("REPRODUCED: console Escape requests campaign reset before console update and does not close the console");return 0;}
 end_and_other_close();consumption();ownership();legacy_and_editor();
 printf("PASS: %u held-input frames; console Escape closes without reset/pause spill, End editing/other close paths, repeat/release/fast taps, normal command/completion, keyboard menu transitions, unchanged co-op controller data and legacy/editor hotkeys\n",probes);
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cc', default='clang')
    parser.add_argument('--baseline-ref')
    args = parser.parse_args()

    def read(path):
        if args.baseline_ref:
            return subprocess.check_output(['git', 'show', f'{args.baseline_ref}:{path}'], cwd=ROOT, text=True)
        return (ROOT/path).read_text()

    inputs = read('source/input/input_xbox.c')
    console = read('source/main/console.c')
    port = read('port/linux/src/xinput_sdl.c')
    main_loop = read('source/main/main.c')
    key_header = read('source/input/input.h')
    start = key_header.rfind('enum', 0, key_header.index('_key_escape = 0'))
    keys = block(key_header[start:], 'enum') + ';\n'
    globals_source = r'''
static struct {void *keyboard_handle;byte key_ticks[NUMBER_OF_KEYS],key_latches[NUMBER_OF_KEYS];
 boolean suppressed;short buffered_key_read_index,buffered_key_write_index;
 struct key_stroke buffered_keys[MAXIMUM_BUFFERED_KEYSTROKES];byte gamepad_states[4][32];} input_globals;
static long key_down_times[NUMBER_OF_KEYS];
static boolean consumed_keys[NUMBER_OF_KEYS];
'''
    source = PRELUDE + keys + globals_source
    source += block(console, 'struct console_globals') + ';\nstatic struct console_globals console_globals;\n'
    source += block(console, 'boolean console_is_active(') + '\n'
    for marker in ('static const short virtual_key_to_key_code', 'static const short ascii_to_key_code'):
        start = inputs.index(marker)
        source += inputs[start:inputs.index(';', start)+1] + '\n'
    for marker in ('static void update_hold_ticks(', 'void input_flush(', 'boolean input_key_is_down(', 'boolean input_get_key('):
        source += block(inputs[inputs.index('/* ---------- public code */'):], marker) + '\n'
    # The device poll calls the queue adapter declared later in this harness.
    source += 'DWORD WINAPI XInputDebugGetKeystroke(PXINPUT_DEBUG_KEYSTROKE);\n'
    source += block(inputs[inputs.index('/* ---------- private code */'):], 'static void input_update_keyboard_devices(') + '\n'
    source += (block(inputs, 'void input_consume_key(') if not args.baseline_ref else 'static void input_consume_key(short key){(void)key;}') + '\n'
    source += block(port, 'DWORD WINAPI XInputDebugGetKeystroke(') + '\n'
    for marker in ('void console_open(', 'void console_close(', 'boolean console_update('):
        source += block(console, marker) + '\n'
    source += block(port, 'static void keys_held_over_switch(') + '\n'
    source += block(main_loop, 'void main_reset_map(') + '\n'
    source += 'static void reset_hotkey(void){' + block(main_loop, 'if ((!game_in_editor() &&') + '}\n'
    source += f'\n#define BASELINE {int(bool(args.baseline_ref))}\n' + TESTS
    flags = ['-std=gnu11', '-O2', '-fuse-ld=lld']
    flags += ['--target=i686-pc-windows-msvc'] if sys.platform == 'win32' else ['-m32']
    with tempfile.TemporaryDirectory(prefix='halo-console-escape-') as directory:
        path = Path(directory)
        (path/'test.c').write_text(source)
        exe = path/'test.exe'
        subprocess.run([args.cc, *flags, str(path/'test.c'), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=15)


if __name__ == '__main__':
    main()
