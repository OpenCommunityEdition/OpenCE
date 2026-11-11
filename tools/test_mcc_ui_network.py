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
    (directory / "mcc_share.h").write_text("")
    (directory / "mcc_validation.h").write_text((ROOT / "port/linux/game/mcc_validation.h").read_text())
    (directory / "mcc_precache.h").write_text((ROOT / "port/linux/game/mcc_precache.h").read_text())
    (directory / "mcc_identity.h").write_text((ROOT / "port/linux/game/mcc_identity.h").read_text())
    implementation = (ROOT / "port/linux/game/mcc_ui_network.inl").read_text()
    server_source = (ROOT / "source/networking/network_server_manager.c").read_text()
    widget_source = (ROOT / "source/interface/ui_widget.c").read_text()
    pregame = function(server_source, "network_game_server_idle_pregame_tasks")
    # Keep the real countdown/start/heartbeat else-if chain: a mocked timer
    # alone cannot expose a deferred start suppressing every keepalive.
    heartbeat = pregame[pregame.index("\t\tif (server->countdown_state.active == TRUE)"):
                        pregame.index("\n\t}\n\telse if (server->time_of_first_client_loading_completion)")]
    source = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define _error_silent 0
#define NETWORK_SERVER_MANAGER_FILE "network_server_manager.c"
#define match_assert(file,line,condition) assert(condition)
#define MAXIMUM_NETWORK_PLAYER_COUNT 3
#define MAXIMUM_NETWORK_MACHINE_COUNT 4
#define TEST_FLAG(flags,bit) (((flags) & (1u << (bit))) != 0)
#define SET_FLAG(flags,bit,value) ((flags) = (value) ? ((flags) | (1u << (bit))) : ((flags) & ~(1u << (bit))))
#define _network_client_machine_precached_bit 0
#define csstrcmp strcmp
#define csmemcpy memcpy
#define csmemset memset
#define MILLISECONDS_PER_SECOND 1000
enum { _network_game_server_state_pregame, _network_game_server_state_ingame, _network_game_server_state_postgame };
struct network_player { int machine_index, controller_index, player_list_index, team_index; };
struct network_game {
    struct { int version; char name[128]; } map;
    int variant, variant_options, cooperative_flags, minimum_players, maximum_players, maximum_teams, difficulty;
    char name[32];
    struct network_player players[3];
    int number_of_games_played, local_data, machine_count;
};
struct network_game_server_client_machine {int machine_index,flags,joined;};
struct network_game_server { int state,sent_start_game_message; struct network_game game;
    struct network_game_server_client_machine client_machines[4];
    struct {int timer;boolean active,paused,adjusted_time_this_tick;long last_countdown_message_time;} countdown_state;
    long time_of_last_keep_alive;
} server,other_server;
static int exists=1,loaded=1,fail_reset,countdowns,opened,switched,reset,leaver;
static int splitscreen,quickstart,closed_ui,loaded_ui,errors;
static int client_exists=1,local_machine=NONE,map_changes;
static int validation=1,updates,cancels;
static unsigned long heartbeat_clock,last_heartbeat;
static int heartbeat_messages,start_attempts;
struct network_message {int unused;};
struct message_server_pregame_countdown {short seconds_to_start;};
struct message_server_pregame_keep_alive {long unused;};
enum {_message_server_pregame_countdown,_message_server_pregame_keep_alive};
static unsigned long system_milliseconds(void) {return heartbeat_clock;}
static boolean server_ok_to_countdown(struct network_game_server *s) {(void)s;return TRUE;}
static struct network_message *create_network_game_message(int kind,void *packet,unsigned long size) {
    static struct network_message message;(void)kind;(void)packet;(void)size;return &message;
}
static boolean network_game_server_send_message_to_all_machines(struct network_game_server *s,
    struct network_message *message) {
    (void)s;(void)message;heartbeat_messages++;last_heartbeat=heartbeat_clock;return TRUE;
}
static void mcc_load_network_start(struct network_game const *game) {(void)game;}
static int download_active[4],download_needs[4];
static int mcc_share_game_machine_active(long i) {return download_active[i];}
static int map_share_machine_needs_map(long i) {return download_needs[i];}
static int network_game_server_client_machine_is_joined_to_game(struct network_game_server *s,
    struct network_game_server_client_machine *m) {(void)s;return m->joined;}
static int network_game_server_machine_has_players(struct network_game_server *s,long machine) {
    int i;for(i=0;i<3;i++)if(s->game.players[i].machine_index==machine)return 1;return 0;
}
static int network_game_server_client_machine_is_local(struct network_game_server *s,
    struct network_game_server_client_machine *m) {(void)s;return m->machine_index==0;}
static int halo_server_mode(void) {return 0;}
static int cache_files_precache_map_loaded(char const *name) {(void)name;return 1;}
int mcc_validation_request(char const *name,uint32_t *sum,unsigned char digest[32]) {
    (void)name;(void)digest;if(sum)*sum=123;return validation;
}
int mcc_validation_lookup(char const *name,uint32_t *sum,unsigned char digest[32]) {
    return mcc_validation_request(name,sum,digest)==1;
}
void mcc_validation_update(void) {updates++;}
void mcc_validation_cancel(void) {cancels++;}
char const *mcc_validation_error(void) {return "bad package";}
int mcc_identity_prepare_host(char const *name) {(void)name;return validation==1;}
void display_error_text_when_main_menu_loaded(wchar_t const *text) {(void)text;errors++;}
static struct network_game_server *selected_server=&server;
static struct network_game original;
static char map_name[128],screen_name[256];
static struct network_game_server *global_network_game_server_get(void) {return exists ? selected_server : NULL;}
static void *global_network_game_client_get(void) {return client_exists ? &client_exists : NULL;}
static short network_game_client_get_local_machine_index(void) {return (short)local_machine;}
static void network_game_server_change_map_name(struct network_game_server *s,char const *name) {
    strcpy(s->game.map.name,name);s->game.map.version=123;map_changes++;
}
static int mcc_cache_tags_loaded(void) {return loaded;}
static int mcc_level_name(char const *name) {return !strncmp(name,"mcc_maps\\",9);}
static int network_player_is_valid(struct network_player const *p) {return p->machine_index>=0;}
static void main_set_multiplayer_map_name(char const *name) {strcpy(map_name,name);}
static char const *main_get_multiplayer_map_name(void) {return server.game.map.name;}
static void network_game_server_open_game(struct network_game_server *s) {(void)s;opened++;}
static void third_person_session_lock(void) {}
static void network_game_server_switch_to_postgame(struct network_game_server *s) {s->state=_network_game_server_state_postgame;switched++;}
static void network_game_server_port_cooperative_won(char const *next) {
    switched++;reset++;server.state=_network_game_server_state_postgame;
    if(fail_reset)return;
    server.game.number_of_games_played++;
    if(next)strcpy(server.game.map.name,next);
    server.state=_network_game_server_state_pregame;
}
static void network_event(char const *s) {(void)s;}
static void countdown_timer_set_time_remaining(int *timer,long ms) {*timer=(int)ms;}
static long countdown_timer_get_time_remaining(int *timer) {return *timer;}
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
        "network_game_server_start_now", "network_game_server_pause_countdown", "network_game_server_begin_game_start_countdown",
        "network_game_server_client_machine_is_precached", "network_game_server_have_all_machines_have_precached"
    ]) + '\n' + implementation + '\n' + function(widget_source, "network_game_reset_to_pregame_ui") + '\n' + (
        # Exercise the production early-return boundary without starting a world.
        function(server_source, "network_game_server_start_network_game").split(
            "if (server->sent_start_game_message")[0] + "start_attempts++;return FALSE;\n}\n"
    ) + "static boolean heartbeat_tick(struct network_game_server *server) {\n" + (
        "long now=(long)heartbeat_clock;boolean success=TRUE;\n" + heartbeat + "\nreturn success;\n}\n"
    ) + r'''
#define CHECK(c,n) do {if(!(c))return n;} while(0)
int main(int argc,char **argv) {
    int i,result;struct network_game_server untouched;
    if(argc!=2)return 99;
    if(!strncmp(argv[1],"heartbeat-",10)) {
      char const *mode=argv[1]+10;
      server.state=_network_game_server_state_pregame;local_machine=0;validation=0;
      strcpy(server.game.map.name,"levels\\test\\carousel\\carousel");server.game.map.version=123;
      server.game.players[0].machine_index=0;
      server.game.players[1].machine_index=server.game.players[2].machine_index=NONE;
      for(i=0;i<4;i++){server.client_machines[i].machine_index=i;server.client_machines[i].flags=1;}
      CHECK(mcc_ui_network_defer_map(&server,"mcc_maps\\Package\\mission"),110);
      network_game_server_begin_game_start_countdown(&server,1000);
      if(!strcmp(mode,"long")) {
        server.countdown_state.timer=30000;mcc_ui_network_update_map(&server);
        CHECK(server.countdown_state.timer==30000,111);return 0;
      }
      if(!strcmp(mode,"ce")||!strcmp(mode,"xbox")||!strcmp(mode,"cancel")||
         !strcmp(mode,"other-server")||!strcmp(mode,"disconnected")||
         !strcmp(mode,"started")||!strcmp(mode,"ingame")) {
        if(!strcmp(mode,"ce"))CHECK(!mcc_ui_network_defer_map(&server,"custom_maps\\arena"),112);
        if(!strcmp(mode,"xbox"))CHECK(!mcc_ui_network_defer_map(&server,"levels\\a10\\a10"),113);
        if(!strcmp(mode,"cancel"))mcc_ui_network_server_dispose(&server);
        if(!strcmp(mode,"other-server"))selected_server=&other_server;
        if(!strcmp(mode,"disconnected"))client_exists=0;
        if(!strcmp(mode,"started"))server.sent_start_game_message=1;
        if(!strcmp(mode,"ingame"))server.state=_network_game_server_state_ingame;
        mcc_ui_network_update_map(&server);
        CHECK(server.countdown_state.timer==1000&&!mcc_ui_network_map_pending(&server),114);return 0;
      }
      if(!strcmp(mode,"failed")||!strcmp(mode,"failed-cancel"))validation=-1;
      if(!strcmp(mode,"paused"))network_game_server_pause_countdown(&server,TRUE);
      if(!strcmp(mode,"stopped"))server.countdown_state.active=FALSE;
      CHECK(network_game_server_have_all_machines_have_precached(&server),115);
      for(i=1;i<=40;i++) {
        heartbeat_clock=(unsigned long)i*1000;
        if(server.countdown_state.active && server.countdown_state.timer>0)
          server.countdown_state.timer-=1000;
        mcc_ui_network_update_map(&server);heartbeat_tick(&server);
        CHECK(heartbeat_clock-last_heartbeat<=15000,116);
        CHECK(!start_attempts&&!server.sent_start_game_message,117);
      }
      CHECK(heartbeat_messages>=6&&!map_changes,118);
      if(!strcmp(mode,"paused")||!strcmp(mode,"stopped")) {
        CHECK(!server.countdown_state.active,119);
        CHECK(server.countdown_state.paused==!strcmp(mode,"paused"),120);return 0;
      }
      CHECK(heartbeat_messages>=15&&server.countdown_state.timer==3000,121);
      if(validation<0) {
        CHECK(errors==1,122);
        if(!strcmp(mode,"failed-cancel")) {
          CHECK(!mcc_ui_network_defer_map(&server,"custom_maps\\arena"),123);
          server.countdown_state.timer=1000;mcc_ui_network_update_map(&server);
          CHECK(!mcc_ui_network_map_pending(&server)&&server.countdown_state.timer==1000,124);
        }
        return 0;
      }
      validation=1;mcc_ui_network_update_map(&server);
      CHECK(map_changes==1&&!mcc_ui_network_map_pending(&server),125);
      if(!strcmp(mode,"revision")) {
        /* A changed current package is unvalidated even without a deferred
         * selection token; its timer must resume the same heartbeat path. */
        validation=0;server.countdown_state.timer=0;mcc_ui_network_update_map(&server);
        CHECK(mcc_ui_network_map_pending(&server)&&server.countdown_state.timer==3000,126);
        validation=1;mcc_ui_network_update_map(&server);
        CHECK(!mcc_ui_network_map_pending(&server),127);
      }
      server.countdown_state.timer=0;heartbeat_tick(&server);
      CHECK(start_attempts==1,128);return 0;
    }
    if(!strncmp(argv[1],"validation-",11)) {
      char const *name="mcc_maps\\Package\\arena";
      server.state=_network_game_server_state_pregame;local_machine=0;validation=0;
      if(!strcmp(argv[1],"validation-dedicated"))client_exists=0;
      strcpy(server.game.map.name,"levels\\test\\bloodgulch\\bloodgulch");
      CHECK(mcc_ui_network_defer_map(&server,name)&&!updates,50);
      CHECK(mcc_ui_network_map_pending(&server),51);
      CHECK(network_game_server_start_network_game(&server)&&!server.sent_start_game_message,61);
      mcc_ui_network_update_map(&server);CHECK(updates==1&&!map_changes,52);
      if(!strcmp(argv[1],"validation-ready")||!strcmp(argv[1],"validation-dedicated")) {
        validation=1;mcc_ui_network_update_map(&server);
        CHECK(map_changes==1&&!mcc_ui_network_map_pending(&server),53);
        mcc_ui_network_update_map(&server);CHECK(map_changes==1&&updates==1,54);
      } else if(!strcmp(argv[1],"validation-failed")) {
        validation=-1;for(i=0;i<10;i++)mcc_ui_network_update_map(&server);
        CHECK(errors==1&&!map_changes&&updates==1,55);
        CHECK(mcc_ui_network_map_pending(&server),56);
      } else {
        if(!strcmp(argv[1],"validation-gameplay"))server.state=_network_game_server_state_ingame;
        else if(!strcmp(argv[1],"validation-started"))server.sent_start_game_message=1;
        else if(!strcmp(argv[1],"validation-back"))mcc_ui_network_server_dispose(&server);
        else if(!strcmp(argv[1],"validation-xbox"))CHECK(!mcc_ui_network_defer_map(&server,"levels\\a10\\a10"),57);
        else if(!strcmp(argv[1],"validation-ce"))CHECK(!mcc_ui_network_defer_map(&server,"custom_maps\\arena"),58);
        else return 59;
        for(i=0;i<10;i++)mcc_ui_network_update_map(&server);
        CHECK(updates==1&&!map_changes&&cancels>=2,60);
      }
      return 0;
    }
    if(!strncmp(argv[1],"map-",4)) {
      char const *name="mcc_maps\\Package\\arena";
      server.state=_network_game_server_state_pregame;
      strcpy(server.game.map.name,"levels\\test\\bloodgulch\\bloodgulch");
      untouched=server;
      CHECK(mcc_ui_network_defer_map(&server,name),31);
      CHECK(mcc_ui_network_map_pending(&server)&&!mcc_ui_network_map_pending(&other_server),32);
      for(i=0;i<100;i++)mcc_ui_network_update_map(&server);
      CHECK(!map_changes&&!memcmp(&server,&untouched,sizeof(server)),33);
      if(!strcmp(argv[1],"map-replace")) {
        name="mcc_maps\\Other\\arena";CHECK(mcc_ui_network_defer_map(&server,name),34);
      }
      if(!strcmp(argv[1],"map-xbox"))CHECK(!mcc_ui_network_defer_map(&server,"levels\\a10\\a10"),35);
      if(!strcmp(argv[1],"map-ce"))CHECK(!mcc_ui_network_defer_map(&server,"custom_maps\\a10"),36);
      if(!strcmp(argv[1],"map-dispose"))mcc_ui_network_server_dispose(&server);
      if(!strcmp(argv[1],"map-no-client"))client_exists=0;
      if(!strcmp(argv[1],"map-other-server"))selected_server=&other_server;
      if(!strcmp(argv[1],"map-started"))server.sent_start_game_message=1;
      if(!strcmp(argv[1],"map-ingame"))server.state=_network_game_server_state_ingame;
      local_machine=0;mcc_ui_network_update_map(&server);
      CHECK(!mcc_ui_network_map_pending(&server),37);
      if(!strcmp(argv[1],"map-join")||!strcmp(argv[1],"map-replace")) {
        CHECK(map_changes==1&&!strcmp(server.game.map.name,name),38);
        CHECK(!mcc_ui_network_defer_map(&server,name),39);
      } else CHECK(!map_changes,40);
      mcc_ui_network_update_map(&server);CHECK(map_changes<=1,41);
      client_exists=0;CHECK(!mcc_ui_network_defer_map(&server,name),42);
      return 0;
    }
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

    if(!strncmp(argv[1],"download-",9)) {
      int expected=1;
      char const *next="mcc_maps\\Package\\mission_two";
      local_machine=0;server.game.map.version=123;
      for(i=0;i<4;i++){server.client_machines[i].machine_index=i;server.client_machines[i].flags=1;
        server.client_machines[i].joined=1;}
      if(!strcmp(argv[1],"download-campaign")) {
        mcc_ui_network_campaign_won(next);loaded=0;network_game_reset_to_pregame_ui();
        CHECK(strstr(screen_name,"connected_pregame_screen")&&countdowns==1,90);
      } else {
        server.state=_network_game_server_state_pregame;
        network_game_server_begin_game_start_countdown(&server,3000);
      }
      server.countdown_state.timer=1000;
      download_active[1]=1;download_needs[1]=1;server.client_machines[1].flags=0;
      server.game.players[1].machine_index=NONE;server.game.players[2].machine_index=NONE;
      if(!strcmp(argv[1],"download-campaign"))server.game.players[1].machine_index=1;
      if(!strcmp(argv[1],"download-admission"))download_needs[1]=0;
      if(!strcmp(argv[1],"download-off")){download_active[1]=0;download_needs[1]=0;expected=0;}
      if(!strcmp(argv[1],"download-ce")){strcpy(server.game.map.name,"custom_maps\\arena");expected=0;}
      if(!strcmp(argv[1],"download-xbox")){strcpy(server.game.map.name,"levels\\test\\bloodgulch\\bloodgulch");expected=0;}
      if(!strcmp(argv[1],"download-unjoined")){server.client_machines[1].joined=0;expected=0;}
      if(!strcmp(argv[1],"download-ingame")){server.state=_network_game_server_state_ingame;expected=0;}
      if(!strcmp(argv[1],"download-started")){server.sent_start_game_message=1;expected=0;}
      if(!strcmp(argv[1],"download-other-server")){selected_server=&other_server;expected=0;}
      if(!strcmp(argv[1],"download-long"))server.countdown_state.timer=30000;
      if(!strcmp(argv[1],"download-paused"))network_game_server_pause_countdown(&server,TRUE);
      if(!strcmp(argv[1],"download-stopped"))server.countdown_state.active=FALSE;
      /* The real inherited predicate intentionally ignores a playerless
       * downloader. The MCC barrier must prevent that from starting a load. */
      CHECK(network_game_server_have_all_machines_have_precached(&server),91);
      mcc_ui_network_update_map(&server);
      CHECK(!!mcc_ui_network_map_pending(&server)==expected,92);
      if(!expected){CHECK(server.countdown_state.timer==1000&&countdowns==1,93);return 0;}
      CHECK(network_game_server_start_network_game(&server)&&!server.sent_start_game_message,94);
      if(!strcmp(argv[1],"download-long")){CHECK(server.countdown_state.timer==30000,95);return 0;}
      if(!strcmp(argv[1],"download-paused")||!strcmp(argv[1],"download-stopped")){
        CHECK(!server.countdown_state.active&&countdowns==1,96);
        CHECK(server.countdown_state.paused==!strcmp(argv[1],"download-paused"),104);return 0;
      }
      CHECK(server.countdown_state.active&&!server.countdown_state.paused&&
        server.countdown_state.timer==3000&&countdowns==1,97);
      for(i=0;i<8;i++){server.countdown_state.timer=2000;mcc_ui_network_update_map(&server);}
      CHECK(server.countdown_state.timer==3000&&countdowns==1,98);
      /* DONE permits AddPlayer but the short admission guard still holds
       * this pre-load barrier. A valid player then resumes ordinary precache. */
      download_needs[1]=0;server.countdown_state.timer=1000;mcc_ui_network_update_map(&server);
      CHECK(server.countdown_state.timer==3000&&mcc_ui_network_map_pending(&server),99);
      download_active[1]=0;server.game.players[1].machine_index=1;
      server.countdown_state.timer=2000;mcc_ui_network_update_map(&server);
      CHECK(server.countdown_state.timer==2000&&!mcc_ui_network_map_pending(&server),100);
      CHECK(!network_game_server_have_all_machines_have_precached(&server),101);
      network_game_server_client_machine_is_precached(&server,&server.client_machines[1],"mcc_maps\\wrong");
      CHECK(!network_game_server_have_all_machines_have_precached(&server),102);
      network_game_server_client_machine_is_precached(&server,&server.client_machines[1],server.game.map.name);
      CHECK(network_game_server_have_all_machines_have_precached(&server),103);
      return 0;
    }
    if(!strncmp(argv[1],"campaign-",9)) {
      char const *next="mcc_maps\\Package\\mission_two";
      int refuse=0,fallback=0;
      if(!strcmp(argv[1],"campaign-end")){next=NULL;fallback=1;}
      if(!strcmp(argv[1],"campaign-failure")){fail_reset=1;fallback=1;}
      if(!strcmp(argv[1],"campaign-xbox")){strcpy(server.game.map.name,"levels\\a10\\a10");refuse=1;}
      if(!strcmp(argv[1],"campaign-ce")){strcpy(server.game.map.name,"custom_maps\\a10");refuse=1;}
      if(!strcmp(argv[1],"campaign-target-xbox")){next="levels\\a30\\a30";refuse=1;}
      if(!strcmp(argv[1],"campaign-inactive")){loaded=0;refuse=1;}
      if(!strcmp(argv[1],"campaign-client")){exists=0;refuse=1;}
      if(!strcmp(argv[1],"campaign-not-ingame")){server.state=_network_game_server_state_pregame;refuse=1;}
      mcc_ui_network_campaign_won(next);
      if(refuse){CHECK(!reset&&!switched&&!mcc_ui_network_restart_pregame(),70);return 0;}
      CHECK(reset==1&&switched==1&&!countdowns,71);
      if(fallback){
        CHECK(!mcc_ui_network_restart_pregame()&&!countdowns,72);
        CHECK(!strcmp(server.game.map.name,original.map.name),73);return 0;
      }
      CHECK(!strcmp(server.game.map.name,next),74);
      loaded=0; /* The intended unload must not invalidate the UI token. */
      if(!strcmp(argv[1],"campaign-checksum")){
        server.game.map.version++;local_machine=0;
        CHECK(!mcc_ui_network_defer_map(&server,next),75);
      }
      if(!strcmp(argv[1],"campaign-dispose")){mcc_ui_network_server_dispose(&server);fallback=1;}
      if(!strcmp(argv[1],"campaign-round")){server.game.number_of_games_played++;fallback=1;}
      if(!strcmp(argv[1],"campaign-other-server")){selected_server=&other_server;fallback=1;}
      if(!strcmp(argv[1],"campaign-reselect")){
        mcc_ui_network_defer_map(&server,"custom_maps\\arena");
        mcc_ui_network_defer_map(&server,next);fallback=1;
      }
      if(!strcmp(argv[1],"campaign-map")){strcpy(server.game.map.name,"mcc_maps\\different");fallback=1;}
      if(!strcmp(argv[1],"campaign-state")){server.state=_network_game_server_state_ingame;fallback=1;}
      if(fallback){
        CHECK(!mcc_ui_network_restart_pregame()&&!countdowns,76);
        selected_server=&server;CHECK(!mcc_ui_network_restart_pregame(),77);return 0;
      }
      network_game_reset_to_pregame_ui();
      CHECK(closed_ui==1&&loaded_ui==1&&!errors&&countdowns==1,78);
      CHECK(!server.countdown_state.paused&&server.countdown_state.active,79);
      CHECK(strstr(screen_name,"connected_pregame_screen")&&!strstr(screen_name,"map_select"),80);
      CHECK(!strcmp(server.game.map.name,next)&&!mcc_ui_network_restart_pregame(),81);
      network_game_reset_to_pregame_ui();
      CHECK(server.countdown_state.paused&&strstr(screen_name,"map_select_postgame"),82);
      return 0;
    }
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
                                 "ui-state", "ui-server", "ui-exit", "map-join", "map-replace",
                                 "map-xbox", "map-ce", "map-dispose", "map-no-client",
                                 "map-other-server", "map-started", "map-ingame",
                                 "validation-ready", "validation-dedicated", "validation-failed", "validation-gameplay",
                                 "validation-started", "validation-back", "validation-xbox", "validation-ce"])
def test_restart_transaction(restart_tool, case):
    subprocess.run([str(restart_tool), case], check=True, capture_output=True)


@pytest.mark.parametrize("case", ["ready", "checksum", "end", "failure", "xbox", "ce", "target-xbox",
                                 "inactive", "client", "not-ingame", "dispose", "round", "other-server",
                                 "reselect", "map", "state"])
def test_campaign_follow_on_lobby(restart_tool, case):
    subprocess.run([str(restart_tool), "campaign-"+case], check=True, capture_output=True)


def test_prior_campaign_handoff_exposes_paused_map_picker(restart_tool, tmp_path):
    source = (restart_tool.parent / "restart.c").read_text()
    # Previously mcc_main called this shared transition directly. Keep the
    # real UI dispatcher and reproduce that path without the MCC UI token.
    original = function(source, "mcc_ui_network_campaign_won")
    source = source.replace(original, "void mcc_ui_network_campaign_won(char const *next) {"
                            "network_game_server_port_cooperative_won(next);}\n")
    path = tmp_path / "prior.c"
    path.write_text(source)
    executable = tmp_path / "prior.exe"
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    command = [compiler, "-std=c99", "-I", str(restart_tool.parent), str(path), "-o", str(executable)]
    if os.name == "nt":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    built = subprocess.run(command, capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    result = subprocess.run([str(executable), "campaign-ready"], capture_output=True, text=True)
    assert result.returncode == 78, "The prior handoff must fail the ready-lobby/countdown assertion"


@pytest.mark.parametrize("case", ["initial", "campaign", "admission", "long", "paused",
                                 "stopped", "off", "ce", "xbox", "unjoined", "ingame", "started", "other-server"])
def test_download_preload_barrier(restart_tool, case):
    subprocess.run([str(restart_tool), "download-"+case], check=True, capture_output=True)


@pytest.mark.parametrize("case", ["pending", "failed", "failed-cancel", "revision", "long", "paused", "stopped",
                                 "cancel", "ce", "xbox", "other-server", "disconnected", "started", "ingame"])
def test_pending_validation_preserves_heartbeat(restart_tool, case):
    subprocess.run([str(restart_tool), "heartbeat-"+case], check=True, capture_output=True)


def test_without_barrier_playerless_downloader_does_not_hold_start(restart_tool, tmp_path):
    source = (restart_tool.parent / "restart.c").read_text()
    source = source.replace(function(source, "mcc_ui_network_download_pending"),
                            "static boolean mcc_ui_network_download_pending(struct network_game_server *s) {return FALSE;}\n")
    path = tmp_path / "without_barrier.c"
    path.write_text(source)
    executable = tmp_path / "without_barrier.exe"
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    command = [compiler, "-std=c99", "-I", str(restart_tool.parent), str(path), "-o", str(executable)]
    if os.name == "nt":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    built = subprocess.run(command, capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    result = subprocess.run([str(executable), "download-initial"], capture_output=True, text=True)
    assert result.returncode == 92, "Without the barrier a playerless downloader is omitted from pre-load readiness"
