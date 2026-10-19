#include "harness.h"
#include <limits.h>
#include <math.h>

#define SHORT_MAX 32767
#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS 128
#define MAXIMUM_OBJECTS_PER_MAP 8192
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(x,a,b) MIN(MAX(x,a),b)
#define VALID_INDEX(x,n) ((x)>=0&&(x)<(n))
#define _pi 3.14159265358979323846f
#define csmemcpy memcpy
#define _game_connection_network_server 2
enum { _object_type_biped, _object_type_vehicle, _object_type_weapon, _object_type_equipment,
       _object_type_scenery, _object_type_machine, _object_type_projectile };
#define _object_mask_weapon FLAG(_object_type_weapon)
#define _object_mask_equipment FLAG(_object_type_equipment)
#define _object_mask_projectile FLAG(_object_type_projectile)
#define _object_mask_sightblocking (FLAG(_object_type_vehicle)|FLAG(_object_type_scenery))
#define _object_mask_device FLAG(_object_type_machine)
#define _object_mask_all 0xffffffffUL
enum { _equipment_powerup_none, _equipment_powerup_double_speed, _equipment_powerup_overshield,
       _equipment_powerup_active_camouflage, _equipment_powerup_full_spectrum_vision,
       _equipment_powerup_health, _equipment_powerup_grenade };
enum { _item_attached_to_unit_bit, _item_belongs_to_player_bit, _item_has_nonzero_angular_velocity_bit,
       _item_on_structure_bit, _item_on_object_bit, _item_does_not_accelerate_bit, _item_part_of_respawn_system_bit };
#define _equipment_orient_to_ground_bit 5
enum { _object_connected_to_map_bit, _object_cannot_be_garbage_bit, _object_shadowless_bit, _object_outside_of_map_bit };
enum { _collision_test_structure_bit, _collision_test_objects_bit, _collision_test_front_facing_surfaces_bit,
       _collision_test_back_facing_surfaces_bit };
#define _collision_test_objects_sight_blocking_flags 0
struct location {long leaf_index; short cluster_index;};
struct tag_reference {long index;};
struct tag_block {long count; void *address;};
enum { _scenario_object_placement_not_automatic_bit };
struct scenario_object_datum {short palette_entry_index; unsigned short placement_flags;};
struct scenario_object_palette_entry {struct tag_reference reference;};
struct object_definition {struct {short type;} object;};
struct equipment_definition {struct {short type;} object; struct {short powerup_type;} equipment;};
struct weapon_magazine {short rounds_total,rounds_loaded;};
struct test_object {long definition_index; struct {
 unsigned long flags; real_point3d position,bounding_sphere_center; real bounding_sphere_radius;
 long parent_object_index; short type,forced_shader_permutation_index; real_vector3d forward,up; real base_change_colors[12];
 } object; struct {unsigned long flags;} item; struct {struct weapon_magazine magazines[2];} weapon;};
#define item_datum test_object
#define object_datum test_object
struct object_iterator {long index;};
struct object_placement_data {long definition_index; short variant_number; real_point3d position;
 real_vector3d forward,up; real change_colors[12];};
struct collision_result {real_point3d point; struct {real_vector3d n;} plane;};
struct collision_model_instance {int unused;};
struct network_game {int players[128];};
static struct test_object objects[1024];
static struct equipment_definition definitions[16];
static struct scenario_object_datum placements_data[32];
static struct scenario_object_palette_entry palette_data[16];
static struct tag_block placements={0,placements_data},palette={16,palette_data};
static struct {long actual_count;} headers,*object_header_data=&headers;
static struct network_game game;
static int used,connection=2,coop=1,setting=1,percent=100,multiplier=2,wall=0,gap=0,slope=0,crowded=0,allocation_fail=0,mesh_clear=0;
static real world_bound=20.f;
static char const *mode="per_player";
static char const *config_string(char const *name){(void)name;return setting?mode:"none";}
static long config_integer(char const *name){return !strcmp(name,"network.coop_enemies_multiplier")?multiplier:percent;}
static int game_connection(void){return connection;}
static int network_coop_active(void){return coop;}
static struct network_game *network_game_get_game(void){return &game;}
static int network_player_is_valid(int const *p){return *p;}
static struct object_definition *object_definition_get(long i){return (struct object_definition *)&definitions[i];}
static struct equipment_definition *equipment_definition_get(long i){return &definitions[i];}
static struct test_object *item_get(long i){return &objects[i];}
static struct test_object *item_try_and_get(long i){return i>=0&&i<used?&objects[i]:NULL;}
static struct test_object *object_get(long i){return &objects[i];}
static struct test_object *weapon_get(long i){return &objects[i];}
static void *global_scenario_get(void){return &placements;}
static struct tag_block *scenario_get_object_type_scenario_datums(void *s,short t,long *size)
{(void)s;(void)t;*size=sizeof(placements_data[0]);return &placements;}
static void *tag_block_get_element_with_size(struct tag_block *b,long i,long size){return (char *)b->address+i*size;}
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
static void vector_from_points3d(real_point3d const *a,real_point3d const *b,real_vector3d *v)
{v->i=b->x-a->x;v->j=b->y-a->y;v->k=b->z-a->z;}
static void scenario_location_from_point(struct location *l,real_point3d const *p)
{l->leaf_index=l->cluster_index=fabsf(p->x)<world_bound&&fabsf(p->y)<world_bound&&p->z>0?0:NONE;}
static short objects_in_sphere(int cls,unsigned long types,struct location const *l,real_point3d const *c,real r,long *out,unsigned max)
{(void)cls;(void)types;(void)l;(void)c;(void)r;unsigned n=0;if(crowded)return max;for(int i=0;i<used&&n<max;i++)out[n++]=i;return n;}
static int collision_model_instance_new(struct collision_model_instance *m,long i){(void)m;(void)i;return 1;}
static int collision_model_test_sphere(struct collision_model_instance const *m,real_point3d const *p,real r)
{(void)m;(void)p;(void)r;return !mesh_clear;}
static int collision_test_vector(unsigned long flags,real_point3d const *p,real_vector3d const *v,long ignore,struct collision_result *c)
{(void)flags;(void)ignore;
 if(wall&&((p->x<.3f&&p->x+v->i>=.3f)||(p->x>.3f&&p->x+v->i<=.3f)))return 1;
 if(v->k<-.5f&&!gap&&fabsf(p->x)<world_bound&&fabsf(p->y)<world_bound){c->point=(real_point3d){p->x,p->y,0};c->plane.n=(real_vector3d){0,0,slope?.5f:1};return 1;}
 return 0;}
static int collision_test_sphere(real_point3d const *p,real r,long ignore)
{(void)ignore;return p->z-r<0||fabsf(p->x)+r>=world_bound||fabsf(p->y)+r>=world_bound||(wall&&fabsf(p->x-.3f)<r);}
static void object_placement_data_new(struct object_placement_data *p,long d,long owner)
{(void)owner;memset(p,0,sizeof(*p));p->definition_index=d;}
static long object_new(struct object_placement_data const *p)
{if(allocation_fail)return NONE;int i=used++;objects[i].definition_index=p->definition_index;
 objects[i].object.type=definitions[p->definition_index].object.type;
 objects[i].object.position=objects[i].object.bounding_sphere_center=p->position;
 objects[i].object.bounding_sphere_radius=.2f;objects[i].object.parent_object_index=NONE;
 headers.actual_count++;return i;}
static void object_iterator_new(struct object_iterator *i,unsigned long mask,int flags){(void)mask;(void)flags;i->index=-1;}
static void *object_iterator_next(struct object_iterator *i){return ++i->index<used?&objects[i->index]:NULL;}

#include "under_test.inc"

static void setup(short type)
{
 memset(objects,0,sizeof(objects));memset(&game,0,sizeof(game));
 used=1;headers.actual_count=1;objects[0].definition_index=0;objects[0].object.type=type;
 objects[0].object.parent_object_index=NONE;objects[0].object.bounding_sphere_radius=.2f;
 objects[0].object.position=objects[0].object.bounding_sphere_center=(real_point3d){0,0,.24f};
 objects[0].object.flags=FLAG(_object_connected_to_map_bit)|FLAG(_object_cannot_be_garbage_bit);
 definitions[0].object.type=type;placements.count=1;placements_data[0].palette_entry_index=0;palette_data[0].reference.index=0;
 game.players[0]=game.players[1]=1;coop_pickups_new_game();
}

int main(int argc,char **argv)
{
 const char *case_name=argc>1?argv[1]:"";
 setup(_object_type_weapon);
 CASE("counts") {
  for(int p=1;p<=128;p++)for(int pct=25;pct<=200;pct+=25){
   long sum=0;for(int n=1;n<=2000;n++){
    short extra=pickups_extra_count(n,p,pct,0);sum+=extra;
    long expected=MIN(((long)n*pct*(p-1)+50)/100,(long)n*7);
    CHECK(extra>=0&&extra<=7&&sum==expected,"players %d percent %d N %d: %ld/%ld",p,pct,n,sum,expected);
   }
  }
  CHECK(pickups_extra_count(0,4,100,0)==0&&pickups_extra_count(1,1,100,0)==0,"invalid/single player");
  CHECK(pickups_extra_count(32767,128,200,0)==7,"large ordinal cap");
  CHECK(coop_scaling_growth(12,50)==550&&coop_scaling_growth(128,200)==700,"growth and cap");
  for(int m=2;m<=32;m*=2)for(int p=1;p<=128;p++){
   CHECK(pickups_extra_count(1,p,50,m)==MIN(m,8)-1,"static count depends on players/cap");
   mode="multiplier";multiplier=m;setup(_object_type_weapon);coop_pickups_new_game();
   memset(&game,0,sizeof(game));for(int i=0;i<p;i++)game.players[i]=1;
   coop_pickups_register(0,placements_data,&palette);
   CHECK(((objects[0].item.flags&PICKUPS_COUNT_MASK)>>PICKUPS_COUNT_SHIFT)==MIN(m,8)-1,"static registration");
   coop_pickups_update();CHECK(used==MIN(m,8),"static spawn count/cap");
   coop_pickups_update();CHECK(used==MIN(m,8),"static copies recursively scaled");
  }
  mode="invalid";coop_pickups_new_game();CHECK(!pickups_enabled,"unknown mode enabled");
 } else CASE("eligibility") {
  CHECK(pickups_definition_allowed(0),"weapon excluded");definitions[0].object.type=_object_type_equipment;
  for(int p=0;p<=6;p++){definitions[0].equipment.powerup_type=p;
   CHECK(pickups_definition_allowed(0)==(p==2||p==3||p==5||p==6),"powerup filter %d",p);}
  definitions[0].object.type=_object_type_vehicle;CHECK(!pickups_definition_allowed(0),"vehicle included");
  setup(_object_type_weapon);connection=1;coop_pickups_register(0,placements_data,&palette);CHECK(!objects[0].item.flags,"client scaled");
  connection=2;coop=0;coop_pickups_register(0,placements_data,&palette);CHECK(!objects[0].item.flags,"singleplayer scaled");
  coop=1;setting=0;coop_pickups_new_game();coop_pickups_register(0,placements_data,&palette);CHECK(!objects[0].item.flags,"disabled scaled");
 } else CASE("health-kits") {
  for(int p=1;p<=128;p++)for(int pct=50;pct<=200;pct+=150){
   percent=pct;setup(_object_type_equipment);
   definitions[0].equipment.powerup_type=_equipment_powerup_health;
   objects[0].item.flags=FLAG(_equipment_orient_to_ground_bit);
   memset(&game,0,sizeof(game));for(int i=0;i<p;i++)game.players[i]=1;
   coop_pickups_register(0,placements_data,&palette);coop_pickups_update();
   int expected=1+MIN((pct*(p-1)+50)/100,7);
   CHECK(used==expected,"health kits: players %d percent %d: %d/%d",p,pct,used,expected);
   CHECK(objects[0].object.position.x==0&&objects[0].object.position.y==0,"original health kit moved");
   for(int i=1;i<used;i++){
    CHECK(objects[i].definition_index==0&&objects[i].object.type==_object_type_equipment,"health kit definition changed");
    CHECK((objects[i].item.flags&PICKUPS_COPY_FLAG)&&!(objects[i].item.flags&PICKUPS_COUNT_MASK),"health copy can multiply");
    CHECK(TEST_FLAG(objects[i].item.flags,_equipment_orient_to_ground_bit),"health kit lost ground orientation");
    CHECK(!collision_test_sphere(&objects[i].object.bounding_sphere_center,.2f,NONE),"health kit clips map");
    for(int j=0;j<i;j++){real_vector3d d;vector_from_points3d(&objects[j].object.position,&objects[i].object.position,&d);
     CHECK(d.i*d.i+d.j*d.j+d.k*d.k>.4f*.4f,"health kits overlap");}
   }
   coop_pickups_update();CHECK(used==expected,"health kits recursively scaled");
  }
  /* Health kits use the same no-floor/no-space fallback as other equipment. */
  percent=100;
  setup(_object_type_equipment);definitions[0].equipment.powerup_type=_equipment_powerup_health;
  gap=1;coop_pickups_register(0,placements_data,&palette);coop_pickups_update();CHECK(used==1,"health kit spawned over void");gap=0;
  setup(_object_type_equipment);definitions[0].equipment.powerup_type=_equipment_powerup_health;
  crowded=1;coop_pickups_register(0,placements_data,&palette);coop_pickups_update();CHECK(used==1,"health kit spawned in full space");
 } else CASE("floor-and-walls") {
  real_point3d p;CHECK(pickups_position(0,&objects[0].object.position,.2f,0,&p),"open floor rejected");
  gap=1;CHECK(!pickups_position(0,&objects[0].object.position,.2f,0,&p),"void accepted");gap=0;
  slope=1;CHECK(!pickups_position(0,&objects[0].object.position,.2f,0,&p),"steep slope accepted");slope=0;
  wall=1;CHECK(!pickups_position(0,&objects[0].object.position,.2f,0,&p),"through wall");
  /* The ray stops short of the wall, but the whole pickup would clip it. */
  real_point3d origin={-.2f,0,.24f};
  CHECK(!pickups_position(0,&origin,.1f,0,&p),"bounding sphere clipping wall accepted");
  wall=0;world_bound=.65f;CHECK(!pickups_position(0,&objects[0].object.position,.2f,0,&p),"sphere crosses map boundary");
 } else CASE("occupied-and-full") {
  real_point3d p={.55f,0,.24f};objects[1]=objects[0];objects[1].object.position=objects[1].object.bounding_sphere_center=p;used=2;
  CHECK(pickups_occupied(&p,.2f,0),"overlapping pickup accepted");
  objects[1].object.type=_object_type_scenery;mesh_clear=1;CHECK(!pickups_occupied(&p,.2f,0),"empty mesh space rejected");
  crowded=1;CHECK(pickups_occupied(&p,.2f,0),"query overflow accepted");crowded=0;
  setup(_object_type_weapon);coop_pickups_register(0,placements_data,&palette);headers.actual_count=MAXIMUM_OBJECTS_PER_MAP-512;
  coop_pickups_update();CHECK(used==1,"reserved pool consumed");
  setup(_object_type_weapon);allocation_fail=1;coop_pickups_register(0,placements_data,&palette);coop_pickups_update();CHECK(used==1,"allocation failure");
 } else CASE("checkpoint-and-bsp") {
  coop_pickups_register(0,placements_data,&palette);unsigned long checkpoint=objects[0].item.flags;
  CHECK((checkpoint&PICKUPS_COUNT_MASK)!=0,"source not registered");objects[0].object.flags=0;
  coop_pickups_update();CHECK(used==1&&objects[0].item.flags==checkpoint,"inactive BSP spawned or lost count");
  objects[0].object.flags=FLAG(_object_connected_to_map_bit)|FLAG(_object_outside_of_map_bit);
  coop_pickups_update();CHECK(used==1&&objects[0].item.flags==checkpoint,"other BSP consumed budget");
  objects[0].object.flags=FLAG(_object_connected_to_map_bit);coop_pickups_update();CHECK(used==2,"active BSP failed");
  CHECK(!(objects[0].item.flags&PICKUPS_COUNT_MASK),"processed budget not cleared");
  coop_pickups_update();CHECK(used==2,"recursive copies");
  used=1;objects[0].item.flags=checkpoint;coop_pickups_update();CHECK(used==2,"pre-spawn checkpoint not replayed");
  coop_pickups_update();CHECK(used==2,"post-spawn checkpoint duplicated");
  placements_data[0].placement_flags=FLAG(_scenario_object_placement_not_automatic_bit);
  coop_pickups_register(0,placements_data,&palette);coop_pickups_update();CHECK(used==2,"script recreation exceeded placement budget");
 } else CASE("copies") {
  objects[0].weapon.magazines[0]=(struct weapon_magazine){17,5};
  objects[0].item.flags=FLAG(_item_does_not_accelerate_bit);for(int i=2;i<128;i++)game.players[i]=1;
  coop_pickups_register(0,placements_data,&palette);coop_pickups_update();CHECK(used==8,"8x total: %d",used);
  for(int i=1;i<used;i++){
   CHECK(!(objects[i].item.flags&PICKUPS_COUNT_MASK),"copy can multiply");
   CHECK(objects[i].weapon.magazines[0].rounds_total==17&&objects[i].weapon.magazines[0].rounds_loaded==5,"ammo lost");
   CHECK(!collision_test_sphere(&objects[i].object.bounding_sphere_center,.2f,NONE),"out of map/clipping");
   for(int j=0;j<i;j++){real_vector3d d;vector_from_points3d(&objects[j].object.position,&objects[i].object.position,&d);
    CHECK(d.i*d.i+d.j*d.j+d.k*d.k>.4f*.4f,"copies overlap");}
  }
 } else return 2;
 return 0;
}
