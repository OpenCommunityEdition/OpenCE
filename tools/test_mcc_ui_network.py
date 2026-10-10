"""Exercise the MCC restart transaction across the round-lifecycle boundary."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def restart_tool(tmp_path_factory):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is needed")
    directory = tmp_path_factory.mktemp("mcc_ui_network")
    (directory / "mcc_cache.h").write_text("")
    implementation = (ROOT / "port/linux/game/mcc_ui_network.inl").read_text()
    server_source = (ROOT / "source/networking/network_server_manager.c").read_text()
    widget_source = (ROOT / "source/interface/ui_widget.c").read_text()
    source = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define _error_silent 0
#define NETWORK_SERVER_MANAGER_FILE "network_server_manager.c"
#define match_assert(file,line,condition) assert(condition)
#define MAXIMUM_NETWORK_PLAYER_COUNT 3
#define csmemcpy memcpy
#define csmemset memset
enum { _network_game_server_state_pregame, _network_game_server_state_ingame, _network_game_server_state_postgame };
struct network_player { int machine_index, controller_index, player_list_index, team_index; };
struct network_game {
    struct { int version; char name[128]; } map;
    int variant, variant_options, cooperative_flags, minimum_players, maximum_players, maximum_teams, difficulty;
    char name[32];
    struct network_player players[3];
    int number_of_games_played, local_data, machine_count;
};
struct network_game_server { int state; struct network_game game;
    struct {int timer;boolean active,paused,adjusted_time_this_tick;} countdown_state;
} server,other_server;
static int exists=1,loaded=1,fail_reset,countdowns,opened,switched,reset,leaver;
static int splitscreen,quickstart,closed_ui,loaded_ui,errors;
static struct network_game_server *selected_server=&server;
static struct network_game original;
static char map_name[128],screen_name[256];
static struct network_game_server *global_network_game_server_get(void) {return exists ? selected_server : NULL;}
static int mcc_cache_tags_loaded(void) {return loaded;}
static int mcc_level_name(char const *name) {return !strncmp(name,"mcc_maps\\",9);}
static int network_player_is_valid(struct network_player const *p) {return p->machine_index>=0;}
static void main_set_multiplayer_map_name(char const *name) {strcpy(map_name,name);}
static void network_game_server_open_game(struct network_game_server *s) {(void)s;opened++;}
static void network_game_server_switch_to_postgame(struct network_game_server *s) {s->state=_network_game_server_state_postgame;switched++;}
static void network_event(char const *s) {(void)s;}
static void countdown_timer_set_time_remaining(int *timer,long ms) {*timer=(int)ms;}
static void network_game_server_countdown_started(struct network_game_server *s) {
    assert(s->state==_network_game_server_state_pregame);countdowns++;
}
static void ui_widgets_close_all(void) {closed_ui++;}
static boolean network_game_is_splitscreen_local(void) {return splitscreen;}
static boolean network_game_is_quickstart_local(void) {return quickstart;}
static char const *pc_menus_screen(char const *name) {return name;}
static boolean ui_widget_load_by_name_or_tag(char const *name,long tag,void *parent,long controller,long a,long b,long c) {
    (void)tag;(void)parent;(void)controller;(void)a;(void)b;(void)c;
    strcpy(screen_name,name);loaded_ui++;return TRUE;
}
static void error(int priority,char const *text,...) {(void)priority;(void)text;errors++;}
boolean mcc_ui_network_restart_settings(struct network_game_server *s);
static int network_game_server_reset_to_pregame(struct network_game_server *s) {
    int i;reset++;
    memset(&s->countdown_state,0,sizeof(s->countdown_state));
    s->game.number_of_games_played++;
    if(fail_reset)return 0;
    s->game.local_data=0;
    for(i=0;i<3;i++)s->game.players[i].team_index=1-s->game.players[i].team_index;
    if(leaver)s->game.players[2].machine_index=-1;
    if(!mcc_ui_network_restart_settings(s))exit(81);
    /* Existing reset sends this exact state to every machine. */
    if(strcmp(s->game.map.name,original.map.name) || s->game.variant!=original.variant ||
       s->game.variant_options!=original.variant_options || s->game.difficulty!=original.difficulty ||
       s->game.cooperative_flags!=original.cooperative_flags || strcmp(s->game.name,original.name) ||
       s->game.map.version!=original.map.version || s->game.machine_count!=original.machine_count ||
       s->game.minimum_players!=original.minimum_players || s->game.maximum_players!=original.maximum_players ||
       s->game.maximum_teams!=original.maximum_teams)exit(82);
    s->state=_network_game_server_state_pregame;return 1;
}
''' + '\n'.join(function(server_source, name) for name in [
        "network_game_server_pause_countdown", "network_game_server_begin_game_start_countdown"
    ]) + '\n' + implementation + '\n' + function(widget_source, "network_game_reset_to_pregame_ui") + r'''
#define CHECK(c,n) do {if(!(c))return n;} while(0)
int main(int argc,char **argv) {
    int i,result;struct network_game_server untouched;
    if(argc!=2)return 99;
    server.state=_network_game_server_state_ingame;
    strcpy(server.game.map.name,"mcc_maps\\a10");server.game.map.version=123;
    strcpy(server.game.name,"MCC game");
    server.game.variant=atoi(argv[1])==1 ? 0 : 7;
    server.game.variant_options=13;server.game.cooperative_flags=1;
    server.game.minimum_players=2;server.game.maximum_players=16;
    server.game.maximum_teams=2;server.game.difficulty=3;
    server.game.number_of_games_played=4;server.game.local_data=1;server.game.machine_count=3;
    for(i=0;i<3;i++) {server.game.players[i].machine_index=i;server.game.players[i].controller_index=0;
      server.game.players[i].player_list_index=i;server.game.players[i].team_index=i%2;}
    original=server.game;
    if(!strcmp(argv[1],"no-server"))exists=0;
    if(!strcmp(argv[1],"inactive"))loaded=0;
    if(!strcmp(argv[1],"xbox"))strcpy(server.game.map.name,"levels\\a10\\a10");
    if(!strcmp(argv[1],"ce"))strcpy(server.game.map.name,"custom_maps\\a10");
    if(!strcmp(argv[1],"pregame"))server.state=_network_game_server_state_pregame;
    if(!strcmp(argv[1],"leaver"))leaver=1;
    if(!strcmp(argv[1],"failure"))fail_reset=1;
    if(!strcmp(argv[1],"dispose"))fail_reset=1;
    if(!strcmp(argv[1],"splitscreen"))splitscreen=1;
    untouched=server;
    CHECK(!mcc_ui_network_restart_settings(NULL),1);
    CHECK(!mcc_ui_network_restart_settings(&server),2);
    result=mcc_ui_restart_network_game();
    if(!exists || !loaded || !mcc_level_name(untouched.game.map.name) || untouched.state==_network_game_server_state_pregame) {
      CHECK(!result && !switched && !reset && !countdowns && !opened,3);
      CHECK(!memcmp(&server,&untouched,sizeof(server)),4);
      CHECK(!mcc_ui_network_restart_pregame(),15);
      network_game_reset_to_pregame_ui();
      CHECK(closed_ui==1&&loaded_ui==1&&!countdowns,16);
      if(exists)CHECK(server.countdown_state.paused&&strstr(screen_name,"map_select_postgame"),17);
      else CHECK(strstr(screen_name,"connected_pregame_screen"),18);
      return 0;
    }
    if(fail_reset) {
      CHECK(!result && !countdowns && !opened,5);
      CHECK(server.state==_network_game_server_state_postgame,6);
      if(!strcmp(argv[1],"dispose")) {
        mcc_ui_network_server_dispose(&server);
        CHECK(!mcc_ui_network_restart_settings(&server),14);
        server=untouched;
      }
      fail_reset=0;result=mcc_ui_restart_network_game();
    }
    CHECK(result && !countdowns && opened==1,7);
    CHECK(server.game.number_of_games_played==5 && !server.game.local_data,8);
    CHECK(!strcmp(map_name,original.map.name),9);
    CHECK(server.game.players[0].team_index==0 && server.game.players[1].team_index==1,10);
    if(leaver)CHECK(server.game.players[2].machine_index==-1,11);
    CHECK(!mcc_ui_network_restart_settings(&server),12);
    CHECK(!mcc_ui_restart_network_game(),13);
    /* Reliable messages are delivered later. The local client has unloaded
     * the MCC map before this actual native UI transition executes. */
    loaded=0;
    if(!strncmp(argv[1],"ui-",3)) {
      struct network_game_server pending=server;
      if(!strcmp(argv[1],"ui-dispose"))mcc_ui_network_server_dispose(&server);
      if(!strcmp(argv[1],"ui-map"))strcpy(server.game.map.name,"mcc_maps\\different");
      if(!strcmp(argv[1],"ui-version"))server.game.map.version++;
      if(!strcmp(argv[1],"ui-round"))server.game.number_of_games_played++;
      if(!strcmp(argv[1],"ui-state"))server.state=_network_game_server_state_ingame;
      if(!strcmp(argv[1],"ui-server"))selected_server=&other_server;
      if(!strcmp(argv[1],"ui-exit"))exists=0;
      CHECK(!mcc_ui_network_restart_pregame()&&!countdowns,19);
      selected_server=&server;exists=1;server=pending;
      CHECK(!mcc_ui_network_restart_pregame()&&!countdowns,20);
      network_game_reset_to_pregame_ui();
      CHECK(server.countdown_state.paused&&strstr(screen_name,"map_select_postgame"),21);
      return 0;
    }
    network_game_reset_to_pregame_ui();
    CHECK(closed_ui==1&&loaded_ui==1&&!errors&&countdowns==1,22);
    CHECK(server.countdown_state.active&&!server.countdown_state.paused&&server.countdown_state.timer==3000,23);
    CHECK(strstr(screen_name,splitscreen?"splitscreen_pregame_wrapper_normal":"connected_pregame_screen"),24);
    CHECK(!strstr(screen_name,"map_select_postgame"),25);
    CHECK(!mcc_ui_network_restart_pregame()&&countdowns==1,26);
    if(!strcmp(argv[1],"second-round")) {
      server.state=_network_game_server_state_ingame;loaded=1;original=server.game;
      CHECK(mcc_ui_restart_network_game()&&countdowns==1&&server.game.number_of_games_played==6,27);
      loaded=0;network_game_reset_to_pregame_ui();
      CHECK(countdowns==2&&closed_ui==2&&loaded_ui==2&&!server.countdown_state.paused,28);
    } else if(!splitscreen) {
      /* A subsequent ordinary End Game must return to its original map
       * picker and pause the timer, not reuse an old restart token. */
      network_game_reset_to_pregame_ui();
      CHECK(server.countdown_state.paused&&!server.countdown_state.active&&countdowns==1,29);
      CHECK(strstr(screen_name,"map_select_postgame"),30);
    }
    return 0;
}
'''
    path = directory / "restart.c"
    path.write_text(source)
    executable = directory / ("restart.exe" if os.name == "nt" else "restart")
    command = [compiler, "-std=c99", "-I", str(directory), str(path), "-o", str(executable)]
    if os.name == "nt":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return executable


@pytest.mark.parametrize("case", ["1", "2", "leaver", "failure", "dispose", "no-server", "inactive", "xbox", "ce", "pregame",
                                 "splitscreen", "second-round", "ui-dispose", "ui-map", "ui-version", "ui-round",
                                 "ui-state", "ui-server", "ui-exit"])
def test_restart_transaction(restart_tool, case):
    subprocess.run([str(restart_tool), case], check=True, capture_output=True)
