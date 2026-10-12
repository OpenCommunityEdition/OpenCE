#include "harness.h"
#define MAXIMUM_LOCAL_PLAYERS 4
#define UI_WIDGET_DEFINITION_TAG 1
#define CAMPAIGN_PAUSE_SCREEN "campaign pause"
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
enum { _network_game_client_state_pregame, _network_game_client_state_ingame, _error_silent };
struct network_game { int unused; };
struct network_game_client {
 short state; struct network_game game; void *connection; long next_update_number;
 boolean join_in_progress,connection_silent;
};
static struct { boolean loaded; } menu_tags;
static boolean coop=TRUE,host=TRUE,deleted,tag_present=TRUE,pc_preferred=TRUE,splitscreen,quickstart;
static int transitions,quits,closed,resets,keepalives,paused;
static long network_game_client_late_join_time=123;
static boolean network_game_client_late_join_clock_pending=TRUE;
static char const *opened;
static boolean network_coop_active(void){return coop;}
static void *global_network_game_server_get(void){return host?(void*)1:NULL;}
static boolean campaign_fail(void){return FALSE;}
static void network_game_server_port_cooperative_won(char const *map){CHECK(!map,"must retain current map");transitions++;}
static void network_game_client_local_player_quit(short controller){CHECK(controller==quits,"quit local players in order");quits++;}
static void ui_widgets_close_all(void){closed++;}
static long tag_loaded(long kind,char const *name){(void)kind;(void)name;return tag_present?1:NONE;}
static boolean network_game_is_splitscreen_local(void){return splitscreen;}
static boolean network_game_is_quickstart_local(void){return quickstart;}
static void network_game_server_pause_countdown(void *server,boolean value){CHECK(server&&value,"host countdown paused");paused++;}
static void *ui_widget_load_by_name_or_tag(char const *name,long tag,void *parent,short player,long invoke,long focus,short index)
{(void)tag;(void)parent;(void)player;(void)invoke;(void)focus;(void)index;opened=name;return (void*)1;}
static void error(int level,char const *message){(void)level;CHECK(FALSE,"%s",message);}
static void network_game_reset_for_next_round(struct network_game *game,boolean unload)
{CHECK(game&&unload,"unload the played map");resets++;coop=FALSE;menu_tags.loaded=pc_preferred;}
static void network_connection_keep_alive(void *connection){CHECK(connection,"connection retained");keepalives++;}
static void network_event(char const *message){(void)message;}
#include "under_test.inc"

static char const *const xbox_postgame="ui\\shell\\main_menu\\multiplayer_type_select\\connected\\connected_map_select_postgame_wrapper";
static char const *const pc_postgame="pc\\main_menu\\multiplayer_type_select\\connected\\connected_map_select_wrapper";
static char const *const xbox_pregame="ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen";
static char const *const pc_lobby="pc\\main_menu\\multiplayer_type_select\\lobby\\lobby_screen";

static void check_transition(boolean cooperative,boolean pc,boolean server)
{
 struct network_game_client client={0};
 client.state=_network_game_client_state_ingame;client.connection=(void*)1;
 client.next_update_number=234;client.connection_silent=TRUE;
 coop=cooperative;pc_preferred=pc;host=server;
 CHECK(network_game_client_switch_to_pregame(&client),"switch succeeds");
 CHECK(resets==1&&keepalives==2&&closed==1,"reset lifecycle changed");
 CHECK(client.state==_network_game_client_state_pregame&&!client.next_update_number&&
  client.join_in_progress&&!client.connection_silent,"client state not reset");
 CHECK(!network_game_client_late_join_time&&!network_game_client_late_join_clock_pending,"late join state retained");
 CHECK(paused==!!server,"only host pauses countdown");
 CHECK(opened&&!strcmp(opened,server?(pc?(cooperative?pc_lobby:pc_postgame):xbox_postgame):(pc?pc_lobby:xbox_pregame)),
  "wrong postgame screen: %s",opened?opened:"none");
 CHECK(!coop,"played scenario must be gone before opening the UI");
}

int main(int argc,char **argv)
{
 char const *case_name=argc==2?argv[1]:"";
 CASE("host") {CHECK(coop_leave_game(&deleted),"leave succeeds");CHECK(transitions==1&&!quits&&!closed&&deleted,"host uses round transition only");}
 else CASE("client") {host=FALSE;CHECK(coop_leave_game(&deleted),"leave succeeds");CHECK(!transitions&&quits==4&&closed==1&&deleted,"client only leaves local machine");}
 else CASE("local") {coop=FALSE;CHECK(!coop_leave_game(&deleted),"not a coop action");CHECK(!transitions&&!quits&&!closed&&!deleted,"other modes untouched");}
 else CASE("pause-pc") {
  int n;menu_tags.loaded=TRUE;
  for(n=1;n<=4;n*=2){char name[128],expected[80];
   snprintf(name,sizeof(name),"ui\\shell\\multiplayer_game\\pause_game\\%dp_pause_game",n);
   snprintf(expected,sizeof(expected),"pc\\pause\\coop\\%dp_pause_game",n);
   CHECK(!strcmp(pc_menus_coop_pause_screen(name),expected),"coop viewport route");
   tag_present=FALSE;CHECK(!strcmp(pc_menus_coop_pause_screen(name),CAMPAIGN_PAUSE_SCREEN),"missing assets fallback");tag_present=TRUE;
  }
 }
 else CASE("pause-xbox") {menu_tags.loaded=FALSE;CHECK(!strcmp(pc_menus_coop_pause_screen("ui\\shell\\multiplayer_game\\pause_game\\1p_pause_game"),CAMPAIGN_PAUSE_SCREEN),"Xbox campaign fallback");}
 else CASE("generic-routes") {
  char const *other="unrelated screen";int loaded;
  for(loaded=0;loaded<=1;loaded++){
   menu_tags.loaded=loaded;
   CHECK(pc_menus_screen(other)==other,"unrelated route changed");
   CHECK(!strcmp(pc_menus_screen(xbox_postgame),loaded?pc_postgame:xbox_postgame),"generic postgame changed");
   CHECK(!strcmp(pc_menus_screen(xbox_pregame),loaded?pc_lobby:xbox_pregame),"generic pregame changed");
  }
 }
 else CASE("coop-pc-host") check_transition(TRUE,TRUE,TRUE);
 else CASE("coop-pc-client") check_transition(TRUE,TRUE,FALSE);
 else CASE("coop-xbox-host") check_transition(TRUE,FALSE,TRUE);
 else CASE("coop-xbox-client") check_transition(TRUE,FALSE,FALSE);
 else CASE("mp-pc-host") check_transition(FALSE,TRUE,TRUE);
 else CASE("mp-pc-client") check_transition(FALSE,TRUE,FALSE);
 else CASE("mp-xbox-host") check_transition(FALSE,FALSE,TRUE);
 else CASE("mp-xbox-client") check_transition(FALSE,FALSE,FALSE);
 else CASE("already-pregame") {
  struct network_game_client client={0};
  CHECK(network_game_client_switch_to_pregame(&client),"idempotent transition");
  CHECK(!resets&&!keepalives&&!closed&&!opened,"already-pregame path changed");
 }
 else CASE("splitscreen") {
  int quick;splitscreen=TRUE;
  for(quick=0;quick<=1;quick++){
   quickstart=quick;network_game_reset_to_pregame_ui(TRUE);
   CHECK(opened&&!strcmp(opened,quick?"ui\\shell\\main_menu\\multiplayer_type_select\\split_screen\\pregame\\splitscreen_pregame_wrapper_normal":
    "ui\\shell\\main_menu\\multiplayer_type_select\\split_screen\\splitscreen_map_select_postgame_wrapper"),"local split-screen path changed");
  }
  CHECK(!paused,"local split-screen host countdown changed");
 }
 else return 2;
 return 0;
}
