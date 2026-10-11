#include "harness.h"
#define MAXIMUM_LOCAL_PLAYERS 4
#define UI_WIDGET_DEFINITION_TAG 1
#define CAMPAIGN_PAUSE_SCREEN "campaign pause"
static boolean session_coop=TRUE;
struct network_game {struct {long game_engine_index;} variant;short difficulty;};
static struct network_game session;
static struct network_game *network_game_get_game(void){session.variant.game_engine_index=session_coop?0:1;session.difficulty=1;return &session;}
static boolean coop=TRUE,host=TRUE,deleted=FALSE,menu_loaded=TRUE,tag_present=TRUE;
static int transitions,quits,closed;
static struct {boolean loaded;} menu_tags;
static boolean network_coop_active(void){return coop;}
static void *global_network_game_server_get(void){return host?(void*)1:NULL;}
static boolean campaign_fail(void){return FALSE;}
static void network_game_server_port_cooperative_won(char const *map){CHECK(!map,"must retain current map");transitions++;}
static void network_game_client_local_player_quit(short controller){CHECK(controller==quits,"quit local players in order");quits++;}
static void ui_widgets_close_all(void){closed++;}
static long tag_loaded(long kind,char const *name){(void)kind;(void)name;return tag_present?1:NONE;}
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#include "under_test.inc"
int main(int argc,char **argv){CHECK(argc==2,"case");char const *case_name=argv[1];
 CASE("host") {CHECK(coop_leave_game(&deleted),"leave succeeds");CHECK(transitions==1&&!quits&&!closed&&deleted,"host uses round transition only");}
 CASE("client") {host=FALSE;CHECK(coop_leave_game(&deleted),"leave succeeds");CHECK(!transitions&&quits==4&&closed==1&&deleted,"client only leaves local machine");}
 CASE("local") {coop=FALSE;CHECK(!coop_leave_game(&deleted),"not a coop action");CHECK(!transitions&&!quits&&!closed&&!deleted,"other modes untouched");}
 CASE("routes") {menu_tags.loaded=menu_loaded;char const *prefix="ui\\shell\\multiplayer_game\\pause_game\\";char name[128];
  for(int n=1;n<=4;n*=2){snprintf(name,sizeof(name),"%s%dp_pause_game",prefix,n);char expected[80];snprintf(expected,sizeof(expected),"pc\\pause\\coop\\%dp_pause_game",n);CHECK(!strcmp(pc_menus_screen(name),expected),"coop viewport route");}
  CHECK(!strcmp(pc_menus_screen("arbitrary screen"),"arbitrary screen"),"other screens untouched");
  CHECK(!strcmp(pc_menus_screen("ui\\shell\\main_menu\\multiplayer_type_select\\connected\\connected_map_select_postgame_wrapper"),"pc\\main_menu\\multiplayer_type_select\\lobby\\lobby_screen"),"host returns straight to lobby");
  coop=FALSE;CHECK(!strcmp(pc_menus_screen("ui\\shell\\main_menu\\multiplayer_type_select\\connected\\connected_map_select_postgame_wrapper"),"pc\\main_menu\\multiplayer_type_select\\lobby\\lobby_screen"),"session remains coop after ui map loads");session_coop=FALSE;CHECK(pc_menus_screen(name)==name,"multiplayer unchanged");coop=TRUE;tag_present=FALSE;CHECK(!strcmp(pc_menus_screen(name),CAMPAIGN_PAUSE_SCREEN),"missing assets fallback");tag_present=TRUE;menu_tags.loaded=FALSE;CHECK(!strcmp(pc_menus_screen(name),CAMPAIGN_PAUSE_SCREEN),"failed menus fallback");session_coop=TRUE;CHECK(!strcmp(pc_menus_screen("ui\\shell\\main_menu\\multiplayer_type_select\\connected\\connected_map_select_postgame_wrapper"),"ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen"),"stock menus host returns to lobby");
 }
 return 0;
}
