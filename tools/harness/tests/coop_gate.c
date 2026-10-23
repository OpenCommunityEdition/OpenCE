#include "harness.h"
#include <string.h>

typedef unsigned short word;
#define SET_FLAG(f,b,v) ((f)=(v)?(f)|(1U<<(b)):(f)&~(1U<<(b)))
#define MAX_PLAYERS 128
#include "list_header.inc"
static struct object_list_header_datum headers[8];
static int used[8], fail_allocate;
static struct {long value,next;} refs[2048];
static int references;
static void *object_list_header_data,*object_list_data;
#define object_list_header_get(i) (&headers[(i)])
#define datum_new gate_datum_new
static long gate_datum_new(void *data){int i;if(fail_allocate)return NONE;for(i=0;i<8;i++)if(!used[i]){used[i]=1;return i;}return NONE;}
static void reference_list_add(void *data,long *head,long object){long p=references++;CHECK(p<2048,"fixture references");refs[p].value=object;refs[p].next=*head;*head=p;}
static long object_list_get_first(long list,long *ref){if(list==NONE)return NONE;*ref=headers[list].first_reference_index;if(*ref==NONE)return NONE;long v=refs[*ref].value;*ref=refs[*ref].next;return v;}
static long object_list_get_next(long list,long *ref){if(*ref==NONE)return NONE;long v=refs[*ref].value;*ref=refs[*ref].next;return v;}
static short object_list_count(long list){return list==NONE?0:headers[list].count;}
#include "lists.inc"

static int host=TRUE,waiting=TRUE,ignore_players,teleports,rollbacks,failures,bad_place,crowded,side_effects;
static int player_count;
struct data_iterator {int index;};
struct player_datum {long unit_index;};
static struct player_datum people[MAX_PLAYERS];
static struct {real_point3d position;} objects[MAX_PLAYERS+1];
static void *player_data;
static long data_next_index(void *data,long previous){return previous+1<player_count?previous+1:NONE;}
static struct player_datum *player_get(long i){CHECK(i>=0&&i<player_count,"invalid player");return &people[i];}
static boolean coop_scripts_host(void){return host;}
static long player_index_from_unit_index(long index){int i;for(i=0;i<player_count;i++)if(people[i].unit_index==index)return i;return NONE;}
static real_point3d *object_get_origin(long i,real_point3d *p){*p=objects[i].position;return p;}
static void object_set_position(long i,real_point3d const *p,void *forward,void *up){objects[i].position=*p;rollbacks++;}
static boolean scenario_trigger_volume_test_object(short volume,long i){return objects[i].position.x>10&&objects[i].position.x<20;}
static boolean player_position_switches_bsp(real_point3d const *p){return p->y>50;}
static void biped_fix_ignore_players(boolean value){ignore_players=value;}
static boolean player_teleport(long p,long anchor,real_point3d const *point){
 long u=people[p].unit_index;teleports++;CHECK(u!=NONE,"teleport dead player");
 if(failures){if(side_effects)objects[u].position.x=25;return FALSE;}
 if(crowded&&!ignore_players)return FALSE;
 objects[u].position=*point;
 if(bad_place==1)objects[u].position.x=25;
 if(bad_place==2)objects[u].position.y=100;
 return TRUE;
}
static boolean hs_runtime_waiting_on_call(void){return waiting;}
#include "players.inc"
#include "gather.inc"
/* The original volume implementation is compiled too, including ALL of an
empty list; provenance must preserve that behavior for AI lists. */
#define MAXIMUM_TRIGGER_VOLUMES_PER_SCENARIO 256
#define VALID_INDEX(i,n) ((unsigned)(i)<(n))
#define BIT_VECTOR_SET_FLAG(bits,i,value) ((void)0)
#define _error_silent 0
#define error(...) ((void)0)
#include "volume.inc"
#include "gate.inc"
static void reset(int count){int i;memset(used,0,sizeof(used));memset(headers,0,sizeof(headers));memset(objects,0,sizeof(objects));references=0;fail_allocate=0;host=waiting=TRUE;ignore_players=teleports=rollbacks=failures=bad_place=crowded=side_effects=0;player_count=count;for(i=0;i<count;i++){people[i].unit_index=i;objects[i].position.x=i?1:15;}}
static boolean gate(long list){boolean r=hs_trigger_volume_test_objects_all(107,list);CHECK(!ignore_players,"collision override leaked");return r;}
static int all_inside(void){int i;for(i=0;i<player_count;i++)if(people[i].unit_index!=NONE&&!scenario_trigger_volume_test_object(107,people[i].unit_index))return FALSE;return TRUE;}
int main(int argc,char **argv){char const *name=argc>1?argv[1]:"";long list;int n;
 CHECK(sizeof(struct object_list_header_datum)==12,"checkpoint ABI changed");
 if(!strcmp(name,"crowds")||!strcmp(name,"refused")||!strcmp(name,"outside")||!strcmp(name,"bsp-result")){
  for(n=2;n<=128;n*=2){reset(n);list=hs_players();failures=!strcmp(name,"refused");bad_place=!strcmp(name,"outside")?1:!strcmp(name,"bsp-result")?2:0;crowded=!strcmp(name,"crowds");side_effects=TRUE;
   CHECK(gate(list)==(!failures&&!bad_place),"unsafe result for %d players",n);
   CHECK(all_inside()==(!failures&&!bad_place),"incorrect final positions");
   CHECK(teleports<=2*(n-1),"unbounded placement search");
   if(failures||bad_place){for(int i=1;i<n;i++)CHECK(objects[i].position.x==1&&objects[i].position.y==0,"bad placement not rolled back");CHECK(rollbacks==2*(n-1),"missing rollback");failures=bad_place=0;CHECK(gate(list)&&all_inside(),"failed gate did not retry");}
  }
 }else if(!strcmp(name,"one-player")){
  reset(1);list=hs_players();CHECK(gate(list)&&!teleports,"one-player host changed");objects[0].position.x=1;CHECK(!gate(list)&&!teleports,"one player outside passed gate");
 }else if(!strcmp(name,"empty")){
  reset(4);for(n=0;n<4;n++)people[n].unit_index=NONE;
  list=hs_players();CHECK(object_list_is_players(list),"empty players lost provenance");CHECK(!gate(list)&&!teleports,"empty players advanced gate");
  long ai=object_list_new();CHECK(gate(ai)&&!teleports,"empty AI semantics changed");
  waiting=FALSE;CHECK(gate(list),"non-waiting empty ALL semantics changed");
  waiting=TRUE;host=FALSE;CHECK(gate(list),"local/client empty ALL semantics changed");
 }else if(!strcmp(name,"allocation")){
  reset(2);fail_allocate=1;list=hs_players();CHECK(list==NONE&&!gate(list)&&!teleports,"allocation failure opened gate");
 }else if(!strcmp(name,"lifetime")){
  reset(0);list=hs_players();CHECK(object_list_is_players(list),"mark failed");used[list]=0;
  long recycled=object_list_new();CHECK(recycled==list&&!object_list_is_players(recycled),"provenance survived slot reuse");
  object_list_mark_players(recycled);object_list_add(recycled,MAX_PLAYERS);CHECK(!object_list_is_players(recycled),"changed list retained provenance");
 }else if(!strcmp(name,"contexts")){
  reset(2);list=hs_players();waiting=FALSE;CHECK(gate(list)&&!teleports,"if/not test moved players or lost ANY semantics");
  waiting=TRUE;host=FALSE;CHECK(!gate(list)&&!teleports,"non-host gathered players");
  objects[1].position.x=15;CHECK(gate(list),"non-host ALL changed");
 }else if(!strcmp(name,"mixed")){
  reset(2);list=hs_players();object_list_add(list,MAX_PLAYERS);CHECK(!gate(list)&&!teleports,"mixed AI/player list gathered");
 }else if(!strcmp(name,"no-arrival")){
  reset(3);objects[0].position.x=1;list=hs_players();CHECK(!gate(list)&&!teleports,"gathered with nobody inside");
 }else if(!strcmp(name,"bsp-anchor")){
  reset(3);objects[0].position.y=100;list=hs_players();CHECK(!gate(list)&&!teleports,"used a loading-zone anchor");
  objects[1].position.x=15;CHECK(gate(list)&&all_inside(),"did not choose a safe second anchor");
 }else return 2;
 return 0;
}
