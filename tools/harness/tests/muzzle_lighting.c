#include "harness.h"
#define PIN(x,a,b) ((x)<(a)?(a):(x)>(b)?(b):(x))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
typedef struct { real red,green,blue; } real_rgb_color;
typedef struct { real_point3d position; real_vector3d forward; } real_matrix4x3;
enum { _object_invisible_bit, _point_light_port_glow_bit, _point_light_connected_to_map_bit, MAXIMUM_WEAPONS_PER_UNIT=4 };
struct weapon_datum { long definition_index; struct { long parent_object_index; int flags; struct { int size; } node_matrices; } object; };
struct unit_datum { struct { short current_weapon_index; } unit; };
struct object_marker { real_matrix4x3 matrix; };
struct observer_result { real_point3d position; };
static struct weapon_datum weapons[21];
static struct unit_datum unit;
static struct observer_result camera;
static const char *tag="weapons\\assault rifle\\assault rifle";
static int on=1,present=1,held=1,marker=1,world_marker,allocs,deletes,disconnects,submits,fail,missing;
static long tick=30;
static real render_time=1.0f, marker_x=0.5f;
static real_vector3d forward={1,0,0};
static const real_vector3d *global_forward3d=&forward;
struct light_datum { long object_index,definition_index; int flags; real_point3d position; real_rgb_color color; real radius; int active; };
static struct light_datum lights[64];
static struct { int valid; } data={1},*light_data=&data;
static long light_port_glow_definition=99;
int lighting_config_boolean(const char *n) { return on; }
static long game_time_get(void) { return tick; }
real render_interpolation_game_time_sec(long t) { return render_time; }
static struct weapon_datum *weapon_try_and_get(long w) { return present && w>=0 && w<12 ? &weapons[w] : NULL; }
static struct weapon_datum *object_get(long w) { return &weapons[w]; }
static struct unit_datum *unit_try_and_get(long u) { return present && u==20 ? &unit : NULL; }
static const char *tag_get_name(long d) { return tag; }
static long unit_inventory_get_weapon(long u,short s) { return held; }
static int first_person_weapon_adjust_light(long w,const char *n,real_point3d *p,real_vector3d *f,real_vector3d *u) { p->x=marker_x;p->y=p->z=0;*f=forward;return marker; }
static int object_get_marker_by_name(long w,const char *n,struct object_marker *m,short c) { m->matrix.forward=forward;m->matrix.position.x=!strcmp(n,"cannon flash")?4:marker_x;m->matrix.position.y=m->matrix.position.z=0;return world_marker; }
static short local_player_get_next(short p) { return p==NONE?0:NONE; }
static const struct observer_result *observer_get_camera(short p) { return &camera; }
static real distance3d(const real_point3d *a,const real_point3d *b) { return a->x-b->x; }
long light_port_glow_definition_get(void) { return missing?NONE:99; }
static struct light_datum *datum_try_and_get(void *a,long i) { return i>=0 && i<allocs && lights[i].active?&lights[i]:NULL; }
static long light_new_unattached(long d,long o,short n,const real_point3d *p,const real_vector3d *f,real r)
{
 int i=allocs; if(fail)return NONE;allocs++;
 lights[i].active=1;lights[i].object_index=o;lights[i].definition_index=d;lights[i].position=*p;
 lights[i].flags=FLAG(_point_light_connected_to_map_bit);return i;
}
boolean light_port_glow_set(long i,long o,const real_rgb_color *c,real r)
{
 struct light_datum *l=datum_try_and_get(light_data,i);
 if(!l || l->object_index!=o || l->definition_index!=99)return FALSE;
 l->color=*c;l->radius=r;l->flags|=FLAG(_point_light_port_glow_bit);return TRUE;
}
static void light_delete(long i) { lights[i].active=0;deletes++; }
static void light_disconnect_from_map(long i) { CHECK(TEST_FLAG(lights[i].flags,_point_light_connected_to_map_bit),"disconnect once");lights[i].flags&=~FLAG(_point_light_connected_to_map_bit);disconnects++; }
static void light_reconnect_to_map(long i) { CHECK(!TEST_FLAG(lights[i].flags,_point_light_connected_to_map_bit),"reconnect once");lights[i].flags|=FLAG(_point_light_connected_to_map_bit); }
void plasma_occlusion_submit(long l,long r) { submits++; }
#include "under_test.inc"
int main(int argc,char **argv)
{
 int i; real old; long light; const char *name=argv[1];
 for(i=0;i<12;i++) { weapons[i].object.parent_object_index=20;weapons[i].object.node_matrices.size=sizeof(real_matrix4x3); }
 if(!strcmp(name,"pulse"))
 {
  muzzle_lighting_shot(1,0);CHECK(!allocs,"shot is cosmetic bookkeeping only");muzzle_lighting_update();
  CHECK(allocs==1 && lights[0].color.red==1,"initial full pulse");
  muzzle_lighting_submitted(0,3);CHECK(muzzle_lighting_selected(3) && submits==1,"selected and occluded");
  muzzle_lighting_frame_end();CHECK(!lights[0].color.red && !TEST_FLAG(lights[0].flags,_point_light_connected_to_map_bit),"dark and disconnected during simulation");
  render_time=1.025f;marker_x=0.75f;muzzle_lighting_update();old=lights[0].color.red;
  CHECK(old>0 && old<1 && lights[0].position.x==0.75f,"subtick fade and rendered marker motion");
  muzzle_lighting_shot(1,0);muzzle_lighting_update();CHECK(lights[0].color.red==old && allocs==1,"same tick does not duplicate or restart");
  render_time=1.05f;muzzle_lighting_update();CHECK(lights[0].color.red<old,"continuous decay");
  tick++;muzzle_lighting_shot(1,0);muzzle_lighting_update();CHECK(lights[0].color.red==1 && allocs==1,"next shot refreshes bounded light");
  render_time=1.2f;tick+=5;muzzle_lighting_update();CHECK(deletes==1,"expired light released");
 }
 else if(!strcmp(name,"profiles"))
 {
  for(i=0;i<NUMBEROF(muzzle_profiles);i++) { tag=muzzle_profiles[i].tag;tick++;muzzle_lighting_shot(1,muzzle_profiles[i].triggers==2?1:0);CHECK(pulses[0].active,"supported weapon");muzzle_lighting_forget(); }
  tag="weapons\\plasma pistol\\plasma pistol";tick++;muzzle_lighting_shot(1,1);CHECK(pulses[0].radius==2.2f && pulses[0].duration==0.16f,"charged profile");
  muzzle_lighting_forget();tag="custom\\weapon";muzzle_lighting_shot(1,0);CHECK(!pulses[0].active,"unknown stays stock");
 }
 else if(!strcmp(name,"off"))
 {
  on=0;muzzle_lighting_shot(1,0);muzzle_lighting_update();CHECK(!allocs,"default off allocates nothing");
  on=1;muzzle_lighting_shot(1,0);muzzle_lighting_update();muzzle_lighting_frame_end();muzzle_lighting_frame_end();
  CHECK(disconnects==2,"cleanup idempotent including initial move");
  on=0;muzzle_lighting_frame_end();CHECK(deletes==1,"disable releases light");on=1;muzzle_lighting_update();CHECK(allocs==1,"enable cannot replay old shot");
 }
 else if(!strcmp(name,"capacity"))
 {
  for(i=0;i<12;i++)muzzle_lighting_shot(i,0);
  CHECK(pulses[7].weapon==7,"first eight admitted");
  held=7;muzzle_lighting_update();CHECK(allocs==1,"invalid/inactive weapons do not consume lights");
 }
 else if(!strcmp(name,"lifecycle"))
 {
  muzzle_lighting_shot(1,0);muzzle_lighting_update();held=2;muzzle_lighting_update();CHECK(deletes==1,"switch releases pulse");
  held=1;tick++;muzzle_lighting_shot(1,0);muzzle_lighting_update();present=0;muzzle_lighting_update();CHECK(deletes==2,"deleted owner releases pulse");
  present=1;tick++;muzzle_lighting_shot(1,0);muzzle_lighting_update();light=pulses[0].light;lights[light].definition_index=98;
  on=0;muzzle_lighting_frame_end();CHECK(deletes==2,"stale light not deleted");on=1;tick++;muzzle_lighting_shot(1,0);tick+=3;muzzle_lighting_update();CHECK(allocs==3,"old undrawn event discarded");
  muzzle_lighting_shot(1,0);muzzle_lighting_forget();muzzle_lighting_update();CHECK(allocs==3,"map reset discards pending events");
 }
 else if(!strcmp(name,"failures"))
 {
  fail=1;muzzle_lighting_shot(1,0);muzzle_lighting_update();CHECK(!allocs && !pulses[0].active,"allocation failure stock fallback");
  fail=0;missing=1;tick++;muzzle_lighting_shot(1,0);muzzle_lighting_update();CHECK(!allocs,"missing template fallback");
  missing=0;marker=0;tick++;muzzle_lighting_shot(1,0);muzzle_lighting_update();CHECK(!allocs,"missing marker fallback");
 }
 else if(!strcmp(name,"world"))
 {
  marker=0;world_marker=1;marker_x=4;
  muzzle_lighting_shot(1,0);muzzle_lighting_update();
  CHECK(allocs==1 && lights[0].position.x==4,"world-model muzzle fallback");
  muzzle_lighting_frame_end();marker_x=26;muzzle_lighting_update();
  CHECK(deletes==1,"distant muzzle is released");
  marker_x=4;weapons[1].object.node_matrices.size=0;tick++;
  muzzle_lighting_shot(1,0);muzzle_lighting_update();
  CHECK(allocs==1,"missing world pose stays stock");
 }
 else if(!strcmp(name,"warthog"))
 {
  tag="vehicles\\warthog\\warthog gun";marker=0;world_marker=1;
  weapons[1].object.flags=FLAG(_object_invisible_bit);weapons[1].object.node_matrices.size=0;
  weapons[20].object.node_matrices.size=sizeof(real_matrix4x3);
  muzzle_lighting_shot(1,0);muzzle_lighting_update();
  CHECK(muzzle_lighting_vehicle_occlusion(pulses[0].light),"turret includes vehicle blockers");
  CHECK(allocs==1 && lights[0].radius==5.0f && lights[0].color.green==0.75f,"wide warm turret pulse uses vehicle marker");
  muzzle_lighting_frame_end();marker_x=1;render_time+=1.0f/120;muzzle_lighting_update();
  CHECK(lights[0].position.x==1 && lights[0].color.red<1,"turret follows rendered aim with smooth fade");
  world_marker=0;muzzle_lighting_update();CHECK(deletes==1,"missing vehicle marker releases pulse");
 }
 else if(!strcmp(name,"scorpion"))
 {
  tag="vehicles\\scorpion\\scorpion cannon";marker=0;world_marker=1;marker_x=2;
  weapons[1].object.flags=FLAG(_object_invisible_bit);weapons[1].object.node_matrices.size=0;
  weapons[20].object.node_matrices.size=sizeof(real_matrix4x3);
  muzzle_lighting_shot(1,0);muzzle_lighting_shot(1,1);muzzle_lighting_update();
  CHECK(allocs==2&&pulses[0].trigger==0&&pulses[1].trigger==1,"simultaneous tank guns retain separate pulses");
  CHECK(lights[0].radius==7&&lights[1].radius==5,"cannon has wider spill than coaxial gun");
  CHECK(lights[0].position.x==4&&lights[1].position.x>2.079f&&lights[1].position.x<2.081f,"distinct rendered sources beyond barrels");
  CHECK(muzzle_lighting_vehicle_occlusion(0)&&muzzle_lighting_vehicle_occlusion(1),"both guns include chassis blockers");
  muzzle_lighting_frame_end();render_time=1.05f;tick++;muzzle_lighting_update();old=lights[0].color.red;
  muzzle_lighting_shot(1,1);muzzle_lighting_update();
  CHECK(lights[0].color.red==old&&lights[1].color.red==1&&allocs==2,"machinegun does not restart or replace cannon pulse");
  muzzle_lighting_shot(1,1);render_time=1.08f;muzzle_lighting_update();
  CHECK(lights[1].color.red<1,"duplicate shot does not postpone machinegun decay");
  render_time=1.18f;tick+=4;muzzle_lighting_update();
  CHECK(lights[0].active&&!lights[1].active,"each trigger expires on its own envelope");
  on=0;muzzle_lighting_update();CHECK(deletes==2,"disable releases both tank lights");
 }
 else if(!strcmp(name,"move"))
 {
  real_point3d p={3,2,1};muzzle_lighting_shot(1,0);muzzle_lighting_update();muzzle_lighting_frame_end();
  CHECK(light_port_glow_move(0,&p) && lights[0].position.x==3,"move disconnected glow");
  CHECK(muzzle_lighting_radius(0,0.2f)==1.8f,"maximum bounds cover own radius");
  CHECK(muzzle_lighting_radius(25,0.2f)==0.2f,"unowned bounds untouched");
  lights[0].object_index=1;CHECK(!light_port_glow_move(0,&p),"attached light cannot be moved");
  lights[0].object_index=NONE;light_data->valid=0;CHECK(!light_port_glow_move(0,&p),"invalid array cannot be moved");
 }
 return 0;
}
