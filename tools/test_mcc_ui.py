"""Exercise MCC callbacks through the actual native event-handler dispatcher.

Mouse accept is queued and then delivered to the same A/confirmation records
as controller input. Host commands and tag storage are narrow fakes; routing,
navigation flags, failure handling and map-widget permission checks are real.
"""
import os
from pathlib import Path
import re
import shutil
import subprocess

import pytest
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="session")
def ui_tool(tmp_path_factory):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is needed for MCC widget tests")
    ui = (ROOT / "port/linux/game/mcc_ui.c").read_text()
    ui_header = (ROOT / "port/linux/game/mcc_ui.h").read_text()
    native = (ROOT / "source/interface/ui_widget.c").read_text()
    events = (ROOT / "source/interface/ui_widget_event_handler_functions.c").read_text()
    tags = (ROOT / "port/linux/game/mcc_tags.c").read_text()
    profiles = (ROOT / "source/interface/player_ui.c").read_text()
    actions = re.search(r"enum\s*\{.*?\};", ui_header, re.S).group()
    names = ["mcc_ui_settings_profile_released", "mcc_ui_settings_close", "mcc_ui_settings_begin",
             "mcc_ui_owns_widget", "mcc_ui_trusted_action", "mcc_ui_settings_needed", "mcc_ui_scenario_type", "mcc_ui_controller",
             "mcc_ui_failure", "mcc_ui_host_required", "mcc_ui_restart", "mcc_ui_end_round", "mcc_ui_new_game",
             "mcc_ui_request_team", "mcc_ui_choose_team", "mcc_ui_close_for_controller",
             "mcc_ui_event_function"]
    source = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#ifdef _WIN32
#define csstrcasecmp _stricmp
#else
#include <strings.h>
#define csstrcasecmp strcasecmp
#endif
typedef unsigned char boolean;
typedef unsigned short word;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#define csmemcpy memcpy
#define MAXIMUM_NUMBER_OF_LOCAL_PLAYERS 4
#define PC_MENU_FUNCTION_BASE 256
#define TEST_FLAG(value,bit) ((value)&(1u<<(bit)))
#define _error_silent 0
#define match_assert(file,line,condition) assert(condition)
#define match_vassert(file,line,condition,...) assert(condition)
enum {_game_connection_local,_game_connection_network_client,_game_connection_network_server};
enum {_event_handler_close_current_widget_bit,_event_handler_close_other_widget_bit,
_event_handler_close_all_widgets_bit,_event_handler_open_widget_bit,_event_handler_reload_self_bit,
_event_handler_reload_widget_bit,_event_handler_give_focus_to_widget_bit,_event_handler_run_function_bit,
_event_handler_replace_with_other_widget_bit,_event_handler_go_back_to_previous_widget_bit,
_event_handler_run_scenario_script_bit,_event_handler_look_for_conditional_widget_on_failure_bit};
enum {_conditional_widget_load_if_event_handler_function_fails_bit};
enum {_ui_audio_feedback_none,_ui_audio_feedback_cursor,_ui_audio_feedback_forward,_ui_audio_feedback_back};
struct widget_instance {long definition_tag_index;char const *name;short local_player_index;
short horizontal_offset,vertical_offset;struct widget_instance *parent,*next,*previous,*child,*focused_child;};
struct event_record {short type,controller_index;};
struct tag_reference {long index;};
struct tag_block {long count;void *address;};
struct ui_widget_definition {struct tag_block conditional_widgets;};
struct ui_widget_event_handler_reference {uint32_t flags;short event_type,function;
struct tag_reference widget_tag,sound_effect;char script[32];};
struct ui_widget_conditional_reference {struct tag_reference widget_tag;long flags;};
struct widget_stack_data {int unused;};
static struct {struct widget_instance *active_widgets[4];void *widget_stack[4];} widget_globals;
struct network_player {int valid;short machine_index,controller_index;char team_index;};
struct network_game {struct {struct {boolean teams;} universal_variant;} variant;struct network_player players[4];};
struct scenario {short type;};
static struct scenario scenario;
static struct scenario *global_scenario = &scenario;
static struct network_game game;
static struct network_player sent_player;
static int mcc=1,connection,coop,saved=1,profile=1,have_settings=1,profile_begin,settings_open;
static int campaign=1,pc_menus=1,pause_owned=1;
static int settings_load=1,settings_controller=-1,last_profile=1004,available_profile=1005,assigned_controller=-1;
static long settings_history_tag;
static short mcc_settings_controller=NONE;
struct player_profile {int value;};
struct game_variant {int value;};
static struct {
    long edit_profile_index;
    struct {long active_profile_index;} local_players[4];
    struct {union {struct player_profile player;struct game_variant variant;} original,current;} edit_profile;
} player_ui_globals;
static struct {int original,current;} player_ui_edit_options;
enum {_saved_game_file_type_player_profile=1,_saved_game_file_type_game_variant=2};
static struct widget_instance settings_widgets[4];
static int revert_count,reset_count,restart_count,save_count,persist_count,menu_count,quit_count,end_count,settings_collision;
static int posted_button=-1,posted_controller=-1,deleted_count,opened_count,legacy_count,port_count;
static int message_count,script_count,team_sent,denied_count,top_dispatch,quit_controller=-1,balance_allowed=1;
static int local_players=1,left_count,left_controller=-1,stack_disposed;
static boolean quit_closes_widgets;
static boolean free_widget_on_delete,round_active=TRUE;
static struct widget_instance *released_widget;
static long opened_tag;
static wchar_t const *message;
static char const *widget_tag_name="ui\\shell\\multiplayer_game\\pause_game\\blue_team_button";
static char owned[0x60],unowned[0x60];
static boolean mcc_cache_tags_loaded(void){return mcc;}
static boolean mcc_level_name(char const *name){return name&&!strncmp(name,"mcc_maps\\",9);}
static boolean mcc_pause_owns(long tag){return pause_owned&&tag==300;}
static void *tag_get(long group,long index){(void)group;return index==100?owned:unowned;}
static char *tag_get_name(long index){(void)index;return (char *)widget_tag_name;}
static boolean mcc_cache_contains(void const *p,long size){return p==owned && size<=sizeof(owned);}
static char const *config_string(char const *name){(void)name;return pc_menus==2?"PC":pc_menus?"pc":"xbox";}
static short game_connection(void){return connection;}
static void display_error_text_deferred(wchar_t const *text,short controller){assert(controller>=0&&controller<4);message_count++;message=text;}
static void *global_network_game_server_get(void){return connection==_game_connection_network_server?&game:NULL;}
static void *global_network_game_client_get(void){return connection?&game:NULL;}
static boolean mcc_ui_restart_network_game(void){restart_count++;return TRUE;}
static void main_reset_map(void){reset_count++;}
static void main_revert_map(void){revert_count++;}
static void main_goto_main_menu(void){menu_count++;}
static boolean game_engine_running(void){return TRUE;}
static boolean game_engine_can_score(void){return round_active;}
static void ui_widget_delete(struct widget_instance *widget);
static void ui_widgets_close_all(void);
static void game_engine_end_game(void){end_count++;ui_widgets_close_all();}
static boolean network_coop_active(void){return coop;}
static boolean mcc_maps_level_campaign(char const *name){(void)name;return campaign;}
static char *main_get_map_name(void){return "mcc_maps\\a10";}
static boolean game_state_port_saved_game_valid(void){return saved;}
static void game_state_save_to_persistent_storage(void){persist_count++;}
static void game_state_save(void){save_count++;saved=TRUE;}
static void ui_widgets_close_all_for_local_player(short controller);
static void network_game_client_local_player_quit(short controller){quit_count++;quit_controller=controller;if(quit_closes_widgets){ui_widgets_close_all_for_local_player(controller);local_players--;}}
static short local_player_count(void){return (short)local_players;}
static void player_ui_local_player_left_multiplayer_game(short controller){left_count++;left_controller=controller;}
static struct network_game *network_game_get_game(void){return connection?&game:NULL;}
static short network_game_client_get_local_machine_index(void){return 2;}
static boolean network_player_is_valid(struct network_player const *player){return player->valid;}
static boolean network_game_client_update_local_player_data(void *client,struct network_player *player){assert(client);sent_player=*player;team_sent++;return TRUE;}
static boolean mcc_ui_team_balance_allows(short machine,short controller,short team){(void)machine;(void)controller;(void)team;return balance_allowed;}
static void event_manager_post_button(short controller,short button){posted_button=button;posted_controller=controller;}
static long tag_loaded(long group,char const *name){(void)group;(void)name;return have_settings?(settings_collision?100:200):NONE;}
static int saved_game_file_get_type(long index){return index>=1000&&index<=1005?1:index==2000?2:0;}
static boolean player_profile_get(long index,struct player_profile *out){profile_begin++;if(!profile)return FALSE;out->value=(int)index;return TRUE;}
static boolean playlist_profile_get(long index,struct game_variant *out){out->value=(int)index;return TRUE;}
static void playlist_profile_get_options(long index,int *out){*out=(int)index;}
static long player_ui_get_player1_last_used_profile_index(void){return last_profile;}
static void player_profiles_enumerate_available_to_local_player_index(short controller,word *count,long *index,boolean defaults){assert(controller==0&&!defaults&&*count==1);*count=available_profile==NONE?0:1;*index=available_profile;}
static void player_ui_set_active_player_profile(short controller,long index,struct player_profile *p){assert(p->value==index);assigned_controller=controller;player_ui_globals.local_players[controller].active_profile_index=index;}
void mcc_ui_settings_profile_released(void);
void mcc_ui_settings_close(short controller);
static void player_ui_end_editing_profile(void);
static struct widget_instance *widget_instance_get_topmost_parent(struct widget_instance *widget){assert(widget!=released_widget);while(widget->parent)widget=widget->parent;return widget;}
static boolean ui_widget_port_dispatch_event(struct widget_instance *widget,short type,short controller,boolean *deleted){(void)widget;(void)deleted;assert(type==32&&controller==2);top_dispatch++;return TRUE;}
static void error(int priority,char const *text,...){(void)priority;(void)text;}
static void console_warning(char const *text,...){(void)text;}
static boolean pc_menu_tag(long index){return index==200;}
static boolean main_menu_is_active(void){return FALSE;}
static boolean pc_menu_event_function_invoke(struct widget_instance *widget,struct event_record *event,long function,boolean *deleted){(void)widget;(void)event;(void)function;(void)deleted;port_count++;return TRUE;}
static boolean legacy_function(struct widget_instance *widget,struct event_record *event,boolean *deleted){(void)widget;(void)event;(void)deleted;legacy_count++;return TRUE;}
static struct {boolean (*functions[102])(struct widget_instance *,struct event_record *,boolean *);char const *names[102];} event_handler_function_list;
static boolean hs_evaluate_by_name(char const *name){assert(!strcmp(name,"my_action"));script_count++;return TRUE;}
static void widget_instance_give_focus_by_tag(struct widget_instance *widget,long tag,short controller){(void)widget;(void)tag;(void)controller;}
static void widget_instance_reload_recursive(struct widget_instance *widget){(void)widget;}
static void ui_widget_reload_by_tag(long tag){(void)tag;}
static struct widget_instance *widget_instance_find_by_tag_index(long tag){(void)tag;return NULL;}
static void ui_widget_delete(struct widget_instance *widget){int i;assert(widget!=released_widget);deleted_count++;for(i=0;i<4;i++)if(widget_globals.active_widgets[i]==widget)widget_globals.active_widgets[i]=NULL;if(free_widget_on_delete){released_widget=widget;memset(widget,0xDD,sizeof(*widget));free(widget);}}
static boolean ui_widget_launch_widget(struct widget_instance *widget,long tag){assert(widget!=released_widget);opened_count++;opened_tag=tag;return TRUE;}
static struct widget_instance *ui_widget_load_by_name_or_tag(char const *name,long tag,struct widget_instance *widget,short controller,long a,long b,long c){
    (void)tag;(void)widget;(void)b;(void)c;
    if(!name||!strstr(name,"pc\\main_menu\\settings_select"))return NULL;
    assert(controller>=0&&controller<4);
    if(!settings_load)return NULL;
    if(widget_globals.active_widgets[controller])ui_widget_delete(widget_globals.active_widgets[controller]);
    settings_widgets[controller]=(struct widget_instance){200,"native settings",controller};
    widget_globals.active_widgets[controller]=&settings_widgets[controller];
    settings_controller=controller;settings_history_tag=a;settings_open++;return &settings_widgets[controller];
}
static void widget_instance_go_back_to_previous(struct widget_instance *widget){assert(widget!=released_widget);}
static void unspatialized_impulse_sound_new(long tag,float volume){(void)tag;(void)volume;}
static void pop_widget(void **stack,struct widget_stack_data *data){(void)data;*stack=NULL;}
static void dispose_widget_stack(void **stack){assert(*stack);stack_disposed++;*stack=NULL;}
static boolean virtual_keyboard_active(void){return FALSE;}
static void virtual_keyboard_close(void){}
static void ui_play_audio_feedback_sound(long sound){(void)sound;}
''' + actions + '\n' + '\n'.join(function(profiles, name) for name in [
        "player_ui_get_active_player_profile_index", "player_ui_get_edit_player_profile", "player_ui_get_edit_playlist_profile",
        "player_ui_begin_editing_profile", "clear_profile_edit_data", "player_ui_end_editing_profile"
    ]) + '\n' + function(native, "ui_widgets_close_all_for_local_player") + '\n' + \
        function(native, "ui_widgets_close_all") + '\n' + \
        structure(ui, "mcc_ui_widget_prefix") + '\n' + '\n'.join(function(ui, name) for name in names) + '\n' + \
        '\n'.join(function(events, name) for name in ["network_game_remove_local_player", "ui_widget_function_denied",
                                                      "ui_widget_event_handler_function_invoke"]) + '\n' + \
        function(native, "event_handler_dispatch") + r'''
struct mcc_runtime {unsigned char *tag_index;uint32_t used;struct {uint32_t tag_count;}report;};
static unsigned char metadata[0x400];
static void *mcc_runtime_pointer(struct mcc_runtime *r,uint32_t address,uint32_t bytes){(void)r;return address<=sizeof(metadata)&&bytes<=sizeof(metadata)-address?metadata+address:NULL;}
static int mcc_hud(struct mcc_runtime *r,uint32_t group,unsigned char *p){(void)r;(void)group;(void)p;return TRUE;}
''' + '\n'.join(function(tags, name) for name in ["mcc_word", "mcc_block", "mcc_tags_prepare"]) + r'''
#define CHECK(c,n) do{if(!(c)){fprintf(stderr,"check %d failed\n",n);return n;}}while(0)
static boolean fire(struct widget_instance *widget,short button,struct ui_widget_event_handler_reference *handler){
    boolean deleted=FALSE;struct event_record event={1,widget->local_player_index};struct ui_widget_definition definition={0};
    if(button==handler->event_type)event_handler_dispatch(widget,&definition,&event,handler,&deleted);
    return deleted;
}
int main(int argc,char **argv){
    struct widget_instance widget={100,"button",2};int i;
    struct ui_widget_event_handler_reference mouse={128,28,108,{NONE},{NONE},{0}};
    struct ui_widget_event_handler_reference open={8,0,0,{101},{NONE},{0}};
    struct ui_widget_event_handler_reference action={132,0,11,{NONE},{NONE},{0}};
    for(i=0;i<102;i++)event_handler_function_list.functions[i]=legacy_function;
    for(i=0;i<4;i++)player_ui_globals.local_players[i].active_profile_index=1000+i;
    player_ui_globals.edit_profile_index=NONE;
    event_handler_function_list.functions[72]=network_game_remove_local_player;
    widget_globals.active_widgets[2]=&widget;
    if(argc!=2)return 99;
    if(!strcmp(argv[1],"metadata")){
        unsigned char entries[32]={0},before[0x400];struct mcc_runtime r={entries,sizeof(metadata),{1}};
        uint32_t group='DeLa',address=0,count=2,events=0x100;
        memcpy(entries,&group,4);memcpy(entries+20,&address,4);
        memcpy(metadata+0x54,&count,4);memcpy(metadata+0x58,&events,4);
        memset(metadata+0x100,0xA5,0x90);memcpy(before,metadata,sizeof(before));
        CHECK(mcc_tags_prepare(&r),1);CHECK(!memcmp(before,metadata,sizeof(before)),2);
        events=0x3FF;memcpy(metadata+0x58,&events,4);CHECK(!mcc_tags_prepare(&r),3);return 0;
    }
    if(!strcmp(argv[1],"confirmation")){
        CHECK(!fire(&widget,28,&mouse)&&posted_button==0&&posted_controller==2,4);
        CHECK(!revert_count&&!opened_count&&!deleted_count,5);
        CHECK(fire(&widget,posted_button,&open)&&opened_count==1&&opened_tag==101,6);
        CHECK(!revert_count,7);
        CHECK(fire(&widget,0,&action)&&revert_count==1&&deleted_count==1,8);return 0;
    }
    if(!strcmp(argv[1],"client-denial")||!strcmp(argv[1],"no-checkpoint")){
        if(!strcmp(argv[1],"client-denial")){connection=1;coop=TRUE;}else saved=FALSE;
        CHECK(!fire(&widget,0,&action)&&message_count==1,9);
        CHECK(!revert_count&&!deleted_count&&!opened_count,10);
        action.function=12;connection=1;
        CHECK(!fire(&widget,0,&action)&&message_count==2&&!reset_count&&!restart_count,11);return 0;
    }
    if(!strcmp(argv[1],"restart")){
        action.function=12;CHECK(fire(&widget,0,&action)&&reset_count==1,12);
        connection=2;coop=TRUE;widget_globals.active_widgets[2]=&widget;
        CHECK(fire(&widget,0,&action)&&restart_count==1&&reset_count==1,13);
        action.function=163;coop=FALSE;widget_globals.active_widgets[2]=&widget;
        CHECK(fire(&widget,0,&action)&&restart_count==2,14);return 0;
    }
    if(!strcmp(argv[1],"quit")){
        action.function=13;action.flags=128;CHECK(!fire(&widget,0,&action)&&menu_count==1&&persist_count==1,15);
        coop=TRUE;connection=1;fire(&widget,0,&action);CHECK(quit_count==4&&persist_count==1,16);
        coop=FALSE;action.function=72;fire(&widget,0,&action);CHECK(quit_count==5&&quit_controller==2,17);return 0;
    }
    if(!strcmp(argv[1],"team")){
        connection=1;game.variant.universal_variant.teams=TRUE;game.players[0]=(struct network_player){1,1,2,0};
        game.players[1]=(struct network_player){1,2,2,1};action.function=157;
        CHECK(fire(&widget,0,&action)&&team_sent==1&&sent_player.team_index==1&&sent_player.machine_index==2,18);
        widget_tag_name="ui\\shell\\multiplayer_game\\pause_game\\red_team_button";
        fire(&widget,0,&action);CHECK(team_sent==2&&sent_player.team_index==0,19);
        game.variant.universal_variant.teams=FALSE;CHECK(!fire(&widget,0,&action)&&message_count==1,20);return 0;
    }
    if(!strcmp(argv[1],"settings")||!strcmp(argv[1],"settings-uppercase")){
        if(!strcmp(argv[1],"settings-uppercase"))pc_menus=2;
        campaign=FALSE;scenario.type=1;
        action.function=137;action.flags=136;action.widget_tag.index=300;
        CHECK(fire(&widget,0,&action)&&settings_open==1&&profile_begin==1,21);
        CHECK(!opened_count,22);have_settings=FALSE;
        CHECK(!fire(&widget,0,&action)&&message_count==1&&settings_open==1,23);
        have_settings=TRUE;settings_collision=TRUE;
        CHECK(!fire(&widget,0,&action)&&profile_begin==1&&settings_open==1&&message_count==2,36);
        CHECK(mcc_ui_settings_needed("mcc_maps\\dangercanyon"),24);mcc=FALSE;CHECK(!mcc_ui_settings_needed("mcc_maps\\dangercanyon"),25);return 0;
    }
    if(!strcmp(argv[1],"save")){
        action.function=179;fire(&widget,0,&action);CHECK(save_count==1&&persist_count==1,26);
        coop=TRUE;connection=2;fire(&widget,0,&action);CHECK(save_count==2&&persist_count==1,27);
        connection=1;CHECK(!fire(&widget,0,&action)&&save_count==2&&message_count==1,28);return 0;
    }
    if(!strcmp(argv[1],"team-balance")){
        connection=1;game.variant.universal_variant.teams=TRUE;balance_allowed=FALSE;action.function=157;
        CHECK(!fire(&widget,0,&action)&&message_count==1&&!team_sent&&!deleted_count,41);return 0;
    }
    if(!strcmp(argv[1],"new-game")){
        struct widget_instance *heap_widget=malloc(sizeof(*heap_widget));
        CHECK(heap_widget!=NULL,42);*heap_widget=widget;
        widget_tag_name="ui\\shell\\multiplayer_game\\pause_game\\new_game_button";
        fire(&widget,28,&mouse);CHECK(posted_button==0,37);
        /* The actual engine synchronously frees the whole tree. Pending
         * close/go-back flags must not touch it again after MCC handles it. */
        open.flags|=(1u<<_event_handler_close_current_widget_bit)|(1u<<_event_handler_go_back_to_previous_widget_bit);
        connection=2;widget_globals.active_widgets[2]=heap_widget;free_widget_on_delete=TRUE;
        CHECK(fire(heap_widget,posted_button,&open)&&end_count==1&&!opened_count&&deleted_count==1,38);
        CHECK(!widget_globals.active_widgets[2]&&released_widget==heap_widget,43);
        free_widget_on_delete=FALSE;widget_globals.active_widgets[2]=&widget;
        connection=1;CHECK(!fire(&widget,0,&open)&&end_count==1&&message_count==1&&!opened_count&&deleted_count==1,39);
        connection=2;round_active=FALSE;
        CHECK(!fire(&widget,0,&open)&&end_count==1&&message_count==2&&deleted_count==1,44);
        open.flags=8;
        mcc=FALSE;fire(&widget,0,&open);CHECK(opened_count==1&&end_count==1,40);return 0;
    }
    if(!strcmp(argv[1],"trusted-pause")){
        widget.definition_tag_index=300;
        CHECK(tag_get('DeLa',300)==unowned&&mcc_ui_trusted_action(&widget,MCC_PAUSE_ACTION_RESUME),45);
        action.function=MCC_PAUSE_ACTION_RESUME;
        CHECK(fire(&widget,0,&action)&&deleted_count==1&&!legacy_count,46);
        widget_globals.active_widgets[2]=&widget;action.function=MCC_PAUSE_ACTION_REVERT;
        CHECK(fire(&widget,0,&action)&&revert_count==1,47);
        widget_globals.active_widgets[2]=&widget;action.function=MCC_PAUSE_ACTION_RESTART;
        CHECK(fire(&widget,0,&action)&&reset_count==1,48);
        action.flags=128;action.function=MCC_PAUSE_ACTION_SAVE;
        CHECK(!fire(&widget,0,&action)&&save_count==1&&persist_count==1,49);
        action.function=MCC_PAUSE_ACTION_QUIT;
        CHECK(!fire(&widget,0,&action)&&menu_count==1&&persist_count==2,50);
        connection=1;coop=TRUE;
        CHECK(!fire(&widget,0,&action)&&quit_count==4&&persist_count==2,51);
        campaign=FALSE;scenario.type=1;coop=FALSE;
        CHECK(fire(&widget,0,&action)&&quit_count==5&&quit_controller==2,52);return 0;
    }
    if(!strcmp(argv[1],"trusted-teams")){
        widget.definition_tag_index=300;widget_tag_name="generated\\arbitrary_button_name";
        connection=1;campaign=FALSE;scenario.type=1;game.variant.universal_variant.teams=TRUE;
        game.players[1]=(struct network_player){1,2,2,1};
        action.function=MCC_PAUSE_ACTION_RED_TEAM;
        CHECK(fire(&widget,0,&action)&&team_sent==1&&sent_player.team_index==0,53);
        widget_globals.active_widgets[2]=&widget;action.function=MCC_PAUSE_ACTION_BLUE_TEAM;
        CHECK(fire(&widget,0,&action)&&team_sent==2&&sent_player.team_index==1,54);
        balance_allowed=FALSE;CHECK(!fire(&widget,0,&action)&&team_sent==2&&message_count==1,55);return 0;
    }
    if(!strcmp(argv[1],"split-resume")||!strcmp(argv[1],"split-team")||
       !strcmp(argv[1],"split-team-denied")||!strcmp(argv[1],"split-quit")||
       !strcmp(argv[1],"split-quit-sync")||!strcmp(argv[1],"local-quit")){
        struct widget_instance *heap_widget=malloc(sizeof(*heap_widget));
        struct widget_instance other={300,"other player's pause",1};
        struct widget_stack_data history={0},other_history={0};
        boolean team=!strncmp(argv[1],"split-team",10),resume=!strcmp(argv[1],"split-resume");
        boolean denied=!strcmp(argv[1],"split-team-denied"),local=!strcmp(argv[1],"local-quit");
        CHECK(heap_widget!=NULL,73);*heap_widget=widget;heap_widget->definition_tag_index=300;
        campaign=FALSE;scenario.type=1;connection=local?0:1;local_players=local?1:2;
        widget_globals.active_widgets[1]=&other;widget_globals.widget_stack[1]=&other_history;
        widget_globals.active_widgets[2]=heap_widget;widget_globals.widget_stack[2]=&history;
        free_widget_on_delete=TRUE;quit_closes_widgets=!strcmp(argv[1],"split-quit-sync");
        action.flags=128;action.function=resume?MCC_PAUSE_ACTION_RESUME:
            team?MCC_PAUSE_ACTION_RED_TEAM:MCC_PAUSE_ACTION_QUIT;
        if(team){game.variant.universal_variant.teams=TRUE;game.players[1]=(struct network_player){1,2,2,1};}
        if(denied){
            balance_allowed=FALSE;
            CHECK(!fire(heap_widget,0,&action)&&!deleted_count&&!stack_disposed&&!team_sent&&message_count==1,74);
            CHECK(widget_globals.active_widgets[2]==heap_widget&&widget_globals.widget_stack[2]==&history,75);
            free(heap_widget);
        }else{
            /* The real close-for-controller helper frees the caller and its
             * history. Native quit may do so first; dispatch must not reuse it. */
            CHECK(fire(heap_widget,0,&action)&&deleted_count==1&&stack_disposed==1,76);
            CHECK(released_widget==heap_widget&&!widget_globals.active_widgets[2]&&!widget_globals.widget_stack[2],77);
            CHECK(!opened_count&&!legacy_count&&!persist_count,78);
            if(team)CHECK(team_sent==1&&sent_player.team_index==0&&!quit_count,79);
            else if(resume)CHECK(!team_sent&&!quit_count&&!menu_count,80);
            else if(local)CHECK(menu_count==1&&!quit_count&&!left_count,81);
            else CHECK(quit_count==1&&quit_controller==2&&left_count==1&&left_controller==2&&!menu_count,82);
        }
        CHECK(widget_globals.active_widgets[1]==&other&&widget_globals.widget_stack[1]==&other_history,83);
        return 0;
    }
    if(!strcmp(argv[1],"trusted-end")){
        struct widget_instance *heap_widget=malloc(sizeof(*heap_widget));
        CHECK(heap_widget!=NULL,56);*heap_widget=widget;heap_widget->definition_tag_index=300;
        widget_globals.active_widgets[2]=heap_widget;free_widget_on_delete=TRUE;
        connection=2;campaign=FALSE;scenario.type=1;action.function=MCC_PAUSE_ACTION_END_GAME;
        /* Builder confirmations use run-function only for END_GAME. */
        action.flags=128;
        CHECK(fire(heap_widget,0,&action)&&end_count==1&&deleted_count==1&&!opened_count,57);
        CHECK(released_widget==heap_widget&&!widget_globals.active_widgets[2],58);
        widget.definition_tag_index=300;connection=1;free_widget_on_delete=FALSE;
        CHECK(!fire(&widget,0,&action)&&end_count==1&&message_count==1,59);return 0;
    }
    if(!strcmp(argv[1],"trusted-settings")){
        widget.definition_tag_index=300;action.function=MCC_PAUSE_ACTION_SETTINGS;action.flags=128;
        CHECK(!fire(&widget,0,&action)&&!profile_begin&&!settings_open&&message_count==1,60);
        campaign=FALSE;scenario.type=1;pc_menus=FALSE;
        CHECK(!fire(&widget,0,&action)&&!profile_begin&&!settings_open&&message_count==2,61);
        pc_menus=TRUE;
        CHECK(fire(&widget,0,&action)&&profile_begin==1&&settings_open==1,62);
        settings_collision=TRUE;
        CHECK(!fire(&widget,0,&action)&&profile_begin==1&&settings_open==1&&message_count==3,63);return 0;
    }
    if(!strcmp(argv[1],"settings-namespace")){
        CHECK(!mcc_ui_settings_needed("mcc_maps\\a10"),69);
        CHECK(!mcc_ui_settings_needed("mercury_falling")&&!mcc_ui_settings_needed("a10"),70);
        campaign=FALSE;CHECK(mcc_ui_settings_needed("mcc_maps\\dangercanyon"),71);
        CHECK(!mcc_ui_settings_needed("ui")&&!mcc_ui_settings_needed("custom_maps\\dangercanyon")&&
            !mcc_ui_settings_needed("dangercanyon")&&!mcc_ui_settings_needed(NULL),72);return 0;
    }
    if(!strcmp(argv[1],"network-stale-solo")){
        /* Network startup changes its own map, leaving main_get_map_name()
         * and the solo catalog classification at the previous campaign. */
        int role;widget.definition_tag_index=300;scenario.type=1;campaign=TRUE;action.flags=128;
        CHECK(!strcmp(main_get_map_name(),"mcc_maps\\a10"),84);
        for(role=1;role<=2;role++){
            connection=role;action.function=MCC_PAUSE_ACTION_SETTINGS;
            CHECK(fire(&widget,0,&action)&&settings_open==role&&profile_begin==role,85);
            widget_globals.active_widgets[2]=&widget;action.function=MCC_PAUSE_ACTION_QUIT;
            CHECK(fire(&widget,0,&action)&&quit_count==role&&quit_controller==2,86);
            CHECK(!persist_count&&!menu_count,87);
        }
        return 0;
    }
    if(!strcmp(argv[1],"campaign-stale-catalog")){
        /* Saving/reloading a live scenario must not depend on later catalog
         * changes or the last multiplayer selection. */
        widget.definition_tag_index=300;scenario.type=0;campaign=FALSE;action.flags=128;
        action.function=MCC_PAUSE_ACTION_SETTINGS;
        CHECK(!fire(&widget,0,&action)&&!profile_begin&&!settings_open,88);
        action.function=MCC_PAUSE_ACTION_SAVE;
        CHECK(!fire(&widget,0,&action)&&save_count==1&&persist_count==1,89);
        action.function=MCC_PAUSE_ACTION_QUIT;
        CHECK(!fire(&widget,0,&action)&&persist_count==2&&menu_count==1&&!quit_count,90);
        coop=TRUE;connection=1;
        CHECK(!fire(&widget,0,&action)&&persist_count==2&&quit_count==4,91);return 0;
    }
    if(!strcmp(argv[1],"settings-no-scenario")){
        widget.definition_tag_index=300;global_scenario=NULL;campaign=FALSE;action.flags=128;
        action.function=MCC_PAUSE_ACTION_SETTINGS;
        CHECK(!fire(&widget,0,&action)&&!profile_begin&&!settings_open&&message_count==1,92);
        global_scenario=&scenario;scenario.type=2;
        CHECK(!fire(&widget,0,&action)&&!profile_begin&&!settings_open&&message_count==2,93);return 0;
    }
    if(!strcmp(argv[1],"settings-controller")){
        struct widget_instance *heap_widget=malloc(sizeof(*heap_widget));
        struct widget_instance other={300,"other notification",0};
        CHECK(heap_widget!=NULL,94);*heap_widget=widget;heap_widget->definition_tag_index=300;
        scenario.type=1;action.flags=128;action.function=MCC_PAUSE_ACTION_SETTINGS;
        widget_globals.active_widgets[0]=&other;widget_globals.active_widgets[2]=heap_widget;free_widget_on_delete=TRUE;
        CHECK(fire(heap_widget,0,&action)&&released_widget==heap_widget&&settings_controller==2,95);
        CHECK(player_ui_globals.edit_profile_index==1002&&player_ui_get_edit_player_profile()->value==1002,96);
        CHECK(mcc_settings_controller==2&&settings_history_tag==300&&widget_globals.active_widgets[0]==&other,97);
        CHECK(player_ui_globals.local_players[0].active_profile_index==1000&&assigned_controller==NONE,98);
        free_widget_on_delete=FALSE;ui_widgets_close_all_for_local_player(2);
        CHECK(!player_ui_get_edit_player_profile()&&mcc_settings_controller==NONE,99);return 0;
    }
    if(!strcmp(argv[1],"settings-split-denied")){
        int count;widget.definition_tag_index=300;scenario.type=1;action.flags=128;
        for(count=2;count<=4;count++){
            local_players=count;action.function=MCC_PAUSE_ACTION_SETTINGS;
            CHECK(!fire(&widget,0,&action)&&!profile_begin&&!settings_open&&!deleted_count,100);
            widget.definition_tag_index=100;action.function=137;
            CHECK(!fire(&widget,0,&action)&&!profile_begin&&!settings_open&&!deleted_count,101);
            widget.definition_tag_index=300;
        }
        CHECK(message_count==6&&widget_globals.active_widgets[2]==&widget,102);return 0;
    }
    if(!strcmp(argv[1],"settings-busy")){
        widget.definition_tag_index=300;scenario.type=1;action.flags=128;action.function=MCC_PAUSE_ACTION_SETTINGS;
        player_ui_begin_editing_profile(1000);player_ui_get_edit_player_profile()->value=765;
        CHECK(!fire(&widget,0,&action)&&profile_begin==1&&!settings_open,103);
        CHECK(player_ui_globals.edit_profile_index==1000&&player_ui_get_edit_player_profile()->value==765,104);
        mcc_ui_settings_close(NONE);CHECK(player_ui_globals.edit_profile_index==1000,105);
        player_ui_end_editing_profile();player_ui_begin_editing_profile(2000);
        CHECK(!fire(&widget,0,&action)&&player_ui_get_edit_playlist_profile()!=NULL&&!settings_open,106);
        player_ui_end_editing_profile();CHECK(fire(&widget,0,&action)&&mcc_settings_controller==2,107);
        CHECK(!fire(&widget,0,&action)&&settings_open==1&&profile_begin==2,108);return 0;
    }
    if(!strcmp(argv[1],"settings-lifecycle")){
        struct widget_stack_data history={0};
        widget.definition_tag_index=300;scenario.type=1;action.flags=128;action.function=MCC_PAUSE_ACTION_SETTINGS;
        CHECK(fire(&widget,0,&action)&&mcc_settings_controller==2,109);
        /* Moving to a nested subpage deletes the root widget but must keep
         * its native edit buffer; only closing the whole owner clears it. */
        ui_widget_delete(widget_globals.active_widgets[2]);
        settings_widgets[2].definition_tag_index=201;widget_globals.active_widgets[2]=&settings_widgets[2];
        widget_globals.widget_stack[2]=&history;mcc_ui_settings_close(1);
        CHECK(player_ui_get_edit_player_profile()!=NULL&&mcc_settings_controller==2,110);
        ui_widgets_close_all_for_local_player(2);
        CHECK(!player_ui_get_edit_player_profile()&&mcc_settings_controller==NONE&&!widget_globals.widget_stack[2],111);
        widget.local_player_index=1;widget_globals.active_widgets[1]=&widget;
        CHECK(fire(&widget,0,&action)&&settings_controller==1&&player_ui_globals.edit_profile_index==1001,112);
        /* Cancel/save's native profile-clear releases ownership before any
         * later unrelated editor begins; closing old UI cannot cancel it. */
        player_ui_end_editing_profile();CHECK(mcc_settings_controller==NONE,113);
        player_ui_begin_editing_profile(1000);ui_widgets_close_all_for_local_player(1);
        CHECK(player_ui_globals.edit_profile_index==1000,114);
        player_ui_end_editing_profile();widget_globals.active_widgets[1]=&widget;
        CHECK(fire(&widget,0,&action)&&mcc_settings_controller==1,115);
        ui_widgets_close_all();CHECK(!player_ui_get_edit_player_profile()&&mcc_settings_controller==NONE,116);
        widget_globals.active_widgets[1]=&widget;CHECK(fire(&widget,0,&action),117);
        mcc_ui_settings_close(NONE);CHECK(!player_ui_get_edit_player_profile()&&mcc_settings_controller==NONE,118);return 0;
    }
    if(!strcmp(argv[1],"settings-open-failure")){
        widget.definition_tag_index=300;scenario.type=1;action.flags=128;action.function=MCC_PAUSE_ACTION_SETTINGS;
        settings_load=FALSE;
        CHECK(!fire(&widget,0,&action)&&!deleted_count&&!settings_open&&mcc_settings_controller==NONE,119);
        CHECK(!player_ui_get_edit_player_profile()&&widget_globals.active_widgets[2]==&widget,120);
        settings_load=TRUE;profile=FALSE;
        CHECK(!fire(&widget,0,&action)&&!settings_open&&mcc_settings_controller==NONE,121);
        profile=TRUE;player_ui_globals.local_players[2].active_profile_index=2000;
        CHECK(!fire(&widget,0,&action)&&!player_ui_get_edit_playlist_profile()&&mcc_settings_controller==NONE,127);
        player_ui_globals.local_players[2].active_profile_index=1002;
        profile=TRUE;CHECK(fire(&widget,0,&action)&&settings_open==1&&player_ui_globals.edit_profile_index==1002,122);return 0;
    }
    if(!strcmp(argv[1],"settings-replaced-editor")){
        widget.definition_tag_index=300;scenario.type=1;action.flags=128;action.function=MCC_PAUSE_ACTION_SETTINGS;
        CHECK(fire(&widget,0,&action)&&mcc_settings_controller==2,128);
        /* Any later native editor entry invalidates MCC's ownership even
         * when it does not call the normal end-edit helper first. */
        player_ui_begin_editing_profile(1000);
        CHECK(mcc_settings_controller==NONE&&player_ui_globals.edit_profile_index==1000,129);
        ui_widgets_close_all();CHECK(player_ui_globals.edit_profile_index==1000,130);return 0;
    }
    if(!strcmp(argv[1],"settings-profile-fallback")){
        widget.definition_tag_index=300;scenario.type=1;action.flags=128;action.function=MCC_PAUSE_ACTION_SETTINGS;
        player_ui_globals.local_players[2].active_profile_index=NONE;
        CHECK(!fire(&widget,0,&action)&&!profile_begin&&assigned_controller==NONE,123);
        widget.local_player_index=0;widget_globals.active_widgets[0]=&widget;
        player_ui_globals.local_players[0].active_profile_index=NONE;
        CHECK(fire(&widget,0,&action)&&player_ui_globals.edit_profile_index==1004&&assigned_controller==0,124);
        ui_widgets_close_all_for_local_player(0);player_ui_globals.local_players[0].active_profile_index=NONE;last_profile=NONE;
        widget_globals.active_widgets[0]=&widget;
        CHECK(fire(&widget,0,&action)&&player_ui_globals.edit_profile_index==1005&&assigned_controller==0,125);
        CHECK(player_ui_globals.local_players[2].active_profile_index==NONE,126);return 0;
    }
    if(!strcmp(argv[1],"trusted-isolation")){
        action.function=MCC_PAUSE_ACTION_RESUME;
        CHECK(!mcc_ui_trusted_action(&widget,action.function)&&!fire(&widget,0,&action)&&!deleted_count,64);
        widget.definition_tag_index=301;
        CHECK(!mcc_ui_trusted_action(&widget,action.function)&&!fire(&widget,0,&action)&&!deleted_count,65);
        widget.definition_tag_index=300;pause_owned=FALSE;
        CHECK(!fire(&widget,0,&action)&&!deleted_count,66);
        pause_owned=TRUE;mcc=FALSE;
        CHECK(!fire(&widget,0,&action)&&!deleted_count,67);
        mcc=TRUE;action.function=MCC_PAUSE_ACTION_SETTINGS+1;
        CHECK(!mcc_ui_trusted_action(&widget,action.function)&&!fire(&widget,0,&action)&&!deleted_count,68);return 0;
    }
    if(!strcmp(argv[1],"isolation")){
        mcc=FALSE;action.flags=128;fire(&widget,0,&action);CHECK(legacy_count==1&&!revert_count,29);
        mcc=TRUE;widget.definition_tag_index=101;fire(&widget,0,&action);CHECK(legacy_count==2&&!revert_count,30);
        widget.definition_tag_index=200;action.function=256;fire(&widget,0,&action);CHECK(port_count==1,31);
        widget.definition_tag_index=100;fire(&widget,0,&action);CHECK(port_count==1,32);
        action.function=67;fire(&widget,0,&action);CHECK(legacy_count==2,33);return 0;
    }
    if(!strcmp(argv[1],"unsupported")){
        action.function=190;action.flags=140;action.widget_tag.index=300;
        CHECK(!fire(&widget,0,&action)&&message_count==1&&!opened_count&&!deleted_count,34);return 0;
    }
    if(!strcmp(argv[1],"script")){
        action.function=0;action.flags=1024;strcpy(action.script,"my_action");fire(&widget,0,&action);
        CHECK(script_count==1&&!revert_count&&!message_count,35);return 0;
    }
    return 98;
}
'''
    work = tmp_path_factory.mktemp("mcc-ui")
    path = work / "ui.c"
    path.write_text(source)
    executable = work / ("ui.exe" if os.name == "nt" else "ui")
    command = [compiler, "-std=c99", str(path), "-o", str(executable)]
    if os.name == "nt":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return executable


@pytest.mark.parametrize("case", ["metadata", "confirmation", "client-denial", "no-checkpoint", "restart",
                                 "quit", "team", "team-balance", "settings", "settings-uppercase", "save", "new-game", "isolation", "unsupported", "script",
                                 "trusted-pause", "trusted-teams", "trusted-end", "trusted-settings", "trusted-isolation", "settings-namespace",
                                 "split-resume", "split-team", "split-team-denied", "split-quit", "split-quit-sync", "local-quit",
                                 "network-stale-solo", "campaign-stale-catalog", "settings-no-scenario",
                                 "settings-controller", "settings-split-denied", "settings-busy", "settings-lifecycle",
                                 "settings-open-failure", "settings-profile-fallback", "settings-replaced-editor"])
def test_mcc_widget_events(ui_tool, case):
    result = subprocess.run([str(ui_tool), case], capture_output=True, text=True)
    assert result.returncode == 0, (case, result.returncode, result.stdout, result.stderr)


def test_native_settings_admission_preserves_mcc_pause_ownership(tmp_path):
    """Run the real pre-allocation route with stock, CE and MCC tag layouts.

    Stop at the allocation boundary: this checks which maps reach the native
    builder and whether its campaign patch is enabled, without loading assets.
    """
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is needed for native menu admission tests")
    menu = (ROOT / "port/linux/game/menu_tags.c").read_text()
    admission = function(menu, "menu_tags_loaded").split("menus = halo_menus_load();", 1)[0]
    source = r'''
#include <assert.h>
#include <string.h>
typedef int boolean;
#define FALSE 0
#define NONE (-1)
#define MULTIPLAYER_COLLECTION "collection"
static int mcc, wants_settings, pc=1, campaign_tag, multiplayer_tag;
static int built, built_campaign;
static int mcc_cache_tags_loaded(void){return mcc;}
static int mcc_ui_settings_needed(char const *name){(void)name;return wants_settings;}
static int single_player_campaign_map(void){return campaign_tag;}
static long tag_loaded(long group,char const *name){(void)group;(void)name;return multiplayer_tag?1:NONE;}
static int menus_pc_chosen(void){return pc;}
''' + admission + r'''
    built++;built_campaign=campaign;
}
static void check(char const *name,int expected,int expected_campaign)
{
    built=0;built_campaign=0;menu_tags_loaded(name);
    assert(built==expected);assert(built_campaign==expected_campaign);
}
int main(void)
{
    /* Upstream's stock/CE campaign Settings and normal menu route remain. */
    check("ui",1,0);
    campaign_tag=1;check("a10",1,1);check("custom_maps\\a10",1,1);
    campaign_tag=0;multiplayer_tag=1;check("bloodgulch",1,0);
    pc=0;check("bloodgulch",0,0);check("ui",0,0);pc=1;
    /* Even an MCC map carrying the native campaign widget must not get
       that widget patched, nor get an extra set of campaign Settings tags. */
    mcc=1;campaign_tag=1;multiplayer_tag=0;
    check("mcc_maps\\a10",0,0);
    /* MCC multiplayer retains native Settings without the campaign patch. */
    wants_settings=1;check("mcc_maps\\dangercanyon",1,0);
    wants_settings=0;pc=0;check("mcc_maps\\dangercanyon",0,0);
    return 0;
}
'''
    path = tmp_path / "admission.c"
    path.write_text(source)
    exe = tmp_path / ("admission.exe" if os.name == "nt" else "admission")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else []
    result = subprocess.run([compiler, *flags, str(path), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
