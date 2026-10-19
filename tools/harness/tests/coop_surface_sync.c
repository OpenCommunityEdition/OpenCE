#include "harness.h"
#include <math.h>
typedef unsigned short word;
#define MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO 16
#define MAXIMUM_BREAKABLE_SURFACES_PER_MAP 256
#define MAXIMUM_QUEUED_EVENTS 128
#define MIN(a,b) ((a)<(b)?(a):(b))
#define csmemset memset
#define _error_silent 0
#define error(...) ((void)0)
#define BIT_VECTOR_TEST_FLAG(vector,bit) ((((byte *)(vector))[(bit)/8]>>((bit)%8))&1)
#define BIT_VECTOR_SET_FLAG(vector,bit,on) do {if(on)((byte *)(vector))[(bit)/8]|=(byte)(1u<<((bit)%8));else ((byte *)(vector))[(bit)/8]&=(byte)~(1u<<((bit)%8));}while(0)
#include "types.inc"
struct structure_bsp {struct {long count;struct structure_breakable_surface *data;} breakable_surfaces;};
struct damage_data {real_point3d epicenter;int location;};
static struct structure_breakable_surface surface[256];
static struct structure_bsp bsp={{64,surface}};
static struct breakable_surface_globals host,client,*globals=&host;
static struct breakable_surface_datum breakable_surface_scratch;
static byte breakable_surface_flags_scratch[32];
static boolean warned_about_breakable_surfaces;
static short global_structure_bsp_index;
static int effects,stopped;
static boolean is_host=TRUE,is_client;
static long tick=50;
static int queue_resets;
static struct {boolean refresh;} host_resend={TRUE};
static boolean host_surfaces_refresh;
static short host_surfaces_next,host_surfaces_bsp=NONE;
static struct {
 struct distributed_coop_event events[MAXIMUM_QUEUED_EVENTS];
 short sends[MAXIMUM_QUEUED_EVENTS],count;
 word next_number,applied_number;boolean applied_any;
} coop_events;
static struct {struct {long count;} cutscene_chapter_titles;} scenario;
#define global_structure_bsp_get() (&bsp)
#define global_scenario_get() (&scenario)
#define TAG_BLOCK_GET_ELEMENT(block,index,type) (&(block)->data[index])
#define scenario_location_from_point(location,point) ((void)0)
static void breakable_surface_effect(short index,struct damage_data const *damage,long seed){(void)index;(void)damage;(void)seed;effects++;}
static boolean coop_host(void){return is_host;}
static boolean coop_client(void){return is_client;}
static long game_time_get(void){return tick;}
static void game_time_set_distributed(long value){tick=value;}
static void update_queues_reset_and_fill_with_lies(void){queue_resets++;}
static void game_state_port_restamp_revert_time(void){}
static void hs_runtime_port_shift_sleep_times(long ticks){(void)ticks;}
static void coop_enemies_reset(void){}
static boolean distributed_real_valid(real value){return isfinite(value);}
#define client_apply_sound(event) ((void)0)
#define client_apply_hud(event) ((void)0)
#define client_apply_player_effect(event) ((void)0)
#define client_apply_nav_point(event) ((void)0)
#define client_apply_unit_animation(event) ((void)0)
#define client_apply_scenery_animation(event) ((void)0)
#define client_apply_effect(event) ((void)0)
#define client_apply_attach(event) ((void)0)
#define cinematic_set_title_delayed(...) ((void)0)
#define _object_mask_unit 1
#define distributed_object_index_valid(index) FALSE
#define network_objects_client_has(index) FALSE
#define object_try_and_get_and_verify_type(...) FALSE
#define unit_open(index) ((void)0)
#define unit_close(index) ((void)0)
static void client_stop_script_sounds(void){stopped++;}
static void network_coop_note_surface_broken(short index,real_point3d const *epicenter){(void)index;(void)epicenter;}
#include "under_test.inc"
static void select_client(void){globals=&client;is_host=FALSE;is_client=TRUE;}
static void select_host(void){globals=&host;is_host=TRUE;is_client=FALSE;}
static void setup(void)
{
 memset(&host,0,sizeof(host));memset(&client,0,sizeof(client));
 memset(host.breakable_surface_flags,0xff,sizeof(host.breakable_surface_flags));
 memset(client.breakable_surface_flags,0xff,sizeof(client.breakable_surface_flags));
 for(int i=0;i<256;i++)host.breakable_surfaces[0][i].vitality=client.breakable_surfaces[0][i].vitality=1;
 memset(&coop_events,0,sizeof(coop_events));global_structure_bsp_index=0;effects=stopped=0;
 host_resend.refresh=TRUE;host_surfaces_refresh=FALSE;host_surfaces_next=0;host_surfaces_bsp=NONE;
 bsp.breakable_surfaces.count=64;tick=50;queue_resets=0;select_host();
}
static void set_extant(struct breakable_surface_globals *state,int index,boolean extant)
{
 BIT_VECTOR_SET_FLAG(state->breakable_surface_flags[0],index,extant);
 state->breakable_surfaces[0][index].vitality=extant?1:0;
}
static void deliver(void)
{
 struct distributed_coop_event events[MAXIMUM_QUEUED_EVENTS];int count=coop_events.count;
 memcpy(events,coop_events.events,count*sizeof(events[0]));coop_events.count=0;
 select_client();network_coop_handle_events(events,(short)count);select_host();
}
int main(int argc,char **argv)
{
 char const *case_name=argc>1?argv[1]:"";setup();
 CASE("rewind") {
  /* Pane0 was already broken at checkpoint; pane1 broke after it. */
  set_extant(&host,0,FALSE);set_extant(&client,0,FALSE);set_extant(&client,1,FALSE);
  host_resend.refresh=FALSE;network_coop_reverted(100);
  CHECK(tick==100&&queue_resets==1,"revert lost monotonic clock/input reset");
  host_send_broken_surfaces();deliver();select_client();
  CHECK(!breakable_surface_extant(0),"checkpoint's broken pane restored");
  CHECK(breakable_surface_extant(1),"newly broken pane did not restore");
  CHECK(breakable_surface_get(1)->vitality==1,"restored pane has dead vitality");
  CHECK(BIT_VECTOR_TEST_FLAG(breakable_surface_flags_get(),1),"collision flag stayed broken");
  CHECK(effects==0,"restoration generated a break effect");
  select_host();set_extant(&client,1,FALSE);network_coop_reverted(200);host_send_broken_surfaces();deliver();
  select_client();CHECK(breakable_surface_extant(1),"second rewind failed");
 } else CASE("ordering") {
  struct distributed_coop_event reordered[2];
  set_extant(&client,1,FALSE);set_extant(&client,33,FALSE);host_send_broken_surfaces();
  memcpy(reordered,coop_events.events,sizeof(reordered));coop_events.count=0;
  select_client();network_coop_handle_events(&reordered[1],1);network_coop_handle_events(&reordered[0],1);
  CHECK(!breakable_surface_extant(1)&&breakable_surface_extant(33),"reordered older event was not discarded");
  select_host();host_send_broken_surfaces();
  struct distributed_coop_event stale={.kind=_coop_event_surface_broken,.number=0,.frame=0,.value=1};
  deliver();select_client();network_coop_handle_events(&stale,1);
  CHECK(breakable_surface_extant(1)&&breakable_surface_extant(33),"periodic repair/stale event ordering failed");
  stale.number=(word)(coop_events.applied_number+1);network_coop_handle_events(&stale,1);
  CHECK(!breakable_surface_extant(1),"new instantaneous break ignored");
 } else CASE("repair") {
  set_extant(&host,31,FALSE);set_extant(&host,32,FALSE);set_extant(&client,1,FALSE);
  host_send_broken_surfaces();coop_events.count=0; /* Drop one complete snapshot. */
  host_send_broken_surfaces();deliver();select_client();
  CHECK(breakable_surface_extant(1)&&!breakable_surface_extant(31)&&!breakable_surface_extant(32),"periodic snapshot did not repair lost state");
  CHECK(effects==2,"broken transitions should each emit one effect");
  select_host();host_send_broken_surfaces();deliver();CHECK(effects==2,"unchanged broken panes emitted effects again");
 } else CASE("full") {
  coop_events.count=MAXIMUM_QUEUED_EVENTS;host_resend.refresh=FALSE;host_surfaces_refresh=TRUE;
  host_send_broken_surfaces();CHECK(host_surfaces_refresh,"full event queue lost pending restore");
  coop_events.count=0;host_send_broken_surfaces();
  CHECK(coop_events.count==2&&!host_surfaces_refresh,"pending restore did not retry completely");
  coop_events.count=MAXIMUM_QUEUED_EVENTS-1;host_surfaces_refresh=TRUE;host_send_broken_surfaces();
  CHECK(coop_events.count==MAXIMUM_QUEUED_EVENTS&&host_surfaces_refresh,"partial snapshot lost retry");
  struct distributed_coop_event first=coop_events.events[MAXIMUM_QUEUED_EVENTS-1];
  CHECK(first.value==0,"partial snapshot did not start at first chunk");
  memset(client.breakable_surface_flags[0],0,8);
  select_client();network_coop_handle_events(&first,1);select_host();
  coop_events.count=0;host_send_broken_surfaces();
  CHECK(coop_events.count==1&&!host_surfaces_refresh,"partial snapshot did not resume remaining chunk");
  CHECK(coop_events.events[0].value==32,"partial snapshot restarted instead of continuing at32");
  deliver();select_client();
  for(int index=0;index<64;index++)CHECK(breakable_surface_extant(index),"partial snapshot omitted surface%d",index);
 } else CASE("starvation") {
  bsp.breakable_surfaces.count=256;host_resend.refresh=FALSE;host_surfaces_refresh=TRUE;
  memset(client.breakable_surface_flags[0],0,32);
  for(int chunk=0;chunk<8;chunk++){
   /* Other co-op traffic leaves exactly one slot on every call. */
   coop_events.count=MAXIMUM_QUEUED_EVENTS-1;host_send_broken_surfaces();
   CHECK(coop_events.count==MAXIMUM_QUEUED_EVENTS,"available event slot unused");
   struct distributed_coop_event event=coop_events.events[MAXIMUM_QUEUED_EVENTS-1];
   CHECK(event.value==32*chunk,"starved chunk%d: restarted at%d",chunk,event.value);
   select_client();network_coop_handle_events(&event,1);select_host();
   CHECK(host_surfaces_refresh==(chunk!=7),"pass completion lost under one-slot capacity");
  }
  select_client();for(int index=0;index<256;index++)CHECK(breakable_surface_extant(index),"chunk progress omitted surface%d",index);
 } else CASE("restart") {
  bsp.breakable_surfaces.count=256;host_resend.refresh=FALSE;host_surfaces_refresh=TRUE;
  for(int chunk=0;chunk<3;chunk++){
   coop_events.count=MAXIMUM_QUEUED_EVENTS-1;host_send_broken_surfaces();
   CHECK(coop_events.events[MAXIMUM_QUEUED_EVENTS-1].value==32*chunk,"retry cursor failed before reset");
  }
  network_coop_reverted(100);coop_events.count=MAXIMUM_QUEUED_EVENTS-1;host_send_broken_surfaces();
  CHECK(coop_events.events[MAXIMUM_QUEUED_EVENTS-1].value==0,"revert did not restart pass");
  global_structure_bsp_index=1;coop_events.count=MAXIMUM_QUEUED_EVENTS-1;host_send_broken_surfaces();
  CHECK(coop_events.events[MAXIMUM_QUEUED_EVENTS-1].value==0&&coop_events.events[MAXIMUM_QUEUED_EVENTS-1].frame==1,"BSP change did not restart pass");
 } else CASE("bounds") {
  struct distributed_coop_event event={.kind=_coop_event_surface_broken,.type=1,.frame=0,.value=0,.object_index=0};
  select_client();event.frame=1;client_apply_event(&event);CHECK(breakable_surface_extant(0),"wrong BSP applied");
  event.frame=0;event.value=-1;client_apply_event(&event);CHECK(breakable_surface_extant(0),"negative chunk applied");
  event.value=1;client_apply_event(&event);CHECK(breakable_surface_extant(0),"unaligned chunk applied");
  event.value=256;client_apply_event(&event);CHECK(breakable_surface_extant(0),"past-global chunk applied");
  event.value=64;client_apply_event(&event);CHECK(breakable_surface_extant(0),"past-BSP chunk applied");
  event.value=0;event.type=2;client_apply_event(&event);CHECK(breakable_surface_extant(0),"unknown type applied");
  global_structure_bsp_index=NONE;event.frame=NONE;event.type=1;client_apply_event(&event);
  CHECK(client.breakable_surfaces[0][0].vitality==1,"missing BSP mutated state");
  global_structure_bsp_index=0;event.frame=0;event.value=32;bsp.breakable_surfaces.count=33;client_apply_event(&event);
  CHECK(!breakable_surface_extant(32)&&breakable_surface_extant(33),"partial chunk escaped BSP's surface count");
 } else CASE("authority") {
  struct distributed_coop_event event={.kind=_coop_event_surface_broken,.type=1,.frame=0,.value=0,.object_index=0};
  network_coop_handle_events(&event,1);CHECK(breakable_surface_extant(0),"host accepted client's state handler");
 } else return 2;
 return 0;
}
