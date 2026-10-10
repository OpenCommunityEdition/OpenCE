/* Real MCC handlers with a deterministic local/remote player world. */
#include "cseries.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "memory/data.h"
#include "game/game.h"
#include "game/cheats.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "interface/hud.h"
#include "interface/hud_definitions.h"
#include "scenario/scenario_definitions.h"
#include "mcc_campaign.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef memset
#undef memcpy
#undef strcmp
#undef strlen

#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static boolean loaded=TRUE;
static short connection;
static long args[7],result,added[4];
static int count,draws;
static real_point3d drawn;
static struct player_datum players[3];
static struct unit_datum units[3];
static struct scenario scenario;
static struct scenario_cutscene_flag flags[2];
static struct hud_globals_definition hud;
static struct data_array player_array,list_array,syntax_array;
struct data_array *player_data=&player_array,*object_list_header_data=&list_array,*hs_syntax_data=&syntax_array;
struct hud_globals_definition *hud_globals=&hud;
real global_gravity=0.0035651792f;
real sound_gain_under_dialog=.7f;
struct cheat_globals cheat;
short const hs_external_global_count=3;
static struct hs_external_global_definition globals[3]={{"sound_gain_under_dialog",_hs_type_real,0,&sound_gain_under_dialog},
    {"cheat_infinite_ammo",_hs_type_boolean,0,&cheat.infinite_ammo},{"unsafe",_hs_type_real,0,NULL}};
struct hs_external_global_definition *hs_global_external_get(short index) {return &globals[index];}
void *csmemset(void *p,long c,unsigned long n) {return memset(p,c,n);}
void *csmemcpy(void *p,void const *q,unsigned long n) {return memcpy(p,q,n);}
long csstrcasecmp(char const *a,char const *b) {return _stricmp(a,b);}
long csstrcmp(char const *a,char const *b) {return strcmp(a,b);}
unsigned long csstrlen(char const *s) {return (unsigned long)strlen(s);}
void error(short p,char const *f,...) {(void)p;(void)f;}
boolean mcc_cache_tags_loaded(void) {return loaded;}
short game_connection(void) {return connection;}
struct scenario *global_scenario_get(void) {return &scenario;}
long local_player_get_player_index(short local) {return local==0 ? 0 : local==1 ? 2 : NONE;}
void *datum_try_and_get(struct data_array *data,long index) {
    if(data==&player_array)return index>=0&&index<3 ? players+index : NULL;
    if(data==&list_array)return index==8 ? &list_array : NULL;
    return NULL;
}
void *object_try_and_get_and_verify_type(long index,unsigned long mask) {(void)mask;return index>=0&&index<3 ? units+index : NULL;}
void *object_get_and_verify_type(long index,unsigned long mask) {return object_try_and_get_and_verify_type(index,mask);}
real_point3d *object_get_origin(long index,real_point3d *p) {p->x=(real)index*4;p->y=p->z=0;return p;}
void unit_get_head_position(long index,real_point3d *p) {object_get_origin(index,p);}
short hud_get_nav_point_render_type(short local,real_point3d const *h,real_point3d const *p,long ref) {
    (void)local;(void)h;(void)p;(void)ref;return 0;
}
void custom_render_nav_point(short local,real_point3d const *p,short arrow,short type) {
    (void)local;(void)arrow;(void)type;draws++;drawn=*p;
}
long object_list_new(void) {count=0;return 8;}
void object_list_add(long list,long object) {(void)list;added[count++]=object;}
long object_list_get_first(long list,long *cursor) {(void)list;*cursor=0;return 0;}
long object_list_get_next(long list,long *cursor) {(void)list;return ++*cursor==1 ? 2 : NONE;}
boolean hs_macro_function_parse(short f,long x) {(void)f;(void)x;return TRUE;}
boolean hs_parse(long x,short type) {(void)x;(void)type;return TRUE;}
long *hs_macro_function_evaluate(short f,long t,boolean initialize) {(void)f;(void)t;return initialize ? args : NULL;}
void hs_return(long thread,long value) {(void)thread;result=value;}
void mcc_sleep_forever_evaluate(short f,long t,boolean i) {(void)f;(void)t;(void)i;}
static long bits(real n) {union {real n;long b;} u;u.n=n;return u.b;}
static real number(long b) {union {real n;long b;} u;u.b=b;return u.n;}
static void call(char const *name) {
    short id=mcc_campaign_find(name);
    struct hs_function_definition *definition=mcc_campaign_function(id);
    if(!definition)exit(3);
    definition->evaluate(id,0,TRUE);
}
int main(int argc,char **argv) {
    unsigned char saved[MCC_CAMPAIGN_SNAPSHOT_BYTES],bad[MCC_CAMPAIGN_SNAPSHOT_BYTES];
    real original=global_gravity;
    int i;
    CHECK(argc==2);
    for(i=0;i<3;i++){players[i].unit_index=i;players[i].team_index=1;units[i].object.bounding_sphere_center.x=(real)i*4;}
    hud.waypoint.arrows.count=2;hud.waypoint.arrow_bitmap.index=1;
    scenario.cutscene_flags.address=flags;scenario.cutscene_flags.count=2;
    flags[1].position.x=10;
    mcc_campaign_begin();
    if(!strcmp(argv[1],"players")) {
        call("local_players");CHECK(result==8&&count==2&&added[0]==0&&added[1]==2);
        units[2].object.damage_flags=1u<<_object_dead_bit;
        call("local_players");CHECK(count==1&&added[0]==0);
        players[0].unit_index=NONE;call("local_players");CHECK(count==0);
        connection=_game_connection_local;call("game_is_authoritative");CHECK(result);
        connection=_game_connection_network_server;call("game_is_authoritative");CHECK(result);
        connection=_game_connection_network_client;call("game_is_authoritative");CHECK(!result);
    } else if(!strcmp(argv[1],"distance")) {
        args[0]=8;args[1]=bits(5);args[2]=args[3]=bits(0);call("objects_distance_to_position");CHECK(number(result)==3);
        args[0]=NONE;call("objects_distance_to_position");CHECK(number(result)==-1);
        args[0]=8;args[1]=0x7fc00000;call("objects_distance_to_position");CHECK(number(result)==-1);
    } else if(!strcmp(argv[1],"gravity")) {
        args[0]=bits(.25f);call("physics_set_gravity");CHECK(global_gravity==original*.25f);
        mcc_campaign_snapshot(saved);
        call("physics_constants_reset");CHECK(global_gravity==original);
        CHECK(mcc_campaign_restore(saved,sizeof(saved)));CHECK(global_gravity==original*.25f);
        args[0]=0x7fc00000;call("physics_set_gravity");CHECK(global_gravity==original*.25f);
        connection=_game_connection_network_client;args[0]=bits(2);call("physics_set_gravity");CHECK(global_gravity==original*.25f);
        mcc_campaign_begin();CHECK(global_gravity==original);
        connection=_game_connection_local;args[0]=bits(2);call("physics_set_gravity");mcc_campaign_dispose();CHECK(global_gravity==original);
        loaded=FALSE;CHECK(!mcc_campaign_restore(saved,sizeof(saved)));
        mcc_campaign_dispose();CHECK(global_gravity==original);
    } else if(!strcmp(argv[1],"controls")) {
        sound_gain_under_dialog=.35f;cheat.infinite_ammo=TRUE;mcc_campaign_snapshot(saved);
        sound_gain_under_dialog=.88f;cheat.infinite_ammo=FALSE;
        CHECK(mcc_campaign_restore(saved,sizeof(saved)) && sound_gain_under_dialog==.35f && cheat.infinite_ammo);
        connection=_game_connection_network_client;cheat.infinite_ammo=FALSE;
        CHECK(mcc_campaign_restore(saved,sizeof(saved)) && !cheat.infinite_ammo);
        mcc_campaign_dispose();CHECK(sound_gain_under_dialog==.7f && !cheat.infinite_ammo);
    } else if(!strcmp(argv[1],"navigation")) {
        call("breadcrumbs_nav_points_active");CHECK(result==TRUE);
        args[0]=0;args[1]=1;args[2]=bits(1);args[3]=bits(2);args[4]=bits(3);args[5]=(long)"test";args[6]=bits(.5f);
        call("breadcrumbs_activate_team_nav_point_position");mcc_campaign_render(0);CHECK(draws==1&&drawn.x==1&&drawn.z==3.5f);
        mcc_campaign_snapshot(saved);
        args[0]=1;args[1]=(long)"test";call("breadcrumbs_deactivate_team_nav_point");draws=0;mcc_campaign_render(0);CHECK(!draws);
        CHECK(mcc_campaign_restore(saved,sizeof(saved)));mcc_campaign_render(1);CHECK(draws==1);
        players[2].team_index=2;draws=0;mcc_campaign_render(1);CHECK(!draws);
        mcc_campaign_begin();args[0]=0;args[1]=1;args[2]=1;args[3]=bits(1);
        call("breadcrumbs_activate_team_nav_point_flag");mcc_campaign_render(0);CHECK(draws==1&&drawn.x==10&&drawn.z==1);
        args[0]=1;args[1]=1;call("breadcrumbs_deactivate_team_nav_point_flag");draws=0;mcc_campaign_render(0);CHECK(!draws);
        args[0]=0;args[1]=1;args[2]=2;args[3]=bits(0);call("breadcrumbs_activate_team_nav_point_object");
        mcc_campaign_render(0);CHECK(draws==1&&drawn.x==8);
        units[2].object.damage_flags=1u<<_object_dead_bit;draws=0;mcc_campaign_render(0);CHECK(!draws);
        units[2].object.damage_flags=0;args[0]=1;args[1]=2;call("breadcrumbs_deactivate_team_nav_point_object");mcc_campaign_render(0);CHECK(!draws);
        loaded=FALSE;mcc_campaign_render(0);CHECK(!draws);
    } else if(!strcmp(argv[1],"validation")) {
        mcc_campaign_snapshot(saved);CHECK(mcc_campaign_validate(saved,sizeof(saved)));
        CHECK(!mcc_campaign_validate(saved,sizeof(saved)-1));
        memcpy(bad,saved,sizeof(saved));bad[0]=2;CHECK(!mcc_campaign_restore(bad,sizeof(bad)));
        memcpy(bad,saved,sizeof(saved));bad[12]=2;CHECK(!mcc_campaign_restore(bad,sizeof(bad)));
        memcpy(bad,saved,sizeof(saved));{long nan=0x7fc00000;memcpy(bad+4,&nan,4);}CHECK(!mcc_campaign_restore(bad,sizeof(bad)));
        memcpy(bad,saved,sizeof(saved));bad[16]=4;CHECK(!mcc_campaign_restore(bad,sizeof(bad)));
        CHECK(global_gravity==original);
        CHECK(mcc_campaign_global_settable(0));CHECK(mcc_campaign_global_settable(1));
        CHECK(!mcc_campaign_global_settable(2));CHECK(!mcc_campaign_global_settable(3));CHECK(!mcc_campaign_global_settable(-1));
        loaded=FALSE;CHECK(!mcc_campaign_global_settable(0));
    } else return 2;
    mcc_campaign_dispose();return 0;
}
