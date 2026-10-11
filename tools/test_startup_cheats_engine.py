"""Run the actual map loader and game-thread cheat dispatch in freestanding wasm."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import wasm_test
import re
import shutil
import subprocess

root = Path(__file__).resolve().parent.parent
out = root / 'build/startup-cheats-engine-tests'
out.mkdir(parents=True, exist_ok=True)
source = (root / 'source/game/cheats.c').read_text()
header = (root / 'port/android/include/halo_startup_cheats.h').read_text()
java = (root / 'port/android/app/src/main/java/com/halo/decomp/StartupCheats.java').read_text()
assert re.findall(r'"((?:set cheat_|cheat_)[^"]+)"', header) == re.findall(r'"((?:set cheat_|cheat_)[^"]+)"', java), 'Java and engine commands must agree'

def function(name):
    m = re.search(r'^(?:static )?(?:void|boolean) '+name+r'\(', source, re.M)
    brace = source.index('{', m.start())
    depth, end = 1, brace+1
    while depth:
        depth += (source[end] == '{')-(source[end] == '}'); end += 1
    return source[m.start():end]

prefix = r'''
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define NULL ((void*)0)
static unsigned int android_startup_pending;
static int android_startup_flags[10];
static boolean android_startup_loaded;
struct {boolean deathless_player,jetpack,infinite_ammo,bump_possession,super_jump,
 reflexive_damage_effects,medusa,omnipotent,controller_enabled,bottomless_clip;} cheat;
static int spawned,client,shots[6],synced[16],commands_in[16];
static unsigned int requested;
struct player {long unit_index;};static struct player player;
long local_player_get_player_index(int i){return spawned?0:NONE;}
struct player *player_get(long i){player.unit_index=spawned?0:NONE;return &player;}
int network_game_distributed_client(void){return client;}
void cheats_network_client_enforce(void){if(client){boolean *f=(boolean*)&cheat;for(int i=0;i<10;i++)f[i]=0;}}
unsigned int host_touch_cheats_read(int *commands){unsigned int result=requested;requested=0;for(int i=0;i<16;i++)commands[i]=commands_in[i];return result;}
void host_touch_cheat_result(int i,int status){synced[i]=status;}
void host_touch_cheat_sync(int i,int status){synced[i]=status;}
void cheat_active_camouflage_local_player(int i){shots[0]++;}
void cheat_active_camouflage(void){shots[1]++;}
void cheat_all_powerups(void){shots[2]++;}
void cheat_all_vehicles(void){shots[3]++;}
void cheat_all_weapons(void){shots[4]++;}
void cheat_teleport_to_camera(void){shots[5]++;}
struct camera {struct {long cluster_index;} location;};static struct camera camera;
struct camera *observer_get_camera(int i){return &camera;}
int strcmp(char const*a,char const*b){while(*a&&*a==*b){a++;b++;}return *a-*b;}
unsigned long strcspn(char const*s,char const*q){unsigned long n=0;for(;s[n];n++)for(int i=0;q[i];i++)if(s[n]==q[i])return n;return n;}
void csmemset(void *d,int value,unsigned long n){char *p=d;while(n--)*p++=value;}
int sprintf(char*d,char const*format,char const*s){int i=0;while(s[i]){d[i]=s[i];i++;}d[i++]=' ';d[i++]='1';d[i]=0;return i;}
typedef struct {int index;} FILE;static FILE input;
static char const *lines[20];static int file_exists;
FILE *fopen(char const*path,char const*mode){input.index=0;return file_exists?&input:NULL;}
char *fgets(char *d,int size,FILE*f){char const*s=lines[f->index++];int i=0;if(!s)return NULL;while(s[i]&&i<size-1){d[i]=s[i];i++;}d[i]=0;return d;}
int fclose(FILE*f){return 0;}
'''
code = prefix+header+'\n'+function('android_startup_cheats_load')+'\n'+function('android_touch_cheats_update')+r'''
int run_tests(void){
 android_startup_cheats_load();if(android_startup_loaded||android_startup_pending)return 1;
 file_exists=1;lines[0]="set cheat_jetpack 1\n";lines[1]=ANDROID_CHEATS_BEGIN;
 lines[2]="set cheat_deathless_player 1\r\n";lines[3]="set cheat_infinite_ammo 0\n";
 for(int i=10;i<16;i++)lines[i-6]=android_startup_commands[i];
 lines[10]=ANDROID_CHEATS_END;lines[11]=NULL;
 android_startup_cheats_load();
 if(!android_startup_loaded||android_startup_pending!=0xfc00||android_startup_flags[0]!=1||android_startup_flags[1])return 2;
 android_touch_cheats_update();if(cheat.deathless_player||shots[4]||!android_startup_loaded)return 3;
 spawned=1;camera.location.cluster_index=NONE;android_touch_cheats_update();
 if(shots[5]||android_startup_pending!=(1u<<15))return 12;
 camera.location.cluster_index=0;android_touch_cheats_update();
 if(!cheat.deathless_player||cheat.jetpack||cheat.infinite_ammo||android_startup_loaded||android_startup_pending)return 4;
 for(int i=0;i<6;i++)if(shots[i]!=1)return 5;
 android_touch_cheats_update();for(int i=0;i<6;i++)if(shots[i]!=1)return 6;
 requested=1;commands_in[0]=0;android_touch_cheats_update();if(cheat.deathless_player)return 7;
 requested=1u<<14;android_touch_cheats_update();if(shots[4]!=2)return 8;
 /* A new map reloads selected flags/actions, even after pause-menu changes. */
 android_startup_cheats_load();android_touch_cheats_update();if(!cheat.deathless_player||shots[4]!=3)return 9;
 /* Clients never execute startup commands or bypass the existing host rules. */
 android_startup_cheats_load();client=1;android_touch_cheats_update();if(cheat.deathless_player||shots[4]!=3)return 10;
 /* A file without a managed block never changes unrelated cheat state. */
 client=0;lines[0]="set cheat_jetpack 1";lines[1]=NULL;android_startup_cheats_load();
 cheat.super_jump=1;android_touch_cheats_update();if(!cheat.super_jump||cheat.jetpack)return 11;
 return 0;
}
'''
(out / 'test.c').write_text(code)
ndk = Path('C:/Android/ndk/29.0.14206865/toolchains/llvm/prebuilt/windows-x86_64/bin')
subprocess.run([str(ndk/'clang.exe'),'--target=wasm32','-DHALO_ANDROID=1','-std=c99','-ffreestanding','-fno-builtin','-c',str(out/'test.c'),'-o',str(out/'test.o')],check=True)
subprocess.run([str(ndk/'ld.lld.exe'),'-flavor','wasm','--no-entry','--export=run_tests',str(out/'test.o'),'-o',str(out/'test.wasm')],check=True)
wasm_test.check(out/'test.wasm', 'Startup engine', 'Engine init parsing, spawn deferral, once-per-map execution, pause commands and client rules passed')
